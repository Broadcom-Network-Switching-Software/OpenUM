/*! \file bcma_clicmd_temp.c
 *
 * CLI 'TempQuery' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#if defined(CFG_SDKCLI_INCLUDED) && defined(CFG_TEMP_MONITOR_INCLUDED)

#include <appl/sdkcli/bcma_cli.h>

int
bcma_clicmd_temp(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    int32 temp_data[CFG_MAX_TEMP_MONITOR_CNT];
    int32 temp_cnt;
    int32 i, j;

    COMPILER_REFERENCE(cli);
    COMPILER_REFERENCE(args);

    board_temp_monitor_get(temp_data, CFG_MAX_TEMP_MONITOR_CNT, &temp_cnt);

    for (i=0; i < temp_cnt; i++) {
        /* The value in temp_data[] is 10 times of monitored temperture */
        j = temp_data[i] % 10;
        temp_data[i] /= 10;
        sal_printf("Monitored temperature %d = %d.%d C\n", i, temp_data[i], j);
    }

    return BCMA_CLI_CMD_OK;
}

#endif /* defined(CFG_SDKCLI_INCLUDED) && defined(CFG_TEMP_MONITOR_INCLUDED) */