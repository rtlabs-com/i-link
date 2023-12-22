/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2019 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#ifndef IOLINK_PL_HW_H
#define IOLINK_PL_HW_H

#include "osal.h"
#ifdef __rtk__
#include <types.h>
#include <dev.h>
#else
#include "osal_fileops.h"
#endif
#include <fcntl.h>  /* O_RDWR */
#include <unistd.h> /* close */

#include "iolink_types.h"
#include "iolink.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief         API for PL hardware access
 *
 */

/**
 * Open the IO-Link channel given by @a name
 *
 * @param name    Name of the channel to open
 * @return        Handle to be used in all further
 *                operations on channel, or -1 on error
 */
static inline int iolink_pl_hw_open (const char * name)
{
#ifdef __linux__
   return _iolink_pl_hw_open (name);
#else
   return open (name, O_RDWR, 0);
#endif
}

/**
 * Close the IO-Link channel
 *
 * @param fd      Handle of IO-Link channel
 */
static inline void iolink_pl_hw_close (int fd)
{
   close (fd);
}

iolink_baudrate_t iolink_pl_hw_get_baudrate (int fd);

uint8_t iolink_pl_hw_get_cycletime (int fd);

void iolink_pl_hw_set_cycletime (int fd, uint8_t cycbyte);

bool iolink_pl_hw_set_mode (int fd, iolink_pl_mode_t mode);

void iolink_pl_hw_enable_cycle_timer (int fd);

void iolink_pl_hw_disable_cycle_timer (int fd);

bool iolink_pl_hw_get_data (int fd, uint8_t * rxdata, uint8_t len);
void iolink_pl_hw_get_error (int fd, uint8_t * cqerr, uint8_t * devdly);
void iolink_pl_hw_send_msg (int fd);
void iolink_pl_hw_dl_msg (int fd, uint8_t rxbytes, uint8_t txbytes, uint8_t * data);
void iolink_pl_hw_transfer_req (
   int fd,
   uint8_t rxbytes,
   uint8_t txbytes,
   uint8_t * data);

bool iolink_pl_hw_init_sdci (int fd);

void iolink_pl_hw_configure_event (int fd, os_event_t * event, uint32_t flag);

void iolink_pl_hw_pl_handler (int fd);

#ifdef __cplusplus
}
#endif

#endif /* IOLINK_PL_HW_H */
