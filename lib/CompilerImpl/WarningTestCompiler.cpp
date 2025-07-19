/* Copyright 2017-2018 Politecnico di Milano.
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

#include "versioningCompiler/CompilerImpl/WarningTestCompiler.hpp"
#include <fstream>

using namespace vc;

// ----------------------------------------------------------------------------
// --------------------------- detailed constructor ---------------------------
// ----------------------------------------------------------------------------
WarningTestCompiler::WarningTestCompiler(const std::string &compilerID,
                               const std::filesystem::path &compilerCallString,
                               const std::filesystem::path &libWorkingDir,
                               const std::filesystem::path &mylogfile,
                               const std::filesystem::path &installDir,
                               bool supportsIR)
    : SystemCompiler(compilerID, compilerCallString, libWorkingDir, mylogfile,
               installDir, supportsIR), hasTruncatedLog(true) {}

// ----------------------------------------------------------------------------
// --------------- print a command to log file and execute it -----------------
// ----------------------------------------------------------------------------
void WarningTestCompiler::log_exec(const std::string &command) const {
  FILE *output;
  std::ofstream logstream;
  std::string _command = command;
  char buf[256];
  if (!logFile.empty()) {
    _command = _command + " 2>&1";
    lockMutex(logFile);
    if (hasTruncatedLog) {
      logstream.open(logFile, std::ofstream::out | std::ofstream::trunc);
      hasTruncatedLog = false;
    } else {
      logstream.open(logFile, std::ofstream::app);
    }
    logstream << _command << std::endl;
  }
  output = popen(_command.c_str(), "r");
  if (!logFile.empty()) {
    while (fgets(buf, sizeof(buf), output) != 0) {
      logstream << buf;
    }
    logstream << std::endl;
    logstream.close();
    unlockMutex(logFile);
  }
  pclose(output);
  return;
}

// ----------------------------------------------------------------------------
// ----------------------- print a string to log file -------------------------
// ----------------------------------------------------------------------------
void WarningTestCompiler::log_string(const std::string &command) const {
  std::ofstream logstream;
  if (!logFile.empty()) {
    lockMutex(logFile);
    if (hasTruncatedLog) {
      logstream.open(logFile, std::ofstream::out | std::ofstream::trunc);
      hasTruncatedLog = false;
    } else {
      logstream.open(logFile, std::ofstream::app);
    }
    logstream << command << std::endl;
    logstream.close();
    unlockMutex(logFile);
  }
  return;
}
