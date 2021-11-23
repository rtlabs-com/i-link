/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * http://www.rt-labs.com
 * Copyright 2021 rt-labs AB, Sweden.
 * See LICENSE file in the project root for full license information.
 ********************************************************************/

#ifndef IOLINK_IFM_H
#define IOLINK_IFM_H

#include "iolink_handler.h"

#define IFM_VENDOR_ID 0x0136

#define IFM_RFID_DEVICE_ID 0x03C7
#define IFM_HMI_DEVICE_ID  0x02A9

void ifmrfid_setup (iolink_app_port_ctx_t * app_port);

void ifmHMI_setup (iolink_app_port_ctx_t * app_port);

#endif // IOLINK_IFM_H
