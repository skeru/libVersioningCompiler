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
#ifndef LIB_VERSIONING_COMPILER_CLANG_LLVM_FILE_LOG_DIAG_CONSUMER_HPP
#define LIB_VERSIONING_COMPILER_CLANG_LLVM_FILE_LOG_DIAG_CONSUMER_HPP

#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/TextDiagnostic.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include <fstream>

namespace clang {
  class DiagnosticOptions;
  class LangOptions;
  class TextDiagnostic;
}

namespace vc {
/// This is basically a TextDiagnosticPrinter who writes into a std::fstream
/// instead of writing to the terminal
class FileLogDiagnosticConsumer : public clang::DiagnosticConsumer {

protected:
  /// The log file name currently used. Leave "" to disable logging
  std::string _logFileName;

  /// The output file stream. It will be used as input/output/append mode
  std::fstream _log;

  llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts;

  /// \brief Handle to the currently active text diagnostic emitter.
  std::unique_ptr<clang::TextDiagnostic> TextDiag;

  /// A string to prefix to error messages.
  std::string Prefix;
  const clang::LangOptions *LangOpts;

  /// Buffer to store message queue
  llvm::SmallVector<std::string, 8> Entries;

  std::string MainFilename;

public:
  FileLogDiagnosticConsumer(llvm::StringRef LogFileName,
                            clang::DiagnosticOptions *Diags);

  FileLogDiagnosticConsumer(const FileLogDiagnosticConsumer&) = delete;

  virtual ~FileLogDiagnosticConsumer() override;

  inline void setLogFileName(std::string fileName) {
    _logFileName = std::move(fileName);
    if (_log.is_open()) {
      _log.close();
    }
    if (_logFileName != "") {
      _log.open(_logFileName,
                std::fstream::in | std::fstream::out | std::fstream::app);
    }
    return;
  }

  void BeginSourceFile(const clang::LangOptions &LO,
                       const clang::Preprocessor *PP) override;

  void EndSourceFile() override;

  void HandleDiagnostic(clang::DiagnosticsEngine::Level DiagLevel,
                        const clang::Diagnostic &Info) override;

  /// setPrefix - Set the diagnostic printer prefix string, which will be
  /// printed at the start of any diagnostics. If empty, no prefix string is
  /// used.
  void setPrefix(std::string Value) { Prefix = std::move(Value); }
};

} // end namespace vc

#endif /* end of include guard: LIB_VERSIONING_COMPILER_CLANG_LLVM_FILE_LOG_DIAG_CONSUMER_HPP */
