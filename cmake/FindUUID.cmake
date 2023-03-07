# - Find UUID
# Find the native UUID includes and library
# This module defines
#  UUID_INCLUDE_DIR, where to find jpeglib.h, etc.
#  UUID_LIBRARIES, the libraries needed to use UUID.
#  UUID_FOUND, If false, do not try to use UUID.
# also defined, but not for general use are
#  UUID_LIBRARY, where to find the UUID library.
#
#  Copyright (c) 2006-2009 Mathieu Malaterre <mathieu.malaterre@gmail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

# On MacOSX we have:
# $ nm -g /usr/lib/libSystem.dylib | grep uuid_generate
# 000b3aeb T _uuid_generate
# 0003e67e T _uuid_generate_random
# 000b37a1 T _uuid_generate_time
if(APPLE)
  set(UUID_LIBRARY_VAR System)
else(APPLE)
  # Linux type:
  set(UUID_LIBRARY_VAR uuid)
endif(APPLE)

set(UUID_ROOT $ENV{UUID_ROOT})

find_library(
  UUID_LIBRARY
  NAMES ${UUID_LIBRARY_VAR}
  PATHS ${UUID_ROOT}/lib
  NO_DEFAULT_PATH)

if(NOT UUID_LIBRARY)
  find_library(
    UUID_LIBRARY
    NAMES ${UUID_LIBRARY_VAR}
    PATHS /lib /usr/lib /usr/local/lib)
endif(NOT UUID_LIBRARY)

# Must be *after* the lib itself
set(CMAKE_FIND_FRAMEWORK_SAVE ${CMAKE_FIND_FRAMEWORK})
set(CMAKE_FIND_FRAMEWORK NEVER)

find_path(
  UUID_INCLUDE_DIR uuid/uuid.h
  PATHS ${UUID_ROOT}/include
  NO_DEFAULT_PATH)

if(NOT UUID_INCLUDE_DIR)
  find_path(UUID_INCLUDE_DIR uuid/uuid.h /usr/local/include /usr/include)
endif(NOT UUID_INCLUDE_DIR)

if(UUID_LIBRARY AND UUID_INCLUDE_DIR)
  set(UUID_LIBRARIES ${UUID_LIBRARY})
  set(UUID_FOUND "YES")
else(UUID_LIBRARY AND UUID_INCLUDE_DIR)
  set(UUID_FOUND "NO")
endif(UUID_LIBRARY AND UUID_INCLUDE_DIR)

if(UUID_FOUND)
  if(UUID_FIND_VERBOSE)
    message(STATUS "Found UUID ......... = ${UUID_LIBRARY}")
  endif(UUID_FIND_VERBOSE)
else(UUID_FOUND)
  if(UUID_FIND_REQUIRED)
    message("library: ${UUID_LIBRARY}")
    message("include: ${UUID_INCLUDE_DIR}")
    message(FATAL_ERROR "Could not find UUID library")
  endif(UUID_FIND_REQUIRED)
endif(UUID_FOUND)

# Deprecated declarations.
#SET (NATIVE_UUID_INCLUDE_PATH ${UUID_INCLUDE_DIR} )
#GET_FILENAME_COMPONENT (NATIVE_UUID_LIB_PATH ${UUID_LIBRARY} PATH)

mark_as_advanced(UUID_LIBRARY UUID_INCLUDE_DIR)
set(CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK_SAVE})
