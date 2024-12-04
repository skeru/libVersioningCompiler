# Copyright 2017 Politecnico di Milano.
# Developed by : Stefano Cherubin, Davide Gadioli
#                PhD student, Politecnico di Milano
#                <first_name>.<family_name>@polimi.it
#                Alessandro Vacca
#                MSc student, Politecnico di Milano
#                <first_name>2.<family_name>@mail.polimi.it
#
# This file is part of libVersioningCompiler
#
# libVersioningCompiler is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation, either version 3
# of the License, or (at your option) any later version.
#
# libVersioningCompiler is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libVersioningCompiler. If not, see <http://www.gnu.org/licenses/>

# cmake_path had been introduced in cmake 3.20
cmake_minimum_required(VERSION 3.20)

project(
  libVersioningCompiler
  VERSION 3.0
  DESCRIPTION
    "library for runtime compilation and dynamic loading of a C/C++ function"
  LANGUAGES CXX C)

# Add the option to enable or disable the JIT compiler (default=OFF), as it has not been adapted to LLVM >= 18
option(ENABLE_JIT "Enable JIT compilation support" OFF)

#----- Set the name of the Version Compiler library
set(VC_LIB_NAME "VersioningCompiler")

#############################################
#       Set the library directory           #
#############################################

link_directories(${CMAKE_CURRENT_BINARY_DIR})

#############################################
#             EXTERNAL LIB                  #
#############################################

# Load additional CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# check for DL library
if(NOT APPLE AND NOT CMAKE_DL_LIBS)
  message(FATAL_ERROR "Missing DL libs!")
endif(NOT APPLE AND NOT CMAKE_DL_LIBS)

# The uuuid library
find_package(UUID REQUIRED)
include_directories(${UUID_INCLUDE_DIR})
link_directories(${UUID_LIBRARY})

# LibClang library
find_package(LibClang)
if(LIBCLANG_FOUND)
  include_directories(${LIBCLANG_INCLUDE_DIRS})
  link_directories(${LIBCLANG_LIBRARY_DIR})
endif(LIBCLANG_FOUND)

# LLVM library
find_package(LLVM_config)
if(LLVM_FOUND)
  include_directories(${LLVM_INCLUDE_DIR})
  link_directories(${LLVM_LIBRARY_DIR})
endif(LLVM_FOUND)

set(LLVM_VERSION_MIN_REQUIRED "13.0.0")

if(LLVM_FOUND
   AND (NOT (LLVM_PACKAGE_VERSION VERSION_LESS LLVM_VERSION_MIN_REQUIRED))
   AND LIBCLANG_FOUND)
  set(ENABLE_CLANG_AS_LIB true)
else()
  if(NOT LLVM_FOUND)
    message(WARNING "LLVM not detected")
  else(NOT LLVM_FOUND)
    if(LLVM_PACKAGE_VERSION VERSION_LESS LLVM_VERSION_MIN_REQUIRED)
      message(WARNING "You are using an obsolete version of LLVM:\
\n\t${LLVM_PACKAGE_VERSION} detected.\t\
Minimum required is ${LLVM_VERSION_MIN_REQUIRED}")
    else(LLVM_PACKAGE_VERSION VERSION_LESS LLVM_VERSION_MIN_REQUIRED)
      message(WARNING "LibClang not detected")
    endif(LLVM_PACKAGE_VERSION VERSION_LESS LLVM_VERSION_MIN_REQUIRED)
  endif(NOT LLVM_FOUND)
  message(WARNING "Clang-as-a-library will be disabled")
endif()

set(VC_LIB_DEPS ${CMAKE_DL_LIBS} ${UUID_LIBRARY} stdc++)

if(ENABLE_CLANG_AS_LIB)
  set(VC_LIB_DEPS ${VC_LIB_DEPS} ${LIBCLANG_LIBRARIES} ${LLVM_MODULE_LIBFILES}
                  ${LLVM_SYSTEM})

  set(VC_DEPS_EXTRA_LINK_FLAGS "${LLVM_LFLAGS}")
endif(ENABLE_CLANG_AS_LIB)

if(ENABLE_JIT AND NOT ENABLE_CLANG_AS_LIB)
  message(WARNING "JITCompiler requires Clang-as-a-library. Disabling JITCompiler")
  set(ENABLE_JIT OFF)
endif()

list(REMOVE_DUPLICATES VC_LIB_DEPS)

#############################################
#             LIBRARY                       #
#############################################

if(NOT CMAKE_BUILD_TYPE)
  message("No build type detected. Build type set to Release by default")
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# https://gcc.gnu.org/wiki/Visibility#line-14 - Much lower chance of symbol collision.
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# CMAKE_INTERPROCEDURAL_OPTIMIZATION applies different optimizations depending on the compiler.
# It breaks cross compiler compatibility (e.g. library compiled with g++ which is being used with clang later on), so
# I'm not enabling link time optimization by default.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

# There is no no-rtti cmake setting so far (related https://gitlab.kitware.com/cmake/cmake/-/issues/17991) and no no-use-cxa-atexit cmake setting as well.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-use-cxa-atexit -fno-rtti")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

#----- Sources
set(SRC_PREFIX "lib")
set(VC_LIB_SRC
    ${SRC_PREFIX}/Version.cpp
    ${SRC_PREFIX}/Option.cpp
    ${SRC_PREFIX}/Compiler.cpp
    ${SRC_PREFIX}/CompilerImpl/SystemCompiler.cpp
    ${SRC_PREFIX}/CompilerImpl/SystemCompilerOptimizer.cpp
    ${SRC_PREFIX}/CompilerImpl/TAFFOCompiler.cpp)

#----- Headers
set(VC_LIB_HDR_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/include/versioningCompiler)
set(VC_LIB_HDR1
    ${VC_LIB_HDR_PREFIX}/Version.hpp ${VC_LIB_HDR_PREFIX}/Option.hpp
    ${VC_LIB_HDR_PREFIX}/Compiler.hpp ${VC_LIB_HDR_PREFIX}/Utils.hpp)
if(ENABLE_JIT)
  list(APPEND VC_LIB_HDR1 ${VC_LIB_HDR_PREFIX}/JITUtils.hpp)
endif()
set(VC_LIB_HDR_PREFIX2
    ${CMAKE_CURRENT_SOURCE_DIR}/include/versioningCompiler/CompilerImpl)
set(VC_LIB_HDR2
    ${VC_LIB_HDR_PREFIX2}/SystemCompiler.hpp
    ${VC_LIB_HDR_PREFIX2}/SystemCompilerOptimizer.hpp
    ${VC_LIB_HDR_PREFIX2}/TAFFOCompiler.hpp)

#----- Additionl files to enable Clang-as-a-library

if(ENABLE_CLANG_AS_LIB)
  set(COMPILE_DEFINITIONS "ENABLE_CLANG_AS_LIB=true")

  set(VC_LIB_SRC
      ${VC_LIB_SRC}
      ${SRC_PREFIX}/CompilerImpl/ClangLibCompiler.cpp
      ${SRC_PREFIX}/CompilerImpl/ClangLLVM/FileLogDiagnosticConsumer.cpp
      ${SRC_PREFIX}/CompilerImpl/ClangLLVM/LLVMInstanceManager.cpp)

  if(ENABLE_JIT)
    list(APPEND VC_LIB_SRC
            ${SRC_PREFIX}/CompilerImpl/JITCompiler.cpp)
  endif()

  set(VC_LIB_HDR2 ${VC_LIB_HDR2} ${VC_LIB_HDR_PREFIX2}/ClangLibCompiler.hpp)

  if(ENABLE_JIT)
    list(APPEND VC_LIB_HDR2
            ${VC_LIB_HDR_PREFIX2}/JITCompiler.hpp)
  endif()

  set(VC_LIB_HDR_PREFIX3
      ${CMAKE_CURRENT_SOURCE_DIR}/include/versioningCompiler/CompilerImpl/ClangLLVM
  )

  set(VC_LIB_HDR3
      ${VC_LIB_HDR_PREFIX3}/OptUtils.hpp
      ${VC_LIB_HDR_PREFIX3}/FileLogDiagnosticConsumer.hpp
      ${VC_LIB_HDR_PREFIX3}/LLVMInstanceManager.hpp)
endif(ENABLE_CLANG_AS_LIB)

set(VC_LIB_HDR ${VC_LIB_HDR1} ${VC_LIB_HDR2} ${VC_LIB_HDR3})

#----- setup library

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if(ENABLE_CLANG_AS_LIB)
  link_directories(${LLVM_LIBRARY_DIR} ${LIBCLANG_LIBRARY_DIR}
                   "${CMAKE_INSTALL_PREFIX}/lib")
endif(ENABLE_CLANG_AS_LIB)
add_library(${VC_LIB_NAME} STATIC ${VC_LIB_SRC} ${VC_LIB_HDR})
target_link_libraries(${VC_LIB_NAME} ${VC_LIB_DEPS})

#----- additional filesystem helpers

include("${CMAKE_CURRENT_LIST_DIR}/cmake/NormalizePathHelper.cmake")

#############################################
#               CONFIGURE                   #
#############################################

set(FIND_LIBVC_OUTPUT
    "${CMAKE_CURRENT_SOURCE_DIR}/config/FindLibVersioningCompiler.cmake")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config/FindLibVersioningCompiler.cmake.in"
  "${FIND_LIBVC_OUTPUT}" @ONLY)

#############################################
#           INSTALL LIBRARY VC              #
#############################################

install(TARGETS ${VC_LIB_NAME} DESTINATION lib)
install(FILES ${VC_LIB_HDR1} DESTINATION include/versioningCompiler)
install(FILES ${VC_LIB_HDR2}
        DESTINATION include/versioningCompiler/CompilerImpl)
install(FILES ${FIND_LIBVC_OUTPUT} DESTINATION lib/cmake)
if(ENABLE_CLANG_AS_LIB)
  install(FILES ${VC_LIB_HDR3}
          DESTINATION include/versioningCompiler/CompilerImpl/ClangLLVM)
endif(ENABLE_CLANG_AS_LIB)

#############################################
#             TEST APP                      #
#############################################

# If not specified, guess which c++ library should use
if(NOT CPP_LIBRARY)
  if(APPLE)
    set(CPP_LIBRARY "c++")
  elseif(UNIX)
    set(CPP_LIBRARY "stdc++")
  else()
    set(CPP_LIBRARY "c++")
  endif()
endif(NOT CPP_LIBRARY)

# TestJit.cpp
#----- Sources

# It does make no sense building it if there is no LLVM, given that the JIT implementation relies on LLVM.
if(ENABLE_CLANG_AS_LIB AND ENABLE_JIT)
  set(VC_TESTJIT_APP_SRC TestJit.cpp)
  set(VC_EXEJIT_NAME libVC_testJit)
  add_executable(${VC_EXEJIT_NAME} ${VC_TESTJIT_APP_SRC})

  target_link_libraries(${VC_EXEJIT_NAME} ${VC_LIB_NAME} ${VC_LIB_DEPS}
                        ${CPP_LIBRARY})

  target_compile_definitions(${VC_EXEJIT_NAME} PRIVATE -DHAVE_CLANG_AS_LIB)
  set(TEST_CODE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test_code")
  convert_to_native_normalized_path(${TEST_CODE_PATH})
  target_compile_definitions(${VC_EXEJIT_NAME}
                             PRIVATE -DFORCED_PATH_TO_TEST="${TEST_CODE_PATH}")
  if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    target_compile_definitions(${VC_EXEJIT_NAME} PRIVATE -DVC_DEBUG)
  endif(CMAKE_BUILD_TYPE MATCHES DEBUG)
endif(ENABLE_CLANG_AS_LIB AND ENABLE_JIT)

# Test.cpp
#----- Sources
set(VC_TEST_APP_SRC Test.cpp)
set(VC_EXE_NAME libVC_test)
add_executable(${VC_EXE_NAME} ${VC_TEST_APP_SRC})

target_link_libraries(${VC_EXE_NAME} ${VC_LIB_NAME} ${VC_LIB_DEPS}
                      ${CPP_LIBRARY})
set(TEST_CODE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test_code")
convert_to_native_normalized_path(${TEST_CODE_PATH})
target_compile_definitions(${VC_EXE_NAME}
                           PRIVATE -DFORCED_PATH_TO_TEST="${TEST_CODE_PATH}")
target_compile_definitions(
  ${VC_EXE_NAME} PRIVATE -DLLVM_TOOLS_BINARY_DIR="${LLVM_TOOLS_BINARY_DIR}")

if(ENABLE_CLANG_AS_LIB)
  target_compile_definitions(${VC_EXE_NAME} PRIVATE -DHAVE_CLANG_AS_LIB)
endif(ENABLE_CLANG_AS_LIB)
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  target_compile_definitions(${VC_EXE_NAME} PRIVATE -DVC_DEBUG)
endif(CMAKE_BUILD_TYPE MATCHES DEBUG)

# testUtils.cpp
#----- Sources

set(VC_TESTUTILS_APP_SRC TestUtils.cpp)
set(VC_EXEUTILS_NAME libVC_testUtils)
add_executable(${VC_EXEUTILS_NAME} ${VC_TESTUTILS_APP_SRC})

target_link_libraries(${VC_EXEUTILS_NAME} ${VC_LIB_NAME} ${VC_LIB_DEPS}
                      ${CPP_LIBRARY})
set(TEST_CODE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test_code")
convert_to_native_normalized_path(${TEST_CODE_PATH})
target_compile_definitions(${VC_EXEUTILS_NAME}
                           PRIVATE -DFORCED_PATH_TO_TEST="${TEST_CODE_PATH}")

if(ENABLE_CLANG_AS_LIB)
  target_compile_definitions(${VC_EXEUTILS_NAME} PRIVATE -DHAVE_CLANG_AS_LIB)
endif(ENABLE_CLANG_AS_LIB)
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  target_compile_definitions(${VC_EXEUTILS_NAME} PRIVATE -DVC_DEBUG)
endif(CMAKE_BUILD_TYPE MATCHES DEBUG)

#############################################
#            INSTALL TEST BIN               #
#############################################

install(TARGETS ${VC_EXE_NAME} DESTINATION bin/test)
install(TARGETS ${VC_EXEUTILS_NAME} DESTINATION bin/test)
if(ENABLE_CLANG_AS_LIB AND ENABLE_JIT)
  install(TARGETS ${VC_EXEJIT_NAME} DESTINATION bin/test)
endif(ENABLE_CLANG_AS_LIB AND ENABLE_JIT)
