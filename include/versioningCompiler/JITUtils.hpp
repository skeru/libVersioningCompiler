/* Copyright 2017 Politecnico di Milano.
 * Developed by : Stefano Cherubin
 *                PhD student, Politecnico di Milano
 *                <first_name>.<family_name>@polimi.it
 *                Marco Festa
 *                Ms student, Politecnico di Milano
 *                <first_name>2.<family_name>@mail.polimi.it
 *                Nicole Gervasoni
 *                Ms student, Politecnico di Milano
 *                <first_name>annamaria.<family_name>@mail.polimi.it
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
#ifndef LIB_VERSIONING_JIT_COMPILER_UTILS_HPP
#define LIB_VERSIONING_JIT_COMPILER_UTILS_HPP

/** C-like simplified interface to libVC */

#include <iostream>
#include "versioningCompiler/Version.hpp"
#include "versioningCompiler/CompilerImpl/JITCompiler.hpp"

namespace vc {

/** Default compiler to be used in this helper. */
    compiler_ptr_t _libVC_jit_compiler;

    llvm::TargetMachine *_targetMachine;

/** Instantiate JIT Compiler */
    compiler_ptr_t vc_utils_init();



/** Create a Version using default parameters */
    version_ptr_t createVersion(const std::string &src,
                                const std::string &fn,
                                const opt_list_t &options);

/** Compile a version and extract function pointer (has to be casted) */
    void *compileAndGetSymbol(version_ptr_t &v);


//---------- implementation ----------

    compiler_ptr_t vc_utils_init() {

        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();

        std::cout << "Setting up target machine.." << std::endl;
        _targetMachine = llvm::EngineBuilder().selectTarget();

        // ---------- Compiler initialization ---------
        std::cout << "Setting up compiler.." << std::endl;
        _libVC_jit_compiler = vc::make_compiler<vc::JITCompiler>(
                "jitCompiler",
                ".",
                "./test_jit.log",
                *_targetMachine
        );
    }

    version_ptr_t createVersion(const std::string &src,
                                const std::string &fn,
                                const opt_list_t &options) {

        std::cout << "Setting up builder.." << std::endl;
        Version::Builder builder;
        builder._compiler = _libVC_jit_compiler;
        builder._fileName_src = {src};
        builder._functionName = {fn};
        builder._optionList = options;
        // builder.addFunctionFlag();
        builder._autoremoveFilesEnable = true;
        return builder.build();
    }

    void *compileAndGetSymbol(version_ptr_t &v) {

        v->compile();
        return v->getSymbol();
    }

} // end namespace vc

#endif /* end of include guard: LIB_VERSIONING_JIT_COMPILER_UTILS_HPP */
