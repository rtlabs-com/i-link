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
#include "iolink_ode.h"
#include "test_util.h"

// Test fixture

class ODETest : public TestBase
{
protected:
   // Override default setup
   virtual void SetUp() {
      TestBase::SetUp(); // Re-use default setup
   };
};

static inline iolink_ode_state_t ode_get_state (iolink_port_t * port)
{
   iolink_ode_port_t * ode = iolink_get_ode_ctx (port);

   return ode->state;
}

static inline arg_block_od_t * ode_create_ArgBlock_od (uint8_t data_len)
{
   arg_block_od_t * arg_block_od = NULL;

   if (data_len < 1)
   {
      return arg_block_od;
   }

   arg_block_od =
           (arg_block_od_t *) calloc (1, sizeof(arg_block_od_t) + data_len);
   EXPECT_TRUE (arg_block_od);

   arg_block_od->arg_block_id =  IOLINK_ARG_BLOCK_ID_VOID_BLOCK;

   return arg_block_od;
}

static inline void ode_verify_smi_err (iolink_arg_block_id_t exp_smi_ref_arg_id,
                                       iolink_arg_block_id_t exp_smi_exp_arg_id,
                                       iolink_smi_errortypes_t exp_errortype)
{
   EXPECT_EQ (exp_smi_ref_arg_id, mock_iolink_smi_ref_arg_block_id);

   iolink_arg_block_id_t arg_block_id =
						mock_iolink_smi_arg_block->void_block.arg_block_id;
   EXPECT_EQ (IOLINK_ARG_BLOCK_ID_JOB_ERROR, arg_block_id);

   arg_block_joberror_t * job_error =
                           (arg_block_joberror_t*)mock_iolink_smi_arg_block;
   iolink_smi_errortypes_t error = job_error->error;
   iolink_arg_block_id_t exp_arg_block_id = job_error->exp_arg_block_id;

   EXPECT_EQ (exp_errortype, error);
   EXPECT_EQ (exp_smi_exp_arg_id, exp_arg_block_id);
}

static inline void ode_verify_smi_read (
                                       iolink_arg_block_id_t exp_smi_ref_arg_id,
                                       iolink_arg_block_id_t exp_smi_arg_id,
                                       uint8_t exp_len, const uint8_t * data)
{
   EXPECT_EQ (exp_smi_ref_arg_id, mock_iolink_smi_ref_arg_block_id);
   iolink_arg_block_id_t arg_block_id =
						mock_iolink_smi_arg_block->void_block.arg_block_id;
   EXPECT_EQ (exp_smi_arg_id, arg_block_id);
   EXPECT_EQ (arg_block_id, IOLINK_ARG_BLOCK_ID_OD_RD);

   if (mock_iolink_smi_arg_block->void_block.arg_block_id ==
                                                IOLINK_ARG_BLOCK_ID_OD_RD)
   {
      arg_block_od_t * arg_block_od =
                           (arg_block_od_t *)mock_iolink_smi_arg_block;

      EXPECT_EQ (exp_len, mock_iolink_smi_arg_block_len);
      EXPECT_TRUE (ArraysMatchN (data, arg_block_od->data,
                                 exp_len - sizeof(arg_block_od_t)));
   }
   else
   {
      CC_ASSERT (0);
   }
}

static inline void ode_verify_smi_write (
                                       iolink_arg_block_id_t exp_smi_ref_arg_id,
                                       iolink_arg_block_id_t exp_smi_arg_id,
                                       uint8_t exp_len)
{
   EXPECT_EQ (exp_smi_ref_arg_id, mock_iolink_smi_ref_arg_block_id);
   iolink_arg_block_id_t arg_block_id =
						mock_iolink_smi_arg_block->void_block.arg_block_id;
   EXPECT_EQ (exp_smi_arg_id, arg_block_id);
   EXPECT_EQ (arg_block_id, IOLINK_ARG_BLOCK_ID_VOID_BLOCK);
   EXPECT_EQ (exp_len, mock_iolink_smi_arg_block_len);
}

static inline void ode_start (iolink_port_t * port)
{
   /* Start ODE */
   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));
   OD_Start (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODactive, ode_get_state (port));
}

static inline void ode_deviceread (iolink_port_t * port,
                                   uint16_t index, uint8_t subindex,
                                   uint8_t * data, uint8_t len)
{
   arg_block_od_t * arg_block_od = NULL;
   arg_block_od = ode_create_ArgBlock_od (len);
   ASSERT_TRUE (arg_block_od);

   arg_block_od->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_RD;
   arg_block_od->index = index;
   arg_block_od->subindex = subindex;

   memset (arg_block_od->data, 0, len);

   ode_start (port);

   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceRead_req (iolink_get_portnumber (port),
                                  IOLINK_ARG_BLOCK_ID_OD_RD,
                                  sizeof(arg_block_od_t) + len,
                                  (arg_block_t *)arg_block_od));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODblocked, ode_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   mock_iolink_al_read_cnf_cb (port, len, data, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);

   ode_verify_smi_read (IOLINK_ARG_BLOCK_ID_OD_RD, IOLINK_ARG_BLOCK_ID_OD_RD,
                        sizeof(arg_block_od_t) + len, data);

   free (arg_block_od);
}

static inline void ode_devicewrite (iolink_port_t * port,
                                    uint16_t index, uint8_t subindex,
                                    uint8_t * data, uint8_t len)
{
   arg_block_od_t * arg_block_od = NULL;

   arg_block_od = ode_create_ArgBlock_od (len);
   ASSERT_TRUE (arg_block_od);

   arg_block_od->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_WR;
   arg_block_od->index = index;
   arg_block_od->subindex = subindex;

   memcpy (arg_block_od->data, data, len);

   ode_start (port);

   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceWrite_req (iolink_get_portnumber (port),
                                   IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                   sizeof(arg_block_od_t) + len,
                                   (arg_block_t *)arg_block_od));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODblocked, ode_get_state (port));
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 1);
   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);

   ode_verify_smi_write (IOLINK_ARG_BLOCK_ID_OD_WR,
                        IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                        sizeof (arg_block_void_t));
   /* Verify AL_Write_req data */
   EXPECT_TRUE (ArraysMatchN (data, mock_iolink_al_data, len));
   EXPECT_EQ (mock_iolink_al_data_index, index);
   EXPECT_EQ (mock_iolink_al_data_subindex, subindex);

   free (arg_block_od);
}

/* TODO test:
 * DeviceRead_req()
 *    IOLINK_SMI_ERRORTYPE_PORT_NUM_INVALID
 *    IOLINK_SMI_ERRORTYPE_ARGBLOCK_INCONSISTENT
 * DeviceWrite_req()
 *    IOLINK_SMI_ERRORTYPE_PORT_NUM_INVALID
 *    IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID
 *    IOLINK_SMI_ERRORTYPE_ARGBLOCK_INCONSISTENT
 */

// Tests
TEST_F (ODETest, Ode_start_stop)
{
   ode_start (port);

   OD_Stop (port);
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
}

TEST_F (ODETest, Ode_DeviceRead_1)
{
   uint8_t data[1] = {0x65};
   uint16_t index = 8;
   uint8_t subindex = 0;

   ode_deviceread (port, index, subindex, data, sizeof(data));
}

TEST_F (ODETest, Ode_DeviceRead_2)
{
   uint8_t data[2] = {0x68, 0x91};
   uint16_t index = 2;
   uint8_t subindex = 1;

   ode_deviceread (port, index, subindex, data, sizeof(data));
}

TEST_F (ODETest, Ode_DeviceRead_3)
{
   uint8_t data[3] = {0x67, 0x37, 0x75};
   uint16_t index = 3;
   uint8_t subindex = 0;

   ode_deviceread (port, index, subindex, data, sizeof(data));
}

TEST_F (ODETest, Ode_DeviceRead_bad_argblock)
{
   iolink_arg_block_id_t bad_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   arg_block_od_t * arg_block_od = NULL;

   uint8_t data[1] = {0x65};
   uint16_t index = 7;
   uint8_t subindex = 0;

   arg_block_od = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od);

   /* Bad argblock type */
   arg_block_od->arg_block_id = bad_arg_block_id;
   arg_block_od->index = index;
   arg_block_od->subindex = subindex;
   arg_block_od->data[0] = 2;

   ode_start (port);

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceRead_req (portnumber, IOLINK_ARG_BLOCK_ID_OD_RD,
                                  sizeof(arg_block_od_t) + sizeof(data),
                                  (arg_block_t *)arg_block_od));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODactive, ode_get_state (port));

   ode_verify_smi_err (bad_arg_block_id, IOLINK_ARG_BLOCK_ID_OD_RD,
                       IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);

   free (arg_block_od);
}

TEST_F (ODETest, Ode_DeviceRead_bad_argblock_inactive)
{
   iolink_arg_block_id_t bad_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   arg_block_od_t * arg_block_od = NULL;

   uint8_t data[1] = {0x62};
   uint16_t index = 12;
   uint8_t subindex = 0;

   arg_block_od = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od);

   /* Bad argblock type */
   arg_block_od->arg_block_id = bad_arg_block_id;
   arg_block_od->index = index;
   arg_block_od->subindex = subindex;
   arg_block_od->data[0] = data[0];

   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceRead_req (portnumber, IOLINK_ARG_BLOCK_ID_OD_RD,
                                  sizeof(arg_block_od_t) + sizeof(data),
                                  (arg_block_t *)arg_block_od));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   ode_verify_smi_err (bad_arg_block_id, IOLINK_ARG_BLOCK_ID_OD_RD,
                       IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);

   free (arg_block_od);
}

TEST_F (ODETest, Ode_DeviceRead_bad_argblock_busy)
{
   iolink_arg_block_id_t bad_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   int i;

   arg_block_od_t * arg_block_od_first = NULL;
   arg_block_od_t * arg_block_od_busy = NULL;

   uint8_t data[1] = {0x65};
   uint16_t index = 8;
   uint8_t subindex = 0;

   arg_block_od_first = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od_first);
   arg_block_od_busy = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od_busy);

   arg_block_od_first->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_RD;
   arg_block_od_first->index = index;
   arg_block_od_first->subindex = subindex;
   arg_block_od_first->data[0] = 2;

   /* Bad argblock type */
   arg_block_od_busy->arg_block_id = bad_arg_block_id;
   arg_block_od_busy->index = index;
   arg_block_od_busy->subindex = subindex;
   arg_block_od_busy->data[0] = 3;

   ode_start (port);

   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceRead_req (portnumber, IOLINK_ARG_BLOCK_ID_OD_RD,
                                  sizeof(arg_block_od_t) + sizeof(data),
                                  (arg_block_t *)arg_block_od_first));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODblocked, ode_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);

   for (i = 0; i < 5; i++)
   {
      /* ODE is now busy waiting for AL_Read_cnf() */
      EXPECT_EQ (IOLINK_ERROR_NONE,
                 SMI_DeviceRead_req (portnumber, IOLINK_ARG_BLOCK_ID_OD_RD,
                                     sizeof(arg_block_od_t) + sizeof(data),
                                     (arg_block_t *)arg_block_od_busy));
      mock_iolink_job.callback (&mock_iolink_job);
      /* Verify JOB_ERROR */
      EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
      EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1 + i);
      ode_verify_smi_err (bad_arg_block_id, IOLINK_ARG_BLOCK_ID_OD_RD,
                          IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED);
   }

   mock_iolink_al_read_cnf_cb (port, sizeof(data), data,
                               IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 5);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_data_index, index);
   EXPECT_EQ (mock_iolink_al_data_subindex, subindex);

   ode_verify_smi_read (IOLINK_ARG_BLOCK_ID_OD_RD, IOLINK_ARG_BLOCK_ID_OD_RD,
                        sizeof(arg_block_od_t) + sizeof(data), data);

   free (arg_block_od_first);
   free (arg_block_od_busy);
}

TEST_F (ODETest, Ode_DeviceRead_too_small)
{
   arg_block_od_t * arg_block_od = NULL;

   uint8_t data[6] = {0x67, 0x37, 0x75, 0x22, 0x31, 0x99};
   uint16_t index = 9;
   uint8_t subindex = 4;

   uint8_t arg_block_len = 2;
   /* Buffer of size 2 is too samll for data[6] */
   arg_block_od = ode_create_ArgBlock_od (arg_block_len);
   ASSERT_TRUE (arg_block_od);

   arg_block_od->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_RD;
   arg_block_od->index = index;
   arg_block_od->subindex = subindex;
   arg_block_od->data[0] = 3;
   arg_block_od->data[1] = 4;

   ode_start (port);

   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceRead_req (portnumber, IOLINK_ARG_BLOCK_ID_OD_RD,
                                  sizeof(arg_block_od_t) + arg_block_len,
                                  (arg_block_t *)arg_block_od));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODblocked, ode_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   mock_iolink_al_read_cnf_cb (port, sizeof(data), data,
                               IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);

   ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_OD_RD, IOLINK_ARG_BLOCK_ID_OD_RD,
                       IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID);

   free (arg_block_od);
}

TEST_F (ODETest, Ode_DeviceRead_busy)
{
   int i;

   arg_block_od_t * arg_block_od_first = NULL;
   arg_block_od_t * arg_block_od_busy = NULL;

   uint8_t data[1] = {0x65};
   uint16_t index = 8;
   uint8_t subindex = 0;

   arg_block_od_first = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od_first);
   arg_block_od_busy = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od_busy);

   arg_block_od_first->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_RD;
   arg_block_od_first->index = index;
   arg_block_od_first->subindex = subindex;
   arg_block_od_first->data[0] = 2;

   arg_block_od_busy->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_RD;
   arg_block_od_busy->index = index;
   arg_block_od_busy->subindex = subindex;
   arg_block_od_busy->data[0] = 3;

   ode_start (port);

   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceRead_req (portnumber, IOLINK_ARG_BLOCK_ID_OD_RD,
                                  sizeof(arg_block_od_t) + sizeof(data),
                                  (arg_block_t *)arg_block_od_first));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODblocked, ode_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);

   for (i = 0; i < 5; i++)
   {
      /* ODE is now busy, waiting for AL_Read_cnf() */
      EXPECT_EQ (IOLINK_ERROR_NONE,
                 SMI_DeviceRead_req (portnumber, IOLINK_ARG_BLOCK_ID_OD_RD,
                                     sizeof(arg_block_od_t) + sizeof(data),
                                     (arg_block_t *)arg_block_od_busy));
      mock_iolink_job.callback (&mock_iolink_job);
      /* Verify JOB_ERROR */
      EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
      EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1 + i);
      ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_OD_RD, IOLINK_ARG_BLOCK_ID_OD_RD,
                          IOLINK_SMI_ERRORTYPE_SERVICE_TEMP_UNAVAILABLE);
   }

   mock_iolink_al_read_cnf_cb (port, sizeof(data), data,
                               IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   ode_verify_smi_read (IOLINK_ARG_BLOCK_ID_OD_RD, IOLINK_ARG_BLOCK_ID_OD_RD,
                        sizeof(arg_block_od_t) + sizeof(data), data);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 5);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_data_index, index);
   EXPECT_EQ (mock_iolink_al_data_subindex, subindex);

   free (arg_block_od_first);
   free (arg_block_od_busy);
}

TEST_F (ODETest, Ode_DeviceRead_inactive)
{
   arg_block_od_t * arg_block_od = NULL;

   uint8_t data[1] = {0x33};
   uint16_t index = 4;
   uint8_t subindex = 3;

   arg_block_od = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od);

   arg_block_od->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_RD;
   arg_block_od->index = index;
   arg_block_od->subindex = subindex;
   arg_block_od->data[0] = data[0];

   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceRead_req (portnumber, IOLINK_ARG_BLOCK_ID_OD_RD,
                                  sizeof(arg_block_od_t) + sizeof(data),
                                  (arg_block_t *)arg_block_od));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);

   ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_OD_RD, IOLINK_ARG_BLOCK_ID_OD_RD,
                       IOLINK_SMI_ERRORTYPE_IDX_NOTAVAIL);

   free (arg_block_od);
}

TEST_F (ODETest, Ode_DeviceWrite_1)
{
   uint8_t data[1] = {0x65};
   uint16_t index = 4;
   uint8_t subindex = 0;

   ode_devicewrite (port, index, subindex, data, sizeof(data));
}

TEST_F (ODETest, Ode_DeviceWrite_2)
{
   uint8_t data[2] = {0x99, 0x12};
   uint16_t index = 7;
   uint8_t subindex = 1;

   ode_devicewrite (port, index, subindex, data, sizeof(data));
}

TEST_F (ODETest, Ode_DeviceWrite_3)
{
   uint8_t data[3] = {0x9, 0x2, 0x30};
   uint16_t index = 2;
   uint8_t subindex = 2;

   ode_devicewrite (port, index, subindex, data, sizeof(data));
}

TEST_F (ODETest, Ode_DeviceWrite_bad_argblock)
{
   iolink_arg_block_id_t bad_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   arg_block_od_t * arg_block_od = NULL;

   uint8_t data[1] = {0x69};
   uint16_t index = 8;
   uint8_t subindex = 0;

   arg_block_od = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od);

   /* Bad argblock type */
   arg_block_od->arg_block_id = bad_arg_block_id;
   arg_block_od->index = index;
   arg_block_od->subindex = subindex;
   arg_block_od->data[0] = data[0];

   ode_start (port);

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceWrite_req (portnumber, IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                   sizeof(arg_block_od_t) + sizeof(data),
                                   (arg_block_t *)arg_block_od));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODactive, ode_get_state (port));

   ode_verify_smi_err (bad_arg_block_id, IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                       IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);

   free (arg_block_od);
}

TEST_F (ODETest, Ode_DeviceWrite_bad_argblock_inactive)
{
   iolink_arg_block_id_t bad_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   arg_block_od_t * arg_block_od = NULL;

   uint8_t data[1] = {0x69};
   uint16_t index = 8;
   uint8_t subindex = 0;

   arg_block_od = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od);

   /* Bad argblock type */
   arg_block_od->arg_block_id = bad_arg_block_id;
   arg_block_od->index = index;
   arg_block_od->subindex = subindex;
   arg_block_od->data[0] = data[0];

   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceWrite_req (portnumber, IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                   sizeof(arg_block_od_t) + sizeof(data),
                                   (arg_block_t *)arg_block_od));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   ode_verify_smi_err (bad_arg_block_id, IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                       IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);

   free (arg_block_od);
}

TEST_F (ODETest, Ode_DeviceWrite_bad_argblock_busy)
{
   iolink_arg_block_id_t bad_arg_block_id = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   int i;

   arg_block_od_t * arg_block_od_first = NULL;
   arg_block_od_t * arg_block_od_busy = NULL;

   uint8_t data[1] = {0x71};
   uint16_t index = 5;
   uint8_t subindex = 0;

   arg_block_od_first = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od_first);
   arg_block_od_busy = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od_busy);

   arg_block_od_first->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_WR;
   arg_block_od_first->index = index;
   arg_block_od_first->subindex = subindex;
   arg_block_od_first->data[0] = data[0];

   /* Bad argblock type */
   arg_block_od_busy->arg_block_id = bad_arg_block_id;
   arg_block_od_busy->index = index;
   arg_block_od_busy->subindex = subindex;
   arg_block_od_busy->data[0] = data[0];

   ode_start (port);

   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceWrite_req (portnumber, IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                   sizeof(arg_block_od_t) + sizeof(data),
                                   (arg_block_t *)arg_block_od_first));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODblocked, ode_get_state (port));
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);

   for (i = 0; i < 5; i++)
   {
      /* ODE is now busy waiting for AL_Write_cnf() */
      EXPECT_EQ (IOLINK_ERROR_NONE,
                 SMI_DeviceWrite_req (portnumber,
                                      IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                      sizeof(arg_block_od_t) + sizeof(data),
                                      (arg_block_t *)arg_block_od_busy));
      mock_iolink_job.callback (&mock_iolink_job);
      /* Verify JOB_ERROR */
      EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
      EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1 + i);
      ode_verify_smi_err (bad_arg_block_id, IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                          IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED);
   }

   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 5);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_data_index, index);
   EXPECT_EQ (mock_iolink_al_data_subindex, subindex);

   ode_verify_smi_write (IOLINK_ARG_BLOCK_ID_OD_WR,
                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                         sizeof (arg_block_void_t));
   /* Verify AL_Write_req data */
   EXPECT_TRUE (ArraysMatchN (data, mock_iolink_al_data, sizeof(data)));

   free (arg_block_od_first);
   free (arg_block_od_busy);
}

TEST_F (ODETest, Ode_DeviceWrite_busy)
{
   int i;

   arg_block_od_t * arg_block_od_first = NULL;
   arg_block_od_t * arg_block_od_busy = NULL;

   uint8_t data[1] = {0x69};
   uint16_t index = 9;
   uint8_t subindex = 0;

   arg_block_od_first = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od_first);
   arg_block_od_busy = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od_busy);

   arg_block_od_first->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_WR;
   arg_block_od_first->index = index;
   arg_block_od_first->subindex = subindex;
   arg_block_od_first->data[0] = data[0];

   arg_block_od_busy->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_WR;
   arg_block_od_busy->index = index;
   arg_block_od_busy->subindex = subindex;
   arg_block_od_busy->data[0] = data[0];

   ode_start (port);

   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceWrite_req (portnumber, IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                   sizeof(arg_block_od_t) + sizeof(data),
                                   (arg_block_t *)arg_block_od_first));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODblocked, ode_get_state (port));
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);

   for (i = 0; i < 5; i++)
   {
      /* ODE is now busy, waiting for AL_Write_cnf() */
      EXPECT_EQ (IOLINK_ERROR_NONE,
                 SMI_DeviceWrite_req (portnumber,
                                      IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                      sizeof(arg_block_od_t) + sizeof(data),
                                      (arg_block_t *)arg_block_od_busy));
      mock_iolink_job.callback (&mock_iolink_job);
      /* Verify JOB_ERROR */
      EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
      EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1 + i);
      ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_OD_WR,
                          IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                          IOLINK_SMI_ERRORTYPE_SERVICE_TEMP_UNAVAILABLE);
   }

   mock_iolink_al_write_cnf_cb (port, IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   ode_verify_smi_write (IOLINK_ARG_BLOCK_ID_OD_WR,
                         IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                         sizeof (arg_block_void_t));
   /* Verify AL_Write_req data */
   EXPECT_TRUE (ArraysMatchN (data, mock_iolink_al_data, sizeof(data)));
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 5);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_data_index, index);
   EXPECT_EQ (mock_iolink_al_data_subindex, subindex);

   free (arg_block_od_first);
   free (arg_block_od_busy);
}

TEST_F (ODETest, Ode_DeviceWrite_inactive)
{
   arg_block_od_t * arg_block_od = NULL;

   uint8_t data[1] = {0x64};
   uint16_t index = 5;
   uint8_t subindex = 2;

   arg_block_od = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od);

   arg_block_od->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_WR;
   arg_block_od->index = index;
   arg_block_od->subindex = subindex;
   arg_block_od->data[0] = data[0];

   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceWrite_req (portnumber, IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                   sizeof(arg_block_od_t) + sizeof(data),
                                   (arg_block_t *)arg_block_od));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);

   ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_OD_WR,
                       IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                       IOLINK_SMI_ERRORTYPE_IDX_NOTAVAIL);

   free (arg_block_od);
}

TEST_F (ODETest, Ode_ParamReadBatch_inactive)
{
   arg_block_t arg_block;

   arg_block.void_block.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_INXDEX_LIST;

   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_ParamReadBatch_req (portnumber,
                                      IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT,
                                      sizeof(arg_block_t), &arg_block));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);

   ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_PORT_INXDEX_LIST,
                       IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT,
                       IOLINK_SMI_ERRORTYPE_SERVICE_NOT_SUPPORTED);
}

TEST_F (ODETest, Ode_ParamReadBatch_not_supported)
{
   arg_block_t arg_block;

   arg_block.void_block.arg_block_id = IOLINK_ARG_BLOCK_ID_PORT_INXDEX_LIST;

   ode_start (port);

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_ParamReadBatch_req (portnumber,
                                      IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT,
                                      sizeof(arg_block_t), &arg_block));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODactive, ode_get_state (port));

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);

   ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_PORT_INXDEX_LIST,
                       IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT,
                       IOLINK_SMI_ERRORTYPE_SERVICE_NOT_SUPPORTED);
}

TEST_F (ODETest, Ode_ParamReadBatch_busy)
{
   int i;

   arg_block_od_t * arg_block_od_first = NULL;
   arg_block_t arg_block_readbatch;

   uint8_t data[1] = {0x65};
   uint16_t index = 8;
   uint8_t subindex = 0;

   arg_block_od_first = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od_first);

   arg_block_od_first->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_RD;
   arg_block_od_first->index = index;
   arg_block_od_first->subindex = subindex;
   arg_block_od_first->data[0] = 2;

   arg_block_readbatch.void_block.arg_block_id =
                                          IOLINK_ARG_BLOCK_ID_PORT_INXDEX_LIST;

   ode_start (port);

   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceRead_req (portnumber, IOLINK_ARG_BLOCK_ID_OD_RD,
                                  sizeof(arg_block_od_t) + sizeof(data),
                                  (arg_block_t *)arg_block_od_first));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODblocked, ode_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);

   for (i = 0; i < 5; i++)
   {
      /* ODE is now busy, waiting for AL_Read_cnf() */
      EXPECT_EQ (IOLINK_ERROR_NONE,
                 SMI_ParamReadBatch_req (portnumber,
                                         IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT,
                                         sizeof(arg_block_t),
                                         &arg_block_readbatch));
      mock_iolink_job.callback (&mock_iolink_job);
      /* Verify JOB_ERROR */
      EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
      EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1 + i);
      ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_PORT_INXDEX_LIST,
                          IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT,
                          IOLINK_SMI_ERRORTYPE_SERVICE_NOT_SUPPORTED);
   }

   mock_iolink_al_read_cnf_cb (port, sizeof(data), data,
                               IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   ode_verify_smi_read (IOLINK_ARG_BLOCK_ID_OD_RD, IOLINK_ARG_BLOCK_ID_OD_RD,
                        sizeof(arg_block_od_t) + sizeof(data), data);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 5);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_data_index, index);
   EXPECT_EQ (mock_iolink_al_data_subindex, subindex);

   free (arg_block_od_first);
}

TEST_F (ODETest, Ode_ParamWriteBatch_inactive)
{
   arg_block_t arg_block;

   arg_block.void_block.arg_block_id = IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT;

   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_ParamWriteBatch_req (portnumber,
                                       IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                       sizeof(arg_block_t), &arg_block));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_Inactive, ode_get_state (port));

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);

   ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT,
                       IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                       IOLINK_SMI_ERRORTYPE_SERVICE_NOT_SUPPORTED);
}

TEST_F (ODETest, Ode_ParamWriteBatch_not_supported)
{
   arg_block_t arg_block;

   arg_block.void_block.arg_block_id = IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT;

   ode_start (port);

   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_ParamWriteBatch_req (portnumber,
                                       IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                       sizeof(arg_block_t), &arg_block));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODactive, ode_get_state (port));

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);

   ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT,
                       IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                       IOLINK_SMI_ERRORTYPE_SERVICE_NOT_SUPPORTED);
}

TEST_F (ODETest, Ode_ParamWriteBatch_busy)
{
   int i;

   arg_block_od_t * arg_block_od_first = NULL;
   arg_block_t arg_block_readbatch;

   uint8_t data[1] = {0x65};
   uint16_t index = 8;
   uint8_t subindex = 0;

   arg_block_od_first = ode_create_ArgBlock_od (sizeof(data));
   ASSERT_TRUE (arg_block_od_first);

   arg_block_od_first->arg_block_id = IOLINK_ARG_BLOCK_ID_OD_RD;
   arg_block_od_first->index = index;
   arg_block_od_first->subindex = subindex;
   arg_block_od_first->data[0] = 2;

   arg_block_readbatch.void_block.arg_block_id =
                                                IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT;

   ode_start (port);

   EXPECT_EQ (mock_iolink_al_read_req_cnt, 0);
   EXPECT_EQ (IOLINK_ERROR_NONE,
              SMI_DeviceRead_req (portnumber, IOLINK_ARG_BLOCK_ID_OD_RD,
                                  sizeof(arg_block_od_t) + sizeof(data),
                                  (arg_block_t *)arg_block_od_first));
   mock_iolink_job.callback (&mock_iolink_job);
   EXPECT_EQ (ODE_STATE_ODblocked, ode_get_state (port));
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);

   for (i = 0; i < 5; i++)
   {
      /* ODE is now busy, waiting for AL_Read_cnf() */
      EXPECT_EQ (IOLINK_ERROR_NONE,
                 SMI_ParamWriteBatch_req (portnumber,
                                          IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                                          sizeof(arg_block_t),
                                          &arg_block_readbatch));
      mock_iolink_job.callback (&mock_iolink_job);
      /* Verify JOB_ERROR */
      EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
      EXPECT_EQ (mock_iolink_smi_joberror_cnt, 1 + i);
      ode_verify_smi_err (IOLINK_ARG_BLOCK_ID_DEV_PAR_BAT,
                          IOLINK_ARG_BLOCK_ID_VOID_BLOCK,
                          IOLINK_SMI_ERRORTYPE_SERVICE_NOT_SUPPORTED);
   }

   mock_iolink_al_read_cnf_cb (port, sizeof(data), data,
                               IOLINK_SMI_ERRORTYPE_NONE);
   mock_iolink_job.callback (&mock_iolink_job);

   ode_verify_smi_read (IOLINK_ARG_BLOCK_ID_OD_RD, IOLINK_ARG_BLOCK_ID_OD_RD,
                        sizeof(arg_block_od_t) + sizeof(data), data);
   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 1);
   EXPECT_EQ (mock_iolink_smi_joberror_cnt, 5);
   EXPECT_EQ (mock_iolink_al_write_req_cnt, 0);
   EXPECT_EQ (mock_iolink_al_read_req_cnt, 1);
   EXPECT_EQ (mock_iolink_al_data_index, index);
   EXPECT_EQ (mock_iolink_al_data_subindex, subindex);

   free (arg_block_od_first);
}
