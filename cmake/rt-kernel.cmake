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

set(OSAL_INCLUDES
  ${IOLINKMASTER_SOURCE_DIR}/../osal/include
  ${IOLINKMASTER_SOURCE_DIR}/../osal/src/rt-kernel
  ${RTK}/include
  ${RTK}/include/kern
  ${RTK}/include/arch/${ARCH}
  ${RTK}/include/drivers
  )
