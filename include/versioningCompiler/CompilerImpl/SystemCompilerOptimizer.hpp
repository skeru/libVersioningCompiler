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
#ifndef LIB_VERSIONING_COMPILER_SYSTEM_COMPILER_OPTIMIZER_HPP
#define LIB_VERSIONING_COMPILER_SYSTEM_COMPILER_OPTIMIZER_HPP

#include "versioningCompiler/CompilerImpl/SystemCompiler.hpp"

namespace vc {

/** Compiler implementation with optimizer support.
 * It exploits system calls.
 */
class SystemCompilerOptimizer : public SystemCompiler {

public:
  SystemCompilerOptimizer();

  SystemCompilerOptimizer(const std::string compilerID,
                          const std::filesystem::path compilerCallString,
                          const std::filesystem::path optimizerCallString,
                          const std::filesystem::path libWorkingDir,
                          const std::filesystem::path log = "",
                          const std::filesystem::path installDir =
                              std::filesystem::u8path("/usr/bin"),
                          const std::filesystem::path optimizerInstallDir =
                              std::filesystem::u8path("/usr/bin"));

  inline virtual ~SystemCompilerOptimizer() {}

  virtual bool hasOptimizer() const override;

  virtual std::filesystem::path
  runOptimizer(const std::filesystem::path &src_IR,
               const std::string &versionID,
               const opt_list_t options) const override;

protected:
  std::filesystem::path optInstallDirectory;
  std::filesystem::path optCallString;
};

} // end namespace vc

#endif /* end of include guard:                                                \
          LIB_VERSIONING_COMPILER_SYSTEM_COMPILER_OPTIMIZER_HPP */
