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
#ifndef LIB_VERSIONING_COMPILER_UTILS_HPP
#define LIB_VERSIONING_COMPILER_UTILS_HPP

/** C-like simplified interface to libVC */

#include "versioningCompiler/CompilerImpl/SystemCompiler.hpp"
#include "versioningCompiler/Version.hpp"

namespace vc {

/** Default compiler to be used in this helper. */
compiler_ptr_t libVC_default_compiler;

/** Instantiate default Compiler */
void vc_utils_init();

/** Create a Version using default parameters */
version_ptr_t createVersion(const std::vector<std::filesystem::path> &src,
                            const std::vector<std::string> &fn,
                            const opt_list_t &options);

/** Create a Version using default parameters */
version_ptr_t createVersion(const std::filesystem::path &src,
                            const std::string &fn,
                            const opt_list_t &options);

/** Compile a version and extract function pointer (has to be casted) */
void* compileAndGetSymbol(version_ptr_t& v);

//---------- implementation ----------

void vc_utils_init() {
  libVC_default_compiler = make_compiler<SystemCompiler>();
  return;
}

version_ptr_t createVersion(const std::filesystem::path &src,
                            const std::string &fn,
                            const opt_list_t &options) {
  Version::Builder builder;
  builder._compiler = libVC_default_compiler;
  builder._fileName_src.push_back(src);
  builder._functionName.push_back(fn);
  builder._optionList = options;
  return builder.build();
}

version_ptr_t createVersion(const std::vector<std::filesystem::path> &src,
                            const std::vector<std::string> &fn,
                            const opt_list_t &options) {
  Version::Builder builder;
  builder._compiler = libVC_default_compiler;
  builder._fileName_src = src;
  builder._functionName = fn;
  builder._optionList = options;
  return builder.build();
}

void* compileAndGetSymbol(version_ptr_t& v) {
  v->compile();
  return v->getSymbol();
}

} // end namespace vc

#endif /* end of include guard: LIB_VERSIONING_COMPILER_UTILS_HPP */
