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

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*isr_func_t) (void *);

/**
 * A function that creates an interrupt handler thread, which will call the
 * isr_func when a falling edge is detected on the GPIO pin.
 *
 * @note In USB mode, the GPIO pin value does not matter, instead the interrupt
 * handler is setup to poll a pre defined pin on the FT2232H chip.
 * @note In USB mode, state of the GPIO pin is polled and isr_func is called
 * when GPIO pin is detected as low.
 *
 * @param gpio_pin  In: The GPIO pin that is connected to the IRQ pin
 * @param isr_func  In: The function that will be called when the interrupt gets
 * triggered
 * @param irq_arg   In: Pointer to the iolink_hw_drv_t structure containing
 * thread parameter
 */
int _iolink_setup_int (int gpio_pin, isr_func_t isr_func, void * irq_arg);

#ifdef __cplusplus
}
#endif

#endif
