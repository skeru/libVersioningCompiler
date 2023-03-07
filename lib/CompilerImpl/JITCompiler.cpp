/* Copyright 2017 Politecnico di Milano.
 * Developed by : Stefano Cherubin
 *                PhD student, Politecnico di Milano
 *                <first_name>.<family_name>@polimi.it
 *                Marco Festa
 *                Ms student, Politecnico di Milano
 *                <first_name>2.<family_name>@mail.polimi.it
 *                Nicole Gervasoni
 *                Ms student, Politecnico di Milano
 *                <first_name>annamaria.<family_name>@mail.polimi.it
 *                Moreno Giussani
 *                Ms student, Politecnico di Milano
 *                <first_name>.<family_name>@mail.polimi.it
 *
 * This file is part of libVersioningCompiler
 *
 * libVersioningCompiler is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * libVersioningCompiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libVersioningCompiler. If not, see <http://www.gnu.org/licenses/>
 */
#include "versioningCompiler/CompilerImpl/JITCompiler.hpp"
#include "versioningCompiler/CompilerImpl/ClangLLVM/FileLogDiagnosticConsumer.hpp"
#include "versioningCompiler/DebugUtils.hpp"

#include "clang/Driver/Driver.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Job.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/CodeGen/CodeGenAction.h"


#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/Diagnostic.h"

#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/Support/Error.h"
#include "llvm/Pass.h"
// opt stuff
#include "versioningCompiler/CompilerImpl/ClangLLVM/OptUtils.hpp"

#ifndef OPT_EXE_NAME
#define OPT_EXE_NAME "opt"
#endif
#include <vector>
#include <iostream>

using namespace vc;
using namespace clang;

// static mutex object initialization
std::mutex JITCompiler::opt_parse_mtx;

// ----------------------------------------------------------------------------
// --------------------------- detailed constructor ---------------------------
// ----------------------------------------------------------------------------
JITCompiler::JITCompiler(
        const std::string &compilerID,
        const std::string &libWorkingDir,
        const std::string &log //,
        //llvm::orc::JITTargetMachineBuilder targetMachineBuilder
) : Compiler(
        compilerID,
        "#", // compiler call string
        libWorkingDir,
        log, // log filename
        "~", // install directory
        true // support IR
),
    _ES(std::make_unique<llvm::orc::ExecutionSession>(std::move(*llvm::orc::SelfExecutorProcessControl::Create()))),
    _JTMB(_ES->getExecutorProcessControl().getTargetTriple()),
    _tsctx(std::make_unique<llvm::LLVMContext>()),
    _mangle(*_ES, this->_dataLayout),
    _dataLayout(*this->_JTMB.getDefaultDataLayoutForTarget()), _mainJD(_ES->createBareJITDylib("<main>")) {
      this->_mainJD.addGenerator(
        llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            this->_dataLayout.getGlobalPrefix())));
    
  std::cout << "Constructing compiler object.." << std::endl;
  _llvmManager = LLVMInstanceManager::getInstance();

  // custom diagnostic engine creation
  _diagnosticOptions = new clang::DiagnosticOptions();
  _diagnosticIDs = new clang::DiagnosticIDs();
  _diagConsumer = std::make_shared<vc::FileLogDiagnosticConsumer>(
          log,
          _diagnosticOptions.get());
  _diagEngine = new clang::DiagnosticsEngine(_diagnosticIDs,
                                             _diagnosticOptions.get(),
                                             _diagConsumer.get(),
                                             false);

  // initialize JIT objects
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);

  return;
}

// ---------------------------------------------------------------------------
// ------------------------------- generateIR --------------------------------
// ---------------------------------------------------------------------------
/** Generate LLVM-IR from a C/C++ source file.
 *
 * This implementation exploits the clang driver to handle all the stages of
 * the compilation process.
 */
std::string JITCompiler::generateIR(const std::vector<std::string> &src,
                                    const std::vector<std::string> &func,
                                    const std::string &versionID,
                                    const opt_list_t options) {
  // What we want to generate
  const std::string &llvmIRfileName = Compiler::getBitcodeFileName(versionID);
  // what we return when generateIR fails
  const std::string &failureFileName = "";
  std::string log_str = "";

  auto report_error = [&](const std::string message) {
      std::string error_string = "JITCompiler::generateIR";
      error_string = error_string + " ERROR during processing of version ";
      error_string = error_string + versionID;
      error_string = error_string + "\n\t";
      error_string = error_string + message;
      Compiler::log_string(error_string);
      return;
  };

  const std::string &command_filename = _llvmManager->getClangExePath();
  if(command_filename==""){
    report_error("Clang exe path is empty! Is the _llvmManager initialized?");
  }
  // clang++ <options> -fpic -shared src -olibFileName -Wno-return-type-c-linkage
  std::vector<const char *> cmd_str;
  cmd_str.reserve(options.size() + 6);
  cmd_str.push_back(command_filename.c_str());
  cmd_str.push_back(std::move("-c"));
  cmd_str.push_back(std::move("-emit-llvm"));
  cmd_str.push_back(std::move("-fpic"));
  cmd_str.push_back(std::move("-Wno-return-type-c-linkage"));
  const std::string outputArgument = "-o" + llvmIRfileName;
  cmd_str.push_back(std::move(outputArgument).c_str());

  // create a local copy of option strings
  const auto &argv_owner = getArgV(options);
  std::vector<const char *> argv;
  argv.reserve(argv_owner.size());
  for (const auto &arg : argv_owner) {
    argv.push_back(arg.c_str());
  }
  cmd_str.insert(cmd_str.end(),
                 argv.begin(),
                 argv.end());
  for (const auto& src_file : src) {
   cmd_str.push_back(src_file.c_str());
  }

  // log the command line string used to create this task
  for (const auto &arg : cmd_str) {
    log_str = log_str + arg + " ";
  }
  Compiler::log_string(log_str);

  driver::Driver NikiLauda(_llvmManager->getClangExePath(),
                           _llvmManager->getDefaultTriple()->str(),
                           *_diagEngine);
  NikiLauda.setTitle("clang as a library");
  NikiLauda.setCheckInputsExist(false);
  NikiLauda.CCPrintOptionsFilename = logFile.c_str();
  Compiler::lockMutex(logFile);
#ifdef VC_DEBUG
  NikiLauda.CCPrintOptions = true;
#else
  NikiLauda.CCPrintOptions = false;
#endif

  std::unique_ptr<driver::Compilation> C(NikiLauda.BuildCompilation(cmd_str));
  if (!C) {
    report_error("clang::driver::Compilation not created");
    Compiler::unlockMutex(logFile);
    return failureFileName;
  }

  llvm::SmallVector<std::pair<int, const driver::Command *>, 1> failCmd;
  const auto res = NikiLauda.ExecuteCompilation(*C, failCmd);
  Compiler::unlockMutex(logFile);

  if (exists(llvmIRfileName)) {
    return llvmIRfileName;
  }
  const std::string &error_str = "Unknown error:"
                                 " unable to generate bitcode file"
                                 " - Driver error code: " + std::to_string(res);
  report_error(error_str);
  return failureFileName;
}

//------------------------------------------------------------------------
// ------------------------------ runOptimizer -------------------------------
// ---------------------------------------------------------------------------
/** Run modular code optimizer on the LLVM-IR intermediate file.
 *
 * This implementation is strongly based on the LLVM tool `opt`.
 * LLVM tool `opt` is covered by the LLVM license.
 * See LLVM_LICENSE.txt for more details.
 *
 * If you aim at understanding this method, please consider taking a look at
 * ${LLVM_SOURCE_CODE}/tool/opt/opt.cpp for more details.
 * This implementation tries to reuse as much as possible LLVM/Clang APIs.
 * Unfortunatly there is no driver for accessing easily `opt`, like there is
 * for clang.
 * Therefore, this method is just an adapted copy of the original `opt`.
 * It supports all the options of `opt` but the ones related to output
 * management.
 * It is a constraint for this method to generate a well-formed output file.
 * If this method is not able to generate a well-formed optimized output file,
 * it will return an empty string as generated failureFileName.
 *
 * This method only supports the legacy pass manager.
 */
std::string JITCompiler::runOptimizer(const std::string &src_IR,
                                      const std::string &versionID,
                                      const opt_list_t options) const {
  // What we want to generate
  const std::string optBCfilename = Compiler::getOptBitcodeFileName(versionID);

  // what we return when generateIR fails
  const std::string failureFileName = "";

  auto report_error = [&](const std::string message) {
      std::string error_string = "JITCompiler::runOptimizer";
      error_string = error_string + " ERROR during processing of version ";
      error_string = error_string + versionID;
      error_string = error_string + "\n\t";
      error_string = error_string + message;
      Compiler::log_string(error_string);
      return;
  };

  llvm::LLVMContext optContext;
  const std::vector<std::string> &argv_owner = getArgV(options);
  const size_t argc = argv_owner.size() + 1; // opt <options>
  std::vector<const char *> argv;
  std::string log_str = OPT_EXE_NAME;
  log_str+=" ";
  argv.reserve(argc);
  argv.push_back(std::move(OPT_EXE_NAME));
  for (const auto &arg : argv_owner) {
    argv.push_back(arg.c_str());
    log_str = log_str + arg + " ";
  }
  Compiler::log_string(log_str);

  // command line options are static and should be accessed exclusively
  opt_parse_mtx.lock();

  resetOptOptions();
  llvm::cl::ParseCommandLineOptions(argc,
                                    const_cast<const char **>(argv.data()),
                                    "JITCompiler::runOptimizer");

  optContext.setDiscardValueNames(DiscardValueNames);
  if (!DisableDITypeMap) {
    optContext.enableDebugTypeODRUniquing();
  }

  // load llvm::Module
  llvm::SMDiagnostic parsingInputErrorCode;
  auto module = llvm::parseIRFile(src_IR, parsingInputErrorCode, optContext);
  if (!module) {
    std::string parsing_error_str;
    llvm::raw_string_ostream s_ostream(parsing_error_str);
    parsingInputErrorCode.print(argv[0], s_ostream);
    report_error("Unable to load module from source file\n" + s_ostream.str());
    if (!Compiler::exists(src_IR)) {
      report_error("Cannot find source file " + src_IR);
    }
    return failureFileName;
  }

  // Strip debug info before running the verifier.
  if (StripDebug) {
    llvm::StripDebugInfo(*module);
  }

  // Immediately run the verifier to catch any problems before starting up the
  // pass pipelines.  Otherwise we can crash on broken code during
  // doInitialization().
  if (!NoVerify && llvm::verifyModule(*module, &llvm::errs())) {
    report_error("Verifier check fail: input module is broken!\n");
    return failureFileName;
  }

  // If we are supposed to override the target triple, do so now.
  if (!TargetTriple.empty()) {
    module->setTargetTriple(llvm::Triple::normalize(TargetTriple));
  }

  // retrieve module triple and prepare Target Machine object (may be empty)
  llvm::Triple moduleTriple(module->getTargetTriple());
  std::string optCPUStr, optFeaturesStr;
  llvm::TargetMachine *optTMachine = nullptr;
  const llvm::TargetOptions Options = llvm::codegen::InitTargetOptionsFromCodeGenFlags(moduleTriple);
  // llvm static helper function
  std::string lookupError;
  if (moduleTriple.getArch()) {
    optCPUStr = llvm::codegen::getCPUStr();            // llvm static helper function
    optFeaturesStr = llvm::codegen::getFeaturesStr();  // llvm static helper function
  }
  const llvm::Target *TheTarget = llvm::TargetRegistry::lookupTarget(llvm::codegen::getMArch(),
                                                               moduleTriple,
                                                               lookupError);
  // Some modules don't specify a triple, and this is okay.
  if (!TheTarget) {
    optTMachine = nullptr;
  } else {
    optTMachine = TheTarget->createTargetMachine(moduleTriple.getTriple(),
                                                 optCPUStr,
                                                 optFeaturesStr,
                                                 Options,
                                                 llvm::codegen::getRelocModel(),
                                                 llvm::CodeModel::Small,// llvm::codegen::getCodeModel returns zero (casted to Tiny), which is wrong! Going with small which is the default model for majority of supported targets
                                                 GetCodeGenOptLevel());
  }
  std::unique_ptr<llvm::TargetMachine> actualTM(optTMachine);
  // Override function attributes based on CPUStr, FeaturesStr,
  // and command line flags.
  llvm::codegen::setFunctionAttributes(optCPUStr, optFeaturesStr, *module);

  // Create a PassManager to hold and optimize the collection of passes we are
  // about to build.
  llvm::legacy::PassManager Passes;

  // Add an appropriate TargetLibraryInfo pass for the module's triple.
  llvm::TargetLibraryInfoImpl TLII(moduleTriple);

  // The -disable-simplify-libcalls flag actually disables all builtin optzns.
  if (DisableSimplifyLibCalls) {
    TLII.disableAllFunctions();
  }
  Passes.add(new llvm::TargetLibraryInfoWrapperPass(TLII));

  // Add an appropriate DataLayout instance for this module.
  const llvm::DataLayout &DL = module->getDataLayout();
  if (DL.isDefault() && !DefaultDataLayout.empty()) {
    module->setDataLayout(DefaultDataLayout);
  }

  // Add internal analysis passes from the target machine.
  Passes.add(llvm::createTargetTransformInfoWrapperPass(
          (actualTM) ? actualTM->getTargetIRAnalysis() : llvm::TargetIRAnalysis()));

  std::unique_ptr<llvm::legacy::FunctionPassManager> FPasses;
  if (OptLevelO0 ||
      OptLevelO1 ||
      OptLevelO2 ||
      OptLevelOs ||
      OptLevelOz ||
      OptLevelO3) {
    FPasses.reset(new llvm::legacy::FunctionPassManager(module.get()));
    FPasses->add(llvm::createTargetTransformInfoWrapperPass(
            actualTM ? actualTM->getTargetIRAnalysis() : llvm::TargetIRAnalysis()));
  }

  // Create a new optimization pass for each one specified on the command line
  for (uint64_t i = 0; i < PassList.size(); ++i) {
    if (StandardLinkOpts &&
        StandardLinkOpts.getPosition() < PassList.getPosition(i)) {
      AddStandardLinkPasses(Passes);
      StandardLinkOpts = false;
    }
    if (OptLevelO0 && OptLevelO0.getPosition() < PassList.getPosition(i)) {
      AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 0, 0);
      OptLevelO0 = false;
    }
    if (OptLevelO1 && OptLevelO1.getPosition() < PassList.getPosition(i)) {
      AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 1, 0);
      OptLevelO1 = false;
    }
    if (OptLevelO2 && OptLevelO2.getPosition() < PassList.getPosition(i)) {
      AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 2, 0);
      OptLevelO2 = false;
    }
    if (OptLevelOs && OptLevelOs.getPosition() < PassList.getPosition(i)) {
      AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 2, 1);
      OptLevelOs = false;
    }
    if (OptLevelOz && OptLevelOz.getPosition() < PassList.getPosition(i)) {
      AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 2, 2);
      OptLevelOz = false;
    }
    if (OptLevelO3 && OptLevelO3.getPosition() < PassList.getPosition(i)) {
      AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 3, 0);
      OptLevelO3 = false;
    }

    const llvm::PassInfo *PassInf = PassList[i];
    llvm::Pass *P = nullptr;
    if (PassInf->getNormalCtor()) {
      P = PassInf->getNormalCtor()();
    } else {
      report_error("Cannot create pass: " +
                   std::string(PassInf->getPassName()));
    }
    if (P) {
      auto Kind = P->getPassKind();
      addPass(Passes, P);
    }
  } // end for each pass in PassList

  // now add standard presets of optimization passes
  if (StandardLinkOpts) {
    AddStandardLinkPasses(Passes);
    StandardLinkOpts = false;
  }
  if (OptLevelO0) {
    AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 0, 0);
  }
  if (OptLevelO1) {
    AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 1, 0);
  }
  if (OptLevelO2) {
    AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 2, 0);
  }
  if (OptLevelOs) {
    AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 2, 1);
  }
  if (OptLevelOz) {
    AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 2, 2);
  }
  if (OptLevelO3) {
    AddOptimizationPasses(Passes, *FPasses, actualTM.get(), 3, 0);
  }

  // run function passes
  if (FPasses) {
    FPasses->doInitialization();
    for (llvm::Function &F : *module) {
      FPasses->run(F);
    }
    FPasses->doFinalization();
  }

  // Check that the module is well formed on completion of optimization
  if (!NoVerify && !VerifyEach) {
    Passes.add(llvm::createVerifierPass());
  }

  std::error_code outFileCreationErrorCode;
  std::unique_ptr<llvm::ToolOutputFile> Out =
          std::make_unique<llvm::ToolOutputFile>(optBCfilename,
                                                  outFileCreationErrorCode,
                                                  llvm::sys::fs::OF_None);
  if (!Out) {
    report_error("Could not create output file");
    return failureFileName;
  }

  // In run twice mode, we want to make sure the output is bit-by-bit
  // equivalent if we run the pass manager again, so setup two buffers and
  // a stream to write to them.
  llvm::SmallVector<char, 0> Buffer;
  llvm::SmallVector<char, 0> CompileTwiceBuffer;
  std::unique_ptr<llvm::raw_svector_ostream> BOS;
  llvm::raw_ostream *OS = nullptr;

  // Write bitcode or assembly to the output as the last step...
  OS = &Out->os();
  if (RunTwice) {
    BOS = std::make_unique<llvm::raw_svector_ostream>(Buffer);
    OS = BOS.get();
  }
  if (OutputAssembly) {
    if (EmitSummaryIndex) {
      report_error("Text output is incompatible with -module-summary");
    }
    if (EmitModuleHash) {
      report_error("Text output is incompatible with -module-hash");
    }
    Passes.add(createPrintModulePass(*OS, "", PreserveAssemblyUseListOrder));
  } else if (OutputThinLTOBC) {
    Passes.add(createWriteThinLTOBitcodePass(*OS));
  } else {
    Passes.add(createBitcodeWriterPass(*OS,
                                       PreserveBitcodeUseListOrder,
                                       EmitSummaryIndex,
                                       EmitModuleHash));
  }

#ifdef VC_DEBUG
  // Before executing passes, print the final values of the LLVM options.
  llvm::cl::PrintOptionValues();
#endif


  // If requested, run all passes again with the same pass manager to catch
  // bugs caused by persistent state in the passes
  if (RunTwice) {
    std::unique_ptr<llvm::Module> module2(llvm::CloneModule(*module));
    Passes.run(*module2);
    CompileTwiceBuffer = Buffer;
    Buffer.clear();
  }

  // Now that we have all of the passes ready, run them.
  Passes.run(*module);

  // Compare the two outputs and make sure they're the same
  if (RunTwice) {
    if (Buffer.size() != CompileTwiceBuffer.size() ||
        (memcmp(Buffer.data(),
                CompileTwiceBuffer.data(),
                Buffer.size()) != 0)) {
      // running twice the same passes generated diverging bitcode versions
      const std::string error_messsage =
              "Running the pass manager twice changed the output.\n"
              "Writing the result of the second run to the specified output.\n"
              "To generate the one-run comparison binary, just run without\n"
              "the compile-twice option\n";
      report_error(error_messsage);
      Out->os() << BOS->str();
      Out->keep();
      if (Compiler::exists(optBCfilename)) {
        return optBCfilename;
      }
      return failureFileName;
    } // end if divengent bitcodes
    Out->os() << BOS->str();
  }

  opt_parse_mtx.unlock();

  // Declare success.
  Out->keep();
  if (Compiler::exists(optBCfilename)) {
    return optBCfilename;
  }
  return failureFileName;
}

// ---------------------------------------------------------------------------
// -------------------------------- addModule --------------------------------
// ---------------------------------------------------------------------------
/** addModule wrapper
 *
 * Makes the actual call to llvm CompileLayer addModule functions and saves the JIT compilation
 * results in the JITCompiler class state
 */
void JITCompiler::addModule(std::unique_ptr<llvm::Module> m, const std::string &versionID) {
  llvm::orc::ThreadSafeModule TSM(std::move(m),_tsctx);
  auto cm_iterator = _layer_map.find(versionID);

  if (cm_iterator == _layer_map.end()) {
    std::string error_string = "JITCompiler::addModule ";
    error_string = error_string + "compileLayer for " + versionID + " not found";
    Compiler::log_string(error_string);
    return;
  }
  _resource_tracker_map[versionID] = _mainJD.createResourceTracker();
  llvm::cantFail(cm_iterator->second->add(_resource_tracker_map[versionID],std::move(TSM)));
  return;
}

// ---------------------------------------------------------------------------
// ------------------------------- findSymbol --------------------------------
// ---------------------------------------------------------------------------
llvm::Expected<llvm::JITEvaluatedSymbol> JITCompiler::findSymbol(const std::string Name, const std::string &versionID) {
  llvm::StringRef llvmName(Name);
  return _ES->lookup({&_mainJD}, _mangle(llvmName.str()));
}

// ---------------------------------------------------------------------------
// ------------------------------- loadSymbol --------------------------------
// ---------------------------------------------------------------------------
/** Symbol Loading
 *
 * This implementation looks for the version saved module and adds it to the llvm
 * CompileLayer. It later resolves the symbol and returns it to its caller. This is
 * the function which actually performs the JIT compilation
 */


//  if (exists(bin)) {
//    *handler = dlopen(bin.c_str(), RTLD_NOW);
//  } else {
//    std::string error_str = "cannot load symbol from " + bin +
//                            " : file not found";
//    log_string(error_str);
//    return symbols;
//  }
//  if (*handler) {
//    for (const std::string& f : func) {
//      void *symbol = dlsym(*handler, f.c_str());
//      if (!symbol) {
//        std::string error_str = "cannot load symbol " + f + " from " + bin +
//        " : symbol not found";
//        log_string(error_str);
//      }
//      symbols.push_back(symbol);
//    } // end for
//  } else {
//    char* error = dlerror();
//    std::string error_str(error);
//    log_string(error_str);
//  }
//  return symbols;

std::vector<void*> JITCompiler::loadSymbols(const std::string &bin,
                               const std::vector<std::string> &func,
                               void ** handler) {

  std::vector<void *> symbols = {};

  // In JITCompiler implementation bin is used as a reference to our version ID
  const std::string versionID = bin;

  auto mm_iterator = _modules_map.find(versionID);
  if (mm_iterator == _modules_map.end()) {
    std::string error_string = "JITCompiler::loadSymbol ";
    error_string = error_string + "cannot load symbol from " + versionID + " - module not found";
    Compiler::log_string(error_string);
    return symbols;
  }
  *handler = new std::string(bin);
  // Calling a wrapper to the actual llvm addModule method
    auto report_error = [&](const std::string message) {
      std::string error_string = "JITCompiler::loadSymbols";
      error_string = error_string + " ERROR during processing of version ";
      error_string = error_string + versionID;
      error_string = error_string + "\n\t";
      error_string = error_string + message;
      Compiler::log_string(error_string);
      return;
  };
  llvm::SMDiagnostic parsing_input_error_code;
  addModule(std::move(llvm::parseIRFile(mm_iterator->second,parsing_input_error_code, *(_tsctx.getContext()))), bin);
  if (parsing_input_error_code.getMessage().size()>0){
    report_error(parsing_input_error_code.getMessage().str());
  }
    auto hm_iterator = _resource_tracker_map.find(versionID);
  if (hm_iterator == _resource_tracker_map.end()) {
    std::string error_string = "JITCompiler::loadSymbol ";
    error_string = error_string + "cannot load symbol from " + versionID + " - addModule call failed";
    Compiler::log_string(error_string);
    return symbols;
  }

  _isloaded_map[versionID] = true;

  for (const std::string& f : func) {
    auto findSym=findSymbol(f, versionID);
    llvm::cantFail(findSym.takeError());
    void *symbol = (void* )findSym->getAddress();
    if (!symbol) {
      std::string error_str = "cannot load symbol " + f + " from " + bin +
      " : symbol not found";
      Compiler::log_string(error_str);
    }
    symbols.push_back(symbol);
  } // end for

  return symbols;

}

// ---------------------------------------------------------------------------
// ------------------------------- releaseSymbol -----------------------------
// ---------------------------------------------------------------------------
/** Module release
 *
 * This implementation uses the handler argument to keep the internal JITCompiler
 * class state consistent cleaning the loaded symbols and removing the llvm Module
 * from the specific CompileLayer
 */
void JITCompiler::releaseSymbol(void **handler) {

  std::string *id = static_cast< std::string *>(*handler);

  auto map_rows_iterator = _isloaded_map.find(*id);
  if (map_rows_iterator == _isloaded_map.end()) {
    std::string error_string = "JITCompiler::releaseSymbol";
    error_string = error_string + " ERROR - module not found ";
    Compiler::log_string(error_string);
    return;
  } else {
    llvm::cantFail(_resource_tracker_map[*id]->remove()); // release all resources linked to this ResourceTrackerSP
    _resource_tracker_map.erase(*id);
    _isloaded_map.erase(*id);
  }
  delete(id);
  *handler = nullptr;
  return;
}

// ---------------------------------------------------------------------------
// ------------------------------- generateBin -------------------------------
// ---------------------------------------------------------------------------
/** JIT Compile the llvm module
 *
 * This implementation parses a previously generated (and eventually optimized) IR file
 * elevating it to its in-memory representation. This module is then saved in the JITCompiler
 * class instance state to be later added (by the loadSymbol method) to llvm's CompilerLayer
 */
std::string JITCompiler::generateBin(const std::vector<std::string> &src,
                                     const std::vector<std::string> &func,
                                     const std::string &versionID,
                                     const opt_list_t options) {

  // The source IR file name to JIT compile
  std::string source = src[0]; // if contains IR then it's just one element, otherwise will generate it

  _obj_map[versionID] = std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(*this->_ES,
                    []() { return std::make_unique<llvm::SectionMemoryManager>(); });
  _layer_map[versionID] = std::make_unique<llvm::orc::IRCompileLayer>(
          *this->_ES, 
          *(_obj_map[versionID]),
          std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(this->_JTMB)));

  // IR filename
  const std::string llvmIRfileName = Compiler::getBitcodeFileName(versionID);
  // IR optimized filename
  const std::string llvmOPTIRfileName = Compiler::getOptBitcodeFileName(versionID);
  // If IR files were not generated, generate it now but don't optimize it
  if (!exists(llvmOPTIRfileName) && !exists(llvmIRfileName)) {
    Compiler::log_string("IR file not found, generating from source..");

    source = generateIR(src, func, versionID, options);
  }


  // What we return when generateIR fails
  const std::string failureName = "";


  auto report_error = [&](const std::string message) {
      std::string error_string = "JITCompiler::generateBin";
      error_string = error_string + " ERROR during processing of version ";
      error_string = error_string + versionID;
      error_string = error_string + "\n\t";
      error_string = error_string + message;
      Compiler::log_string(error_string);
      return;
  };

  llvm::SMDiagnostic parsing_input_error_code;
  Compiler::log_string("Jitting IR file: " + source);

  _modules_map[versionID] = source; // this used to be std::move(llvm::parseIRFile(source, parsing_input_error_code, *(_tsctx.getContext())));
  
  auto m = std::move(llvm::parseIRFile(_modules_map[versionID], parsing_input_error_code, *(_tsctx.getContext())));
  if (parsing_input_error_code.getMessage().str().length() > 0) {
    report_error("Module was not generated: "+parsing_input_error_code.getMessage().str());
    return failureName;
  }
  addModule(std::move(m), versionID);
  return versionID;
}

// ---------------------------------------------------------------------------
// ------------------------------ hasOptimizer -------------------------------
// ---------------------------------------------------------------------------
bool JITCompiler::hasOptimizer() const {
  return true;
}

// ---------------------------------------------------------------------------
// ----------------------------- getOptionString -----------------------------
// ---------------------------------------------------------------------------
inline std::string JITCompiler::getOptionString(const Option &o) const {
  // remove single and double quotes
  std::string tmp_val = o.getValue();
  if (tmp_val.length() > 1 &&
      (tmp_val[0] == '\"' && tmp_val[tmp_val.length() - 1] == '\"')) {
    tmp_val = tmp_val.substr(1, tmp_val.length() - 2);
  }
  return o.getPrefix() + tmp_val;
}

// ---------------------------------------------------------------------------
// --------------------------------- getArgV ---------------------------------
// ---------------------------------------------------------------------------
inline std::vector<std::string> JITCompiler::getArgV(
        const opt_list_t optionList) const {
  std::vector<std::string> v;
  v.reserve(optionList.size());
  for (const auto &o : optionList) {
    v.push_back(getOptionString(o));
  }
  return v;
}


