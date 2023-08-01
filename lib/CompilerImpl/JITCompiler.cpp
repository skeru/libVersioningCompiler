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
#include "versioningCompiler/CompilerImpl/ClangLLVM/OptUtils.hpp" // opt stuff
#include "versioningCompiler/DebugUtils.hpp"

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Job.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/Error.h"
#include "llvm/Transforms/Utils/Cloning.h"

#ifndef OPT_EXE_FULLPATH
#define OPT_EXE_FULLPATH "/bin/opt"
#endif
#include <iostream>
#include <vector>

using namespace vc;
using namespace clang;

// static mutex object initialization
std::mutex JITCompiler::opt_parse_mtx;

// ----------------------------------------------------------------------------
// --------------------------- detailed constructor ---------------------------
// ----------------------------------------------------------------------------
JITCompiler::JITCompiler(const std::string &compilerID,
                         const std::filesystem::path &libWorkingDir,
                         const std::filesystem::path &log)
    : Compiler(compilerID,
               std::filesystem::u8path("#"), // compiler call string
               libWorkingDir,
               log,                          // log filename
               std::filesystem::u8path("~"), // install directory
               true                          // support IR
               ),
      _ES(std::make_unique<llvm::orc::ExecutionSession>(
          std::move(*llvm::orc::SelfExecutorProcessControl::Create()))),
      _JTMB(llvm::orc::JITTargetMachineBuilder::detectHost().get()),
      _tsctx(std::make_unique<llvm::LLVMContext>()),
      _mangle(*_ES, this->_dataLayout),
      _dataLayout(*this->_JTMB.getDefaultDataLayoutForTarget()),
      _mainJD(_ES->createBareJITDylib("<main>")) {
  this->_mainJD.addGenerator(
      std::move(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
                    this->_dataLayout.getGlobalPrefix())
                    .get()));

  std::cout << "Constructing compiler object.." << std::endl;
  _llvmManager = LLVMInstanceManager::getInstance();

  // custom diagnostic engine creation
  _diagnosticOptions = new clang::DiagnosticOptions();
  _diagnosticIDs = new clang::DiagnosticIDs();
  _diagConsumer = std::make_shared<vc::FileLogDiagnosticConsumer>(
      log, _diagnosticOptions.get());
  _diagEngine = new clang::DiagnosticsEngine(
      _diagnosticIDs, _diagnosticOptions.get(), _diagConsumer.get(), false);

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
std::filesystem::path
JITCompiler::generateIR(const std::vector<std::filesystem::path> &src,
                        const std::vector<std::string> &func,
                        const std::string &versionID,
                        const opt_list_t options) {
  // What we want to generate
  const std::filesystem::path &llvmIRfileName =
      Compiler::getBitcodeFileName(versionID);
  // what we return when generateIR fails
  const std::filesystem::path &failureFileName = std::filesystem::u8path("");
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

  const std::filesystem::path &command_filename =
      _llvmManager->getClangExePath();
  if (command_filename.empty()) {
    report_error("Clang exe path is empty! Is the _llvmManager initialized?");
  }
  // clang++ <options> -fpic -shared src -olibFileName
  // -Wno-return-type-c-linkage
  std::vector<const char *> cmd_str;
  cmd_str.reserve(options.size() + 6);
  cmd_str.push_back(command_filename.c_str());
  cmd_str.push_back(std::move("-c"));
  cmd_str.push_back(std::move("-emit-llvm"));
  cmd_str.push_back(std::move("-fpic"));
  cmd_str.push_back(std::move("-Wno-return-type-c-linkage"));
  const std::string outputArgument = "-o" + llvmIRfileName.string();
  cmd_str.push_back(std::move(outputArgument).c_str());

  // create a local copy of option strings
  const auto &argv_owner = getArgV(options);
  std::vector<const char *> argv;
  argv.reserve(argv_owner.size());
  for (const auto &arg : argv_owner) {
    argv.push_back(arg.c_str());
  }
  cmd_str.insert(cmd_str.end(), argv.begin(), argv.end());
  for (const auto &src_file : src) {
    cmd_str.push_back(src_file.c_str());
  }

  // log the command line string used to create this task
  for (const auto &arg : cmd_str) {
    log_str = log_str + arg + " ";
  }
  Compiler::log_string(log_str);

  driver::Driver NikiLauda(_llvmManager->getClangExePath().string(),
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
                                 " - Driver error code: " +
                                 std::to_string(res);
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
 * The error reporting had been changed, from llvm::errs() to report_error().
 * It supports all the options of `opt` but the ones related to output
 * management.
 * It is a constraint for this method to generate a well-formed output file.
 * If this method is not able to generate a well-formed optimized output file,
 * it will return an empty path string as generated failureFileName.
 */
std::filesystem::path
JITCompiler::runOptimizer(const std::filesystem::path &src_IR,
                          const std::string &versionID,
                          const opt_list_t options) const {
  // What we want to generate
  const std::filesystem::path optBCfilename =
      Compiler::getOptBitcodeFileName(versionID);

  // what we return when generateIR fails
  const std::filesystem::path failureFileName = "";
  // Our custom error reporting function
  auto report_error = [&](const std::string message) {
    std::string error_string = "JITCompiler::runOptimizer";
    error_string = error_string + " ERROR during processing of version ";
    error_string = error_string + versionID;
    error_string = error_string + "\n\t";
    error_string = error_string + message;
    Compiler::log_string(error_string);
    return;
  };
  // Convert options to an argv-like array
  const std::vector<std::string> &argv_owner = getArgV(options);
  const size_t argc = argv_owner.size() + 1; // opt <options>, +1 for opt
  std::vector<const char *> argv;            // fake argv
  // log the command line string used to create this task
  std::string log_str = std::filesystem::u8path(OPT_EXE_FULLPATH).string();
  log_str += " ";
  argv.reserve(argc);
  argv.push_back(std::move(
      std::filesystem::u8path(OPT_EXE_FULLPATH).c_str())); // /bin/opt...
  for (const auto &arg : argv_owner) {
    argv.push_back(arg.c_str());
    log_str = log_str + arg + " ";
  }
  // Equivalent command
  log_str += src_IR.string() + " -o " + optBCfilename.c_str();
  Compiler::log_string("JIT: " + log_str);

  // command line options are static and should be accessed exclusively
  opt_parse_mtx.lock();

  // Clean old opt values
  resetOptOptions();
  
  // Parse invoking params
  llvm::cl::ParseCommandLineOptions(argc,
                                    const_cast<const char **>(argv.data()),
                                    "JITCompiler::runOptimizer");

  // Set input and output files, like if they had been parsed from the cli
  InputFilename = src_IR.string();
  OutputFilename = optBCfilename.string();
  
  // parse plugins, this was done in opt.cpp in the main function.
  // From now on, we can almost follow what is being done in opt.cpp
  // By remembering to modify return values and error reporting accordingly.
  llvm::SmallVector<llvm::PassPlugin, 1> PluginList;
  PassPlugins.setCallback([&](const std::string &PluginPath) {
    llvm::Expected<PassPlugin> Plugin = llvm::PassPlugin::Load(PluginPath);
    if (!Plugin) {
      report_error(" Failed to load passes from '" + PluginPath +
                   "'. Request ignored.\n");
      return;
    }
    PluginList.emplace_back(Plugin.get());
  });

  using namespace llvm;

  // Prepare the context, in opt.cpp is simply Context
  LLVMContext optContext;
  
  // If `-passes=` is specified, use NPM (new pass manager).
  // If `-enable-new-pm` is specified and there are no codegen passes, use NPM.
  // e.g. `-enable-new-pm -sroa` will use NPM.
  // but `-enable-new-pm -codegenprepare` will still revert to legacy PM.
  const bool UseNPM = (EnableNewPassManager && !shouldForceLegacyPM()) ||
                      PassPipeline.getNumOccurrences() > 0;
  if (UseNPM && !PassList.empty()) {
    report_error(
        "The `opt -passname` syntax for the new pass manager is "
        "not supported, please use `opt -passes=<pipeline>` (or the `-p` "
        "alias for a more concise version).\nSee "
        "https://llvm.org/docs/NewPassManager.html#invoking-opt "
        "for more details on the pass pipeline syntax.\n\n");
    return failureFileName;
  }
  if (!UseNPM && PluginList.size()) {
    report_error(std::string(argv[0]) + ": " + PassPlugins.ArgStr.str() +
                 " specified with legacy PM.\n");
    return failureFileName;
  }

  TimeTracerRAII TimeTracer(argv[0]);

  SMDiagnostic Err;

  optContext.setDiscardValueNames(DiscardValueNames);
  if (!DisableDITypeMap) {
    optContext.enableDebugTypeODRUniquing();
  }

  Expected<std::unique_ptr<ToolOutputFile>> RemarksFileOrErr =
      setupLLVMOptimizationRemarks(optContext, RemarksFilename, RemarksPasses,
                                   RemarksFormat, RemarksWithHotness,
                                   RemarksHotnessThreshold);
  if (Error E = RemarksFileOrErr.takeError()) {
    report_error("During optimization remarks, " + toString(std::move(E)) +
                 '\n');
    return failureFileName;
  }
  std::unique_ptr<ToolOutputFile> RemarksFile = std::move(*RemarksFileOrErr);

// Figure out what stream we are supposed to write to...
// EDIT: we move this section to the new pass manager

// Load the input module....
#if LLVM_VERSION_MAJOR < 16
  // Load the input module...
  auto SetDataLayout = [](StringRef) -> Optional<std::string> {
    if (ClDataLayout.empty())
      return None;
    return ClDataLayout;
  };
#else
  auto SetDataLayout = [](StringRef, StringRef) -> std::optional<std::string> {
    if (ClDataLayout.empty())
      return std::nullopt;
    return ClDataLayout;
  };
#endif
  // EDIT: injected our source file here
  std::unique_ptr<llvm::Module> M;
  if (NoUpgradeDebugInfo)
    M = parseAssemblyFileWithIndexNoUpgradeDebugInfo(
            InputFilename, Err, optContext, nullptr, SetDataLayout)
            .Mod;
  else
#if LLVM_VERSION_MAJOR < 16
    M = parseIRFile(InputFilename, Err, optContext, SetDataLayout);
#else
    M = parseIRFile(InputFilename, Err, optContext,
                    llvm::ParserCallbacks(SetDataLayout));
#endif
  if (!M) {
    std::string parsing_error_str;
    llvm::raw_string_ostream s_ostream(parsing_error_str);
    Err.print(argv[0], s_ostream);
    report_error("Unable to load module from source file\n" + s_ostream.str());
    if (!Compiler::exists(src_IR)) {
      report_error("Cannot find source file " + src_IR.string() + "\n");
    }
    return failureFileName;
  }

  // Strip debug info before running the verifier.
  if (StripDebug) {
    llvm::StripDebugInfo(*M);
  }
  // Erase module-level named metadata, if requested.
  if (StripNamedMetadata) {
    while (!M->named_metadata_empty()) {
      NamedMDNode *NMD = &*M->named_metadata_begin();
      M->eraseNamedMetadata(NMD);
    }
  }
  // If we are supposed to override the target triple or data layout, do so now.
  if (!TargetTriple.empty())
    M->setTargetTriple(Triple::normalize(TargetTriple));

  // Immediately run the verifier to catch any problems before starting up the
  // pass pipelines.  Otherwise we can crash on broken code during
  // doInitialization().
  // EDIT: to have a stream
  std::string parsing_error_str;
  llvm::raw_string_ostream s_ostream(parsing_error_str);
  if (!NoVerify && llvm::verifyModule(*M, &s_ostream)) {
    report_error(
        "Verifier check fail: input module is broken! Error details: " +
        s_ostream.str() + "\n");
    return failureFileName;
  }
  // Enable testing of whole program devirtualization on this module by invoking
  // the facility for updating public visibility to linkage unit visibility when
  // specified by an internal option. This is normally done during LTO which is
  // not performed via opt.
  updateVCallVisibilityInModule(*M,
                                /* WholeProgramVisibilityEnabledInLTO */ false,
                                /* DynamicExportSymbols */ {});
  // Figure out what stream we are supposed to write to...
  // EDIT: moved to NPM because it conflicts with the legacy PM
  // retrieve module triple and prepare Target Machine object (may be empty)
  llvm::Triple ModuleTriple(M->getTargetTriple());
  std::string CPUStr, FeaturesStr;
  llvm::TargetMachine *Machine = nullptr;
  llvm::TargetOptions Options =
      llvm::codegen::InitTargetOptionsFromCodeGenFlags(ModuleTriple);

  if (ModuleTriple.getArch()) {
    CPUStr = llvm::codegen::getCPUStr();
    FeaturesStr = llvm::codegen::getFeaturesStr();
    Machine = GetTargetMachine(ModuleTriple, CPUStr, FeaturesStr, Options);
  } else if (ModuleTriple.getArchName() != "unknown" &&
             ModuleTriple.getArchName() != "") {
    report_error(std::string(argv[0]) + ": unrecognized architecture '" +
                 ModuleTriple.getArchName().str() + "' provided.\n");
    return failureFileName;
  }

  std::unique_ptr<TargetMachine> TM(Machine);
  // Override function attributes based on CPUStr, FeaturesStr,
  // and command line flags.
  llvm::codegen::setFunctionAttributes(CPUStr, FeaturesStr, *M);

  // If the output is set to be emitted to standard out, and standard out is a
  // console, print out a warning message and refuse to do it.  We don't
  // impress anyone by spewing tons of binary goo to a terminal.
  // EDIT: Moved in NPM
  if (OutputThinLTOBC)
    M->addModuleFlag(llvm::Module::Error, "EnableSplitLTOUnit", SplitLTOUnit);

  // Add an appropriate TargetLibraryInfo pass for the module's triple.
  llvm::TargetLibraryInfoImpl TLII(ModuleTriple);

  // The -disable-simplify-libcalls flag actually disables all builtin optzns.
  if (DisableSimplifyLibCalls) {
    TLII.disableAllFunctions();
  } else {
    // Disable individual builtin functions in TargetLibraryInfo.
    llvm::LibFunc F;
    for (auto &FuncName : DisableBuiltins)
      if (TLII.getLibFunc(FuncName, F))
        TLII.setUnavailable(F);
      else {
        report_error(std::string(argv[0]) +
                     ": cannot disable nonexistent builtin function " +
                     FuncName + '\n');
        return failureFileName;
      }
  }

  // Use legacy pass manager, default in LLVM 15 and lower
  // Legacy PM implementation (Out variable name had been replaced to avoid
  // confusion)
  // --------------------------------------------------------------------------------------------
  if (!UseNPM) {
    // Create a PassManager to hold and optimize the collection of passes we are
    // about to build.
    llvm::legacy::PassManager Passes;

    Passes.add(new llvm::TargetLibraryInfoWrapperPass(TLII));

    // Add an appropriate DataLayout instance for this module.
    const llvm::DataLayout &DL = M->getDataLayout();
    if (DL.isDefault() && !ClDataLayout.empty()) {
      M->setDataLayout(ClDataLayout);
    }

    // Add internal analysis passes from the target machine.
    Passes.add(llvm::createTargetTransformInfoWrapperPass(
        (TM) ? TM->getTargetIRAnalysis() : llvm::TargetIRAnalysis()));

    std::unique_ptr<llvm::legacy::FunctionPassManager> FPasses;
    if (OptLevelO0 || OptLevelO1 || OptLevelO2 || OptLevelOs || OptLevelOz ||
        OptLevelO3) {
      FPasses.reset(new llvm::legacy::FunctionPassManager(M.get()));
      FPasses->add(llvm::createTargetTransformInfoWrapperPass(
          TM ? TM->getTargetIRAnalysis() : llvm::TargetIRAnalysis()));
    }

    // Create a new optimization pass for each one specified on the command line
    for (uint64_t i = 0; i < PassList.size(); ++i) {
      if (StandardLinkOpts &&
          StandardLinkOpts.getPosition() < PassList.getPosition(i)) {
        AddStandardLinkPasses(Passes);
        StandardLinkOpts = false;
      }
      if (OptLevelO0 && OptLevelO0.getPosition() < PassList.getPosition(i)) {
        AddOptimizationPasses(Passes, *FPasses, TM.get(), 0, 0);
        OptLevelO0 = false;
      }
      if (OptLevelO1 && OptLevelO1.getPosition() < PassList.getPosition(i)) {
        AddOptimizationPasses(Passes, *FPasses, TM.get(), 1, 0);
        OptLevelO1 = false;
      }
      if (OptLevelO2 && OptLevelO2.getPosition() < PassList.getPosition(i)) {
        AddOptimizationPasses(Passes, *FPasses, TM.get(), 2, 0);
        OptLevelO2 = false;
      }
      if (OptLevelOs && OptLevelOs.getPosition() < PassList.getPosition(i)) {
        AddOptimizationPasses(Passes, *FPasses, TM.get(), 2, 1);
        OptLevelOs = false;
      }
      if (OptLevelOz && OptLevelOz.getPosition() < PassList.getPosition(i)) {
        AddOptimizationPasses(Passes, *FPasses, TM.get(), 2, 2);
        OptLevelOz = false;
      }
      if (OptLevelO3 && OptLevelO3.getPosition() < PassList.getPosition(i)) {
        AddOptimizationPasses(Passes, *FPasses, TM.get(), 3, 0);
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
      AddOptimizationPasses(Passes, *FPasses, TM.get(), 0, 0);
    }
    if (OptLevelO1) {
      AddOptimizationPasses(Passes, *FPasses, TM.get(), 1, 0);
    }
    if (OptLevelO2) {
      AddOptimizationPasses(Passes, *FPasses, TM.get(), 2, 0);
    }
    if (OptLevelOs) {
      AddOptimizationPasses(Passes, *FPasses, TM.get(), 2, 1);
    }
    if (OptLevelOz) {
      AddOptimizationPasses(Passes, *FPasses, TM.get(), 2, 2);
    }
    if (OptLevelO3) {
      AddOptimizationPasses(Passes, *FPasses, TM.get(), 3, 0);
    }

    // run function passes
    if (FPasses) {
      FPasses->doInitialization();
      for (llvm::Function &F : *M) {
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
        std::make_unique<llvm::ToolOutputFile>(optBCfilename.c_str(),
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
    }
#if LLVM_VERSION_MAJOR < 16
    else if (OutputThinLTOBC) {
      Passes.add(createWriteThinLTOBitcodePass(*OS));
    }
#endif
    else {
      Passes.add(createBitcodeWriterPass(*OS, PreserveBitcodeUseListOrder,
                                         EmitSummaryIndex, EmitModuleHash));
    }

    // #ifdef VC_DEBUG
    //  Before executing passes, print the final values of the LLVM options.
    llvm::cl::PrintOptionValues();
    // #endif

    // If requested, run all passes again with the same pass manager to catch
    // bugs caused by persistent state in the passes
    if (RunTwice) {
      std::unique_ptr<llvm::Module> module2(llvm::CloneModule(*M));
      Passes.run(*module2);
      CompileTwiceBuffer = Buffer;
      Buffer.clear();
    }

    // Now that we have all of the passes ready, run them.
    Passes.run(*M);

    // Compare the two outputs and make sure they're the same
    if (RunTwice) {
      if (Buffer.size() != CompileTwiceBuffer.size() ||
          (memcmp(Buffer.data(), CompileTwiceBuffer.data(), Buffer.size()) !=
           0)) {
        // running twice the same passes generated diverging bitcode versions
        const std::string error_message =
            "Running the pass manager twice changed the output.\n"
            "Writing the result of the second run to the specified output.\n"
            "To generate the one-run comparison binary, just run without\n"
            "the compile-twice option\n";
        report_error(error_message);
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
    report_error(
        "Unexpected error: Opt succeded but optBCfilename does not exist");
    return failureFileName;
  } else if (LLVM_VERSION_MAJOR >= 16) {
    // New PM implementation
    // --------------------------------------------------------------------------------------------
    // Use the new pass manager.
    // Figure out what stream we are supposed to write to...
    // EDIT: It conflicts with the old PM implementation, so we implement it
    // here
    std::unique_ptr<ToolOutputFile> Out;
    std::unique_ptr<ToolOutputFile> ThinLinkOut;
    if (NoOutput) {
      if (!OutputFilename.empty())
        report_error(
            "WARNING: The -o (output filename) option is ignored when\n"
            "the --disable-output option is used.\n");
    } else {
      // Default to standard output. EDIT: this should never happen, as we set
      // the OutputFilename regardless
      if (OutputFilename.empty())
        OutputFilename = "-";
      std::error_code EC;
      sys::fs::OpenFlags Flags =
          OutputAssembly ? sys::fs::OF_TextWithCRLF : sys::fs::OF_None;
      Out.reset(new ToolOutputFile(OutputFilename, EC, Flags));
      if (EC) {
        report_error(EC.message() + '\n');
        return failureFileName;
      }
      if (!ThinLinkBitcodeFile.empty()) {
        ThinLinkOut.reset(
            new ToolOutputFile(ThinLinkBitcodeFile, EC, sys::fs::OF_None));
        if (EC) {
          report_error(EC.message() + '\n');
          return failureFileName;
        }
      }
    }
    // EDIT: moved due to Out defined afterwards
    // If the output is set to be emitted to standard out, and standard out is a
    // console, print out a warning message and refuse to do it.  We don't
    // impress anyone by spewing tons of binary goo to a terminal.
    if (!Force && !NoOutput && !OutputAssembly)
      if (CheckBitcodeOutputToConsole(Out->os()))
        NoOutput = true;
    // EDIT: now this section should be identical to the one in opt.cpp
    if (legacy::debugPassSpecified()) {
      report_error(
          "-debug-pass does not work with the new PM, either use "
          "-debug-pass-manager, or use the legacy PM (-enable-new-pm=0)\n");
      return failureFileName;
    }
    auto NumOLevel = OptLevelO0 + OptLevelO1 + OptLevelO2 + OptLevelO3 +
                     OptLevelOs + OptLevelOz;
    if (NumOLevel > 1) {
      report_error("Cannot specify multiple -O#\n");
      return failureFileName;
    }
    if (NumOLevel > 0 && (PassPipeline.getNumOccurrences() > 0)) {
      report_error("Cannot specify -O# and --passes=/--foo-pass, use "
                   "-passes='default<O#>,other-pass'\n");
      return failureFileName;
    }
    std::string Pipeline = PassPipeline;

    if (OptLevelO0)
      Pipeline = "default<O0>";
    if (OptLevelO1)
      Pipeline = "default<O1>";
    if (OptLevelO2)
      Pipeline = "default<O2>";
    if (OptLevelO3)
      Pipeline = "default<O3>";
    if (OptLevelOs)
      Pipeline = "default<Os>";
    if (OptLevelOz)
      Pipeline = "default<Oz>";
    opt_tool::OutputKind OK = opt_tool::OK_NoOutput;
    // if (!opt_tool::NoOutput)
    OK = OutputAssembly ? opt_tool::OK_OutputAssembly
                        : (OutputThinLTOBC ? opt_tool::OK_OutputThinLTOBitcode
                                           : opt_tool::OK_OutputBitcode);

    opt_tool::VerifierKind VK = opt_tool::VK_VerifyOut;
    if (NoVerify)
      VK = opt_tool::VK_NoVerifier;
    else if (VerifyEach)
      VK = opt_tool::VK_VerifyEachPass;

    // The user has asked to use the new pass manager and provided a pipeline
    // string. Hand off the rest of the functionality to the new code for that
    // layer.
    auto ok = runPassPipeline(argv[0], *M, TM.get(), &TLII, Out.get(),
                              ThinLinkOut.get(), RemarksFile.get(), Pipeline,
                              PluginList, OK, VK, PreserveAssemblyUseListOrder,
                              PreserveBitcodeUseListOrder, EmitSummaryIndex,
                              EmitModuleHash, EnableDebugify,
                              VerifyDebugInfoPreserve);
    if (ok) {
      if (exists(optBCfilename))
        return optBCfilename;
      else {
        report_error(optBCfilename.string() +
                     " should exist because opt succeded but it does not.");
        return failureFileName;
      }
    } else {
      return failureFileName;
    }
  } else {
    const std::string error_message =
        "The new pass manager implementation for LLVM 15 or less is not "
        "implemented yet in libVersioningCompiler.";
    report_error(error_message);
    return failureFileName;
  }
}

// ---------------------------------------------------------------------------
// -------------------------------- addModule --------------------------------
// ---------------------------------------------------------------------------
/** addModule wrapper
 *
 * Makes the actual call to llvm CompileLayer addModule functions and saves
 * the JIT compilation results in the JITCompiler class state
 */
void JITCompiler::addModule(std::unique_ptr<llvm::Module> m,
                            const std::string &versionID) {
  llvm::orc::ThreadSafeModule TSM(std::move(m), _tsctx);
  auto lm_iterator = _layer_map.find(versionID);

  if (lm_iterator == _layer_map.end()) {
    std::string error_string = "JITCompiler::addModule ";
    error_string =
        error_string + "compileLayer for " + versionID + " not found";
    Compiler::log_string(error_string);
    return;
  }
  auto rt_iterator = _resource_tracker_map.find(versionID);
  if (rt_iterator == _resource_tracker_map.end()) {
    _resource_tracker_map[versionID] = _mainJD.createResourceTracker();
  }
  handleAllErrors(std::move(lm_iterator->second->add(
                      _resource_tracker_map[versionID], std::move(TSM))),
                  [&](llvm::ErrorInfoBase &EIB) {
                    llvm::errs()
                        << "Error while adding a module: " << EIB.message()
                        << "\n";
                  });
  return;
}

// ---------------------------------------------------------------------------
// ------------------------------- findSymbol --------------------------------
// ---------------------------------------------------------------------------
llvm::Expected<llvm::JITEvaluatedSymbol>
JITCompiler::findSymbol(const std::string Name, const std::string &versionID) {
  llvm::StringRef llvmName(Name);
  return _ES->lookup({&_mainJD}, _mangle(llvmName.str()));
}

// ---------------------------------------------------------------------------
// ------------------------------- loadSymbol --------------------------------
// ---------------------------------------------------------------------------
/** Symbol Loading
 *
 * This implementation looks for the version saved module and adds it to the
 * llvm CompileLayer. It later resolves the symbol and returns it to its
 * caller. This is the function which actually performs the JIT compilation
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

std::vector<void *>
JITCompiler::loadSymbols(const std::filesystem::path &bin,
                         const std::vector<std::string> &func, void **handler) {

  std::vector<void *> symbols = {};

  // In JITCompiler implementation bin is used as a reference to our version
  // ID
  const std::string versionID = bin.string();

  auto mm_iterator = _modules_map.find(versionID);
  if (mm_iterator == _modules_map.end()) {
    std::string error_string = "JITCompiler::loadSymbol ";
    error_string = error_string + "cannot load symbol from " + versionID +
                   " - module not found";
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
  // It seems that there is no better way to clone a module.
  // Otherwise we should call llvm::parseIRFile each time
  addModule(std::move(llvm::CloneModule(*mm_iterator->second)), bin);
  auto rt_iterator = _resource_tracker_map.find(versionID);
  if (rt_iterator == _resource_tracker_map.end()) {
    std::string error_string = "JITCompiler::loadSymbol ";
    error_string = error_string + "cannot load symbol from " + versionID +
                   " - resource tracker not found";
    Compiler::log_string(error_string);
    return symbols;
  }

  _isloaded_map[versionID] = true;

  for (const std::string &f : func) {
    auto findSym = findSymbol(f, versionID);
    handleAllErrors(
        std::move(findSym.takeError()), [&](llvm::ErrorInfoBase &EIB) {
          llvm::errs() << "Error while lokking for symbol: " << EIB.message()
                       << "\n";
        });
    void *symbol = (void *)findSym->getAddress();
    if (!symbol) {
      std::string error_str = "cannot load symbol " + f + " from " +
                              bin.string() + " : symbol not found";
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
 * This implementation uses the handler argument to keep the internal
 * JITCompiler class state consistent cleaning the loaded symbols and removing
 * the llvm Module from the specific CompileLayer
 */
void JITCompiler::releaseSymbol(void **handler) {

  std::string *id = static_cast<std::string *>(*handler);

  auto map_rows_iterator = _isloaded_map.find(*id);
  if (map_rows_iterator == _isloaded_map.end()) {
    std::string error_string = "JITCompiler::releaseSymbol";
    error_string = error_string + " ERROR - module not found ";
    Compiler::log_string(error_string);
    return;
  } else {
    llvm::handleAllErrors(_resource_tracker_map[*id]->remove(),
                          [&](llvm::ErrorInfoBase &EIB) {
                            llvm::errs() << "Error while deleting a resource "
                                            "tracker during a symbol release: "
                                         << EIB.message() << "\n";
                          }); // release all resources linked
                              // to this ResourceTrackerSP
    _resource_tracker_map.erase(*id);
    _isloaded_map.erase(*id);
  }
  delete (id);
  *handler = nullptr;
  return;
}

// ---------------------------------------------------------------------------
// ------------------------------- generateBin -------------------------------
// ---------------------------------------------------------------------------
/** JIT Compile the llvm module
 *
 * This implementation parses a previously generated (and eventually
 * optimized) IR file elevating it to its in-memory representation. This
 * module is then saved in the JITCompiler class instance state to be later
 * added (by the loadSymbol method) to llvm's CompilerLayer
 */
std::filesystem::path
JITCompiler::generateBin(const std::vector<std::filesystem::path> &src,
                         const std::vector<std::string> &func,
                         const std::string &versionID,
                         const opt_list_t options) {

  // The source IR file name to JIT compile
  if (src.size() == 0) {
    std::string error_string = "JITCompiler::generateBin";
    error_string = error_string + " ERROR - no source file provided";
    Compiler::log_string(error_string);
    return "";
  }
  std::filesystem::path source = src[0]; // if contains IR then it's just one
                                         // element, otherwise will generate it

  _obj_map[versionID] =
      std::make_unique<llvm::orc::RTDyldObjectLinkingLayer>(*this->_ES, []() {
        return std::make_unique<llvm::SectionMemoryManager>();
      });
  _layer_map[versionID] = std::make_unique<llvm::orc::IRCompileLayer>(
      *this->_ES, *(_obj_map[versionID]),
      std::make_unique<llvm::orc::ConcurrentIRCompiler>(
          std::move(this->_JTMB)));
  _resource_tracker_map[versionID] = _mainJD.createResourceTracker();

  // IR filename
  const std::filesystem::path llvmIRfileName =
      Compiler::getBitcodeFileName(versionID);
  // IR optimized filename
  const std::filesystem::path llvmOPTIRfileName =
      Compiler::getOptBitcodeFileName(versionID);

  // If IR files were not generated, generate it now but don't optimize it

  auto report_error = [&](const std::string message) {
    std::string error_string = "JITCompiler::generateBin";
    error_string = error_string + " ERROR during processing of version ";
    error_string = error_string + versionID;
    error_string = error_string + "\n\t";
    error_string = error_string + message;
    Compiler::log_string(error_string);
    return;
  };
  // What we return when generateIR fails
  const std::string failureName = "";

  if (!exists(llvmOPTIRfileName) && !exists(llvmIRfileName)) {
    Compiler::log_string("IR file not found, generating from source..");

    source = generateIR(src, func, versionID, options);
    if (source == failureName) {
      report_error("Failed to generate version " + versionID +
                   ": generateIR file not produced.");
      return failureName;
    } else if (!exists(source)) {
      report_error("Failed to generate version " + versionID +
                   ": generateIR file not found, this should not happen.");
      return failureName;
    }
  } else if (!exists(source)) {
    report_error("Cannot load module in version " + versionID +
                 ": the source file " + source.string() + " does not exist.");
    return failureName;
  }

  llvm::SMDiagnostic parsing_input_error_code;
  Compiler::log_string("Jitting IR file: " + source.string());

  _modules_map[versionID] = std::move(llvm::parseIRFile(
      source.string(), parsing_input_error_code, *(_tsctx.getContext())));
  if (parsing_input_error_code.getMessage().str().length() > 0) {
    report_error(
        "Module with source '" + source.string() +
        "' was not generated: " + parsing_input_error_code.getMessage().str());
    return failureName;
  }
  return versionID;
}

// ---------------------------------------------------------------------------
// ------------------------------ hasOptimizer -------------------------------
// ---------------------------------------------------------------------------
bool JITCompiler::hasOptimizer() const { return true; }

// ---------------------------------------------------------------------------
// ----------------------------- getOptionString -----------------------------
// ---------------------------------------------------------------------------
inline std::string JITCompiler::getOptionString(const Option &o) const {
  // remove single and double quotes
  std::string tmp_val = o.getValue();
  if (tmp_val.length() > 1 &&
      ((tmp_val[0] == '\"' && tmp_val[tmp_val.length() - 1] == '\"') ||
       (tmp_val[0] == '\'' && tmp_val[tmp_val.length() - 1] == '\''))) {
    tmp_val = tmp_val.substr(1, tmp_val.length() - 2);
  }
  return o.getPrefix() + tmp_val;
}

// ---------------------------------------------------------------------------
// --------------------------------- getArgV ---------------------------------
// ---------------------------------------------------------------------------
inline std::vector<std::string>
JITCompiler::getArgV(const opt_list_t optionList) const {
  std::vector<std::string> v;
  v.reserve(optionList.size());
  for (const auto &o : optionList) {
    v.push_back(getOptionString(o));
  }
  return v;
}
