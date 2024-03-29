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

# If we are building this project only
# we look for installed rtkernel
if (CMAKE_PROJECT_NAME STREQUAL IOLINKMASTER)
    option(IOLINK_BUILD_RTKERNEL "Build rt-kernel from source instead of pre-built libraries" OFF)

    if (IOLINK_BUILD_RTKERNEL)
        add_subdirectory(${RTK} rt-kernel EXCLUDE_FROM_ALL)
    else()
        find_package(rtkernel)
        find_package(${BSP})
    endif()
endif()

add_library(iolmaster_osal)

target_sources(iolmaster_osal
  PRIVATE
  ${IOLINKMASTER_SOURCE_DIR}/iol_osal/rt-kernel/osal_irq.c
  ${IOLINKMASTER_SOURCE_DIR}/iol_osal/rt-kernel/osal_spi.c
)

target_include_directories(iolmaster_osal
  PUBLIC
    $<BUILD_INTERFACE:${IOLINKMASTER_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${IOLINKMASTER_SOURCE_DIR}/iol_osal/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(iolmaster_osal
  PRIVATE
  kern
  dev
  osal
  ${BSP}
)
