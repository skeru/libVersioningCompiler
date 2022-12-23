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

if(NOT LLVM_FOUND)
  set(LLVM_KNOWN_MAJOR_VERSIONS
      15
      14
      13
      12
      11
      10
      9
      8
      7
      6.0
      5.0
      4.0
      3.9
      3.8)
  foreach(ver ${LLVM_KNOWN_MAJOR_VERSIONS})
    find_package(LLVM ${ver} QUIET)
    if(LLVM_FOUND)
      # Call the main findLLVM.cmake to check LLVM setup and set some flags such as:
      #   LLVM_LIBRARY_DIR, LLVM_INCLUDE_DIR, LLVM_TOOLS_BINARY_DIR, LLVM_VERSION_MAJOR, LLVM_PACKAGE_VERSION
      if(LLVM_FIND_VERBOSE)
        find_package(LLVM ${ver} CONFIG)
      else(LLVM_FIND_VERBOSE)
        find_package(LLVM ${ver} CONFIG QUIET)
      endif(LLVM_FIND_VERBOSE)
      break()
    endif(LLVM_FOUND)
  endforeach(ver ${LLVM_KNOWN_VERSIONS})
endif(NOT LLVM_FOUND)

find_path(
  LIBCLANG_INCLUDE_DIRS clang-c/Index.h
  PATHS ${LLVM_INCLUDE_DIR}
  PATH_SUFFIXES LLVM/include # Windows package from http://llvm.org/releases/
  DOC "The path to the directory that contains clang-c/Index.h")

# On Windows with MSVC, the import library uses the ".imp" file extension
# instead of the comon ".lib"
if (MSVC)
  find_file (LIBCLANG_LIBRARY libclang.imp
    PATH_SUFFIXES LLVM/lib
    DOC "The file that corresponds to the libclang library.")
endif(MSVC)

find_library (LIBCLANG_LIBRARY NAMES libclang.imp libclang clang
  PATHS ${LLVM_LIBRARY_DIR}
  PATH_SUFFIXES LLVM/lib # Windows package from http://llvm.org/releases/
  DOC "The file that corresponds to the libclang library.")

if (NOT LIBCLANG_LIBRARY)
  message(STATUS "LibClang library ... = NOT FOUND")
else()
  message(STATUS "LibClang library ... = ${LIBCLANG_LIBRARY}")
endif()

get_filename_component(LIBCLANG_LIBRARY_DIR ${LIBCLANG_LIBRARY} PATH)

macro(FIND_AND_ADD_CLANG_LIB _libname_)
  find_library(
    CLANG_${_libname_}_LIB
    NAMES ${_libname_}
    PATHS ${LLVM_LIBRARY_DIR})
  if(CLANG_${_libname_}_LIB)
    set(LIBCLANG_LIBRARIES ${LIBCLANG_LIBRARIES} ${CLANG_${_libname_}_LIB})
    get_filename_component(LIBCLANG_${_libname_}_LIBRARY_DIR
                           ${LIBCLANG_LIBRARY} PATH)
    set(LIBCLANG_LIBRARY_DIR ${LIBCLANG_LIBRARY_DIR}
                             ${LIBCLANG_${_libname_}_LIBRARY_DIR})
  else()
    if(LIBCLANG_FIND_VERBOSE)
      message(
        WARNING
          "Clang dependency: Cannot find library '${_libname_}' in paths '${LLVM_LIBRARY_DIR}'."
      )
    endif(LIBCLANG_FIND_VERBOSE)
  endif(CLANG_${_libname_}_LIB)
endmacro(FIND_AND_ADD_CLANG_LIB)

set (LIBCLANG_LIBRARIES)
# check if is being linked as shared or not
if(NOT LLVM_CONFIG_EXECUTABLE)
  find_program(
    LLVM_CONFIG_EXECUTABLE
    NAMES "llvm-config-${LLVM_VERSION_MAJOR}" "llvm-config"
    DOC "llvm-config executable"
    PATHS ${LLVM_TOOLS_BINARY_DIR})
endif(NOT LLVM_CONFIG_EXECUTABLE)

# Some distros name libclang as .so.version
list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".so.${LLVM_VERSION_MAJOR}")
find_library(
  is_there_clang-cpp
  NAMES clang-cpp
  PATHS ${LLVM_LIBRARY_DIR})
if(is_there_clang-cpp)
  find_and_add_clang_lib(clang-cpp)
  list(REMOVE_DUPLICATES LIBCLANG_LIBRARY_DIR)
else()
  message(WARNING "Cannot find library libclang! Attempting to find the single libraries!")
  find_and_add_clang_lib(clang-cpp)
  find_and_add_clang_lib(clang NAMES clang libclang)
  # Clang shared library provides just the limited C interface, so it
  # can not be used.  We look for the static libraries.
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
  list (REMOVE_DUPLICATES LIBCLANG_LIBRARY_DIR)
endif()
list(REMOVE_ITEM CMAKE_FIND_LIBRARY_SUFFIXES ".so.${LLVM_VERSION_MAJOR}")
# -----------------------------------------------------------------------------
# Actions taken when all components have been found

if (LIBCLANG_INCLUDE_DIRS AND LIBCLANG_LIBRARY)
  set (LIBCLANG_FOUND TRUE)
else (LIBCLANG_INCLUDE_DIRS AND LIBCLANG_LIBRARY)
  if (LIBCLANG_FIND_VERBOSE)
    if (NOT LIBCLANG_INCLUDE_DIRS)
      message (STATUS "Unable to find libClang header files!")
    endif (NOT LIBCLANG_INCLUDE_DIRS)
    if (NOT LIBCLANG_LIBRARY)
      message (STATUS "Unable to find libClang library files!")
    endif (NOT LIBCLANG_LIBRARY)
  endif (LIBCLANG_FIND_VERBOSE)
endif (LIBCLANG_INCLUDE_DIRS AND LIBCLANG_LIBRARY)

if (LIBCLANG_FOUND)
  if (LIBCLANG_FIND_VERBOSE)
    message (STATUS "Found components for libClang")
    message (STATUS "LibClang headers dirs= ${LIBCLANG_INCLUDE_DIRS}")
    message (STATUS "LibClang libs path . = ${LIBCLANG_LIBRARIES}")
  endif (LIBCLANG_FIND_VERBOSE)
else (LIBCLANG_FOUND)
  if (LIBCLANG_FIND_REQUIRED)
    message (WARNING "Could not find libClang!")
  endif (LIBCLANG_FIND_REQUIRED)
endif (LIBCLANG_FOUND)

if(LIBCLANG_FOUND)
  set(LibClang_FOUND TRUE)
endif(LIBCLANG_FOUND)

mark_as_advanced (
    LibClang_FOUND
    LIBCLANG_FOUND
    LIBCLANG_INCLUDE_DIRS
    LIBCLANG_LIBRARIES
    LIBCLANG_LIBRARY_DIR
  )
