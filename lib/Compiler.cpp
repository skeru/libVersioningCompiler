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
#include "versioningCompiler/Compiler.hpp"
#include <cstdio>
#include <dlfcn.h> // needed for loadSymbol
#include <fstream>
#include <sys/stat.h>

using namespace vc;

// ----------------------------------------------------------------------------
// ----------------------- detailed default constructor -----------------------
// ----------------------------------------------------------------------------
Compiler::Compiler(const std::string &compilerID,
                   const std::string &compilerCallString,
                   const std::string &libWorkingDir,
                   const std::string &log,
                   const std::string &installDir,
                   bool supportIR) :
                                    id(compilerID),
                                    callString(compilerCallString),
                                    logFile(log),
                                    libWorkingDirectory(libWorkingDir),
                                    installDirectory(installDir),
                                    hasSupportIR(supportIR)
{
  addReferenceToLogFile(logFile);
}

// ----------------------------------------------------------------------------
// ------------------------- get compiler identifier --------------------------
// ----------------------------------------------------------------------------
std::string Compiler::getId() const
{
  return id;
}

// ----------------------------------------------------------------------------
// ------------------- check if compiler supports LLVM-IR ---------------------
// ----------------------------------------------------------------------------
bool Compiler::hasIRSupport() const
{
  return hasSupportIR;
}

// ----------------------------------------------------------------------------
// --------------- print a command to log file and execute it -----------------
// ----------------------------------------------------------------------------
void Compiler::log_exec(const std::string &command) const
{
  FILE *output;
  std::ofstream logstream;
  std::string _command = command;
  char buf[256];
  if (logFile != "") {
    _command = _command + " 2>&1";
    lockMutex(logFile);
    logstream.open(logFile, std::ofstream::app);
    logstream << _command << std::endl;
  }
  output = popen(_command.c_str(), "r");
  if (logFile != "") {
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
void Compiler::log_string(const std::string &command) const
{
  std::ofstream logstream;
  if (logFile != "") {
    lockMutex(logFile);
    logstream.open(logFile, std::ofstream::app);
    logstream << command << std::endl;
    logstream.close();
    unlockMutex(logFile);
  }
  return;
}

// ---------------------------------------------------------------------------
// ------------------------------- loadSymbol --------------------------------
// ---------------------------------------------------------------------------
void Compiler::releaseSymbol(void ** handler, void ** symbol) const
{
  if (dlclose(*handler)) {
    char* error = dlerror();
    std::string error_str(error);
    log_string(error_str);
  }
  *handler = nullptr;
  *symbol = nullptr;
  return;
}

// ---------------------------------------------------------------------------
// ------------------------------- loadSymbol --------------------------------
// ---------------------------------------------------------------------------
void* Compiler::loadSymbol(const std::string &bin,
                           const std::string &func,
                           void ** handler) const
{
  void *symbol = nullptr;
  if (exists(bin)) {
    *handler = dlopen(bin.c_str(), RTLD_NOW);
  } else {
    std::string error_str = "cannot load symbol from " + bin +
                            " : file not found";
    log_string(error_str);
    return symbol;
  }
  if (*handler) {
    symbol = dlsym(*handler, func.c_str());
    if (!symbol) {
      std::string error_str = "cannot load symbol from " + bin +
                              " : symbol not found";
      log_string(error_str);
    }
  } else {
    char* error = dlerror();
    std::string error_str(error);
    log_string(error_str);
  }
  return symbol;
}

// ----------------------------------------------------------------------------
// -------------------------- check file existence ----------------------------
// ----------------------------------------------------------------------------
bool Compiler::exists(const std::string &name)
{
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

// ----------------------------------------------------------------------------
// ---------------------- compose intermediate file name ----------------------
// ----------------------------------------------------------------------------
std::string Compiler::getBitcodeFileName(const std::string &versionID) const
{
  std::string filename = libWorkingDirectory;
  filename = filename + "/";
  filename = filename + "IR_";
  filename = filename + versionID;
  filename = filename + ".bc";
  return filename;
}

// ----------------------------------------------------------------------------
// ----------------- compose optimized intermediate file name -----------------
// ----------------------------------------------------------------------------
std::string Compiler::getOptBitcodeFileName(const std::string &versionID) const
{
  const std::string filename = libWorkingDirectory
                               + "/"
                               "opt_IR_"
                               + versionID
                               + ".bc";
  return filename;
}

// ----------------------------------------------------------------------------
// ------------------ compose binary shared object file name ------------------
// ----------------------------------------------------------------------------
std::string Compiler::getSharedObjectFileName(const std::string &versionID) const
{
  const std::string filename = libWorkingDirectory
                               + "/"
                               "lib"
                               + versionID
                               + ".so";
  return filename;
}

// ----------------------------------------------------------------------------
// --------------- notification of unsupported feature request ----------------
// ----------------------------------------------------------------------------
void Compiler::unsupported(const std::string &message) const
{
  const std::string error_string = "Required Compiler unsupported feature:\n\t"
                                   + message;
  log_string(error_string);
  return;
}

// ----------------------------------------------------------------------------
// --------------------- static mutex map initialization ----------------------
// ----------------------------------------------------------------------------
std::map<std::string, std::pair<uint64_t, std::shared_ptr<std::mutex>>>
Compiler::log_access_mtx_map;

// ----------------------------------------------------------------------------
// ------------ static initialization of mutex to access mutex map ------------
// ----------------------------------------------------------------------------
std::mutex Compiler::mtx_map_mtx;

// ----------------------------------------------------------------------------
// ---------------- static mutex map accessors : add reference ----------------
// ----------------------------------------------------------------------------
void Compiler::addReferenceToLogFile(const std::string &logFileName)
{
  if (logFileName == "") {
    return;
  }
  mtx_map_mtx.lock();
  auto it = log_access_mtx_map.find(logFileName);
  if (it != log_access_mtx_map.end()) {
    it->second.first ++;
  } else {
    log_access_mtx_map[logFileName] =
      std::make_pair<uint64_t, std::shared_ptr<std::mutex>>(
        1,
        std::make_shared<std::mutex>());
  }
  mtx_map_mtx.unlock();
  return;
}

// ----------------------------------------------------------------------------
// -------------- static mutex map accessors : remove reference ---------------
// ----------------------------------------------------------------------------
void Compiler::removeReferenceToLogFile(const std::string &logFileName)
{
  if (logFileName == "") {
    return;
  }
  mtx_map_mtx.lock();
  auto it = log_access_mtx_map.find(logFileName);
  if (it != log_access_mtx_map.end()) {
    it->second.first --;
    if(it->second.first <= 0) {
      log_access_mtx_map.erase(it);
    }
  }
  mtx_map_mtx.unlock();
  return;
}

// ----------------------------------------------------------------------------
// ----------------- static mutex map accessors : lock mutex ------------------
// ----------------------------------------------------------------------------
void Compiler::lockMutex(const std::string &logFileName)
{
  if (logFileName == "") {
    return;
  }
  auto it = log_access_mtx_map.find(logFileName);
  if (it != log_access_mtx_map.end()) {
    it->second.second->lock();
  }
  return;
}

// ----------------------------------------------------------------------------
// ---------------- static mutex map accessors : unlock mutex -----------------
// ----------------------------------------------------------------------------
void Compiler::unlockMutex(const std::string &logFileName)
{
  if (logFileName == "") {
    return;
  }
  auto it = log_access_mtx_map.find(logFileName);
  if (it != log_access_mtx_map.end()) {
    it->second.second->unlock();
  }
  return;
}
