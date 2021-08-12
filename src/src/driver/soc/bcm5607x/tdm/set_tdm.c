/*
 * $Id: set_tdm.c,v 1.1 Broadcom SDK $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * File:        set_tdm.c
 * Purpose:     TDM algorithm
 */
//#include <cdk/cdk_string.h>
#include "system.h"

#include "tdm_top.h"
#include "tdm_fl_top.h"

uint32 cdk_debug_level = CDK_DBG_ERR | CDK_DBG_WARN;
int (*cdk_debug_printf)(const char *format,...) = cdk_printf;

#ifndef _TDM_STANDALONE
tdm_mod_t
*SOC_SEL_TDM(tdm_soc_t *chip, tdm_mod_t *_tdm)
{
    int (*core_exec[TDM_EXEC_CORE_SIZE])( tdm_mod_t* ) = {
        &tdm_core_init,
        &tdm_core_post,
        &tdm_fl_proc_cal,
        &tdm_core_vbs_scheduler_wrapper,
        &tdm_fl_shim_core_vbs_scheduler_ovs,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_acc_alloc,
        &tdm_core_vmap_prealloc,
        &tdm_core_vmap_alloc_mix,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_fl_shim_get_pipe_ethernet,
        &tdm_pick_vec,
        &tdm_fl_shim_get_port_pm,
    };
    int (*chip_exec[TDM_EXEC_CHIP_SIZE])( tdm_mod_t* ) = {
        &tdm_fl_main_init,
        &tdm_fl_main_transcription,
        &tdm_fl_main_ingress,
        &tdm_core_null,
        &tdm_fl_filter,
        &tdm_fl_parse_tdm_tbl,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_core_null,
        &tdm_fl_chk,
        &tdm_fl_main_free,
        &tdm_fl_main_corereq,
        &tdm_fl_main_post
    };

    if (!_tdm) {
        return NULL;
    }

    TDM_COPY(&(_tdm->_chip_data.soc_pkg),chip,sizeof(tdm_soc_t));

    TDM_COPY(_tdm->_core_exec,core_exec,sizeof(_tdm->_core_exec));
    TDM_COPY(_tdm->_chip_exec,chip_exec,sizeof(_tdm->_chip_exec));

    return _tdm;
}
#endif



tdm_mod_t
*_soc_set_tdm_tbl( tdm_mod_t *_tdm )
{
    int idx, tdm_ver_chk[8];

    if (!_tdm) {
        return NULL;
    }
    TDM_BIG_BAR
    TDM_PRINT0("TDM: Release version: ");
    _soc_tdm_ver(tdm_ver_chk);
    TDM_PRINT2("%d%d",tdm_ver_chk[0],tdm_ver_chk[1]);
    for (idx=2; idx<8; idx+=2) {
        TDM_PRINT2(".%d%d",tdm_ver_chk[idx],tdm_ver_chk[idx+1]);
    }
    TDM_PRINT0("\n"); TDM_SML_BAR

    /* Path virtualized API starting in chip executive */
    return ((_tdm->_chip_exec[TDM_CHIP_EXEC__INIT](_tdm))==PASS)?(_tdm):(NULL);
}
