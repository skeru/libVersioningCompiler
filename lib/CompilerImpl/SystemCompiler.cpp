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
#include "versioningCompiler/CompilerImpl/SystemCompiler.hpp"

using namespace vc;

// ----------------------------------------------------------------------------
// ----------------------- zero-parameters constructor ------------------------
// ----------------------------------------------------------------------------
SystemCompiler::SystemCompiler() : SystemCompiler(
                                                  "cc",
                                                  "cc",
                                                  ".",
                                                  "",
                                                  "/usr/bin"
                                                ) { }

// ----------------------------------------------------------------------------
// --------------------------- detailed constructor ---------------------------
// ----------------------------------------------------------------------------
SystemCompiler::SystemCompiler(const std::string &compilerID,
                               const std::string &compilerCallString,
                               const std::string &libWorkingDir,
                               const std::string &mylogfile,
                               const std::string &installDir,
                               bool supportsIR
                             ) : Compiler(
                                          compilerID,
                                          compilerCallString,
                                          libWorkingDir,
                                          mylogfile,
                                          installDir,
                                          supportsIR
                                        ) { }

// ----------------------------------------------------------------------------
// ---------------------- optimizer support declaration -----------------------
// ----------------------------------------------------------------------------
bool SystemCompiler::hasOptimizer() const
{
  return false;
}

// ----------------------------------------------------------------------------
// ------------------------------- generate IR --------------------------------
// ----------------------------------------------------------------------------
std::string SystemCompiler::generateIR(const std::vector<std::string> &src,
                                       const std::vector<std::string> &func,
                                       const std::string &versionID,
                                       const opt_list_t options)
{
  // NO LLVM-IR support enabled by default
  if (hasIRSupport()) {
    // system call - command construction
    std::string command = installDirectory + "/" + callString;
    std::string IRFile = Compiler::getBitcodeFileName(versionID);
    command = command + " -c -emit-llvm -o " + IRFile;
    // does not work with gcc
    for (auto &o : options) {
      command = command + " " + getOptionString(o);
    }
    for (const auto & src_file : src) {
      command = command + " " + src_file;
    }
    Compiler::log_exec(command);
    if (exists(IRFile)) {
      return IRFile;
    }
    return "";
  }
  return "";
}

// ----------------------------------------------------------------------------
// ----------------------------- run IR optimizer -----------------------------
// ----------------------------------------------------------------------------
std::string SystemCompiler::runOptimizer(const std::string &src_IR,
                                         const std::string &versionID,
                                         const opt_list_t options) const
{
  std::string error = "SystemCompiler::runOptimizer: ";
  error = error + "System compiler does not support optimizer";
  Compiler::unsupported(error);
  return "";
}

// ----------------------------------------------------------------------------
// ------------------------------- generate bin -------------------------------
// ----------------------------------------------------------------------------
std::string SystemCompiler::generateBin(const std::vector<std::string> &src,
                                        const std::vector<std::string> &func,
                                        const std::string &versionID,
                                        const opt_list_t options)
{
  // system call - command construction
  std::string command = installDirectory + "/" + callString;
  std::string binaryFile = Compiler::getSharedObjectFileName(versionID);
  command = command + " -fpic -shared -o " + binaryFile;
  for (const auto &o : options) {
    command = command + " " + getOptionString(o);
  }
  for (const auto & src_file : src) {
    command = command + " " + src_file;
  }
  log_exec(command);
  if (exists(binaryFile)) {
    return binaryFile;
  }
  return "";
}

// ----------------------------------------------------------------------------
// -------------- convert the Option into a command line string ---------------
// ----------------------------------------------------------------------------
inline std::string SystemCompiler::getOptionString(const Option &o) const
{
  std::string tmp_val = o.getValue();
  if (tmp_val.length() > 2 &&
      // escape whitespaces with double quotes
      (tmp_val.find(" ") != std::string::npos ||
       tmp_val.find("\t") != std::string::npos) &&
      // ...but not if already within quotes
      !(tmp_val[0] == tmp_val[tmp_val.length() - 1] && tmp_val[0] == '\"'))
  {
    tmp_val = "\"" + tmp_val + "\"";
  }
  return o.getPrefix() + tmp_val;
}
