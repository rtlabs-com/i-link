#********************************************************************
#        _       _         _
#  _ __ | |_  _ | |  __ _ | |__   ___
# | '__|| __|(_)| | / _` || '_ \ / __|
# | |   | |_  _ | || (_| || |_) |\__ \
# |_|    \__|(_)|_| \__,_||_.__/ |___/
#
# www.rt-labs.com
# Copyright 2017 rt-labs AB, Sweden.
#
# This software is dual-licensed under GPLv3 and a commercial
# license. See the file LICENSE.md distributed with this software for
# full license information.
#*******************************************************************/

add_executable(iol_master_ifm_sample_app
  iolink_smi.c
  iolink_ifm.c
  iolink_handler.c
  iol_master_sample_app.c
  )

target_include_directories(iol_master_ifm_sample_app
  PRIVATE
  ${IOLINK_SOURCE_DIR}/src
  ${IOLINK_BINARY_DIR}/src
  )

target_link_libraries (iol_master_ifm_sample_app
  PUBLIC
  iolmaster
  iolmaster_osal
  )
