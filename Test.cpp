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
#include "versioningCompiler/Version.hpp"
#include "versioningCompiler/CompilerImpl/SystemCompiler.hpp"
#include "versioningCompiler/CompilerImpl/SystemCompilerOptimizer.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h>

#if HAVE_CLANG_AS_LIB
#include "versioningCompiler/CompilerImpl/ClangLibCompiler.hpp"
#endif

#ifndef FORCED_PATH_TO_TEST
#define FORCED_PATH_TO_TEST "../libVersioningCompiler/test_code"
#endif
#define PATH_TO_C_TEST_CODE FORCED_PATH_TO_TEST"/test_code.c"

#ifndef TEST_FUNCTION
#define TEST_FUNCTION "test_function"
#endif

// someone should provide the signature of the function now versioning
// in the form of function pointer type.
typedef int (*signature_t)(int);

int main(int argc, char const *argv[])
{
  // At least one builder is needed. A builder will provide the immutable
  // object Verison, which identifies a function version configuration.
  // There are more that one builder just to show different constructors.
  // It is possible to reuse the same when the difference between a version
  // and another one is just a minor change. Otherwise it is possible to reuse
  // the same builder by calling reset() on it, or just using another builder.
  // WARNING: builder does not call any compiler
  vc::Version::Builder builder, another_builder;
  builder._functionName = TEST_FUNCTION;
  builder._fileName_src = PATH_TO_C_TEST_CODE;
  builder.addFunctionFlag();
  // ---------- Initialize the compiler to be used. ----------
  // Should be done just once.
  // Compiler is stateless from all points of view but logging:
  // if required (i.e. log_filename != "") it keeps a continous log of commands
  // and errors.
  // Right now only Compilers called via a system call are available. That's
  // why they are called SystemCompilers.
  // Support for Compiler-as-a-library will be added soon.
  std::shared_ptr<vc::Compiler> cc = std::make_shared<vc::SystemCompiler>();
  std::shared_ptr<vc::Compiler> gcc = std::make_shared<vc::SystemCompiler>(
                                          "gcc",
                                          "gcc",
                                          ".",
                                          "./test.log",
                                          "/usr/bin",
                                          false
                                        );
  // FAQ: I have a separate install folder for LLVM/clang.
  // ANS: Here it is an example of how to handle that case.
  std::shared_ptr<vc::Compiler> clang = std::make_shared<vc::SystemCompilerOptimizer>(
                                          "llvm/clang",
                                          "clang",
                                          "opt",
                                          ".",
                                          "./test.log",
                                          "/usr/bin",
                                          "/usr/lib/llvm/bin"
                                        );
#if HAVE_CLANG_AS_LIB
  std::shared_ptr<vc::Compiler> clangAsLib = std::make_shared<vc::ClangLibCompiler>(
                                          "clangAsALibrary",
                                          ".",
                                          "test_clang.log"
                                        );
#endif
  // ---------- End compilers initialization ----------
  // start configuring version v
  builder._compiler = cc;
  // finalization of a version. no compilation has been called yet.
  std::shared_ptr<vc::Version> v = builder.build();
  // end configuring version v

  // start configuring version v2
  // want to reuse the same parameters as Version v. Use the same builder.
  // just modify the compiler...
  builder._compiler = gcc;
  builder._autoremoveFilesEnable = true;
  // ...and the option list
  builder.options({vc::Option("o", "-O", "2")});
  // Version v2 is finalized as v, with the above mentioned changes.
  std::shared_ptr<vc::Version> v2 = builder.build();
  // end configuring version v2

  // start configuring version v3
  // another way to clone a version: construct a builder by cloning v2
  another_builder = vc::Version::Builder(v2);
  another_builder._compiler = clang;
  another_builder._optOptionList = {
                                    vc::Option("fp-contract", "-fp-contract=", "fast"),
                                    vc::Option("inline", "-inline"),
                                    vc::Option("unroll", "-loop-unroll"),
                                    vc::Option("mem2reg", "-mem2reg")
                                   };
  std::shared_ptr<vc::Version> v3 = another_builder.build();
  // end configuring version v3

  // start configuring version v4
#if HAVE_CLANG_AS_LIB
  builder._compiler = clangAsLib;
#endif
  builder._autoremoveFilesEnable = true;
  builder._optOptionList = {
                            vc::Option("mem2reg", "-mem2reg"),
                            vc::Option("o", "-O", "3"),
                           };
  std::shared_ptr<vc::Version> v4 = builder.build();
  // end configuring version v4

  // actually compile v.
  bool ok = v->compile();
  if (! ok) { // something during the compilation went wrong
    if (! v->hasGeneratedBin()) {
      std::cerr << "Error: compilation failed." << std::endl;
    } else {
      std::cerr << "Error: symbol not loaded" << std::endl;
    }
    return -1;
  }
  // same for v2
  std::cerr << "Notify: v compiled. Going for v2" << std::endl;
  ok = v2->compile();
  if (! ok) {
    if (! v2->hasGeneratedBin()) {
      std::cerr << "Error: compilation 2 failed." << std::endl;
    } else {
      std::cerr << "Error: symbol 2 not loaded" << std::endl;
    }
    return -1;
  }
  // want to do fancy things using v3.
  // prepare LLVM-IR file (generate IR and run optimizer, if available)
  std::cerr << "Notify: v2 compiled. Going for v3 prepareIR" << std::endl;
  ok = v3->prepareIR();
  if (! ok) {
    if (! clang->hasIRSupport()) {
      std::cerr << "Error: something went wrong with the compiler." << std::endl;
    } else if (! v3->hasGeneratedIR()) {
      std::cerr << "Error: generation of IR v3 failed." << std::endl;
    } else {
      std::cerr << "Error: optimization of IR v3 failed." << std::endl;
    }
    std::cerr << "\tPlease check the compiler/optimizer install path" << std::endl;
  }
  std::cerr << "Notify: v3 IR prepared. Going for v3 compilation" << std::endl;
  // compiles from optimized version, if available
  ok = v3->compile();
  if (! ok) {
    if (! v3->hasGeneratedBin()) {
      std::cerr << "Error: compilation 3 failed." << std::endl;
    } else {
      std::cerr << "Error: symbol 3 not loaded" << std::endl;
    }
    return -1;
  }
  std::cerr << "Notify: v3 compiled. Going for v4" << std::endl;
  ok = v4->prepareIR();
  if (! ok) {
    if (! clang->hasIRSupport()) {
      std::cerr << "Error: something went wrong with the compiler." << std::endl;
    } else if (! v3->hasGeneratedIR()) {
      std::cerr << "Error: generation of IR v4 failed." << std::endl;
    } else {
      std::cerr << "Error: optimization of IR v4 failed." << std::endl;
    }
    std::cerr << "\tPlease check the compiler/optimizer install path" << std::endl;
  }
  std::cout << "Notify: v4 IR prepared. Going for v4 compilation" << std::endl;
  ok = v4->compile();
  if (!ok) {
    if (v4->hasGeneratedBin()) {
      std::cerr << "v4 has generated binary "<< v4->getFileName_bin() << std::endl;
    }
    if (v4->hasLoadedSymbol()) {
      std::cerr << "v4 has loaded symbol" << std::endl;
    }
    std::cerr << "Error: compile v4 has failed" << std::endl;
    return -1;
  }
  std::cout << "Notify: v4 compiled." << std::endl;

  std::shared_ptr<vc::Version> v5 =
    vc::Version::Builder::createFromSO(v4->getFileName_bin(),
                                       TEST_FUNCTION,
                                       gcc,
                                       false,
                                       "version created from shared object");
  if (! v5->compile()) {
    if (v5->hasGeneratedBin()) {
      std::cerr << "v5 has binary " << v5->getFileName_bin() << std::endl;
    }
    if (v5->hasLoadedSymbol()) {
      std::cerr << "v5 has loaded symbol" << std::endl;
    }
    return -1;
  }
  std::cout << "Notify: v5 loaded." << std::endl;

  std::vector<signature_t> f;
  f.push_back((signature_t)v->getSymbol());
  f.push_back((signature_t)v2->getSymbol());
  f.push_back((signature_t)v3->getSymbol());
  f.push_back((signature_t)v4->getSymbol());
  f.push_back((signature_t)v5->getSymbol());
  if (f[0]) {
    f[0](42);
    f[1](24);
    f[2](5);
    f[3](6);
    f[4](7);
  }
  v->fold();
  v2->fold();
  v3->fold();
  v4->fold();
  v5->fold();
  v3->reload();
  signature_t reloaded = (signature_t)v3->getSymbol();
  if (reloaded) {
    reloaded(15);
  } else {
    std::cerr << "Error in folding and reloading v3" << std::endl;
  }
  return 0;
}
