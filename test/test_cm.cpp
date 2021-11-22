/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2019 rt-labs AB, Sweden.
 * See LICENSE file in the project root for full license information.
 ********************************************************************/

#include "options.h"
#include "osal.h"
#include <gtest/gtest.h>

#include "mocks.h"
#include "iolink_cm.h"
#include "test_util.h"

// Test fixture

class CMTest : public TestBase
{
protected:
   // Override default setup
   virtual void SetUp() {
      TestBase::SetUp(); // Re-use default setup
   };
};

static inline iolink_cm_state_t cm_get_state (iolink_port_t * port)
{
   iolink_cm_port_t * cm = iolink_get_cm_ctx (port);

   return cm->state;
}

static inline void cm_verify_paraml (uint8_t exp_cycletime,
                                     iolink_sm_target_mode_t exp_mode,
                                     uint8_t exp_revisionid,
                                     iolink_inspectionlevel_t exp_inspectionl,
                                     uint16_t exp_vendorid,
                                     uint32_t exp_deviceid)
{
   EXPECT_EQ (exp_cycletime, mock_iolink_cfg_paraml.cycletime);
   EXPECT_EQ (exp_mode, mock_iolink_cfg_paraml.mode);
   EXPECT_EQ (exp_revisionid, mock_iolink_cfg_paraml.revisionid);
   EXPECT_EQ (exp_inspectionl, mock_iolink_cfg_paraml.inspectionlevel);
   EXPECT_EQ (exp_vendorid, mock_iolink_cfg_paraml.vendorid);
   EXPECT_EQ (exp_deviceid, mock_iolink_cfg_paraml.deviceid);
}

static inline void cm_verify_mock_cnt (uint8_t exp_sm_operate_cnt,
                                       uint8_t exp_ds_startup_cnt,
                                       uint8_t exp_ds_delete_cnt,
                                       uint8_t exp_od_start_cnt,
                                       uint8_t exp_od_stop_cnt,
                                       uint8_t exp_pd_start_cnt,
                                       uint8_t exp_pd_stop_cnt)
{
   EXPECT_EQ (exp_sm_operate_cnt, mock_iolink_sm_operate_cnt);
   EXPECT_EQ (exp_ds_startup_cnt, mock_iolink_ds_startup_cnt);
   EXPECT_EQ (exp_ds_delete_cnt, mock_iolink_ds_delete_cnt);
   EXPECT_EQ (mock_iolink_od_start_cnt, exp_od_start_cnt);
   EXPECT_EQ (mock_iolink_od_stop_cnt, exp_od_stop_cnt);
   EXPECT_EQ (mock_iolink_pd_start_cnt, exp_pd_start_cnt);
   EXPECT_EQ (mock_iolink_pd_stop_cnt, exp_pd_stop_cnt);
}

static inline void cm_verify_smi_portcfg_cnf (uint8_t exp_smi_portcfg_cnf_cnt)
{
   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);
   EXPECT_EQ (IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST,
              mock_iolink_smi_ref_arg_block_id);
   iolink_arg_block_id_t arg_block_id =
						mock_iolink_smi_arg_block->void_block.arg_block_id;
   EXPECT_EQ (arg_block_id, IOLINK_ARG_BLOCK_ID_VOID_BLOCK);
}

static inline void cm_verify_smi_err (iolink_arg_block_id_t exp_smi_ref_arg_id,
                                      iolink_arg_block_id_t exp_smi_exp_arg_id,
                                      iolink_smi_errortypes_t exp_errortype,
                                      uint8_t exp_smi_joberror_cnt,
                                      uint8_t exp_smi_portstatus_cnf_cnt,
                                      uint8_t exp_smi_portcfg_cnf_cnt)
{
   EXPECT_EQ (exp_smi_joberror_cnt, mock_iolink_smi_joberror_cnt);
   EXPECT_EQ (exp_smi_portstatus_cnf_cnt, mock_iolink_smi_portstatus_cnf_cnt);
   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);
   EXPECT_EQ (exp_smi_ref_arg_id, mock_iolink_smi_ref_arg_block_id);

   iolink_arg_block_id_t arg_block_id =
						mock_iolink_smi_arg_block->void_block.arg_block_id;

   EXPECT_EQ (IOLINK_ARG_BLOCK_ID_JOB_ERROR, arg_block_id);

   if (arg_block_id == IOLINK_ARG_BLOCK_ID_JOB_ERROR)
   {
      arg_block_joberror_t * job_error =
                           (arg_block_joberror_t*)mock_iolink_smi_arg_block;
      iolink_smi_errortypes_t error = job_error->error;
      iolink_arg_block_id_t exp_arg_block_id = job_error->exp_arg_block_id;

      EXPECT_EQ (exp_errortype, error);
      EXPECT_EQ (exp_smi_exp_arg_id, exp_arg_block_id);
   }
}

static inline void cm_verify_smi_masterident (
                                          iolink_arg_block_id_t exp_ref_arg_id,
                                          iolink_arg_block_id_t exp_exp_arg_id,
                                          uint16_t exp_vendorid,
                                          uint32_t exp_masterid,
                                          iolink_master_type_t exp_master_type,
                                          uint8_t exp_max_number_of_ports,
                                          iolink_port_types_t exp_port_type,
                                          uint8_t exp_smi_cnf_cnt,
                                          uint8_t exp_smi_joberror_cnt)
{
   EXPECT_EQ (exp_smi_cnf_cnt, mock_iolink_smi_cnf_cnt);
   EXPECT_EQ (exp_smi_joberror_cnt, mock_iolink_smi_joberror_cnt);

   EXPECT_EQ (exp_exp_arg_id, IOLINK_ARG_BLOCK_ID_MASTERIDENT);
   if (mock_iolink_smi_arg_block->void_block.arg_block_id ==
                                                IOLINK_ARG_BLOCK_ID_MASTERIDENT)
   {
      int i;
      arg_block_masterident_t * masterident =
                           (arg_block_masterident_t *)mock_iolink_smi_arg_block;
      uint8_t exp_arg_block_len = sizeof(arg_block_masterident_head_t) +
                        sizeof(iolink_port_types_t) * exp_max_number_of_ports;

      EXPECT_EQ (exp_vendorid, masterident->h.vendorid);
      EXPECT_EQ (exp_masterid, masterident->h.masterid);
      EXPECT_EQ (exp_master_type, masterident->h.master_type);
      EXPECT_EQ (0, masterident->h.features_1);
      EXPECT_EQ (0, masterident->h.features_2);
      EXPECT_EQ (exp_max_number_of_ports, masterident->h.max_number_of_ports);

      EXPECT_EQ (exp_arg_block_len, mock_iolink_smi_arg_block_len);

      for (i = 0; i < masterident->h.max_number_of_ports; i++)
      {
         EXPECT_EQ (exp_port_type, masterident->port_types[i]);
      }
   }
   else
   {
      CC_ASSERT (0);
   }
}

static inline void cm_verify_smi_portconfig (iolink_port_t * port,
                                             portconfiglist_t * exp_configlist)
{
   arg_block_void_t arg_block_void;
   iolink_arg_block_id_t arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_exp_arg_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;

   uint8_t exp_smi_cnf_cnt = mock_iolink_smi_cnf_cnt;
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt;
   memset (&arg_block_void, 0, sizeof(arg_block_void_t));
   arg_block_void.arg_block_id = arg_block_id;

   /* SMI_ReadbackPortConfiguration_req() */
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_ReadbackPortConfiguration_req (iolink_get_portnumber (port),
                                              exp_exp_arg_id,
                                              sizeof(arg_block_void_t),
                                              (arg_block_t *)&arg_block_void));
   mock_iolink_job.callback (&mock_iolink_job);
   exp_smi_cnf_cnt++;

   EXPECT_EQ (exp_smi_cnf_cnt, mock_iolink_smi_cnf_cnt);
   EXPECT_EQ (exp_smi_joberror_cnt, mock_iolink_smi_joberror_cnt);

   arg_block_id = mock_iolink_smi_arg_block->void_block.arg_block_id;

   EXPECT_EQ (exp_exp_arg_id, arg_block_id);

   if (mock_iolink_smi_arg_block->void_block.arg_block_id ==
                                                IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST)
   {
      arg_block_portconfiglist_t * arg_block_portcfg =
                     (arg_block_portconfiglist_t *) mock_iolink_smi_arg_block;

      EXPECT_TRUE (PortConfigMatch (exp_configlist,
                                    &arg_block_portcfg->configlist));
   }
   else
   {
      CC_ASSERT (0);
   }
}

static inline void cm_verify_portstatus (iolink_port_t * port,
                                    iolink_port_status_info_t exp_port_status,
                                    uint8_t exp_port_quality_info,
                                    uint8_t exp_revision_id,
                                    iolink_transmission_rate_t exp_trans_rate,
                                    uint8_t exp_master_cycle_time,
                                    uint16_t exp_vendorid,
                                    uint32_t exp_deviceid,
                                    uint8_t exp_number_of_diags,
                                    diag_entry_t * exp_diag_entries)
{
   arg_block_void_t arg_block_void;
   iolink_arg_block_id_t exp_exp_arg_block_id =
                                          IOLINK_ARG_BLOCK_ID_PORT_STATUS_LIST;
   iolink_arg_block_id_t exp_ref_arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   uint8_t exp_smi_portstatus_cnf_cnt = mock_iolink_smi_portstatus_cnf_cnt + 1;

   memset (&arg_block_void, 0, sizeof(arg_block_void_t));
   arg_block_void.arg_block_id = exp_ref_arg_block_id;

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortStatus_req (iolink_get_portnumber (port),
                                  exp_exp_arg_block_id,
                                  sizeof(arg_block_void_t),
                                  (arg_block_t *)&arg_block_void));
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (exp_smi_portstatus_cnf_cnt, mock_iolink_smi_portstatus_cnf_cnt);
   EXPECT_EQ (exp_ref_arg_block_id, mock_iolink_smi_ref_arg_block_id);

   iolink_arg_block_id_t arg_block_id =
						mock_iolink_smi_arg_block->void_block.arg_block_id;
   EXPECT_EQ (exp_exp_arg_block_id, arg_block_id);

   if (exp_exp_arg_block_id ==
       mock_iolink_smi_arg_block->void_block.arg_block_id)
   {
      arg_block_portstatuslist_t * port_status_list =
                     (arg_block_portstatuslist_t *)mock_iolink_smi_arg_block;

      EXPECT_EQ (exp_port_status, port_status_list->port_status_info);
      EXPECT_EQ (exp_port_quality_info, port_status_list->port_quality_info);
      EXPECT_EQ (exp_revision_id, port_status_list->revision_id);
      EXPECT_EQ (exp_trans_rate, port_status_list->transmission_rate);
      EXPECT_EQ (exp_master_cycle_time, port_status_list->master_cycle_time);
      EXPECT_EQ (exp_vendorid, port_status_list->vendorid);
      EXPECT_EQ (exp_deviceid, port_status_list->deviceid);
      EXPECT_EQ (exp_number_of_diags, port_status_list->number_of_diags);
   }
}

static inline void cm_x_to_dido (iolink_port_t * port,
                                 iolink_cm_state_t exp_state, bool is_do)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_sm_target_mode_t exp_target_mode;
   iolink_inspectionlevel_t exp_inspection_level =
                                          IOLINK_INSPECTIONLEVEL_NO_CHECK;
   iolink_port_status_info_t exp_status_info;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   uint8_t exp_port_cycle_time = 0;
   uint8_t exp_revisionid = 0;
   iolink_transmission_rate_t exp_trans_rate =
                                          IOLINK_TRANSMISSION_RATE_NOT_DETECTED;
   uint16_t exp_vendorid = 0;
   uint32_t exp_deviceid = 0;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;
   uint8_t exp_smi_portcfg_cnf_cnt = mock_iolink_smi_portcfg_cnf_cnt + 1;
   uint8_t exp_od_start_cnt = mock_iolink_od_start_cnt;
   uint8_t exp_od_stop_cnt = mock_iolink_od_stop_cnt + 1;
   uint8_t exp_pd_start_cnt = mock_iolink_pd_start_cnt;
   uint8_t exp_pd_stop_cnt = mock_iolink_pd_stop_cnt + 1;

   iolink_sm_portmode_t portmode;

   memset (&port_cfg, 0, sizeof(arg_block_portconfiglist_t));
   port_cfg.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;
   port_cfg.configlist.port_cycle_time = exp_port_cycle_time;
   port_cfg.configlist.vendorid = exp_vendorid;
   port_cfg.configlist.deviceid = exp_deviceid;
   port_cfg.configlist.in_buffer_len = 0; // TODO
   port_cfg.configlist.out_buffer_len = 0; // TODO
   port_cfg.configlist.validation_backup = IOLINK_VALIDATION_CHECK_NO;

   if (is_do)
   {
      port_cfg.configlist.portmode = IOLINK_PORTMODE_DO_CQ;
      port_cfg.configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_DO;
      exp_target_mode = IOLINK_SMTARGET_MODE_DO;
      exp_status_info = IOLINK_PORT_STATUS_INFO_DO;
      portmode = IOLINK_SM_PORTMODE_DO;
   }
   else
   {
      port_cfg.configlist.portmode = IOLINK_PORTMODE_DI_CQ;
      port_cfg.configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_DI;
      exp_target_mode = IOLINK_SMTARGET_MODE_DI;
      exp_status_info = IOLINK_PORT_STATUS_INFO_DI;
      portmode = IOLINK_SM_PORTMODE_DI;
   }

   EXPECT_EQ (exp_state, cm_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);

   SM_PortMode_ind (port, portmode);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_Port_DIDO, cm_get_state (port));

   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);
   cm_verify_mock_cnt (exp_sm_operate_cnt, exp_ds_startup_cnt,
                       exp_ds_delete_cnt, exp_od_start_cnt, exp_od_stop_cnt,
                       exp_pd_start_cnt, exp_pd_stop_cnt);
}

static inline void cm_deactive_to_dido (iolink_port_t * port,
                                        bool is_do)
{
   cm_x_to_dido (port, CM_STATE_Port_Deactivated, is_do);
}

static inline void cm_deactive_to_startup (iolink_port_t * port,
                                           iolink_portmode_t mode,
                                           iolink_validation_check_t validation)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_sm_target_mode_t exp_target_mode;
   iolink_inspectionlevel_t exp_inspection_level;
   uint8_t exp_param_revisionid;
   iolink_port_status_info_t exp_status_info =
                                          IOLINK_PORT_STATUS_INFO_DEACTIVATED;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   uint8_t exp_port_cycle_time = 0; // AFAP (As fast as possible)
   uint8_t exp_cfg_revid = 0;
   iolink_transmission_rate_t exp_trans_rate =
                                          IOLINK_TRANSMISSION_RATE_NOT_DETECTED;
   uint16_t exp_vendorid = 0;
   uint32_t exp_deviceid = 0;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_smi_portcfg_cnf_cnt = mock_iolink_smi_portcfg_cnf_cnt + 1;
   uint8_t exp_od_start_cnt = mock_iolink_od_start_cnt;
   uint8_t exp_od_stop_cnt = mock_iolink_od_stop_cnt + 1;
   uint8_t exp_pd_start_cnt = mock_iolink_pd_start_cnt;
   uint8_t exp_pd_stop_cnt = mock_iolink_pd_stop_cnt + 1;

   memset (&port_cfg, 0, sizeof(arg_block_portconfiglist_t));
   port_cfg.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;
   port_cfg.configlist.port_cycle_time = exp_port_cycle_time;
   port_cfg.configlist.vendorid = exp_vendorid;
   port_cfg.configlist.deviceid = exp_deviceid;
   port_cfg.configlist.in_buffer_len = 0; // TODO
   port_cfg.configlist.out_buffer_len = 0; // TODO
   port_cfg.configlist.validation_backup = validation;
   port_cfg.configlist.portmode = mode;
   port_cfg.configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_NO_SUPPORT;

   if (mode == IOLINK_PORTMODE_IOL_AUTO)
   {
      //port_cfg.configlist.validation_backup = IOLINK_VALIDATION_CHECK_NO;

      exp_target_mode = IOLINK_SMTARGET_MODE_AUTOCOM;
      exp_inspection_level = IOLINK_INSPECTIONLEVEL_NO_CHECK;
      exp_param_revisionid = 0;
      exp_ds_delete_cnt++;
   }
   else if (mode == IOLINK_PORTMODE_IOL_MAN)
   {
      if (validation == IOLINK_VALIDATION_CHECK_V10)
      {
         exp_param_revisionid = IOL_DIR_PARAM_REV_V10;
         exp_inspection_level = IOLINK_INSPECTIONLEVEL_IDENTICAL;
      }
      else
      {
         exp_param_revisionid = IOL_DIR_PARAM_REV_V11;
         exp_inspection_level = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
      }
      //port_cfg.configlist.portmode = IOLINK_PORTMODE_IOL_MAN;

      exp_target_mode = IOLINK_SMTARGET_MODE_CFGCOM;
      // TODO if vid != DS_vid && did != DS_did --> exp_ds_delete_cnt++;
      exp_ds_delete_cnt++;
   }

   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));

   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_param_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_cfg_revid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);
   cm_verify_mock_cnt (exp_sm_operate_cnt, exp_ds_startup_cnt,
                       exp_ds_delete_cnt, exp_od_start_cnt, exp_od_stop_cnt,
                       exp_pd_start_cnt, exp_pd_stop_cnt);
}

static inline void cm_startup_to_ds_parammanager (iolink_port_t * port)
{
   iolink_port_status_info_t exp_status_info = IOLINK_PORT_STATUS_INFO_PREOP;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   iolink_transmission_rate_t exp_trans_rate = mock_iolink_trans_rate;
   uint8_t exp_port_cycle_time = mock_iolink_min_cycletime;
   uint8_t exp_revisionid = mock_iolink_revisionid;
   uint16_t exp_vendorid = mock_iolink_vendorid;
   uint32_t exp_deviceid = mock_iolink_deviceid;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt + 1;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_od_start_cnt = mock_iolink_od_start_cnt;
   uint8_t exp_od_stop_cnt = mock_iolink_od_stop_cnt;
   uint8_t exp_pd_start_cnt = mock_iolink_pd_start_cnt;
   uint8_t exp_pd_stop_cnt = mock_iolink_pd_stop_cnt;

   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   SM_PortMode_ind (port, IOLINK_SM_PORTMODE_COMREADY);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_DS_ParamManager, cm_get_state (port));

   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);
   cm_verify_mock_cnt (exp_sm_operate_cnt, exp_ds_startup_cnt,
                       exp_ds_delete_cnt, exp_od_start_cnt, exp_od_stop_cnt,
                       exp_pd_start_cnt, exp_pd_stop_cnt);
}

static inline void cm_deactive_to_ds_parammanager (iolink_port_t * port)
{
   cm_deactive_to_startup (port, IOLINK_PORTMODE_IOL_AUTO,
                           IOLINK_VALIDATION_CHECK_NO);
   cm_startup_to_ds_parammanager (port);
}

static inline void cm_startup_to_sm_port_fault (iolink_port_t * port,
                                                iolink_sm_portmode_t mode,
                                                uint8_t exp_port_cycle_time)
{
   iolink_port_status_info_t exp_status_info =
                                          IOLINK_PORT_STATUS_INFO_PORT_DIAG;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   iolink_transmission_rate_t exp_trans_rate = mock_iolink_trans_rate;
   uint8_t exp_revisionid = mock_iolink_revisionid;
   uint16_t exp_vendorid = mock_iolink_vendorid;
   uint32_t exp_deviceid = mock_iolink_deviceid;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_od_start_cnt = mock_iolink_od_start_cnt;
   uint8_t exp_od_stop_cnt = mock_iolink_od_stop_cnt;
   uint8_t exp_pd_start_cnt = mock_iolink_pd_start_cnt;
   uint8_t exp_pd_stop_cnt = mock_iolink_pd_stop_cnt;

   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   SM_PortMode_ind (port, mode);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_PortFault, cm_get_state (port));

   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);
   cm_verify_mock_cnt (exp_sm_operate_cnt, exp_ds_startup_cnt,
                       exp_ds_delete_cnt, exp_od_start_cnt, exp_od_stop_cnt,
                       exp_pd_start_cnt, exp_pd_stop_cnt);
}

static inline void cm_deactive_to_sm_port_fault (iolink_port_t * port,
                                                 iolink_sm_portmode_t mode)
{
   cm_deactive_to_startup (port, IOLINK_PORTMODE_IOL_AUTO,
                           IOLINK_VALIDATION_CHECK_NO);
   cm_startup_to_sm_port_fault (port, mode, 0);
}

static inline void cm_x_to_waiting_on_op (iolink_port_t * port,
                                          iolink_cm_state_t exp_state)
{
   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt + 1;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt + 1;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_od_start_cnt = mock_iolink_od_start_cnt;
   uint8_t exp_od_stop_cnt = mock_iolink_od_stop_cnt;
   uint8_t exp_pd_start_cnt = mock_iolink_pd_start_cnt;
   uint8_t exp_pd_stop_cnt = mock_iolink_pd_stop_cnt;

   switch (exp_state)
   {
   case CM_STATE_Port_Deactivated:
      EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));
      cm_deactive_to_ds_parammanager (port);
      exp_ds_delete_cnt++;
      exp_od_stop_cnt++;
      exp_pd_stop_cnt++;
      break;
   case CM_STATE_SM_Startup:
      EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
      cm_startup_to_ds_parammanager (port);
      break;
   default:
      ASSERT_TRUE (0);
      break;
   }

   DS_Ready (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_WaitingOnOperate, cm_get_state (port));

   cm_verify_mock_cnt (exp_sm_operate_cnt, exp_ds_startup_cnt,
                       exp_ds_delete_cnt, exp_od_start_cnt, exp_od_stop_cnt,
                       exp_pd_start_cnt, exp_pd_stop_cnt);
}

static inline void cm_startup_to_waiting_on_op (iolink_port_t * port)
{
   cm_x_to_waiting_on_op (port, CM_STATE_SM_Startup);
}

static inline void cm_deactive_to_waiting_on_op (iolink_port_t * port)
{
   cm_x_to_waiting_on_op (port, CM_STATE_Port_Deactivated);
}

static inline void cm_x_to_port_active (iolink_port_t * port,
                                        iolink_cm_state_t exp_state)
{
   iolink_port_status_info_t exp_status_info = IOLINK_PORT_STATUS_INFO_OP;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_VALID | // TODO
                                   IOLINK_PORT_QUALITY_INFO_PDO_VALID; // TODO
   iolink_transmission_rate_t exp_trans_rate = mock_iolink_trans_rate;
   uint8_t exp_port_cycle_time = mock_iolink_min_cycletime;
   uint8_t exp_revisionid = mock_iolink_revisionid;
   uint16_t exp_vendorid = mock_iolink_vendorid;
   uint32_t exp_deviceid = mock_iolink_deviceid;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt + 1;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt + 1;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_od_start_cnt = mock_iolink_od_start_cnt + 1;
   uint8_t exp_od_stop_cnt = mock_iolink_od_stop_cnt;
   uint8_t exp_pd_start_cnt = mock_iolink_pd_start_cnt + 1;
   uint8_t exp_pd_stop_cnt = mock_iolink_pd_stop_cnt;
   uint8_t exp_smi_portevent_ind_cnt = mock_iolink_smi_portevent_ind_cnt;

   iolink_eventcode_t exp_smi_portevent_code =
                                          IOLINK_EVENTCODE_PORT_STATUS_CHANGE;
   switch (exp_state)
   {
   case CM_STATE_Port_Deactivated:
      EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));
      cm_deactive_to_waiting_on_op (port);
      exp_ds_delete_cnt++;
      exp_od_stop_cnt++;
      exp_pd_stop_cnt++;
      break;
   case CM_STATE_SM_Startup:
      EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
      cm_startup_to_waiting_on_op (port);
      break;
   default:
      ASSERT_TRUE (0);
      break;
   }
   EXPECT_EQ (exp_smi_portevent_ind_cnt, mock_iolink_smi_portevent_ind_cnt);
   SM_PortMode_ind (port, IOLINK_SM_PORTMODE_OPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   exp_smi_portevent_ind_cnt++;
   EXPECT_EQ (CM_STATE_Port_Active, cm_get_state (port));
   EXPECT_EQ (exp_smi_portevent_ind_cnt, mock_iolink_smi_portevent_ind_cnt);

   EXPECT_EQ (IOLINK_ARG_BLOCK_ID_VOID_BLOCK, mock_iolink_smi_ref_arg_block_id);

   iolink_arg_block_id_t arg_block_id =
						mock_iolink_smi_arg_block->void_block.arg_block_id;
   EXPECT_EQ (arg_block_id, IOLINK_ARG_BLOCK_ID_PORT_EVENT);

   if (mock_iolink_smi_arg_block->void_block.arg_block_id ==
       IOLINK_ARG_BLOCK_ID_PORT_EVENT)
   {
      iolink_eventcode_t event_code =
                        mock_iolink_smi_arg_block->port_event.event.event_code;

      EXPECT_EQ (exp_smi_portevent_code, event_code);
   }

   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);
   cm_verify_mock_cnt (exp_sm_operate_cnt, exp_ds_startup_cnt,
                       exp_ds_delete_cnt, exp_od_start_cnt, exp_od_stop_cnt,
                       exp_pd_start_cnt, exp_pd_stop_cnt);
}

static inline void cm_startup_to_port_active (iolink_port_t * port)
{
   cm_x_to_port_active (port, CM_STATE_SM_Startup);
}

static inline void cm_deactive_to_port_active (iolink_port_t * port)
{
   cm_x_to_port_active (port, CM_STATE_Port_Deactivated);
}

static inline void cm_x_to_deactive (iolink_port_t * port,
                                     iolink_cm_state_t exp_state)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_sm_target_mode_t exp_target_mode = IOLINK_SMTARGET_MODE_INACTIVE;
   iolink_inspectionlevel_t exp_inspection_level =
                                          IOLINK_INSPECTIONLEVEL_NO_CHECK;
   iolink_port_status_info_t exp_status_info =
                                          IOLINK_PORT_STATUS_INFO_DEACTIVATED;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   uint8_t exp_port_cycle_time = 0;
   uint8_t exp_revisionid = 0;
   iolink_transmission_rate_t exp_trans_rate =
                                          IOLINK_TRANSMISSION_RATE_NOT_DETECTED;
   uint16_t exp_vendorid = 0;
   uint32_t exp_deviceid = 0;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_smi_portcfg_cnf_cnt = mock_iolink_smi_portcfg_cnf_cnt + 1;
   uint8_t exp_od_start_cnt = mock_iolink_od_start_cnt;
   uint8_t exp_od_stop_cnt = mock_iolink_od_stop_cnt + 1;
   uint8_t exp_pd_start_cnt = mock_iolink_pd_start_cnt;
   uint8_t exp_pd_stop_cnt = mock_iolink_pd_stop_cnt + 1;
   uint8_t exp_smi_portevent_ind_cnt = mock_iolink_smi_portevent_ind_cnt;

   memset (&port_cfg, 0, sizeof(arg_block_portconfiglist_t));
   port_cfg.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;
   port_cfg.configlist.port_cycle_time = exp_port_cycle_time;
   port_cfg.configlist.vendorid = exp_vendorid;
   port_cfg.configlist.deviceid = exp_deviceid;
   port_cfg.configlist.in_buffer_len = 0; // TODO
   port_cfg.configlist.out_buffer_len = 0; // TODO
   port_cfg.configlist.portmode = IOLINK_PORTMODE_DEACTIVE;
   port_cfg.configlist.validation_backup = IOLINK_VALIDATION_CHECK_NO;
   port_cfg.configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_NO_SUPPORT;

   EXPECT_EQ (exp_state, cm_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));

   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);
   cm_verify_mock_cnt (exp_sm_operate_cnt, exp_ds_startup_cnt,
                       exp_ds_delete_cnt, exp_od_start_cnt, exp_od_stop_cnt,
                       exp_pd_start_cnt, exp_pd_stop_cnt);

   SM_PortMode_ind (port, IOLINK_SM_PORTMODE_INACTIVE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (exp_smi_portevent_ind_cnt, mock_iolink_smi_portevent_ind_cnt);
}

static inline void cm_check_mode_to_startup (iolink_port_t * port,
                                           iolink_portmode_t mode,
                                           iolink_validation_check_t validation)
{
   cm_x_to_deactive (port, CM_STATE_CheckPortMode);
   cm_deactive_to_startup (port, mode, validation);
}

static inline void cm_all_portconfiguration (iolink_port_t * port)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_sm_target_mode_t exp_target_mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   iolink_inspectionlevel_t exp_inspection_level =
                                             IOLINK_INSPECTIONLEVEL_NO_CHECK;
   uint8_t exp_port_cycle_time = 0; // AFAP (As fast as possible)
   uint8_t exp_revisionid = 0;
   uint16_t exp_vendorid = 0;
   uint32_t exp_deviceid = 0;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_smi_portcfg_cnf_cnt = mock_iolink_smi_portcfg_cnf_cnt;

   memset (&port_cfg, 0, sizeof(arg_block_portconfiglist_t));
   port_cfg.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;
   port_cfg.configlist.port_cycle_time = exp_port_cycle_time;
   port_cfg.configlist.vendorid = exp_vendorid;
   port_cfg.configlist.deviceid = exp_deviceid;
   port_cfg.configlist.in_buffer_len = 0; // TODO
   port_cfg.configlist.out_buffer_len = 0; // TODO
   port_cfg.configlist.portmode = IOLINK_PORTMODE_IOL_AUTO;
   port_cfg.configlist.validation_backup = IOLINK_VALIDATION_CHECK_NO;
   port_cfg.configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_NO_SUPPORT;

   cm_deactive_to_startup (port, IOLINK_PORTMODE_IOL_AUTO,
                           IOLINK_VALIDATION_CHECK_NO);
   exp_ds_delete_cnt++;
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   exp_ds_delete_cnt++;
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   /* Port configuration is not allowed in DS_ParamManager_2 */
   cm_startup_to_ds_parammanager (port);
   exp_ds_startup_cnt++;
   exp_ds_delete_cnt++;
   /* Port configuration is ignored */
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (CM_STATE_DS_ParamManager, cm_get_state (port));
   DS_Ready (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_WaitingOnOperate, cm_get_state (port));
   exp_sm_operate_cnt++;
   EXPECT_EQ (CM_STATE_WaitingOnOperate, cm_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_startup_to_sm_port_fault (port, IOLINK_SM_PORTMODE_REVISION_FAULT,
                                mock_iolink_min_cycletime);
   exp_ds_delete_cnt++;
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_startup_to_waiting_on_op (port);
   exp_ds_startup_cnt++;
   exp_sm_operate_cnt++;
   exp_ds_delete_cnt++;
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_startup_to_port_active (port);
   exp_ds_startup_cnt++;
   exp_sm_operate_cnt++;
   exp_ds_delete_cnt++;
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_x_to_dido (port, CM_STATE_SM_Startup, false);
   exp_ds_delete_cnt++;
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   exp_ds_delete_cnt++;
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_x_to_deactive (port, CM_STATE_SM_Startup);
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (iolink_get_portnumber (port),
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   exp_ds_delete_cnt++;
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

// Tests
TEST_F (CMTest, Cm_startup_autostart)
{
   cm_deactive_to_startup (port, IOLINK_PORTMODE_IOL_AUTO,
                           IOLINK_VALIDATION_CHECK_NO);
}

TEST_F (CMTest, Cm_startup_manual_v11)
{
   cm_deactive_to_startup (port, IOLINK_PORTMODE_IOL_MAN,
                           IOLINK_VALIDATION_CHECK_V11);
}

TEST_F (CMTest, Cm_startup_manual_v10)
{
   cm_deactive_to_startup (port, IOLINK_PORTMODE_IOL_MAN,
                           IOLINK_VALIDATION_CHECK_V10);
}

TEST_F (CMTest, Cm_ds_parammanager)
{
   cm_deactive_to_ds_parammanager (port);
}

TEST_F (CMTest, Cm_startup_fault_rev)
{
   cm_deactive_to_sm_port_fault (port, IOLINK_SM_PORTMODE_REVISION_FAULT);
}

TEST_F (CMTest, Cm_startup_fault_comp)
{
   cm_deactive_to_sm_port_fault (port, IOLINK_SM_PORTMODE_COMP_FAULT);
}

TEST_F (CMTest, Cm_startup_fault_sernum)
{
   cm_deactive_to_sm_port_fault (port, IOLINK_SM_PORTMODE_SERNUM_FAULT);
}

TEST_F (CMTest, Cm_startup_fault_cycletime)
{
   cm_deactive_to_sm_port_fault (port, IOLINK_SM_PORTMODE_CYCTIME_FAULT);
}

TEST_F (CMTest, Cm_ds_parammanager_comlost)
{
   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt + 1;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;
   uint8_t exp_smi_portcfg_cnf_cnt = mock_iolink_smi_portcfg_cnf_cnt;

   cm_deactive_to_ds_parammanager (port);
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);

   /* Ignore COMLOST */
   SM_PortMode_ind (port, IOLINK_SM_PORTMODE_COMLOST);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (CM_STATE_DS_ParamManager, cm_get_state (port));

   DS_Fault (port, IOLINK_DS_FAULT_COM_ERR);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_CheckPortMode, cm_get_state (port));

   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_ds_parammanager_lock)
{
   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt + 1;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;

   cm_deactive_to_ds_parammanager (port);
   DS_Fault (port, IOLINK_DS_FAULT_LOCK);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_PortFault, cm_get_state (port));

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_ds_parammanager_id)
{
   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt + 1;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;

   cm_deactive_to_ds_parammanager (port);
   DS_Fault (port, IOLINK_DS_FAULT_ID);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_PortFault, cm_get_state (port));

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_ds_parammanager_size)
{
   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt + 1;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;

   cm_deactive_to_ds_parammanager (port);
   DS_Fault (port, IOLINK_DS_FAULT_SIZE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_PortFault, cm_get_state (port));

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_ds_parammanager_fault_down)
{
   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt + 1;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;

   cm_deactive_to_ds_parammanager (port);
   DS_Fault (port, IOLINK_DS_FAULT_DOWN);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_PortFault, cm_get_state (port));

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_ds_parammanager_fault_up)
{
   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt + 1;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;

   cm_deactive_to_ds_parammanager (port);
   DS_Fault (port, IOLINK_DS_FAULT_UP);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_PortFault, cm_get_state (port));

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_waiting_on_op)
{
   cm_deactive_to_waiting_on_op (port);
}

TEST_F (CMTest, Cm_active)
{
   mock_iolink_trans_rate = IOLINK_TRANSMISSION_RATE_COM3;
   cm_deactive_to_port_active (port);
}

TEST_F (CMTest, Cm_acitve_unexpected_inactive)
{
   uint8_t exp_smi_portevent_ind_cnt = mock_iolink_smi_portevent_ind_cnt + 1;

   mock_iolink_trans_rate = IOLINK_TRANSMISSION_RATE_COM1;
   cm_deactive_to_port_active (port);
   EXPECT_EQ (exp_smi_portevent_ind_cnt, mock_iolink_smi_portevent_ind_cnt);

   /* Inactive when we did not request it */
   SM_PortMode_ind (port, IOLINK_SM_PORTMODE_INACTIVE);
   mock_iolink_job.callback (&mock_iolink_job);
   exp_smi_portevent_ind_cnt++;
   EXPECT_EQ (exp_smi_portevent_ind_cnt, mock_iolink_smi_portevent_ind_cnt);
}

TEST_F (CMTest, Cm_di)
{
   cm_deactive_to_dido (port, false);
}

TEST_F (CMTest, Cm_do)
{
   cm_deactive_to_dido (port, true);
}

TEST_F (CMTest, Cm_sm_COMLOST)
{
   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_smi_portcfg_cnf_cnt = mock_iolink_smi_portcfg_cnf_cnt;

   cm_deactive_to_startup (port, IOLINK_PORTMODE_IOL_AUTO,
                           IOLINK_VALIDATION_CHECK_NO);
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);
   exp_ds_delete_cnt++;
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   SM_PortMode_ind (port, IOLINK_SM_PORTMODE_COMLOST);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);
   EXPECT_EQ (CM_STATE_CheckPortMode, cm_get_state (port));
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   /* COMLOST for DS_ParamManager_2 is tested in Cm_ds_parammanager_comlost */

   cm_check_mode_to_startup (port, IOLINK_PORTMODE_IOL_AUTO,
                             IOLINK_VALIDATION_CHECK_NO);
   exp_smi_portcfg_cnf_cnt += 2;
   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);
   cm_startup_to_sm_port_fault (port, IOLINK_SM_PORTMODE_REVISION_FAULT, 0);
   exp_ds_delete_cnt++;
   SM_PortMode_ind (port, IOLINK_SM_PORTMODE_COMLOST);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);
   EXPECT_EQ (CM_STATE_CheckPortMode, cm_get_state (port));
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_check_mode_to_startup (port, IOLINK_PORTMODE_IOL_AUTO,
                             IOLINK_VALIDATION_CHECK_NO);
   exp_smi_portcfg_cnf_cnt += 2;
   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);
   cm_startup_to_waiting_on_op (port);
   exp_sm_operate_cnt++;
   exp_ds_startup_cnt++;
   exp_ds_delete_cnt++;
   SM_PortMode_ind (port, IOLINK_SM_PORTMODE_COMLOST);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);
   EXPECT_EQ (CM_STATE_CheckPortMode, cm_get_state (port));
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_check_mode_to_startup (port, IOLINK_PORTMODE_IOL_AUTO,
                             IOLINK_VALIDATION_CHECK_NO);
   exp_smi_portcfg_cnf_cnt += 2;
   cm_startup_to_port_active (port);
   exp_sm_operate_cnt++;
   exp_ds_startup_cnt++;
   exp_ds_delete_cnt++;
   SM_PortMode_ind (port, IOLINK_SM_PORTMODE_COMLOST);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (exp_smi_portcfg_cnf_cnt, mock_iolink_smi_portcfg_cnf_cnt);
   EXPECT_EQ (CM_STATE_CheckPortMode, cm_get_state (port));
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_PortConfig)
{
   mock_iolink_trans_rate = IOLINK_TRANSMISSION_RATE_COM2;
   cm_all_portconfiguration (port);
}

TEST_F (CMTest, Cm_DS_Change)
{
   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;

   cm_deactive_to_ds_parammanager (port);
   exp_ds_startup_cnt++;
   DS_Change (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_startup_to_sm_port_fault (port, IOLINK_SM_PORTMODE_REVISION_FAULT,
                                mock_iolink_min_cycletime);
   DS_Change (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_startup_to_waiting_on_op (port);
   exp_sm_operate_cnt++;
   exp_ds_startup_cnt++;
   DS_Change (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_startup_to_port_active (port);
   exp_sm_operate_cnt++;
   exp_ds_startup_cnt++;
   DS_Change (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_deactivate)
{
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;

   cm_deactive_to_port_active (port);
   exp_ds_delete_cnt++;
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
   cm_x_to_deactive (port, CM_STATE_Port_Active);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_deactive_to_dido (port, true);
   exp_ds_delete_cnt++;
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
   cm_x_to_deactive (port, CM_STATE_Port_DIDO);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   cm_x_to_deactive (port, CM_STATE_Port_Deactivated);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_SMI_PortConfig_unknown_portmode)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_sm_target_mode_t exp_target_mode = IOLINK_SMTARGET_MODE_INACTIVE;
   iolink_inspectionlevel_t exp_inspection_level =
                                          IOLINK_INSPECTIONLEVEL_NO_CHECK;
   iolink_port_status_info_t exp_status_info =
                                          IOLINK_PORT_STATUS_INFO_DEACTIVATED;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   uint8_t exp_port_cycle_time = 0;
   uint8_t exp_revisionid = 0;
   iolink_transmission_rate_t exp_trans_rate =
                                          IOLINK_TRANSMISSION_RATE_NOT_DETECTED;
   uint16_t exp_vendorid = 0;
   uint32_t exp_deviceid = 0;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   memset (&port_cfg, 0, sizeof(arg_block_portconfiglist_t));
   port_cfg.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;
   port_cfg.configlist.port_cycle_time = exp_port_cycle_time;
   port_cfg.configlist.vendorid = exp_vendorid;
   port_cfg.configlist.deviceid = exp_deviceid;
   port_cfg.configlist.in_buffer_len = 0; // TODO
   port_cfg.configlist.out_buffer_len = 0; // TODO
   port_cfg.configlist.validation_backup = IOLINK_VALIDATION_CHECK_NO;
   port_cfg.configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_NO_SUPPORT;

   port_cfg.configlist.portmode = (iolink_portmode_t)189; /* Bad value */

   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (portnumber,
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));

   cm_verify_smi_err (IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST,
                      IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_INCONSISTENT,
                      exp_smi_joberror_cnt, 0, 0);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_SMI_PortConfig_unknown_validation_backup)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_sm_target_mode_t exp_target_mode = IOLINK_SMTARGET_MODE_INACTIVE;
   iolink_inspectionlevel_t exp_inspection_level =
                                          IOLINK_INSPECTIONLEVEL_NO_CHECK;
   iolink_port_status_info_t exp_status_info =
                                          IOLINK_PORT_STATUS_INFO_DEACTIVATED;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   uint8_t exp_port_cycle_time = 0;
   uint8_t exp_revisionid = 0;
   iolink_transmission_rate_t exp_trans_rate =
                                          IOLINK_TRANSMISSION_RATE_NOT_DETECTED;
   uint16_t exp_vendorid = 0;
   uint32_t exp_deviceid = 0;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   memset (&port_cfg, 0, sizeof(arg_block_portconfiglist_t));
   port_cfg.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;
   port_cfg.configlist.port_cycle_time = exp_port_cycle_time;
   port_cfg.configlist.vendorid = exp_vendorid;
   port_cfg.configlist.deviceid = exp_deviceid;
   port_cfg.configlist.in_buffer_len = 0; // TODO
   port_cfg.configlist.out_buffer_len = 0; // TODO
   port_cfg.configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_NO_SUPPORT;
   /* Bad value */
   port_cfg.configlist.validation_backup = (iolink_validation_check_t)189;

   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (portnumber,
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));

   cm_verify_smi_err (IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST,
                      IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_INCONSISTENT,
                      exp_smi_joberror_cnt, 0, 0);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_SMI_PortConfig_unknown_iq_behavior)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_sm_target_mode_t exp_target_mode = IOLINK_SMTARGET_MODE_INACTIVE;
   iolink_inspectionlevel_t exp_inspection_level =
                                          IOLINK_INSPECTIONLEVEL_NO_CHECK;
   iolink_port_status_info_t exp_status_info =
                                          IOLINK_PORT_STATUS_INFO_DEACTIVATED;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   uint8_t exp_port_cycle_time = 0;
   uint8_t exp_revisionid = 0;
   iolink_transmission_rate_t exp_trans_rate =
                                          IOLINK_TRANSMISSION_RATE_NOT_DETECTED;
   uint16_t exp_vendorid = 0;
   uint32_t exp_deviceid = 0;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt + 1;
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   memset (&port_cfg, 0, sizeof(arg_block_portconfiglist_t));
   port_cfg.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;
   port_cfg.configlist.port_cycle_time = exp_port_cycle_time;
   port_cfg.configlist.vendorid = exp_vendorid;
   port_cfg.configlist.deviceid = exp_deviceid;
   port_cfg.configlist.in_buffer_len = 0; // TODO
   port_cfg.configlist.out_buffer_len = 0; // TODO
   port_cfg.configlist.validation_backup = IOLINK_VALIDATION_CHECK_NO;

   port_cfg.configlist.iq_behavior = (iolink_iq_behavior_t)189; /* Bad value */

   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (portnumber,
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));

   cm_verify_smi_err (IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST,
                      IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_INCONSISTENT,
                      exp_smi_joberror_cnt, 0, 0);
   cm_verify_paraml (exp_port_cycle_time, exp_target_mode, exp_revisionid,
                     exp_inspection_level, exp_vendorid, exp_deviceid);
   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, CM_ignore_additional_DS_Ready)
{
   int i;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt + 1;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt + 1;
   uint8_t exp_smi_portevent_ind_cnt = mock_iolink_smi_portevent_ind_cnt + 1;

   cm_deactive_to_port_active (port);

   for (i = 0; i < 5; i++)
   {
      /* Ignore additional DS_Ready */
      DS_Ready (port);
      mock_iolink_job.callback (&mock_iolink_job);
   }

   EXPECT_EQ (exp_smi_portevent_ind_cnt, mock_iolink_smi_portevent_ind_cnt);
   EXPECT_EQ (exp_ds_startup_cnt, mock_iolink_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
}

TEST_F (CMTest, Cm_SMI_PortConfig_bad_argblock)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_port_status_info_t exp_status_info =
                                          IOLINK_PORT_STATUS_INFO_DEACTIVATED;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   uint8_t exp_port_cycle_time = 0;
   uint8_t exp_revisionid = 0;
   iolink_transmission_rate_t exp_trans_rate =
                                          IOLINK_TRANSMISSION_RATE_NOT_DETECTED;
   uint16_t exp_vendorid = 0;
   uint32_t exp_deviceid = 0;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   iolink_arg_block_id_t bad_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;

   memset (&port_cfg, 0, sizeof(arg_block_portconfiglist_t));
   port_cfg.configlist.port_cycle_time = exp_port_cycle_time;
   port_cfg.configlist.vendorid = exp_vendorid;
   port_cfg.configlist.deviceid = exp_deviceid;
   port_cfg.configlist.in_buffer_len = 0; // TODO
   port_cfg.configlist.out_buffer_len = 0; // TODO
   port_cfg.configlist.validation_backup = IOLINK_VALIDATION_CHECK_NO;
   port_cfg.configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_NO_SUPPORT;

   /* Bad ArgBlockID */
   port_cfg.arg_block_id = bad_arg_block_id;

   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (portnumber,
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));

   cm_verify_smi_err (bad_arg_block_id, IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED,
                      exp_smi_joberror_cnt, 0, 0);
   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_SMI_PortConfig_bad_exp_arg_block_id)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_port_status_info_t exp_status_info =
                                          IOLINK_PORT_STATUS_INFO_DEACTIVATED;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   uint8_t exp_port_cycle_time = 0;
   uint8_t exp_revisionid = 0;
   iolink_transmission_rate_t exp_trans_rate =
                                          IOLINK_TRANSMISSION_RATE_NOT_DETECTED;
   uint16_t exp_vendorid = 0;
   uint32_t exp_deviceid = 0;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   iolink_arg_block_id_t bad_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_DS_DATA;

   memset (&port_cfg, 0, sizeof(arg_block_portconfiglist_t));
   port_cfg.configlist.port_cycle_time = exp_port_cycle_time;
   port_cfg.configlist.vendorid = exp_vendorid;
   port_cfg.configlist.deviceid = exp_deviceid;
   port_cfg.configlist.in_buffer_len = 0; // TODO
   port_cfg.configlist.out_buffer_len = 0; // TODO
   port_cfg.configlist.validation_backup = IOLINK_VALIDATION_CHECK_NO;
   port_cfg.configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_NO_SUPPORT;

   port_cfg.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;

   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (portnumber,
                                         /* Bad exp_arg_block_id */
                                         bad_exp_arg_block_id,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));

   cm_verify_smi_err (IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST,
                      bad_exp_arg_block_id,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_INCONSISTENT,
                      exp_smi_joberror_cnt, 0, 0);
   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_SMI_PortConfig_bad_argblock_len)
{
   arg_block_portconfiglist_t port_cfg;

   iolink_port_status_info_t exp_status_info =
                                          IOLINK_PORT_STATUS_INFO_DEACTIVATED;
   uint8_t exp_port_quality_info = IOLINK_PORT_QUALITY_INFO_INVALID;
   uint8_t exp_port_cycle_time = 0;
   uint8_t exp_revisionid = 0;
   iolink_transmission_rate_t exp_trans_rate =
                                          IOLINK_TRANSMISSION_RATE_NOT_DETECTED;
   uint16_t exp_vendorid = 0;
   uint32_t exp_deviceid = 0;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   memset (&port_cfg, 0, sizeof(arg_block_portconfiglist_t));
   port_cfg.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;
   port_cfg.configlist.port_cycle_time = exp_port_cycle_time;
   port_cfg.configlist.vendorid = exp_vendorid;
   port_cfg.configlist.deviceid = exp_deviceid;
   port_cfg.configlist.in_buffer_len = 0; // TODO
   port_cfg.configlist.out_buffer_len = 0; // TODO
   port_cfg.configlist.validation_backup = IOLINK_VALIDATION_CHECK_NO;
   port_cfg.configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_NO_SUPPORT;

   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (portnumber,
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         /* Bad ArgBlockLength */
                                         sizeof(arg_block_portconfiglist_t) + 1,
                                         (arg_block_t *)&port_cfg));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (CM_STATE_Port_Deactivated, cm_get_state (port));

   cm_verify_smi_err (IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST,
                      IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID,
                      exp_smi_joberror_cnt, 0, 0);
   cm_verify_portstatus (port, exp_status_info, exp_port_quality_info,
                         exp_revisionid, exp_trans_rate, exp_port_cycle_time,
                         exp_vendorid, exp_deviceid, 0, NULL);

   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);
}

TEST_F (CMTest, Cm_SMI_PortStatus_bad_argblock)
{
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   arg_block_void_t arg_block_void;
   iolink_arg_block_id_t bad_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   iolink_arg_block_id_t exp_arg_block_id =
                                          IOLINK_ARG_BLOCK_ID_PORT_STATUS_LIST;

   memset (&arg_block_void, 0, sizeof(arg_block_void_t));
   arg_block_void.arg_block_id = bad_arg_block_id;

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortStatus_req (portnumber, exp_arg_block_id,
                                  sizeof(arg_block_void_t),
                                  (arg_block_t *)&arg_block_void));
   mock_iolink_job.callback (&mock_iolink_job);

   cm_verify_smi_err (bad_arg_block_id, exp_arg_block_id,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED,
                      exp_smi_joberror_cnt, 0, 0);
}

TEST_F (CMTest, Cm_SMI_PortStatus_bad_argblock_len)
{
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   arg_block_void_t arg_block_void;
   iolink_arg_block_id_t arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_arg_block_id =
                                          IOLINK_ARG_BLOCK_ID_PORT_STATUS_LIST;

   memset (&arg_block_void, 0, sizeof(arg_block_void_t));
   arg_block_void.arg_block_id = arg_block_id;

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortStatus_req (portnumber, exp_arg_block_id,
                                  /* Bad ArgBlockLength */
                                  sizeof(arg_block_void_t) + 1,
                                  (arg_block_t *)&arg_block_void));
   mock_iolink_job.callback (&mock_iolink_job);

   cm_verify_smi_err (arg_block_id, exp_arg_block_id,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID,
                      exp_smi_joberror_cnt, 0, 0);
}

TEST_F (CMTest, Cm_SMI_MasterIdent)
{
   uint8_t exp_smi_cnf_cnt = mock_iolink_smi_cnf_cnt + 1;
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt;
   uint16_t exp_vendorid = 1171;
   uint32_t exp_masterid = 123;
   uint8_t exp_max_number_of_ports = 2;
   iolink_port_types_t exp_port_type = IOLINK_PORT_TYPES_A;
   iolink_master_type_t exp_master_type = IOLINK_MASTER_TYPE_MASTER_ACC;

   arg_block_void_t arg_block_void;
   iolink_arg_block_id_t arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;

   memset (&arg_block_void, 0, sizeof(arg_block_void_t));
   arg_block_void.arg_block_id = arg_block_id;

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_MasterIdentification_req (portnumber, exp_arg_block_id,
                                            sizeof(arg_block_void_t),
                                            (arg_block_t *)&arg_block_void));
   mock_iolink_job.callback (&mock_iolink_job);

   cm_verify_smi_masterident (arg_block_id, exp_arg_block_id, exp_vendorid,
                              exp_masterid, exp_master_type,
                              exp_max_number_of_ports, exp_port_type,
                              exp_smi_cnf_cnt, exp_smi_joberror_cnt);
}

TEST_F (CMTest, Cm_SMI_MasterIdent_bad_argblock)
{
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   arg_block_void_t arg_block_void;
   iolink_arg_block_id_t bad_arg_block_id = IOLINK_ARG_BLOCK_ID_DEV_EVENT;
   iolink_arg_block_id_t exp_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;

   memset (&arg_block_void, 0, sizeof(arg_block_void_t));
   arg_block_void.arg_block_id = bad_arg_block_id;

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_MasterIdentification_req (portnumber, exp_arg_block_id,
                                            sizeof(arg_block_void_t),
                                            (arg_block_t *)&arg_block_void));
   mock_iolink_job.callback (&mock_iolink_job);

   cm_verify_smi_err (bad_arg_block_id, exp_arg_block_id,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED,
                      exp_smi_joberror_cnt, 0, 0);
}

TEST_F (CMTest, Cm_SMI_MasterIdent_bad_argblock_len)
{
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   arg_block_void_t arg_block_void;
   iolink_arg_block_id_t arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;

   memset (&arg_block_void, 0, sizeof(arg_block_void_t));
   arg_block_void.arg_block_id = arg_block_id;

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_MasterIdentification_req (portnumber, exp_arg_block_id,
                                            /* Bad ArgBlockLength */
                                            sizeof(arg_block_void_t) + 1,
                                            (arg_block_t *)&arg_block_void));
   mock_iolink_job.callback (&mock_iolink_job);

   cm_verify_smi_err (arg_block_id, exp_arg_block_id,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID,
                      exp_smi_joberror_cnt, 0, 0);
}

TEST_F (CMTest, Cm_SMI_ReadbackPortConfiguration)
{
   arg_block_portconfiglist_t arg_block_portcfg;
   portconfiglist_t configlist;

   uint8_t exp_port_cycle_time = 0; // AFAP (As fast as possible)
   uint16_t exp_vendorid = 1;
   uint32_t exp_deviceid = 2;

   uint8_t exp_sm_operate_cnt = mock_iolink_sm_operate_cnt;
   uint8_t exp_ds_startup_cnt = mock_iolink_ds_startup_cnt;
   uint8_t exp_ds_delete_cnt = mock_iolink_ds_delete_cnt;
   uint8_t exp_smi_portcfg_cnf_cnt = mock_iolink_smi_portcfg_cnf_cnt;

   memset (&configlist, 0, sizeof(portconfiglist_t));

   /* SMI_ReadbackPortConfiguration_req() */
   cm_verify_smi_portconfig (port, &configlist);

   memset (&arg_block_portcfg, 0, sizeof(arg_block_portconfiglist_t));
   arg_block_portcfg.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;
   configlist.port_cycle_time = exp_port_cycle_time;
   configlist.vendorid = exp_vendorid;
   configlist.deviceid = exp_deviceid;
   configlist.in_buffer_len = 0; // TODO
   configlist.out_buffer_len = 0; // TODO
   configlist.portmode = IOLINK_PORTMODE_IOL_AUTO;
   configlist.validation_backup = IOLINK_VALIDATION_CHECK_NO;
   configlist.iq_behavior = IOLINK_IQ_BEHAVIOR_NO_SUPPORT;
   memcpy (&arg_block_portcfg.configlist, &configlist,
           sizeof(portconfiglist_t));

   /* SMI_PortConfiguration_req() */
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_PortConfiguration_req (portnumber,
                                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                         sizeof(arg_block_portconfiglist_t),
                                         (arg_block_t *)&arg_block_portcfg));
   mock_iolink_job.callback (&mock_iolink_job);
   exp_ds_delete_cnt++;
   exp_smi_portcfg_cnf_cnt++;
   EXPECT_EQ (CM_STATE_SM_Startup, cm_get_state (port));
   cm_verify_smi_portcfg_cnf (exp_smi_portcfg_cnf_cnt);
   EXPECT_EQ (mock_iolink_sm_operate_cnt, exp_sm_operate_cnt);
   EXPECT_EQ (mock_iolink_ds_startup_cnt, exp_ds_startup_cnt);
   EXPECT_EQ (mock_iolink_ds_delete_cnt, exp_ds_delete_cnt);

   /* SMI_ReadbackPortConfiguration_req() */
   cm_verify_smi_portconfig (port, &configlist);
}

TEST_F (CMTest, Cm_SMI_ReadbackPortConfiguration_bad_argblock)
{
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   arg_block_void_t arg_block_void;
   iolink_arg_block_id_t bad_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   iolink_arg_block_id_t exp_arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;

   memset (&arg_block_void, 0, sizeof(arg_block_void_t));
   arg_block_void.arg_block_id = bad_arg_block_id;

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_ReadbackPortConfiguration_req (portnumber, exp_arg_block_id,
                                              sizeof(arg_block_void_t),
                                              (arg_block_t *)&arg_block_void));
   mock_iolink_job.callback (&mock_iolink_job);

   cm_verify_smi_err (bad_arg_block_id, exp_arg_block_id,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED,
                      exp_smi_joberror_cnt, 0, 0);
}
TEST_F (CMTest, Cm_SMI_ReadbackPortConfiguration_bad_exp_arg_block_id)
{
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   arg_block_void_t arg_block_void;
   iolink_arg_block_id_t arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t bad_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_DS_DATA;

   memset (&arg_block_void, 0, sizeof(arg_block_void_t));
   arg_block_void.arg_block_id = arg_block_id;

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_ReadbackPortConfiguration_req (portnumber,
                                              /* Bad exp_arg_block_id */
                                              bad_exp_arg_block_id,
                                              sizeof(arg_block_void_t),
                                              (arg_block_t *)&arg_block_void));
   mock_iolink_job.callback (&mock_iolink_job);

   cm_verify_smi_err (arg_block_id, bad_exp_arg_block_id,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_INCONSISTENT,
                      exp_smi_joberror_cnt, 0, 0);
}

TEST_F (CMTest, Cm_SMI_ReadbackPortConfiguration_bad_argblock_len)
{
   uint8_t exp_smi_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   arg_block_void_t arg_block_void;
   iolink_arg_block_id_t arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_arg_block_id =  IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST;

   memset (&arg_block_void, 0, sizeof(arg_block_void_t));
   arg_block_void.arg_block_id = arg_block_id;

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_ReadbackPortConfiguration_req (portnumber, exp_arg_block_id,
                                              /* Bad ArgBlockLength */
                                              sizeof(arg_block_void_t) + 1,
                                              (arg_block_t *)&arg_block_void));
   mock_iolink_job.callback (&mock_iolink_job);

   cm_verify_smi_err (arg_block_id, exp_arg_block_id,
                      IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID,
                      exp_smi_joberror_cnt, 0, 0);
}
