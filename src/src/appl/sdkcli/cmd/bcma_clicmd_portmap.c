/*! \file bcma_clicmd_portmap.c
 *
 * CLI 'portmap' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED

#include <appl/sdkcli/bcma_cli.h>

int
bcma_clicmd_portmap(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    uint16 uport;
    uint8 unit = 0, lport;
    int pport, mmu_port, speed, lane_number, core_number;
    sys_error_t rv = SYS_OK;

    COMPILER_REFERENCE(cli);
    COMPILER_REFERENCE(args);

    sal_printf("U-Port  L-Port  P-Port  MMU-Port    Core     Lanes   MaxSpeed\n");
    sal_printf("=============================================================\n");
    SAL_UPORT_ITER(uport) {
        lport = -1;
        pport = -1;
        mmu_port = -1;
        speed = 0;
        lane_number = 0;
        core_number = 0;
        rv = board_uport_to_lport(uport, &unit, &lport);
        if (rv == SYS_OK) {
            pport = SOC_PORT_L2P_MAPPING(lport);
            if ((pport != -1) && (SOC_PORT_P2L_MAPPING(pport) != -1)) {
                mmu_port = SOC_PORT_P2M_MAPPING(pport);
                speed = SOC_PORT_SPEED_MAX(lport);
                lane_number = SOC_PORT_LANE_NUMBER(lport);
                if (IS_QTCE_PORT(lport)) {
                    core_number = QTCE_CORE_NUM_GET(lport);
                } else if (IS_TSCE_PORT(lport)) {
                    core_number = TSCE_CORE_NUM_GET(lport);
                } else {
                    core_number = TSCF_CORE_NUM_GET(lport);
                }
            }
        }
        if (IS_TSCF_PORT(lport)) {
            sal_printf("  %2d      %2d      %2d      %2d      Falcon-%d    %2d      %d\n",
                       uport, (int)lport, pport, mmu_port, core_number, lane_number, speed);
        } else {
            sal_printf("  %2d      %2d      %2d      %2d      Merlin-%d    %2d      %d\n",
                       uport, (int)lport, pport, mmu_port, core_number, lane_number, speed);
        }
   }

    return BCMA_CLI_CMD_OK;
}
#endif /* CFG_SDKCLI_INCLUDED */
