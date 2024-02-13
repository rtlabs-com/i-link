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
#include <string.h>
#include "osal.h"
#include "osal_log.h"
#include "iolink.h"
#include "iolink_main.h"
#include "iolink_dl.h"
#include "iolink_handler.h"
#include "iolink_ifm.h"

#define EVENT_PD_0           BIT (0)
#define EVENT_COMLOST_0      BIT (8)
#define EVENT_PORTE_0        BIT (16)
#define EVENT_RETRY_ESTCOM_0 BIT (24)

#define VERIFY_ITEM(structure, item, portnumber, text)                         \
   if (structure->item != item)                                                \
   {                                                                           \
      LOG_WARNING (                                                            \
         LOG_STATE_ON,                                                         \
         "%s: Port %u: Expected %s 0x%x, got 0x%x\n",                          \
         __func__,                                                             \
         portnumber,                                                           \
         text,                                                                 \
         (unsigned int)item,                                                   \
         (unsigned int)structure->item);                                       \
      return 1;                                                                \
   }

iolink_app_master_ctx_t iolink_app_master;

static void SMI_cnf_cb (
   void * arg,
   uint8_t portnumber,
   iolink_arg_block_id_t ref_arg_block_id,
   uint16_t arg_block_len,
   arg_block_t * arg_block);

static uint8_t verify_smi_masterident (
   iolink_app_port_ctx_t * app_port,
   uint16_t vendorid,
   uint32_t masterid)
{
   arg_block_void_t arg_block_void;

   bzero (&arg_block_void, sizeof (arg_block_void_t));
   arg_block_void.arg_block.id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;

   iolink_error_t err = SMI_MasterIdentification_req (
      app_port->portnumber,
      IOLINK_ARG_BLOCK_ID_MASTERIDENT,
      sizeof (arg_block_void_t),
      (arg_block_t *)&arg_block_void);
   if (
      (err != IOLINK_ERROR_NONE) ||
      (wait_for_cnf (app_port, SMI_MASTERIDENT_CNF, 1000) !=
       IOLINK_SMI_ERRORTYPE_NONE))
   {
      return 1;
   }

   VERIFY_ITEM (app_port->app_master, vendorid, app_port->portnumber, "vendorid");
   VERIFY_ITEM (app_port->app_master, masterid, app_port->portnumber, "masterid");

   return 0;
}

static void iolink_app_init_port (
   iolink_app_port_ctx_t * app_port,
   iolink_m_cfg_t * m_cfg)
{
   iolink_port_t * port =
      iolink_get_port (iolink_app_master.master, app_port->portnumber);

   app_port->event_mtx  = os_mutex_create();
   app_port->status_mtx = os_mutex_create();
   app_port->pdout_mtx  = os_mutex_create();
   iolink_dl_instantiate (
      port,
      m_cfg->dl_thread_prio,
      m_cfg->dl_thread_stack_size);

   app_port->allocated = 1;
   app_port->event     = os_event_create();

   if (verify_smi_masterident (app_port, MASTER_VENDOR_ID, MASTER_ID) != 0)
   {
      LOG_WARNING (
         LOG_STATE_ON,
         "%s: Verify MasterIdentification failed\n",
         __func__);
   }
}

void iolink_common_config (
   arg_block_portconfiglist_t * port_cfg,
   uint16_t vid,
   uint32_t did,
   uint8_t cycle_time,
   iolink_portmode_t portmode,
   iolink_validation_check_t validation)
{
   port_cfg->arg_block.id                 = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;
   port_cfg->configlist.port_cycle_time   = cycle_time;
   port_cfg->configlist.vendorid          = vid;
   port_cfg->configlist.deviceid          = did;
   port_cfg->configlist.portmode          = portmode;
   port_cfg->configlist.validation_backup = validation;
   port_cfg->configlist.iq_behavior       = IOLINK_IQ_BEHAVIOR_NO_SUPPORT;
}

static uint8_t iolink_config_port_sdci_auto (iolink_app_port_ctx_t * app_port)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_common_config (
      &port_cfg,
      0,
      0,
      0 /* AFAP (As fast as possible) */,
      IOLINK_PORTMODE_IOL_AUTO,
      IOLINK_VALIDATION_CHECK_NO);

   app_port->app_port_state = IOL_STATE_STARTING;

   iolink_error_t err = SMI_PortConfiguration_req (
      app_port->portnumber,
      IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
      sizeof (arg_block_portconfiglist_t),
      (arg_block_t *)&port_cfg);
   if (
      (err != IOLINK_ERROR_NONE) ||
      (wait_for_cnf (app_port, SMI_PORTCFG_CNF, 1000) !=
       IOLINK_SMI_ERRORTYPE_NONE))
   {
      return 1;
   }

   return 0;
}

static uint8_t iolink_config_port_dido (iolink_app_port_ctx_t * app_port, bool di)
{
   arg_block_portconfiglist_t port_cfg;

   bzero (&port_cfg, sizeof (arg_block_portconfiglist_t));

   iolink_common_config (
      &port_cfg,
      0,
      0,
      0 /* AFAP (As fast as possible) */,
      (di) ? IOLINK_PORTMODE_DI_CQ : IOLINK_PORTMODE_DO_CQ,
      (di) ? IOLINK_IQ_BEHAVIOR_DI : IOLINK_IQ_BEHAVIOR_DO);

   iolink_error_t err = SMI_PortConfiguration_req (
      app_port->portnumber,
      IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
      sizeof (arg_block_portconfiglist_t),
      (arg_block_t *)&port_cfg);
   if (
      (err != IOLINK_ERROR_NONE) ||
      (wait_for_cnf (app_port, SMI_PORTCFG_CNF, 1000) !=
       IOLINK_SMI_ERRORTYPE_NONE))
   {
      return 1;
   }

   return 0;
}

static uint8_t iolink_config_port (
   iolink_app_port_ctx_t * app_port,
   iolink_pl_mode_t port_mode)
{
   uint8_t res = 0;

   switch (port_mode)
   {
   case iolink_mode_SDCI:
      res = iolink_config_port_sdci_auto (app_port);
      break;
   case iolink_mode_DO:
      res = iolink_config_port_dido (app_port, false);
      break;
   case iolink_mode_DI:
      res = iolink_config_port_dido (app_port, true);
      break;
   case iolink_mode_INACTIVE:
      break;
   }

   return res;
}

uint8_t get_port_status (iolink_app_port_ctx_t * app_port)
{
   arg_block_void_t arg_block_void;

   bzero (&arg_block_void, sizeof (arg_block_void_t));
   arg_block_void.arg_block.id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;

   iolink_error_t err = SMI_PortStatus_req (
      app_port->portnumber,
      IOLINK_ARG_BLOCK_ID_PORT_STATUS_LIST,
      sizeof (arg_block_void_t),
      (arg_block_t *)&arg_block_void);
   if (
      (err != IOLINK_ERROR_NONE) ||
      (wait_for_cnf (app_port, SMI_PORTSTATUS_CNF, 1000) !=
       IOLINK_SMI_ERRORTYPE_NONE))
   {
      return 1;
   }

   return 0;
}

static uint8_t iolink_start_port (iolink_app_port_ctx_t * app_port)
{
   iolink_app_port_status_t * port_status = &app_port->status;
   uint8_t portnumber                     = app_port->portnumber;

   if (get_port_status (app_port) != 0)
   {
      return 1;
   }

   os_mutex_lock (app_port->status_mtx);
   /* Verify port status data */
   if (port_status->port_status_info != IOLINK_PORT_STATUS_INFO_OP)
   {
      LOG_WARNING (
         LOG_STATE_ON,
         "%s: Port %u is not in state operational, port_status_info = %u\n",
         __func__,
         portnumber,
         port_status->port_status_info);
      os_mutex_unlock (app_port->status_mtx);

      return 1;
   }

   if (port_status->vendorid == IFM_VENDOR_ID)
   {
      switch (port_status->deviceid)
      {
      case IFM_RFID_DEVICE_ID:
         ifmrfid_setup (app_port);
         break;
      case IFM_HMI_DEVICE_ID:
         ifmHMI_setup (app_port);
         break;
      default:
         app_port->type           = UNKNOWN;
         app_port->app_port_state = IOL_STATE_RUNNING;
         LOG_WARNING (
            LOG_STATE_ON,
            "%s: Port %u: Unknown iolink device 0x%06x for VID 0x%04x\n",
            __func__,
            portnumber,
            (int)port_status->deviceid,
            port_status->vendorid);
         break;
      }
   }
   else
   {
      app_port->type = UNKNOWN;
      LOG_WARNING (
         LOG_STATE_ON,
         "%s: Port %u: Unknown device: Vendor ID = 0x%04X, Device ID = "
         "0x%06X\n",
         __func__,
         portnumber,
         port_status->vendorid,
         (int)port_status->deviceid);
   }

   LOG_INFO (LOG_STATE_ON, "%s: Port %u: Start done!\n", __func__, portnumber);
   os_mutex_unlock (app_port->status_mtx);

   return 0;
}

static void PD_cb (
   uint8_t portnumber,
   void * arg,
   uint8_t data_len,
   const uint8_t * inputdata)
{
   uint8_t port_index                   = portnumber - 1;
   iolink_app_master_ctx_t * app_master = (iolink_app_master_ctx_t *)arg;
   iolink_app_port_ctx_t * app_port = &iolink_app_master.app_port[port_index];

   os_event_set (app_master->app_event, EVENT_PD_0 << port_index);
   memcpy (app_port->pdin.data, inputdata, data_len);
   app_port->pdin.data_len = data_len;
}

static void iolink_retry_estcom (os_timer_t * tmr, void * arg)
{
   uint8_t port_idx = ((uintptr_t)arg) & 0xFF;

   iolink_app_master.app_port[port_idx].app_port_state =
      IOL_STATE_WU_RETRY_WAIT_TSD;
   os_event_set (iolink_app_master.app_event, EVENT_RETRY_ESTCOM_0 << port_idx);
}

void iolink_handler (iolink_m_cfg_t m_cfg)
{
   long unsigned int i;
   os_event_t * app_event = os_event_create();
   iolink_pl_mode_t port_mode[IOLINK_NUM_PORTS];
   os_timer_t * iolink_tsd_tmr[IOLINK_NUM_PORTS] = {NULL};

   for (i = 0; i < m_cfg.port_cnt; i++)
   {
      if (*m_cfg.port_cfgs[i].mode == iolink_mode_SDCI)
      {
         iolink_tsd_tmr[i] =
            os_timer_create (1000 * 1000, iolink_retry_estcom, (void *)i, true);
      }
   }

   bzero (port_mode, sizeof (port_mode));
   bzero (&iolink_app_master, sizeof (iolink_app_master));
   iolink_app_master.app_event = app_event;

   m_cfg.cb_arg = &iolink_app_master;
   m_cfg.cb_smi = SMI_cnf_cb;
   m_cfg.cb_pd  = PD_cb;

   iolink_m_t * master = iolink_m_init (&m_cfg);

   if (master == NULL)
   {
      LOG_ERROR (LOG_STATE_ON, "%s: iolink_m_init() failed!\n", __func__);
      CC_ASSERT (0);
   }

   iolink_app_master.master = master;

   for (i = 0; i < m_cfg.port_cnt; i++)
   {
      if (*m_cfg.port_cfgs[i].mode != iolink_mode_INACTIVE)
      {
         port_mode[i]                             = *m_cfg.port_cfgs[i].mode;
         iolink_app_master.app_port[i].portnumber = i + 1;
         iolink_app_master.app_port[i].app_master = &iolink_app_master;
         iolink_app_init_port (
            &iolink_app_master.app_port[i],
            &m_cfg);
      }

      /* Config allocated port */
      iolink_app_port_ctx_t * app_port = &iolink_app_master.app_port[i];

      if (app_port->allocated == 1)
      {
         iolink_get_port (iolink_app_master.master, app_port->portnumber);

         if (iolink_config_port (app_port, port_mode[i]) != 0)
         {
            LOG_WARNING (
               LOG_STATE_ON,
               "%s: Failed to config port %lu\n",
               __func__,
               i + 1);
         }
      }
   }

   while (true)
   {
      uint32_t event_value;

      if (!os_event_wait (iolink_app_master.app_event, 0xFFFFFFFF, &event_value, 1000))
      {
         os_event_clr (iolink_app_master.app_event, event_value);

         for (i = 0; i < m_cfg.port_cnt; i++)
         {
            iolink_app_port_ctx_t * app_port = &iolink_app_master.app_port[i];

            if (((EVENT_PD_0 << i) & event_value) != 0)
            {
               if (app_port->app_port_state == IOL_STATE_RUNNING)
               {
                  if (app_port->run_function != NULL)
                  {
                     app_port->run_function (app_port);
                  }
               }
            }

            if (((EVENT_PORTE_0 << i) & event_value) != 0)
            {
               if (app_port->app_port_state == IOL_STATE_STARTING)
               {
                  if (iolink_start_port (app_port) != 0)
                  {
                     LOG_WARNING (
                        LOG_STATE_ON,
                        "%s: Failed to start port %lu\n",
                        __func__,
                        i + 1);
                  }
               }
               else if (app_port->app_port_state != IOL_STATE_STOPPING)
               {
                  LOG_WARNING (
                     LOG_STATE_ON,
                     "%s: EVENT_PORT for port %lu, when in port_state %u\n",
                     __func__,
                     i + 1,
                     app_port->app_port_state);
               }
            }

            if (((EVENT_RETRY_ESTCOM_0 << i) & event_value) != 0)
            {
               if (app_port->app_port_state == IOL_STATE_WU_RETRY_WAIT_TSD)
               {
                  iolink_dl_reset (iolink_get_port (
                     iolink_app_master.master,
                     app_port->portnumber));
                  if (iolink_config_port (app_port, port_mode[i]) != 0)
                  {
                     LOG_WARNING (
                        LOG_STATE_ON,
                        "%s: Failed to config port %lu\n",
                        __func__,
                        i + 1);
                  }
               }
            }

            if (((EVENT_COMLOST_0 << i) & event_value) != 0)
            {
               os_timer_stop (iolink_tsd_tmr[i]);
               if (app_port->app_port_state == IOL_STATE_STARTING)
               {
                  /* Wait 500ms before sending new WURQ */
                  os_timer_set (iolink_tsd_tmr[i], 500 * 1000);
                  os_timer_start (iolink_tsd_tmr[i]);
               }
               else if (app_port->app_port_state != IOL_STATE_STOPPING)
               {
                  /* Send WURQ immediately */
                  iolink_retry_estcom (NULL, (void *)i);
               }
               else // (app_port->app_port_state == IOL_STATE_STOPPING)
               {
                  app_port->app_port_state = IOL_STATE_INACTIVE;
               }
            }
         }
      }
   }
}

static void handle_smi_deviceevent (
   iolink_app_port_ctx_t * app_port,
   arg_block_devevent_t * arg_block_devevent)
{
   if (app_port->type == GOLDEN)
   {
      int i;

      os_mutex_lock (app_port->event_mtx);
      for (i = 0; i < arg_block_devevent->event_count; i++)
      {
         uint8_t event_index  = app_port->events.count++;
         diag_entry_t * entry = &arg_block_devevent->diag_entry[i];

         CC_ASSERT (event_index < PORT_EVENT_COUNT);
         app_port->events.diag_entry[event_index].event_qualifier =
            entry->event_qualifier;
         app_port->events.diag_entry[event_index].event_code = entry->event_code;
      }
      os_mutex_unlock (app_port->event_mtx);
   }
}

static void handle_smi_portevent (
   iolink_app_port_ctx_t * app_port,
   diag_entry_t * event)
{
   uint8_t portnumber                   = app_port->portnumber;
   iolink_app_master_ctx_t * app_master = app_port->app_master;
   uint8_t port_index                   = portnumber - 1;

   if (
      ((app_port->type == GOLDEN) || (app_port->type == UNKNOWN)) &&
      event->event_code != IOLINK_EVENTCODE_NO_DEV)
   {
      uint8_t event_index;

      os_mutex_lock (app_port->event_mtx);
      event_index = app_port->events.count++;
      CC_ASSERT (event_index < PORT_EVENT_COUNT);
      app_port->events.diag_entry[event_index].event_qualifier =
         event->event_qualifier;
      app_port->events.diag_entry[event_index].event_code = event->event_code;
      os_mutex_unlock (app_port->event_mtx);
   }

   if (event->event_code != IOLINK_EVENTCODE_NO_DEV)
   {
      LOG_DEBUG (
         LOG_STATE_ON,
         "%s (%d): type = %d, event_code = 0x%04X, count = %d\n",
         __func__,
         portnumber,
         app_port->type,
         event->event_code,
         app_port->events.count);
   }

   switch (event->event_code)
   {
   case IOLINK_EVENTCODE_PORT_STATUS_CHANGE:
      /* Port status changed - Use SMI_PortStatus() for details */
      os_event_set (app_master->app_event, (EVENT_PORTE_0 << port_index));
      break;
   case IOLINK_EVENTCODE_BAD_DID:
   case IOLINK_EVENTCODE_BAD_VID:
      LOG_WARNING (
         LOG_STATE_ON,
         "%s: Port %u: portevent bad vid/did.\n",
         __func__,
         portnumber);
      /* Port status changed - Use SMI_PortStatus() for details */
      os_event_set (app_master->app_event, (EVENT_PORTE_0 << port_index));
      break;
   case IOLINK_EVENTCODE_NO_DEV: /* COMLOST */
      if (app_port->app_port_state != IOL_STATE_STARTING)
      {
         LOG_WARNING (
            LOG_STATE_ON,
            "%s: Port %u: portevent COMLOST\n",
            __func__,
            portnumber);
      }

      os_event_set (app_master->app_event, (EVENT_COMLOST_0 << port_index));
      break;
   case IOLINK_EVENTCODE_BACKUP_INCON:
   case IOLINK_EVENTCODE_BACKUP_INCON_SIZE:
   case IOLINK_EVENTCODE_BACKUP_INCON_ID:
   case IOLINK_EVENTCODE_BACKUP_INCON_UL:
   case IOLINK_EVENTCODE_BACKUP_INCON_DL:
      break;
   default:
      LOG_WARNING (
         LOG_STATE_ON,
         "%s: Port %u: Unknown SMI_PortEvent code 0x%04x\n",
         __func__,
         portnumber,
         event->event_code);
      break;
   }
}

static void handle_smi_joberror (
   iolink_app_port_ctx_t * app_port,
   iolink_arg_block_id_t ref_arg_block_id,
   arg_block_joberror_t * arg_block_err)
{
   app_port->errortype = arg_block_err->error;

   if (app_port->errortype == IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID)
   {
      LOG_WARNING (
         LOG_STATE_ON,
         "%s: Port %u: Read failed, buffer too small.\n",
         __func__,
         app_port->portnumber);
   }

   switch (ref_arg_block_id)
   {
   case IOLINK_ARG_BLOCK_ID_OD_RD:
      os_event_set (app_port->event, SMI_READ_CNF);
      break;
   case IOLINK_ARG_BLOCK_ID_OD_WR:
      os_event_set (app_port->event, SMI_WRITE_CNF);
      break;
   case IOLINK_ARG_BLOCK_ID_DS_DATA:
      os_event_set (app_port->event, SMI_PARSERV_TO_DS_CNF);
      break;
   default:
      LOG_WARNING (
         LOG_STATE_ON,
         "%s: Port %u: SMI failed! errortype = 0x%04x, ref = 0x%04x\n",
         __func__,
         app_port->portnumber,
         app_port->errortype,
         ref_arg_block_id);
      break;
   }
}

static void SMI_cnf_cb (
   void * arg,
   uint8_t portnumber,
   iolink_arg_block_id_t ref_arg_block_id,
   uint16_t arg_block_len,
   arg_block_t * arg_block)
{
   iolink_app_master_ctx_t * app_m  = arg;
   iolink_app_port_ctx_t * app_port = &app_m->app_port[portnumber - 1];

   bool match_found = true;

   CC_ASSERT (arg_block != NULL);

   switch (arg_block->id)
   {
   case IOLINK_ARG_BLOCK_ID_JOB_ERROR:
      handle_smi_joberror (
         app_port,
         ref_arg_block_id,
         (arg_block_joberror_t *)arg_block);
      LOG_DEBUG (LOG_STATE_ON, "%s: IOLINK_ARG_BLOCK_ID_JOB_ERROR\n", __func__);
      break;
   case IOLINK_ARG_BLOCK_ID_PORT_STATUS_LIST:
   {
      iolink_app_port_status_t * port_status = &app_port->status;
      arg_block_portstatuslist_t * port_status_list =
         (arg_block_portstatuslist_t *)arg_block;

      port_status->port_status_info  = port_status_list->port_status_info;
      port_status->port_quality_info = port_status_list->port_quality_info;
      port_status->revision_id       = port_status_list->revision_id;
      port_status->transmission_rate = port_status_list->transmission_rate;
      port_status->master_cycle_time = port_status_list->master_cycle_time;
      port_status->vendorid          = port_status_list->vendorid;
      port_status->deviceid          = port_status_list->deviceid;

      /* SMI_PortStatus_cnf */
      os_event_set (app_port->event, SMI_PORTSTATUS_CNF);
      break;
   }
   case IOLINK_ARG_BLOCK_ID_PORT_EVENT:;
      /* SMI_PortEvent_ind */
      arg_block_portevent_t * arg_block_portevent = (arg_block_portevent_t *)arg_block;
      LOG_DEBUG (LOG_STATE_ON, "%s: IOLINK_ARG_BLOCK_ID_PORT_EVENT\n", __func__);
      handle_smi_portevent (app_port, &arg_block_portevent->event);
      break;
   case IOLINK_ARG_BLOCK_ID_DEV_EVENT:
      /* SMI_DeviceEvent_ind */
      LOG_DEBUG (LOG_STATE_ON, "%s: IOLINK_ARG_BLOCK_ID_DEV_EVENT\n", __func__);
      handle_smi_deviceevent (app_port, (arg_block_devevent_t *)arg_block);
      break;
   case IOLINK_ARG_BLOCK_ID_OD_RD:
      /* SMI_DeviceRead_cnf */
      LOG_DEBUG (LOG_STATE_ON, "%s: IOLINK_ARG_BLOCK_ID_OD_RD\n", __func__);
      app_port->param_read.data_len = arg_block_len - sizeof (arg_block_od_t);
      os_event_set (app_port->event, SMI_READ_CNF);
      break;
   case IOLINK_ARG_BLOCK_ID_PD_IN:;
      /* SMI_PDIn_cnf */
      arg_block_pdin_t * arg_block_pdin = (arg_block_pdin_t *)arg_block;
      memcpy (app_port->pdin.data, arg_block_pdin->data, arg_block_pdin->h.len);
      app_port->pdin.data_len = arg_block_pdin->h.len;
      app_port->pdin.pqi      = arg_block_pdin->h.port_qualifier_info;
      break;
   case IOLINK_ARG_BLOCK_ID_PD_IN_OUT:
      /* SMI_PDInOut_cnf */
      LOG_DEBUG (LOG_STATE_ON, "%s: IOLINK_ARG_BLOCK_ID_PD_IN_OUT\n", __func__);
      break;
   case IOLINK_ARG_BLOCK_ID_MASTERIDENT:;
      /* SMI_MasterIdentification_cnf() */
      arg_block_masterident_t * arg_block_masterident = (arg_block_masterident_t *)arg_block;
      LOG_DEBUG (LOG_STATE_ON, "%s: IOLINK_ARG_BLOCK_ID_MASTERIDENT\n", __func__);
      app_m->vendorid = arg_block_masterident->h.vendorid;
      app_m->masterid = arg_block_masterident->h.masterid;
      os_event_set (app_port->event, SMI_MASTERIDENT_CNF);
      break;
   default:
      match_found = false;
      break;
   }

   if (!match_found)
   {
      switch (ref_arg_block_id)
      {
      case IOLINK_ARG_BLOCK_ID_PD_OUT:
         /* SMI_DeviceWrite_cnf */
         LOG_DEBUG (LOG_STATE_ON, "%s: IOLINK_ARG_BLOCK_ID_PD_OUT\n", __func__);
         break;
      case IOLINK_ARG_BLOCK_ID_DS_DATA:
         /* SMI_ParServToDS_cnf */
         LOG_DEBUG (LOG_STATE_ON, "%s: IOLINK_ARG_BLOCK_ID_DS_DATA\n", __func__);
         os_event_set (app_port->event, SMI_PARSERV_TO_DS_CNF);
         break;
      case IOLINK_ARG_BLOCK_ID_OD_WR:
         /* SMI_DeviceWrite_cnf */
         LOG_DEBUG (LOG_STATE_ON, "%s: IOLINK_ARG_BLOCK_ID_OD_WR\n", __func__);
         os_event_set (app_port->event, SMI_WRITE_CNF);
         break;
      case IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST:
         /* SMI_PortConfiguration_cnf */
         LOG_DEBUG (
            LOG_STATE_ON,
            "%s: IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST\n",
            __func__);
         os_event_set (app_port->event, SMI_PORTCFG_CNF);
         break;
      default:
         LOG_WARNING (
            LOG_STATE_ON,
            "%s: Port %u: Unexpected ref_arg_block_id 0x%04x and arg_block_id "
            "0x%04x\n",
            __func__,
            portnumber,
            ref_arg_block_id,
            arg_block->id);
         break;
      }
   }
}

iolink_smi_errortypes_t wait_for_cnf (
   iolink_app_port_ctx_t * app_port,
   uint32_t mask,
   uint32_t ms)
{
   uint32_t event_value;
   iolink_smi_errortypes_t errortype = IOLINK_SMI_ERRORTYPE_NONE;

   /* Wait for SMI_xxx_cnf() */
   if (os_event_wait (app_port->event, mask, &event_value, ms))
   {
      LOG_WARNING (
         LOG_STATE_ON,
         "%s: timeout for port %u (mask = 0x%X)\n",
         __func__,
         app_port->portnumber,
         (unsigned int)mask);

      return IOLINK_SMI_ERRORTYPE_APP_DEV; // TODO
   }

   os_event_clr (app_port->event, event_value);

   if ((mask == SMI_WRITE_CNF) || (mask == SMI_PARSERV_TO_DS_CNF))
   {
      errortype = app_port->errortype;

      if (errortype != IOLINK_SMI_ERRORTYPE_NONE)
      {
         app_port->errortype = IOLINK_SMI_ERRORTYPE_NONE;
      }
   }

   return errortype;
}
