# Try to find libclang
#
# Once done this will define:
# - HAVE_LIBCLANG
#               System has libclang.
# - LIBCLANG_INCLUDES
#               The libclang include directories.
# - LIBCLANG_LIBRARIES
#               The libraries needed to use libclang.
# - LIBCLANG_LIBRARY_DIR
#               The path to the directory containing libclang.

# most recent versions come first
# http://apt.llvm.org/
set (LIBCLANG_KNOWN_LLVM_VERSIONS 4.0.0 4.0
  3.9.0 3.9
  3.8.1 3.8.0 3.8
  3.7.1 3.7.0 3.7
  3.6.2 3.6.1 3.6.0 3.6
  3.5.2 3.5.1 3.5.0 3.5
  3.4.2 3.4.1 3.4
  3.3
  3.2
  3.1)

set (libclang_llvm_header_search_paths)
set (libclang_llvm_lib_search_paths
                                  # LLVM Fedora
                                  /usr/lib/llvm
                                )

foreach (version ${LIBCLANG_KNOWN_LLVM_VERSIONS})
  string(REPLACE "." "" undotted_version "${version}")
  list(APPEND libclang_llvm_header_search_paths
    # LLVM Debian/Ubuntu nightly packages: http://apt.llvm.org/
    "/usr/lib/llvm-${version}/include/"
    # LLVM MacPorts
    "/opt/local/libexec/llvm-${version}/include"
    # LLVM Homebrew
    "/usr/local/Cellar/llvm/${version}/include"
    # LLVM Homebrew/versions
    "/usr/local/lib/llvm-${version}/include"
    # FreeBSD ports versions
    "/usr/local/llvm${undotted_version}/include"
    )

  list(APPEND libclang_llvm_lib_search_paths
    # LLVM Debian/Ubuntu nightly packages: http://apt.llvm.org/
    "/usr/lib/llvm-${version}/lib/"
    # LLVM MacPorts
    "/opt/local/libexec/llvm-${version}/lib"
    # LLVM Homebrew
    "/usr/local/Cellar/llvm/${version}/lib"
    # LLVM Homebrew/versions
    "/usr/local/lib/llvm-${version}/lib"
    # FreeBSD ports versions
    "/usr/local/llvm${undotted_version}/lib"
    )
endforeach()

find_path (LIBCLANG_INCLUDES clang-c/Index.h
  PATHS ${libclang_llvm_header_search_paths}
  PATH_SUFFIXES LLVM/include #Windows package from http://llvm.org/releases/
  DOC "The path to the directory that contains clang-c/Index.h")

# On Windows with MSVC, the import library uses the ".imp" file extension
# instead of the comon ".lib"
if (MSVC)
  find_file (LIBCLANG_LIBRARY libclang.imp
    PATH_SUFFIXES LLVM/lib
    DOC "The file that corresponds to the libclang library.")
endif(MSVC)

find_library (LIBCLANG_LIBRARY NAMES libclang.imp libclang clang
  PATHS ${libclang_llvm_lib_search_paths}
  PATH_SUFFIXES LLVM/lib #Windows package from http://llvm.org/releases/
  DOC "The file that corresponds to the libclang library.")

get_filename_component (LIBCLANG_LIBRARY_DIR ${LIBCLANG_LIBRARY} PATH)

MACRO(FIND_AND_ADD_CLANG_LIB _libname_)
find_library(CLANG_${_libname_}_LIB
  NAMES ${_libname_}
  PATHS ${libclang_llvm_lib_search_paths} ${LLVM_LIBRARY_DIR})
if (CLANG_${_libname_}_LIB)
   set (LIBCLANG_LIBRARIES ${LIBCLANG_LIBRARIES} ${CLANG_${_libname_}_LIB})
   get_filename_component (LIBCLANG_${_libname_}_LIBRARY_DIR ${LIBCLANG_LIBRARY} PATH)
   set (LIBCLANG_LIBRARY_DIR ${LIBCLANG_LIBRARY_DIR} ${LIBCLANG_${_libname_}_LIBRARY_DIR} )
endif (CLANG_${_libname_}_LIB)
ENDMACRO(FIND_AND_ADD_CLANG_LIB)

set (LIBCLANG_LIBRARIES ${LIBCLANG_LIBRARY})
# Clang shared library provides just the limited C interface, so it
# can not be used.  We look for the static libraries.
FIND_AND_ADD_CLANG_LIB(clangFrontend)
FIND_AND_ADD_CLANG_LIB(clangSerialization)
FIND_AND_ADD_CLANG_LIB(clangDriver)
FIND_AND_ADD_CLANG_LIB(clangCodeGen)
FIND_AND_ADD_CLANG_LIB(clangParse)
FIND_AND_ADD_CLANG_LIB(clangSema)
FIND_AND_ADD_CLANG_LIB(clangAnalysis)
FIND_AND_ADD_CLANG_LIB(clangAST)
FIND_AND_ADD_CLANG_LIB(clangEdit)
FIND_AND_ADD_CLANG_LIB(clangLex)
FIND_AND_ADD_CLANG_LIB(clangBasic)

list (REMOVE_DUPLICATES LIBCLANG_LIBRARY_DIR)

## -----------------------------------------------------------------------------
## Actions taken when all components have been found

if (LIBCLANG_INCLUDES AND LIBCLANG_LIBRARY)
  set (HAVE_LIBCLANG TRUE)
else (LIBCLANG_INCLUDES AND LIBCLANG_LIBRARY)
  if (NOT LIBCLANG_FIND_QUIETLY)
    if (NOT LIBCLANG_INCLUDES)
      message (STATUS "Unable to find libClang header files!")
    endif (NOT LIBCLANG_INCLUDES)
    if (NOT LIBCLANG_LIBRARY)
      message (STATUS "Unable to find libClang library files!")
    endif (NOT LIBCLANG_LIBRARY)
  endif (NOT LIBCLANG_FIND_QUIETLY)
endif (LIBCLANG_INCLUDES AND LIBCLANG_LIBRARY)

if (HAVE_LIBCLANG)
  if (NOT LIBCLANG_FIND_QUIETLY)
    message (STATUS "Found components for libClang")
    message (STATUS "LIBCLANG_INCLUDES .. = ${LIBCLANG_INCLUDES}")
    message (STATUS "LIBCLANG_LIBRARIES . = ${LIBCLANG_LIBRARIES}")
  endif (NOT LIBCLANG_FIND_QUIETLY)
else (HAVE_LIBCLANG)
  if (LIBCLANG_FIND_REQUIRED)
    message (WARNING "Could not find libClang!")
  endif (LIBCLANG_FIND_REQUIRED)
endif (HAVE_LIBCLANG)


mark_as_advanced (
    HAVE_LIBCLANG
    LIBCLANG_INCLUDES
    LIBCLANG_LIBRARIES
    LIBCLANG_LIBRARY_DIR
  )
