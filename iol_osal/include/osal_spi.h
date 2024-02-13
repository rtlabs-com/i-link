/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2020 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef OSAL_PL_HW_SPI_H
#define OSAL_PL_HW_SPI_H

#include <sys/types.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

void _iolink_pl_hw_spi_transfer (
   int fd,
   void * data_read,
   const void * data_written,
   size_t n_bytes_to_transfer);

#ifdef __cplusplus
}
#endif

#endif
