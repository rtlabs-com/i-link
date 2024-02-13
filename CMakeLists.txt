#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# www.rt-labs.com
# Copyright 2019 rt-labs AB, Sweden.
#
# This software is dual-licensed under GPLv3 and a commercial
# license. See the file LICENSE.md distributed with this software for
# full license information.
#*******************************************************************/

cmake_minimum_required (VERSION 3.21)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/tools")
project (IOLINKMASTER VERSION 0.1.0)

# Set required standard level
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

# Always use standard .o suffix
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

# Default to installing in build directory
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${IOLINKMASTER_BINARY_DIR}/install
    CACHE PATH "Default install path" FORCE)
endif()

# Platform configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/${CMAKE_SYSTEM_NAME}.cmake)

# Add Osal dependent on archtitecture
include(AddOsal)
include(CMakeDependentOption)

include(GetGitRevision)

# Options

option (BUILD_SHARED_LIBS "Build shared library" OFF)
option (LOG_ENABLE "Enable logging" OFF)
option (BUILD_TESTING "Build unit tests" OFF)
option (IOLINKMASTER_BUILD_DOCS "Build docs" OFF)

set(LOG_STATE_VALUES "ON;OFF")
set(LOG_LEVEL_VALUES "DEBUG;INFO;WARNING;ERROR")

Set(IOLINK_NUM_PORTS "4"
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

# Source paths
add_subdirectory (src)

# Include paths
target_include_directories(iolmaster
  PUBLIC
  ${IOLINKMASTER_BINARY_DIR}/include
  ${IOLINKMASTER_BINARY_DIR}/src
  ${IOLINKMASTER_SOURCE_DIR}/include
  ${IOLINKMASTER_SOURCE_DIR}/src
)

if (BUILD_TESTING)
  include(CTest)
  add_subdirectory (test)
endif (BUILD_TESTING)

if (CMAKE_PROJECT_NAME STREQUAL IOLINKMASTER)
  if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory (samples/ifm_sample_app)
    add_subdirectory (samples/irq_test)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "rt-kernel")
    add_subdirectory (samples/ifm_sample_app)
  endif ()
endif()

if (IOLINKMASTER_BUILD_DOCS)
  add_subdirectory(docs)

  add_custom_target(codespell
    COMMAND codespell
        ${IOLINKMASTER_SOURCE_DIR}/docs/
        ${IOLINKMASTER_SOURCE_DIR}/include/
        ${IOLINKMASTER_SOURCE_DIR}/iol_osal/
        ${IOLINKMASTER_SOURCE_DIR}/samples/
        ${IOLINKMASTER_SOURCE_DIR}/src/
        ${IOLINKMASTER_SOURCE_DIR}/test/
        --skip *_build*
    COMMENT "Running spell check on source code"
    )
endif()

include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE "${IOLINKMASTER_SOURCE_DIR}/LICENSE.md")
set (CPACK_PACKAGE_IOLMCONTACT info.iolink@rt-labs.com)
include (CPack)
