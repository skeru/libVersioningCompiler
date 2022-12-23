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
#include "versioningCompiler/CompilerImpl/SystemCompilerOptimizer.hpp"

using namespace vc;

// ----------------------------------------------------------------------------
// ----------------------- zero-parameters constructor ------------------------
// ----------------------------------------------------------------------------
SystemCompilerOptimizer::SystemCompilerOptimizer() :
  SystemCompilerOptimizer(
                          "llvm/clang",
                          "clang",
                          "opt",
                          ".",
                          "",
                          "/usr/bin",
                          "/usr/bin"
                        ) { }

// ----------------------------------------------------------------------------
// --------------------------- detailed constructor ---------------------------
// ----------------------------------------------------------------------------
SystemCompilerOptimizer::SystemCompilerOptimizer(
                         const std::string compilerID,
                         const std::filesystem::path compilerCallString,
                         const std::filesystem::path optimizerCallString,
                         const std::filesystem::path libWorkingDir,
                         const std::filesystem::path log,
                         const std::filesystem::path installDir,
                         const std::filesystem::path optimizerInstallDir
                       ) : SystemCompiler(
                                          compilerID,
                                          compilerCallString,
                                          libWorkingDir,
                                          log,
                                          installDir,
                                          true
                                         ),
                          optCallString(optimizerCallString),
                          optInstallDirectory(optimizerInstallDir) { }

// ----------------------------------------------------------------------------
// ---------------------- optimizer support declaration -----------------------
// ----------------------------------------------------------------------------
bool SystemCompilerOptimizer::hasOptimizer() const
{
  return true;
}

// ----------------------------------------------------------------------------
// ----------------------------- run IR optimizer -----------------------------
// ----------------------------------------------------------------------------
std::filesystem::path SystemCompilerOptimizer::runOptimizer(const std::filesystem::path &src_IR,
                                                  const std::string &versionID,
                                                  const opt_list_t options) const
{
  std::string commandString;
  commandString = ( optInstallDirectory/ optCallString).string();
  std::filesystem::path optimizedFileName = Compiler::getOptBitcodeFileName(versionID);
  for (auto &o : options) {
    commandString  = commandString + " " + getOptionString(o);
  }
  commandString = commandString + " -o " + optimizedFileName.string() + " " + src_IR.string();
  Compiler::log_exec(commandString);
  if (exists(optimizedFileName)) {
    return optimizedFileName;
  }
  return "";
}
