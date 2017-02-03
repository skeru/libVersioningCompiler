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
#ifndef LIB_VERSIONING_COMPILER_DEBUG_UTILS_HPP
#define LIB_VERSIONING_COMPILER_DEBUG_UTILS_HPP

#include <string>

#ifdef VC_DEBUG
#include <iostream>
#endif

namespace vc {

  /** Notify message */
  inline void Notify(const std::string message) {
    #ifdef VC_DEBUG
    std::cerr << "VC Debug notification: " << message << std::endl;
    #endif
    return;
  }

} // end of namespace vc

#endif /* end of include guard: LIB_VERSIONING_COMPILER_DEBUG_UTILS_HPP */
