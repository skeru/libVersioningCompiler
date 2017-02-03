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
#ifndef LIB_VERSIONING_COMPILER_CLANG_LLVM_LLVM_INSTANCE_MANAGER
#define LIB_VERSIONING_COMPILER_CLANG_LLVM_LLVM_INSTANCE_MANAGER

#include "llvm/ADT/Triple.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"

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

  std::string getClangExePath() const;

  std::shared_ptr<llvm::Triple> getDefaultTriple() const;

  LLVMInstanceManager(const LLVMInstanceManager&) = delete;

  void operator=(const LLVMInstanceManager&) = delete;

private:
  inline void initializeLLVM();

  std::string clangExeStr;

  std::shared_ptr<llvm::Triple> triple;

};

#endif /* end of include guard: LIB_VERSIONING_COMPILER_CLANG_LLVM_LLVM_INSTANCE_MANAGER */
