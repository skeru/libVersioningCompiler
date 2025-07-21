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
#include <stdio.h>
#include <math.h>
#include <float.h>

#ifdef TEST_FUNCTION

float global_var = -1.0f;

/**
 * Test function for the versioning compiler library.
 * In the test, this function is compiled and loaded dynamically.
 */
float test_function(int x) {
  // Intentional implict conversion from int to float.
  // Used to test whether the compiler detects the warning.
  float y = x; 
  y = y * y;
  global_var = y;
  return y;
}

/**
 * Test function for the versioning compiler library.
 * In the test, this function is compiled and loaded dynamically.
 */
float test_function2(int x) {
  float y;
  if (!x) {
    y = global_var;
  } else {
    y = (float)(x * x * x);
  }
  return y;
}

/**
 * Test function for the versioning compiler library.
 * In the test, this function is compiled and loaded dynamically.
 * This function features a call to an external function (printf).
 */
int test_function3(float expected){
  if (fabs(expected - global_var) < FLT_EPSILON) {
    printf("PASSED\n");
    return 0;
  }
  printf("FAILED: global variable = %.3f, got = %.3f\n", global_var, expected);
  return 1;
}

#endif /* TEST_FUNCTION */
