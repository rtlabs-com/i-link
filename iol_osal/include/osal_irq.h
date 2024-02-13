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

#ifndef OSAL_PL_HW_IRQ_H
#define OSAL_PL_HW_IRQ_H

#ifdef __rtk__
#include "bsp.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*isr_func_t) (void *);

int _iolink_setup_int (int gpio_pin, isr_func_t isr_func, void* irq_arg);

#ifdef __cplusplus
}
#endif

#endif
