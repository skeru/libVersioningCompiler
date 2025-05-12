# Try to find libclang
#
# Once done this will define:
# - LIBCLANG_FOUND / LibClang_FOUND
#               System has libclang.
# - LIBCLANG_INCLUDE_DIRS
#               The libclang include directories.
# - LIBCLANG_LIBRARIES
#               The libraries needed to use libclang.
# - LIBCLANG_LIBRARY_DIR
#               The path to the directory containing libclang.

# Find the latest llvm version, unless LLVM_FOUND is yet set

find_package(LLVM_config QUIET)

if(NOT LLVM_CONFIG_EXECUTABLE)
  if(LIBCLANG_FIND_VERBOSE)
    message(STATUS "FindLibClang is looking for llvm-config...")
  endif(LIBCLANG_FIND_VERBOSE)
  find_program(
    LLVM_CONFIG_EXECUTABLE
    NAMES "llvm-config"
    DOC "llvm-config executable"
    PATHS ${LLVM_TOOLS_BINARY_DIR} /usr/bin
    NO_DEFAULT_PATH)
endif(NOT LLVM_CONFIG_EXECUTABLE)

if(NOT LLVM_INCLUDE_DIR)
  message(STATUS "FindLibClang did not find LLVM include directories")
else(NOT LLVM_INCLUDE_DIR)
  if(LIBCLANG_FIND_VERBOSE)
    message(STATUS "Looking for clang headers in = ${LLVM_INCLUDE_DIR}")
  endif(LIBCLANG_FIND_VERBOSE)
  find_path(
    LIBCLANG_INCLUDE_DIRS clang-c/Index.h
    PATHS ${LLVM_INCLUDE_DIR}
    NO_DEFAULT_PATH
    PATH_SUFFIXES LLVM/include # Windows package from http://llvm.org/releases/
    DOC "The path to the directory that contains clang-c/Index.h"
  )
endif(NOT LLVM_INCLUDE_DIR)

if (LIBCLANG_INCLUDE_DIRS)
  set(LIBCLANG_FOUND TRUE)
else(LIBCLANG_INCLUDE_DIRS)
  message(STATUS "LibClang could not find its include")
endif(LIBCLANG_INCLUDE_DIRS)


# Find LLVM Config
if(NOT LLVM_CONFIG_EXECUTABLE)
  set(LLVM_PATH_CANDIDATES "${LLVM_ROOT}")
  list(APPEND LLVM_PATH_CANDIDATES "${LLVM_TOOLS_BINARY_DIR}") # Manually specified by user
  list(APPEND LLVM_PATH_CANDIDATES "/usr/bin/") # Ubuntu
  list(APPEND LLVM_PATH_CANDIDATES "/opt/homebrew/opt/llvm/") # Manjaro
  list(APPEND LLVM_PATH_CANDIDATES "/usr/local/Cellar/llvm/") # Homebrew Cellar
  if(LIBCLANG_FIND_VERBOSE)
    message(STATUS "FindLibClang is looking for llvm-config...")
  endif(LIBCLANG_FIND_VERBOSE)
  find_program(
    LLVM_CONFIG_EXECUTABLE
    NAMES "llvm-config-${LLVM_VERSION_MAJOR}" "llvm-config"
    DOC "llvm-config executable"
    PATHS ${LLVM_PATH_CANDIDATES}
    NO_DEFAULT_PATH)
endif(NOT LLVM_CONFIG_EXECUTABLE)

if(LIBCLANG_FIND_VERBOSE)
  if(NOT LLVM_CONFIG_EXECUTABLE)
    message(WARNING "FindLibClang could not find llvm-config.")
  endif(NOT LLVM_CONFIG_EXECUTABLE)
  message(STATUS "LIBCLANG_INCLUDE_DIRS = ${LIBCLANG_INCLUDE_DIRS}")
endif(LIBCLANG_FIND_VERBOSE)

# check if is being linked as shared or not
if(NOT LLVM_SHARED_MODE)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --shared-mode
    OUTPUT_VARIABLE LLVM_SHARED_MODE
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(NOT LLVM_SHARED_MODE)

# Find libclang library

# On Windows with MSVC, the import library uses the ".imp" file extension
# instead of the common ".lib"
if(MSVC)
  if(LIBCLANG_FIND_VERBOSE)
    message(STATUS "Looking for libclang in windows mode")
  endif(LIBCLANG_FIND_VERBOSE)
  find_file(
    LIBCLANG_LIBRARY
    libclang-${LLVM_VERSION}.imp
    libclang-${LLVM_VERSION_MAJOR}.imp
    libclang.imp
    libclang-${LLVM_VERSION}.lib
    libclang-${LLVM_VERSION_MAJOR}.lib
    libclang.lib
    libclang-${LLVM_VERSION}.dll
    libclang-${LLVM_VERSION_MAJOR}.dll
    libclang.dll
    PATH_SUFFIXES LLVM/lib
    DOC "The file that corresponds to the libclang library.")
else(MSVC)
  if(LLVM_SHARED_MODE STREQUAL "shared")
    if(LIBCLANG_FIND_VERBOSE)
      message(STATUS "Looking for libclang in shared mode")
    endif(LIBCLANG_FIND_VERBOSE)
    find_library(
      LIBCLANG_LIBRARY
      NAMES libclang-${LLVM_VERSION}.so.1 libclang-${LLVM_VERSION_MAJOR}.so.1
            libclang-${LLVM_VERSION}.so libclang-${LLVM_VERSION_MAJOR}.so
            libclang clang
      PATHS ${LLVM_LIBRARY_DIR}
      NO_DEFAULT_PATH
      DOC "The file that corresponds to the libclang library.")
  else(LLVM_SHARED_MODE STREQUAL "shared")
    if(LIBCLANG_FIND_VERBOSE)
      message(STATUS "Looking for libclang in static mode")
    endif(LIBCLANG_FIND_VERBOSE)
    # Look for libclang library, giving higher priority to statically linked
    # libraries (.a,.lib)
    find_library(
      LIBCLANG_LIBRARY
      NAMES libclang.a libclang.lib libclang.imp libclang clang
      PATHS ${LLVM_LIBRARY_DIR}
      NO_DEFAULT_PATH
      DOC "The file that corresponds to the libclang library.")
  endif(LLVM_SHARED_MODE STREQUAL "shared")
endif(MSVC)

if(NOT LIBCLANG_LIBRARY)
  message(STATUS "LibClang library ... = NOT FOUND")
else()
  message(STATUS "LibClang library ... = ${LIBCLANG_LIBRARY}")
endif()

get_filename_component(LIBCLANG_LIBRARY_DIR ${LIBCLANG_LIBRARY} PATH)

macro(FIND_AND_ADD_CLANG_LIB _libname_)
  if(LLVM_SHARED_MODE STREQUAL "shared")
    # Some distros name libraries as libraries.so.versionMajor
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".so.${LLVM_VERSION_MAJOR}")
    find_library(
      CLANG_${_libname_}_LIB
      NAMES ${_libname_}.so ${_libname_}
      PATHS ${LLVM_LIBRARY_DIR}
      NO_DEFAULT_PATH)
    list(REMOVE_ITEM CMAKE_FIND_LIBRARY_SUFFIXES ".so.${LLVM_VERSION_MAJOR}")
  else(LLVM_SHARED_MODE STREQUAL "shared") # if static
    # I would then expect that distros name the static version of libclang as
    # .a.versionMajor
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".a.${LLVM_VERSION_MAJOR}")
    find_library(
      CLANG_${_libname_}_LIB
      NAMES ${_libname_}.a ${_libname_}.lib ${_libname_}
      PATHS ${LLVM_LIBRARY_DIR}
      NO_DEFAULT_PATH)
    list(REMOVE_ITEM CMAKE_FIND_LIBRARY_SUFFIXES ".a.${LLVM_VERSION_MAJOR}")
  endif(LLVM_SHARED_MODE STREQUAL "shared")
  if(CLANG_${_libname_}_LIB)
    set(LIBCLANG_LIBRARIES ${LIBCLANG_LIBRARIES} ${CLANG_${_libname_}_LIB})
    get_filename_component(LIBCLANG_${_libname_}_LIBRARY_DIR
                           ${LIBCLANG_LIBRARY} PATH)
    set(LIBCLANG_LIBRARY_DIR ${LIBCLANG_LIBRARY_DIR}
                             ${LIBCLANG_${_libname_}_LIBRARY_DIR})
  else(CLANG_${_libname_}_LIB)
    if(LIBCLANG_FIND_VERBOSE)
      message(
        WARNING
          "Clang dependency: Cannot find library '${_libname_}' in paths '${LLVM_LIBRARY_DIR}'."
      )
    endif(LIBCLANG_FIND_VERBOSE)
  endif(CLANG_${_libname_}_LIB)
endmacro(FIND_AND_ADD_CLANG_LIB)

set(LIBCLANG_LIBRARIES)

if(NOT LLVM_FOUND)
  if(LIBCLANG_FIND_VERBOSE)
    message(WARNING "LLVM has not been found. Avoid searching for LibClang.")
  endif(LIBCLANG_FIND_VERBOSE)
else(NOT LLVM_FOUND)
# Only Look for Clang libraries if LLVM somehow been found
find_and_add_clang_lib(clang-cpp)
if(NOT CLANG_clang-cpp_LIB)
  if(LIBCLANG_FIND_VERBOSE)
    message(WARNING "Cannot find library libclang! Attempting to find the single libraries!")
  endif(LIBCLANG_FIND_VERBOSE)
  find_and_add_clang_lib(clang NAMES clang libclang)
  # Clang shared library provides just the limited C interface, so it can not be
  # used.  We look for the static libraries.
  find_and_add_clang_lib(clangFrontend)
  find_and_add_clang_lib(clangSerialization)
  find_and_add_clang_lib(clangDriver)
  find_and_add_clang_lib(clangCodeGen)
  find_and_add_clang_lib(clangParse)
  find_and_add_clang_lib(clangSema)
  find_and_add_clang_lib(clangAnalysis)
  find_and_add_clang_lib(clangAST)
  find_and_add_clang_lib(clangEdit)
  find_and_add_clang_lib(clangLex)
  find_and_add_clang_lib(clangBasic)
endif(NOT CLANG_clang-cpp_LIB)
list(REMOVE_DUPLICATES LIBCLANG_LIBRARY_DIR)
# -----------------------------------------------------------------------------
# Actions taken when all components have been found
if(LIBCLANG_INCLUDE_DIRS AND LIBCLANG_LIBRARY)
  set(LIBCLANG_FOUND TRUE)
else(LIBCLANG_INCLUDE_DIRS AND LIBCLANG_LIBRARY)
  if(LIBCLANG_FIND_VERBOSE)
    if(NOT LIBCLANG_INCLUDE_DIRS)
      message(STATUS "Unable to find libClang header files!")
    endif(NOT LIBCLANG_INCLUDE_DIRS)
    if(NOT LIBCLANG_LIBRARY)
      message(STATUS "Unable to find libClang library files!")
    endif(NOT LIBCLANG_LIBRARY)
  endif(LIBCLANG_FIND_VERBOSE)
endif(LIBCLANG_INCLUDE_DIRS AND LIBCLANG_LIBRARY)

endif(NOT LLVM_FOUND)

# Wrap up

if(LIBCLANG_FOUND)
  if(LIBCLANG_FIND_VERBOSE)
    message(STATUS "Found components for libClang")
    message(STATUS "LibClang headers dirs = ${LIBCLANG_INCLUDE_DIRS}")
    message(STATUS "LibClang libs path .. = ${LIBCLANG_LIBRARY_DIR}")
    message(STATUS "LibClang libs ....... = ${LIBCLANG_LIBRARIES}")
  endif(LIBCLANG_FIND_VERBOSE)
else(LIBCLANG_FOUND)
  if(LIBCLANG_FIND_REQUIRED)
    message(WARNING "Could not find libClang!")
  endif(LIBCLANG_FIND_REQUIRED)
endif(LIBCLANG_FOUND)

mark_as_advanced(LIBCLANG_FOUND LIBCLANG_INCLUDE_DIRS
                 LIBCLANG_LIBRARIES LIBCLANG_LIBRARY_DIR)
