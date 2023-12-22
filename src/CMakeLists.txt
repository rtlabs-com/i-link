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

# NOTE: add headers to make them show up in an IDE
set (IOLINKMASTER_SOURCES
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
  iolink_pl_hw.c
  iolink_pl_hw_drv.h
  iolink_pl_hw.h
  iolink_sm.c
  iolink_sm.h
  iolink_types.h
  options.h
  ${IOLINKMASTER_BINARY_DIR}/include/version.h
  )

add_library (iolmaster
  ${IOLINKMASTER_SOURCES}
  ${OSAL_SOURCES}
  )

generate_export_header(iolmaster
  BASE_NAME iolmaster
  EXPORT_FILE_NAME ${IOLINKMASTER_BINARY_DIR}/include/iolmaster_export.h
  )

include_directories(${OSAL_INCLUDES})

target_link_libraries(iolmaster osal)

set_property(TARGET iolmaster PROPERTY POSITION_INDEPENDENT_CODE ON)

install (TARGETS iolmaster DESTINATION bin)
install (FILES
  ${IOLINKMASTER_SOURCE_DIR}/include/iolink.h
  ${IOLINKMASTER_SOURCE_DIR}/include/iolink_max14819.h
  ${IOLINKMASTER_BINARY_DIR}/include/iolmaster_export.h
  DESTINATION include)
