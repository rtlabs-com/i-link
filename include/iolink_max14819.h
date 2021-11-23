/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * http://www.rt-labs.com
 * Copyright 2019 rt-labs AB, Sweden.
 * See LICENSE file in the project root for full license information.
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

typedef struct iolink_14819_drv iolink_14819_drv_t;

/**
 * IO-Link MAX14819 driver configuration
 */
typedef struct iolink_14819_cfg
{
   uint8_t chip_address;
   const char * spi_slave_name;
   uint8_t IntE;
   uint8_t CQCtrlA, CQCtrlB;
   uint8_t LEDCtrl;
   uint8_t CQCfgA, CQCfgB;
   uint8_t LPCnfgA, LPCnfgB;
   uint8_t IOStCfgA, IOStCfgB;
   uint8_t DrvCurrLim;
   uint8_t Clock;
   unsigned int pin[2];
   void (*register_read_reg_fn) (void * read_reg_function);
} iolink_14819_cfg_t;

/**
 * Initialises a iolink_max14819 driver instance and registers
 * the instance as @a name.
 *
 * May only be called once per hardware block.
 *
 * @param name    Driver instance name.  When using rt-kernel, this must be a
 *                pointer to a static memory area (in order to save memory,
 *                rt-kernel does not make a local copy of the name).
 * @param cfg     Driver configuration.
 * @return        Driver handle.
 */
#ifdef __rtk__
#include <dev.h> /* drv_t */
drv_t * iolink_14819_init (const char * name, const iolink_14819_cfg_t * cfg);
#else
int iolink_14819_init (const char * name, const iolink_14819_cfg_t * cfg);
#endif /* __rtk__ */

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
