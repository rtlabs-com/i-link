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

include(GenerateExportHeader)

add_library (iolmaster)

target_sources(iolmaster
  PRIVATE
  iolink_al.c
  iolink_cm.c
  iolink_dl.c
  iolink_ds.c
  iolink_main.c
  iolink_max14819_pl.c
  iolink_ode.c
  iolink_pde.c
  iolink_pl.c
  iolink_sm.c
  )

generate_export_header(iolmaster
  BASE_NAME iolmaster
  EXPORT_FILE_NAME ${IOLINKMASTER_BINARY_DIR}/include/iolmaster_export.h
  )

target_link_libraries(iolmaster osal iolmaster_osal)
