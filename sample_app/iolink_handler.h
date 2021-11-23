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

#ifndef IOLINK_HANDLER_H
#define IOLINK_HANDLER_H

#include "sys/osal_sys.h"
#include "iolink.h"

#define MASTER_VENDOR_ID 1171
#define MASTER_ID        123

typedef enum
{
   NONE,
   IFM_LAMP,
   IFM_RFID,
   IFM_SENS,
   IFM_HMI,
   GOLDEN,
   UNKNOWN,
} iolink_device_type_t;

/* Port events for app_port.event */
typedef enum
{
   SMI_READ_CNF          = 0x01,
   SMI_WRITE_CNF         = 0x02,
   SMI_PORTCFG_CNF       = 0x04,
   SMI_PORTSTATUS_CNF    = 0x08,
   SMI_MASTERIDENT_CNF   = 0x10,
   SMI_PARSERV_TO_DS_CNF = 0x20,
} iolink_app_port_event_t;

typedef enum
{
   IOL_STATE_INACTIVE = 0,
   IOL_STATE_STARTING,
   IOL_STATE_STOPPING,
   IOL_STATE_RUNNING,
   IOL_STATE_WU_RETRY_WAIT_TSD,
} iolink_app_port_state_t;

typedef struct iolink_app_portstatus
{
   iolink_port_status_info_t port_status_info;
   iolink_port_quality_info_t port_quality_info;
   uint8_t revision_id;
   iolink_transmission_rate_t transmission_rate;
   uint8_t master_cycle_time;
   uint16_t vendorid;
   uint32_t deviceid;
} iolink_app_port_status_t;

#define PORT_EVENT_COUNT 10

typedef struct iolink_app_master_ctx iolink_app_master_ctx_t;
typedef struct iolink_app_port_ctx iolink_app_port_ctx_t;

typedef struct iolink_app_port_ctx
{
   iolink_app_master_ctx_t * app_master;
   uint8_t portnumber;
   iolink_device_type_t type;
   os_event_t * event;
   uint8_t allocated;
   iolink_app_port_state_t app_port_state;
   iolink_app_port_status_t status;
   os_mutex_t * status_mtx;
   iolink_smi_errortypes_t errortype;

   struct
   {
      uint8_t count;
      diag_entry_t diag_entry[PORT_EVENT_COUNT];
      iolink_eventcode_t eventcode[PORT_EVENT_COUNT];
   } events;
   os_mutex_t * event_mtx;
   struct
   {
      iolink_port_qualifier_info_t pqi;
      uint8_t data[IOLINK_PD_MAX_SIZE];
      uint8_t data_len;
   } pdin;
   struct
   {
      uint16_t data_len;
   } param_read;
   os_mutex_t * pdout_mtx;
   void (*run_function) (iolink_app_port_ctx_t * app_port);
#ifdef __rtk__
   bool alarm_active;
#endif
} iolink_app_port_ctx_t;

typedef struct iolink_app_master_ctx
{
   iolink_m_t * master;
   iolink_app_port_ctx_t app_port[IOLINK_NUM_PORTS];
   os_event_t * app_event;
   uint16_t vendorid;
   uint32_t masterid;
} iolink_app_master_ctx_t;

iolink_app_master_ctx_t iolink_app_master;

void iolink_handler (iolink_m_cfg_t m_cfg);

iolink_smi_errortypes_t wait_for_cnf (
   iolink_app_port_ctx_t * app_port,
   uint32_t mask,
   uint32_t ms);

uint8_t get_port_status (iolink_app_port_ctx_t * app_port);

#endif // IOLINK_HANDLER_H
