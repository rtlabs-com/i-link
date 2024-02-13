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

# NOTE: add headers to make them show up in an IDE
target_sources(iolmaster
  PRIVATE
  ${IOLINKMASTER_SOURCE_DIR}/include/iolink_max14819.h
  iolink_al.c
  iolink_al.h
  iolink_cm.c
  iolink_cm.h
  iolink_dl.c
  iolink_dl.h
  iolink_ds.c
  iolink_ds.h
  iolink_main.c
  iolink_main.h
  iolink_max14819_pl.c
  iolink_max14819_pl.h
  iolink_ode.c
  iolink_ode.h
  iolink_pde.c
  iolink_pde.h
  iolink_pl.c
  iolink_pl.h
  iolink_pl_hw_drv.h
  iolink_sm.c
  iolink_sm.h
  iolink_types.h
  options.h
  ${IOLINKMASTER_BINARY_DIR}/include/version.h
  )

generate_export_header(iolmaster
  BASE_NAME iolmaster
  EXPORT_FILE_NAME ${IOLINKMASTER_BINARY_DIR}/include/iolmaster_export.h
  )

target_link_libraries(iolmaster osal iolmaster_osal)

install (TARGETS iolmaster DESTINATION bin)
install (FILES
  ${IOLINKMASTER_SOURCE_DIR}/include/iolink.h
  ${IOLINKMASTER_SOURCE_DIR}/include/iolink_max14819.h
  ${IOLINKMASTER_BINARY_DIR}/include/iolmaster_export.h
  DESTINATION include)
