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
#include <cmath>
#include <type_traits>
#include <fstream>
#include <limits>

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

#ifndef THIRD_FUNCTION
#define THIRD_FUNCTION "test_function3"
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

#ifndef DEFAULT_COMPILER_DIR
#define DEFAULT_COMPILER_DIR "/usr/bin"
#endif

#ifndef DEFAULT_COMPILER_NAME
#define DEFAULT_COMPILER_NAME "gcc"
#endif

#ifndef LLVM_FOUND
#define LLVM_FOUND 0
#endif

// someone should provide the signature of the function now versioning
// in the form of function pointer type.
typedef float (*compute_func_t)(int);   // For test_function and test_function2
typedef int (*validate_func_t)(float);  // For test_function3
int ret_value = 0;

void checkResult(float result, float expected){
  if (std::fabs(result - expected) < 10*std::numeric_limits<float>::epsilon()) {
    std::cout << "PASSED" << std::endl;
  }else{
    std::cout << "FAILED: expected = " << expected << ", got = " << result << std::endl;
    if(!ret_value)
      ret_value=1;
  }
}

int check_log_for_content(const std::string &log_file, const std::string &expected) {
    std::ifstream log(log_file);
    if (!log.is_open()) {
        std::cerr << "FAILED: unable to open log file: " << log_file << std::endl;
        ret_value=1;
        return 2;
    }

    std::string line;
    while (std::getline(log, line)) {
        if (line.find(expected) != std::string::npos) {
            return 0;
        }
    }
    return 1;
}

void delete_warning_log() {
const std::filesystem::path log_path = std::filesystem::u8path("./test_warning.log");
  if (std::filesystem::exists(log_path)) {
    std::filesystem::remove(log_path);
  }
}

int main(int argc, char const *argv[]) {
  std::cout << std::endl;
  std::cout << "=== libVC_test ===" << std::endl;
  std::cout << std::endl;
  // At least one builder is needed. A builder will provide the immutable
  // object Verison, which identifies a function version configuration.
  // There are more that one builder just to show different constructors.
  // It is possible to reuse the same when the difference between a version
  // and another one is just a minor change. Otherwise it is possible to reuse
  // the same builder by calling reset() on it, or just using another builder.
  // WARNING: builder does not call any compiler
  vc::Version::Builder builder, another_builder;
  delete_warning_log();
  auto t_fun_index = builder.addFunctionName(TEST_FUNCTION); // This returns 0
  if (t_fun_index == -1)
    std::cerr << "Error: TEST_FUNCTION name not added correctly" << std::endl;
  auto second_fun_index =
      builder.addFunctionName(SECOND_FUNCTION); // This returns 1
  if (second_fun_index == -1)
    std::cerr << "Error: SECOND_FUNCTION name not added correctly" << std::endl;
  auto third_fun_index =
      builder.addFunctionName(THIRD_FUNCTION); // This returns 2
  if (third_fun_index == -1)
    std::cerr << "Error: THIRD_FUNCTION name not added correctly" << std::endl;
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
  vc::compiler_ptr_t default_comp = vc::make_compiler<vc::SystemCompiler>(
      "default_comp", std::filesystem::u8path(DEFAULT_COMPILER_NAME), std::filesystem::u8path("."),
      std::filesystem::u8path("./test.log"),
      std::filesystem::u8path(DEFAULT_COMPILER_DIR), false);
  vc::compiler_ptr_t warning_comp = vc::make_compiler<vc::SystemCompiler>(
      "warning_comp", std::filesystem::u8path(DEFAULT_COMPILER_NAME), std::filesystem::u8path("."),
      std::filesystem::u8path("./test_warning.log"),
      std::filesystem::u8path(DEFAULT_COMPILER_DIR), false);
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
  builder._compiler = default_comp;
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
#if LLVM_FOUND  
  another_builder.setCompiler(clang);
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
  builder._autoremoveFilesEnable = true;
  builder.setOptOptions({
      vc::Option("mem2reg", "-passes='defaultO3,mem2reg'"),
  });
#endif
  vc::version_ptr_t v4 = builder.build();
  // end configuring version v4

  std::cout << ">>> Compilation and IR Generation Log" << std::endl;;

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
      v4->getFileName_bin(), TEST_FUNCTION, default_comp, false,
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
  std::cout << "Notify: v5 compiled. Going for v6" << std::endl;

  // start configuring version v6
  builder._compiler = warning_comp;
  builder.options({vc::Option("o", "-O", "2"),
                        vc::Option("werror", "-Werror"),
                        vc::Option("wall", "-Wall"),
                        vc::Option("wall", "-Wconversion")});
  vc::version_ptr_t v6 = builder.build();
  // end configuring version v6

  ok = v6->compile();
  if (!ok) {
    if (!v6->hasGeneratedBin()) {
      std::cout << "Notify: v6 compilation failed as expected." << std::endl;
    } else {
      std::cerr << "Error: symbol 6 not loaded" << std::endl;
    }
  }

  std::cout << std::endl;
  std::cout << ">>> Test Configuration" << std::endl;
            
  std::cout << "- v:  Default system compiler with IR option -fPIC." << std::endl
            << "- v2: Based on v; changed compiler and added -O2 optimization." << std::endl
            << "- v3: Cloned from v2; switched to Clang (if LLVM available) with" << std::endl
            << "       -passes='inline,loop-unroll,mem2reg' and --fp-contract=fast." << std::endl
            << "- v4: Based on v2; uses clangAsLib if available, with "
            << "-passes='defaultO3,mem2reg'." << std::endl
            << "- v5: Built from shared object using v4 binary; falls back to default system compiler." << std::endl
            << "- test_function(x):  Computes x*x and stores the result in a global variable." << std::endl
            << "- test_function2(x): Returns global value if x == 0; otherwise computes x*x*x." << std::endl
            << "- test_function3(x): Verifies the global variable matches expected value x." << std::endl;         

  std::cout << "\n>>> Test Cases" << std::endl;

  std::vector<compute_func_t> f;
  f.push_back((compute_func_t)v->getSymbol(t_fun_index));
  f.push_back((compute_func_t)v->getSymbol(second_fun_index));
  f.push_back((compute_func_t)v2->getSymbol(t_fun_index));
  f.push_back((compute_func_t)v3->getSymbol(t_fun_index));
  f.push_back((compute_func_t)v4->getSymbol()); // equivalent to v4->getSymbol(0)
                                             // or v4->getSymbol(t_fun_index)
  f.push_back((compute_func_t)v2->getSymbol(
      second_fun_index)); // equivalent to v5->getSymbol(1).
  f.push_back((compute_func_t)v5->getSymbol());
  std::vector<validate_func_t> f2;
  f2.push_back((validate_func_t)v->getSymbol(third_fun_index));
  f2.push_back((validate_func_t)v2->getSymbol(third_fun_index));
  f2.push_back((validate_func_t)v3->getSymbol(third_fun_index));
  f2.push_back((validate_func_t)v4->getSymbol(third_fun_index));
  if (f[0]) {
    std::cout << "Test 01: Version v  --> test_function(42)\t";
    checkResult(f[0](42),1764.f); // sets v's global variable to 1764
    std::cout << "Test 02: Version v  --> test_function2(0)\t";
    checkResult(f[1](0),1764.f); // checks v's global variable value, 1764
    std::cout << "Test 03: Version v2 --> test_function(24)\t";
    checkResult(f[2](24),576.f); // sets v2's global variable to 576
    std::cout << "Test 04: Version v3 --> test_function(3)\t";
    checkResult(f[3](3),9.f); // sets v3's global variable to 9.
    std::cout << "Test 05: Version v4 --> test_function(0)\t";
    checkResult(f[4](0),0.f); // sets v4's global variable to 0
    std::cout << "Test 06: Version v2 --> test_function2(0)\t";
    checkResult(f[5](0),576.f); // checks v2's global variable value, which is 576.
    std::cout << "Test 07: Version v2 --> test_function(22)\t";
    checkResult(f[2](22),484.f); // sets v2's global variable to 484
    std::cout << "Test 08: Version v2 --> test_function2(0)\t";
    checkResult(f[5](0),484.f); // checks v2's global variable (484)
    std::cout << "Test 09: Version v2 --> test_function2(6)\t";
    checkResult(f[5](6),216.f); // computes "6**3 = 216"
    std::cout << "Test 10: Version v2 --> test_function2(0)\t";
    checkResult(f[5](0),484.f); // check v2's global variable (484)
    std::cout << "Test 11: Version v5 --> test_function(3)\t";
    checkResult(f[6](3),9.f); // sets v5's global variable to 9"
  } else {
    std::cerr << "Error: function pointers unavailable" << std::endl;
  }
  if (f2[0]){
    // Now check the status of the global variable for each version
    // exept for v5 which only has test_function
    std::cout << "Test 12: Version v  --> test_function3(1764)\t";
    if(f2[0](1764.f) && !ret_value)
      ret_value=1;
    std::cout << "Test 13: Version v2 --> test_function3(484)\t";
    if(f2[1](484.f) && !ret_value)
      ret_value=1;
    std::cout << "Test 14: Version v3 --> test_function3(9)\t";
    if(f2[2](9.f) && !ret_value)
      ret_value=1;
    std::cout << "Test 15: Version v4 --> test_function3(9)\t";
    if(f2[3](9.f) && !ret_value)
      ret_value=1;
  } else {
    std::cerr << "Error: function pointers unavailable" << std::endl;
  }
  v->fold();
  v2->fold();
  v3->fold();
  v4->fold();
  v5->fold();
  std::cout << "All Versions folded, reloading v3..." << std::endl;
  v3->reload();
  compute_func_t reloaded =
      (compute_func_t)v3->getSymbol(); // equivalent to v3->getSymbol(t_fun_index)
  compute_func_t reloaded2 = 
      (compute_func_t)v3->getSymbol(1); // equivalent to v3->getSymbol(second_fun_index)
  if (reloaded) {
    std::cout << "Test 16: Version v3 --> test_function2(0)\t";
    checkResult(reloaded2(0),-1.f); // v3's global variable was set to 9. After folding, the version is
                                  // reset to its initial state so v3's global value is -1.
    std::cout << "Test 17: Version v3 --> test_function(15)\t";
    checkResult(reloaded(15),225.f); 
    std::cout << "Test 18: Version v4 --> test_function2(3)\t";
    checkResult(reloaded2(0),225.f);
  } else {
    std::cerr << "Error in folding and reloading v3" << std::endl;
  }

  //check warning log
  std::cout << "Test 19: Check correct detection of warnings\t";
  int ret_warning = check_log_for_content("test_warning.log", "-Werror=conversion");
  if(ret_warning==0){
    std::cout << "PASSED" << std::endl;
  }else if(ret_warning==1){
    std::cout << "FAILED: error not detected in the log file" << std::endl;
    if(!ret_value)
      ret_value=1;
  }
  return ret_value;
}
