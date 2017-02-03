# - Check for the presence of DL
#
# The following variables are set when DL is found:
#  HAVE_DL       = Set to true, if all components of DL
#                          have been found.
#  DL_INCLUDES   = Include path for the header files of DL
#  DL_LIBRARY    = Link these to use DL

## -----------------------------------------------------------------------------
SET(DL_ROOT $ENV{DL_ROOT})

## Check for the header files
find_path (DL_INCLUDES dlfcn.h
  PATHS ${DL_ROOT}/include
  NO_DEFAULT_PATH
  )

if (NOT DL_INCLUDES)
  find_path (DL_INCLUDES dlfcn.h
    PATHS /usr/local/include /usr/include ${CMAKE_EXTRA_INCLUDES}
    )
endif (NOT DL_INCLUDES)

## -----------------------------------------------------------------------------
## Check for the library

find_library (DL_LIBRARY dl
  PATHS ${DL_ROOT}/lib
  NO_DEFAULT_PATH
  )

if (NOT DL_LIBRARY)
  find_library (DL_LIBRARY dl
    PATHS /usr/local/lib /usr/lib /lib ${CMAKE_EXTRA_LIBRARIES}
    )
endif (NOT DL_LIBRARY)

## -----------------------------------------------------------------------------
## Actions taken when all components have been found

if (DL_INCLUDES AND DL_LIBRARY)
  set (HAVE_DL TRUE)
else (DL_INCLUDES AND DL_LIBRARY)
  if (NOT DL_FIND_QUIETLY)
    if (NOT DL_INCLUDES)
      message (STATUS "Unable to find DL header files!")
    endif (NOT DL_INCLUDES)
    if (NOT DL_LIBRARY)
      message (STATUS "Unable to find DL library files!")
    endif (NOT DL_LIBRARY)
  endif (NOT DL_FIND_QUIETLY)
endif (DL_INCLUDES AND DL_LIBRARY)

if (HAVE_DL)
  if (NOT DL_FIND_QUIETLY)
    message (STATUS "Found components for DL")
    message (STATUS "DL_INCLUDES ........ = ${DL_INCLUDES}")
    message (STATUS "DL_LIBRARY ......... = ${DL_LIBRARY}")
  endif (NOT DL_FIND_QUIETLY)
else (HAVE_DL)
  if (DL_FIND_REQUIRED)
    message (FATAL_ERROR "Could not find DL!")
  endif (DL_FIND_REQUIRED)
endif (HAVE_DL)

mark_as_advanced (
  HAVE_DL
  DL_LIBRARY
  DL_INCLUDES
  )
