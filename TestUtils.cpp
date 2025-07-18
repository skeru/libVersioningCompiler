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
#include "versioningCompiler/Utils.hpp"

#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <type_traits>

#ifndef FORCED_PATH_TO_TEST
#define FORCED_PATH_TO_TEST "../libVersioningCompiler/test_code"
#endif
#define PATH_TO_C_TEST_CODE FORCED_PATH_TO_TEST "/test_code.c"

#ifndef TEST_FUNCTION
#define TEST_FUNCTION "test_function"
#endif

#ifndef THIRD_FUNCTION
#define THIRD_FUNCTION "test_function3"
#endif

// someone should provide the signature of the function now versioning
// in the form of function pointer type.
typedef float (*signature_t)(int);
typedef int (*signature_t_new)(float);
int ret_value = 0;

using namespace vc; // libVersioningCompiler namespace

template <typename T>
void checkResult(T result, T expected){
  static_assert(std::is_arithmetic<T>::value, "checkResult only supports numeric types");
  if (std::fabs(result - expected) < 1e-5) {
    std::cout << "PASSED" << std::endl;
  }else{
    std::cout << "FAILED: expected = " << expected << ", got = " << result << std::endl;
    if(!ret_value)
      ret_value=1;
  }
}

int main(int argc, char const *argv[]) {
  std::cout << "\n=== libVC_testUtils ===\n" << std::endl;
  std::cout << ">>> Test Configuration" << std::endl 
            << "This test validates the basic utility interface of libVersioningCompiler." << std::endl
            << "- Initialize default system compiler and create version v using utility functions." << std::endl
            << "- Validate test_function(x) that computes x*x and stores the result in a global variable." << std::endl
            << "- Through test_function3(x) verify the global variable matches expected value x." << std::endl;
  vc_utils_init();
  opt_list_t options;
  options.push_back(Option("O", "-O", "2"));
  options.push_back(Option("O", "-D", "TEST_FUNCTION"));
  std::vector<std::string> functions;
  functions.push_back(TEST_FUNCTION);
  functions.push_back(THIRD_FUNCTION);
  std::vector<std::filesystem::path> test_code_path;
  test_code_path.push_back(PATH_TO_C_TEST_CODE);
  version_ptr_t v = createVersion(test_code_path, functions, options);
  std::vector<void *> simbols = compileAndGetSymbols(v);
  signature_t fn_ptr = (signature_t)simbols[0];
  signature_t_new fn_ptr_new = (signature_t_new)simbols[1];
  std::cout << "\n>>> Test Case" << std::endl;
  if (fn_ptr){ 
    std::cout << "Test 01: Version v  --> test_function(3)\t";
    checkResult(fn_ptr(3),9.f);
  } else
    std::cerr << "Error: function pointer unavailable" << std::endl;
  if (fn_ptr_new){
    std::cout << "Test 02: Version v  --> test_function3(9)\t";
    if(fn_ptr_new(9.f) && !ret_value)
      ret_value=1;
  } else
    std::cerr << "Error: function pointer unavailable" << std::endl;
  return ret_value;
}
