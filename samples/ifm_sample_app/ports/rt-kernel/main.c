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
#include "app_handler.h"
#include <bsp.h>

#define APP_MASTER_THREAD_STACK_SIZE  (4 * 1024)
#define APP_MASTER_THREAD_PRIO        6
#define APP_DL_THREAD_STACK_SIZE      1500
#define APP_DL_THREAD_PRIO            (APP_MASTER_THREAD_PRIO + 1)
#define APP_HANDLER_THREAD_STACK_SIZE (2048)
#define APP_HANDLER_THREAD_PRIO       6


#ifdef SPI_IOLINK0
#define APP_IOLINK_CHIP0_SPI SPI_IOLINK0
#define APP_IOLINK_CHIP0_IRQ IRQ_IOLINK0
#endif

#ifdef ADR_IOLINK0
#define APP_IOLINK_CHIP0_ADDRESS ADR_IOLINK0
#endif

#ifdef SPI_IOLINK1
#define APP_IOLINK_CHIP1_SPI SPI_IOLINK1
#define APP_IOLINK_CHIP1_IRQ IRQ_IOLINK1
#endif

#ifdef ADR_IOLINK1
#define APP_IOLINK_CHIP1_ADDRESS ADR_IOLINK1
#endif


#ifndef APP_IOLINK_CHIP0_ADDRESS
#define APP_IOLINK_CHIP0_ADDRESS 0x0
#endif

#ifndef APP_IOLINK_CHIP1_ADDRESS
#define APP_IOLINK_CHIP1_ADDRESS 0x0
#endif

static iolink_pl_mode_t mode_ch[] = {
#ifdef APP_IOLINK_CHIP0_SPI
   iolink_mode_SDCI,
   iolink_mode_INACTIVE,
#endif
#ifdef APP_IOLINK_CHIP1_SPI
   iolink_mode_SDCI,
   iolink_mode_INACTIVE,
#endif
};

void app_handler_thread_start (void * ctx)
{
   const iolink_m_cfg_t * cfg = (const iolink_m_cfg_t *)ctx;
   app_handler (*cfg);
}

static iolink_hw_drv_t * app_14819_init (
   const char * name,
   const iolink_14819_cfg_t * cfg,
   int irq)
{
   iolink_hw_drv_t * drv;
   drv = iolink_14819_init (cfg);
   if (drv == NULL)
   {
      LOG_ERROR (IOLINK_APP_LOG, "APP: Failed to open SPI %s\n", name);
      return NULL;
   }
   return drv;
}

int main (int argc, char ** argv)
{
   os_thread_t * app_handler_thread;
   iolink_hw_drv_t * hw[2];

#ifdef APP_IOLINK_CHIP0_SPI
   static const iolink_14819_cfg_t iol_14819_0_cfg = {
      .chip_address   = APP_IOLINK_CHIP0_ADDRESS,
      .chip_irq       = APP_IOLINK_CHIP0_IRQ,
      .spi_slave_name = APP_IOLINK_CHIP0_SPI,
      .CQCfgA         = MAX14819_CQCFG_DRVDIS | MAX14819_CQCFG_SINKSEL (0x2),
      .LPCnfgA        = MAX14819_LPCNFG_LPEN,
      .IOStCfgA       = MAX14819_IOSTCFG_DICSINK | MAX14819_IOSTCFG_DIEC3TH,
      .DrvCurrLim     = 0x00,
      .Clock          = MAX14819_CLOCK_XTALEN | MAX14819_CLOCK_TXTXENDIS,
   };
#endif

#ifdef APP_IOLINK_CHIP1_SPI
   static const iolink_14819_cfg_t iol_14819_1_cfg = {
      .chip_address   = APP_IOLINK_CHIP1_ADDRESS,
      .chip_irq       = APP_IOLINK_CHIP1_IRQ,
      .spi_slave_name = APP_IOLINK_CHIP1_SPI,
      .CQCfgA         = MAX14819_CQCFG_DRVDIS | MAX14819_CQCFG_SINKSEL (0x2),
      .LPCnfgA        = MAX14819_LPCNFG_LPEN,
      .IOStCfgA       = MAX14819_IOSTCFG_DICSINK | MAX14819_IOSTCFG_DIEC3TH,
      .DrvCurrLim     = 0x00,
      .Clock          = MAX14819_CLOCK_XTALEN | MAX14819_CLOCK_TXTXENDIS,
   };
#endif

#ifdef APP_IOLINK_CHIP0_SPI
   hw[0] = iolink_14819_init (&iol_14819_0_cfg);
   if (hw[0] == NULL)
   {
      LOG_ERROR (IOLINK_APP_LOG, "APP: Failed to open driver: 0\n");
      exit(-1);
   }
#endif

#ifdef APP_IOLINK_CHIP1_SPI
   hw[1] = iolink_14819_init (&iol_14819_1_cfg);
   if (hw[1] == NULL)
   {
      LOG_ERROR (IOLINK_APP_LOG, "APP: Failed to open driver: 1\n");
      exit(-1);
   }
#endif

   iolink_port_cfg_t port_cfgs[] = {
#ifdef APP_IOLINK_CHIP0_SPI
      {
         .name = "/iolink0/0",
         .mode = &mode_ch[0],
         .drv  = hw[0],
         .arg  = (void *)0,
      },
      {
         .name = "/iolink0/1",
         .mode = &mode_ch[1],
         .drv  = hw[0],
         .arg  = (void *)1,
      },
#endif
#ifdef APP_IOLINK_CHIP1_SPI
      {
         .name = "/iolink1/0",
         .mode = &mode_ch[2],
         .drv  = hw[1],
         .arg  = (void *)0,
      },
      {
         .name = "/iolink1/1",
         .mode = &mode_ch[3],
         .drv  = hw[1],
         .arg  = (void *)1,
      },
#endif
   };

   iolink_m_cfg_t app_cfg = {
      .cb_arg                   = NULL,
      .cb_smi                   = NULL,
      .cb_pd                    = NULL,
      .port_cnt                 = NELEMENTS (port_cfgs),
      .port_cfgs                = port_cfgs,
      .master_thread_prio       = APP_MASTER_THREAD_PRIO,
      .master_thread_stack_size = APP_MASTER_THREAD_STACK_SIZE,
      .dl_thread_prio           = APP_DL_THREAD_PRIO,
      .dl_thread_stack_size     = APP_DL_THREAD_STACK_SIZE,
   };

   os_usleep (200 * 1000);

   app_handler_thread = os_thread_create (
      "app_handler_thread",
      APP_HANDLER_THREAD_PRIO,
      APP_HANDLER_THREAD_STACK_SIZE,
      app_handler_thread_start,
      (void *)&app_cfg);
   CC_ASSERT (app_handler_thread != NULL);

   for (;;)
   {
      os_usleep (1000 * 1000);
   }

   return 0;
}
