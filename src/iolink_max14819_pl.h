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

/**
 * @file
 * @brief Driver for the MAX14819 IO-Link chip
 *
 * The iolink_max14819 driver is used for communication with a MAX14819
 * IO-Link chip. The chip contains two IO-Link master transceivers with logic.
 *
 */

#ifndef IOLINK_MAX14819_PL_H
#define IOLINK_MAX14819_PL_H

#include <iolink.h>
// TODO
#ifdef __rtk__
#include <kern/types.h>
#include <gpio.h>
#endif /* __rtk__ */

#ifdef __cplusplus
extern "C" {
#endif

#define MAX14819_NUM_CHANNELS 2
#define IOLINK14819_TX_MAX    (66)

#include <osal.h>
#include <iolink_pl_hw_drv.h>

/* Driver structure */
typedef struct iolink_14819_drv
{
   /* Generic driver interface */
   iolink_hw_drv_t drv;

   /* Private data */
   int fd_spi;
   uint8_t chip_address;

   uint32_t pl_flag;

   bool wurq_request[MAX14819_NUM_CHANNELS];
   bool is_iolink[MAX14819_NUM_CHANNELS];
   os_mutex_t * exclusive;

   os_event_t * dl_event[MAX14819_NUM_CHANNELS];
#ifdef __rtk__
   gpio_t pin[MAX14819_NUM_CHANNELS];
#endif
} iolink_14819_drv_t;

#ifdef __cplusplus
}
#endif

#endif /* IOLINK_MAX14819_PL_H */
