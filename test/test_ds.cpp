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

#include "options.h"
#include "osal.h"
#include <gtest/gtest.h>

#include "mocks.h"
#include "iolink_ds.h"
#include "test_util.h"

// Helpers
static inline iolink_ds_state_t ds_get_state (iolink_port_t * port)
{
   iolink_ds_port_t * ds = iolink_get_ds_ctx (port);

   return ds->state;
}

static inline iolink_error_t par_serv_to_ds (
   iolink_port_t * port,
   uint16_t len,
   const uint8_t * data,
   uint16_t vid,
   uint32_t did)
{
   uint16_t arg_block_ds_data_len = sizeof (arg_block_ds_data_t) + len;
   uint8_t buf[arg_block_ds_data_len];
   arg_block_ds_data_t * arg_block_ds_data;

   memset (buf, 0, sizeof (buf));
   arg_block_ds_data = (arg_block_ds_data_t *)buf;
   memcpy (arg_block_ds_data->ds_data, data, len);

   arg_block_ds_data->arg_block_id = IOLINK_ARG_BLOCK_ID_DS_DATA;
   arg_block_ds_data->vid          = vid;
   arg_block_ds_data->did          = did;

   return SMI_ParServToDS_req (
      iolink_get_portnumber (port),
      IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
      arg_block_ds_data_len,
      (arg_block_t *)arg_block_ds_data);
}

static inline void ds_verify_id (iolink_port_t * port, uint16_t vid, uint32_t did)
{
   iolink_ds_port_t * ds = iolink_get_ds_ctx (port);

   EXPECT_EQ (vid, ds->master_ds.vid);
   EXPECT_EQ (did, ds->master_ds.did);
}

static inline void ds_check_fault (
   iolink_port_t * port,
   iolink_ds_fault_t expected_fault)
{
   if (expected_fault == IOLINK_DS_FAULT_DOWN || expected_fault == IOLINK_DS_FAULT_UP)
   {
      EXPECT_EQ (DS_STATE_DS_Fault, ds_get_state (port));
      // Write DS_CMD_BREAK
      mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
      mock_iolink_job.callback (&mock_iolink_job);
   }

   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));
   EXPECT_EQ (expected_fault, mock_iolink_ds_fault);
   EXPECT_EQ (1, mock_iolink_ds_fault_cnt);
   EXPECT_EQ (0, mock_iolink_ds_ready_cnt);
}

static inline void ds_state_0_to_4 (
   iolink_port_t * port,
   uint16_t len,
   const uint8_t * data,
   uint16_t vid,
   uint32_t did,
   iolink_validation_check_t validation)
{
   iolink_port_info_t * port_info = iolink_get_port_info (port);
   portconfiglist_t cfg_list;

   if (len && data)
   {
      EXPECT_EQ (IOLINK_ERROR_NONE, par_serv_to_ds (port, len, data, vid, did));
   }
   ds_verify_id (port, vid, did);

   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.portmode          = IOLINK_PORTMODE_IOL_MAN;
   cfg_list.validation_backup = validation;

   EXPECT_EQ (DS_STATE_CheckActivationState, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Init (port, &cfg_list));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));

   port_info->vendorid = mock_iolink_vendorid;
   port_info->deviceid = mock_iolink_deviceid;

   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Startup (port));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_CheckIdentity, ds_get_state (port));
   EXPECT_EQ (1, mock_iolink_al_read_req_cnt);
}

static inline void ds_state_0_to_5 (
   iolink_port_t * port,
   uint16_t len,
   const uint8_t * data,
   uint16_t vid,
   uint32_t did,
   uint8_t * ds_size,
   iolink_validation_check_t validation)
{
   ds_state_0_to_4 (port, len, data, vid, did, validation);

   // Read device Data_Storage_Size
   mock_iolink_al_read_cnf_cb (port, 4, ds_size, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
}

static inline void ds_state_0_to_6 (
   iolink_port_t * port,
   uint16_t len,
   const uint8_t * data,
   uint16_t vid,
   uint32_t did,
   uint8_t * ds_size,
   iolink_validation_check_t validation,
   uint8_t * state_property)
{
   ds_state_0_to_5 (port, len, data, vid, did, ds_size, validation);

   EXPECT_EQ (DS_STATE_CheckMemSize, ds_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 2);
   // Read device State_Property
   mock_iolink_al_read_cnf_cb (port, 1, state_property, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
}

static inline void ds_state_0_to_10 (
   iolink_port_t * port,
   uint16_t len,
   const uint8_t * data,
   uint16_t vid,
   uint32_t did,
   uint8_t * ds_size,
   iolink_validation_check_t validation,
   uint8_t * state_property)
{
   uint32_t checksum[] = {123};

   ds_state_0_to_6 (port, len, data, vid, did, ds_size, validation, state_property);

   EXPECT_EQ (DS_STATE_CheckUpload, ds_get_state (port));
   // Read checksum
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (checksum),
      (uint8_t *)checksum,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_Decompose_Set, ds_get_state (port));
   // Write DS_CMD_DL_START
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
}

// Test fixture

class DSTest : public TestBase
{
 protected:
   // Override default setup
   virtual void SetUp()
   {
      TestBase::SetUp(); // Re-use default setup
   };
};

TEST_F (DSTest, Off_DS_Startup)
{
   portconfiglist_t cfg_list;

   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.portmode = IOLINK_PORTMODE_IOL_AUTO;

   EXPECT_EQ (DS_STATE_CheckActivationState, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Init (port, &cfg_list));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_Off, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Startup (port));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_Off, ds_get_state (port));

   EXPECT_EQ (1, mock_iolink_ds_ready_cnt);
}

TEST_F (DSTest, Off_DS_Delete)
{
   portconfiglist_t cfg_list;
   uint8_t data_store_data[] = {0x0, 0x21, 0x0, 0x01, 0x2};
   uint16_t vid              = 123;
   uint32_t did              = 456;

   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      par_serv_to_ds (port, sizeof (data_store_data), data_store_data, vid, did));

   ds_verify_id (port, vid, did);
   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.portmode          = IOLINK_PORTMODE_IOL_MAN;
   cfg_list.validation_backup = IOLINK_VALIDATION_CHECK_V11;

   EXPECT_EQ (DS_STATE_CheckActivationState, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Init (port, &cfg_list));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_Off, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Delete (port));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_Off, ds_get_state (port));
   ds_verify_id (port, 0, 0);
}

TEST_F (DSTest, Off_DS_Upload)
{
   portconfiglist_t cfg_list;

   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.portmode          = IOLINK_PORTMODE_IOL_MAN;
   cfg_list.validation_backup = IOLINK_VALIDATION_CHECK_V10;

   EXPECT_EQ (DS_STATE_CheckActivationState, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Init (port, &cfg_list));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_Off, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Upload (port));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_Off, ds_get_state (port));
}

TEST_F (DSTest, DS_wait_ds_act_DS_Delete)
{
   uint8_t data_store_data[] = {0x0, 0x21, 0x0, 0x01, 0x2};
   uint16_t vid              = 123;
   uint32_t did              = 456;

   portconfiglist_t cfg_list;

   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      par_serv_to_ds (port, sizeof (data_store_data), data_store_data, vid, did));

   ds_verify_id (port, vid, did);
   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.portmode = IOLINK_PORTMODE_IOL_AUTO;

   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.portmode          = IOLINK_PORTMODE_IOL_MAN;
   cfg_list.validation_backup = IOLINK_VALIDATION_CHECK_V11_RESTORE;

   EXPECT_EQ (DS_STATE_CheckActivationState, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Init (port, &cfg_list));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));

   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Delete (port));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));
   ds_verify_id (port, 0, 0);
}

TEST_F (DSTest, DS_download)
{
   uint8_t data_storage_size[] = {0, 0, 8, 0}; // 2048 B
   uint8_t data_store_data[]   = {
      0x01,
      0x23,
      0x00,
      0x02,
      0x12,
      0x34,
      0x01,
      0x24,
      0x01,
      0x01,
      0x55,
      0x01,
      0x24,
      0x02,
      0x01,
      0xaa,
   };
   uint16_t vid = mock_iolink_vendorid;
   uint32_t did = mock_iolink_deviceid;

   uint32_t checksum[]      = {123};
   uint8_t state_property[] = {0};

   int i;

   ds_state_0_to_10 (
      port,
      sizeof (data_store_data),
      data_store_data,
      vid,
      did,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_RESTORE,
      state_property);

   for (i = 0; i < 3; i++) // Number of paramters in DS
   {
      EXPECT_EQ (DS_STATE_Write_Parameter, ds_get_state (port));
      mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
      mock_iolink_job.callback (&mock_iolink_job);
   }
   EXPECT_EQ (DS_STATE_Download_Done, ds_get_state (port));
   // Write DS_CMD_DL_END
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_Download_Done, ds_get_state (port));
   // Read checksum
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (checksum),
      (uint8_t *)checksum,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));

   EXPECT_EQ (1, mock_iolink_ds_ready_cnt);
}

TEST_F (DSTest, DS_download_upload_req)
{
   uint8_t data_storage_size[] = {0, 0, 8, 0}; // 2048 B
   uint8_t data_store_data[]   = {
      0x01,
      0x23,
      0x00,
      0x02,
      0x12,
      0x34,
      0x01,
      0x24,
      0x01,
      0x01,
      0x55,
      0x01,
      0x24,
      0x02,
      0x01,
      0xaa,
   };
   uint16_t vid = mock_iolink_vendorid;
   uint32_t did = mock_iolink_deviceid;

   uint32_t checksum[]      = {123};
   uint8_t state_property[] = {DS_STATE_PROPERTY_UPLOAD_REQ};

   int i;

   ds_state_0_to_10 (
      port,
      sizeof (data_store_data),
      data_store_data,
      vid,
      did,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_RESTORE,
      state_property);

   for (i = 0; i < 3; i++) // Number of paramters in DS
   {
      EXPECT_EQ (DS_STATE_Write_Parameter, ds_get_state (port));
      mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
      mock_iolink_job.callback (&mock_iolink_job);
   }
   EXPECT_EQ (DS_STATE_Download_Done, ds_get_state (port));
   // Write DS_CMD_DL_END
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_Download_Done, ds_get_state (port));
   // Read checksum
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (checksum),
      (uint8_t *)checksum,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));

   EXPECT_EQ (1, mock_iolink_ds_ready_cnt);
}

#include <arpa/inet.h>
TEST_F (DSTest, DS_upload_upload_req)
{
   uint8_t data_storage_size[]        = {0, 0, 8, 0}; // 2048 B
   uint8_t state_property[]           = {DS_STATE_PROPERTY_UPLOAD_REQ};
   uint32_t checksum[]                = {123};
   ds_index_list_entry_t index_list[] = {
      {
         .index    = htons (0x399),
         .subindex = 2,
      },
      {
         .index    = 2,
         .subindex = 3,
      },
      {
         .index    = htons (0x345),
         .subindex = 0,
      },
      {
         .index    = 0, /* Index_List termination */
         .subindex = 0,
      },
   };
   uint8_t i;

   ds_state_0_to_5 (
      port,
      0,
      NULL,
      0,
      0,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_BAK_RESTORE);

   EXPECT_EQ (DS_STATE_CheckMemSize, ds_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 2);
   // Read device State_Property
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (state_property),
      state_property,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_CheckUpload, ds_get_state (port));

   // Write DS_CMD_UL_START
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));
   // Read device DS Index_List
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (index_list),
      (uint8_t *)index_list,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   // Read foreach element in index list
   for (i = 0; i < ARRAY_SIZE (index_list) - 1; i++)
   {
      ds_index_list_entry_t * il_entry = &index_list[i];
      uint8_t param_data[]             = {1, 2, 3, 4};

      EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));

      EXPECT_EQ (htons (il_entry->index), mock_iolink_al_data_index);
      EXPECT_EQ (il_entry->subindex, mock_iolink_al_data_subindex);

      mock_iolink_al_read_cnf_cb (
         port,
         sizeof (param_data),
         param_data,
         IOLINK_SMI_ERRORTYPE_NONE);
      mock_iolink_job.callback (&mock_iolink_job);
   }

   EXPECT_EQ (DS_STATE_StoreDataSet, ds_get_state (port));
   // Read checksum
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (checksum),
      (uint8_t *)checksum,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_StoreDataSet, ds_get_state (port));
   // Write UL_End
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));

   EXPECT_EQ (1, mock_iolink_ds_ready_cnt);
}

TEST_F (DSTest, DS_upload_DS_invalid)
{
   uint8_t data_storage_size[]        = {0, 0, 8, 0}; // 2048 B
   uint8_t state_property[]           = {0};
   uint32_t checksum[]                = {123};
   ds_index_list_entry_t index_list[] = {
      {
         .index    = htons (0x399),
         .subindex = 2,
      },
      {
         .index    = 2,
         .subindex = 3,
      },
      {
         .index    = htons (0x345),
         .subindex = 0,
      },
      {
         .index    = 0, /* Index_List termination */
         .subindex = 0,
      },
   };
   uint8_t i;

   ds_state_0_to_5 (
      port,
      0,
      NULL,
      0,
      0,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_BAK_RESTORE);

   EXPECT_EQ (DS_STATE_CheckMemSize, ds_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 2);
   // Read device State_Property
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (state_property),
      state_property,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_CheckUpload, ds_get_state (port));

   // Read checksum
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (checksum),
      (uint8_t *)checksum,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   // Write DS_CMD_UL_START
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));
   // Read device DS Index_List
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (index_list),
      (uint8_t *)index_list,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   // Read foreach element in index list
   for (i = 0; i < ARRAY_SIZE (index_list) - 1; i++)
   {
      ds_index_list_entry_t * il_entry = &index_list[i];
      uint8_t param_data[]             = {1, 2, 3, 4};

      EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));

      EXPECT_EQ (htons (il_entry->index), mock_iolink_al_data_index);
      EXPECT_EQ (il_entry->subindex, mock_iolink_al_data_subindex);

      mock_iolink_al_read_cnf_cb (
         port,
         sizeof (param_data),
         param_data,
         IOLINK_SMI_ERRORTYPE_NONE);
      mock_iolink_job.callback (&mock_iolink_job);
   }

   EXPECT_EQ (DS_STATE_StoreDataSet, ds_get_state (port));
   // Read checksum
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (checksum),
      (uint8_t *)checksum,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_StoreDataSet, ds_get_state (port));
   // Write UL_End
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));

   EXPECT_EQ (1, mock_iolink_ds_ready_cnt);
}

TEST_F (DSTest, DS_Fault_ID_vid)
{
   uint8_t data_store_data[] = {0x0, 0x21, 0x0, 0x01, 0x2};
   uint16_t vid              = 1;
   uint32_t did              = mock_iolink_deviceid;

   iolink_port_info_t * port_info = iolink_get_port_info (port);

   portconfiglist_t cfg_list;

   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      par_serv_to_ds (port, sizeof (data_store_data), data_store_data, vid, did));
   ds_verify_id (port, vid, did);

   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.portmode          = IOLINK_PORTMODE_IOL_MAN;
   cfg_list.validation_backup = IOLINK_VALIDATION_CHECK_V11_RESTORE;

   EXPECT_EQ (DS_STATE_CheckActivationState, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Init (port, &cfg_list));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));

   port_info->vendorid = mock_iolink_vendorid;
   port_info->deviceid = mock_iolink_deviceid;

   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Startup (port));
   mock_iolink_job.callback (&mock_iolink_job);

   ds_check_fault (port, IOLINK_DS_FAULT_ID);
}

TEST_F (DSTest, DS_Fault_ID)
{
   uint8_t data_store_data[] = {0x0, 0x21, 0x0, 0x01, 0x2};
   uint16_t vid              = mock_iolink_vendorid;
   uint32_t did              = 2;

   iolink_port_info_t * port_info = iolink_get_port_info (port);

   portconfiglist_t cfg_list;

   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      par_serv_to_ds (port, sizeof (data_store_data), data_store_data, vid, did));
   ds_verify_id (port, vid, did);

   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.portmode          = IOLINK_PORTMODE_IOL_MAN;
   cfg_list.validation_backup = IOLINK_VALIDATION_CHECK_V11_RESTORE;

   EXPECT_EQ (DS_STATE_CheckActivationState, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Init (port, &cfg_list));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));

   port_info->vendorid = mock_iolink_vendorid;
   port_info->deviceid = mock_iolink_deviceid;

   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Startup (port));
   mock_iolink_job.callback (&mock_iolink_job);

   ds_check_fault (port, IOLINK_DS_FAULT_ID);
}

TEST_F (DSTest, DS_Fault_ID_read_error)
{
   iolink_port_info_t * port_info = iolink_get_port_info (port);

   portconfiglist_t cfg_list;

   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.portmode          = IOLINK_PORTMODE_IOL_MAN;
   cfg_list.validation_backup = IOLINK_VALIDATION_CHECK_V11_RESTORE;

   EXPECT_EQ (DS_STATE_CheckActivationState, ds_get_state (port));
   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Init (port, &cfg_list));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_WaitingOnDSActivity, ds_get_state (port));

   port_info->vendorid = mock_iolink_vendorid;
   port_info->deviceid = mock_iolink_deviceid;

   EXPECT_EQ (IOLINK_ERROR_NONE, DS_Startup (port));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_CheckIdentity, ds_get_state (port));
   EXPECT_EQ (1, mock_iolink_al_read_req_cnt);

   // Read device Data_Storage_Size, Fault
   mock_iolink_al_read_cnf_cb (port, 0, NULL, IOLINK_SMI_ERRORTYPE_IDX_NOTAVAIL);
   mock_iolink_job.callback (&mock_iolink_job);

   ds_check_fault (port, IOLINK_DS_FAULT_ID);
}

TEST_F (DSTest, DS_Fault_Size)
{
   uint8_t data_storage_size[] = {0, 0, 8, 1}; // 2049 B
   uint8_t data_store_data[]   = {0x0, 0x21, 0x0, 0x01, 0x2};
   uint16_t vid                = 0;
   uint32_t did                = 0;

   ds_state_0_to_5 (
      port,
      sizeof (data_store_data),
      data_store_data,
      vid,
      did,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_RESTORE);

   ds_check_fault (port, IOLINK_DS_FAULT_SIZE);
}

TEST_F (DSTest, DS_Fault_Size_read_error)
{
   uint8_t data_storage_size[] = {0, 0, 8, 0}; // 2048 B
   uint8_t data_store_data[]   = {0x0, 0x21, 0x0, 0x01, 0x2};
   uint16_t vid                = 0;
   uint32_t did                = 0;

   ds_state_0_to_5 (
      port,
      sizeof (data_store_data),
      data_store_data,
      vid,
      did,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_RESTORE);

   EXPECT_EQ (DS_STATE_CheckMemSize, ds_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 2);

   // Read device State_Property, fault
   mock_iolink_al_read_cnf_cb (port, 0, NULL, IOLINK_SMI_ERRORTYPE_IDX_NOTAVAIL);
   mock_iolink_job.callback (&mock_iolink_job);

   ds_check_fault (port, IOLINK_DS_FAULT_SIZE);
}

TEST_F (DSTest, DS_Fault_Lock)
{
   uint8_t state_property[]    = {DS_STATE_PROPERTY_STATE_LOCKED};
   uint8_t data_storage_size[] = {0, 0, 8, 0}; // 2048 B
   uint8_t data_store_data[]   = {0x0, 0x21, 0x0, 0x01, 0x2};
   uint16_t vid                = 0;
   uint32_t did                = 0;

   ds_state_0_to_5 (
      port,
      sizeof (data_store_data),
      data_store_data,
      vid,
      did,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_RESTORE);

   EXPECT_EQ (DS_STATE_CheckMemSize, ds_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 2);

   // Read device State_Property
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (state_property),
      state_property,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   ds_check_fault (port, IOLINK_DS_FAULT_LOCK);
}

TEST_F (DSTest, DS_Fault_UL_Read_Param_Dev_Err)
{
   uint8_t data_storage_size[]        = {0, 0, 8, 0}; // 2048 B
   uint8_t state_property[]           = {DS_STATE_PROPERTY_UPLOAD_REQ};
   ds_index_list_entry_t index_list[] = {
      {
         .index    = htons (0x399),
         .subindex = 2,
      },
      {
         .index    = 2,
         .subindex = 3,
      },
      {
         .index    = htons (0x345),
         .subindex = 0,
      },
      {
         .index    = 0, /* Index_List termination */
         .subindex = 0,
      },
   };

   uint8_t i;

   ds_state_0_to_5 (
      port,
      0,
      NULL,
      0,
      0,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_BAK_RESTORE);

   EXPECT_EQ (DS_STATE_CheckMemSize, ds_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 2);
   // Read device State_Property
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (state_property),
      state_property,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_CheckUpload, ds_get_state (port));

   // Write DS_CMD_UL_START
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));
   // Read device DS Index_List
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (index_list),
      (uint8_t *)index_list,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   // Read foreach element in index list
   for (i = 0; i < 2; i++)
   {
      ds_index_list_entry_t * il_entry = &index_list[i];
      uint8_t param_data[]             = {1, 2, 3, 4};

      EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));

      EXPECT_EQ (htons (il_entry->index), mock_iolink_al_data_index);
      EXPECT_EQ (il_entry->subindex, mock_iolink_al_data_subindex);

      if (i == 0)
         mock_iolink_al_read_cnf_cb (
            port,
            sizeof (param_data),
            param_data,
            IOLINK_SMI_ERRORTYPE_NONE);
      else
         mock_iolink_al_read_cnf_cb (
            port,
            sizeof (param_data),
            param_data,
            IOLINK_SMI_ERRORTYPE_IDX_NOTAVAIL);

      mock_iolink_job.callback (&mock_iolink_job);
   }

   ds_check_fault (port, IOLINK_DS_FAULT_UP);
}

TEST_F (DSTest, DS_Fault_UL_Read_Param_Com_Err)
{
   uint8_t data_storage_size[]        = {0, 0, 8, 0}; // 2048 B
   uint8_t state_property[]           = {DS_STATE_PROPERTY_UPLOAD_REQ};
   ds_index_list_entry_t index_list[] = {
      {
         .index    = htons (0x399),
         .subindex = 2,
      },
      {
         .index    = 2,
         .subindex = 3,
      },
      {
         .index    = htons (0x345),
         .subindex = 0,
      },
      {
         .index    = 0, /* Index_List termination */
         .subindex = 0,
      },
   };

   uint8_t i;

   ds_state_0_to_5 (
      port,
      0,
      NULL,
      0,
      0,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_BAK_RESTORE);

   EXPECT_EQ (DS_STATE_CheckMemSize, ds_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 2);
   // Read device State_Property
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (state_property),
      state_property,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_CheckUpload, ds_get_state (port));

   // Write DS_CMD_UL_START
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));
   // Read device DS Index_List
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (index_list),
      (uint8_t *)index_list,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   // Read foreach element in index list
   for (i = 0; i < 2; i++)
   {
      ds_index_list_entry_t * il_entry = &index_list[i];
      uint8_t param_data[]             = {1, 2, 3, 4};

      EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));

      EXPECT_EQ (htons (il_entry->index), mock_iolink_al_data_index);
      EXPECT_EQ (il_entry->subindex, mock_iolink_al_data_subindex);

      if (i == 0)
         mock_iolink_al_read_cnf_cb (
            port,
            sizeof (param_data),
            param_data,
            IOLINK_SMI_ERRORTYPE_NONE);
      else
         mock_iolink_al_read_cnf_cb (
            port,
            sizeof (param_data),
            param_data,
            IOLINK_SMI_ERRORTYPE_COM_ERR);

      mock_iolink_job.callback (&mock_iolink_job);
   }

   ds_check_fault (port, IOLINK_DS_FAULT_UP);
}

TEST_F (DSTest, DS_Fault_UL_StoreDataSet_Com_Err_Read)
{
   uint8_t data_storage_size[]        = {0, 0, 8, 0}; // 2048 B
   uint8_t state_property[]           = {DS_STATE_PROPERTY_UPLOAD_REQ};
   uint32_t checksum[]                = {123};
   ds_index_list_entry_t index_list[] = {
      {
         .index    = htons (0x399),
         .subindex = 2,
      },
      {
         .index    = 2,
         .subindex = 3,
      },
      {
         .index    = htons (0x345),
         .subindex = 0,
      },
      {
         .index    = 0, /* Index_List termination */
         .subindex = 0,
      },
   };
   uint8_t i;

   ds_state_0_to_5 (
      port,
      0,
      NULL,
      0,
      0,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_BAK_RESTORE);

   EXPECT_EQ (DS_STATE_CheckMemSize, ds_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 2);
   // Read device State_Property
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (state_property),
      state_property,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_CheckUpload, ds_get_state (port));

   // Write DS_CMD_UL_START
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));
   // Read device DS Index_List
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (index_list),
      (uint8_t *)index_list,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   // Read foreach element in index list
   for (i = 0; i < ARRAY_SIZE (index_list) - 1; i++)
   {
      ds_index_list_entry_t * il_entry = &index_list[i];
      uint8_t param_data[]             = {1, 2, 3, 4};

      EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));

      EXPECT_EQ (htons (il_entry->index), mock_iolink_al_data_index);
      EXPECT_EQ (il_entry->subindex, mock_iolink_al_data_subindex);

      mock_iolink_al_read_cnf_cb (
         port,
         sizeof (param_data),
         param_data,
         IOLINK_SMI_ERRORTYPE_NONE);
      mock_iolink_job.callback (&mock_iolink_job);
   }

   EXPECT_EQ (DS_STATE_StoreDataSet, ds_get_state (port));
   // Read checksum
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (checksum),
      (uint8_t *)checksum,
      IOLINK_SMI_ERRORTYPE_COM_ERR);
   mock_iolink_job.callback (&mock_iolink_job);

   ds_check_fault (port, IOLINK_DS_FAULT_UP);
}

TEST_F (DSTest, DS_Fault_UL_StoreDataSet_Com_Err_Write)
{
   uint8_t data_storage_size[]        = {0, 0, 8, 0}; // 2048 B
   uint8_t state_property[]           = {DS_STATE_PROPERTY_UPLOAD_REQ};
   uint32_t checksum[]                = {123};
   ds_index_list_entry_t index_list[] = {
      {
         .index    = htons (0x399),
         .subindex = 2,
      },
      {
         .index    = 2,
         .subindex = 3,
      },
      {
         .index    = htons (0x345),
         .subindex = 0,
      },
      {
         .index    = 0, /* Index_List termination */
         .subindex = 0,
      },
   };
   uint8_t i;

   ds_state_0_to_5 (
      port,
      0,
      NULL,
      0,
      0,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_BAK_RESTORE);

   EXPECT_EQ (DS_STATE_CheckMemSize, ds_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 2);
   // Read device State_Property
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (state_property),
      state_property,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (DS_STATE_CheckUpload, ds_get_state (port));

   // Write DS_CMD_UL_START
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));
   // Read device DS Index_List
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (index_list),
      (uint8_t *)index_list,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   // Read foreach element in index list
   for (i = 0; i < ARRAY_SIZE (index_list) - 1; i++)
   {
      ds_index_list_entry_t * il_entry = &index_list[i];
      uint8_t param_data[]             = {1, 2, 3, 4};

      EXPECT_EQ (DS_STATE_ReadParameter, ds_get_state (port));

      EXPECT_EQ (htons (il_entry->index), mock_iolink_al_data_index);
      EXPECT_EQ (il_entry->subindex, mock_iolink_al_data_subindex);

      mock_iolink_al_read_cnf_cb (
         port,
         sizeof (param_data),
         param_data,
         IOLINK_SMI_ERRORTYPE_NONE);
      mock_iolink_job.callback (&mock_iolink_job);
   }

   EXPECT_EQ (DS_STATE_StoreDataSet, ds_get_state (port));
   // Read checksum
   mock_iolink_al_read_cnf_cb (
      port,
      sizeof (checksum),
      (uint8_t *)checksum,
      IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (DS_STATE_StoreDataSet, ds_get_state (port));
   // Write UL_End, fail with COM_ERR
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_COM_ERR);
   mock_iolink_job.callback (&mock_iolink_job);

   ds_check_fault (port, IOLINK_DS_FAULT_UP);
}

TEST_F (DSTest, DS_Fault_DL_Write_Param_Dev_Err)
{
   uint8_t data_storage_size[] = {0, 0, 8, 0}; // 2048 B
   uint8_t data_store_data[]   = {
      0x01,
      0x23,
      0x00,
      0x02,
      0x12,
      0x34,
      0x01,
      0x24,
      0x01,
      0x01,
      0x55,
      0x01,
      0x24,
      0x02,
      0x01,
      0xaa,
   };
   uint16_t vid = 0;
   uint32_t did = 0;

   uint8_t state_property[] = {0};

   int i;

   ds_state_0_to_10 (
      port,
      sizeof (data_store_data),
      data_store_data,
      vid,
      did,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_BAK_RESTORE,
      state_property);

   for (i = 0; i < 2; i++)
   {
      EXPECT_EQ (DS_STATE_Write_Parameter, ds_get_state (port));
      if (i == 0)
         mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
      else
         mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_IDX_NOTAVAIL);
      mock_iolink_job.callback (&mock_iolink_job);
   }

   ds_check_fault (port, IOLINK_DS_FAULT_DOWN);
}

TEST_F (DSTest, DS_Fault_DL_Write_Param_Com_Err)
{
   uint8_t data_storage_size[] = {0, 0, 8, 0}; // 2048 B
   uint8_t data_store_data[]   = {
      0x01,
      0x23,
      0x00,
      0x02,
      0x12,
      0x34,
      0x01,
      0x24,
      0x01,
      0x01,
      0x55,
      0x01,
      0x24,
      0x02,
      0x01,
      0xaa,
   };
   uint16_t vid = 0;
   uint32_t did = 0;

   uint8_t state_property[] = {0};

   int i;

   ds_state_0_to_10 (
      port,
      sizeof (data_store_data),
      data_store_data,
      vid,
      did,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_BAK_RESTORE,
      state_property);

   for (i = 0; i < 2; i++)
   {
      EXPECT_EQ (DS_STATE_Write_Parameter, ds_get_state (port));
      if (i == 0)
         mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
      else
         mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_COM_ERR);
      mock_iolink_job.callback (&mock_iolink_job);
   }

   ds_check_fault (port, IOLINK_DS_FAULT_DOWN);
}

TEST_F (DSTest, DS_Fault_DL_Download_Done_Com_Err)
{
   uint8_t data_storage_size[] = {0, 0, 8, 0}; // 2048 B
   uint8_t data_store_data[]   = {
      0x01,
      0x23,
      0x00,
      0x02,
      0x12,
      0x34,
      0x01,
      0x24,
      0x01,
      0x01,
      0x55,
      0x01,
      0x24,
      0x02,
      0x01,
      0xaa,
   };
   uint16_t vid = 0;
   uint32_t did = 0;

   uint8_t state_property[] = {0};

   int i;

   ds_state_0_to_10 (
      port,
      sizeof (data_store_data),
      data_store_data,
      vid,
      did,
      data_storage_size,
      IOLINK_VALIDATION_CHECK_V11_BAK_RESTORE,
      state_property);

   for (i = 0; i < 3; i++) // Number of paramters in DS
   {
      EXPECT_EQ (DS_STATE_Write_Parameter, ds_get_state (port));
      mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
      mock_iolink_job.callback (&mock_iolink_job);
   }
   EXPECT_EQ (DS_STATE_Download_Done, ds_get_state (port));
   // Write DS_CMD_DL_END, respond with COM_ERR
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_COM_ERR);
   mock_iolink_job.callback (&mock_iolink_job);

   ds_check_fault (port, IOLINK_DS_FAULT_DOWN);
}

TEST_F (DSTest, DS_Chk_Cfg_vid)
{
   uint8_t data_store_data[] = {
      0x01,
      0x23,
      0x00,
      0x02,
      0x12,
      0x34,
      0x01,
      0x24,
      0x01,
      0x01,
      0x55,
      0x01,
      0x24,
      0x02,
      0x01,
      0xaa,
   };
   uint16_t vid = 123;
   uint32_t did = 456;

   portconfiglist_t cfg_list;

   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.vendorid = vid;
   cfg_list.deviceid = did - 1;

   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      par_serv_to_ds (port, sizeof (data_store_data), data_store_data, vid, did));

   EXPECT_FALSE (DS_Chk_Cfg (port, &cfg_list));
}

TEST_F (DSTest, DS_Chk_Cfg_vid_did)
{
   uint8_t data_store_data[] = {
      0x01,
      0x23,
      0x00,
      0x02,
      0x12,
      0x34,
      0x01,
      0x24,
      0x01,
      0x01,
      0x55,
      0x01,
      0x24,
      0x02,
      0x01,
      0xaa,
   };
   uint16_t vid = 123;
   uint32_t did = 456;

   portconfiglist_t cfg_list;

   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.vendorid = vid;
   cfg_list.deviceid = did;

   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      par_serv_to_ds (port, sizeof (data_store_data), data_store_data, vid, did));

   EXPECT_TRUE (DS_Chk_Cfg (port, &cfg_list));
}

TEST_F (DSTest, DS_Chk_Cfg_no_ds)
{
   uint16_t vid = 123;
   uint32_t did = 456;

   portconfiglist_t cfg_list;

   memset (&cfg_list, 0, sizeof (portconfiglist_t));
   cfg_list.vendorid = vid;
   cfg_list.deviceid = did;

   EXPECT_FALSE (DS_Chk_Cfg (port, &cfg_list));
}
