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
#include <stdlib.h>

#include "osal.h"
#include "osal_irq.h"
#include "osal_log.h"
#include "iolink.h"
#include "iolink_max14819.h"
#include "app_handler.h"

#define APP_MASTER_THREAD_STACK_SIZE  (4 * 1024)
#define APP_MASTER_THREAD_PRIO        6
#define APP_DL_THREAD_STACK_SIZE      1500
#define APP_DL_THREAD_PRIO            (APP_MASTER_THREAD_PRIO + 1)

static iolink_pl_mode_t mode_ch[] = {
   iolink_mode_SDCI,
   iolink_mode_INACTIVE,
};

int main (int argc, char ** argv)
{
   os_thread_t * app_handler_thread;
   iolink_hw_drv_t * hw;

   if (argc != 4)
   {
      printf ("usage: %s <spi> <irq> <chip address>\n", argv[0]);
      return -1;
   }

   char *  spi     = argv[1];
   int     irq     = atoi (argv[2]);
   int     address = atoi (argv[3]);

   iolink_14819_cfg_t iol_14819_0_cfg = {
      .chip_address   = address,
      .chip_irq       = irq,
      .spi_slave_name = spi,
      .CQCfgA         = MAX14819_CQCFG_DRVDIS | MAX14819_CQCFG_SINKSEL (0x2),
      .LPCnfgA        = MAX14819_LPCNFG_LPEN,
      .IOStCfgA       = MAX14819_IOSTCFG_DICSINK | MAX14819_IOSTCFG_DIEC3TH,
      .DrvCurrLim     = 0x00,
      .Clock          = MAX14819_CLOCK_XTALEN | MAX14819_CLOCK_TXTXENDIS,
   };

   hw = iolink_14819_init (&iol_14819_0_cfg);
   if (hw == NULL)
   {
      LOG_ERROR (IOLINK_APP_LOG, "APP: Failed to open driver\n");
      return -1;
   }

   iolink_port_cfg_t port_cfgs[] = {
      {
         .name = "/iolink0/0",
         .mode = &mode_ch[0],
         .drv  = hw,
         .arg  = (void *)0,
      },
      {
         .name = "/iolink0/1",
         .mode = &mode_ch[1],
         .drv  = hw,
         .arg  = (void *)1,
      },
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

   app_handler (app_cfg);
   return 0;
}
