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

include(AddGoogleTest)

add_definitions(
  -DUNIT_TEST
  )

# TODO: ett snyggare sätt att bygga test beroende på linux/linux-usb / rt-kernel
add_executable(iol_test
  # Units to be tested
  ${IOLINKMASTER_SOURCE_DIR}/src/iolink_main.c
  ${IOLINKMASTER_SOURCE_DIR}/src/iolink_sm.c
  ${IOLINKMASTER_SOURCE_DIR}/src/iolink_al.c
  ${IOLINKMASTER_SOURCE_DIR}/src/iolink_cm.c
  ${IOLINKMASTER_SOURCE_DIR}/src/iolink_ds.c
  ${IOLINKMASTER_SOURCE_DIR}/src/iolink_ode.c
  ${IOLINKMASTER_SOURCE_DIR}/src/iolink_pde.c
  ${IOLINKMASTER_SOURCE_DIR}/iol_osal/linux/osal_spi_usb_helpers.c

  # Unit tests
  test_sm.cpp
  test_al.cpp
  test_cm.cpp
  test_ds.cpp
  test_ode.cpp
  test_pde.cpp
  test_spi_usb.cpp

  # Test utils
  mocks.h
  mocks.cpp
  test_util.h

  # Testrunner
  iolink_test.cpp
  )

target_include_directories(iol_test
  PRIVATE
  .
  "$<TARGET_PROPERTY:iolmaster,INCLUDE_DIRECTORIES>"
  SYSTEM
  "$<TARGET_PROPERTY:iolmaster,SYSTEM_INCLUDE_DIRECTORIES>"
  )

target_link_libraries(iol_test PUBLIC osal)
add_gtest(iol_test)

# No need for gmock
set(BUILD_GMOCK OFF CACHE BOOL "")
