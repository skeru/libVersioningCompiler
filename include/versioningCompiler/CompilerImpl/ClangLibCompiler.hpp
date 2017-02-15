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
#ifndef LIB_VERSIONING_COMPILER_CLANG_LIB_COMPILER_HPP
#define LIB_VERSIONING_COMPILER_CLANG_LIB_COMPILER_HPP

#include "versioningCompiler/Compiler.hpp"
#include "versioningCompiler/CompilerImpl/ClangLLVM/FileLogDiagnosticConsumer.hpp"
#include "versioningCompiler/CompilerImpl/ClangLLVM/LLVMInstanceManager.hpp"

#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/CompilerInstance.h"

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Target/TargetMachine.h"

#include <string>
#include <vector>

namespace vc {
/** Compiler implementation exploiting the clang-as-a-library paradigm.
 * It supports the default llvm optimizer optimizations.
 */
class ClangLibCompiler : public Compiler {

public:
  ClangLibCompiler();

  ClangLibCompiler(const std::string &compilerID,
                   const std::string &libWorkingDir,
                   const std::string &log = "");

  inline virtual ~ClangLibCompiler() {}

  virtual bool hasOptimizer() const override;

  virtual std::string generateIR(const std::string &src,
                                 const std::string &func,
                                 const std::string &versionID,
                                 const std::list<Option> options) const
  override;

  virtual std::string runOptimizer(const std::string &src_IR,
                                   const std::string &versionID,
                                   const std::list<Option> options) const
  override;

  virtual std::string generateBin(const std::string &src,
                                  const std::string &func,
                                  const std::string &versionID,
                                  const std::list<Option> options) const
  override;

  virtual void *loadSymbol(const std::string &bin,
                           const std::string &func) const override;

  virtual std::string getOptionString(const Option &o) const override;

private:
  inline
  std::vector<std::string> getArgV(const std::list<Option> optionList) const;

private:
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> _diagnosticIDs;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> _diagnosticOptions;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine> _diagEngine;
  std::shared_ptr<FileLogDiagnosticConsumer> _diagConsumer;
  std::shared_ptr<LLVMInstanceManager> _llvmManager;

/** \brief mutex to regulate exclusive access to static command line options
 * during optimizer option parsing and processing.
 */
  static std::mutex opt_parse_mtx;

};
} /* end namespace vc */

#endif /* end of include guard: LIB_VERSIONING_COMPILER_CLANG_LIB_COMPILER_HPP */
