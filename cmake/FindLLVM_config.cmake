# Copyright 2022 Politecnico di Milano.
# Developed by : Stefano Cherubin
#                PhD student, Politecnico di Milano
#                <first_name>.<family_name>@polimi.it
#                Moreno Giussani
#                Ms student, Politecnico di Milano
#                <first_name>.<family_name>@mail.polimi.it
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
#
#                       FindLLVM_config.cmake
#
# Find the native LLVM includes and library and get the configuration parameters
#
#  LLVM_FOUND               - system has LLVM installed
#  LLVM_PACKAGE_VERSION     - LLVM version
#  LLVM_VERSION_MAJOR       - LLVM major release number
#  LLVM_INCLUDE_DIR         - where to find llvm include files
#  LLVM_LIBRARY_DIR         - where to find llvm libs
#  LLVM_CFLAGS              - llvm compiler flags
#  LLVM_LFLAGS              - llvm linker flags
#  LLVM_MODULE_LIBS         - list of llvm libs for working with modules.
#  LLVM_PACKAGE_VERSION     - Installed version.
#  LLVM_TOOLS_BINARY_DIR    - Where to find llvm-* programs

if(NOT LLVM_FOUND)
  set(LLVM_KNOWN_MAJOR_VERSIONS
      20.1
      20
      19.1
      19
      18
      17
      16
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
    if(NOT LLVM_CONFIG_EXECUTABLE AND NOT LLVM_TOOLS_BINARY_DIR)
      find_package(LLVM ${ver} QUIET)
    endif(NOT LLVM_CONFIG_EXECUTABLE AND NOT LLVM_TOOLS_BINARY_DIR)
    if(LLVM_FOUND)
      # Call the main findLLVM.cmake to check LLVM setup and set some flags such
      # as: LLVM_LIBRARY_DIR, LLVM_INCLUDE_DIR, LLVM_TOOLS_BINARY_DIR,
      # LLVM_VERSION_MAJOR, LLVM_PACKAGE_VERSION
      if(LLVM_FIND_VERBOSE)
        message(STATUS "Package LLVM found with default cmake finder for version ${ver}")
        message(STATUS "Moving on to look for llvm-config executable")
        find_package(LLVM ${ver} CONFIG)
      else(LLVM_FIND_VERBOSE)
        find_package(LLVM ${ver} CONFIG QUIET)
      endif(LLVM_FIND_VERBOSE)
      break()
    else(LLVM_FOUND)
      set(LLVM_PATH_CANDIDATES "${LLVM_ROOT}")
      list(APPEND LLVM_PATH_CANDIDATES "${LLVM_TOOLS_BINARY_DIR}") # Manually specified by user
      list(APPEND LLVM_PATH_CANDIDATES "/usr/bin/") # Ubuntu
      list(APPEND LLVM_PATH_CANDIDATES "/opt/homebrew/opt/llvm/") # Manjaro
      list(APPEND LLVM_PATH_CANDIDATES "/usr/local/opt/llvm@${ver}/bin/") # Homebrew Cellar symlink
      list(APPEND LLVM_PATH_CANDIDATES "/usr/local/Cellar/llvm@${ver}/") # Homebrew Cellar

      if(LLVM_FIND_VERBOSE)
        message(STATUS "LLVM not found by cmake default finder for version ${ver}.")
        message(STATUS "Moving on to deep search in known install paths for version ${ver}.")
      endif(LLVM_FIND_VERBOSE)
      if(NOT LLVM_CONFIG_EXECUTABLE)
         find_program(
           LLVM_CONFIG_EXECUTABLE
           NAMES "llvm-config-${ver}"
           DOC "llvm-config executable with version filename"
           PATHS ${LLVM_PATH_CANDIDATES}
           NO_DEFAULT_PATH)
      else(NOT LLVM_CONFIG_EXECUTABLE)
        message(STATUS "Using manually provided LLVM_CONFIG_EXECUTABLE: ${LLVM_CONFIG_EXECUTABLE}.")
        set(LLVM_FOUND TRUE)
        break()
      endif(NOT LLVM_CONFIG_EXECUTABLE)
    endif(LLVM_FOUND)
    if(NOT LLVM_CONFIG_EXECUTABLE)
      if(LLVM_FIND_VERBOSE)
        message(STATUS "LLVM config exe not found for version ${ver}")
      endif(LLVM_FIND_VERBOSE)
    else(NOT LLVM_CONFIG_EXECUTABLE)
      set(LLVM_VERSION_MAJOR ${ver})
      message(STATUS "Found LLVM version ${ver}")
      set(LLVM_FOUND TRUE)
      break()
    endif(NOT LLVM_CONFIG_EXECUTABLE)
  endforeach(ver ${LLVM_KNOWN_VERSIONS})
endif(NOT LLVM_FOUND)

if(NOT LLVM_CONFIG_EXECUTABLE)
  set(LLVM_PATH_CANDIDATES "${LLVM_ROOT}")
  list(APPEND LLVM_PATH_CANDIDATES "${LLVM_TOOLS_BINARY_DIR}") # Manually specified by user
  list(APPEND LLVM_PATH_CANDIDATES "/usr/bin/") # Ubuntu
  list(APPEND LLVM_PATH_CANDIDATES "/opt/homebrew/opt/llvm/") # Manjaro
  list(APPEND LLVM_PATH_CANDIDATES "/usr/local/opt/llvm/bin/") # Homebrew Cellar symlink
  list(APPEND LLVM_PATH_CANDIDATES "/usr/local/Cellar/llvm/") # Homebrew Cellar
  
  find_program(
    LLVM_CONFIG_EXECUTABLE
    NAMES "llvm-config"
    DOC "llvm-config executable, including versionless filename"
    PATHS ${LLVM_PATH_CANDIDATES}
    NO_DEFAULT_PATH)

    if(LLVM_CONFIG_EXECUTABLE)
      message(STATUS "Found LLVM version")
      set(LLVM_FOUND TRUE)
    endif(LLVM_CONFIG_EXECUTABLE)
endif(NOT LLVM_CONFIG_EXECUTABLE)

if(LLVM_CONFIG_EXECUTABLE)
  message(STATUS "llvm-config ........ = ${LLVM_CONFIG_EXECUTABLE}")
else(LLVM_CONFIG_EXECUTABLE)
  message(STATUS "llvm-config ........ = NOT FOUND")
  message(WARNING "Could NOT find LLVM config executable")
endif(LLVM_CONFIG_EXECUTABLE)

if(NOT LLVM_SHARED_MODE)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --shared-mode
    OUTPUT_VARIABLE LLVM_SHARED_MODE
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(NOT LLVM_SHARED_MODE)

if(LLVM_SHARED_MODE STREQUAL "shared")
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --link-shared
    OUTPUT_VARIABLE LLVM_LINK_SHARED_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(LLVM_LINK_SHARED_OUTPUT STREQUAL "")
    message(STATUS "Setting link mode to shared...= OK")
  else(LLVM_LINK_SHARED_OUTPUT STREQUAL "")
    message(
      WARNING "Setting link mode to shared... = ${LLVM_LINK_SHARED_OUTPUT}")
  endif(LLVM_LINK_SHARED_OUTPUT STREQUAL "")
endif(LLVM_SHARED_MODE STREQUAL "shared")

if(LLVM_SHARED_MODE STREQUAL "static")
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --link-static
    OUTPUT_VARIABLE LLVM_LINK_STATIC_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(LLVM_LINK_STATIC_OUTPUT STREQUAL "")
    message(STATUS "Setting link mode to static...= OK")
  else(LLVM_LINK_STATIC_OUTPUT STREQUAL "")
    message(
      WARNING "Setting link mode to static... = ${LLVM_LINK_STATIC_OUTPUT}")
  endif(LLVM_LINK_STATIC_OUTPUT STREQUAL "")
endif(LLVM_SHARED_MODE STREQUAL "static")

if(NOT LLVM_CFLAGS)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
    OUTPUT_VARIABLE LLVM_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(NOT LLVM_CFLAGS)

if(NOT LLVM_LFLAGS)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
    OUTPUT_VARIABLE LLVM_LFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(NOT LLVM_LFLAGS)

if(NOT LLVM_MODULE_LIBS)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs
    OUTPUT_VARIABLE LLVM_MODULE_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(NOT LLVM_MODULE_LIBS)

if(NOT LLVM_MODULE_LIBFILES)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libfiles
    OUTPUT_VARIABLE LLVM_MODULE_LIBFILES
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(LLVM_MODULE_LIBFILES)
endif(NOT LLVM_MODULE_LIBFILES)

if(NOT LLVM_SYSTEM)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(LLVM_SYSTEM)
endif(NOT LLVM_SYSTEM)
# This should never happen
if(NOT LLVM_TOOLS_BINARY_DIR)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --bindir
    OUTPUT_VARIABLE LLVM_TOOLS_BINARY_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(NOT LLVM_TOOLS_BINARY_DIR)

if(NOT LLVM_INCLUDE_DIR)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
    OUTPUT_VARIABLE LLVM_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(LLVM_INCLUDE_DIR)
endif(NOT LLVM_INCLUDE_DIR)

if(NOT LLVM_LIBRARY_DIR)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
    OUTPUT_VARIABLE LLVM_LIBRARY_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(LLVM_LIBRARY_DIR)
endif(NOT LLVM_LIBRARY_DIR)

if(NOT LLVM_PACKAGE_VERSION)
  execute_process(
    COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
    OUTPUT_VARIABLE LLVM_PACKAGE_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(NOT LLVM_PACKAGE_VERSION)

# pick only major
if((LLVM_FOUND) AND (NOT LLVM_VERSION_MAJOR) AND (NOT LLVM_VERSION_MAJOR-NOTFOUND))
  string(REGEX REPLACE "^([0-9]+)\..*" "\\1" LLVM_VERSION_MAJOR ${LLVM_PACKAGE_VERSION})
endif((LLVM_FOUND) AND (NOT LLVM_VERSION_MAJOR) AND (NOT LLVM_VERSION_MAJOR-NOTFOUND))

if(NOT CLANG_EXE_NAME)
  find_program(
    CLANG_EXE_FULLPATH
    NAMES "clang-${LLVM_VERSION_MAJOR}" "clang"
    DOC "clang executable"
    PATHS ${LLVM_TOOLS_BINARY_DIR}
    NO_DEFAULT_PATH)
  get_filename_component(CLANG_EXE_NAME ${CLANG_EXE_FULLPATH} NAME)
endif(NOT CLANG_EXE_NAME)
if(NOT OPT_EXE_NAME)
  find_program(
    OPT_EXE_FULLPATH
    NAMES "opt-${LLVM_VERSION_MAJOR}" "opt"
    DOC "opt executable"
    PATHS ${LLVM_TOOLS_BINARY_DIR} /usr/bin
    NO_DEFAULT_PATH)
  get_filename_component(OPT_EXE_NAME ${OPT_EXE_FULLPATH} NAME)
endif(NOT OPT_EXE_NAME)

if(LLVM_FIND_VERBOSE)
  if(LLVM_PACKAGE_VERSION)
    message(STATUS "LLVM_PACKAGE_VERSION = ${LLVM_PACKAGE_VERSION}")
    message(STATUS "LLVM_VERSION_MAJOR . = ${LLVM_VERSION_MAJOR}")
  else(LLVM_PACKAGE_VERSION)
    message(STATUS "LLVM_PACKAGE_VERSION = NOT FOUND")
    message(STATUS "LLVM_VERSION_MAJOR . = NOT FOUND")
  endif(LLVM_PACKAGE_VERSION)
  if(LLVM_INCLUDE_DIR)
    message(STATUS "LLVM_INCLUDE_DIR ... = ${LLVM_INCLUDE_DIR}")
  else(LLVM_INCLUDE_DIR)
    message(STATUS "LLVM_INCLUDE_DIR ... = NOT FOUND")
  endif(LLVM_INCLUDE_DIR)
  if(LLVM_LIBRARY_DIR)
    message(STATUS "LLVM_LIBRARY_DIR ... = ${LLVM_LIBRARY_DIR}")
  else(LLVM_LIBRARY_DIR)
    message(STATUS "LLVM_LIBRARY_DIR ... = NOT FOUND")
  endif(LLVM_LIBRARY_DIR)
  if(LLVM_FIND_VERBOSE)
    if(LLVM_CFLAGS)
      message(STATUS "LLVM_CFLAGS ........ = ${LLVM_CFLAGS}")
    else(LLVM_CFLAGS)
      message(STATUS "LLVM_CFLAGS ........ = NOT AVAILABLE")
    endif(LLVM_CFLAGS)
    if(LLVM_LFLAGS)
      message(STATUS "LLVM_LFLAGS ........ = ${LLVM_LFLAGS}")
    else(LLVM_LFLAGS)
      message(STATUS "LLVM_LFLAGS ........ = NOT AVAILABLE")
    endif(LLVM_LFLAGS)
    if(LLVM_MODULE_LIBS)
      message(STATUS "LLVM_MODULE_LIBS ... = ${LLVM_MODULE_LIBS}")
    else(LLVM_MODULE_LIBS)
      message(STATUS "LLVM_MODULE_LIBS ... = NOT FOUND")
    endif(LLVM_MODULE_LIBS)
    if(LLVM_SYSTEM)
      message(STATUS "LLVM_SYSTEM_LIBS ... = ${LLVM_SYSTEM}")
    else(LLVM_SYSTEM)
      message(STATUS "LLVM_SYSTEM_LIBS ... = NOT FOUND")
    endif(LLVM_SYSTEM)
    if(LLVM_TOOLS_BINARY_DIR)
      message(STATUS "LLVM_TOOLS_BINARY_DIR= ${LLVM_TOOLS_BINARY_DIR}")
    else(LLVM_TOOLS_BINARY_DIR)
      message(STATUS "LLVM_TOOLS_BINARY_DIR= NOT FOUND")
    endif(LLVM_TOOLS_BINARY_DIR)
    if(LLVM_SHARED_MODE)
      message(STATUS "LLVM_SHARED_MODE.... = ${LLVM_SHARED_MODE}")
    else(LLVM_SHARED_MODE)
      message(STATUS "LLVM_SHARED_MODE.... = UNKNOWN!")
    endif(LLVM_SHARED_MODE)
  endif(LLVM_FIND_VERBOSE)
endif(LLVM_FIND_VERBOSE)

if(LLVM_FOUND)
  add_compile_definitions(LLVM_FOUND=1)
endif(LLVM_FOUND)
# Required to adjust imports based on the llvm major version
if(LLVM_VERSION_MAJOR)
  add_compile_definitions(LLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR})
  list(REMOVE_DUPLICATES COMPILE_DEFINITIONS)
endif(LLVM_VERSION_MAJOR)
# Required to adjust paths without having runtime penalties for string
# composition
if(NOT CLANG_EXE_FULLPATH-NOTFOUND)
  add_compile_definitions(CLANG_EXE_FULLPATH="${CLANG_EXE_FULLPATH}")
endif(NOT CLANG_EXE_FULLPATH-NOTFOUND)
if(NOT OPT_EXE_FULLPATH-NOTFOUND)
  if(LLVM_FIND_VERBOSE)
    message(STATUS "OPT_EXE_FULLPATH ..... = ${OPT_EXE_FULLPATH}")
  endif(LLVM_FIND_VERBOSE)
  add_compile_definitions(OPT_EXE_FULLPATH="${OPT_EXE_FULLPATH}")
endif(NOT OPT_EXE_FULLPATH-NOTFOUND)
if(NOT CLANG_EXE_NAME-NOTFOUND)
  add_compile_definitions(CLANG_EXE_NAME="${CLANG_EXE_NAME}")
endif(NOT CLANG_EXE_NAME-NOTFOUND)
if(NOT OPT_EXE_NAME-NOTFOUND)
  if(LLVM_FIND_VERBOSE)
    message(STATUS "OPT ..... = ${OPT_EXE_NAME}")
  endif(LLVM_FIND_VERBOSE)
  add_compile_definitions(OPT_EXE_NAME="${OPT_EXE_NAME}")
else(NOT OPT_EXE_NAME-NOTFOUND)
  if(LLVM_FIND_VERBOSE)
    message(STATUS "OPT_EXE not found")
  endif(LLVM_FIND_VERBOSE)
endif(NOT OPT_EXE_NAME-NOTFOUND)
if(NOT LLVM_INCLUDE_DIR-NOTFOUND)
  if(LLVM_FIND_VERBOSE)
    message(STATUS "LLVM_INCLUDE_DIR ..... = ${LLVM_INCLUDE_DIR}")
  endif(LLVM_FIND_VERBOSE)
else(NOT LLVM_INCLUDE_DIR-NOTFOUND)
  if(LLVM_FIND_VERBOSE)
    message(STATUS "LLVM_INCLUDE_DIR not found")
  endif(LLVM_FIND_VERBOSE)
endif(NOT LLVM_INCLUDE_DIR-NOTFOUND)

mark_as_advanced(
  LLVM_FOUND
  LLVM_INCLUDE_DIR
  LLVM_LIBRARY_DIR
  LLVM_CFLAGS
  LLVM_LFLAGS
  LLVM_MODULE_LIBS
  LLVM_SYSTEM
  LLVM_VERSION
  LLVM_TOOLS_BINARY_DIR
  LLVM_SHARED_MODE)
