/*! \file board_temp.c
 *
 * Chip monitored temperature query routine.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_TEMP_MONITOR_INCLUDED

sys_error_t
board_temp_monitor_get(int32 *temp_data, int32 temp_data_size, int32 *temp_mon_cnt)
{
    TOP_TMON0_RESULTr_t tmon0;
    TOP_TMON1_RESULTr_t tmon1;
    TOP_SOFT_RESET_REG_2r_t rst2;
    uint32 fval;
    uint32 temp_cnt = 0;
    int32 temp;

    if (temp_data_size < CFG_MAX_TEMP_MONITOR_CNT)
        return SYS_ERR_PARAMETER;

    READ_TOP_TMON0_RESULTr(0, tmon0);
    if (TOP_TMON0_RESULTr_TMON_DATA_VALIDf_GET(tmon0)) {
        fval = TOP_TMON0_RESULTr_TMON_DATAf_GET(tmon0);
        /* 10 times of monitored temperature (C) */
        temp = (45799000 - (55010 * fval)) / 10000;
        temp_data[0] = temp;
        temp_cnt++;
        sal_printf("10 times of temperature 0 = %d\n", temp);
    }

    READ_TOP_TMON1_RESULTr(0, tmon1);
    if (TOP_TMON1_RESULTr_TMON_DATA_VALIDf_GET(tmon1)) {
        fval = TOP_TMON1_RESULTr_TMON_DATAf_GET(tmon1);
        /* 10 times of monitored temperature (C) */
        temp = (45799000 - (55010 * fval)) / 10000;
        temp_data[1] = temp;
        temp_cnt++;
        sal_printf("10 times of temperature 1 = %d\n", temp);
    }

    *temp_mon_cnt = temp_cnt;

    READ_TOP_SOFT_RESET_REG_2r(0, rst2);
    TOP_SOFT_RESET_REG_2r_TOP_TEMP_MON_PEAK_RST_Lf_SET(rst2, 0);
    WRITE_TOP_SOFT_RESET_REG_2r(0, rst2);
    TOP_SOFT_RESET_REG_2r_TOP_TEMP_MON_PEAK_RST_Lf_SET(rst2, 1);
    WRITE_TOP_SOFT_RESET_REG_2r(0, rst2);

    return SYS_OK;
}

#endif /* CFG_TEMP_MONITOR_INCLUDED */

