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
#ifndef LIB_VERSIONING_COMPILER_CLANG_LLVM_LLVM_INSTANCE_MANAGER
#define LIB_VERSIONING_COMPILER_CLANG_LLVM_LLVM_INSTANCE_MANAGER

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/InitializePasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#if LLVM_VERSION_MAJOR >= 14
#include "llvm/MC/TargetRegistry.h"
#else
#include "llvm/Support/TargetRegistry.h"
#endif

#include <filesystem>
#include <string>
/** LLVMInstanceManager is a lazy-initialized singleton.
 *
 * It performs LLVM initialization at the first request and calls
 * llvm_shutdown() at the end of the program.
 */
class LLVMInstanceManager {

public:
  static std::shared_ptr<LLVMInstanceManager> getInstance()
  {
    if (!instance) {
      instance = std::shared_ptr<LLVMInstanceManager>(
                  new LLVMInstanceManager());
    }
    return instance;
  }

private:
  LLVMInstanceManager();

  static std::shared_ptr<LLVMInstanceManager> instance;

public:
  ~LLVMInstanceManager();

  std::filesystem::path getClangExePath() const;

  std::shared_ptr<llvm::Triple> getDefaultTriple() const;

  LLVMInstanceManager(const LLVMInstanceManager&) = delete;

  void operator=(const LLVMInstanceManager&) = delete;

private:
  inline void initializeLLVM();

  std::filesystem::path clangExeStr;

  std::shared_ptr<llvm::Triple> triple;

};

#endif /* end of include guard: LIB_VERSIONING_COMPILER_CLANG_LLVM_LLVM_INSTANCE_MANAGER */
