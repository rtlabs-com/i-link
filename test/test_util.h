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

#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <gtest/gtest.h>
#include "mocks.h"
#include "options.h"
#include "iolink_main.h"

#include <string.h> /* memset */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(ar) (sizeof (ar) / sizeof (ar[0]))
#endif /* ARRAY_SIZE */

#define IOLINK_MASTER_THREAD_STACK_SIZE (4 * 1024)
#define IOLINK_MASTER_THREAD_PRIO       6
#define IOLINK_DL_THREAD_STACK_SIZE     1500
#define IOLINK_DL_THREAD_PRIO           (IOLINK_MASTER_THREAD_PRIO + 1)

typedef struct
{
   uint16_t eventcode;
   iolink_event_instance_t instance;
   iolink_event_mode_t mode;
   iolink_event_type_t type;
   iolink_event_source_t source;
} al_event_t;

class TestBase : public ::testing::Test
{
 protected:
   virtual void SetUp()
   {
      iolink_port_cfg_t port_cfgs[] = {
         {
            .name = "/ioltest1/0",
            .mode = NULL,
         },
         {
            .name = "/ioltest1/1",
            .mode = NULL,
         },
      };
      iolink_m_cfg_t m_cfg = {
         .cb_arg                   = NULL,
         .cb_smi                   = mock_SMI_cnf,
         .cb_pd                    = NULL,
         .port_cnt                 = NELEMENTS (port_cfgs),
         .port_cfgs                = port_cfgs,
         .master_thread_prio       = IOLINK_MASTER_THREAD_PRIO,
         .master_thread_stack_size = IOLINK_MASTER_THREAD_STACK_SIZE,
         .dl_thread_prio           = IOLINK_DL_THREAD_PRIO,
         .dl_thread_stack_size     = IOLINK_DL_THREAD_STACK_SIZE,
      };
      m = iolink_m_init (&m_cfg);

      portnumber = 1;
      port       = iolink_get_port (m, portnumber);

      /* Reset mock variables */
      mock_iolink_revisionid       = IOL_DIR_PARAM_REV_V11;
      mock_iolink_master_cycletime = 0;
      mock_iolink_min_cycletime    = 79;
      mock_iolink_vendorid         = 1171;
      mock_iolink_deviceid         = 0x112345;
      mock_iolink_functionid       = 0x999A;
      mock_iolink_trans_rate       = IOLINK_TRANSMISSION_RATE_COM1;

      mock_iolink_restart_device_cnt         = 0;
      mock_iolink_mastercmd_master_ident_cnt = 0;
      mock_iolink_mastercmd_preop_cnt        = 0;
      mock_iolink_write_devmode_inactive_cnt = 0;
      mock_iolink_write_devmode_operate_cnt  = 0;
      memset (mock_iolink_al_data, 0, sizeof (mock_iolink_al_data));
      mock_iolink_al_data_len        = 0;
      mock_iolink_al_data_index      = 0;
      mock_iolink_al_data_subindex   = 0;
      mock_iolink_al_read_cnf_cnt    = 0;
      mock_iolink_al_read_req_cnt    = 0;
      mock_iolink_al_write_cnf_cnt   = 0;
      mock_iolink_al_write_req_cnt   = 0;
      mock_iolink_al_control_req_cnt = 0;
      mock_iolink_al_control_ind_cnt = 0;
      memset (mock_iolink_dl_pdout_data, 0, sizeof (mock_iolink_dl_pdout_data));
      mock_iolink_dl_pdout_data_len = 0;
      memset (mock_iolink_dl_pdin_data, 0, sizeof (mock_iolink_dl_pdin_data));
      mock_iolink_dl_pdin_data_len          = 0;
      mock_iolink_dl_control_req_cnt        = 0;
      mock_iolink_dl_eventconf_req_cnt      = 0;
      mock_iolink_al_event_cnt              = 0;
      mock_iolink_sm_operate_cnt            = 0;
      mock_iolink_al_getinput_req_cnt       = 0;
      mock_iolink_al_getinputoutput_req_cnt = 0;
      mock_iolink_al_newinput_inf_cnt       = 0;
      mock_iolink_ds_delete_cnt             = 0;
      mock_iolink_ds_startup_cnt            = 0;
      mock_iolink_ds_ready_cnt              = 0;
      mock_iolink_ds_fault_cnt              = 0;
      mock_iolink_ds_fault                  = IOLINK_DS_FAULT_NONE;
      mock_iolink_od_start_cnt              = 0;
      mock_iolink_od_stop_cnt               = 0;
      mock_iolink_pd_start_cnt              = 0;
      mock_iolink_pd_stop_cnt               = 0;
      mock_iolink_sm_portmode               = IOLINK_SM_PORTMODE_INACTIVE;
      mock_iolink_al_read_errortype         = IOLINK_SMI_ERRORTYPE_NONE;
      mock_iolink_al_write_errortype        = IOLINK_SMI_ERRORTYPE_NONE;
      mock_iolink_controlcode               = IOLINK_CONTROLCODE_NONE;
      memset (mock_iolink_al_events, 0, sizeof (mock_iolink_al_events));
      memset (&mock_iolink_cfg_paraml, 0, sizeof (mock_iolink_cfg_paraml));

      mock_iolink_smi_cnf_cnt            = 0;
      mock_iolink_smi_joberror_cnt       = 0;
      mock_iolink_smi_portcfg_cnf_cnt    = 0;
      mock_iolink_smi_portstatus_cnf_cnt = 0;
      mock_iolink_smi_portevent_ind_cnt  = 0;
      mock_iolink_smi_arg_block_len      = 0;
      mock_iolink_smi_ref_arg_block_id   = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
      memset (mock_iolink_smi_arg_block, 0, sizeof (arg_block_t) + 64);
   }
   virtual void TearDown()
   {
      iolink_m_deinit (&m);
   }

   iolink_m_t * m = NULL;
   iolink_port_t * port;
   uint8_t portnumber;
};

inline std::string FormatHexInt (int value)
{
   std::stringstream ss;
   ss << std::hex << std::showbase << value;
   return ss.str();
}

inline std::string FormatByte (uint8_t value)
{
   std::stringstream ss;
   ss << std::setfill ('0') << std::setw (2) << std::hex << std::showbase
      << static_cast<unsigned int> (value);
   return ss.str();
}

template <typename T>
::testing::AssertionResult ArraysMatchN (
   const T (*expected),
   const T (*actual),
   const size_t size)
{
   for (size_t i (0); i < size; ++i)
   {
      if (expected[i] != actual[i])
      {
         return ::testing::AssertionFailure()
                << "actual[" << i << "] ("
                << FormatByte (static_cast<int> (actual[i])) << ") != expected["
                << i << "] (" << FormatByte (static_cast<int> (expected[i]))
                << ")";
      }
   }

   return ::testing::AssertionSuccess();
}

template <typename T, size_t size>
::testing::AssertionResult ArraysMatch (
   const T (&expected)[size],
   const T (&actual)[size])
{
   for (size_t i (0); i < size; ++i)
   {
      if (expected[i] != actual[i])
      {
         return ::testing::AssertionFailure()
                << "actual[" << i << "] ("
                << FormatByte (static_cast<int> (actual[i])) << ") != expected["
                << i << "] (" << FormatByte (static_cast<int> (expected[i]))
                << ")";
      }
   }

   return ::testing::AssertionSuccess();
}
inline ::testing::AssertionResult EventMatch (
   al_event_t * e1,
   diag_entry_t * diag,
   uint8_t event_cnt)
{
   int i;
   al_event_t e;
   al_event_t * e2 = &e;

   for (i = 0; i < event_cnt; i++)
   {
      e2->eventcode = diag[i].event_code;
      e2->instance  = (iolink_event_instance_t) (diag[i].event_qualifier & 0x7);
      e2->source = (iolink_event_source_t) ((diag[i].event_qualifier >> 3) & 0x1);
      e2->type   = (iolink_event_type_t) ((diag[i].event_qualifier >> 4) & 0x3);
      e2->mode   = (iolink_event_mode_t) ((diag[i].event_qualifier >> 6) & 0x3);
      if (e1->eventcode != e2->eventcode)
         return ::testing::AssertionFailure()
                << i << ": eventcode: (" << e2->eventcode << ") != expected ("
                << e1->eventcode << ")";
      if (e1->instance != e2->instance)
         return ::testing::AssertionFailure()
                << i << ": instance: (" << +e2->instance << ") != expected ("
                << +e1->instance << ")";
      if (e1->mode != e2->mode)
         return ::testing::AssertionFailure()
                << i << ": mode: (" << +e2->mode << ") != expected ("
                << +e1->mode << ")";
      if (e1->type != e2->type)
         return ::testing::AssertionFailure()
                << i << ": type: (" << +e2->type << ") != expected ("
                << +e1->type << ")";
      if (e1->source != e2->source)
         return ::testing::AssertionFailure()
                << i << ": source: (" << e2->source << ") != expected ("
                << e1->source << ")";
      e1++;
   }

   return ::testing::AssertionSuccess();
}

inline ::testing::AssertionResult PortConfigMatch (
   const portconfiglist_t * expected,
   const portconfiglist_t * actual)
{
   if (expected->portmode != actual->portmode)
      return ::testing::AssertionFailure()
             << "portmode (" << actual->portmode << ") != expected ("
             << expected->portmode << ")";
   if (expected->validation_backup != actual->validation_backup)
      return ::testing::AssertionFailure()
             << "validation_backup (" << actual->validation_backup
             << ") != expected (" << expected->validation_backup << ")";
   if (expected->iq_behavior != actual->iq_behavior)
      return ::testing::AssertionFailure()
             << "iq_behavior (" << actual->iq_behavior << ") != expected ("
             << expected->iq_behavior << ")";
   if (expected->port_cycle_time != actual->port_cycle_time)
      return ::testing::AssertionFailure()
             << "port_cycle_time (" << actual->port_cycle_time
             << ") != expected (" << expected->port_cycle_time << ")";
   if (expected->vendorid != actual->vendorid)
      return ::testing::AssertionFailure()
             << "vendorid (" << actual->vendorid << ") != expected ("
             << expected->vendorid << ")";
   if (expected->deviceid != actual->deviceid)
      return ::testing::AssertionFailure()
             << "deviceid (" << actual->deviceid << ") != expected ("
             << expected->deviceid << ")";
   if (expected->in_buffer_len != actual->in_buffer_len)
      return ::testing::AssertionFailure()
             << "in_buffer_len (" << actual->in_buffer_len << ") != expected ("
             << expected->in_buffer_len << ")";
   if (expected->out_buffer_len != actual->out_buffer_len)
      return ::testing::AssertionFailure()
             << "out_buffer_len (" << actual->out_buffer_len
             << ") != expected (" << expected->out_buffer_len << ")";

   return ::testing::AssertionSuccess();
}

#endif /* TEST_UTIL_H */
