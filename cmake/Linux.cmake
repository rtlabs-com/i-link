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

set(OSAL_SOURCES
  ${IOLINKMASTER_SOURCE_DIR}/iol_osal/linux/osal_drv.c
  ${IOLINKMASTER_SOURCE_DIR}/iol_osal/linux/osal_fileops.c
  )
set(OSAL_INCLUDES
  ${IOLINKMASTER_SOURCE_DIR}/iol_osal/include
  )

set(GOOGLE_TEST_INDIVIDUAL TRUE)
