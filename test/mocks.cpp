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

#include "mocks.h"
#include "iolink_max14819.h"
#include "iolink_sm.h"
#include "iolink_al.h"
#include "iolink_main.h"
#include "iolink_types.h"

#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

uint8_t mock_iolink_revisionid = IOL_DIR_PARAM_REV_V11;
uint8_t mock_iolink_master_cycletime = 0;
uint8_t mock_iolink_min_cycletime = 79;
uint16_t mock_iolink_vendorid = 1171;
uint32_t mock_iolink_deviceid = 0x112345;
uint16_t mock_iolink_functionid = 0x999A;
iolink_transmission_rate_t mock_iolink_trans_rate =
                                             IOLINK_TRANSMISSION_RATE_COM1;

uint8_t mock_iolink_restart_device_cnt = 0;
uint8_t mock_iolink_mastercmd_master_ident_cnt = 0;
uint8_t mock_iolink_mastercmd_preop_cnt = 0;
uint8_t mock_iolink_write_devmode_inactive_cnt = 0;
uint8_t mock_iolink_write_devmode_operate_cnt = 0;
uint8_t mock_iolink_al_data [64] = {0};
uint8_t mock_iolink_al_data_len = 0;
uint16_t mock_iolink_al_data_index = 0;
uint8_t mock_iolink_al_data_subindex = 0;
uint8_t mock_iolink_al_read_cnf_cnt = 0;
uint8_t mock_iolink_al_read_req_cnt = 0;
uint8_t mock_iolink_al_write_cnf_cnt = 0;
uint8_t mock_iolink_al_write_req_cnt = 0;
uint8_t mock_iolink_al_control_req_cnt = 0;
uint8_t mock_iolink_al_control_ind_cnt = 0;
uint8_t mock_iolink_dl_pdout_data [IOLINK_PD_MAX_SIZE] = {0};
uint8_t mock_iolink_dl_pdout_data_len = 0;
uint8_t mock_iolink_dl_pdin_data [IOLINK_PD_MAX_SIZE] = {0};
uint8_t mock_iolink_dl_pdin_data_len = 0;
uint8_t mock_iolink_dl_control_req_cnt = 0;
uint8_t mock_iolink_dl_eventconf_req_cnt = 0;
uint8_t mock_iolink_al_setoutput_req_cnt = 0;
uint8_t mock_iolink_al_getinput_req_cnt = 0;
uint8_t mock_iolink_al_getinputoutput_req_cnt = 0;
uint8_t mock_iolink_al_newinput_inf_cnt = 0;
uint8_t mock_iolink_al_event_cnt = 0;
uint8_t mock_iolink_sm_operate_cnt = 0;
uint8_t mock_iolink_ds_delete_cnt = 0;
uint8_t mock_iolink_ds_startup_cnt = 0;
uint8_t mock_iolink_ds_ready_cnt = 0;
uint8_t mock_iolink_ds_fault_cnt = 0;
iolink_ds_fault_t mock_iolink_ds_fault = IOLINK_DS_FAULT_NONE;
uint8_t mock_iolink_od_start_cnt = 0;
uint8_t mock_iolink_od_stop_cnt = 0;
uint8_t mock_iolink_pd_start_cnt = 0;
uint8_t mock_iolink_pd_stop_cnt = 0;
diag_entry_t mock_iolink_al_events[6];
iolink_sm_portmode_t mock_iolink_sm_portmode = IOLINK_SM_PORTMODE_INACTIVE;
iolink_smi_errortypes_t mock_iolink_al_read_errortype = IOLINK_SMI_ERRORTYPE_NONE;
iolink_smi_errortypes_t mock_iolink_al_write_errortype = IOLINK_SMI_ERRORTYPE_NONE;
iolink_controlcode_t mock_iolink_controlcode = IOLINK_CONTROLCODE_NONE;
void (*mock_iolink_al_write_cnf_cb)(iolink_port_t * port,
                                    iolink_smi_errortypes_t errortype);
void (*mock_iolink_al_read_cnf_cb)(iolink_port_t * port, uint8_t len,
                                   uint8_t * data,
                                   iolink_smi_errortypes_t errortype);

uint8_t mock_iolink_smi_cnf_cnt = 0;
uint8_t mock_iolink_smi_joberror_cnt = 0;
uint8_t mock_iolink_smi_portcfg_cnf_cnt = 0;
uint8_t mock_iolink_smi_portstatus_cnf_cnt = 0;
uint8_t mock_iolink_smi_portevent_ind_cnt = 0;
uint8_t arg_block_buff[sizeof(arg_block_t) + 64];
arg_block_t * mock_iolink_smi_arg_block = (arg_block_t *)arg_block_buff;
uint16_t mock_iolink_smi_arg_block_len = 0;
iolink_arg_block_id_t mock_iolink_smi_ref_arg_block_id =
                                          IOLINK_ARG_BLOCK_ID_MASTERIDENT;

iolink_smp_parameterlist_t mock_iolink_cfg_paraml;
iolink_job_t mock_iolink_job;

iolink_error_t mock_DL_Write_req (iolink_port_t * port, uint8_t address,
                                  uint8_t value)
{
   (void) port;
   if (address == IOL_DIR_PARAMA_MASTER_CMD) {
      if (value == IOL_MASTERCMD_DEVICE_IDENT)
         mock_iolink_restart_device_cnt++;
      else if (value == IOL_MASTERCMD_MASTER_IDENT)
         mock_iolink_mastercmd_master_ident_cnt++;
      else if (value == IOL_MASTERCMD_DEVICE_PREOP)
         mock_iolink_mastercmd_preop_cnt++;
   }
   DL_Write_cnf (port, IOLINK_STATUS_NO_ERROR);
   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_DL_Read_req (iolink_port_t * port, uint8_t address)
{
   uint8_t data;

   switch (address)
   {
   case IOL_DIR_PARAMA_MASTER_CMD:
      data = 0;
      break;
   case IOL_DIR_PARAMA_MASTER_CYCL:
      data = mock_iolink_master_cycletime;
      break;
   case IOL_DIR_PARAMA_MIN_CYCL:
      data = mock_iolink_min_cycletime;
      break;
   case IOL_DIR_PARAMA_MSEQ_CAP:
      data = 0;
      break;
   case IOL_DIR_PARAMA_REV_ID:
      data = mock_iolink_revisionid;
      break;
   case IOL_DIR_PARAMA_PDI:
      data = 0;
      break;
   case IOL_DIR_PARAMA_PDO:
      data = 0;
      break;
   case IOL_DIR_PARAMA_VID_1:
      data = mock_iolink_vendorid >> 8;
      break;
   case IOL_DIR_PARAMA_VID_2:
      data = mock_iolink_vendorid & 0xFF;
      break;
   case IOL_DIR_PARAMA_DID_1:
      data = mock_iolink_deviceid >> 16;
      break;
   case IOL_DIR_PARAMA_DID_2:
      data = mock_iolink_deviceid >> 8;
      break;
   case IOL_DIR_PARAMA_DID_3:
      data = mock_iolink_deviceid & 0xFF;
      break;
   case IOL_DIR_PARAMA_FID_1:
      data = 0;
      break;
   case IOL_DIR_PARAMA_FID_2:
      data = 0;
      break;
   case IOL_DIR_PARAMA_SYS_CMD:
      data = 0;
      break;
   default:
      printf ("DL_Read: Address not valid: %u\n", address);
      return IOLINK_ERROR_ADDRESS_INVALID;
   }
   DL_Read_cnf (port, data, IOLINK_STATUS_NO_ERROR);

   //IOLINK_DL_EVENT_TIMEOUT;
   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_DL_SetMode_req (iolink_port_t * port, iolink_dl_mode_t mode,
                                    iolink_mode_vl_t *valuelist)
{
   switch (mode)
   {
   case IOLINK_DLMODE_INACTIVE:
      break;
   case IOLINK_DLMODE_STARTUP:
      break;
   case IOLINK_DLMODE_PREOPERATE:
      break;
   case IOLINK_DLMODE_OPERATE:
      break;
   }
   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_DL_Write_Devicemode_req (iolink_port_t * port,
                                             iolink_dl_mode_t devicemode)
{
   switch (devicemode)
   {
   case IOLINK_DLMODE_INACTIVE:
      mock_iolink_write_devmode_inactive_cnt++;
      break;
   case IOLINK_DLMODE_OPERATE:
      mock_iolink_write_devmode_operate_cnt++;
      break;
   default:
      CC_ASSERT (0);
   }

   DL_Write_Devicemode_cnf (port, IOLINK_STATUS_NO_ERROR, devicemode);

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_DL_ISDUTransport_req (iolink_port_t * port,
                                          iolink_isdu_vl_t *valuelist)
{
   if (valuelist->readwrite == IOLINK_RWDIRECTION_WRITE)
   {
      if (valuelist->length <= sizeof(mock_iolink_al_data))
      {
         mock_iolink_al_data_len = valuelist->length;
         memcpy (mock_iolink_al_data, valuelist->data_write, valuelist->length);
      }
      else
      {
         CC_ASSERT (0);
      }
   }
   else if (valuelist->readwrite == IOLINK_RWDIRECTION_READ)
   {

   }
   else
   {
      CC_ASSERT (0);
   }

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_DL_EventConf_req (iolink_port_t * port)
{
   mock_iolink_dl_eventconf_req_cnt++;
   return IOLINK_ERROR_NONE;
}
iolink_error_t mock_DL_ReadParam_req (iolink_port_t * port, uint8_t address)
{
   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_DL_WriteParam_req (iolink_port_t * port, uint8_t address,
                                       uint8_t value)
{
   mock_iolink_al_data_len = 1;
   mock_iolink_al_data[0] = value;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_DL_Control_req (iolink_port_t * port,
                              iolink_controlcode_t controlcode,
                              void (*dl_control_cnf_cb)(iolink_port_t * port))
{
   mock_iolink_dl_control_req_cnt++;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_DL_PDOutputGet_req (iolink_port_t * port, uint8_t * len,
                                        uint8_t * data)
{
   *len = mock_iolink_dl_pdout_data_len;
   memcpy (data, mock_iolink_dl_pdout_data, *len);

   return IOLINK_ERROR_NONE;
}

void mock_PL_SetMode_req (iolink_port_t * port, iolink_pl_mode_t mode) {}
void mock_iolink_pl_init (iolink_port_t * port, const char * name) {}

iolink_error_t mock_AL_Read_req (iolink_port_t * port, uint16_t index,
                     uint8_t subindex,
                     void (*al_read_cnf_cb)(iolink_port_t * port,
                                            uint8_t len, uint8_t * data,
                                            iolink_smi_errortypes_t errortype))
{
   mock_iolink_al_read_cnf_cb = al_read_cnf_cb;
   mock_iolink_al_read_req_cnt++;
   mock_iolink_al_data_index = index;
   mock_iolink_al_data_subindex = subindex;

   return IOLINK_ERROR_NONE;
}

void mock_AL_Read_cnf (iolink_port_t * port, uint8_t len, const uint8_t * data,
                       iolink_smi_errortypes_t errortype)
{
   if (errortype == IOLINK_SMI_ERRORTYPE_NONE)
   {
      mock_iolink_al_data_len = len;
      memcpy (mock_iolink_al_data, data, len);
   }
   mock_iolink_al_read_cnf_cnt++;
   mock_iolink_al_read_errortype = errortype;
}

void mock_AL_Event_ind (iolink_port_t * port, uint8_t event_cnt,
                        diag_entry_t events[6])
{
   mock_iolink_al_event_cnt = event_cnt;
   memcpy (mock_iolink_al_events, events, event_cnt * sizeof(diag_entry_t));
}

iolink_error_t mock_AL_Write_req (iolink_port_t * port, uint16_t index,
                        uint8_t subindex, uint8_t len,
                        const uint8_t * data,
                        void (*al_write_cb)(iolink_port_t * port,
                                            iolink_smi_errortypes_t errortype))
{
   mock_iolink_al_write_req_cnt++;
   mock_iolink_al_write_cnf_cb = al_write_cb;

   memcpy (mock_iolink_al_data, data, len);
   mock_iolink_al_data_index = index;
   mock_iolink_al_data_subindex = subindex;

   return IOLINK_ERROR_NONE;
}

void mock_AL_Write_cnf (iolink_port_t * port, iolink_smi_errortypes_t errortype)
{
   mock_iolink_al_write_cnf_cnt++;
   mock_iolink_al_write_errortype = errortype;
}

iolink_error_t mock_AL_Control_req (iolink_port_t * port,
                              iolink_controlcode_t controlcode)
{
   mock_iolink_al_control_req_cnt++;
   mock_iolink_controlcode = controlcode;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_AL_Control_ind (iolink_port_t * port,
                                    iolink_controlcode_t controlcode)
{
   mock_iolink_al_control_ind_cnt++;
   mock_iolink_controlcode = controlcode;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_AL_SetOutput_req (iolink_port_t * port, uint8_t len,
                                      uint8_t * data)
{
   if (len <= sizeof(mock_iolink_dl_pdout_data))
   {
      mock_iolink_dl_pdout_data_len = len;
      memcpy (mock_iolink_dl_pdout_data, data, len);
   }
   else
   {
      CC_ASSERT (0);
   }
   mock_iolink_al_setoutput_req_cnt++;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_AL_GetInput_req (iolink_port_t * port, uint8_t * len,
                                         uint8_t * data)
{
   *len = mock_iolink_dl_pdin_data_len;
   memcpy (data, mock_iolink_dl_pdin_data, *len);
   mock_iolink_al_getinput_req_cnt++;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_AL_GetInputOutput_req (iolink_port_t * port, uint8_t * len,
                                           uint8_t * data)
{
   uint8_t pdin_len;
   uint8_t * pdin_data;
   uint8_t * pdout_len;
   uint8_t * pdout_data;

   /* PDIn data length */
   pdin_len = mock_iolink_dl_pdin_data_len;
   data[0] = pdin_len;
   /* PDIn_data[0] */
   pdin_data = &data[1];
   memcpy (pdin_data, mock_iolink_dl_pdin_data, pdin_len);
   /* PDOut data length */
   pdout_len = &data[pdin_len + 1];
   /* PDOut_data[0] */
   pdout_data = &data[pdin_len + 2];

   *pdout_len = mock_iolink_dl_pdout_data_len;
   memcpy (pdout_data, mock_iolink_dl_pdout_data, *pdout_len);

   *len = 1 + pdin_len + 1 + *pdout_len;

   mock_iolink_al_getinputoutput_req_cnt++;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_AL_NewInput_ind (iolink_port_t * port)
{
   mock_iolink_al_newinput_inf_cnt++;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_DL_PDOutputUpdate_req (iolink_port_t * port, uint8_t len,
                                           uint8_t *outputdata)
{
   if (len <= sizeof(mock_iolink_dl_pdout_data))
   {
      mock_iolink_dl_pdout_data_len = len;
      memcpy (mock_iolink_dl_pdout_data, outputdata, len);
   }
   else
   {
      CC_ASSERT (0);
   }

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_SM_Operate (iolink_port_t * port)
{
   mock_iolink_sm_operate_cnt++;

   return IOLINK_ERROR_NONE;
}
void mock_SM_PortMode_ind (iolink_port_t * port, iolink_sm_portmode_t mode)
{
   mock_iolink_sm_portmode = mode;
}

iolink_error_t mock_SM_SetPortConfig_req (iolink_port_t * port,
                                    iolink_smp_parameterlist_t * parameterlist)
{
   iolink_sm_port_t * sm = iolink_get_sm_ctx (port);
   iolink_smp_parameterlist_t * real_paramlist = &sm->real_paramlist;

   memcpy (&mock_iolink_cfg_paraml, parameterlist,
           sizeof(mock_iolink_cfg_paraml));

   switch (mock_iolink_trans_rate)
   {
   case IOLINK_TRANSMISSION_RATE_COM1:
      sm->comrate = IOLINK_MHMODE_COM1;
      break;
   case IOLINK_TRANSMISSION_RATE_COM2:
      sm->comrate = IOLINK_MHMODE_COM2;
      break;
   case IOLINK_TRANSMISSION_RATE_COM3:
      sm->comrate = IOLINK_MHMODE_COM3;
      break;
   case IOLINK_TRANSMISSION_RATE_NOT_DETECTED:
      sm->comrate = IOLINK_MHMODE_INACTIVE;
      break;
   default:
      CC_ASSERT (0);
      break;
   }

   if (parameterlist->revisionid)
      real_paramlist->revisionid = parameterlist->revisionid;
   else
      real_paramlist->revisionid = mock_iolink_revisionid;
   if (parameterlist->cycletime)
      real_paramlist->cycletime = parameterlist->cycletime;
   else
      real_paramlist->cycletime = mock_iolink_min_cycletime;

   if (parameterlist->vendorid)
      real_paramlist->vendorid = parameterlist->vendorid;
   else
      real_paramlist->vendorid = mock_iolink_vendorid;

   if (parameterlist->deviceid)
      real_paramlist->deviceid = parameterlist->deviceid;
   else
      real_paramlist->deviceid = mock_iolink_deviceid;

   return IOLINK_ERROR_NONE;
}

void mock_DS_Fault (iolink_port_t * port, iolink_ds_fault_t fault)
{
   mock_iolink_ds_fault = fault;
   mock_iolink_ds_fault_cnt++;
}

iolink_error_t mock_DS_Delete (iolink_port_t * port)
{
   mock_iolink_ds_delete_cnt++;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_DS_Startup (iolink_port_t * port)
{
   mock_iolink_ds_startup_cnt++;

   return IOLINK_ERROR_NONE;
}

void mock_DS_Ready (iolink_port_t * port)
{
   mock_iolink_ds_ready_cnt++;
}

iolink_error_t mock_OD_Start (iolink_port_t * port)
{
   mock_iolink_od_start_cnt++;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_OD_Stop (iolink_port_t * port)
{
   mock_iolink_od_stop_cnt++;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_PD_Start (iolink_port_t * port)
{
   mock_iolink_pd_start_cnt++;

   return IOLINK_ERROR_NONE;
}

iolink_error_t mock_PD_Stop (iolink_port_t * port)
{
   mock_iolink_pd_stop_cnt++;

   return IOLINK_ERROR_NONE;
}

void mock_SMI_cnf (void * arg, uint8_t portnumber,
                   iolink_arg_block_id_t ref_arg_block_id,
                   uint16_t arg_block_len, arg_block_t * arg_block)
{
   iolink_arg_block_id_t arg_block_id = arg_block->void_block.arg_block_id;

   if (arg_block_id == IOLINK_ARG_BLOCK_ID_JOB_ERROR)
      mock_iolink_smi_joberror_cnt++;
   else if (ref_arg_block_id == IOLINK_ARG_BLOCK_ID_PORT_CFG_LIST)
      mock_iolink_smi_portcfg_cnf_cnt++;
   else if (arg_block_id == IOLINK_ARG_BLOCK_ID_PORT_STATUS_LIST)
     mock_iolink_smi_portstatus_cnf_cnt++;
   else if (arg_block_id == IOLINK_ARG_BLOCK_ID_PORT_EVENT)
      mock_iolink_smi_portevent_ind_cnt++;
   else
      mock_iolink_smi_cnf_cnt++;

   memcpy (mock_iolink_smi_arg_block, arg_block, arg_block_len);

   mock_iolink_smi_arg_block_len = arg_block_len;
   mock_iolink_smi_ref_arg_block_id = ref_arg_block_id;
}

static bool mock_os_mbox_post (os_mbox_t * mbox, void * msg,
                                      uint32_t time)
{
   iolink_job_t * job = (iolink_job_t *)msg;

   CC_ASSERT (job != NULL);
   CC_ASSERT (job->port != NULL);

   if (job)
   {
      CC_ASSERT (job == &mock_iolink_job);
   }

   return false;
}

bool mock_iolink_post_job (iolink_port_t * port, iolink_job_t * job)
{
   return mock_os_mbox_post (NULL, job, 0);
}

iolink_job_t * mock_iolink_fetch_avail_job (iolink_port_t * port)
{
   iolink_job_t * job = &mock_iolink_job;

   job->port = port;

   return job;
}

iolink_job_t * mock_iolink_fetch_avail_api_job (iolink_port_t * port)
{
   iolink_job_t * job = &mock_iolink_job;

   job->port = port;

   return job;
}
