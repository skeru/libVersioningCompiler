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
set (LIBCLANG_KNOWN_LLVM_VERSIONS 15.0.0
        14.0.6
        14.0.5
        14.0.4
        14.0.3
        14.0.2
        14.0.1
        14.0.0
        13.0.1
        13.0.0
)

set (libclang_llvm_header_search_paths
)
set (libclang_llvm_lib_search_paths
                                  # LLVM Fedora
                                  /usr/lib/llvm
                                  /usr/local/lib
                                  # Archlinux
                                  /usr/lib/
                                  /usr/lib/clang/
                                )

foreach (version ${LIBCLANG_KNOWN_LLVM_VERSIONS})
  string(REPLACE "." "" undotted_version "${version}")
  string(REPLACE ".0.0" "" stripped_version "${version}")
  list(APPEND libclang_llvm_header_search_paths
    # git installation
    "/usr/local/include/clang"
    # LLVM Debian/Ubuntu nightly packages: http://apt.llvm.org/
    "/usr/lib/llvm-${version}/include/"
    "/usr/lib/llvm-${stripped_version}/include/"
    # LLVM MacPorts
    "/opt/local/libexec/llvm-${version}/include"
    # LLVM Homebrew
    "/usr/local/Cellar/llvm/${version}/include"
    # LLVM Homebrew/versions
    "/usr/local/lib/llvm-${version}/include"
    # FreeBSD ports versions
    "/usr/local/llvm${undotted_version}/include"
    # Archlinux
    "/usr/lib/clang/${version}/include"
    )

  list(APPEND libclang_llvm_lib_search_paths
    # git installation
    "/usr/local/lib"
    # LLVM Debian/Ubuntu nightly packages: http://apt.llvm.org/
    "/usr/lib/llvm-${version}/lib/"
    "/usr/lib/llvm-${stripped_version}/lib/"
    # LLVM MacPorts
    "/opt/local/libexec/llvm-${version}/lib"
    # LLVM Homebrew
    "/usr/local/Cellar/llvm/${version}/lib"
    # LLVM Homebrew/versions
    "/usr/local/lib/llvm-${version}/lib"
    # FreeBSD ports versions
    "/usr/local/llvm${undotted_version}/lib"
    # Archlinux
    "/usr/lib/"
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

if (NOT LIBCLANG_LIBRARY)
  message(STATUS "LIBCLANG_LIBRARY.... = NOT FOUND")
else()
  message(STATUS "LIBCLANG_LIBRARY.... = ${LIBCLANG_LIBRARY}")
endif()

get_filename_component (LIBCLANG_LIBRARY_DIR ${LIBCLANG_LIBRARY} PATH)

MACRO(FIND_AND_ADD_CLANG_LIB _libname_)
find_library(CLANG_${_libname_}_LIB
  NAMES ${_libname_}
  PATHS ${libclang_llvm_lib_search_paths} ${LLVM_LIBRARY_DIR})
if (CLANG_${_libname_}_LIB)
  set (LIBCLANG_LIBRARIES ${LIBCLANG_LIBRARIES} ${CLANG_${_libname_}_LIB})
  get_filename_component (LIBCLANG_${_libname_}_LIBRARY_DIR ${LIBCLANG_LIBRARY} PATH)
  set (LIBCLANG_LIBRARY_DIR ${LIBCLANG_LIBRARY_DIR} ${LIBCLANG_${_libname_}_LIBRARY_DIR} )
else()
  if (NOT LIBCLANG_FIND_VERBOSE)
	 message(WARNING "Clang dependency: Cannot find library '${_libname_}' in libclang_llvm_lib_search_paths.")
  else()
    message(WARNING "Clang dependency: Cannot find library '${_libname_}' in paths ${libclang_llvm_lib_search_paths}.")
  endif(NOT LIBCLANG_FIND_VERBOSE)
endif(CLANG_${_libname_}_LIB)
ENDMACRO(FIND_AND_ADD_CLANG_LIB)

set (LIBCLANG_LIBRARIES)
## check if is being linked as shared or not
if ( NOT DEFINED ENV{LLVM_CONFIG} )
  list(APPEND llvm_config_name
                              llvm-config-${version}
                              llvm-config${version}
                              llvm-config-${undotted_version}
                              llvm-config${undotted_version}
                              llvm-config-${stripped_version}
                              llvm-config${stripped_version}
                              llvm-config
                              )
else( NOT DEFINED ENV{LLVM_CONFIG} )
  list(APPEND llvm_config_name $ENV{LLVM_CONFIG} )
endif( NOT DEFINED ENV{LLVM_CONFIG} )
find_program(LLVM_CONFIG_EXECUTABLE NAMES ${llvm_config_name} DOC "llvm-config executable")
if (LLVM_CONFIG_EXECUTABLE)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --shared-mode
    OUTPUT_VARIABLE LLVM_LINKING_MODE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()
# If there is a single library (Archlinux), go with it
find_library(is_there_clang-cpp
  NAMES clang-cpp
  PATHS ${libclang_llvm_lib_search_paths} ${LLVM_LIBRARY_DIR})
if (is_there_clang-cpp)
    FIND_AND_ADD_CLANG_LIB(clang-cpp)
else()
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
endif()
## -----------------------------------------------------------------------------
## Actions taken when all components have been found

if (LIBCLANG_INCLUDES AND LIBCLANG_LIBRARY)
  set (HAVE_LIBCLANG TRUE)
else (LIBCLANG_INCLUDES AND LIBCLANG_LIBRARY)
  if (LIBCLANG_FIND_VERBOSE)
    if (NOT LIBCLANG_INCLUDES)
      message (STATUS "Unable to find libClang header files!")
    endif (NOT LIBCLANG_INCLUDES)
    if (NOT LIBCLANG_LIBRARY)
      message (STATUS "Unable to find libClang library files!")
    endif (NOT LIBCLANG_LIBRARY)
  endif (LIBCLANG_FIND_VERBOSE)
endif (LIBCLANG_INCLUDES AND LIBCLANG_LIBRARY)

if (HAVE_LIBCLANG)
  if (LIBCLANG_FIND_VERBOSE)
    message (STATUS "Found components for libClang")
    message (STATUS "LIBCLANG_INCLUDES .. = ${LIBCLANG_INCLUDES}")
    message (STATUS "LIBCLANG_LIBRARIES . = ${LIBCLANG_LIBRARIES}")
  endif (LIBCLANG_FIND_VERBOSE)
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
