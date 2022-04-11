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

#include <iolink_pl_hw.h>
#include "osal.h"
#ifdef __rtk__
#include <dev.h>
#include <fd.h>
#endif

#include "iolink_pl_hw_drv.h"

/**
 * @file
 * @brief PL hardware interface
 *
 */

#define GET_ARG_AND_LOCK(fd)                                                   \
   drv_t * drv                 = fd_get_driver (fd);                           \
   iolink_hw_drv_t * iolink_hw = (iolink_hw_drv_t *)drv;                       \
   void * arg                  = fd_get_arg (fd);                              \
   os_mutex_lock (drv->mtx);

#define UNLOCK(drv) os_mutex_unlock (drv->mtx);

#define UNLOCK_AND_RETURN(drv, item)                                           \
   os_mutex_unlock (drv->mtx);                                                 \
   return item;

iolink_baudrate_t iolink_pl_hw_get_baudrate (int fd)
{
   GET_ARG_AND_LOCK (fd);

   iolink_baudrate_t baudrate = iolink_hw->ops->get_baudrate (iolink_hw, arg);

   UNLOCK_AND_RETURN (drv, baudrate);
}

uint8_t iolink_pl_hw_get_cycletime (int fd)
{
   GET_ARG_AND_LOCK (fd);

   uint8_t cycletime = iolink_hw->ops->get_cycletime (iolink_hw, arg);

   UNLOCK_AND_RETURN (drv, cycletime);
}

void iolink_pl_hw_set_cycletime (int fd, uint8_t cycbyte)
{
   GET_ARG_AND_LOCK (fd);

   iolink_hw->ops->set_cycletime (iolink_hw, arg, cycbyte);

   UNLOCK (drv);
}

bool iolink_pl_hw_set_mode (int fd, iolink_pl_mode_t mode)
{
   GET_ARG_AND_LOCK (fd);

   bool res = iolink_hw->ops->set_mode (iolink_hw, arg, mode);

   UNLOCK_AND_RETURN (drv, res);
}

void iolink_pl_hw_enable_cycle_timer (int fd)
{
   GET_ARG_AND_LOCK (fd);

   iolink_hw->ops->enable_cycle_timer (iolink_hw, arg);

   UNLOCK (drv);
}

void iolink_pl_hw_disable_cycle_timer (int fd)
{
   GET_ARG_AND_LOCK (fd);

   iolink_hw->ops->disable_cycle_timer (iolink_hw, arg);

   UNLOCK (drv);
}

bool iolink_pl_hw_get_data (int fd, uint8_t * rxdata, uint8_t len)
{
   GET_ARG_AND_LOCK (fd);

   bool result = iolink_hw->ops->get_data (iolink_hw, arg, rxdata, len);

   UNLOCK (drv);

   return result;
}

void iolink_pl_hw_get_error (int fd, uint8_t * cqerr, uint8_t * devdly)
{
   GET_ARG_AND_LOCK (fd);

   iolink_hw->ops->get_error (iolink_hw, arg, cqerr, devdly);

   UNLOCK (drv);
}

void iolink_pl_hw_send_msg (int fd)
{
   GET_ARG_AND_LOCK (fd);

   iolink_hw->ops->send_msg (iolink_hw, arg);

   UNLOCK (drv);
}

void iolink_pl_hw_dl_msg (int fd, uint8_t rxbytes, uint8_t txbytes, uint8_t * data)
{
   GET_ARG_AND_LOCK (fd);

   iolink_hw->ops->dl_msg (iolink_hw, arg, rxbytes, txbytes, data);

   UNLOCK (drv);
}

void iolink_pl_hw_transfer_req (
   int fd,
   uint8_t rxbytes,
   uint8_t txbytes,
   uint8_t * data)
{
   GET_ARG_AND_LOCK (fd);

   iolink_hw->ops->transfer_req (iolink_hw, arg, rxbytes, txbytes, data);

   UNLOCK (drv);
}

bool iolink_pl_hw_init_sdci (int fd)
{
   GET_ARG_AND_LOCK (fd);

   bool res = iolink_hw->ops->init_sdci (iolink_hw, arg);

   UNLOCK_AND_RETURN (drv, res);
}

void iolink_pl_hw_configure_event (int fd, os_event_t * event, uint32_t flag)
{
   GET_ARG_AND_LOCK (fd);

   iolink_hw->ops->configure_event (iolink_hw, arg, event, flag);

   UNLOCK (drv);
}

void iolink_pl_hw_pl_handler (int fd)
{
   GET_ARG_AND_LOCK (fd);

   iolink_hw->ops->pl_handler (iolink_hw, arg);

   UNLOCK (drv);
}
