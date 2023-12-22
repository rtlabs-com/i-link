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

#include "iolink_pl.h"
#include "iolink_pl_hw.h"

#include "iolink_main.h" /* iolink_port_t */

/**
 * @file
 * @brief Physical layer
 *
 */

#define IOLINK_RXERR_CHKSM  BIT (0)
#define IOLINK_RXERR_SIZE   BIT (1)
#define IOLINK_RXERR_FRAME  BIT (2)
#define IOLINK_RXERR_PARITY BIT (3)
#define IOLINK_RXERR_TDLY   BIT (4)
#define IOLINK_TXERR_TRANSM BIT (5)
#define IOLINK_TXERR_CYCL   BIT (6)
#define IOLINK_TXERR_CHKSM  BIT (7)
#define IOLINK_TXERR_SIZE   BIT (8)

/*
 * Wrapper functions
 */
void iolink_configure_pl_event (
   iolink_port_t * port,
   os_event_t * event,
   uint32_t flag)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   iolink_pl_hw_configure_event (pl->fd, event, flag);
}

void iolink_pl_handler (iolink_port_t * port)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   iolink_pl_hw_pl_handler (pl->fd);
}

iolink_baudrate_t iolink_pl_get_baudrate (iolink_port_t * port)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   return iolink_pl_hw_get_baudrate (pl->fd);
}

uint8_t iolink_pl_get_cycletime (iolink_port_t * port)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   return iolink_pl_hw_get_cycletime (pl->fd);
}

void iolink_pl_set_cycletime (iolink_port_t * port, uint8_t cycbyte)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   iolink_pl_hw_set_cycletime (pl->fd, cycbyte);
}

bool iolink_pl_get_data (iolink_port_t * port, uint8_t * rxdata, uint8_t len)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   return iolink_pl_hw_get_data (pl->fd, rxdata, len);
}

void iolink_pl_get_error (iolink_port_t * port, uint8_t * cqerr, uint8_t * devdly)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   iolink_pl_hw_get_error (pl->fd, cqerr, devdly);
}

bool iolink_pl_init_sdci (iolink_port_t * port)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   return iolink_pl_hw_init_sdci (pl->fd);
}
/*
 * PL services
 */
void PL_SetMode_req (iolink_port_t * port, iolink_pl_mode_t mode)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   iolink_pl_hw_set_mode (pl->fd, mode);
}
void PL_WakeUp_req (iolink_port_t * port)
{
#if IOLINK_HW == IOLINK_HW_MAX14819
#endif
}
#if IOLINK_HW == IOLINK_HW_MAX14819
void PL_Resend (iolink_port_t * port)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   iolink_pl_hw_send_msg (pl->fd);
}
void PL_Transfer_req (
   iolink_port_t * port,
   uint8_t rxbytes,
   uint8_t txbytes,
   uint8_t * data)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   iolink_pl_hw_transfer_req (pl->fd, rxbytes, txbytes, data);
}
void PL_MessageDownload_req (
   iolink_port_t * port,
   uint8_t rxbytes,
   uint8_t txbytes,
   uint8_t * data)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   iolink_pl_hw_dl_msg (pl->fd, rxbytes, txbytes, data);
}
void PL_EnableCycleTimer (iolink_port_t * port)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   iolink_pl_hw_enable_cycle_timer (pl->fd);
}
void PL_DisableCycleTimer (iolink_port_t * port)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   iolink_pl_hw_disable_cycle_timer (pl->fd);
}
#else
void PL_Transfer_req (iolink_port_t * port, uint8_t data)
{
}
#endif

void iolink_pl_init (iolink_port_t * port, const char * name)
{
   iolink_pl_port_t * pl = iolink_get_pl_ctx (port);

   memset (pl, 0, sizeof (iolink_pl_port_t));
   pl->fd = iolink_pl_hw_open (name);
   CC_ASSERT (pl->fd > 0);
}
