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
#ifndef LIB_VERSIONING_COMPILER_JIT_LIB_COMPILER_HPP
#define LIB_VERSIONING_COMPILER_JIT_LIB_COMPILER_HPP

#include "versioningCompiler/Compiler.hpp"
#include "versioningCompiler/CompilerImpl/ClangLLVM/FileLogDiagnosticConsumer.hpp"
#include "versioningCompiler/CompilerImpl/ClangLLVM/LLVMInstanceManager.hpp"

#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Target/TargetMachine.h"

#include <filesystem>
#include <string>
#include <vector>

namespace vc {
/** Compiler implementation exploiting the clang-as-a-library and llvm ORC JIT
 * paradigm. It supports the default llvm  optimizations.
 */
class JITCompiler : public Compiler {

private:
  // ORC JIT initialization objects
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> _diagnosticIDs;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> _diagnosticOptions;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine> _diagEngine;
  std::shared_ptr<FileLogDiagnosticConsumer> _diagConsumer;
  std::shared_ptr<LLVMInstanceManager> _llvmManager;
  std::unique_ptr<llvm::orc::ExecutionSession> _ES;
  llvm::orc::JITTargetMachineBuilder _JTMB;
  llvm::orc::ThreadSafeContext _tsctx;
  llvm::orc::MangleAndInterner _mangle;
  llvm::DataLayout _dataLayout;
  llvm::orc::JITDylib &_mainJD;

  /** \brief mutex to regulate exclusive access to static command line options
   * during optimizer option parsing and processing.
   */
  static std::mutex opt_parse_mtx;

public:
  // Set of maps used to keep a state of the various versions requesting for JIT
  // functionalities.
  std::map<std::string, std::unique_ptr<llvm::Module>>
      _modules_map; // source string gets placed here rather than the module. If
                    // it is present, generate the module
  std::map<std::string, llvm::orc::ResourceTrackerSP> _resource_tracker_map;
  std::map<std::string, bool> _isloaded_map;
  std::map<std::string, std::unique_ptr<llvm::orc::IRCompileLayer>> _layer_map;
  std::map<std::string, std::unique_ptr<llvm::orc::RTDyldObjectLinkingLayer>>
      _obj_map;

public:
  JITCompiler();

  JITCompiler(const std::string &compilerID,
              const std::filesystem::path &libWorkingDir,
              const std::filesystem::path &log);

  inline ~JITCompiler() {
    for (auto it = _resource_tracker_map.begin();
         it != _resource_tracker_map.end(); ++it) {
      handleAllErrors(
          std::move(it->second->remove()), [&](llvm::ErrorInfoBase &EIB) {
            llvm::errs() << "Error while deleting a resource tracker: "
                         << EIB.message() << "\n";
          });
    }
#if LLVM_VERSION_MAJOR >= 14
    handleAllErrors(std::move(this->_ES->removeJITDylib(this->_mainJD)),
                    [&](llvm::ErrorInfoBase &EIB) {
                      llvm::errs() << "Error while removing the JITDylib: "
                                   << EIB.message() << "\n";
                    });
#endif
  }

  bool hasOptimizer() const override;

  std::filesystem::path
  generateIR(const std::vector<std::filesystem::path> &src,
             const std::vector<std::string> &func, const std::string &versionID,
             const opt_list_t options)

      override;

  std::filesystem::path runOptimizer(const std::filesystem::path &src_IR,
                                     const std::string &versionID,
                                     const opt_list_t options) const override;

  std::filesystem::path
  generateBin(const std::vector<std::filesystem::path> &src,
              const std::vector<std::string> &func,
              const std::string &versionID, const opt_list_t options) override;

  void releaseSymbol(void **handler) override;

  std::vector<void *> loadSymbols(const std::filesystem::path &bin,
                                  const std::vector<std::string> &func,
                                  void **handler) override;

  std::string getOptionString(const Option &o) const override;

  // JIT specific methods
  void addModule(std::unique_ptr<llvm::Module> m, const std::string &versionID);

  llvm::Expected<llvm::JITEvaluatedSymbol>
  findSymbol(const std::string Name, const std::string &versionID);

private:
  inline std::vector<std::string> getArgV(const opt_list_t optionList) const;
};
} /* end namespace vc */

#endif /* end of include guard: LIB_VERSIONING_COMPILER_JIT_LIB_COMPILER_HPP   \
        */
