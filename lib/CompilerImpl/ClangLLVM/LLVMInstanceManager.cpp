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
#include "versioningCompiler/CompilerImpl/ClangLLVM/LLVMInstanceManager.hpp"

// ---------------------------------------------------------------------------
// ------------------------------- constructor -------------------------------
// ---------------------------------------------------------------------------
LLVMInstanceManager::LLVMInstanceManager()
{
  initializeLLVM();
  return;
}

// Lazy initialized instance of LLVMInstanceManager
std::shared_ptr<LLVMInstanceManager> LLVMInstanceManager::instance = nullptr;

// ---------------------------------------------------------------------------
// ------------------------------- destructor --------------------------------
// ---------------------------------------------------------------------------
LLVMInstanceManager::~LLVMInstanceManager()
{
  llvm::llvm_shutdown();
  return;
}

// ---------------------------------------------------------------------------
// --------------------------- get clang exe path ----------------------------
// ---------------------------------------------------------------------------
std::string LLVMInstanceManager::getClangExePath() const
{
  return clangExeStr;
}

// ---------------------------------------------------------------------------
// -------------------- get default target triple object ---------------------
// ---------------------------------------------------------------------------
std::shared_ptr<llvm::Triple> LLVMInstanceManager::getDefaultTriple() const
{
  return triple;
}

// ---------------------------------------------------------------------------
// ----------------------------- initializeLLVM -----------------------------
// ---------------------------------------------------------------------------
void LLVMInstanceManager::initializeLLVM()
{
  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllDisassemblers();
  if (llvm::InitializeNativeTarget()) { // check whether it has native target or not
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetDisassembler();
  }

  // Initialize passes
  llvm::PassRegistry& passRegistry = *llvm::PassRegistry::getPassRegistry();
  llvm::initializeCore(passRegistry);
  llvm::initializeCoroutines(passRegistry);
  llvm::initializeScalarOpts(passRegistry);
  llvm::initializeObjCARCOpts(passRegistry);
  llvm::initializeVectorization(passRegistry);
  llvm::initializeIPO(passRegistry);
  llvm::initializeAnalysis(passRegistry);
  llvm::initializeTransformUtils(passRegistry);
  llvm::initializeInstCombine(passRegistry);
  llvm::initializeInstrumentation(passRegistry);
  llvm::initializeTarget(passRegistry);
  // For codegen passes, only passes that do IR to IR transformation are
  // supported.
  llvm::initializeCodeGenPreparePass(passRegistry);
  llvm::initializeAtomicExpandPass(passRegistry);
  llvm::initializeRewriteSymbolsLegacyPassPass(passRegistry);
  llvm::initializeWinEHPreparePass(passRegistry);
  llvm::initializeDwarfEHPreparePass(passRegistry);
  llvm::initializeSafeStackPass(passRegistry);
  llvm::initializeSjLjEHPreparePass(passRegistry);
  llvm::initializePreISelIntrinsicLoweringLegacyPassPass(passRegistry);
  llvm::initializeGlobalMergePass(passRegistry);
  llvm::initializeInterleavedAccessPass(passRegistry);
  llvm::initializeCountingFunctionInserterPass(passRegistry);
  llvm::initializeUnreachableBlockElimLegacyPassPass(passRegistry);

  // remember default target triple
  auto tripleStr = llvm::sys::getProcessTriple();
  triple = std::make_shared<llvm::Triple>(tripleStr);

  auto clangPath = llvm::sys::findProgramByName("clang");
  if (clangPath) {
      clangExeStr = std::string(*clangPath);
  }
  return;
}
