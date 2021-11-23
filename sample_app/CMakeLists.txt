#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# http://www.rt-labs.com
# Copyright 2017 rt-labs AB, Sweden.
# See LICENSE file in the project root for full license information.
#*******************************************************************/

add_executable(irq_test
  util/irq_test.c
  )

add_executable(iol_master_sample_app
  iolink_smi.c
  iolink_ifm.c
  iolink_handler.c
  iol_master_sample_app.c
  )

target_include_directories(iol_master_sample_app
  PRIVATE
  ${IOLINK_SOURCE_DIR}/src
  ${IOLINK_BINARY_DIR}/src
  )

include_directories(${OSAL_INCLUDES})

target_link_libraries (iol_master_sample_app PUBLIC iolmaster)
target_link_libraries (irq_test PUBLIC pthread)
