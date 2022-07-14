# Find the native LLVM includes and library
#
#  HAVE_LLVM        - system has LLVM installed
#  LLVM_INCLUDE_DIR - where to find llvm include files
#  LLVM_LIBRARY_DIR - where to find llvm libs
#  LLVM_CFLAGS      - llvm compiler flags
#  LLVM_LFLAGS      - llvm linker flags
#  LLVM_MODULE_LIBS - list of llvm libs for working with modules.
#  LLVM_VERSION     - Installed version.

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

set (llvm_config_name)
set (llvm_header_search_paths)
set (llvm_lib_search_paths
                            # LLVM Fedora
                            /usr/lib/llvm
                          )

foreach (version ${LIBCLANG_KNOWN_LLVM_VERSIONS})
  string(REPLACE "." "" undotted_version "${version}")
  string(REPLACE ".0.0" "" stripped_version "${version}")
  list(APPEND llvm_header_search_paths
    # git installation
    "/usr/local/include/llvm"
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
    )

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

  list(APPEND llvm_lib_search_paths
    # git installation
    "/usr/local/lib"
    # LLVM Debian/Ubuntu nightly packages: http://apt.llvm.org/
    "/usr/lib/llvm-${version}/lib/"
    "/usr/lib/llvm-${stripped_version}/lib/"
    "/usr/local/llvm-${undotted_version}/lib"
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
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --cxxflags
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
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --libfiles
  OUTPUT_VARIABLE LLVM_MODULE_LIBFILES
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(LLVM_MODULE_LIBFILES)

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --version
  OUTPUT_VARIABLE LLVM_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (LLVM_VERSION MATCHES "^([0-9]+)(\\.[0-9]+)*$")
  set (LLVM_MAJOR_VERSION ${CMAKE_MATCH_1})
else()
  message (WARNING "LLVM_MAJOR_VERSION NOT DETECTED!")
  set (LLVM_MAJOR_VERSION LLVM_VERSION)
endif()

execute_process(
  COMMAND ${LLVM_CONFIG_EXECUTABLE} --shared-mode
  OUTPUT_VARIABLE LLVM_LINKING_MODE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)


set(LLVM_SYSTEM  z pthread tinfo)

if (LLVM_FIND_VERBOSE)
  if (LLVM_VERSION)
    message (STATUS "LLVM_VERSION ....... = ${LLVM_VERSION}")
    message (STATUS "LLVM_MAJOR_VERSION . = ${LLVM_MAJOR_VERSION}")
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
    message(STATUS "LLVM_LINKING_MODE ...... = ${LLVM_LINKING_MODE}")
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
    if (LLVM_SYSTEM)
      message (STATUS "LLVM_SYSTEM_LIBS ... = ${LLVM_SYSTEM}")
    else(LLVM_SYSTEM)
      message (STATUS "LLVM_SYSTEM_LIBS ... = NOT FOUND")
    endif (LLVM_SYSTEM)
  endif (LLVM_FIND_VERBOSE)
endif (LLVM_FIND_VERBOSE)

# Required to adjust imports based on the llvm major version
add_definitions(-DLLVM_MAJOR_VERSION=${LLVM_MAJOR_VERSION})
mark_as_advanced(
  HAVE_LLVM
  LLVM_INCLUDE_DIR
  LLVM_LIBRARY_DIR
  LLVM_CFLAGS
  LLVM_LFLAGS
  LLVM_MODULE_LIBS
  LLVM_SYSTEM
  LLVM_VERSION
  LLVM_LINKING_MODE
)
