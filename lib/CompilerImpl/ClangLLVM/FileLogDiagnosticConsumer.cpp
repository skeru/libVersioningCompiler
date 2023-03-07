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
#include "versioningCompiler/CompilerImpl/ClangLLVM/FileLogDiagnosticConsumer.hpp"

#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"

#include <fstream>

using namespace clang;
using namespace vc;

// ----------------------------------------------------------------------------
// --------------------------- detailed constructor ---------------------------
// ----------------------------------------------------------------------------
FileLogDiagnosticConsumer::FileLogDiagnosticConsumer(
    std::filesystem::path LogFileName, DiagnosticOptions *diags)
    : LangOpts(nullptr), DiagOpts(diags), _logFileName(LogFileName.string()),
      _log() {}

// ----------------------------------------------------------------------------
// --------------------------- default destructor ---------------------------
// ----------------------------------------------------------------------------
FileLogDiagnosticConsumer::~FileLogDiagnosticConsumer() {
  if (_log.is_open()) {
    _log.close();
  }
  return;
}

// ----------------------------------------------------------------------------
// ---------------------------- begin source file -----------------------------
// ----------------------------------------------------------------------------
void FileLogDiagnosticConsumer::BeginSourceFile(const clang::LangOptions &LO,
                                                const clang::Preprocessor *PP) {
  LangOpts = &LO;
  return;
}

// ----------------------------------------------------------------------------
// ------------------------------ get level name ------------------------------
// ----------------------------------------------------------------------------
static StringRef getLevelName(DiagnosticsEngine::Level Level) {
  switch (Level) {
  case DiagnosticsEngine::Ignored:
    return "ignored";
  case DiagnosticsEngine::Remark:
    return "remark";
  case DiagnosticsEngine::Note:
    return "note";
  case DiagnosticsEngine::Warning:
    return "warning";
  case DiagnosticsEngine::Error:
    return "error";
  case DiagnosticsEngine::Fatal:
    return "fatal error";
  }
  llvm_unreachable("Invalid DiagnosticsEngine level!");
  return "";
}

// ----------------------------------------------------------------------------
// ------------------------- print diagnostic options -------------------------
// ----------------------------------------------------------------------------
/// \brief Print any diagnostic option information to a raw_ostream.
///
/// This implements all of the logic for adding diagnostic options to a message
/// (via logStream). Each relevant option is comma separated and all are
/// enclosed in the standard bracketing: " [...]".
static void printDiagnosticOptions(llvm::raw_ostream &logStream,
                                   DiagnosticsEngine::Level Level,
                                   const Diagnostic &Info,
                                   const DiagnosticOptions &DiagOpts) {
  bool Started = false;
  if (DiagOpts.ShowOptionNames) {
    // Handle special cases for non-warnings early.
    if (Info.getID() == diag::fatal_too_many_errors) {
      logStream << " [-ferror-limit=]";
      return;
    }

    // The code below is somewhat fragile because we are essentially trying to
    // report to the user what happened by inferring what the diagnostic engine
    // did. Eventually it might make more sense to have the diagnostic engine
    // include some "why" information in the diagnostic.

    // If this is a warning which has been mapped to an error by the user (as
    // inferred by checking whether the default mapping is to an error) then
    // flag it as such. Note that diagnostics could also have been mapped by a
    // pragma, but we don't currently have a way to distinguish this.
    if (Level == DiagnosticsEngine::Error &&
        DiagnosticIDs::isBuiltinWarningOrExtension(Info.getID()) &&
        !DiagnosticIDs::isDefaultMappingAsError(Info.getID())) {
      logStream << " [-Werror";
      Started = true;
    }

    StringRef Opt = DiagnosticIDs::getWarningOptionForDiag(Info.getID());
    if (!Opt.empty()) {
      logStream << (Started ? "," : " [")
                << (Level == DiagnosticsEngine::Remark ? "-R" : "-W") << Opt;
      StringRef OptValue = Info.getDiags()->getFlagValue();
      if (!OptValue.empty())
        logStream << "=" << OptValue;
      Started = true;
    }
  }

  // If the user wants to see category information, include it too.
  if (DiagOpts.ShowCategories) {
    unsigned DiagCategory =
        DiagnosticIDs::getCategoryNumberForDiag(Info.getID());
    if (DiagCategory) {
      logStream << (Started ? "," : " [");
      Started = true;
      if (DiagOpts.ShowCategories == 1)
        logStream << DiagCategory;
      else {
        assert(DiagOpts.ShowCategories == 2 && "Invalid ShowCategories value");
        logStream << DiagnosticIDs::getCategoryNameFromID(DiagCategory).str();
      }
    }
  }
  if (Started) {
    logStream << ']';
  }
  return;
}

// ----------------------------------------------------------------------------
// ----------------------------- end source file ------------------------------
// ----------------------------------------------------------------------------
void FileLogDiagnosticConsumer::EndSourceFile() {
  // We emit all the diagnostics in EndSourceFile. However, we don't emit any
  // entry if no diagnostics were present.
  //
  // Note that DiagnosticConsumer has no "end-of-compilation" callback, so we
  // will miss any diagnostics which are emitted after and outside the
  // translation unit processing.
  if (Entries.empty()) {
    return;
  }

  if (!_log.is_open() && !_logFileName.empty()) {
    _log.open(_logFileName,
              std::fstream::in | std::fstream::out | std::fstream::app);
  }

  if (!_log.is_open()) {
    // no log required - no need to handle
    return;
  }

  for (auto &e : Entries) {
    _log << std::endl << e;
  }
  _log << std::endl;
  _log.flush();
  Entries.clear();
  return;
}

// ----------------------------------------------------------------------------
// ---------------------------- handle diagnostic -----------------------------
// ----------------------------------------------------------------------------
void FileLogDiagnosticConsumer::HandleDiagnostic(DiagnosticsEngine::Level Level,
                                                 const Diagnostic &Info) {
  // Initialize the main file name, if we haven't already fetched it.
  if (MainFilename.empty() && Info.hasSourceManager()) {
    const SourceManager &SM = Info.getSourceManager();
    FileID FID = SM.getMainFileID();
    if (FID.isValid()) {
      const FileEntry *FE = SM.getFileEntryForID(FID);
#if LLVM_VERSION_MAJOR < 15
      if (FE && FE->isValid())
#else
      if (FE)
#endif
        MainFilename = std::string(FE->getName());
    }
  }

  // Write to a temporary string to ensure atomic write of diagnostic object.
  SmallString<512> Msg;
  llvm::raw_svector_ostream stringBuffer(Msg);

  // Render the diagnostic message into a temporary buffer eagerly. We'll use
  // this later as we print out the diagnostic to the stringBuffer.
  SmallString<100> OutStr;
  Info.FormatDiagnostic(OutStr);

  llvm::raw_svector_ostream DiagMessageStream(OutStr);
  printDiagnosticOptions(DiagMessageStream, Level, Info, *DiagOpts);

  // Keeps track of the starting position of the location
  // information (e.g., "foo.c:10:4:") that precedes the error
  // message. We use this information to determine how long the
  // file+line+column number prefix is.
  uint64_t StartOfLocationInfo = stringBuffer.tell();

  if (!Prefix.empty()) {
    stringBuffer << Prefix << ": ";
  }

  // Use a dedicated, simpler path for diagnostics without a valid location.
  // This is important as if the location is missing, we may be emitting
  // diagnostics in a context that lacks language options, a source manager, or
  // other infrastructure necessary when emitting more rich diagnostics.
  if (!Info.getLocation().isValid()) {
    TextDiagnostic::printDiagnosticLevel(stringBuffer, Level,
                                         DiagOpts->ShowColors);
    TextDiagnostic::printDiagnosticMessage(
        stringBuffer, Level, DiagMessageStream.str(),
        stringBuffer.tell() - StartOfLocationInfo, DiagOpts->MessageLength,
        DiagOpts->ShowColors);
  }

  // store the string
  Entries.push_back(stringBuffer.str().str());

  // Default implementation (Warnings/errors count).
  DiagnosticConsumer::HandleDiagnostic(Level, Info);

  return;
}
