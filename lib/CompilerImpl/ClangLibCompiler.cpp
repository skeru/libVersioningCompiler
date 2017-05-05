/* Copyright 2017 Politecnico di Milano.
 * Developed by : Stefano Cherubin
 *                PhD student, Politecnico di Milano
 *                <first_name>.<family_name>@polimi.it
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
#include "versioningCompiler/CompilerImpl/ClangLibCompiler.hpp"
#include "versioningCompiler/CompilerImpl/ClangLLVM/FileLogDiagnosticConsumer.hpp"
#include "versioningCompiler/DebugUtils.hpp"

#include "clang/Driver/Driver.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Job.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/CodeGen/CodeGenAction.h"

// opt stuff
#include "versioningCompiler/CompilerImpl/ClangLLVM/OptUtils.hpp"

#include <vector>

using namespace vc;
using namespace clang;

// static mutex object initialization
std::mutex ClangLibCompiler::opt_parse_mtx;

// ----------------------------------------------------------------------------
// ----------------------- zero-parameters constructor ------------------------
// ----------------------------------------------------------------------------
ClangLibCompiler::ClangLibCompiler()
                : ClangLibCompiler(
                                   "ClangLibCompiler", // compilerID
                                   ".",                // libWorkingDir
                                   ""                  // no log file
                                  ) { }

// ----------------------------------------------------------------------------
// --------------------------- detailed constructor ---------------------------
// ----------------------------------------------------------------------------
ClangLibCompiler::ClangLibCompiler(
                                   const std::string &compilerID,
                                   const std::string &libWorkingDir,
                                   const std::string &log
                                  ) : Compiler(
                                               compilerID,
                                               "#", // compiler call string
                                               libWorkingDir,
                                               log, // log filename
                                               "~", // install directory
                                               true // support IR
                                              )
{
  // initialize LLVM stuff and shutdown everything at program tear down
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
std::string ClangLibCompiler::generateIR(const std::string &src,
                                         const std::string &func,
                                         const std::string &versionID,
                                         const opt_list_t options) const
{
  // What we want to generate
  const std::string &llvmIRfileName = Compiler::getBitcodeFileName(versionID);
  // what we return when generateIR fails
  const std::string &failureFileName = "";
  std::string log_str = "";

  auto report_error = [&](const std::string message) {
    std::string error_string = "ClangLibCompiler::generateIR";
    error_string = error_string + " ERROR during processing of version ";
    error_string = error_string + versionID;
    error_string = error_string + "\n\t";
    error_string = error_string + message;
    Compiler::log_string(error_string);
    return;
  };

  const std::string &command_filename = _llvmManager->getClangExePath();

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
  const auto& argv_owner = getArgV(options);
  std::vector<const char*> argv;
  argv.reserve(argv_owner.size());
  for (const auto& arg : argv_owner) {
    argv.push_back(arg.c_str());
  }
  cmd_str.insert(cmd_str.end(),
                 argv.begin(),
                 argv.end());
  cmd_str.push_back(src.c_str());

  // log the command line string used to create this task
  for (const auto& arg : cmd_str) {
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

  llvm::SmallVector<std::pair<int, const driver::Command*>,1> failCmd;
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

// ---------------------------------------------------------------------------
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
std::string ClangLibCompiler::runOptimizer(const std::string &src_IR,
                                           const std::string &versionID,
                                           const opt_list_t options) const
{
  // What we want to generate
  const std::string optBCfilename = Compiler::getOptBitcodeFileName(versionID);

  // what we return when generateIR fails
  const std::string failureFileName = "";

  auto report_error = [&](const std::string message) {
    std::string error_string = "ClangLibCompiler::runOptimizer";
    error_string = error_string + " ERROR during processing of version ";
    error_string = error_string + versionID;
    error_string = error_string + "\n\t";
    error_string = error_string + message;
    Compiler::log_string(error_string);
    return;
  };

  llvm::LLVMContext optContext;
  const std::vector<std::string>& argv_owner = getArgV(options);
  const size_t argc = argv_owner.size() + 1; // opt <options>
  std::vector<const char*> argv;
  std::string log_str = "opt ";
  argv.reserve(argc);
  argv.push_back(std::move("opt"));
  for (const auto& arg : argv_owner) {
    argv.push_back(arg.c_str());
    log_str = log_str + arg + " ";
  }
  Compiler::log_string(log_str);

  // command line options are static and should be accessed exclusively
  opt_parse_mtx.lock();

  resetOptOptions();
  llvm::cl::ParseCommandLineOptions(argc,
                                    const_cast<const char**>(argv.data()),
                                    "ClangLibCompiler::runOptimizer");

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
  llvm::TargetMachine* optTMachine = nullptr;
  const llvm::TargetOptions Options = InitTargetOptionsFromCodeGenFlags();
                                      // llvm static helper function
  std::string lookupError;
  if (moduleTriple.getArch()) {
    optCPUStr = getCPUStr();            // llvm static helper function
    optFeaturesStr = getFeaturesStr();  // llvm static helper function
  }
  const llvm::Target *TheTarget = TargetRegistry::lookupTarget(MArch,
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
                                                 getRelocModel(),
                                                 CMModel,
                                                 GetCodeGenOptLevel());
  }
  std::unique_ptr<TargetMachine> actualTM(optTMachine);
  // Override function attributes based on CPUStr, FeaturesStr,
  // and command line flags.
  setFunctionAttributes(optCPUStr, optFeaturesStr, *module);

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
      OptLevelO3)
  {
    FPasses.reset(new llvm::legacy::FunctionPassManager(module.get()));
    FPasses->add(llvm::createTargetTransformInfoWrapperPass(
      actualTM ? actualTM->getTargetIRAnalysis() : TargetIRAnalysis()));
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
    if (PassInf->getTargetMachineCtor()) {
      P = PassInf->getTargetMachineCtor()(actualTM.get());
    }
    else if (PassInf->getNormalCtor()) {
      P = PassInf->getNormalCtor()();
    }
    else {
      report_error("Cannot create pass: " +
                   std::string(PassInf->getPassName()));
    }
    if (P) {
      PassKind Kind = P->getPassKind();
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
    Passes.add(createVerifierPass());
  }

  std::error_code outFileCreationErrorCode;
  std::unique_ptr<llvm::tool_output_file> Out =
    llvm::make_unique<llvm::tool_output_file>(optBCfilename,
                                              outFileCreationErrorCode,
                                              llvm::sys::fs::F_None);
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
    BOS = llvm::make_unique<llvm::raw_svector_ostream>(Buffer);
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
      std::unique_ptr<llvm::Module> module2(llvm::CloneModule(module.get()));
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
// ------------------------------- generateBin -------------------------------
// ---------------------------------------------------------------------------
/** Generate binary shared object
 *
 * This implementation exploits the clang driver to handle all the stages of
 * the compilation and linking process.
 */
std::string ClangLibCompiler::generateBin(const std::string &src,
                                          const std::string &func,
                                          const std::string &versionID,
                                          const opt_list_t options) const
{
  // What we want to generate
  const std::string libFileName = Compiler::getSharedObjectFileName(versionID);
  // what we return when generateIR fails
  const std::string failureFileName = "";
  std::string log_str = "";

  auto report_error = [&](const std::string message) {
    std::string error_string = "ClangLibCompiler::generateBin";
    error_string = error_string + " ERROR during processing of version ";
    error_string = error_string + versionID;
    error_string = error_string + "\n\t";
    error_string = error_string + message;
    Compiler::log_string(error_string);
    return;
  };

  // clang++ <options> -fpic -shared src -olibFileName -Wno-return-type-c-linkage
  std::vector<const char *> cmd_str;
  cmd_str.reserve(options.size() + 6);
  cmd_str.push_back(std::move("clang++"));
  cmd_str.push_back(std::move("-fpic"));
  cmd_str.push_back(std::move("-shared"));
  cmd_str.push_back(std::move("-Wno-return-type-c-linkage"));
  const std::string outputArgument = "-o" + libFileName;
  cmd_str.push_back(std::move(outputArgument).c_str());

  // create a local copy of option strings
  const auto& argv_owner = getArgV(options);
  std::vector<const char*> argv;
  argv.reserve(argv_owner.size());
  for (const auto& arg : argv_owner) {
    argv.push_back(arg.c_str());
  }
  cmd_str.insert(cmd_str.end(),
                 argv.begin(),
                 argv.end());
  cmd_str.push_back(src.c_str());

  // log the command line string used to create this task
  for (const auto& arg : cmd_str) {
    log_str = log_str + arg + " ";
  }
  Compiler::log_string(log_str);

  driver::Driver NikiLauda(_llvmManager->getClangExePath(),
                           _llvmManager->getDefaultTriple()->str(),
                           *_diagEngine);
  NikiLauda.setTitle("clang as a library");
  NikiLauda.setCheckInputsExist(false);
  NikiLauda.CCPrintOptionsFilename = logFile.c_str();
  #ifdef VC_DEBUG
  NikiLauda.CCPrintOptions = true;
  #else
  NikiLauda.CCPrintOptions = false;
  #endif

  std::unique_ptr<driver::Compilation> C(NikiLauda.BuildCompilation(cmd_str));
  if (!C) {
    report_error("clang::driver::Compilation not created");
    return failureFileName;
  }

  llvm::SmallVector<std::pair<int, const driver::Command*>,1> failCmd;
  const auto res = NikiLauda.ExecuteCompilation(*C, failCmd);

  if (exists(libFileName)) {
    return libFileName;
  }
  const std::string &error_str = "Unknown error:"
                                " unable to generate shared object"
                                " - Driver error code: " + std::to_string(res);
  report_error(error_str);
  return failureFileName;
}

// ---------------------------------------------------------------------------
// ------------------------------ hasOptimizer ------------------------------
// ---------------------------------------------------------------------------
bool ClangLibCompiler::hasOptimizer() const
{
  return true;
}

// ---------------------------------------------------------------------------
// ----------------------------- getOptionString -----------------------------
// ---------------------------------------------------------------------------
inline std::string ClangLibCompiler::getOptionString(const Option &o) const
{
  // remove single and double quotes
  std::string tmp_val = o.getValue();
  if (tmp_val.length() > 1 &&
      (tmp_val[0] == '\"' && tmp_val[tmp_val.length() - 1] == '\"'))
  {
    tmp_val = tmp_val.substr(1, tmp_val.length() - 2);
  }
  return o.getPrefix() + tmp_val;
}

// ---------------------------------------------------------------------------
// --------------------------------- getArgV ---------------------------------
// ---------------------------------------------------------------------------
inline std::vector<std::string> ClangLibCompiler::getArgV(
                                const opt_list_t optionList) const
{
  std::vector<std::string> v;
  v.reserve(optionList.size());
  for (const auto& o : optionList) {
    v.push_back(getOptionString(o));
  }
  return v;
}
