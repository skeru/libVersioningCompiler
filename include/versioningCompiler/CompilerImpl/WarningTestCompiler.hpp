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

#ifndef LIB_VERSIONING_COMPILER_WARNING_TEST_COMPILER_HPP
#define LIB_VERSIONING_COMPILER_WARNING_TEST_COMPILER_HPP

#include "versioningCompiler/CompilerImpl/SystemCompiler.hpp"

#include <filesystem>
#include <string>

namespace vc {

class WarningTestCompiler : public SystemCompiler {

public:
  WarningTestCompiler(const std::string &compilerID,
                 const std::filesystem::path &compilerCallString,
                 const std::filesystem::path &libWorkingDir,
                 const std::filesystem::path &log = "",
                 const std::filesystem::path &installDir =
                     std::filesystem::u8path("/usr/bin"),
                 bool supportsIR = false);

  void log_exec(const std::string &command) const override;
  void log_string(const std::string &command) const override;

  protected:
  mutable bool hasTruncatedLog;
};

} // end namespace vc

#endif