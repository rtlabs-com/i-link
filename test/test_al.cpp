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
#include "iolink_al.h"
#include "test_util.h"

// Test fixture

class ALTest : public TestBase
{
protected:
   // Override default setup
   virtual void SetUp() {
      TestBase::SetUp(); // Re-use default setup
   };
};

// Tests
TEST_F (ALTest, Al_read_param_0_1)
{
   uint16_t index = 0;
   uint8_t subindex = 0;
   uint8_t read_value = 0x12;

   for (index = 0; index < 2; index++)
   {
      iolink_al_port_t * al = iolink_get_al_ctx (port);

      EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
      AL_Read_req (port, index, subindex, mock_AL_Read_cnf);
      mock_iolink_job.callback (&mock_iolink_job);

      EXPECT_EQ (AL_OD_STATE_Await_DL_param_cnf, al->od_state);
      DL_ReadParam_cnf (port, read_value, IOLINK_STATUS_NO_ERROR);
      mock_iolink_job.callback (&mock_iolink_job);
      EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

      EXPECT_EQ (mock_iolink_al_data[0], read_value);
      EXPECT_EQ (mock_iolink_al_data_len, 1);
      EXPECT_EQ (mock_iolink_al_read_cnf_cnt, index + 1);
      EXPECT_EQ (mock_iolink_al_read_errortype, IOLINK_SMI_ERRORTYPE_NONE);
      EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 0);
      EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);

      mock_iolink_al_data[0] = 0;
      mock_iolink_al_data_len = 0;
      read_value++;
   }
}

TEST_F (ALTest, Al_read_isdu_2)
{
   iolink_al_port_t * al = iolink_get_al_ctx (port);

   uint16_t index = 2;
   uint8_t subindex = 0;
   uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
   AL_Read_req (port, index, subindex, mock_AL_Read_cnf);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (AL_OD_STATE_Await_DL_ISDU_cnf, al->od_state);
   DL_ISDUTransport_cnf (port, data, sizeof(data),
                         IOL_ISERVICE_DEVICE_READ_RESPONSE_POS,
                         IOLINK_STATUS_NO_ERROR);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_errortype, IOLINK_SMI_ERRORTYPE_NONE);
   EXPECT_EQ (mock_iolink_al_data_len, sizeof(data));
   EXPECT_TRUE (ArraysMatchN (data, mock_iolink_al_data, sizeof(data)));
   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);
}

TEST_F (ALTest, Al_write_0_err)
{
   iolink_al_port_t * al = iolink_get_al_ctx (port);

   uint16_t index = 0; /* Not allowed to write to index 0 */
   uint8_t subindex = 0;
   uint8_t data[1] = {0x34};

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
   AL_Write_req (port, index, subindex, sizeof(data), data, mock_AL_Write_cnf);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_errortype, IOLINK_SMI_ERRORTYPE_APP_DEV); // TODO what to expect here?
   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);
}

TEST_F (ALTest, Al_write_param_1)
{
   iolink_al_port_t * al = iolink_get_al_ctx (port);

   uint16_t index = 1;
   uint8_t subindex = 0;
   uint8_t data[1] = {0x34};

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
   AL_Write_req (port, index, subindex, sizeof(data), data, mock_AL_Write_cnf);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (AL_OD_STATE_Await_DL_param_cnf, al->od_state);
   DL_WriteParam_cnf (port, IOLINK_STATUS_NO_ERROR);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_errortype, IOLINK_SMI_ERRORTYPE_NONE);
   EXPECT_EQ (mock_iolink_al_data_len, sizeof(data));
   EXPECT_TRUE (ArraysMatchN (data, mock_iolink_al_data, sizeof(data)));
   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);
}

// TODO
//TEST_F (ALTest, Al_write_param_no_isdu_2)
//{
//   uint16_t index = 2;
//   uint8_t subindex = 0;
//   uint8_t data[1] = {0x34};
//
//   // TODO set NO_ISDU_SUPPORT
//
//   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
//   AL_Write_req (port, index, subindex, sizeof(data), data);
//   mock_iolink_job.callback (&mock_iolink_job);
//
//   EXPECT_EQ (AL_OD_STATE_Await_DL_param_cnf, al->od_state);
//   DL_WriteParam_cnf (port, IOLINK_STATUS_NO_ERROR);
//   mock_iolink_job.callback (&mock_iolink_job);
//   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
//
//   // TODO compare written data with "data"
//   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 1);
//   EXPECT_EQ (mock_iolink_al_write_errortype, IOLINK_SMI_ERRORTYPE_NONE);
//   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 0);
//}

TEST_F (ALTest, Al_write_param_isdu_2)
{
   iolink_al_port_t * al = iolink_get_al_ctx (port);

   uint16_t index = 2;
   uint8_t subindex = 0;
   uint8_t data[12] = {3, 2, 3, 4, 1, 5, 6, 8, 9, 1};

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
   AL_Write_req (port, index, subindex, sizeof(data), data, mock_AL_Write_cnf);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (AL_OD_STATE_Await_DL_ISDU_cnf, al->od_state);
   DL_ISDUTransport_cnf (port, data, sizeof(data),
                         IOL_ISERVICE_DEVICE_WRITE_RESPONSE_POS,
                         IOLINK_STATUS_NO_ERROR);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_errortype, IOLINK_SMI_ERRORTYPE_NONE);
   EXPECT_EQ (mock_iolink_al_data_len, sizeof(data));
   EXPECT_TRUE (ArraysMatchN (data, mock_iolink_al_data, sizeof(data)));
   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);
}

static inline void test_al_control (iolink_port_t * port,
                                    iolink_controlcode_t controlcode)
{
   iolink_al_port_t * al = iolink_get_al_ctx (port);

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
   AL_Control_req (port, controlcode);

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
   DL_Control_ind (port, controlcode);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

   EXPECT_EQ (mock_iolink_al_control_ind_cnt, 1);
   EXPECT_EQ (mock_iolink_controlcode, controlcode);
   EXPECT_EQ (mock_iolink_dl_control_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_data_len, 0);
   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 0);
}

TEST_F (ALTest, Al_control_VALID)
{
   test_al_control (port, IOLINK_CONTROLCODE_VALID);
}

TEST_F (ALTest, Al_control_INVALID)
{
   test_al_control (port, IOLINK_CONTROLCODE_INVALID);
}

TEST_F (ALTest, Al_control_PDOUTVALID)
{
   test_al_control (port, IOLINK_CONTROLCODE_PDOUTVALID);
}

TEST_F (ALTest, Al_control_PDOUTINVALID)
{
   test_al_control (port, IOLINK_CONTROLCODE_PDOUTINVALID);
}

TEST_F (ALTest, Al_abort)
{
   iolink_al_port_t * al = iolink_get_al_ctx (port);

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
   /* Abort service */
   AL_Abort (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_al_data_len, 0);
   EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);
}

TEST_F (ALTest, Al_read_param_abort_0_1)
{
   uint16_t index = 0;
   uint8_t subindex = 0;

   for (index = 0; index < 2; index++)
   {
      iolink_al_port_t * al = iolink_get_al_ctx (port);

      EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
      /* Start read service */
      AL_Read_req (port, index, subindex, mock_AL_Read_cnf);
      mock_iolink_job.callback (&mock_iolink_job);

      EXPECT_EQ (AL_OD_STATE_Await_DL_param_cnf, al->od_state);
      /* Abort service */
      AL_Abort (port);
      mock_iolink_job.callback (&mock_iolink_job);
      EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

      EXPECT_EQ (mock_iolink_al_read_cnf_cnt, index + 1);
      EXPECT_EQ (mock_iolink_al_read_errortype, IOLINK_SMI_ERRORTYPE_APP_DEV); // TODO what to expect here?
      EXPECT_EQ (mock_iolink_al_data_len, 0);
      EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 0);
      EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);
   }
}

TEST_F (ALTest, Al_read_isdu_abort_2)
{
   iolink_al_port_t * al = iolink_get_al_ctx (port);

   uint16_t index = 2;
   uint8_t subindex = 0;

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
   /* Start Read service */
   AL_Read_req (port, index, subindex, mock_AL_Read_cnf);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (AL_OD_STATE_Await_DL_ISDU_cnf, al->od_state);
   /* Abort service */
   AL_Abort (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_errortype, IOLINK_SMI_ERRORTYPE_APP_DEV); // TODO what to expect here?
   EXPECT_EQ (mock_iolink_al_data_len, 0);
   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);
}

TEST_F (ALTest, Al_write_param_abort_1)
{
   iolink_al_port_t * al = iolink_get_al_ctx (port);

   uint16_t index = 1;
   uint8_t subindex = 0;
   uint8_t data[1] = {0x34};

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
   AL_Write_req (port, index, subindex, sizeof(data), data, mock_AL_Write_cnf);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (AL_OD_STATE_Await_DL_param_cnf, al->od_state);
   /* Abort service */
   AL_Abort (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_errortype, IOLINK_SMI_ERRORTYPE_APP_DEV); // TODO what to expect here?
   /* al_data_len is assigned after AL_Write_req()
    * Hence, this is expected
    */
   EXPECT_EQ (mock_iolink_al_data_len, sizeof(data));
   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);
}

TEST_F (ALTest, Al_write_param_abort_isdu_2)
{
   iolink_al_port_t * al = iolink_get_al_ctx (port);

   uint16_t index = 2;
   uint8_t subindex = 0;
   uint8_t data[12] = {3, 2, 3, 4, 1, 5, 6, 8, 9, 1};

   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
   AL_Write_req (port, index, subindex, sizeof(data), data, mock_AL_Write_cnf);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (AL_OD_STATE_Await_DL_ISDU_cnf, al->od_state);
   /* Abort service */
   AL_Abort (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_errortype, IOLINK_SMI_ERRORTYPE_APP_DEV); // TODO what to expect here?
   /* al_data_len is assigned after AL_Write_req()
    * Hence, this is expected
    */
   EXPECT_EQ (mock_iolink_al_data_len, sizeof(data));
   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);
}

// TODO
//TEST_F (ALTest, Al_write_param_abort_no_isdu_2)


TEST_F (ALTest, Al_read_param_0_1_multiple_al_service_calls)
{
   uint16_t index = 0;
   uint8_t subindex = 0;
   uint8_t read_value = 0x12;

   for (index = 0; index < 2; index++)
   {
      iolink_al_port_t * al = iolink_get_al_ctx (port);

      EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);
      AL_Read_req (port, index, subindex, mock_AL_Read_cnf);
      mock_iolink_job.callback (&mock_iolink_job);

      EXPECT_EQ (AL_OD_STATE_Await_DL_param_cnf, al->od_state);

      AL_Read_req (port, index, subindex, mock_AL_Read_cnf);
      mock_iolink_job.callback (&mock_iolink_job);
      EXPECT_EQ (mock_iolink_al_data_len, 0);
      EXPECT_EQ (mock_iolink_al_read_cnf_cnt, (index * 2) + 1);
      EXPECT_EQ (mock_iolink_al_read_errortype,
                 IOLINK_SMI_ERRORTYPE_SERVICE_TEMP_UNAVAILABLE);

      AL_Write_req (port, index, subindex, 0, NULL, mock_AL_Write_cnf);
      mock_iolink_job.callback (&mock_iolink_job);
      EXPECT_EQ (mock_iolink_al_data_len, 0);
      EXPECT_EQ (mock_iolink_al_write_cnf_cnt, index + 1);
      EXPECT_EQ (mock_iolink_al_read_cnf_cnt, (index * 2) + 1);
      EXPECT_EQ (mock_iolink_al_read_errortype,
                 IOLINK_SMI_ERRORTYPE_SERVICE_TEMP_UNAVAILABLE);

      DL_ReadParam_cnf (port, read_value, IOLINK_STATUS_NO_ERROR);
      mock_iolink_job.callback (&mock_iolink_job);
      EXPECT_EQ (AL_OD_STATE_OnReq_Idle, al->od_state);

      EXPECT_EQ (mock_iolink_al_data[0], read_value);
      EXPECT_EQ (mock_iolink_al_data_len, 1);
      EXPECT_EQ (mock_iolink_al_read_cnf_cnt, (index * 2) + 2);
      EXPECT_EQ (mock_iolink_al_read_errortype, IOLINK_SMI_ERRORTYPE_NONE);
      EXPECT_EQ (mock_iolink_al_write_cnf_cnt, index + 1);
      EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);

      mock_iolink_al_data[0] = 0;
      mock_iolink_al_data_len = 0;
      read_value++;
   }
}

static inline void al_verify_events (iolink_port_t * port, uint8_t event_cnt,
                                     al_event_t * events)
{
   iolink_al_port_t * al = iolink_get_al_ctx (port);
   unsigned int i;

   EXPECT_EQ (AL_EVENT_STATE_Event_idle, al->event_state);

   for (i = 0; i < event_cnt; i++)
   {
      bool events_left = event_cnt - 1 - i;
      uint8_t event_qualifier;

      event_qualifier = events[i].instance & 0x7;
      event_qualifier |= (events[i].source & 0x1) << 3;
      event_qualifier |= (events[i].type & 0x3) << 4;
      event_qualifier |= (events[i].mode & 0x3) << 6;

      DL_Event_ind (port, events[i].eventcode, event_qualifier, events_left);
      mock_iolink_job.callback (&mock_iolink_job);
      if (events_left)
      {
         EXPECT_EQ (AL_EVENT_STATE_Read_Event_Set, al->event_state);
      }
   }
   // EXPECT_EQ (AL_EVENT_STATE_du_handling, al->event_state);
   // EXPECT_EQ (mock_iolink_dl_eventconf_req_cnt, 0);
   AL_Event_rsp (port);
   mock_iolink_job.callback (&mock_iolink_job);
   // EXPECT_EQ (mock_iolink_dl_eventconf_req_cnt, 1);

   EXPECT_EQ (AL_EVENT_STATE_Event_idle, al->event_state);
   EXPECT_EQ (mock_iolink_al_event_cnt, event_cnt);
   EXPECT_TRUE (EventMatch (events, mock_iolink_al_events, event_cnt));
}

TEST_F (ALTest, Al_Event_1)
{
   al_event_t events[1];

   events[0].eventcode = 1;
   events[0].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[0].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[0].type = IOLINK_EVENT_TYPE_NOTIFICATION;
   events[0].source = IOLINK_EVENT_SOURCE_DEVICE;

   al_verify_events (port, ARRAY_SIZE (events), events);
}

TEST_F (ALTest, Al_Event_2)
{
   al_event_t events[2];

   events[0].eventcode = 1;
   events[0].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[0].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[0].type = IOLINK_EVENT_TYPE_NOTIFICATION;
   events[0].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[1].eventcode = 2;
   events[1].instance = IOLINK_EVENT_INSTANCE_UNKNOWN;
   events[1].mode = IOLINK_EVENT_MODE_APPEARS;
   events[1].type = IOLINK_EVENT_TYPE_ERROR;
   events[1].source = IOLINK_EVENT_SOURCE_MASTER;
   al_verify_events (port, ARRAY_SIZE (events), events);
}

TEST_F (ALTest, Al_Event_3)
{
   al_event_t events[3];

   events[0].eventcode = 1;
   events[0].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[0].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[0].type = IOLINK_EVENT_TYPE_NOTIFICATION;
   events[0].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[1].eventcode = 2;
   events[1].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[1].mode = IOLINK_EVENT_MODE_APPEARS;
   events[1].type = IOLINK_EVENT_TYPE_NOTIFICATION;
   events[1].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[2].eventcode = 3;
   events[2].instance = IOLINK_EVENT_INSTANCE_UNKNOWN;
   events[2].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[2].type = IOLINK_EVENT_TYPE_ERROR;
   events[2].source = IOLINK_EVENT_SOURCE_DEVICE;

   al_verify_events (port, ARRAY_SIZE (events), events);
}

TEST_F (ALTest, Al_Event_4)
{
   al_event_t events[4];

   events[0].eventcode = 10;
   events[0].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[0].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[0].type = IOLINK_EVENT_TYPE_WARNING;
   events[0].source = IOLINK_EVENT_SOURCE_MASTER;

   events[1].eventcode = 12;
   events[1].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[1].mode = IOLINK_EVENT_MODE_DISAPPEARS;
   events[1].type = IOLINK_EVENT_TYPE_NOTIFICATION;
   events[1].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[2].eventcode = 14;
   events[2].instance = IOLINK_EVENT_INSTANCE_UNKNOWN;
   events[2].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[2].type = IOLINK_EVENT_TYPE_ERROR;
   events[2].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[3].eventcode = 16;
   events[3].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[3].mode = IOLINK_EVENT_MODE_APPEARS;
   events[3].type = IOLINK_EVENT_TYPE_NOTIFICATION;
   events[3].source = IOLINK_EVENT_SOURCE_DEVICE;

   al_verify_events (port, ARRAY_SIZE (events), events);
}

TEST_F (ALTest, Al_Event_5)
{
   al_event_t events[5];

   events[0].eventcode = 20;
   events[0].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[0].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[0].type = IOLINK_EVENT_TYPE_WARNING;
   events[0].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[1].eventcode = 21;
   events[1].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[1].mode = IOLINK_EVENT_MODE_DISAPPEARS;
   events[1].type = IOLINK_EVENT_TYPE_NOTIFICATION;
   events[1].source = IOLINK_EVENT_SOURCE_MASTER;

   events[2].eventcode = 22;
   events[2].instance = IOLINK_EVENT_INSTANCE_UNKNOWN;
   events[2].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[2].type = IOLINK_EVENT_TYPE_ERROR;
   events[2].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[3].eventcode = 23;
   events[3].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[3].mode = IOLINK_EVENT_MODE_APPEARS;
   events[3].type = IOLINK_EVENT_TYPE_NOTIFICATION;
   events[3].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[4].eventcode = 0xFFFF;
   events[4].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[4].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[4].type = IOLINK_EVENT_TYPE_WARNING;
   events[4].source = IOLINK_EVENT_SOURCE_DEVICE;

   al_verify_events (port, ARRAY_SIZE (events), events);
}

TEST_F (ALTest, Al_Event_6)
{
   al_event_t events[6];

   events[0].eventcode = 0x8CA0;
   events[0].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[0].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[0].type = IOLINK_EVENT_TYPE_WARNING;
   events[0].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[1].eventcode = 0x8CA1;
   events[1].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[1].mode = IOLINK_EVENT_MODE_DISAPPEARS;
   events[1].type = IOLINK_EVENT_TYPE_NOTIFICATION;
   events[1].source = IOLINK_EVENT_SOURCE_MASTER;

   events[2].eventcode = 0x8CCC;
   events[2].instance = IOLINK_EVENT_INSTANCE_UNKNOWN;
   events[2].mode = IOLINK_EVENT_MODE_APPEARS;
   events[2].type = IOLINK_EVENT_TYPE_ERROR;
   events[2].source = IOLINK_EVENT_SOURCE_MASTER;

   events[3].eventcode = 0x8DDD;
   events[3].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[3].mode = IOLINK_EVENT_MODE_APPEARS;
   events[3].type = IOLINK_EVENT_TYPE_NOTIFICATION;
   events[3].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[4].eventcode = 0x8DFE;
   events[4].instance = IOLINK_EVENT_INSTANCE_APPLICATION;
   events[4].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[4].type = IOLINK_EVENT_TYPE_WARNING;
   events[4].source = IOLINK_EVENT_SOURCE_DEVICE;

   events[5].eventcode = 0x8DFF;
   events[5].instance = IOLINK_EVENT_INSTANCE_UNKNOWN;
   events[5].mode = IOLINK_EVENT_MODE_SINGLE_SHOT;
   events[5].type = IOLINK_EVENT_TYPE_ERROR;
   events[5].source = IOLINK_EVENT_SOURCE_DEVICE;

   al_verify_events (port, ARRAY_SIZE (events), events);
}

TEST_F (ALTest, Al_SetOutput)
{
   uint8_t data[8] = {1, 2, 3, 4, 5, 6, 7, 8};

   EXPECT_EQ (IOLINK_ERROR_NONE, AL_SetOutput_req (port, data));

   EXPECT_EQ (mock_iolink_al_read_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_al_read_errortype, IOLINK_SMI_ERRORTYPE_NONE);
   EXPECT_EQ (mock_iolink_al_write_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_dl_control_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_control_ind_cnt, 0);
}

TEST_F (ALTest, Al_GetInput)
{
   uint8_t offset = 0;
   uint8_t data[14] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
   uint8_t pdin_data[32] = {0};
   uint8_t pdin_data_len = 0;
   uint8_t exp_newinput_ind_cnt = mock_iolink_al_newinput_inf_cnt;
   uint8_t exp_al_data_len = sizeof(data);

   DL_PDInputTransport_ind (port, data, exp_al_data_len);
   exp_newinput_ind_cnt++;
   EXPECT_EQ (exp_newinput_ind_cnt, mock_iolink_al_newinput_inf_cnt);
   AL_GetInput_req (port, &pdin_data_len, pdin_data);
   EXPECT_EQ (exp_al_data_len, pdin_data_len);
   EXPECT_TRUE (ArraysMatchN (data, pdin_data, exp_al_data_len));

   offset = 4;
   exp_al_data_len -= offset;
   DL_PDInputTransport_ind (port, &data[offset], exp_al_data_len);
   exp_newinput_ind_cnt++;
   EXPECT_EQ (exp_newinput_ind_cnt, mock_iolink_al_newinput_inf_cnt);
   AL_GetInput_req (port, &pdin_data_len, pdin_data);
   EXPECT_EQ (exp_al_data_len, pdin_data_len);
   EXPECT_TRUE (ArraysMatchN (&data[offset], pdin_data, exp_al_data_len));
}
