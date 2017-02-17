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
#ifndef LIB_VERSIONING_COMPILER_CLANG_LLVM_OPT_UTILS
#define LIB_VERSIONING_COMPILER_CLANG_LLVM_OPT_UTILS

/** This source file contains portion of code of the LLVM tool `opt`.
 * LLVM tool `opt` is covered by the LLVM license.
 * See LLVM_LICENSE.txt for more details.
 */

// opt includes
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LegacyPassNameParser.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/LinkAllIR.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/Coroutines.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"

// The OptimizationList is automatically populated with registered Passes by the
// PassNameParser.
//
static llvm::cl::list<const llvm::PassInfo*, bool, llvm::PassNameParser>
PassList(llvm::cl::desc("Optimizations available:"));

static llvm::cl::opt<bool> OutputAssembly(
  "S",
  llvm::cl::desc("Write output as LLVM assembly"));

static llvm::cl::opt<bool> OutputThinLTOBC(
  "thinlto-bc",
  llvm::cl::desc("Write output as ThinLTO-ready bitcode"));

static llvm::cl::opt<bool> NoVerify(
  "disable-verify",
  llvm::cl::desc("Do not run the verifier"),
  llvm::cl::Hidden);

static llvm::cl::opt<bool> VerifyEach(
  "verify-each",
  llvm::cl::desc("Verify after each transform"));

static llvm::cl::opt<bool> DisableDITypeMap(
  "disable-debug-info-type-map",
  llvm::cl::desc("Don't use a uniquing type map for debug info"));

static llvm::cl::opt<bool> StripDebug(
  "strip-debug",
  llvm::cl::desc("Strip debugger symbol info from translation unit"));

static llvm::cl::opt<bool> DisableInline(
  "disable-inlining",
  llvm::cl::desc("Do not run the inliner pass"));

static llvm::cl::opt<bool> DisableOptimizations(
  "disable-opt",
  llvm::cl::desc("Do not run any optimization passes"));

static llvm::cl::opt<bool> StandardLinkOpts(
  "std-link-opts",
  llvm::cl::desc("Include the standard link time optimizations"));

static llvm::cl::opt<bool> OptLevelO0(
  "O0",
  llvm::cl::desc("Optimization level 0. Similar to clang -O0"));

static llvm::cl::opt<bool> OptLevelO1(
  "O1",
  llvm::cl::desc("Optimization level 1. Similar to clang -O1"));

static llvm::cl::opt<bool> OptLevelO2(
  "O2",
  llvm::cl::desc("Optimization level 2. Similar to clang -O2"));

static llvm::cl::opt<bool> OptLevelOs(
  "Os",
  llvm::cl::desc("Like -O2 with extra optimizations for size. Similar to clang -Os"));

static llvm::cl::opt<bool> OptLevelOz(
  "Oz",
  llvm::cl::desc("Like -Os but reduces code size further. Similar to clang -Oz"));

static llvm::cl::opt<bool> OptLevelO3(
  "O3",
  llvm::cl::desc("Optimization level 3. Similar to clang -O3"));

static llvm::cl::opt<unsigned> CodeGenOptLevel(
  "codegen-opt-level",
  llvm::cl::desc("Override optimization level for codegen hooks"));

static llvm::cl::opt<std::string> TargetTriple(
  "mtriple",
  llvm::cl::desc("Override target triple for module"));

static llvm::cl::opt<bool> UnitAtATime(
  "funit-at-a-time",
  llvm::cl::desc("Enable IPO. This corresponds to gcc's -funit-at-a-time"),
  llvm::cl::init(true));

static llvm::cl::opt<bool> DisableLoopUnrolling(
  "disable-loop-unrolling",
  llvm::cl::desc("Disable loop unrolling in all relevant passes"),
  llvm::cl::init(false));

static llvm::cl::opt<bool> DisableLoopVectorization(
  "disable-loop-vectorization",
  llvm::cl::desc("Disable the loop vectorization pass"),
  llvm::cl::init(false));

static llvm::cl::opt<bool> DisableSLPVectorization(
  "disable-slp-vectorization",
  llvm::cl::desc("Disable the slp vectorization pass"),
  llvm::cl::init(false));

static llvm::cl::opt<bool> EmitSummaryIndex(
  "module-summary",
  llvm::cl::desc("Emit module summary index"),
  llvm::cl::init(false));

static llvm::cl::opt<bool> EmitModuleHash(
  "module-hash",
  llvm::cl::desc("Emit module hash"),
  llvm::cl::init(false));

static llvm::cl::opt<bool> DisableSimplifyLibCalls(
  "disable-simplify-libcalls",
  llvm::cl::desc("Disable simplify-libcalls"));

static llvm::cl::opt<bool> AnalyzeOnly(
  "analyze",
  llvm::cl::desc("Only perform analysis, no optimization"));

static llvm::cl::opt<std::string> DefaultDataLayout(
  "default-data-layout",
  llvm::cl::desc("data layout string to use if not specified by module"),
  llvm::cl::value_desc("layout-string"),
  llvm::cl::init(""));

static llvm::cl::opt<bool> PreserveBitcodeUseListOrder(
  "preserve-bc-uselistorder",
  llvm::cl::desc("Preserve use-list order when writing LLVM bitcode."),
  llvm::cl::init(true),
  llvm::cl::Hidden);

static llvm::cl::opt<bool> PreserveAssemblyUseListOrder(
  "preserve-ll-uselistorder",
  llvm::cl::desc("Preserve use-list order when writing LLVM assembly."),
  llvm::cl::init(false),
  llvm::cl::Hidden);

static llvm::cl::opt<bool> RunTwice(
  "run-twice",
  llvm::cl::desc("Run all passes twice, re-using the same pass manager."),
  llvm::cl::init(false),
  llvm::cl::Hidden);

static llvm::cl::opt<bool> DiscardValueNames(
  "discard-value-names",
  llvm::cl::desc("Discard names from Value (other than GlobalValue)."),
  llvm::cl::init(false),
  llvm::cl::Hidden);

static llvm::cl::opt<bool> Coroutines(
  "enable-coroutines",
   llvm::cl::desc("Enable coroutine passes."),
   llvm::cl::init(false),
   llvm::cl::Hidden);
// ----------------------------------------------------------------------------
// end of extra optimizer options
// ----------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// --------------------- Reset all command line options ----------------------
// ---------------------------------------------------------------------------
static void resetOptOptions()
{
  auto &optionMap = llvm::cl::getRegisteredOptions();
  for (auto &o : optionMap) {
    o.second->reset();
  }
  return;
}

// ---------------------------------------------------------------------------
// ---------------------- Pass related helper functions ----------------------
// ---------------------------------------------------------------------------
static inline void addPass(llvm::legacy::PassManagerBase &PM, llvm::Pass *P)
{
  // Add the pass to the pass manager...
  PM.add(P);
  // If we are verifying all of the intermediate steps, add the verifier...
  if (VerifyEach) {
    PM.add(llvm::createVerifierPass());
  }
  return;
}

/// This routine adds optimization passes based on selected optimization level,
/// OptLevel.
///
/// OptLevel - Optimization Level
static void AddOptimizationPasses(llvm::legacy::PassManagerBase &MPM,
                                  llvm::legacy::FunctionPassManager &FPM,
                                  llvm::TargetMachine *TM,
                                  uint64_t OptLevel,
                                  uint64_t SizeLevel)
{
  if (!NoVerify || VerifyEach) {
    // Verify that input is correct
    FPM.add(llvm::createVerifierPass());
  }

  PassManagerBuilder Builder;
  Builder.OptLevel = OptLevel;
  Builder.SizeLevel = SizeLevel;

  if (DisableInline) {
    // No inlining pass
  } else if (OptLevel > 1) {
    Builder.Inliner = llvm::createFunctionInliningPass(OptLevel, SizeLevel);
  } else {
    Builder.Inliner = llvm::createAlwaysInlinerLegacyPass();
  }
  Builder.DisableUnitAtATime = !UnitAtATime;
  Builder.DisableUnrollLoops = (DisableLoopUnrolling.getNumOccurrences() > 0) ?
                               DisableLoopUnrolling : OptLevel == 0;
  if (DisableLoopVectorization) {
    // This is final, unless there is a #pragma vectorize enable
    Builder.LoopVectorize = false;
  }
  else if (!Builder.LoopVectorize) {
    // If option wasn't forced via cmd line (-vectorize-loops, -loop-vectorize)
    Builder.LoopVectorize = OptLevel > 1 && SizeLevel < 2;
  }
  // When #pragma vectorize is on for SLP, do the same as above
  Builder.SLPVectorize =
      DisableSLPVectorization ? false : OptLevel > 1 && SizeLevel < 2;

  // Add target-specific passes that need to run as early as possible.
  if (TM) {
    Builder.addExtension(
      llvm::PassManagerBuilder::EP_EarlyAsPossible,
      [&](const llvm::PassManagerBuilder&, llvm::legacy::PassManagerBase &PM) {
        TM->addEarlyAsPossiblePasses(PM);
      });
  }
  if (Coroutines) {
    llvm::addCoroutinePassesToExtensionPoints(Builder);
  }
  Builder.populateFunctionPassManager(FPM);
  Builder.populateModulePassManager(MPM);
  return;
}

static void AddStandardLinkPasses(llvm::legacy::PassManagerBase &PM) {
  llvm::PassManagerBuilder Builder;
  Builder.VerifyInput = true;
  if (DisableOptimizations) {
    Builder.OptLevel = 0;
  }
  if (!DisableInline) {
    Builder.Inliner = llvm::createFunctionInliningPass();
  }
  Builder.populateLTOPassManager(PM);
  return;
}

// ---------------------------------------------------------------------------
// -------------------- CodeGen-related helper functions ---------------------
// ---------------------------------------------------------------------------
static llvm::CodeGenOpt::Level GetCodeGenOptLevel()
{
  if (CodeGenOptLevel.getNumOccurrences()) {
    return static_cast<llvm::CodeGenOpt::Level>(unsigned(CodeGenOptLevel));
  }
  if (OptLevelO1) {
    return llvm::CodeGenOpt::Less;
  }
  if (OptLevelO2) {
    return llvm::CodeGenOpt::Default;
  }
  if (OptLevelO3) {
    return llvm::CodeGenOpt::Aggressive;
  }
  return llvm::CodeGenOpt::None;
}

#endif /* end of include guard: LIB_VERSIONING_COMPILER_CLANG_LLVM_OPT_UTILS */
