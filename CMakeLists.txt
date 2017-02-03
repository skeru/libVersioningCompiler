# Copyright 2017 Politecnico di Milano.
# Developed by : Stefano Cherubin, Davide Gadioli
#                PhD student, Politecnico di Milano
#                <first_name>.<family_name>@polimi.it
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

cmake_minimum_required(VERSION 3.0.2)

#----- Set the name of the Version Compiler library
set (VC_LIB_NAME "VersioningCompiler")

###############################################
##       Set the library directory           ##
###############################################

link_directories( ${CMAKE_CURRENT_BINARY_DIR} )

###############################################
##             EXTERNAL LIB                  ##
###############################################

# Load additional CMake modules
list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# DL library
find_package(DL REQUIRED)
include_directories(${DL_INCLUDE_DIR})
link_directories(${DL_LIBRARY})

# The uuuid library
find_package(UUID REQUIRED)
include_directories(${UUID_INCLUDE_DIR})
link_directories(${UUID_LIBRARY})

# LibClang library
find_package(LibClang)
include_directories(${LIBCLANG_INCLUDES})
link_directories(${LIBCLANG_LIBRARY_DIR})

# LLVM library
find_package(LLVM)
include_directories(${LLVM_INCLUDE_DIR})
link_directories(${LLVM_LIBRARY_DIR})

set (LLVM_VERSION_MIN_REQUIRED "4.0.0")

if ( HAVE_LLVM AND
     (NOT ( LLVM_VERSION VERSION_LESS LLVM_VERSION_MIN_REQUIRED )) AND
     HAVE_LIBCLANG )
  set ( ENABLE_CLANG_AS_LIB true )
else()
  if (NOT HAVE_LLVM)
    message( WARNING "LLVM not detected" )
  else(NOT HAVE_LLVM)
    if (LLVM_VERSION VERSION_LESS LLVM_VERSION_MIN_REQUIRED)
      message( WARNING "You are using an obsolete version of LLVM:\
\n\t${LLVM_VERSION} detected.\t\
Minimum required is ${LLVM_VERSION_MIN_REQUIRED}" )
    else(LLVM_VERSION VERSION_LESS LLVM_VERSION_MIN_REQUIRED)
      message( WARNING "LibClang not detected" )
    endif (LLVM_VERSION VERSION_LESS LLVM_VERSION_MIN_REQUIRED)
  endif (NOT HAVE_LLVM)
  message( WARNING "Clang-as-a-library will be disabled" )
endif()

set (VC_LIB_DEPS ${DL_LIBRARY}
                 ${UUID_LIBRARY}
                )

if (ENABLE_CLANG_AS_LIB)
  set (VC_LIB_DEPS ${VC_LIB_DEPS}
                   ${LIBCLANG_LIBRARIES}
                   ${LLVM_MODULE_LIBS}
                   )

  set ( VC_DEPS_EXTRA_LINK_FLAGS "${LLVM_LFLAGS}" )
endif (ENABLE_CLANG_AS_LIB)

list ( REMOVE_DUPLICATES  VC_LIB_DEPS )

###############################################
##             LIBRARY                       ##
###############################################

if (NOT CMAKE_BUILD_TYPE)
  message ("No build type detected. Build type set to Release by default")
  set ( CMAKE_BUILD_TYPE Release )
endif()

set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
set ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVC_DEBUG" )
set ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fPIC" )

#----- Sources
set ( SRC_PREFIX "lib" )
set ( VC_LIB_SRC
                ${SRC_PREFIX}/Version.cpp
                ${SRC_PREFIX}/Option.cpp
                ${SRC_PREFIX}/Compiler.cpp
                ${SRC_PREFIX}/CompilerImpl/SystemCompiler.cpp
                ${SRC_PREFIX}/CompilerImpl/SystemCompilerOptimizer.cpp
                )

#----- Headers
set ( VC_LIB_HDR_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/include/versioningCompiler )
set ( VC_LIB_HDR1
                ${VC_LIB_HDR_PREFIX}/Version.hpp
                ${VC_LIB_HDR_PREFIX}/Option.hpp
                ${VC_LIB_HDR_PREFIX}/Compiler.hpp
                ${VC_LIB_HDR_PREFIX}/Utils.hpp
                ${VC_LIB_HDR_PREFIX}/DebugUtils.hpp
                )

set ( VC_LIB_HDR_PREFIX2 ${CMAKE_CURRENT_SOURCE_DIR}/include/versioningCompiler/CompilerImpl )
set ( VC_LIB_HDR2
                ${VC_LIB_HDR_PREFIX2}/SystemCompiler.hpp
                ${VC_LIB_HDR_PREFIX2}/SystemCompilerOptimizer.hpp
                )


#----- Additionl files to enable Clang-as-a-library
if (ENABLE_CLANG_AS_LIB)
  set_source_files_properties(${VC_LIB_HDR_PREFIX}/Features.hpp PROPERTIES
                              COMPILE_FLAGS "-DENABLE_CLANG_AS_LIB=true"
                              )

  set (VC_LIB_SRC ${VC_LIB_SRC}
                  ${SRC_PREFIX}/CompilerImpl/ClangLibCompiler.cpp
                  ${SRC_PREFIX}/CompilerImpl/ClangLLVM/FileLogDiagnosticConsumer.cpp
                  ${SRC_PREFIX}/CompilerImpl/ClangLLVM/LLVMInstanceManager.cpp
                  )

  set ( VC_LIB_HDR2 ${VC_LIB_HDR2}
                    ${VC_LIB_HDR_PREFIX2}/ClangLibCompiler.hpp
                    )

set ( VC_LIB_HDR_PREFIX3 ${CMAKE_CURRENT_SOURCE_DIR}/include/versioningCompiler/CompilerImpl/ClangLLVM )

set ( VC_LIB_HDR3
                ${VC_LIB_HDR_PREFIX3}/OptUtils.hpp
                ${VC_LIB_HDR_PREFIX3}/FileLogDiagnosticConsumer.hpp
                ${VC_LIB_HDR_PREFIX3}/LLVMInstanceManager.hpp
                )
endif(ENABLE_CLANG_AS_LIB)

set (VC_LIB_HDR ${VC_LIB_HDR1} ${VC_LIB_HDR2} ${VC_LIB_HDR3} )

#----- setup library

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/include )

add_library( ${VC_LIB_NAME} STATIC ${VC_LIB_SRC} ${VC_LIB_HDR} )
target_link_libraries( ${VC_LIB_NAME} ${VC_LIB_DEPS} )

###############################################
##               CONFIGURE                   ##
###############################################

set (FIND_LIBVC_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/config/FindLibVersioningCompiler.cmake")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config/FindLibVersioningCompiler.cmake.in"
  "${FIND_LIBVC_OUTPUT}"
  @ONLY
)

###############################################
##           INSTALL LIBRARY VC              ##
###############################################

install( TARGETS ${VC_LIB_NAME} DESTINATION lib )
install( FILES ${VC_LIB_HDR1} DESTINATION include/versioningCompiler )
install( FILES ${VC_LIB_HDR2} DESTINATION include/versioningCompiler/CompilerImpl )
install( FILES ${FIND_LIBVC_OUTPUT} DESTINATION lib/cmake )
if (ENABLE_CLANG_AS_LIB)
  install( FILES ${VC_LIB_HDR3} DESTINATION include/versioningCompiler/CompilerImpl/ClangLLVM )
endif (ENABLE_CLANG_AS_LIB)

###############################################
##             TEST APP                      ##
###############################################

#----- Sources
set ( VC_TEST_APP_SRC Test.cpp )

set ( VC_EXE_NAME libVC_test )

if (ENABLE_CLANG_AS_LIB)
  set_source_files_properties(${VC_TEST_APP_SRC} PROPERTIES
                              COMPILE_FLAGS "-flto -DHAVE_CLANG_AS_LIB -DFORCED_PATH_TO_TEST=\\\"${CMAKE_CURRENT_SOURCE_DIR}/test_code\\\""
                              )
else (ENABLE_CLANG_AS_LIB)
  set_source_files_properties(${VC_TEST_APP_SRC} PROPERTIES
                              COMPILE_FLAGS "-flto -DFORCED_PATH_TO_TEST=\\\"${CMAKE_CURRENT_SOURCE_DIR}/test_code\\\""
                              )
endif (ENABLE_CLANG_AS_LIB)

add_executable(${VC_EXE_NAME} ${VC_TEST_APP_SRC})

link_directories(${LLVM_LIBRARY_DIR} ${LIBCLANG_LIBRARY_DIR} "${CMAKE_INSTALL_PREFIX}/lib")
target_link_libraries(${VC_EXE_NAME} ${VC_LIB_NAME} ${VC_LIB_DEPS})


## testUtils.cpp
#----- Sources

set ( VC_TEST2_APP_SRC TestUtils.cpp )

set ( VC_EXE2_NAME libVC_testUtils )


if (ENABLE_CLANG_AS_LIB)
  set_source_files_properties(${VC_TEST2_APP_SRC} PROPERTIES
                              COMPILE_FLAGS "-flto -DHAVE_CLANG_AS_LIB -DFORCED_PATH_TO_TEST=\\\"${CMAKE_CURRENT_SOURCE_DIR}/test_code\\\""
                              )
else (ENABLE_CLANG_AS_LIB)
  set_source_files_properties(${VC_TEST2_APP_SRC} PROPERTIES
                              COMPILE_FLAGS "-flto -DFORCED_PATH_TO_TEST=\\\"${CMAKE_CURRENT_SOURCE_DIR}/test_code\\\""
                              )
endif (ENABLE_CLANG_AS_LIB)

add_executable(${VC_EXE2_NAME} ${VC_TEST2_APP_SRC})

link_directories(${LLVM_LIBRARY_DIR} ${LIBCLANG_LIBRARY_DIR} "${CMAKE_INSTALL_PREFIX}/lib")
target_link_libraries(${VC_EXE2_NAME} ${VC_LIB_NAME} ${VC_LIB_DEPS})

###############################################
##            INSTALL TEST BIN               ##
###############################################

install( TARGETS ${VC_EXE_NAME} DESTINATION bin/test )
install( TARGETS ${VC_EXE2_NAME} DESTINATION bin/test )
