# - Find NanoVDB library
# Find the native NanoVDB includes and library
# This module defines
#  NANOVDB_INCLUDE_DIRS, where to find nanovdb.h, Set when
#                         NANOVDB_INCLUDE_DIRE is found.
#  NANOVDB_ROOT_DIR, The base directory to search for NanoVDB.
#                     This can also be an environment variable.
#  NANOVDB_FOUND, If false, do not try to use NanoVDB.

#=============================================================================
# Copyright 2020 Blender Foundation.
#
# Distributed under the OSI-approved BSD 3-Clause License,
# see accompanying file BSD-3-Clause-license.txt for details.
#=============================================================================

# If NANOVDB_ROOT_DIR was defined in the environment, use it.
IF(NOT NANOVDB_ROOT_DIR AND NOT $ENV{NANOVDB_ROOT_DIR} STREQUAL "")
  SET(NANOVDB_ROOT_DIR $ENV{NANOVDB_ROOT_DIR})
ENDIF()

set(_def_search_paths
  ~/Library/Frameworks
  /Library/Frameworks
  /sw # Fink
  /opt/csw # Blastwave
  /opt
  /usr
  "C:/Program Files (x86)/NanoVDB"
  "${NANOVDB_DIR}"
  "${NANOVDB_ROOT_DIR}"
)


FIND_PATH(NANOVDB_INCLUDE_DIR
  NAMES
    nanovdb/NanoVDB.h
  HINTS
    ${_def_search_paths}
  PATH_SUFFIXES
    include
)


# handle the QUIETLY and REQUIRED arguments and set NANOVDB_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NanoVDB DEFAULT_MSG
NANOVDB_INCLUDE_DIR)



IF(NANOVDB_FOUND)
  SET(NANOVDB_INCLUDE_DIRS ${NANOVDB_INCLUDE_DIR})
  message(STATUS "Found NanoVDB: ${NANOVDB_INCLUDE_DIR}")
  include_directories(
    SYSTEM
    ${NANOVDB_INCLUDE_DIR}
  )
ENDIF(NANOVDB_FOUND)

MARK_AS_ADVANCED(
  NANOVDB_INCLUDE_DIR
)

UNSET(_nanovdb_SEARCH_DIRS)