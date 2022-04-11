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

#ifndef MOCKS_H
#define MOCKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "iolink_sm.h"
#include "iolink_max14819.h"
#include "osal.h"

#include <cstddef>
#include <stdint.h>

extern uint8_t mock_iolink_revisionid;
extern uint8_t mock_iolink_master_cycletime;
extern uint8_t mock_iolink_min_cycletime;
extern uint16_t mock_iolink_vendorid;
extern uint32_t mock_iolink_deviceid;
extern uint16_t mock_iolink_functionid;
extern iolink_transmission_rate_t mock_iolink_trans_rate;

extern uint8_t mock_iolink_restart_device_cnt;
extern uint8_t mock_iolink_mastercmd_master_ident_cnt;
extern uint8_t mock_iolink_mastercmd_preop_cnt;
extern uint8_t mock_iolink_write_devmode_inactive_cnt;
extern uint8_t mock_iolink_write_devmode_operate_cnt;
extern iolink_job_t mock_iolink_job;
extern uint8_t mock_iolink_al_data[64];
extern uint8_t mock_iolink_al_data_len;
extern uint16_t mock_iolink_al_data_index;
extern uint8_t mock_iolink_al_data_subindex;
extern uint8_t mock_iolink_al_read_cnf_cnt;
extern uint8_t mock_iolink_al_read_req_cnt;
extern uint8_t mock_iolink_al_write_cnf_cnt;
extern uint8_t mock_iolink_al_write_req_cnt;
extern uint8_t mock_iolink_al_control_req_cnt;
extern uint8_t mock_iolink_al_control_ind_cnt;
extern uint8_t mock_iolink_dl_pdout_data[IOLINK_PD_MAX_SIZE];
extern uint8_t mock_iolink_dl_pdout_data_len;
extern uint8_t mock_iolink_dl_pdin_data[IOLINK_PD_MAX_SIZE];
extern uint8_t mock_iolink_dl_pdin_data_len;
extern uint8_t mock_iolink_dl_control_req_cnt;
extern uint8_t mock_iolink_dl_eventconf_req_cnt;
extern uint8_t mock_iolink_al_getinput_req_cnt;
extern uint8_t mock_iolink_al_getinputoutput_req_cnt;
extern uint8_t mock_iolink_al_newinput_inf_cnt;
extern iolink_smi_errortypes_t mock_iolink_al_read_errortype;
extern iolink_smi_errortypes_t mock_iolink_al_write_errortype;
extern iolink_controlcode_t mock_iolink_controlcode;
extern uint8_t mock_iolink_al_event_cnt;
extern uint8_t mock_iolink_sm_operate_cnt;
extern uint8_t mock_iolink_ds_delete_cnt;
extern uint8_t mock_iolink_ds_startup_cnt;
extern uint8_t mock_iolink_ds_ready_cnt;
extern uint8_t mock_iolink_ds_fault_cnt;
extern iolink_ds_fault_t mock_iolink_ds_fault;
extern uint8_t mock_iolink_od_start_cnt;
extern uint8_t mock_iolink_od_stop_cnt;
extern uint8_t mock_iolink_pd_start_cnt;
extern uint8_t mock_iolink_pd_stop_cnt;
extern diag_entry_t mock_iolink_al_events[6];
extern iolink_sm_portmode_t mock_iolink_sm_portmode;
extern iolink_smp_parameterlist_t mock_iolink_cfg_paraml;

extern arg_block_t * mock_iolink_smi_arg_block;
extern uint8_t mock_iolink_smi_cnf_cnt;
extern uint8_t mock_iolink_smi_joberror_cnt;
extern uint8_t mock_iolink_smi_portcfg_cnf_cnt;
extern uint8_t mock_iolink_smi_portstatus_cnf_cnt;
extern uint8_t mock_iolink_smi_portevent_ind_cnt;
extern uint16_t mock_iolink_smi_arg_block_len;
extern iolink_arg_block_id_t mock_iolink_smi_ref_arg_block_id;

extern void (*mock_iolink_al_write_cnf_cb) (
   iolink_port_t * port,
   iolink_smi_errortypes_t errortype);
extern void (*mock_iolink_al_read_cnf_cb) (
   iolink_port_t * port,
   uint8_t len,
   uint8_t * data,
   iolink_smi_errortypes_t errortype);

iolink_error_t mock_DL_Write_req (
   iolink_port_t * port,
   uint8_t address,
   uint8_t value);
iolink_error_t mock_DL_Read_req (iolink_port_t * port, uint8_t address);
iolink_error_t mock_DL_SetMode_req (
   iolink_port_t * port,
   iolink_dl_mode_t mode,
   iolink_mode_vl_t * valuelist);
iolink_error_t mock_DL_Write_Devicemode_req (
   iolink_port_t * port,
   iolink_dl_mode_t devicemode);
iolink_error_t mock_DL_EventConf_req (iolink_port_t * port);
iolink_error_t mock_DL_ReadParam_req (iolink_port_t * port, uint8_t address);
iolink_error_t mock_DL_WriteParam_req (
   iolink_port_t * port,
   uint8_t address,
   uint8_t value);
iolink_error_t mock_DL_ISDUTransport_req (
   iolink_port_t * port,
   iolink_isdu_vl_t * valuelist);
iolink_error_t mock_DL_Control_req (
   iolink_port_t * port,
   iolink_controlcode_t controlcode,
   void (*dl_control_cnf_cb) (iolink_port_t * port));
iolink_error_t mock_DL_PDOutputGet_req (
   iolink_port_t * port,
   uint8_t * len,
   uint8_t * data);
iolink_error_t mock_DL_PDOutputUpdate_req (
   iolink_port_t * port,
   uint8_t len,
   uint8_t * outputdata);
iolink_error_t mock_DL_PDOutputUpdate (
   iolink_port_t * port,
   uint8_t len,
   uint8_t * outputdata);
iolink_error_t mock_AL_Read_req (
   iolink_port_t * port,
   uint16_t index,
   uint8_t subindex,
   void (*al_read_cnf_cb) (
      iolink_port_t * port,
      uint8_t len,
      uint8_t * data,
      iolink_smi_errortypes_t errortype));
void mock_AL_Read_cnf (
   iolink_port_t * port,
   uint8_t len,
   const uint8_t * data,
   iolink_smi_errortypes_t errortype);
iolink_error_t mock_AL_Write_req (
   iolink_port_t * port,
   uint16_t index,
   uint8_t subindex,
   uint8_t len,
   const uint8_t * data,
   void (*al_write_cb) (iolink_port_t * port, iolink_smi_errortypes_t errortype));
void mock_AL_Write_cnf (iolink_port_t * port, iolink_smi_errortypes_t errortype);
void mock_AL_Event_ind (
   iolink_port_t * port,
   uint8_t event_cnt,
   diag_entry_t events[6]);
iolink_error_t mock_AL_Control_req (
   iolink_port_t * port,
   iolink_controlcode_t controlcode);
iolink_error_t mock_AL_Control_ind (
   iolink_port_t * port,
   iolink_controlcode_t controlcode);
iolink_error_t mock_AL_SetOutput_req (
   iolink_port_t * port,
   uint8_t len,
   uint8_t * data);
iolink_error_t mock_AL_GetInputOutput_req (
   iolink_port_t * port,
   uint8_t * len,
   uint8_t * data);
iolink_error_t mock_AL_GetInput_req (
   iolink_port_t * port,
   uint8_t * len,
   uint8_t * data);
iolink_error_t mock_AL_NewInput_ind (iolink_port_t * port);

void mock_PL_SetMode_req (iolink_port_t * port, iolink_pl_mode_t mode);
void mock_iolink_pl_init (iolink_port_t * port, const char * name);

iolink_error_t mock_SM_Operate (iolink_port_t * port);

void mock_SM_PortMode_ind (iolink_port_t * port, iolink_sm_portmode_t mode);

void mock_DS_Fault (iolink_port_t * port, iolink_ds_fault_t fault);
iolink_error_t mock_DS_Delete (iolink_port_t * port);
iolink_error_t mock_DS_Startup (iolink_port_t * port);

void mock_DS_Ready (iolink_port_t * port);

iolink_error_t mock_OD_Start (iolink_port_t * port);
iolink_error_t mock_OD_Stop (iolink_port_t * port);
iolink_error_t mock_PD_Start (iolink_port_t * port);
iolink_error_t mock_PD_Stop (iolink_port_t * port);

void mock_SMI_cnf (
   void * arg,
   uint8_t portnumber,
   iolink_arg_block_id_t ref_arg_block_id,
   uint16_t arg_block_len,
   arg_block_t * arg_block);

iolink_error_t mock_SM_SetPortConfig_req (
   iolink_port_t * port,
   iolink_smp_parameterlist_t * parameterlist);

bool mock_iolink_post_job (iolink_port_t * port, iolink_job_t * job);
iolink_job_t * mock_iolink_fetch_avail_job (iolink_port_t * port);
iolink_job_t * mock_iolink_fetch_avail_api_job (iolink_port_t * port);

#ifdef __cplusplus
}
#endif

#endif /* MOCKS_H */
