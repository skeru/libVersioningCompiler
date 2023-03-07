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
 *                Moreno Giussani
 *                Ms student, Politecnico di Milano
 *                <first_name>.<family_name>@mail.polimi.it
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
#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Mangler.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "versioningCompiler/CompilerImpl/ClangLibCompiler.hpp"
#include "versioningCompiler/CompilerImpl/JITCompiler.hpp"
#include "versioningCompiler/CompilerImpl/SystemCompiler.hpp"
#include "versioningCompiler/CompilerImpl/SystemCompilerOptimizer.hpp"
#include "versioningCompiler/Version.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
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

#ifndef TEST_FUNCTION_LBL
#define TEST_FUNCTION_LBL "TEST_FUNCTION"
#endif


using namespace llvm;
using namespace orc;

// someone should provide the signature of the function now versioning
// in the form of function pointer type.
typedef int (*signature_t)(int);

int main(int argc, char const *argv[]) {

  std::cout << "Setting up builder.." << std::endl;

  vc::Version::Builder builder;
  builder._functionName.push_back(TEST_FUNCTION);
  builder._fileName_src.push_back(PATH_TO_C_TEST_CODE);
  builder.addFunctionFlag(TEST_FUNCTION_LBL);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  // ---------- Compiler initialization ---------
  std::cout << "Setting up compiler.." << std::endl;
  vc::compiler_ptr_t jitCompiler = vc::make_compiler<vc::JITCompiler>(
          "jitCompiler",
          ".",
          "./test_jit.log"
  );


  builder._compiler = jitCompiler;
  builder._autoremoveFilesEnable = false;
  builder._optOptionList = {
          vc::Option("mem2reg", "-mem2reg"),
          vc::Option("o", "-O", "3"),
  };
  std::cout << "Building version.." << std::endl;
  vc::version_ptr_t myversion = builder.build();

  std::cout << "Preparing IR for version.." << std::endl;
  bool ok = myversion->prepareIR();
  if (!ok) {
    if (!jitCompiler->hasIRSupport()) {
      std::cerr << "Error: something went wrong with the compiler." << std::endl;
    } else if (!myversion->hasGeneratedIR()) {
      std::cerr << "Error: generation of IR for myversion failed." << std::endl;
    } else {
      std::cerr << "Error: optimization of IR for myversion failed." << std::endl;
    }
    std::cerr << "\tPlease check the compiler/optimizer install path" << std::endl;
    return -1;
  }
  std::cout << "IR prepared. Going for myversion compilation.." << std::endl;

  ok = myversion->compile();
  if (!ok) {
    std::cerr << "Error: myversion compilation has failed." << std::endl;
    if (myversion->hasGeneratedBin()) {
      std::cerr << "\tmyversion has generated binary " << myversion->getFileName_bin() << std::endl;
    }
    if (myversion->hasLoadedSymbol()) {
      std::cerr << "\tmyversion has loaded symbol" << std::endl;
    }
    return -1;
  }
  std::cout << "Object myversion compiled." << std::endl;

  std::cout << "Executing myversion symbol." << std::endl;

  std::vector<signature_t> f;
  f.push_back((signature_t) myversion->getSymbol());
  std::cout << "Result 1: " << f[0](42) << std::endl;
  std::cout << "Result 2: " << f[0](12) << std::endl;
  std::cout << "Result 3: " << f[0](128) << std::endl;

  std::cout << "Folding myversion object." << std::endl;
  myversion->fold();

  std::cout << "Version folded, reloading it." << std::endl;
  myversion->reload();
  std::cout << "Executing myversion reloaded symbol." << std::endl;
  signature_t reloaded = (signature_t) myversion->getSymbol();
  if (reloaded) {
    std::cout << "Result r1: " << reloaded(42) << std::endl;
    std::cout << "Result r2: " << reloaded(12) << std::endl;
    std::cout << "Result r3: " << reloaded(128) << std::endl;
  } else {
    std::cerr << "Error in folding and reloading v3" << std::endl;
  }

  myversion->fold();

  return 0;
}