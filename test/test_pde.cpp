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
#include "iolink_pde.h"
#include "test_util.h"

// Test fixture

class PDETest : public TestBase
{
 protected:
   // Override default setup
   virtual void SetUp()
   {
      TestBase::SetUp(); // Re-use default setup
   };
};

static inline iolink_pde_state_t pde_get_state (iolink_port_t * port)
{
   iolink_pde_port_t * pde = iolink_get_pde_ctx (port);

   return pde->state;
}

static inline void pde_verify_smi_err (
   iolink_arg_block_id_t exp_smi_ref_arg_id,
   iolink_arg_block_id_t exp_smi_exp_arg_id,
   iolink_smi_errortypes_t exp_errortype,
   uint8_t exp_smi_cnf_cnt,
   uint8_t exp_smi_joberror_cnt)
{
   EXPECT_EQ (exp_smi_cnf_cnt, mock_iolink_smi_cnf_cnt);
   EXPECT_EQ (exp_smi_joberror_cnt, mock_iolink_smi_joberror_cnt);
   EXPECT_EQ (exp_smi_ref_arg_id, mock_iolink_smi_ref_arg_block_id);
   iolink_arg_block_id_t arg_block_id =
      mock_iolink_smi_arg_block->void_block.arg_block_id;
   EXPECT_EQ (IOLINK_ARG_BLOCK_ID_JOB_ERROR, arg_block_id);

   arg_block_joberror_t * job_error =
      (arg_block_joberror_t *)mock_iolink_smi_arg_block;

   iolink_smi_errortypes_t error          = job_error->error;
   iolink_arg_block_id_t exp_arg_block_id = job_error->exp_arg_block_id;

   EXPECT_EQ (exp_errortype, error);
   EXPECT_EQ (exp_smi_exp_arg_id, exp_arg_block_id);
}

static inline void pd_start (iolink_port_t * port)
{
   /* Start PDE */
   EXPECT_EQ (PD_STATE_Inactive, pde_get_state (port));
   PD_Start (port);
   EXPECT_EQ (PD_STATE_PDactive, pde_get_state (port));
}

/* TODO test:
 * PDIn_req()
 *    IOLINK_SMI_ERRORTYPE_PORT_NUM_INVALID
 * PDOut_req()
 *    IOLINK_SMI_ERRORTYPE_PORT_NUM_INVALID
 *    IOLINK_SMI_ERRORTYPE_ARGBLOCK_INCONSISTENT
 * PDInOut_req()
 *    IOLINK_SMI_ERRORTYPE_PORT_NUM_INVALID
 */

// Tests
TEST_F (PDETest, pde_start_stop)
{
   pd_start (port);

   PD_Stop (port);
   EXPECT_EQ (PD_STATE_Inactive, pde_get_state (port));

   EXPECT_EQ (mock_iolink_smi_cnf_cnt, 0);
}

TEST_F (PDETest, pde_PDIn_inactive)
{
   iolink_arg_block_id_t ref_arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_exp_arg_block_id =
      IOLINK_ARG_BLOCK_ID_FS_MASTER_ACCESS;
   arg_block_void_t arg_block_void;

   uint8_t exp_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   memset (&arg_block_void, 0, sizeof (arg_block_void_t));
   arg_block_void.arg_block_id = ref_arg_block_id;

   EXPECT_EQ (
      IOLINK_ERROR_PARAMETER_CONFLICT,
      SMI_PDIn_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_void_t),
         (arg_block_t *)&arg_block_void));
   EXPECT_EQ (PD_STATE_Inactive, pde_get_state (port));

   pde_verify_smi_err (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_ARGBLOCK_INCONSISTENT,
      0,
      exp_joberror_cnt);
}

TEST_F (PDETest, pde_PDOut_inactive)
{
   iolink_arg_block_id_t ref_arg_block_id     = IOLINK_ARG_BLOCK_ID_PD_OUT;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   arg_block_pdout_t arg_block_pdout;

   uint8_t exp_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   memset (&arg_block_pdout, 0, sizeof (arg_block_pdout_head_t) + 1);
   arg_block_pdout.h.arg_block_id = ref_arg_block_id;

   EXPECT_EQ (
      IOLINK_ERROR_STATE_INVALID,
      SMI_PDOut_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_pdout_head_t) + 1,
         (arg_block_t *)&arg_block_pdout));
   EXPECT_EQ (PD_STATE_Inactive, pde_get_state (port));

   pde_verify_smi_err (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_DEV_NOT_IN_OPERATE,
      0,
      exp_joberror_cnt);
}

static inline void pde_verify_smi_in (
   iolink_arg_block_id_t exp_smi_ref_arg_id,
   iolink_arg_block_id_t exp_smi_arg_id,
   iolink_port_qualifier_info_t exp_port_qualifier_info,
   uint8_t exp_len,
   uint8_t * exp_data,
   uint8_t exp_data_len)
{
   EXPECT_EQ (exp_smi_ref_arg_id, mock_iolink_smi_ref_arg_block_id);
   iolink_arg_block_id_t arg_block_id =
      mock_iolink_smi_arg_block->void_block.arg_block_id;
   EXPECT_EQ (exp_smi_arg_id, arg_block_id);
   EXPECT_EQ (exp_len, mock_iolink_smi_arg_block_len);

   if (exp_smi_arg_id == IOLINK_ARG_BLOCK_ID_PD_IN)
   {
      arg_block_pdin_t * arg_block_pdin =
         (arg_block_pdin_t *)mock_iolink_smi_arg_block;

      EXPECT_EQ (exp_port_qualifier_info, arg_block_pdin->h.port_qualifier_info);
      EXPECT_EQ (exp_data_len, arg_block_pdin->h.len);
      EXPECT_TRUE (ArraysMatchN (exp_data, arg_block_pdin->data, exp_data_len));
   }
   else
   {
      CC_ASSERT (0);
   }
}

static inline void pde_verify_smi_out (
   iolink_arg_block_id_t exp_smi_ref_arg_id,
   iolink_arg_block_id_t exp_smi_arg_id,
   uint8_t exp_len)
{
   EXPECT_EQ (exp_smi_ref_arg_id, mock_iolink_smi_ref_arg_block_id);
   iolink_arg_block_id_t arg_block_id =
      mock_iolink_smi_arg_block->void_block.arg_block_id;
   EXPECT_EQ (exp_smi_arg_id, arg_block_id);
   EXPECT_EQ (exp_len, mock_iolink_smi_arg_block_len);
}

static uint8_t iolink_arg_block_pdinout_pdin_data_len (
   const arg_block_pdinout_t * arg_block)
{
   return arg_block->data[0];
}

static uint8_t * iolink_arg_block_pdinout_pdin_data (arg_block_pdinout_t * arg_block)
{
   return &arg_block->data[1];
}

uint8_t * iolink_arg_block_pdinout_pdout_data (arg_block_pdinout_t * arg_block)
{
   return &arg_block->data[2 + iolink_arg_block_pdinout_pdin_data_len (arg_block)];
}

static inline void pde_verify_smi_in_out (
   iolink_arg_block_id_t exp_smi_ref_arg_id,
   iolink_arg_block_id_t exp_smi_arg_id,
   iolink_port_qualifier_info_t exp_port_qualifier_info,
   uint8_t exp_oe,
   uint8_t exp_arg_block_len,
   uint8_t * exp_pdin_data,
   uint8_t exp_pdin_data_len,
   uint8_t * exp_pdout_data,
   uint8_t exp_pdout_data_len)
{
   EXPECT_EQ (exp_smi_ref_arg_id, mock_iolink_smi_ref_arg_block_id);
   iolink_arg_block_id_t arg_block_id =
      mock_iolink_smi_arg_block->void_block.arg_block_id;
   EXPECT_EQ (exp_smi_arg_id, arg_block_id);

   if (exp_smi_arg_id == IOLINK_ARG_BLOCK_ID_PD_IN_OUT)
   {
      arg_block_pdinout_t * arg_block_pdinout =
         (arg_block_pdinout_t *)mock_iolink_smi_arg_block;
      uint8_t pdin_data_len =
         iolink_arg_block_pdinout_pdin_data_len (arg_block_pdinout);
      uint8_t * pdin_data =
         iolink_arg_block_pdinout_pdin_data (arg_block_pdinout);

      EXPECT_EQ (exp_port_qualifier_info, arg_block_pdinout->h.port_qualifier_info);
      EXPECT_EQ (exp_oe, arg_block_pdinout->h.oe);

      EXPECT_EQ (exp_pdin_data_len, pdin_data_len);
      EXPECT_TRUE (ArraysMatchN (exp_pdin_data, pdin_data, exp_pdin_data_len));
   }
   else
   {
      CC_ASSERT (0);
   }
}

static void pde_smi_pdout (
   iolink_port_t * port,
   uint8_t oe,
   uint8_t data_len,
   uint8_t * data)
{
   iolink_pde_port_t * pde = iolink_get_pde_ctx (port);

   iolink_arg_block_id_t ref_arg_block_id     = IOLINK_ARG_BLOCK_ID_PD_OUT;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   uint8_t exp_data_len                       = data_len;
   uint8_t arg_block_len   = sizeof (arg_block_pdout_head_t) + exp_data_len;
   uint8_t exp_smi_cnf_cnt = mock_iolink_smi_cnf_cnt + 1;
   uint8_t exp_al_control_req_cnt       = mock_iolink_al_control_req_cnt;
   iolink_controlcode_t exp_controlcode = mock_iolink_controlcode;

   arg_block_pdout_t arg_block_pdout;

   if ((pde->output_enable & 0x1) != (oe & 0x1))
   {
      exp_al_control_req_cnt++;
      if (oe & 0x1)
      {
         exp_controlcode = IOLINK_CONTROLCODE_PDOUTVALID;
      }
      else
      {
         exp_controlcode = IOLINK_CONTROLCODE_PDOUTINVALID;
      }
   }

   memset (&arg_block_pdout, 0, arg_block_len);
   arg_block_pdout.h.arg_block_id = ref_arg_block_id;
   memcpy (arg_block_pdout.data, data, exp_data_len);
   arg_block_pdout.h.len = exp_data_len;
   arg_block_pdout.h.oe  = oe;

   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      SMI_PDOut_req (
         iolink_get_portnumber (port),
         exp_exp_arg_block_id,
         arg_block_len,
         (arg_block_t *)&arg_block_pdout));

   EXPECT_EQ (exp_smi_cnf_cnt, mock_iolink_smi_cnf_cnt);
   pde_verify_smi_out (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      sizeof (arg_block_void_t));
   EXPECT_EQ (oe, pde->output_enable);
   EXPECT_EQ (exp_controlcode, mock_iolink_controlcode);
   EXPECT_EQ (exp_al_control_req_cnt, mock_iolink_al_control_req_cnt);
}

TEST_F (PDETest, pde_PDOut)
{
   uint8_t data[5] = {1, 3, 7, 2, 5};
   pd_start (port);
   uint8_t output_enable = 0;

   pde_smi_pdout (port, output_enable, sizeof (data), data);
   output_enable = 1;
   pde_smi_pdout (port, output_enable, sizeof (data), data);
   data[3] = 11;
   pde_smi_pdout (port, output_enable, sizeof (data), data);
   output_enable = 0;
   data[0]       = 1;
   pde_smi_pdout (port, output_enable, sizeof (data), data);
}

TEST_F (PDETest, pde_PDIn)
{
   iolink_arg_block_id_t ref_arg_block_id     = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_PD_IN;
   uint8_t data[16]     = {1, 3, 7, 2, 5, 4, 9, 6, 8, 11, 19, 17, 12, 7, 12, 2};
   uint8_t exp_data_len = sizeof (data);
   uint8_t exp_smi_cnf_cnt         = mock_iolink_smi_cnf_cnt;
   uint8_t exp_al_getinput_req_cnt = mock_iolink_al_getinput_req_cnt;
   arg_block_void_t arg_block_void;

   mock_iolink_dl_pdin_data_len = exp_data_len;
   memcpy (mock_iolink_dl_pdin_data, data, exp_data_len);

   pd_start (port);

   arg_block_void.arg_block_id = ref_arg_block_id;
   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      SMI_PDIn_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_void_t),
         (arg_block_t *)&arg_block_void));
   exp_al_getinput_req_cnt++;
   exp_smi_cnf_cnt++;
   EXPECT_EQ (exp_al_getinput_req_cnt, mock_iolink_al_getinput_req_cnt);
   EXPECT_EQ (exp_smi_cnf_cnt, mock_iolink_smi_cnf_cnt);
   pde_verify_smi_in (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_PORT_QUALIFIER_INFO_PQ_INVALID,
      sizeof (arg_block_pdin_head_t) + exp_data_len,
      data,
      exp_data_len);

   data[0] = 3;
   data[1] = 5;
   memcpy (mock_iolink_dl_pdin_data, data, exp_data_len);
   EXPECT_EQ (IOLINK_ERROR_NONE, AL_Control_ind (port, IOLINK_CONTROLCODE_VALID));

   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      SMI_PDIn_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_void_t),
         (arg_block_t *)&arg_block_void));
   exp_al_getinput_req_cnt++;
   exp_smi_cnf_cnt++;
   EXPECT_EQ (exp_al_getinput_req_cnt, mock_iolink_al_getinput_req_cnt);
   EXPECT_EQ (exp_smi_cnf_cnt, mock_iolink_smi_cnf_cnt);
   pde_verify_smi_in (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_PORT_QUALIFIER_INFO_PQ_VALID,
      sizeof (arg_block_pdin_head_t) + exp_data_len,
      data,
      exp_data_len);
}

TEST_F (PDETest, pde_PDInOut)
{
   iolink_arg_block_id_t ref_arg_block_id     = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_PD_IN_OUT;
   uint8_t exp_output_enable                  = 0;
   uint8_t pdout_data[3]                      = {66, 77, 88};
   uint8_t exp_pdout_data_len                 = sizeof (pdout_data);
   uint8_t pdin_data[16] = {1, 3, 7, 2, 5, 4, 9, 6, 8, 11, 19, 17, 12, 7, 12, 2};
   uint8_t exp_pdin_data_len             = sizeof (pdin_data);
   uint8_t exp_smi_cnf_cnt               = mock_iolink_smi_cnf_cnt;
   uint8_t exp_al_getinputoutput_req_cnt = mock_iolink_al_getinputoutput_req_cnt;
   arg_block_void_t arg_block_void;

   memcpy (mock_iolink_dl_pdin_data, pdin_data, exp_pdin_data_len);
   mock_iolink_dl_pdin_data_len = exp_pdin_data_len;

   pd_start (port);

   EXPECT_EQ (IOLINK_ERROR_NONE, AL_Control_ind (port, IOLINK_CONTROLCODE_VALID));

   arg_block_void.arg_block_id = ref_arg_block_id;
   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      SMI_PDInOut_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_void_t),
         (arg_block_t *)&arg_block_void));

   exp_al_getinputoutput_req_cnt++;
   exp_smi_cnf_cnt++;
   EXPECT_EQ (exp_al_getinputoutput_req_cnt, mock_iolink_al_getinputoutput_req_cnt);

   EXPECT_EQ (exp_smi_cnf_cnt, mock_iolink_smi_cnf_cnt);
   pde_verify_smi_in_out (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_PORT_QUALIFIER_INFO_PQ_VALID,
      exp_output_enable,
      sizeof (arg_block_pdinout_head_t) + 1 + exp_pdin_data_len + 1,
      pdin_data,
      exp_pdin_data_len,
      NULL,
      0);

   exp_output_enable = 1;
   pde_smi_pdout (port, exp_output_enable, exp_pdout_data_len, pdout_data);
   exp_smi_cnf_cnt++;

   pdin_data[2]  = 0;
   pdin_data[5]  = 42;
   pdin_data[15] = 111;
   memcpy (mock_iolink_dl_pdin_data, pdin_data, exp_pdin_data_len);
   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      SMI_PDInOut_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_void_t),
         (arg_block_t *)&arg_block_void));
   exp_al_getinputoutput_req_cnt++;
   exp_smi_cnf_cnt++;
   EXPECT_EQ (exp_al_getinputoutput_req_cnt, mock_iolink_al_getinputoutput_req_cnt);

   EXPECT_EQ (exp_smi_cnf_cnt, mock_iolink_smi_cnf_cnt);
   pde_verify_smi_in_out (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_PORT_QUALIFIER_INFO_PQ_VALID,
      exp_output_enable,
      sizeof (arg_block_pdinout_head_t) + 1 + exp_pdin_data_len + 1 +
         exp_pdout_data_len,
      pdin_data,
      exp_pdin_data_len,
      pdout_data,
      exp_pdout_data_len);

   exp_output_enable = 0;
   exp_pdout_data_len--;
   pdout_data[0] = 3;
   pdout_data[1] = 9;
   pde_smi_pdout (port, exp_output_enable, exp_pdout_data_len, pdout_data);
   exp_smi_cnf_cnt++;

   pdin_data[2]  = 99;
   pdin_data[5]  = 100;
   pdin_data[15] = 101;
   memcpy (mock_iolink_dl_pdin_data, pdin_data, exp_pdin_data_len);
   EXPECT_EQ (
      IOLINK_ERROR_NONE,
      SMI_PDInOut_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_void_t),
         (arg_block_t *)&arg_block_void));
   exp_al_getinputoutput_req_cnt++;
   exp_smi_cnf_cnt++;
   EXPECT_EQ (exp_al_getinputoutput_req_cnt, mock_iolink_al_getinputoutput_req_cnt);
   EXPECT_EQ (exp_smi_cnf_cnt, mock_iolink_smi_cnf_cnt);
   pde_verify_smi_in_out (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_PORT_QUALIFIER_INFO_PQ_VALID,
      exp_output_enable,
      sizeof (arg_block_pdinout_head_t) + 1 + exp_pdin_data_len + 1 +
         exp_pdout_data_len,
      pdin_data,
      exp_pdin_data_len,
      pdout_data,
      exp_pdout_data_len);
}

TEST_F (PDETest, pde_PDInOut_inactive)
{
   iolink_arg_block_id_t ref_arg_block_id     = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_PD_IN_OUT;
   arg_block_void_t arg_block_void;

   uint8_t exp_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   memset (&arg_block_void, 0, sizeof (arg_block_void_t));
   arg_block_void.arg_block_id = ref_arg_block_id;

   EXPECT_EQ (
      IOLINK_ERROR_STATE_INVALID,
      SMI_PDInOut_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_void_t),
         (arg_block_t *)&arg_block_void));
   EXPECT_EQ (PD_STATE_Inactive, pde_get_state (port));

   pde_verify_smi_err (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_DEV_NOT_IN_OPERATE,
      0,
      exp_joberror_cnt);
}

TEST_F (PDETest, pde_PDIn_bad_argblock)
{
   iolink_arg_block_id_t bad_arg_block_id     = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_PD_IN;
   arg_block_void_t arg_block_void;

   uint8_t exp_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   pd_start (port);

   memset (&arg_block_void, 0, sizeof (arg_block_void_t));
   /* Bad argblock type */
   arg_block_void.arg_block_id = bad_arg_block_id;

   EXPECT_EQ (
      IOLINK_ERROR_PARAMETER_CONFLICT,
      SMI_PDIn_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_void_t),
         (arg_block_t *)&arg_block_void));
   EXPECT_EQ (PD_STATE_PDactive, pde_get_state (port));

   pde_verify_smi_err (
      bad_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED,
      0,
      exp_joberror_cnt);
}

TEST_F (PDETest, pde_PDOut_bad_argblock)
{
   iolink_arg_block_id_t bad_arg_block_id     = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   arg_block_pdout_t arg_block_pdout;

   uint8_t exp_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   pd_start (port);

   memset (&arg_block_pdout, 0, sizeof (arg_block_pdout_head_t));
   /* Bad argblock type */
   arg_block_pdout.h.arg_block_id = bad_arg_block_id;

   EXPECT_EQ (
      IOLINK_ERROR_PARAMETER_CONFLICT,
      SMI_PDOut_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_pdout_head_t),
         (arg_block_t *)&arg_block_pdout));
   EXPECT_EQ (PD_STATE_PDactive, pde_get_state (port));

   pde_verify_smi_err (
      bad_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED,
      0,
      exp_joberror_cnt);
}

TEST_F (PDETest, pde_PDInOut_bad_argblock)
{
   iolink_arg_block_id_t bad_arg_block_id     = IOLINK_ARG_BLOCK_ID_MASTERIDENT;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_PD_IN_OUT;
   arg_block_void_t arg_block_void;

   uint8_t exp_joberror_cnt = mock_iolink_smi_joberror_cnt + 1;

   pd_start (port);

   memset (&arg_block_void, 0, sizeof (arg_block_void_t));
   /* Bad argblock type */
   arg_block_void.arg_block_id = bad_arg_block_id;

   EXPECT_EQ (
      IOLINK_ERROR_PARAMETER_CONFLICT,
      SMI_PDInOut_req (
         portnumber,
         exp_exp_arg_block_id,
         sizeof (arg_block_void_t),
         (arg_block_t *)&arg_block_void));
   EXPECT_EQ (PD_STATE_PDactive, pde_get_state (port));

   pde_verify_smi_err (
      bad_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_ARGBLOCK_NOT_SUPPORTED,
      0,
      exp_joberror_cnt);
}

TEST_F (PDETest, pde_PDIn_bad_argblock_len)
{
   iolink_arg_block_id_t ref_arg_block_id     = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_PD_IN;
   arg_block_void_t arg_block_void;

   uint8_t exp_joberror_cnt = mock_iolink_smi_joberror_cnt;

   pd_start (port);

   memset (&arg_block_void, 0, sizeof (arg_block_void_t));
   arg_block_void.arg_block_id = ref_arg_block_id;

   EXPECT_EQ (
      IOLINK_ERROR_PARAMETER_CONFLICT,
      SMI_PDIn_req (
         portnumber,
         exp_exp_arg_block_id,
         /* Bad ArgBlockLength */
         sizeof (arg_block_void_t) + 1,
         (arg_block_t *)&arg_block_void));
   EXPECT_EQ (PD_STATE_PDactive, pde_get_state (port));
   exp_joberror_cnt++;

   pde_verify_smi_err (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID,
      0,
      exp_joberror_cnt);

   EXPECT_EQ (
      IOLINK_ERROR_PARAMETER_CONFLICT,
      SMI_PDIn_req (
         portnumber,
         exp_exp_arg_block_id,
         /* Bad ArgBlockLength */
         sizeof (arg_block_void_t) - 1,
         (arg_block_t *)&arg_block_void));
   EXPECT_EQ (PD_STATE_PDactive, pde_get_state (port));
   exp_joberror_cnt++;

   pde_verify_smi_err (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID,
      0,
      exp_joberror_cnt);
}

TEST_F (PDETest, pde_PDOut_bad_argblock_len)
{
   iolink_arg_block_id_t ref_arg_block_id     = IOLINK_ARG_BLOCK_ID_PD_OUT;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   arg_block_pdout_t arg_block_pdout;

   uint8_t exp_joberror_cnt = mock_iolink_smi_joberror_cnt;

   pd_start (port);

   memset (&arg_block_pdout, 0, sizeof (arg_block_pdout_t));
   arg_block_pdout.h.arg_block_id = ref_arg_block_id;

   EXPECT_EQ (
      IOLINK_ERROR_PARAMETER_CONFLICT,
      SMI_PDOut_req (
         portnumber,
         exp_exp_arg_block_id,
         /* Bad ArgBlockLength */
         sizeof (arg_block_pdout_head_t),
         (arg_block_t *)&arg_block_pdout));
   EXPECT_EQ (PD_STATE_PDactive, pde_get_state (port));
   exp_joberror_cnt++;

   pde_verify_smi_err (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID,
      0,
      exp_joberror_cnt);

   EXPECT_EQ (
      IOLINK_ERROR_PARAMETER_CONFLICT,
      SMI_PDOut_req (
         portnumber,
         exp_exp_arg_block_id,
         /* Bad ArgBlockLength */
         sizeof (arg_block_pdout_t) + 1,
         (arg_block_t *)&arg_block_pdout));
   EXPECT_EQ (PD_STATE_PDactive, pde_get_state (port));
   exp_joberror_cnt++;

   pde_verify_smi_err (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID,
      0,
      exp_joberror_cnt);
}

TEST_F (PDETest, pde_PDInOut_bad_argblock_len)
{
   iolink_arg_block_id_t ref_arg_block_id     = IOLINK_ARG_BLOCK_ID_VOID_BLOCK;
   iolink_arg_block_id_t exp_exp_arg_block_id = IOLINK_ARG_BLOCK_ID_PD_IN_OUT;
   arg_block_void_t arg_block_void;

   uint8_t exp_joberror_cnt = mock_iolink_smi_joberror_cnt;

   pd_start (port);

   memset (&arg_block_void, 0, sizeof (arg_block_void_t));
   arg_block_void.arg_block_id = ref_arg_block_id;

   EXPECT_EQ (
      IOLINK_ERROR_PARAMETER_CONFLICT,
      SMI_PDInOut_req (
         portnumber,
         exp_exp_arg_block_id,
         /* Bad ArgBlockLength */
         sizeof (arg_block_void_t) + 1,
         (arg_block_t *)&arg_block_void));
   EXPECT_EQ (PD_STATE_PDactive, pde_get_state (port));
   exp_joberror_cnt++;

   pde_verify_smi_err (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID,
      0,
      exp_joberror_cnt);

   EXPECT_EQ (
      IOLINK_ERROR_PARAMETER_CONFLICT,
      SMI_PDInOut_req (
         portnumber,
         exp_exp_arg_block_id,
         /* Bad ArgBlockLength */
         sizeof (arg_block_void_t) + 1,
         (arg_block_t *)&arg_block_void));
   EXPECT_EQ (PD_STATE_PDactive, pde_get_state (port));
   exp_joberror_cnt++;

   pde_verify_smi_err (
      ref_arg_block_id,
      exp_exp_arg_block_id,
      IOLINK_SMI_ERRORTYPE_ARGBLOCK_LENGTH_INVALID,
      0,
      exp_joberror_cnt);
}
