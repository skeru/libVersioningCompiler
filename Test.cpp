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
#include "versioningCompiler/CompilerImpl/SystemCompilerOptimizer.hpp"
#include "versioningCompiler/Version.hpp"
#if HAVE_CLANG_AS_LIB
#include "versioningCompiler/CompilerImpl/ClangLibCompiler.hpp"
#endif

#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

#ifndef FORCED_PATH_TO_TEST
#define FORCED_PATH_TO_TEST "../libVersioningCompiler/test_code"
#endif
#define PATH_TO_C_TEST_CODE FORCED_PATH_TO_TEST "/test_code.c"

#ifndef TEST_FUNCTION
#define TEST_FUNCTION "test_function"
#endif

#ifndef SECOND_FUNCTION
#define SECOND_FUNCTION "test_function2"
#endif

#ifndef TEST_FUNCTION_LBL
#define TEST_FUNCTION_LBL "TEST_FUNCTION"
#endif

#ifndef LLVM_TOOLS_BINARY_DIR
#define LLVM_TOOLS_BINARY_DIR "/usr/bin"
#endif

#ifndef CLANG_EXE_NAME
#define CLANG_EXE_NAME "clang"
#endif

#ifndef OPT_EXE_FULLPATH
#define OPT_EXE_FULLPATH "/usr/bin/opt"
#endif

#ifndef LLVM_VERSION_MAJOR
#define LLVM_VERSION_MAJOR 0
#endif

// someone should provide the signature of the function now versioning
// in the form of function pointer type.
typedef int (*signature_t)(int);

int main(int argc, char const *argv[]) {
  // At least one builder is needed. A builder will provide the immutable
  // object Verison, which identifies a function version configuration.
  // There are more that one builder just to show different constructors.
  // It is possible to reuse the same when the difference between a version
  // and another one is just a minor change. Otherwise it is possible to reuse
  // the same builder by calling reset() on it, or just using another builder.
  // WARNING: builder does not call any compiler
  vc::Version::Builder builder, another_builder;

  auto t_fun_index = builder.addFunctionName(TEST_FUNCTION); // This returns 0
  if (t_fun_index == -1)
    std::cerr << "Error: TEST_FUNCTION name not added correctly" << std::endl;
  auto second_fun_index =
      builder.addFunctionName(SECOND_FUNCTION); // This returns 1
  if (second_fun_index == -1)
    std::cerr << "Error: SECOND_FUNCTION name not added correctly" << std::endl;
  builder.addSourceFile(PATH_TO_C_TEST_CODE);
  builder.addFunctionFlag(TEST_FUNCTION_LBL);
  // ---------- Initialize the compiler to be used. ----------
  // Should be done just once.
  // Compiler is stateless from all points of view but logging:
  // if required (i.e. !log_filename.empty()) it keeps a continous log of
  // commands and errors. Right now only Compilers called via a system call are
  // available. That's why they are called SystemCompilers. Support for
  // Compiler-as-a-library will be added soon.
  vc::compiler_ptr_t cc = vc::make_compiler<vc::SystemCompiler>();

  // WARNING: on linux, paths starting with / are assumed to be absolute paths!
  // Setting both callStrings and install path as absolute path would cause to
  // ignore the install path! See
  // https://en.cppreference.com/w/cpp/filesystem/path/append
  vc::compiler_ptr_t gcc = vc::make_compiler<vc::SystemCompiler>(
      "gcc", std::filesystem::u8path("gcc"), std::filesystem::u8path("."),
      std::filesystem::u8path("./test.log"),
      std::filesystem::u8path("/usr/bin"), false);
  // FAQ: I have a separate install folder for LLVM/clang.
  // ANS: Here it is an example of how to handle that case.
  vc::compiler_ptr_t clang = vc::make_compiler<vc::SystemCompilerOptimizer>(
      "llvm/clang", std::filesystem::u8path(CLANG_EXE_NAME),
      std::filesystem::u8path(OPT_EXE_FULLPATH), std::filesystem::u8path("."),
      std::filesystem::u8path("./test.log"),
      std::filesystem::u8path(LLVM_TOOLS_BINARY_DIR),
      std::filesystem::u8path("/"));
#if HAVE_CLANG_AS_LIB
  vc::compiler_ptr_t clangAsLib = vc::make_compiler<vc::ClangLibCompiler>(
      "clangAsALibrary", std::filesystem::u8path("."),
      std::filesystem::u8path("test_clang.log"));
#endif
  // ---------- End compilers initialization ----------
  // start configuring version v
  builder.setCompiler(cc);
  // avoid relocation problems with global variables
  builder.genIRoptions({vc::Option("fpic", "-fPIC")});
  // finalization of a version. no compilation has been called yet.
  vc::version_ptr_t v = builder.build();
  // end configuring version v

  // start configuring version v2
  // want to reuse the same parameters as Version v. Use the same builder.
  // just modify the compiler...
  builder._compiler = gcc;
  // builder._autoremoveFilesEnable = false; // uncomment this to keep the
  // intermediate files
  // ...and the option list
  builder.options({vc::Option("o", "-O", "2")});
  // Version v2 is finalized as v, with the above mentioned changes.
  vc::version_ptr_t v2 = builder.build();
  // end configuring version v2

  // start configuring version v3
  // another way to clone a version: construct a builder by cloning v2
  another_builder = vc::Version::Builder(v2);
  another_builder.setCompiler(clang);
#if LLVM_VERSION_MAJOR < 16
  another_builder.setOptOptions(
      {vc::Option("fp-contract", "-fp-contract=", "fast"),
       vc::Option("inline", "-inline"), vc::Option("unroll", "-loop-unroll"),
       vc::Option("mem2reg", "-mem2reg")});
#else
  another_builder.setOptOptions(
      {vc::Option("Optimization Passes",
                  "-passes=", "'inline,loop-unroll,mem2reg'"),
       vc::Option("fp-contract", "--fp-contract=", "fast")});
#endif
  vc::version_ptr_t v3 = another_builder.build();
  // end configuring version v3

  // start configuring version v4
#if HAVE_CLANG_AS_LIB
  builder.setCompiler(clangAsLib);
#endif
  builder._autoremoveFilesEnable = true;
#if LLVM_VERSION_MAJOR < 16
  builder.setOptOptions({
      vc::Option("mem2reg", "-mem2reg"),
      vc::Option("o", "-O", "3"),
  });
#else
  builder.setOptOptions({
      vc::Option("mem2reg", "-passes='defaultO3,mem2reg'"),
  });
#endif
  vc::version_ptr_t v4 = builder.build();
  // end configuring version v4

  // actually compile v.
  bool ok = v->compile();
  if (!ok) { // something during the compilation went wrong
    if (!v->hasGeneratedBin()) {
      std::cerr << "Error: compilation failed." << std::endl;
    } else {
      std::cerr << "Error: symbol not loaded" << std::endl;
    }
    return -1;
  }
  // same for v2
  std::cerr << "Notify: v compiled. Going for v2" << std::endl;
  ok = v2->compile();
  if (!ok) {
    if (!v2->hasGeneratedBin()) {
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
  if (!ok) {
    if (!clang->hasIRSupport()) {
      std::cerr << "Error: something went wrong with the compiler."
                << std::endl;
    } else if (!v3->hasGeneratedIR()) {
      std::cerr << "Error: generation of IR v3 failed." << std::endl;
    } else {
      std::cerr << "Error: optimization of IR v3 failed." << std::endl;
    }
    std::cerr << "\tPlease check the compiler/optimizer install path in "
                 "Test.cpp source file"
              << std::endl;
  }
  std::cerr << "Notify: v3 IR prepared. Going for v3 compilation" << std::endl;
  // compiles from optimized version, if available
  ok = v3->compile();
  if (!ok) {
    if (!v3->hasGeneratedBin()) {
      std::cerr << "Error: compilation 3 failed." << std::endl;
    } else {
      std::cerr << "Error: symbol 3 not loaded" << std::endl;
    }
    return -1;
  }
  std::cerr << "Notify: v3 compiled. Going for v4" << std::endl;
  ok = v4->prepareIR();
  if (!ok) {
    if (!clang->hasIRSupport()) {
      std::cerr << "Error: something went wrong with the compiler."
                << std::endl;
    } else if (!v3->hasGeneratedIR()) {
      std::cerr << "Error: generation of IR v4 failed." << std::endl;
    } else {
      std::cerr << "Error: optimization of IR v4 failed." << std::endl;
    }
    std::cerr << "\tPlease check the compiler/optimizer install path"
              << std::endl;
  }
  std::cout << "Notify: v4 IR prepared. Going for v4 compilation" << std::endl;
  ok = v4->compile();
  if (!ok) {
    if (v4->hasGeneratedBin()) {
      std::cerr << "v4 has generated binary " << v4->getFileName_bin()
                << std::endl;
    }
    if (v4->hasLoadedSymbol()) {
      std::cerr << "v4 has loaded symbol" << std::endl;
    }
    std::cerr << "Error: compile v4 has failed" << std::endl;
    return -1;
  }
  std::cout << "Notify: v4 compiled." << std::endl;

  vc::version_ptr_t v5 = vc::Version::Builder::createFromSO(
      v4->getFileName_bin(), TEST_FUNCTION, gcc, false,
      {"version created from shared object"});
  if (!v5->compile()) {
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
  f.push_back((signature_t)v->getSymbol(t_fun_index));
  f.push_back((signature_t)v->getSymbol(second_fun_index));
  f.push_back((signature_t)v2->getSymbol(t_fun_index));
  f.push_back((signature_t)v3->getSymbol(t_fun_index));
  f.push_back((signature_t)v4->getSymbol()); // equivalent to v4->getSymbol(0)
                                             // or v4->getSymbol(t_fun_index)
  f.push_back((signature_t)v2->getSymbol(
      second_fun_index)); // equivalent to v5->getSymbol(1).
  f.push_back((signature_t)v5->getSymbol());
  if (f[0]) {
    std::cout << "Expected: 42**2 = 1764" << std::endl;
    f[0](42); // prints "42**2 = 1764" and sets v's global variable to 1764
    std::cout << "Expected: 1764" << std::endl;
    f[1](0); // prints v's global variable value, 1764
    std::cout << "Expected: 24**2 = 576" << std::endl;
    f[2](24); // prints "24**2 = 576" and sets v2's global variable to 576
    std::cout << "Expected: 3**2 = 9" << std::endl;
    f[3](3); // prints "3**2 = 9" and sets v3's global variable to 9.
    std::cout << "Expected: 0**2 = 0" << std::endl;
    f[4](0); // prints "0**2 = 0" and sets v4's global variable to 0
    std::cout << "Expected: 576" << std::endl;
    f[5](0); // prints v2's global variable value, which is 576.
    std::cout << "Expected: 22**2 = 484" << std::endl;
    f[2](22); // prints "22**2 = 484" and sets v2's global variable to 484
    std::cout << "Expected: 484" << std::endl;
    f[5](0); // prints v2's global variable (484)
    std::cout << "Expected: 6**3 = 216" << std::endl;
    f[5](6); // prints "6**3 = 216"
    std::cout << "Expected: 484" << std::endl;
    f[5](0); // prints v2's global variable (484)
    std::cout << "Expected: 3**2 = 9" << std::endl;
    f[6](3); // prints "3**2 = 9"
  } else {
    std::cerr << "Error function pointers unavailable" << '\n';
  }
  v->fold();
  v2->fold();
  v3->fold();
  v4->fold();
  v5->fold();
  std::cout << "Version folded, reloading it." << std::endl;
  v3->reload();
  signature_t reloaded =
      (signature_t)v3->getSymbol(); // equivalent to v3->getSymbol(t_fun_index)
  signature_t reloaded2 = (signature_t)v3->getSymbol(
      1); // equivalent to v3->getSymbol(second_fun_index)
  if (reloaded) {
    std::cout << "Expected: -1" << std::endl;
    reloaded2(
        0); // v3's global variable was set to 9. After folding, the version is
            // reset to its initial state so v3's global value is -1.
    std::cout << "Expected: 15**2 = 225" << std::endl;
    reloaded(15);
    std::cout << "Expected: 225" << std::endl;
    reloaded2(0);
  } else {
    std::cerr << "Error in folding and reloading v3" << std::endl;
  }
  return 0;
}
