# Find the native LLVM includes and library
#
#  HAVE_LLVM        - system has LLVM installed
#  LLVM_INCLUDE_DIR - where to find llvm include files
#  LLVM_LIBRARY_DIR - where to find llvm libs
#  LLVM_CFLAGS      - llvm compiler flags
#  LLVM_LFLAGS      - llvm linker flags
#  LLVM_MODULE_LIBS - list of llvm libs for working with modules.
#  LLVM_FOUND       - True if llvm found.
#  LLVM_VERSION     - Installed version.

set (LIBCLANG_KNOWN_LLVM_VERSIONS 4.0.0 4.0
  3.9.1 3.9.0 3.9
  3.8.1 3.8.0 3.8
  3.7.1 3.7.0 3.7
  3.6.2 3.6.1 3.6.0 3.6
  3.5.2 3.5.1 3.5.0 3.5
  3.4.2 3.4.1 3.4
  3.3
  3.2
  3.1)

set (llvm_config_name)
set (llvm_header_search_paths)
set (llvm_lib_search_paths
                            # LLVM Fedora
                            /usr/lib/llvm
                          )

foreach (version ${LIBCLANG_KNOWN_LLVM_VERSIONS})
  string(REPLACE "." "" undotted_version "${version}")
  list(APPEND llvm_header_search_paths
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

  list(APPEND llvm_config_name
                              llvm-config-${version}
                              llvm-config${version}
                              llvm-config-${undotted_version}
                              llvm-config${undotted_version}
                            )

  list(APPEND llvm_lib_search_paths
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


find_program(LLVM_CONFIG_EXECUTABLE NAMES ${llvm_config_name} DOC "llvm-config executable")

if (LLVM_CONFIG_EXECUTABLE)
  message (STATUS "Found components for LLVM")
  message (STATUS "llvm-config ........ = ${LLVM_CONFIG_EXECUTABLE}")
  set (HAVE_LLVM true)
else (LLVM_CONFIG_EXECUTABLE)
  message (STATUS "Unable to find llvm-config executable")
  message (WARNING "Could NOT find LLVM config executable")
endif (LLVM_CONFIG_EXECUTABLE)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
  OUTPUT_VARIABLE LLVM_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
  OUTPUT_VARIABLE LLVM_LIBRARY_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cppflags
  OUTPUT_VARIABLE LLVM_CFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --ldflags
  OUTPUT_VARIABLE LLVM_LFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs
  OUTPUT_VARIABLE LLVM_MODULE_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
  OUTPUT_VARIABLE LLVM_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT LLVM_FIND_QUIETLY)
  if (LLVM_VERSION)
    message (STATUS "LLVM_VERSION ....... = ${LLVM_VERSION}")
  else(LLVM_VERSION)
    message (STATUS "LLVM_VERSION ....... = NOT FOUND")
  endif (LLVM_VERSION)
  if (LLVM_INCLUDE_DIR)
    message (STATUS "LLVM_INCLUDE_DIR ... = ${LLVM_INCLUDE_DIR}")
  else(LLVM_INCLUDE_DIR)
    message (STATUS "LLVM_INCLUDE_DIR ... = NOT FOUND")
  endif (LLVM_INCLUDE_DIR)
  if (LLVM_LIBRARY_DIR)
    message (STATUS "LLVM_LIBRARY_DIR ... = ${LLVM_LIBRARY_DIR}")
  else(LLVM_LIBRARY_DIR)
    message (STATUS "LLVM_LIBRARY_DIR ... = NOT FOUND")
  endif (LLVM_LIBRARY_DIR)
  if (LLVM_FIND_VERBOSE)
    if (LLVM_CFLAGS)
      message (STATUS "LLVM_CFLAGS ........ = ${LLVM_CFLAGS}")
    else(LLVM_CFLAGS)
      message (STATUS "LLVM_CFLAGS ........ = NOT AVAILABLE")
    endif (LLVM_CFLAGS)
    if (LLVM_LFLAGS)
      message (STATUS "LLVM_LFLAGS ........ = ${LLVM_LFLAGS}")
    else(LLVM_LFLAGS)
      message (STATUS "LLVM_LFLAGS ........ = NOT AVAILABLE")
    endif (LLVM_LFLAGS)
    if (LLVM_MODULE_LIBS)
      message (STATUS "LLVM_MODULE_LIBS ... = ${LLVM_MODULE_LIBS}")
    else(LLVM_MODULE_LIBS)
      message (STATUS "LLVM_MODULE_LIBS ... = NOT FOUND")
    endif (LLVM_MODULE_LIBS)
  endif (LLVM_FIND_VERBOSE)
endif (NOT LLVM_FIND_QUIETLY)

mark_as_advanced(
  HAVE_LLVM
  LLVM_INCLUDE_DIR
  LLVM_LIBRARY_DIR
  LLVM_CFLAGS
  LLVM_LFLAGS
  LLVM_MODULE_LIBS
  LLVM_VERSION
)
