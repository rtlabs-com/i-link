#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# www.rt-labs.com
# Copyright 2019 rt-labs AB, Sweden.
# See LICENSE file in the project root for full license information.
#*******************************************************************/

cmake_minimum_required (VERSION 3.14)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tools")
project (IOLINKMASTER VERSION 0.1.0)
include(CTest)

# Set required standard level
set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

# Always use standard .o suffix
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

# Options
include(CMakeDependentOption)

option (BUILD_SHARED_LIBS "Build shared library" OFF)
option (LOG_ENABLE "Enable logging" OFF)
option (BUILD_TESTS "Build unit tests" OFF)

set(LOG_STATE_VALUES "ON;OFF")
set(LOG_LEVEL_VALUES "DEBUG;INFO;WARNING;ERROR")

Set(IOLINK_NUM_PORTS "2"
    CACHE STRING "max number IO-Link ports")

Set(IOLINK_NUM_DIAG_ENTRIES "64"
    CACHE STRING "max number IO-Link diagnosis entries")

Set(IOLINK_MAX_EVENTS "6"
    CACHE STRING "max number IO-Link events")

set(LOG_LEVEL INFO CACHE STRING "default log level")
set_property(CACHE LOG_LEVEL PROPERTY STRINGS ${LOG_LEVEL_VALUES})

# Default to release build with debug info
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
endif (NOT CMAKE_BUILD_TYPE)

message (STATUS "Default log level is: ${LOG_LEVEL}")
message (STATUS "Build type is: ${CMAKE_BUILD_TYPE}")
message (STATUS "Building for ${CMAKE_SYSTEM_NAME}")

# Generate version numbers
configure_file (
  ${IOLINKMASTER_SOURCE_DIR}/version.h.in
  ${IOLINKMASTER_BINARY_DIR}/include/version.h
  )

# Generate config options
configure_file (
  ${IOLINKMASTER_SOURCE_DIR}/options.h.in
  ${IOLINKMASTER_BINARY_DIR}/src/options.h
  )

# Include paths
include_directories(
  ${IOLINKMASTER_BINARY_DIR}/include
  ${IOLINKMASTER_BINARY_DIR}/src
  ${IOLINKMASTER_SOURCE_DIR}/include
  ${IOLINKMASTER_SOURCE_DIR}/src
  )

# Platform configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/${CMAKE_SYSTEM_NAME}.cmake)

# Source paths
add_subdirectory (src)
if (${BUILD_TESTS})
  add_subdirectory (test)
endif (${BUILD_TESTS})
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_subdirectory (samples/ifm_sample_app)
endif ()

# Doxygen configuration
cmake_policy(SET CMP0057 NEW)
find_package(Doxygen)
if (DOXYGEN_FOUND)
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
  set(DOXYGEN_TYPEDEF_HIDES_STRUCT YES)
  set(DOXYGEN_STRIP_CODE_COMMENTS NO)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
  set(DOXYGEN_PREDEFINED IOLINKMASTER_EXPORT)
  set(DOXYGEN_EXPAND_AS_DEFINED IOLINKMASTER_EXPORT)
  set(DOXYGEN_COLLABORATION_GRAPH NO)
  set(DOXYGEN_INCLUDE_GRAPH NO)
  set(DOXYGEN_INCLUDED_BY_GRAPH NO)
  set(DOXYGEN_RECURSIVE NO)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  doxygen_add_docs(docs
    README.md
    include
    src
    )
endif ()

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${IOLINKMASTER_SOURCE_DIR}/LICENSE.md")
set (CPACK_PACKAGE_IOLMCONTACT info.iolink@rt-labs.com)
include (CPack)
