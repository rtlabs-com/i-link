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

#ifndef IOLINK_MAX14819_H
#define IOLINK_MAX14819_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief API for initialization of the MAX14819 chip
 *
 */

#define MAX14819_CQCFG_CQFILTEREN    BIT(0)
#define MAX14819_CQCFG_DRVDIS        BIT(1)
#define MAX14819_CQCFG_PUSHPUL       BIT(2)
#define MAX14819_CQCFG_NPN           BIT(3)
#define MAX14819_CQCFG_SINKSEL(x)    (((x) & 0x03) << 4)
#define MAX14819_CQCFG_SINKSEL_MASK  MAX14819_CQCFG_SINKSEL (0x3)
#define MAX14819_CQCFG_SOURCESINK    BIT(6)
#define MAX14819_CQCFG_IEC3TH        BIT(7)

#define MAX14819_CLOCK_XTALEN        BIT (0)
#define MAX14819_CLOCK_EXTCLKEN      BIT (1)
#define MAX14819_CLOCK_CLKDIV(x)     (((x) & 0x03) << 2)
#define MAX14819_CLOCK_CLKDIV_MASK   MAX14819_CLOCK_CLKDIV (0x03)
#define MAX14819_CLOCK_CLKOEN        BIT (4)
#define MAX14819_CLOCK_EXTCLKMIS     BIT (5)
#define MAX14819_CLOCK_TXTXENDIS     BIT (6)
#define MAX14819_CLOCK_VCCWARNEN     BIT (7)

#define MAX14819_IOSTCFG_DICSINK     BIT (0)
#define MAX14819_IOSTCFG_DICSOURCE   BIT (1)
#define MAX14819_IOSTCFG_DIEC3TH     BIT (2)
#define MAX14819_IOSTCFG_DIFILTEREN  BIT (3)
#define MAX14819_IOSTCFG_TX          BIT (4)
#define MAX14819_IOSTCFG_TXEN        BIT (5)
#define MAX14819_IOSTCFG_CQLEVEL     BIT (6)
#define MAX14819_IOSTCFG_DILEVEL     BIT (7)

#define MAX14819_LPCNFG_LPEN         BIT (0)
#define MAX14819_LPCNFG_LPCLIMDIS    BIT (1)
#define MAX14819_LPCNFG_LPCL2X       BIT (2)
#define MAX14819_LPCNFG_LPBL(x)      (((x) & 0x03) << 3)
#define MAX14819_LPCNFG_LPBL_MASK    MAX14819_LPCNFG_BLA (0x03)
#define MAX14819_LPCNFG_LPDYNBL      BIT (5)
#define MAX14819_LPCNFG_LPRT(x)      (((x) & 0x03) << 6)
#define MAX14819_LPCNFG_LPRT_MASK    MAX14819_LPCNFG_BLA (0x03)

typedef struct iolink_14819_drv iolink_14819_drv_t;

/**
 * IO-Link MAX14819 driver configuration
 */
typedef struct iolink_14819_cfg
{
   /** SPI address of the transceiver */
   uint8_t chip_address;

   /** Identification of the SPI slave */
   const char * spi_slave_name;

   /** Initial value of the InterruptEn register */
   uint8_t IntE;

   /** Initial value of the CQCtrlA register */
   uint8_t CQCtrlA;

   /** Initial value of the CQCtrlB register */
   uint8_t CQCtrlB;

   /** Initial value of the LEDCtrl register */
   uint8_t LEDCtrl;

   /** Initial value of the CQCfgA register */
   uint8_t CQCfgA;

   /** Initial value of the CQCfgB register */
   uint8_t CQCfgB;

   /** Initial value of the LPCnfgA register */
   uint8_t LPCnfgA;

   /** Initial value of the LPCnfgB register */
   uint8_t LPCnfgB;

   /** Initial value of the IOStCfgA register */
   uint8_t IOStCfgA;

   /** Initial value of the IOStCfgB register */
   uint8_t IOStCfgB;

   /** Initial value of the DrvCurrLim register */
   uint8_t DrvCurrLim;

   /** Initial value of the Clock register */
   uint8_t Clock;

   /** Optional function to read chip registers from the application */
   void (*register_read_reg_fn) (void * read_reg_function);

} iolink_14819_cfg_t;

/**
 * Initialises a iolink_max14819 driver instance and registers
 * the instance.
 *
 * May only be called once per hardware block.
 *
 * @param cfg     Driver configuration.
 * @return        Driver handle or NULL on failure.
 */
iolink_hw_drv_t * iolink_14819_init (const iolink_14819_cfg_t * cfg);

/**
 * Interrupt service routine for the iolink_max14819 driver instance.
 *
 * @param arg     Reference to the driver instance
 */
void iolink_14819_isr (void * arg);

#ifdef __cplusplus
}
#endif

#endif /* IOLINK_MAX14819_H */
