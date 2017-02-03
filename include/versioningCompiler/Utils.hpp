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
#ifndef LIB_VERSIONING_COMPILER_UTILS_HPP
#define LIB_VERSIONING_COMPILER_UTILS_HPP

/** C-like simplified interface to libVC */

#include "versioningCompiler/Version.hpp"
#include "versioningCompiler/CompilerImpl/SystemCompiler.hpp"

namespace vc {

/** Default compiler to be used in this helper. */
std::shared_ptr<Compiler> libVC_default_compiler;

/** Instantiate default Compiler */
void vc_utils_init();

/** Create a Version using default parameters */
std::shared_ptr<Version> createVersion(const std::string &src,
                                       const std::string &fn,
                                       const std::list<Option> &options);

/** Compile a version and extract function pointer (has to be casted) */
void* compileAndGetSymbol(std::shared_ptr<Version>& v);

//---------- implementation ----------

void vc_utils_init() {
  libVC_default_compiler = std::make_shared<SystemCompiler>();
  return;
}

std::shared_ptr<Version> createVersion(const std::string &src,
                                       const std::string &fn,
                                       const std::list<Option> &options) {
  Version::Builder builder;
  builder._compiler = libVC_default_compiler;
  builder._fileName_src = src;
  builder._functionName = fn;
  builder._optionList = options;
  builder.addFunctionFlag();
  return builder.build();
}

void* compileAndGetSymbol(std::shared_ptr<Version>& v) {
  v->compile();
  return v->getSymbol();
}

} // end namespace vc

#endif /* end of include guard: LIB_VERSIONING_COMPILER_UTILS_HPP */
