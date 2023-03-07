/* Copyright 2017 Politecnico di Milano.
 * Developed by : Stefano Cherubin
 *                PhD student, Politecnico di Milano
 *                <first_name>.<family_name>@polimi.it
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
#ifndef LIB_VERSIONING_COMPILER_CLANG_LLVM_OPT_UTILS
#define LIB_VERSIONING_COMPILER_CLANG_LLVM_OPT_UTILS

/** This source file contains portion of code of the LLVM tool `opt`.
 * LLVM tool `opt` is covered by the LLVM license.
 * See LLVM_LICENSE.txt for more details.
 */

// opt includes
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/InitializePasses.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LegacyPassNameParser.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/LinkAllIR.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"
#include "NewPMDriver.h"
#if LLVM_VERSION_MAJOR < 15
#include "llvm/Transforms/Coroutines.h"
#endif
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

using namespace llvm;
// The OptimizationList is automatically populated with registered Passes by the
// PassNameParser.
//
static cl::list<const PassInfo *, bool, PassNameParser> PassList(
  cl::desc("Optimizations available (use '-passes=' for the new pass manager)"));

static cl::opt<bool> PrintPasses("print-passes", cl::desc("Print available passes that can be "
                                                          "specified in -passes=foo and exit"));

// This flag specifies a textual description of the optimization pass pipeline
// to run over the module. This flag switches opt to use the new pass manager
// infrastructure, completely disabling all of the flags specific to the old
// pass management.

// Removed in LLVM-15
#ifndef LLVM_ENABLE_NEW_PASS_MANAGER
#define LLVM_ENABLE_NEW_PASS_MANAGER 1
#endif
static cl::opt<bool> EnableNewPassManager(
  "enable-new-pm",
  cl::desc("Enable the new pass manager, translating "
            "'opt -foo' to 'opt -passes=foo'. This is strictly for the new PM "
            "migration, use '-passes=' when possible."),
  cl::init(LLVM_ENABLE_NEW_PASS_MANAGER));

// This flag specifies a textual description of the optimization pass pipeline
// to run over the module. This flag switches opt to use the new pass manager
// infrastructure, completely disabling all of the flags specific to the old
// pass management.
static cl::opt<std::string> PassPipeline(
  "passes",
  cl::desc(
      "A textual description of the pass pipeline. To have analysis passes "
      "available before a certain pass, add 'require<foo-analysis>'."));

static cl::opt<bool> OutputAssembly(
  "S",
  cl::desc("Write output as LLVM assembly"));

static cl::opt<bool> OutputThinLTOBC(
  "thinlto-bc",
  cl::desc("Write output as ThinLTO-ready bitcode"));

static cl::opt<bool> SplitLTOUnit(
  "thinlto-split-lto-unit",
  cl::desc("Enable splitting of a ThinLTO LTOUnit"));

static cl::opt<bool> NoVerify(
  "disable-verify",
  cl::desc("Do not run the verifier"),
  cl::Hidden);

static cl::opt<bool> NoUpgradeDebugInfo(
  "disable-upgrade-debug-info",
  cl::desc("Generate invalid output"),
  cl::ReallyHidden);

static cl::opt<bool> VerifyEach(
  "verify-each",
  cl::desc("Verify after each transform"));

// Removed in LLVM 14
static cl::opt<bool> DisableDITypeMap(
  "disable-debug-info-type-map",
  cl::desc("Don't use a uniquing type map for debug info"));

static cl::opt<bool> StripDebug(
  "strip-debug",
  cl::desc("Strip debugger symbol info from translation unit"));

static cl::opt<bool>
  StripNamedMetadata(
      "strip-named-metadata",
      cl::desc("Strip module-level named metadata"));
#if LLVM__MAJOR_VERSION < 14
static cl::opt<bool> DisableInline(
  "disable-inlining",
  cl::desc("Do not run the inliner pass (legacy PM only)"));

static cl::opt<bool> DisableOptimizations(
  "disable-opt",
  cl::desc("Do not run any optimization passes"));

static cl::opt<bool> StandardLinkOpts(
  "std-link-opts",
  cl::desc("Include the standard link time optimizations (legacy PM only)"));
#endif
static cl::opt<bool> OptLevelO0(
  "O0",
  cl::desc("Optimization level 0. Similar to clang -O0. "
           "Use -passes='default<O0>' for the new PM"));

static cl::opt<bool> OptLevelO1(
  "O1",
  cl::desc("Optimization level 1. Similar to clang -O1. "
            "Use -passes='default<O1>' for the new PM"));

static cl::opt<bool> OptLevelO2(
  "O2",
  cl::desc("Optimization level 2. Similar to clang -O2. "
            "Use -passes='default<O2>' for the new PM"));

static cl::opt<bool> OptLevelOs(
  "Os",
  cl::desc("Like -O2 but size-conscious. Similar to clang "
           "-Os. Use -passes='default<Os>' for the new PM"));

static cl::opt<bool> OptLevelOz(
  "Oz",
  cl::desc("Like -O2 but optimize for code size above all else. Similar to "
           "clang -Oz. Use -passes='default<Oz>' for the new PM"));

static cl::opt<bool> OptLevelO3(
  "O3",
  cl::desc("Optimization level 3. Similar to clang -O3. "
           "Use -passes='default<O3>' for the new PM"));

static cl::opt<unsigned> CodeGenOptLevel(
  "codegen-opt-level",
  cl::desc("Override optimization level for codegen hooks, legacy PM only"));

static cl::opt<std::string> TargetTriple(
  "mtriple",
  cl::desc("Override target triple for module"));

static cl::opt<bool> DisableLoopUnrolling(
  "disable-loop-unrolling",
  cl::desc("Disable loop unrolling in all relevant passes"),
  cl::init(false));

static cl::opt<bool> EmitSummaryIndex(
  "module-summary",
  cl::desc("Emit module summary index"),
  cl::init(false));

static cl::opt<bool> EmitModuleHash(
  "module-hash",
  cl::desc("Emit module hash"),
  cl::init(false));

static cl::opt<bool> DisableSimplifyLibCalls(
  "disable-simplify-libcalls",
  cl::desc("Disable simplify-libcalls"));

static cl::list<std::string> DisableBuiltins(
  "disable-builtin",
  cl::desc("Disable specific target library builtin function"),
  cl::ZeroOrMore);

static cl::opt<bool> AnalyzeOnly(
  "analyze",
  cl::desc("Only perform analysis, no optimization"
           "Legacy pass manager only."));

static cl::opt<std::string> DefaultDataLayout(
  "default-data-layout",
  cl::desc("data layout string to use if not specified by module"),
  cl::value_desc("layout-string"),
  cl::init(""));

static cl::opt<bool> PreserveBitcodeUseListOrder(
  "preserve-bc-uselistorder",
  cl::desc("Preserve use-list order when writing LLVM bitcode."),
  cl::init(true),
  cl::Hidden);

static cl::opt<bool> PreserveAssemblyUseListOrder(
  "preserve-ll-uselistorder",
  cl::desc("Preserve use-list order when writing LLVM assembly."),
  cl::init(false),
  cl::Hidden);

static cl::opt<bool> RunTwice(
  "run-twice",
  cl::desc("Run all passes twice, re-using the same pass manager."),
  cl::init(false),
  cl::Hidden);

static cl::opt<bool> DiscardValueNames(
  "discard-value-names",
  cl::desc("Discard names from Value (other than GlobalValue)."),
  cl::init(false),
  cl::Hidden);
#if LLVM_VERSION_MAJOR < 15
static cl::opt<bool> Coroutines(
    "enable-coroutines",
    cl::desc("Enable coroutine passes."),
    cl::init(false),
    cl::Hidden);
#endif
#if LLVM_VERSION_MAJOR > 14
static cl::list<std::string> PassPlugins(
    "load-pass-plugin",
    cl::desc("Load passes from plugin library"));
#endif

static cl::opt<bool> TimeTrace(
    "time-trace",
    cl::desc("Record time trace"));

static cl::opt<unsigned> TimeTraceGranularity(
    "time-trace-granularity",
    cl::desc("Minimum time granularity (in microseconds) traced by time profiler"),
    cl::init(500), cl::Hidden);

static cl::opt<std::string> TimeTraceFile(
    "time-trace-file",
    cl::desc("Specify time trace file destination"),
    cl::value_desc("filename"));

// no pass remarks

cl::opt<opt_tool::PGOKind> PGOKindFlag(
    "pgo-kind",
    cl::init(opt_tool::NoPGO),
    cl::Hidden,
    cl::desc("The kind of profile guided optimization"),
    cl::values(
        clEnumValN(opt_tool::NoPGO, "nopgo", "Do not use PGO."),
        clEnumValN(opt_tool::InstrGen,
                   "pgo-instr-gen-pipeline",
                   "Instrument the IR to generate profile."),
        clEnumValN(opt_tool::InstrUse,
                   "pgo-instr-use-pipeline",
                   "Use instrumented profile to guide PGO."),
        clEnumValN(opt_tool::SampleUse,
                   "pgo-sample-use-pipeline",
                   "Use sampled profile to guide PGO.")));
cl::opt<std::string> ProfileFile("profile-file", cl::desc("Path to the profile."), cl::Hidden);

cl::opt<opt_tool::CSPGOKind> CSPGOKindFlag(
    "cspgo-kind",
    cl::init(opt_tool::NoCSPGO),
    cl::Hidden,
    cl::desc("The kind of context sensitive profile guided optimization"),
    cl::values(
        clEnumValN(opt_tool::NoCSPGO, "nocspgo", "Do not use CSPGO."),
        clEnumValN(opt_tool::CSInstrGen, "cspgo-instr-gen-pipeline", "Instrument (context sensitive) the IR to generate profile."),
        clEnumValN(opt_tool::CSInstrUse, "cspgo-instr-use-pipeline", "Use instrumented (context sensitive) profile to guide PGO.")));

cl::opt<std::string> CSProfileGenFile("cs-profilegen-file", cl::desc("Path to the instrumented context sensitive profile."), cl::Hidden);

// ----------------------------------------------------------------------------
// end of extra optimizer options
// ----------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// --------------------- Reset all command line options ----------------------
// ---------------------------------------------------------------------------
static void resetOptOptions()
{
  auto &optionMap = cl::getRegisteredOptions();
  for (auto &o : optionMap) {
    o.second->reset();
  }
  return;
}

// ---------------------------------------------------------------------------
// ---------------------- Pass related helper functions ----------------------
// ---------------------------------------------------------------------------
static inline void addPass(legacy::PassManagerBase &PM, Pass *P)
{
  // Add the pass to the pass manager...
  PM.add(P);
  // If we are verifying all of the intermediate steps, add the verifier...
  if (VerifyEach) {
    PM.add(createVerifierPass());
  }
  return;
}

/// This routine adds optimization passes based on selected optimization level,
/// OptLevel.
///
/// OptLevel - Optimization Level
static void AddOptimizationPasses(legacy::PassManagerBase &MPM,
                                  legacy::FunctionPassManager &FPM,
                                  TargetMachine *TM,
                                  uint64_t OptLevel,
                                  uint64_t SizeLevel)
{
  if (!NoVerify || VerifyEach) {
    // Verify that input is correct
    FPM.add(createVerifierPass());
  }

  PassManagerBuilder Builder;
  Builder.OptLevel = OptLevel;
  Builder.SizeLevel = SizeLevel;

  if (DisableInline) {
    // No inlining pass
  } else if (OptLevel > 1) {
    Builder.Inliner = createFunctionInliningPass(OptLevel, SizeLevel, false);
  } else {
    Builder.Inliner = createAlwaysInlinerLegacyPass();
  }
  Builder.DisableUnrollLoops = (DisableLoopUnrolling.getNumOccurrences() > 0) ?
                               DisableLoopUnrolling : OptLevel == 0;
  Builder.LoopVectorize = OptLevel > 1 && SizeLevel < 2;
  // When #pragma vectorize is on for SLP, do the same as above
  Builder.SLPVectorize = OptLevel > 1 && SizeLevel < 2;

  // Add target-specific passes that need to run as early as possible.
  if (TM) {
    TM->adjustPassManager(Builder);
  }
#if LLVM_VERSION_MAJOR < 15
  if (Coroutines) {
    addCoroutinePassesToExtensionPoints(Builder);
  }
  switch (PGOKindFlag)
  {
  case opt_tool::InstrGen:
    Builder.EnablePGOInstrGen = true;
    Builder.PGOInstrGen = ProfileFile;
    break;
  case opt_tool::InstrUse:
    Builder.PGOInstrUse = ProfileFile;
    break;
  case opt_tool::SampleUse:
    Builder.PGOSampleUse = ProfileFile;
    break;
  default:
    break;
  }

  switch (CSPGOKindFlag)
  {
  case opt_tool::CSInstrGen:
    Builder.EnablePGOCSInstrGen = true;
    break;
  case opt_tool::CSInstrUse:
    Builder.EnablePGOCSInstrUse = true;
    break;
  default:
    break;
  }
#endif
  Builder.populateFunctionPassManager(FPM);
  Builder.populateModulePassManager(MPM);
  return;
}

// Removed in LLVM 14
static void AddStandardLinkPasses(legacy::PassManagerBase &PM)
{
  PassManagerBuilder Builder;
  Builder.VerifyInput = true;
  if (DisableOptimizations) {
    Builder.OptLevel = 0;
  }
  if (!DisableInline) {
    Builder.Inliner = createFunctionInliningPass();
  }
#if LLVM_VERSION_MAJOR < 15
  Builder.populateLTOPassManager(PM);
#endif
  return;
}

// ---------------------------------------------------------------------------
// -------------------- CodeGen-related helper functions ---------------------
// ---------------------------------------------------------------------------
static CodeGenOpt::Level GetCodeGenOptLevel()
{
  if (CodeGenOptLevel.getNumOccurrences()) {
    return static_cast<CodeGenOpt::Level>(unsigned(CodeGenOptLevel));
  }
  if (OptLevelO1) {
    return CodeGenOpt::Less;
  }
  if (OptLevelO2) {
    return CodeGenOpt::Default;
  }
  if (OptLevelO3) {
    return CodeGenOpt::Aggressive;
  }
  return CodeGenOpt::None;
}

#endif /* end of include guard: LIB_VERSIONING_COMPILER_CLANG_LLVM_OPT_UTILS */
