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

add_library(iolmaster_osal INTERFACE)

target_sources(iolmaster_osal
  INTERFACE
  ${IOLINKMASTER_SOURCE_DIR}/iol_osal/linux/osal_irq.c
  ${IOLINKMASTER_SOURCE_DIR}/iol_osal/linux/osal_spi.c
)

target_include_directories(iolmaster_osal
  INTERFACE
  ${IOLINKMASTER_SOURCE_DIR}/iol_osal/include
)

target_link_libraries(iolmaster_osal
  INTERFACE
  osal
)

set(GOOGLE_TEST_INDIVIDUAL TRUE)
