/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include <stdio.h>

#include "osal.h"
#include "osal_irq.h"
#include "osal_log.h"
#include "iolink.h"
#include "iolink_max14819.h"
#include "iolink_handler.h"


#define IOLINK_MASTER_THREAD_STACK_SIZE (4 * 1024)
#define IOLINK_MASTER_THREAD_PRIO       6
#define IOLINK_DL_THREAD_STACK_SIZE     1500
#define IOLINK_DL_THREAD_PRIO           (IOLINK_MASTER_THREAD_PRIO + 1)
#define IOLINK_HANDLER_THREAD_STACK_SIZE (2048)
#define IOLINK_HANDLER_THREAD_PRIO       6

#ifdef __rtk__

#ifdef SPI_IOLINK0
#  define IOLINK_APP_CHIP0_SPI       SPI_IOLINK0
#  define IOLINK_APP_CHIP0_IRQ       IRQ_IOLINK0
#endif

#ifdef ADR_IOLINK0
#  define IOLINK_APP_CHIP0_ADDRESS   ADR_IOLINK0
#endif

#ifdef SPI_IOLINK1
#  define IOLINK_APP_CHIP1_SPI       SPI_IOLINK1
#  define IOLINK_APP_CHIP1_IRQ       IRQ_IOLINK1
#endif

#ifdef ADR_IOLINK1
#  define IOLINK_APP_CHIP1_ADDRESS   ADR_IOLINK1
#endif

#else

#define IOLINK_APP_CHIP0_SPI      "/dev/spidev0.0"
#define IOLINK_APP_CHIP0_IRQ      25

#endif

#ifndef IOLINK_APP_CHIP0_ADDRESS
#define IOLINK_APP_CHIP0_ADDRESS 0x0
#endif

#ifndef IOLINK_APP_CHIP1_ADDRESS
#define IOLINK_APP_CHIP1_ADDRESS 0x0
#endif


static iolink_pl_mode_t mode_ch[] =
{
#ifdef IOLINK_APP_CHIP0_SPI
   iolink_mode_SDCI,
   iolink_mode_INACTIVE,
#endif
#ifdef IOLINK_APP_CHIP1_SPI
   iolink_mode_SDCI,
   iolink_mode_INACTIVE,
#endif
};

void main_handler_thread (void * ctx)
{
   const iolink_m_cfg_t * cfg = (const iolink_m_cfg_t *)ctx;
   iolink_handler (*cfg);
}

static iolink_hw_drv_t * main_14819_init(const char* name, const iolink_14819_cfg_t * cfg, int irq)
{
   iolink_hw_drv_t  * drv;
   drv = iolink_14819_init (cfg);
   if (drv == NULL)
   {
      LOG_ERROR (IOLINK_APP_LOG, "APP: Failed to open SPI %s\n", name);
      return NULL;
   }

   if (_iolink_setup_int (irq, iolink_14819_isr, drv) < 0)
   {
      LOG_ERROR (IOLINK_APP_LOG, "APP: Failed to setup interrupt %s\n", name);
      return NULL;
   }
   return drv;
}

int main (int argc, char ** argv)
{
   os_thread_t * iolink_handler_thread;
   iolink_hw_drv_t * hw[2];

#ifdef IOLINK_APP_CHIP0_SPI
   static const iolink_14819_cfg_t iol_14819_0_cfg = {
      .chip_address   = IOLINK_APP_CHIP0_ADDRESS,
      .spi_slave_name = IOLINK_APP_CHIP0_SPI,
      .CQCfgA         = MAX14819_CQCFG_DRVDIS | MAX14819_CQCFG_SINKSEL(0x2),
      .LPCnfgA        = MAX14819_LPCNFG_LPEN,
      .IOStCfgA       = MAX14819_IOSTCFG_DICSINK | MAX14819_IOSTCFG_DIEC3TH,
      .DrvCurrLim     = 0x00,
      .Clock          = MAX14819_CLOCK_XTALEN | MAX14819_CLOCK_TXTXENDIS,
   };
#endif

#ifdef IOLINK_APP_CHIP1_SPI
   static const iolink_14819_cfg_t iol_14819_1_cfg = {
      .chip_address   = IOLINK_APP_CHIP1_ADDRESS,
      .spi_slave_name = IOLINK_APP_CHIP1_SPI,
      .CQCfgA         = MAX14819_CQCFG_DRVDIS | MAX14819_CQCFG_SINKSEL(0x2),
      .LPCnfgA        = MAX14819_LPCNFG_LPEN,
      .IOStCfgA       = MAX14819_IOSTCFG_DICSINK | MAX14819_IOSTCFG_DIEC3TH,
      .DrvCurrLim     = 0x00,
      .Clock          = MAX14819_CLOCK_XTALEN | MAX14819_CLOCK_TXTXENDIS,
   };
#endif

#ifdef IOLINK_APP_CHIP0_SPI
   hw[0] = main_14819_init("/iolink0", &iol_14819_0_cfg, IOLINK_APP_CHIP0_IRQ);
#endif

#ifdef IOLINK_APP_CHIP1_SPI
   hw[1] = main_14819_init("/iolink1", &iol_14819_1_cfg, IOLINK_APP_CHIP1_IRQ);
#endif

   iolink_port_cfg_t port_cfgs[] = {
#ifdef IOLINK_APP_CHIP0_SPI
      {
         .name = "/iolink0/0",
         .mode = &mode_ch[0],
         .drv  = hw[0],
         .arg  = (void*)0,
      },
      {
         .name = "/iolink0/1",
         .mode = &mode_ch[1],
         .drv  = hw[0],
         .arg  = (void*)1,
      },
#endif
#ifdef IOLINK_APP_CHIP1_SPI
      {
         .name = "/iolink1/0",
         .mode = &mode_ch[2],
         .drv  = hw[1],
         .arg  = (void*)0,
      },
      {
         .name = "/iolink1/1",
         .mode = &mode_ch[3],
         .drv  = hw[1],
         .arg  = (void*)1,
      },
#endif
   };

   iolink_m_cfg_t iolink_cfg = {
      .cb_arg                   = NULL,
      .cb_smi                   = NULL,
      .cb_pd                    = NULL,
      .port_cnt                 = NELEMENTS (port_cfgs),
      .port_cfgs                = port_cfgs,
      .master_thread_prio       = IOLINK_MASTER_THREAD_PRIO,
      .master_thread_stack_size = IOLINK_MASTER_THREAD_STACK_SIZE,
      .dl_thread_prio           = IOLINK_DL_THREAD_PRIO,
      .dl_thread_stack_size     = IOLINK_DL_THREAD_STACK_SIZE,
   };

   os_usleep (200 * 1000);

   iolink_handler_thread = os_thread_create (
      "iolink_handler_thread",
      IOLINK_HANDLER_THREAD_PRIO,
      IOLINK_HANDLER_THREAD_STACK_SIZE,
      main_handler_thread,
      (void*)&iolink_cfg);
   CC_ASSERT (iolink_handler_thread != NULL);

   for (;;)
   {
      os_usleep (1000 * 1000);
   }

   return 0;
}
