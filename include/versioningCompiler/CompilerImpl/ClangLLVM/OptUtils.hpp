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
 *
 * This source file contains portion of code of the LLVM tool `opt`.
 * LLVM tool `opt` is covered by the LLVM license.
 * See LLVM_LICENSE.txt for more details.
 *
 * It is an adaptation of the LLVM tool `opt` to the needs for
 * libVersioningCompiler. All the edits from the original file are in form of
 * #if LLVM_VERSION_MAJOR ... #endif, otherwise indicated as comments with the
 * EDIT prefix.
 * */
//===- BreakpointPrinter.h - Breakpoint location printer ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Breakpoint location printer.
///
//===----------------------------------------------------------------------===//
#ifndef LLVM_TOOLS_OPT_BREAKPOINTPRINTER_H
#define LLVM_TOOLS_OPT_BREAKPOINTPRINTER_H

namespace llvm {

class ModulePass;
class raw_ostream;

ModulePass *createBreakpointPrinter(raw_ostream &out);
} // namespace llvm

#endif // LLVM_TOOLS_OPT_BREAKPOINTPRINTER_H

#ifndef LIB_VERSIONING_COMPILER_CLANG_LLVM_OPT_UTILS
#define LIB_VERSIONING_COMPILER_CLANG_LLVM_OPT_UTILS
//===- opt.cpp - The LLVM Modular Optimizer -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Optimizations may be specified an arbitrary number of times on the command
// line, They are run in the order specified.
//
//===----------------------------------------------------------------------===//

// opt includes
#include "NewPMDriver.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/RegionPass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/Bitcode/BitcodeReader.h" // EDIT: Added to get the ParserCallback
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LLVMRemarkStreamer.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LegacyPassNameParser.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ModuleSummaryIndex.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/LinkAllIR.h"
#include "llvm/LinkAllPasses.h"
#include "llvm/MC/SubtargetFeature.h"
#if LLVM_VERSION_MAJOR >= 14
#include "llvm/MC/TargetRegistry.h"
#endif
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Remarks/HotnessThresholdParser.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Target/TargetMachine.h"
#if LLVM_VERSION_MAJOR < 15
#include "llvm/Transforms/Coroutines.h"
#endif
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h" // EDIT: added to provide compatibility to legacy PM
#include "llvm/Transforms/IPO/WholeProgramDevirt.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Debugify.h"

#include <algorithm>
#include <memory>
#include <optional>

using namespace llvm;
using namespace opt_tool;

// EDIT: it seems useless, to avoid issues I cannot see, I kept it.
static codegen::RegisterCodeGenFlags CFG;

// The OptimizationList is automatically populated with registered Passes by the
// PassNameParser.
//
static cl::list<const PassInfo *, bool, PassNameParser> PassList(cl::desc(
    "Optimizations available (use '-passes=' for the new pass manager)"));

static cl::opt<bool> PrintPasses("print-passes",
                                 cl::desc("Print available passes that can be "
                                          "specified in -passes=foo and exit"));

// EDIT: the compile definition had been removed in LLVM 15, so I had set it
// manually
#ifndef LLVM_ENABLE_NEW_PASS_MANAGER
#define LLVM_ENABLE_NEW_PASS_MANAGER 0
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
static cl::alias PassPipeline2("p", cl::aliasopt(PassPipeline),
                               cl::desc("Alias for -passes"));

static cl::opt<std::string> InputFilename(cl::Positional,
                                          cl::desc("<input bitcode file>"),
                                          cl::init("-"),
                                          cl::value_desc("filename"));

static cl::opt<std::string> OutputFilename("o",
                                           cl::desc("Override output filename"),
                                           cl::value_desc("filename"));

static cl::opt<bool> Force("f", cl::desc("Enable binary output on terminals"));

static cl::opt<bool> NoOutput("disable-output",
                              cl::desc("Do not write result bitcode file"),
                              cl::Hidden);

// EDIT: We do use it custom, so this flag should be useless
static cl::opt<bool> OutputAssembly("S",
                                    cl::desc("Write output as LLVM assembly"));

static cl::opt<bool>
    OutputThinLTOBC("thinlto-bc",
                    cl::desc("Write output as ThinLTO-ready bitcode"));

static cl::opt<std::string> ThinLinkBitcodeFile(
    "thin-link-bitcode-file", cl::value_desc("filename"),
    cl::desc(
        "A file in which to write minimized bitcode for the thin link only"));

static cl::opt<bool>
    SplitLTOUnit("thinlto-split-lto-unit",
                 cl::desc("Enable splitting of a ThinLTO LTOUnit"));

static cl::opt<bool> NoVerify("disable-verify",
                              cl::desc("Do not run the verifier"), cl::Hidden);

static cl::opt<bool> NoUpgradeDebugInfo("disable-upgrade-debug-info",
                                        cl::desc("Generate invalid output"),
                                        cl::ReallyHidden);

static cl::opt<bool> VerifyEach("verify-each",
                                cl::desc("Verify after each transform"));

static cl::opt<bool>
    DisableDITypeMap("disable-debug-info-type-map",
                     cl::desc("Don't use a uniquing type map for debug info"));

static cl::opt<bool>
    StripDebug("strip-debug",
               cl::desc("Strip debugger symbol info from translation unit"));

static cl::opt<bool>
    StripNamedMetadata("strip-named-metadata",
                       cl::desc("Strip module-level named metadata"));
// EDIT: kept for compatibility with legacy code
static cl::opt<bool>
    DisableInline("disable-inlining",
                  cl::desc("Do not run the inliner pass (legacy PM only)"));
// EDIT: kept for compatibility with legacy code
static cl::opt<bool>
    DisableOptimizations("disable-opt",
                         cl::desc("Do not run any optimization passes"));
// EDIT: kept for compatibility with legacy code
static cl::opt<bool> StandardLinkOpts(
    "std-link-opts",
    cl::desc("Include the standard link time optimizations (legacy PM only)"));

static cl::opt<bool>
    OptLevelO0("O0", cl::desc("Optimization level 0. Similar to clang -O0. "
                              "Use -passes='default<O0>' for the new PM"));

static cl::opt<bool>
    OptLevelO1("O1", cl::desc("Optimization level 1. Similar to clang -O1. "
                              "Use -passes='default<O1>' for the new PM"));

static cl::opt<bool>
    OptLevelO2("O2", cl::desc("Optimization level 2. Similar to clang -O2. "
                              "Use -passes='default<O2>' for the new PM"));

static cl::opt<bool>
    OptLevelOs("Os", cl::desc("Like -O2 but size-conscious. Similar to clang "
                              "-Os. Use -passes='default<Os>' for the new PM"));

static cl::opt<bool> OptLevelOz(
    "Oz",
    cl::desc("Like -O2 but optimize for code size above all else. Similar to "
             "clang -Oz. Use -passes='default<Oz>' for the new PM"));

static cl::opt<bool>
    OptLevelO3("O3", cl::desc("Optimization level 3. Similar to clang -O3. "
                              "Use -passes='default<O3>' for the new PM"));

static cl::opt<unsigned> CodeGenOptLevel(
    "codegen-opt-level",
    cl::desc("Override optimization level for codegen hooks, legacy PM only"));

static cl::opt<std::string>
    TargetTriple("mtriple", cl::desc("Override target triple for module"));

#if LLVM_VERSION_MAJOR < 16
static cl::opt<bool> DisableLoopUnrolling(
    "disable-loop-unrolling",
    cl::desc("Disable loop unrolling in all relevant passes"), cl::init(false));
#endif

static cl::opt<bool> EmitSummaryIndex("module-summary",
                                      cl::desc("Emit module summary index"),
                                      cl::init(false));

static cl::opt<bool> EmitModuleHash("module-hash", cl::desc("Emit module hash"),
                                    cl::init(false));

static cl::opt<bool>
    DisableSimplifyLibCalls("disable-simplify-libcalls",
                            cl::desc("Disable simplify-libcalls"));

static cl::list<std::string> DisableBuiltins(
    "disable-builtin",
    cl::desc("Disable specific target library builtin function"),
    cl::ZeroOrMore);

static cl::opt<bool> EnableDebugify(
    "enable-debugify",
    cl::desc(
        "Start the pipeline with debugify and end it with check-debugify"));

static cl::opt<bool> VerifyDebugInfoPreserve(
    "verify-debuginfo-preserve",
    cl::desc("Start the pipeline with collecting and end it with checking of "
             "debug info preservation."));

static cl::opt<bool>
    PrintBreakpoints("print-breakpoints-for-testing",
                     cl::desc("Print select breakpoints location for testing"));
#if LLVM_VERSION_MAJOR < 16
static cl::opt<bool>
    AnalyzeOnly("analyze", cl::desc("Only perform analysis, no optimization"
                                    "Legacy pass manager only."));

static cl::opt<std::string> ClDataLayout(
    "default-data-layout",
    cl::desc("data layout string to use if not specified by module"),
    cl::value_desc("layout-string"), cl::init(""));
#else
static cl::opt<std::string> ClDataLayout("data-layout",
                                         cl::desc("data layout string to use"),
                                         cl::value_desc("layout-string"),
                                         cl::init("")); 
#endif
static cl::opt<bool> PreserveBitcodeUseListOrder(
    "preserve-bc-uselistorder",
    cl::desc("Preserve use-list order when writing LLVM bitcode."),
    cl::init(true), cl::Hidden);

static cl::opt<bool> PreserveAssemblyUseListOrder(
    "preserve-ll-uselistorder",
    cl::desc("Preserve use-list order when writing LLVM assembly."),
    cl::init(false), cl::Hidden);

static cl::opt<bool>
    RunTwice("run-twice",
             cl::desc("Run all passes twice, re-using the same pass manager."),
             cl::init(false), cl::Hidden);

static cl::opt<bool> DiscardValueNames(
    "discard-value-names",
    cl::desc("Discard names from Value (other than GlobalValue)."),
    cl::init(false), cl::Hidden);

#if LLVM_VERSION_MAJOR < 15
static cl::opt<bool> Coroutines("enable-coroutines",
                                cl::desc("Enable coroutine passes."),
                                cl::init(false), cl::Hidden);
#endif

static cl::opt<bool> TimeTrace("time-trace", cl::desc("Record time trace"));

static cl::opt<unsigned> TimeTraceGranularity(
    "time-trace-granularity",
    cl::desc(
        "Minimum time granularity (in microseconds) traced by time profiler"),
    cl::init(500), cl::Hidden);

static cl::opt<std::string>
    TimeTraceFile("time-trace-file",
                  cl::desc("Specify time trace file destination"),
                  cl::value_desc("filename"));

static cl::opt<bool> RemarksWithHotness(
    "pass-remarks-with-hotness",
    cl::desc("With PGO, include profile count in optimization remarks"),
    cl::Hidden);
#if LLVM_VERSION_MAJOR < 16
static cl::opt<Optional<uint64_t>, false, remarks::HotnessThresholdParser>
    RemarksHotnessThreshold(
        "pass-remarks-hotness-threshold",
        cl::desc("Minimum profile count required for "
                 "an optimization remark to be output. "
                 "Use 'auto' to apply the threshold from profile summary"),
        cl::value_desc("N or 'auto'"), cl::init(0), cl::Hidden);
#else
static cl::opt<std::optional<uint64_t>, false, remarks::HotnessThresholdParser>
    RemarksHotnessThreshold(
        "pass-remarks-hotness-threshold",
        cl::desc("Minimum profile count required for "
                 "an optimization remark to be output. "
                 "Use 'auto' to apply the threshold from profile summary"),
        cl::value_desc("N or 'auto'"), cl::init(0), cl::Hidden);
#endif

static cl::opt<std::string>
    RemarksFilename("pass-remarks-output",
                    cl::desc("Output filename for pass remarks"),
                    cl::value_desc("filename"));

static cl::opt<std::string>
    RemarksPasses("pass-remarks-filter",
                  cl::desc("Only record optimization remarks from passes whose "
                           "names match the given regular expression"),
                  cl::value_desc("regex"));

static cl::opt<std::string> RemarksFormat(
    "pass-remarks-format",
    cl::desc("The format used for serializing remarks (default: YAML)"),
    cl::value_desc("format"), cl::init("yaml"));

// EDIT: This one is useless in LLVM < 15
static cl::list<std::string>
    PassPlugins("load-pass-plugin",
                cl::desc("Load passes from plugin library"));

static inline void addPass(legacy::PassManagerBase &PM, Pass *P) {
  // Add the pass to the pass manager...
  PM.add(P);

  // If we are verifying all of the intermediate steps, add the verifier...
  if (VerifyEach)
    PM.add(createVerifierPass());
}

// EDIT: legacy function
/// This routine adds legacy optimization passes based on selected optimization
/// level, OptLevel.
///
/// OptLevel - Optimization Level
static void AddOptimizationPasses(legacy::PassManagerBase &MPM,
                                  legacy::FunctionPassManager &FPM,
                                  TargetMachine *TM, uint64_t OptLevel,
                                  uint64_t SizeLevel) {
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

#if LLVM_VERSION_MAJOR < 16
  // Otherwise it breaks with multiple definitions of DisableLoopUnrolling
  Builder.DisableUnrollLoops = (DisableLoopUnrolling.getNumOccurrences() > 0)
                                   ? DisableLoopUnrolling
                                   : OptLevel == 0;
#endif
  Builder.LoopVectorize = OptLevel > 1 && SizeLevel < 2;
  // When #pragma vectorize is on for SLP, do the same as above
  Builder.SLPVectorize = OptLevel > 1 && SizeLevel < 2;
  // See
  // https://github.com/llvm/llvm-project/commit/99c47d9e3113a917ea2f84f27e57f2ea3da4fc8c
  // Since opt no longer supports to run default (O0/O1/O2/O3/Os/Oz)
  // pipelines using the legacy PM, there are no in-tree uses of
  // TargetMachine::adjustPassManager remaining. This patch removes the
  // no longer used adjustPassManager functions.

  // Add target-specific passes that need to run as early as possible.
#if LLVM_VERSION_MAJOR < 16
  if (TM) {
    TM->adjustPassManager(Builder);
  }
#endif
#if LLVM_VERSION_MAJOR < 15
  if (Coroutines) {
    addCoroutinePassesToExtensionPoints(Builder);
  }
#endif
  Builder.populateFunctionPassManager(FPM);
  Builder.populateModulePassManager(MPM);
}

// EDIT: kept for compatibility with legacy code
static void AddStandardLinkPasses(legacy::PassManagerBase &PM) {
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
#else
  // Add LTO like passes manually, this is a workaround for the fact that
  // populateLTOPassManager had been purged. 
  PM.add(createConstantMergePass());
  PM.add(createGlobalDCEPass());
  PM.add(createGlobalOptimizerPass());
  PM.add(createIPSCCPPass());
  PM.add(createStripDeadPrototypesPass());
  PM.add(createDeadCodeEliminationPass());
  PM.add(createAggressiveDCEPass());
  PM.add(createStripDeadDebugInfoPass());
#endif
  return;
}

//===----------------------------------------------------------------------===//
// CodeGen-related helper functions.

// EDIT: kept legacy version with OptLevels
static CodeGenOpt::Level GetCodeGenOptLevel() {
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

// EDIT: specified llvm::TargetOptions as it is ambiguous
// Returns the TargetMachine instance or zero if no triple is provided.
static TargetMachine *GetTargetMachine(Triple TheTriple, StringRef CPUStr,
                                       StringRef FeaturesStr,
                                       const llvm::TargetOptions &Options) {
  std::string Error;
  const Target *TheTarget =
      TargetRegistry::lookupTarget(codegen::getMArch(), TheTriple, Error);
  // Some modules don't specify a triple, and this is okay.
  if (!TheTarget) {
    return nullptr;
  }

  return TheTarget->createTargetMachine(
      TheTriple.getTriple(), codegen::getCPUStr(), codegen::getFeaturesStr(),
      Options, codegen::getExplicitRelocModel(),
      codegen::getExplicitCodeModel(), GetCodeGenOptLevel());
}

struct TimeTracerRAII {
  TimeTracerRAII(StringRef ProgramName) {
    if (TimeTrace)
      timeTraceProfilerInitialize(TimeTraceGranularity, ProgramName);
  }
  ~TimeTracerRAII() {
    if (TimeTrace) {
      if (auto E = timeTraceProfilerWrite(TimeTraceFile, OutputFilename)) {
        handleAllErrors(std::move(E), [&](const StringError &SE) {
          errs() << SE.getMessage() << "\n";
        });
        return;
      }
      timeTraceProfilerCleanup();
    }
  }
};

// For use in NPM transition. Currently this contains most codegen-specific
// passes. Remove passes from here when porting to the NPM.
// TODO: use a codegen version of PassRegistry.def/PassBuilder::is*Pass() once
// it exists.
static bool shouldPinPassToLegacyPM(StringRef Pass) {
  std::vector<StringRef> PassNameExactToIgnore = {
      "nvvm-reflect",
      "nvvm-intr-range",
      "amdgpu-simplifylib",
      "amdgpu-usenative",
      "amdgpu-promote-alloca",
      "amdgpu-promote-alloca-to-vector",
      "amdgpu-lower-kernel-attributes",
      "amdgpu-propagate-attributes-early",
      "amdgpu-propagate-attributes-late",
      "amdgpu-unify-metadata",
      "amdgpu-printf-runtime-binding",
      "amdgpu-always-inline"};
  if (llvm::is_contained(PassNameExactToIgnore, Pass))
    return false;

  std::vector<StringRef> PassNamePrefix = {
      "x86-",    "xcore-", "wasm-",  "systemz-", "ppc-",    "nvvm-",
      "nvptx-",  "mips-",  "lanai-", "hexagon-", "bpf-",    "avr-",
      "thumb2-", "arm-",   "si-",    "gcn-",     "amdgpu-", "aarch64-",
      "amdgcn-", "polly-", "riscv-", "dxil-"};
  std::vector<StringRef> PassNameContain = {"ehprepare"};
  std::vector<StringRef> PassNameExact = {"safe-stack",
                                          "cost-model",
                                          "codegenprepare",
                                          "interleaved-load-combine",
                                          "unreachableblockelim",
                                          "verify-safepoint-ir",
                                          "atomic-expand",
                                          "expandvp",
                                          "hardware-loops",
                                          "mve-tail-predication",
                                          "interleaved-access",
                                          "global-merge",
                                          "pre-isel-intrinsic-lowering",
                                          "expand-reductions",
                                          "indirectbr-expand",
                                          "generic-to-nvvm",
                                          "expandmemcmp",
                                          "loop-reduce",
                                          "lower-amx-type",
                                          "pre-amx-config",
                                          "lower-amx-intrinsics",
                                          "polyhedral-info",
                                          "print-polyhedral-info",
                                          "replace-with-veclib",
                                          "jmc-instrument",
                                          "dot-regions",
                                          "dot-regions-only",
                                          "view-regions",
                                          "view-regions-only",
                                          "select-optimize",
                                          "expand-large-div-rem",
                                          "structurizecfg",
                                          "fix-irreducible",
                                          "expand-large-fp-convert"};
  for (const auto &P : PassNamePrefix)
    if (Pass.startswith(P))
      return true;
  for (const auto &P : PassNameContain)
    if (Pass.contains(P))
      return true;
  return llvm::is_contained(PassNameExact, Pass);
}

// For use in NPM transition.
static bool shouldForceLegacyPM() {
  for (const auto &P : PassList) {
    StringRef Arg = P->getPassArgument();
    if (shouldPinPassToLegacyPM(Arg))
      return true;
  }
  return false;
}

// ----------------------------------------------------------------------------
// EDIT: end of opt.cpp definitions
// ----------------------------------------------------------------------------
// EDIT: custom helper
static void resetOptOptions() {
  auto &optionMap = cl::getRegisteredOptions();
  for (auto &o : optionMap) {
    o.second->reset();
  }
  return;
}

#endif /* end of include guard: LIB_VERSIONING_COMPILER_CLANG_LLVM_OPT_UTILS   \
        */
