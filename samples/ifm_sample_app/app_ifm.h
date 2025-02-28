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

#ifndef APP_IFM_H
#define APP_IFM_H

#include "app_handler.h"

#define IFM_VENDOR_ID 0x0136

#define IFM_RFID_DEVICE_ID 0x03C7
#define IFM_HMI_DEVICE_ID  0x02A9

void app_ifm_rfid_setup (app_port_ctx_t * app_port);

void app_ifm_hmi_setup (app_port_ctx_t * app_port);

#endif // APP_IFM_H
