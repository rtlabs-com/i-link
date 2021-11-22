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
#include "iolink_sm.h"
#include "iolink_al.h"
#include "test_util.h"

#include <string.h> /* memset */

// Test fixture

class SMTest : public TestBase
{
protected:
   // Override default setup
   virtual void SetUp() {
      TestBase::SetUp(); // Re-use default setup
   };
};

static inline iolink_sm_state_t sm_get_state (iolink_port_t * port)
{
   iolink_sm_port_t * sm = iolink_get_sm_ctx (port);

   return sm->state;
}

// Helpers
void sm_set_portconfig_inactive (iolink_port_t * port);
void sm_verify_readcomparameters (iolink_port_t * port);
void sm_verify_checkcomp_params (iolink_port_t * port, bool is_v10);
void sm_set_portconfig_v11_autocom (iolink_port_t * port);

void sm_trans_1_2 (iolink_port_t * port);
void sm_trans_8_10 (iolink_port_t * port);
void sm_trans_12 (iolink_port_t * port);
void sm_trans_13 (iolink_port_t * port);

void sm_trans_1_2 (iolink_port_t * port)
{
   uint8_t master_ident_cnt = mock_iolink_mastercmd_master_ident_cnt;

   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));
   sm_set_portconfig_v11_autocom (port);
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));

   // DL_Mode_STARTUP T1
   DL_Mode_ind (port, IOLINK_MHMODE_STARTUP);
   mock_iolink_job.callback (&mock_iolink_job);

   sm_verify_readcomparameters (port);
   sm_verify_checkcomp_params (port, false);

   EXPECT_EQ (SM_STATE_CheckComp, sm_get_state (port));
   // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD
   mock_iolink_job.callback (&mock_iolink_job);

   // <> V10 T21
   EXPECT_EQ (master_ident_cnt + 1, mock_iolink_mastercmd_master_ident_cnt);
   // CompOK T2
   EXPECT_EQ (SM_STATE_waitonDLPreoperate, sm_get_state (port));
}

void sm_trans_8_10 (iolink_port_t * port)
{
   uint8_t serialnumber [] = {0x11, 0x22, 0x33};

   sm_trans_1_2 (port);

   // DL_Mode_PREOPERATE T8
   DL_Mode_ind (port, IOLINK_MHMODE_PREOPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_checkSerNum, sm_get_state (port));

   mock_iolink_al_read_cnf_cb (port, sizeof(serialnumber), serialnumber,
                               IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   // SerNumOk T10
   EXPECT_EQ (SM_STATE_wait, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMREADY, mock_iolink_sm_portmode);
}

void sm_trans_12 (iolink_port_t * port)
{
   uint8_t expected_write_devmode_operate_cnt =
                                    mock_iolink_write_devmode_operate_cnt + 1;

   sm_trans_8_10 (port);

   // SM_Operate T12
   SM_Operate (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_write_master_cycl, sm_get_state (port));

   // AL_Write_cnf IOL_DIR_PARAMA_MASTER_CYCL
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_wait_devmode_cnf, sm_get_state (port));

   // DL_Write_Devicemode_cnf()
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (expected_write_devmode_operate_cnt,
              mock_iolink_write_devmode_operate_cnt);
   EXPECT_EQ (SM_STATE_SMOperate, sm_get_state (port));
}

void sm_trans_13 (iolink_port_t * port)
{
   uint8_t expected_write_devmode_operate_cnt;

   sm_trans_12 (port);
   expected_write_devmode_operate_cnt = mock_iolink_write_devmode_operate_cnt;

   // DL_Mode_OPERATE T13
   DL_Mode_ind (port, IOLINK_MHMODE_OPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (expected_write_devmode_operate_cnt,
              mock_iolink_write_devmode_operate_cnt);
   EXPECT_EQ (SM_STATE_SMOperate, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_OPERATE, mock_iolink_sm_portmode);
}

void sm_trans_15 (iolink_port_t * port, bool is_autocom)
{
   iolink_error_t res;
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_NO_CHECK;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   if (is_autocom) {
     paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   } else {
     paraml.mode = IOLINK_SMTARGET_MODE_CFGCOM;
   }

   // SM_SetPortConfig_CFGCOM_or_AUTOCOM
   res = SM_SetPortConfig_req (port, &paraml);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (IOLINK_ERROR_NONE, res);
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));
}


void sm_trans_16 (iolink_port_t * port, bool is_di)
{
   iolink_error_t res;
   iolink_smp_parameterlist_t paraml;
   iolink_sm_portmode_t expected_sm_portmode;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_NO_CHECK;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   if (is_di) {
     paraml.mode = IOLINK_SMTARGET_MODE_DI;
     expected_sm_portmode = IOLINK_SM_PORTMODE_DI;
   } else {
     paraml.mode = IOLINK_SMTARGET_MODE_DO;
     expected_sm_portmode = IOLINK_SM_PORTMODE_DO;
   }

   // SM_SetPortConfig_DI_or_DO T16
   res = SM_SetPortConfig_req (port, &paraml);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (IOLINK_ERROR_NONE, res);
   EXPECT_EQ (SM_STATE_DIDO, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (expected_sm_portmode, mock_iolink_sm_portmode);
}

void sm_verify_readcomparameters (iolink_port_t * port)
{
   EXPECT_EQ (SM_STATE_ReadComParameter, sm_get_state (port));
   // IOL_DIR_PARAMA_MIN_CYCL:
   mock_iolink_job.callback (&mock_iolink_job);
   // IOL_DIR_PARAMA_MSEQ_CAP:
   mock_iolink_job.callback (&mock_iolink_job);
   // IOL_DIR_PARAMA_REV_ID:
   mock_iolink_job.callback (&mock_iolink_job);
   // IOL_DIR_PARAMA_PDI:
   mock_iolink_job.callback (&mock_iolink_job);
   // IOL_DIR_PARAMA_PDO:
   EXPECT_EQ (SM_STATE_ReadComParameter, sm_get_state (port));
   mock_iolink_job.callback (&mock_iolink_job);
   // -> SM_STATE_checkCompatibility
}

void sm_verify_checkcomp_params (iolink_port_t * port, bool is_v10)
{
   // IOL_DIR_PARAMA_VID_1:
   mock_iolink_job.callback (&mock_iolink_job);
   // IOL_DIR_PARAMA_VID_2:
   mock_iolink_job.callback (&mock_iolink_job);
   // IOL_DIR_PARAMA_DID_1:
   mock_iolink_job.callback (&mock_iolink_job);
   // IOL_DIR_PARAMA_DID_2:
   mock_iolink_job.callback (&mock_iolink_job);
   // IOL_DIR_PARAMA_DID_3:
   mock_iolink_job.callback (&mock_iolink_job);
   // IOL_DIR_PARAMA_FID_1:
   mock_iolink_job.callback (&mock_iolink_job);
   // IOL_DIR_PARAMA_FID_2:
   mock_iolink_job.callback (&mock_iolink_job);
   // -> SM_STATE_CheckVxy or SM_STATE_checkCompV10
}

void sm_checkCompatibility (iolink_port_t * port,
                            iolink_smp_parameterlist_t * paraml,
                            iolink_sm_state_t expected_state,
                            bool is_v10)
{
   iolink_error_t res;

   EXPECT_EQ (expected_state, sm_get_state (port));

   // SM_SetPortConfig_CFGCOM_AUTOCOM T15 or _INACTIVE T14
   res = SM_SetPortConfig_req (port, paraml);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (IOLINK_ERROR_NONE, res);
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));

   // DL_Mode_STARTUP T1
   DL_Mode_ind (port, IOLINK_MHMODE_STARTUP);
   mock_iolink_job.callback (&mock_iolink_job);

   sm_verify_readcomparameters (port);
   if (!is_v10)
   {
      EXPECT_EQ (SM_STATE_CheckVxy, sm_get_state (port));
      // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD
      mock_iolink_job.callback (&mock_iolink_job);
      if (sm_get_state (port) == SM_STATE_RestartDevice)
      {
         return;
      }
   }
   sm_verify_checkcomp_params (port, is_v10);

}

void sm_set_portconfig_inactive (iolink_port_t * port)
{
   iolink_error_t res;
   iolink_smp_parameterlist_t paraml;
   iolink_sm_state_t current_state = sm_get_state (port);
   uint8_t expected_write_devmode_inactive_cnt =
                                       mock_iolink_write_devmode_inactive_cnt;

   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;
   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_NO_CHECK;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_INACTIVE;
   // SM_SetPortConfig_INACTIVE T14
   res = SM_SetPortConfig_req (port, &paraml);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (IOLINK_ERROR_NONE, res);

   if (current_state != SM_STATE_PortInactive &&
       current_state != SM_STATE_DIDO)
   {
      expected_write_devmode_inactive_cnt++;
      EXPECT_EQ (expected_write_devmode_inactive_cnt,
                 mock_iolink_write_devmode_inactive_cnt);
      EXPECT_EQ (SM_STATE_waitForFallback, sm_get_state (port));

      // DL_Write_Devicemode (INACTIVE)
      mock_iolink_job.callback (&mock_iolink_job);
   }
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_INACTIVE, mock_iolink_sm_portmode);
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));
}

void sm_set_portconfig_v11_autocom (iolink_port_t * port)
{
   iolink_error_t res;
   iolink_smp_parameterlist_t paraml;

   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;
   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_NO_CHECK;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   // SM_SetPortConfig_AUTOCOM T15
   res = SM_SetPortConfig_req (port, &paraml);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (IOLINK_ERROR_NONE, res);
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));
}

// Tests

TEST_F (SMTest, Fsm)
{
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));

   sm_trans_13 (port);
}

TEST_F (SMTest, invalid_inspection_level)
{
   iolink_smp_parameterlist_t paraml;
   iolink_error_t res;

   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));

   paraml.inspectionlevel = (iolink_inspectionlevel_t)0x66; // Bad
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_INACTIVE;

   res = SM_SetPortConfig_req (port, &paraml);
   EXPECT_EQ (IOLINK_ERROR_PARAMETER_CONFLICT, res);
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));
}

TEST_F (SMTest, invalid_target_mode)
{
   iolink_smp_parameterlist_t paraml;
   iolink_error_t res;

   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_NO_CHECK;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = (iolink_sm_target_mode_t)0x66; // Bad

   res = SM_SetPortConfig_req (port, &paraml);
   EXPECT_EQ (IOLINK_ERROR_PARAMETER_CONFLICT, res);
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));
   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
}

TEST_F (SMTest, checkCompatibility_IL_TYPE_COMP_bad_revision)
{
   int i;
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V11 + 1;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11; // != mock_iolink_revisionid
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;

   for (i = 0; i < 5; i++)
   {
      sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

      EXPECT_EQ (SM_STATE_RestartDevice, sm_get_state (port));
      mock_iolink_job.callback (&mock_iolink_job); // write_cnf IOL_DIR_PARAMA_REV_ID config_paramlist->revisionid
      mock_iolink_job.callback (&mock_iolink_job); // write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_IDENT
      EXPECT_EQ (SM_STATE_RestartDevice, sm_get_state (port));
      mock_iolink_job.callback (&mock_iolink_job); // Dummy DL_Read_cnf
      EXPECT_EQ (SM_STATE_ReadComParameter, sm_get_state (port));
      EXPECT_EQ (((i + 1) * 2) - 1, mock_iolink_mastercmd_master_ident_cnt);

      sm_verify_readcomparameters (port);

      EXPECT_EQ (SM_STATE_CheckVxy, sm_get_state (port));
      // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD
      mock_iolink_job.callback (&mock_iolink_job);

      // <> V10 T21
      EXPECT_EQ ((i + 1) * 2, mock_iolink_mastercmd_master_ident_cnt);
      // RetryStartup T25
      EXPECT_EQ (i + 1, mock_iolink_restart_device_cnt);
      // RevisionFault T6
      EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

      EXPECT_EQ (i + 1, mock_iolink_mastercmd_preop_cnt);
      // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
      mock_iolink_job.callback (&mock_iolink_job);
      // Verify reported SM_PortMode_ind()
      EXPECT_EQ (IOLINK_SM_PORTMODE_REVISION_FAULT, mock_iolink_sm_portmode);

      sm_set_portconfig_inactive (port);
   }
}

TEST_F (SMTest, checkCompatibility_IL_IDENTICAL_bad_revision)
{
   int i;
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V11 + 1;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11; // != mock_iolink_revisionid
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;

   for (i = 0; i < 5; i++)
   {
      sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

      EXPECT_EQ (SM_STATE_RestartDevice, sm_get_state (port));
      mock_iolink_job.callback (&mock_iolink_job); // write_cnf IOL_DIR_PARAMA_REV_ID config_paramlist->revisionid
      mock_iolink_job.callback (&mock_iolink_job); // write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_IDENT
      EXPECT_EQ (SM_STATE_RestartDevice, sm_get_state (port));
      mock_iolink_job.callback (&mock_iolink_job); // Dummy DL_Read_cnf
      EXPECT_EQ (SM_STATE_ReadComParameter, sm_get_state (port));
      EXPECT_EQ (((i + 1) * 2) - 1, mock_iolink_mastercmd_master_ident_cnt);

      sm_verify_readcomparameters (port);

      EXPECT_EQ (SM_STATE_CheckVxy, sm_get_state (port));
      // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD
      mock_iolink_job.callback (&mock_iolink_job);

      // <> V10 T21
      EXPECT_EQ ((i + 1) * 2, mock_iolink_mastercmd_master_ident_cnt);
      // RetryStartup T25
      EXPECT_EQ (i + 1, mock_iolink_restart_device_cnt);
      // RevisionFault T6
      EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

      EXPECT_EQ (i + 1, mock_iolink_mastercmd_preop_cnt);
      // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
      mock_iolink_job.callback (&mock_iolink_job);
      // Verify reported SM_PortMode_ind()
      EXPECT_EQ (IOLINK_SM_PORTMODE_REVISION_FAULT, mock_iolink_sm_portmode);

      DL_Mode_ind (port, IOLINK_MHMODE_PREOPERATE);
      mock_iolink_job.callback (&mock_iolink_job);
      EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

      sm_set_portconfig_inactive (port);
   }
}

TEST_F (SMTest, checkCompatibility_IL_TYPE_COMP)
{
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
   paraml.cycletime = mock_iolink_min_cycletime;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;
   memset (paraml.serialnumber, 0, sizeof(paraml.serialnumber));
   paraml.serialnumber[0] = 0x11;
   paraml.serialnumber[1] = 0x22;
   paraml.serialnumber[2] = 0x33;
   paraml.serialnumber[3] = 0x44;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

   // <> V10 T21
   EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   // CompOK T2
   EXPECT_EQ (SM_STATE_waitonDLPreoperate, sm_get_state (port));
   DL_Mode_ind (port, IOLINK_MHMODE_PREOPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_checkSerNum, sm_get_state (port));

   mock_iolink_al_read_cnf_cb (port, sizeof(paraml.serialnumber),
                               paraml.serialnumber, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   // SerNumOk T10
   EXPECT_EQ (SM_STATE_wait, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMREADY, mock_iolink_sm_portmode);
}

TEST_F (SMTest, checkCompatibility_IL_TYPE_COMP_bad_serial)
{
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;
   memset (paraml.serialnumber, 0, sizeof(paraml.serialnumber));
   paraml.serialnumber[0] = 0x11;
   paraml.serialnumber[1] = 0x22;
   paraml.serialnumber[2] = 0x33;
   paraml.serialnumber[3] = 0x44;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

   // <> V10 T21
   EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   // CompOK T2
   EXPECT_EQ (SM_STATE_waitonDLPreoperate, sm_get_state (port));
   DL_Mode_ind (port, IOLINK_MHMODE_PREOPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_checkSerNum, sm_get_state (port));

   paraml.serialnumber[4] = 0x66; // bad serial number
   mock_iolink_al_read_cnf_cb (port, sizeof(paraml.serialnumber),
                               paraml.serialnumber, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   // SerNumOk T10
   EXPECT_EQ (SM_STATE_wait, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMREADY, mock_iolink_sm_portmode);
}

TEST_F (SMTest, checkCompatibility_IL_TYPE_COMP_bad_vid)
{
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   // Bad vid
   paraml.vendorid = 0x6666;
   paraml.deviceid = mock_iolink_deviceid;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

   // <> V10 T21
   EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
   // CompFault T7
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

   EXPECT_EQ (1, mock_iolink_mastercmd_preop_cnt);
   // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
   mock_iolink_job.callback (&mock_iolink_job);
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

   DL_Mode_ind (port, IOLINK_MHMODE_PREOPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
}

TEST_F (SMTest, checkCompatibility_IL_TYPE_COMP_bad_did)
{
   int i;
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   // Bad did
   paraml.deviceid = 0x666666;

   for (i = 0; i < 5; i++)
   {
      sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

      EXPECT_EQ (SM_STATE_RestartDevice, sm_get_state (port));
      mock_iolink_job.callback (&mock_iolink_job); // write_cnf IOL_DIR_PARAMA_REV_ID config_paramlist->revisionid
      mock_iolink_job.callback (&mock_iolink_job); // write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_IDENT
      EXPECT_EQ (SM_STATE_RestartDevice, sm_get_state (port));
      mock_iolink_job.callback (&mock_iolink_job); // Dummy DL_Read_cnf
      EXPECT_EQ (SM_STATE_ReadComParameter, sm_get_state (port));
      EXPECT_EQ (((i + 1) * 2) - 1, mock_iolink_mastercmd_master_ident_cnt);

      sm_verify_readcomparameters (port);
      // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD
      mock_iolink_job.callback (&mock_iolink_job);
      sm_verify_checkcomp_params (port, true);

      // <> V10 T21
      EXPECT_EQ ((i + 1) * 2, mock_iolink_mastercmd_master_ident_cnt);
      // RetryStartup T23
      EXPECT_EQ (i + 1, mock_iolink_restart_device_cnt);
      // CompFault T7
      EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

      EXPECT_EQ (i + 1, mock_iolink_mastercmd_preop_cnt);
      // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
      mock_iolink_job.callback (&mock_iolink_job);
      // Verify reported SM_PortMode_ind()
      EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

      sm_set_portconfig_inactive (port);
   }
}

TEST_F (SMTest, checkCompatibility_IL_TYPE_COMP_bad_cyc_time)
{
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
   // Bad cycle time
   paraml.cycletime = mock_iolink_min_cycletime - 1;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

   // <> V10 T21
   EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
   // CycTimeFault T17
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

   EXPECT_EQ (1, mock_iolink_mastercmd_preop_cnt);
   // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
   mock_iolink_job.callback (&mock_iolink_job);
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_CYCTIME_FAULT, mock_iolink_sm_portmode);

   DL_Mode_ind (port, IOLINK_MHMODE_PREOPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
}

TEST_F (SMTest, checkCompatibility_IL_IDENTICAL)
{
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;
   memset (paraml.serialnumber, 0, sizeof(paraml.serialnumber));
   paraml.serialnumber[0] = 0x11;
   paraml.serialnumber[1] = 0x22;
   paraml.serialnumber[2] = 0x33;
   paraml.serialnumber[3] = 0x44;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

   // <> V10 T21
   EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   // CompOK T2
   EXPECT_EQ (SM_STATE_waitonDLPreoperate, sm_get_state (port));
   DL_Mode_ind (port, IOLINK_MHMODE_PREOPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_checkSerNum, sm_get_state (port));

   mock_iolink_al_read_cnf_cb (port, sizeof(paraml.serialnumber),
                               paraml.serialnumber, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   // SerNumOk T10
   EXPECT_EQ (SM_STATE_wait, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMREADY, mock_iolink_sm_portmode);
}

TEST_F (SMTest, checkCompatibility_IL_IDENTICAL_bad_serial)
{
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;
   memset (paraml.serialnumber, 0, sizeof(paraml.serialnumber));
   paraml.serialnumber[0] = 0x11;
   paraml.serialnumber[1] = 0x22;
   paraml.serialnumber[2] = 0x33;
   paraml.serialnumber[3] = 0x44;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

   // <> V10 T21
   EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   // CompOK T2
   EXPECT_EQ (SM_STATE_waitonDLPreoperate, sm_get_state (port));
   DL_Mode_ind (port, IOLINK_MHMODE_PREOPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_checkSerNum, sm_get_state (port));

   paraml.serialnumber[4] = 0x66; // bad serial number
   mock_iolink_al_read_cnf_cb (port, sizeof(paraml.serialnumber),
                               paraml.serialnumber, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   // SerNumFault T11
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_SERNUM_FAULT, mock_iolink_sm_portmode);

   EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
}

TEST_F (SMTest, checkCompatibility_IL_IDENTICAL_bad_vid)
{
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   // Bad vid
   paraml.vendorid = 0x6666;
   paraml.deviceid = mock_iolink_deviceid;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

   // <> V10 T21
   EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
   // CompFault T7
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

   EXPECT_EQ (1, mock_iolink_mastercmd_preop_cnt);
   // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
   mock_iolink_job.callback (&mock_iolink_job);
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
}

TEST_F (SMTest, checkCompatibility_IL_IDENTICAL_bad_did)
{
   int i;
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   // Bad did
   paraml.deviceid = 0x666666;

   for (i = 0; i < 5; i++)
   {
      sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

      EXPECT_EQ (SM_STATE_RestartDevice, sm_get_state (port));
      mock_iolink_job.callback (&mock_iolink_job); // write_cnf IOL_DIR_PARAMA_REV_ID config_paramlist->revisionid
      mock_iolink_job.callback (&mock_iolink_job); // write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_IDENT
      EXPECT_EQ (SM_STATE_RestartDevice, sm_get_state (port));
      mock_iolink_job.callback (&mock_iolink_job); // Dummy DL_Read_cnf
      EXPECT_EQ (SM_STATE_ReadComParameter, sm_get_state (port));
      EXPECT_EQ (((i + 1) * 2) - 1, mock_iolink_mastercmd_master_ident_cnt);

      sm_verify_readcomparameters (port);
      // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD
      mock_iolink_job.callback (&mock_iolink_job);
      sm_verify_checkcomp_params (port, true);

      // <> V10 T21
      EXPECT_EQ ((i + 1) * 2, mock_iolink_mastercmd_master_ident_cnt);
      // RetryStartup T23
      EXPECT_EQ (i + 1, mock_iolink_restart_device_cnt);
      // CompFault T7
      EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

      EXPECT_EQ (i + 1, mock_iolink_mastercmd_preop_cnt);
      // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
      mock_iolink_job.callback (&mock_iolink_job);
      // Verify reported SM_PortMode_ind()
      EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

      sm_set_portconfig_inactive (port);
   }
}

TEST_F (SMTest, checkCompatibility_IL_IDENTICAL_bad_cyc_time)
{
   iolink_smp_parameterlist_t paraml;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   // Bad cycle time
   paraml.cycletime = mock_iolink_min_cycletime - 1;
   paraml.revisionid = IOL_DIR_PARAM_REV_V11;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

   // <> V10 T21
   EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
   // CycTimeFault T17
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

   EXPECT_EQ (1, mock_iolink_mastercmd_preop_cnt);
   // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
   mock_iolink_job.callback (&mock_iolink_job);
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_CYCTIME_FAULT, mock_iolink_sm_portmode);

   DL_Mode_ind (port, IOLINK_MHMODE_PREOPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
}

TEST_F (SMTest, checkCompatibilityV10_IL_TYPE_COMP)
{
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V10;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V10;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, true);

   // V10CompOK T4
   EXPECT_EQ (SM_STATE_waitonDLOperate, sm_get_state (port));
   EXPECT_EQ (0, mock_iolink_mastercmd_master_ident_cnt);
   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
}

TEST_F (SMTest, checkCompatibilityV10_IL_TYPE_COMP_bad_vid)
{
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V10;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V10;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   // Bad vid
   paraml.vendorid = 0x6666;
   paraml.deviceid = mock_iolink_deviceid;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, true);

   // V10CompFault T5
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   EXPECT_EQ (0, mock_iolink_mastercmd_master_ident_cnt);
}

TEST_F (SMTest, checkCompatibilityV10_IL_TYPE_COMP_bad_did)
{
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V10;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V10;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   // Bad did
   paraml.deviceid = 0x666666;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, true);

   // V10CompFault T5
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   EXPECT_EQ (0, mock_iolink_mastercmd_master_ident_cnt);
}

TEST_F (SMTest, checkCompatibilityV10_IL_TYPE_COMP_bad_cyc_time)
{
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V10;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
   // Bad cycle time
   paraml.cycletime = mock_iolink_min_cycletime - 1;
   paraml.revisionid = IOL_DIR_PARAM_REV_V10;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, true);

   // V10CycTimeFault T18
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_CYCTIME_FAULT, mock_iolink_sm_portmode);

   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   EXPECT_EQ (0, mock_iolink_mastercmd_master_ident_cnt);
}

TEST_F (SMTest, checkCompatibilityV10_IL_IDENTICAL)
{
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V10;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V10;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, true);

   // V10CompOK T4
   EXPECT_EQ (SM_STATE_waitonDLOperate, sm_get_state (port));
   EXPECT_EQ (0, mock_iolink_mastercmd_master_ident_cnt);
   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
}

TEST_F (SMTest, checkCompatibilityV10_IL_IDENTICAL_bad_vid)
{
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V10;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V10;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   // Bad vid
   paraml.vendorid = 0x6666;
   paraml.deviceid = mock_iolink_deviceid;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, true);

   // V10CompFault T5
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   EXPECT_EQ (0, mock_iolink_mastercmd_master_ident_cnt);
}

TEST_F (SMTest, checkCompatibilityV10_IL_IDENTICAL_bad_did)
{
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V10;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V10;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   // Bad did
   paraml.deviceid = 0x666666;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, true);

   // V10CompFault T5
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   EXPECT_EQ (0, mock_iolink_mastercmd_master_ident_cnt);
}

TEST_F (SMTest, checkCompatibilityV10_IL_IDENTICAL_bad_serial)
{
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V10;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   paraml.cycletime = 0;
   paraml.revisionid = IOL_DIR_PARAM_REV_V10;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;
   memset (paraml.serialnumber, 0, sizeof(paraml.serialnumber));
   paraml.serialnumber[0] = 0x11;
   paraml.serialnumber[1] = 0x22;
   paraml.serialnumber[2] = 0x33;
   paraml.serialnumber[3] = 0x44;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, true);

   // V10CompOK T4
   EXPECT_EQ (SM_STATE_waitonDLOperate, sm_get_state (port));
   EXPECT_EQ (0, mock_iolink_mastercmd_master_ident_cnt);
   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   DL_Mode_ind (port, IOLINK_MHMODE_OPERATE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_checkSerNum, sm_get_state (port));

   paraml.serialnumber[4] = 0x66; // bad serial number
   mock_iolink_al_read_cnf_cb (port, sizeof(paraml.serialnumber),
                               paraml.serialnumber, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   // SerNumFault T11
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_SERNUM_FAULT, mock_iolink_sm_portmode);
}

TEST_F (SMTest, checkCompatibilityV10_IL_IDENTICAL_bad_cyc_time)
{
   iolink_smp_parameterlist_t paraml;

   mock_iolink_revisionid = IOL_DIR_PARAM_REV_V10;

   paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_IDENTICAL;
   // Bad cycle time
   paraml.cycletime = mock_iolink_min_cycletime - 1;
   paraml.revisionid = IOL_DIR_PARAM_REV_V10;
   paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
   paraml.vendorid = mock_iolink_vendorid;
   paraml.deviceid = mock_iolink_deviceid;

   sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, true);

   // V10CycTimeFault T18
   EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_CYCTIME_FAULT, mock_iolink_sm_portmode);

   EXPECT_EQ (0, mock_iolink_restart_device_cnt);
   EXPECT_EQ (0, mock_iolink_mastercmd_master_ident_cnt);
}

// Test DL_Mode_COMLOST T3
TEST_F (SMTest, Comlost)
{
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));

   sm_trans_1_2 (port);
   // DL_Mode_COMLOST T3
   DL_Mode_ind (port, IOLINK_MHMODE_COMLOST);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_waitonDLPreoperate, sm_get_state (port));
   DL_Read_cnf (port, 0, IOLINK_STATUS_NO_COMM);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMLOST, mock_iolink_sm_portmode);

   sm_trans_8_10 (port);
   // DL_Mode_COMLOST T3
   DL_Mode_ind (port, IOLINK_MHMODE_COMLOST);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMLOST, mock_iolink_sm_portmode);

   sm_trans_13 (port);
   // DL_Mode_COMLOST T3
   DL_Mode_ind (port, IOLINK_MHMODE_COMLOST);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));
   // Verify reported SM_PortMode_ind()
   EXPECT_EQ (IOLINK_SM_PORTMODE_COMLOST, mock_iolink_sm_portmode);
}

// Test SM_SetPortConfig_INACTIVE T14
TEST_F (SMTest, SM_SetPortConfig_INACTIVE)
{
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));

   // PortInactive_0 to PortInactive_0
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));
   // T14
   sm_set_portconfig_inactive (port);

   // wait_4 to PortInactive_0
   sm_trans_8_10 (port);
   // T14
   sm_set_portconfig_inactive (port);

   // SMOperate_5 to PortInactive_0
   sm_trans_12 (port);
   EXPECT_EQ (SM_STATE_SMOperate, sm_get_state (port));
   // T14
   sm_set_portconfig_inactive (port);

   // InspectionFault_6 to PortInactive_0
   {
      iolink_smp_parameterlist_t paraml;

      mock_iolink_mastercmd_master_ident_cnt = 0;
      paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
      paraml.cycletime = 0;
      paraml.revisionid = IOL_DIR_PARAM_REV_V11;
      paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
      // Bad vid
      paraml.vendorid = 0x6666;
      paraml.deviceid = mock_iolink_deviceid;

      sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

      // <> V10 T21
      EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
      // CompFault T7
      EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

      EXPECT_EQ (3, mock_iolink_mastercmd_preop_cnt);
      // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
      mock_iolink_job.callback (&mock_iolink_job);
      // Verify reported SM_PortMode_ind()
      EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

      EXPECT_EQ (0, mock_iolink_restart_device_cnt);
      // T14
      sm_set_portconfig_inactive (port);
   }

   // DIDO_8 (DI) to PortInactive_0
   sm_trans_16 (port, true);
   EXPECT_EQ (SM_STATE_DIDO, sm_get_state (port));
   // T14
   sm_set_portconfig_inactive (port);
   // DIDO_8 (DO) to PortInactive_0
   sm_trans_16 (port, false);
   EXPECT_EQ (SM_STATE_DIDO, sm_get_state (port));
   // T14
   sm_set_portconfig_inactive (port);
}

// Test SM_SetPortConfig_CFGCOM_or_AUTOCOM T15
TEST_F (SMTest, SM_SetPortConfig_CFGCOM_or_AUTOCOM)
{
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));

   // PortInactive_0 to PortInactive_0
   // Test SM_SetPortConfig_CFGCOM T15
   sm_trans_15 (port, false);
   // Test SM_SetPortConfig_AUTOCOM T15
   sm_trans_15 (port, true);

   // wait_4 to PortInactive_0
   sm_trans_8_10 (port);
   // Test SM_SetPortConfig_CFGCOM T15
   sm_trans_15 (port, false);
   sm_trans_8_10 (port);
   // Test SM_SetPortConfig_AUTOCOM T15
   sm_trans_15 (port, true);

   // SMOperate_5 to PortInactive_0
   sm_trans_12 (port);
   EXPECT_EQ (SM_STATE_SMOperate, sm_get_state (port));
   // Test SM_SetPortConfig_CFGCOM T15
   sm_trans_15 (port, false);
   sm_trans_12 (port);
   EXPECT_EQ (SM_STATE_SMOperate, sm_get_state (port));
   // Test SM_SetPortConfig_AUTOCOM T15
   sm_trans_15 (port, true);

   // InspectionFault_6 to PortInactive_0
   {
      int i;
      iolink_smp_parameterlist_t paraml;

      paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
      paraml.cycletime = 0;
      paraml.revisionid = IOL_DIR_PARAM_REV_V11;
      paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
      // Bad vid
      paraml.vendorid = 0x6666;
      paraml.deviceid = mock_iolink_deviceid;

      // i == 0 --> test SM_SetPortConfig_CFGCOM T15
      // i == 1 --> test SM_SetPortConfig_AUTOCOM T15
      for (i = 0; i < 2; i++)
      {
         mock_iolink_mastercmd_master_ident_cnt = 0;
         sm_checkCompatibility (port, &paraml, SM_STATE_PortInactive, false);

         // <> V10 T21
         EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
         // CompFault T7
         EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

         EXPECT_EQ (5 + i, mock_iolink_mastercmd_preop_cnt);
         // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
         mock_iolink_job.callback (&mock_iolink_job);
         // Verify reported SM_PortMode_ind()
         EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

         EXPECT_EQ (0, mock_iolink_restart_device_cnt);
         // T14
         sm_set_portconfig_inactive (port);
         sm_trans_15 (port, i == 1);

         sm_set_portconfig_inactive (port);
      }
   }

   // DIDO_8 to PortInactive_0
   // Test SM_SetPortConfig_CFGCOM T15
   sm_trans_16 (port, false);
   EXPECT_EQ (SM_STATE_DIDO, sm_get_state (port));
   sm_trans_15 (port, false);
   // Test SM_SetPortConfig_CFGCOM T15
   sm_trans_16 (port, true);
   EXPECT_EQ (SM_STATE_DIDO, sm_get_state (port));
   sm_trans_15 (port, false);

   // DIDO_8 to PortInactive_0
   // Test SM_SetPortConfig_AUTOCOM T15
   sm_trans_16 (port, false);
   EXPECT_EQ (SM_STATE_DIDO, sm_get_state (port));
   sm_trans_15 (port, true);
   // Test SM_SetPortConfig_AUTOCOM T15
   sm_trans_16 (port, true);
   EXPECT_EQ (SM_STATE_DIDO, sm_get_state (port));
   sm_trans_15 (port, true);
}

// Test SM_SetPortConfig_DIDO T16
TEST_F (SMTest, SM_SetPortConfig_DIDO)
{
   EXPECT_EQ (SM_STATE_PortInactive, sm_get_state (port));

   // PortInactive_0 to DIDO_8
   // Test SM_SetPortConfig_DO T16
   sm_trans_16 (port, false);
   sm_set_portconfig_inactive (port);
   // Test SM_SetPortConfig_DI T16
   sm_trans_16 (port, true);

   // wait_4 to DIDO_8
   sm_set_portconfig_inactive (port);
   sm_trans_8_10 (port);
   // Test SM_SetPortConfig_DI T16
   sm_trans_16 (port, true);
   sm_set_portconfig_inactive (port);
   sm_trans_8_10 (port);
   // Test SM_SetPortConfig_DO T16
   sm_trans_16 (port, false);

   // SMOperate_5 to DIDO_8
   sm_set_portconfig_inactive (port);
   sm_trans_12 (port);
   EXPECT_EQ (SM_STATE_SMOperate, sm_get_state (port));
   // Test SM_SetPortConfig_DI T16
   sm_trans_16 (port, true);
   sm_set_portconfig_inactive (port);
   sm_trans_12 (port);
   EXPECT_EQ (SM_STATE_SMOperate, sm_get_state (port));
   // Test SM_SetPortConfig_DO T16
   sm_trans_16 (port, false);


   // InspectionFault_6 to DIDO_8
   {
      int i;
      iolink_smp_parameterlist_t paraml;

      paraml.inspectionlevel = IOLINK_INSPECTIONLEVEL_TYPE_COMP;
      paraml.cycletime = 0;
      paraml.revisionid = IOL_DIR_PARAM_REV_V11;
      paraml.mode = IOLINK_SMTARGET_MODE_AUTOCOM;
      // Bad vid
      paraml.vendorid = 0x6666;
      paraml.deviceid = mock_iolink_deviceid;

      // i == 0 --> test SM_SetPortConfig_DO T16
      // i == 1 --> test SM_SetPortConfig_DI T16
      for (i = 0; i < 2; i++)
      {
         mock_iolink_mastercmd_master_ident_cnt = 0;
         sm_checkCompatibility (port, &paraml, SM_STATE_DIDO, false);

         // <> V10 T21
         EXPECT_EQ (1, mock_iolink_mastercmd_master_ident_cnt);
         // CompFault T7
         EXPECT_EQ (SM_STATE_InspectionFault, sm_get_state (port));

         EXPECT_EQ (5 + i, mock_iolink_mastercmd_preop_cnt);
         // DL_Write_cnf IOL_DIR_PARAMA_MASTER_CMD IOL_MASTERCMD_DEVICE_PREOP
         mock_iolink_job.callback (&mock_iolink_job);
         // Verify reported SM_PortMode_ind()
         EXPECT_EQ (IOLINK_SM_PORTMODE_COMP_FAULT, mock_iolink_sm_portmode);

         EXPECT_EQ (0, mock_iolink_restart_device_cnt);
         sm_trans_16 (port, i == 1);
      }
   }

   // DIDO_8 to DIDO_8
   sm_set_portconfig_inactive (port);
   // Test SM_SetPortConfig_DO T16
   sm_trans_16 (port, false);
   EXPECT_EQ (SM_STATE_DIDO, sm_get_state (port));
   // Test SM_SetPortConfig_DO T16
   sm_trans_16 (port, false);
   EXPECT_EQ (SM_STATE_DIDO, sm_get_state (port));
   // Test SM_SetPortConfig_DI T16
   sm_trans_16 (port, true);
   EXPECT_EQ (SM_STATE_DIDO, sm_get_state (port));
}

