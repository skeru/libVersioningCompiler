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
#ifndef LIB_VERSIONING_COMPILER_COMPILER_HPP
#define LIB_VERSIONING_COMPILER_COMPILER_HPP

#include "versioningCompiler/Option.hpp"
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace vc {

/** \brief Abstract class that defines the general behaviour for a Compiler
 */
class Compiler
{
 public:
/** \brief Default detailed constructor.
 *
 * Initialize internal state.
 */
  Compiler(const std::string &compilerID,
           const std::string &compilerCallString,
           const std::string &libWorkingDir,
           const std::string &log = "",
           const std::string &installDir = "/usr/bin",
           bool supportIR = false);

/** \brief Default destructor. */
  inline virtual ~Compiler()
  {
    removeReferenceToLogFile(logFile);
  }

/** \brief Returns the compiler unique identifier. */
  std::string getId() const;

/** \brief Returns true if Compiler can work with LLVM IR files.
 * False otherwise.
 */
  bool hasIRSupport() const;

/** \brief Returns true if Compiler can run an optimization pass on the IR.
 *
 * Implementation specific.
 */
  virtual bool hasOptimizer() const = 0;

/** \brief Generate LLVM-IR starting from a given source file.
 *
 * Returns the LLVM-IR bitcode filename on success. Empty string otherwise.
 *
 * Implementation specific.
 */
  virtual std::string generateIR(const std::string &src,
                                 const std::string &func,
                                 const std::string &versionID,
                                 const std::list<Option> options) const = 0;

/** \brief Runs optimizer on the input LLVM-IR source file.
 *
 * Returns the optimized filename on success. Empty string otherwise.
 *
 * Implementation specific.
 */
  virtual std::string runOptimizer(const std::string &src_IR,
                                   const std::string &versionID,
                                   const std::list<Option> options) const = 0;

/** \brief Runs compiler on the input source file.
 *
 * Returns the binary shared object filename on success. Empty string otherwise.
 *
 * Implementation specific.
 */
  virtual std::string generateBin(const std::string &src,
                                  const std::string &func,
                                  const std::string &versionID,
                                  const std::list<Option> options) const = 0;

/** \brief Opens the binary shared object and loads the symbol relative to the
 * given function.
 *
 * Returns the loaded function pointer on success. nullptr otherwise.
 * Output of this method is supposed to reinterpreted by the caller.
 *
 * Implementation specific.
 */
  virtual void *loadSymbol(const std::string &bin,
                           const std::string &func) const = 0;

/** \brief Converts an Option object into a compiler flag.
 *
 * Implementation specific.
 */
  virtual std::string getOptionString(const Option &o) const = 0;

 protected:
  /** \brief string used to call the compiler. */
  std::string callString;

  /** \brief directory where the compiler is installed. */
  std::string installDirectory;

  /** \brief working directory where Version-related files are supposed to be
   * saved.
   */
  std::string libWorkingDirectory;

  /** \brief log of any action done by the compiler.
   * Use empty string to disable any log.
   */
  std::string logFile;

  /** \brief flag to identify compilers with LLVM-IR bitcode support. */
  bool hasSupportIR;

  /** \brief Execute a system call of `command` and log the output. */
  void log_exec(const std::string &command) const;

  /** \brief Write a string into the log file. */
  void log_string(const std::string &command) const;

  /** \brief Check if file name exists. */
  static bool exists(const std::string &name);

  /** \brief Computes default fileName for LLVM-IR bitcode file.
   */
  std::string getBitcodeFileName(const std::string &versionID) const;

  /** \brief Computes default fileName for optimized LLVM-IR bitcode file.
   */
  std::string getOptBitcodeFileName(const std::string &versionID) const;

  /** \brief Computes default fileName for binary shared object file.
   */
  std::string getSharedObjectFileName(const std::string &versionID) const;

  /** \brief Acquire the lock of the mutex related to logfileName. Blocking.
   */
  static void lockMutex(const std::string &logFileName);

  /** \brief Release the lock of the mutex related to logfileName.
   */
  static void unlockMutex(const std::string &logFileName);

  /** \brief default method to notify that the given implementation of the
   * Compiler class does not support a specific feature.
   *
   * It will log a specified message and does nothing.
   */
  void unsupported(const std::string &message) const;

 private:
  /** \brief compiler unique identifier.
   *
   * It is user-defied at instatiation time.
   */
  std::string id;

  /** Mutex to regulate exclusive access to log file.
   * It also includes a reference counter.
   */
  static
  std::map<std::string, std::pair<uint64_t, std::shared_ptr<std::mutex>>>
  log_access_mtx_map;

  /** \brief Mutex to modify reference counters on the mutex map. */
  static std::mutex mtx_map_mtx;

  /** \brief Increments the reference counter of the mutex related to
   * logFileName. If needed, it adds a new entry.
   */
  static void addReferenceToLogFile(const std::string &logFileName);

  /** \brief Decrements the reference counter of the mutex related to
   * logFileName. If needed, it removes an entry.
   */
  static void removeReferenceToLogFile(const std::string &logFileName);
};

} // end namespace vc

#endif /* end of include guard: LIB_VERSIONING_COMPILER_COMPILER_HPP */
