/*! \file bcma_clicmd_counter.c
 *
 * CLI 'counter' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#ifdef CFG_SWITCH_STAT_INCLUDED

#include <appl/sdkcli/bcma_cli.h>

int
bcma_clicmd_counter(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    char *cmd;
    uint16 uport;
    port_stat_t stat;

    COMPILER_REFERENCE(cli);

    if (!(cmd = BCMA_CLI_ARG_GET(args))) {
        SAL_UPORT_ITER(uport) {
            board_port_stat_get(uport, &stat);
            if (stat.TxOctets_hi || stat.TxOctets_lo || stat.RxOctets_hi || stat.RxOctets_lo) {
                sal_printf(" --------------------------------------------------------------------- \r\n");
                sal_printf("|                                                                     |\r\n");
                sal_printf("|                    user port %02d statistics                          |\r\n", uport);
                sal_printf("|                                                                     |\r\n");
                sal_printf(" --------------------------------------------------------------------- \r\n");
                sal_printf("                                                                       \r\n");
                if (stat.TxOctets_hi || stat.TxOctets_lo) {
                    sal_printf("  Tx bytes   %08x%08x                                         \r\n", stat.TxOctets_hi, stat.TxOctets_lo);
                }
                if (stat.TxPkts_hi || stat.TxPkts_lo) {
                    sal_printf("  Tx packets %08x%08x                                         \r\n", stat.TxPkts_hi, stat.TxPkts_lo);
                }
                if (stat.TxUnicastPkts_hi || stat.TxUnicastPkts_lo) {
                    sal_printf("  Tx unicast packets %08x%08x                                 \r\n", stat.TxUnicastPkts_hi, stat.TxUnicastPkts_lo);
                }
                if (stat.TxMulticastPkts_hi || stat.TxMulticastPkts_lo) {
                    sal_printf("  Tx multicast packets %08x%08x                               \r\n", stat.TxMulticastPkts_hi, stat.TxMulticastPkts_lo);
                }
                if (stat.TxBroadcastPkts_hi || stat.TxBroadcastPkts_lo) {
                    sal_printf("  Tx broadcast packets %08x%08x                               \r\n", stat.TxBroadcastPkts_hi, stat.TxBroadcastPkts_lo);
                }
                if (stat.TxPauseFramePkts_hi || stat.TxPauseFramePkts_lo) {
                    sal_printf("  Tx pause frames    %08x%08x                                 \r\n", stat.TxPauseFramePkts_hi, stat.TxPauseFramePkts_lo);
                }
                if (stat.TxOversizePkts_hi || stat.TxOversizePkts_lo) {
                    sal_printf("  Tx oversize frames %08x%08x                                 \r\n", stat.TxOversizePkts_hi, stat.TxOversizePkts_lo);
                }
                if (stat.RxOctets_hi || stat.RxOctets_lo) {
                    sal_printf("  Rx bytes   %08x%08x                                         \r\n", stat.RxOctets_hi, stat.RxOctets_lo);
                }
                if (stat.RxPkts_hi || stat.RxPkts_lo) {
                    sal_printf("  Rx packets %08x%08x                                         \r\n", stat.RxPkts_hi, stat.RxPkts_lo);
                }
                if (stat.RxUnicastPkts_hi || stat.RxUnicastPkts_lo) {
                    sal_printf("  Rx unicast packets %08x%08x                                 \r\n", stat.RxUnicastPkts_hi, stat.RxUnicastPkts_lo);
                }
                if (stat.RxMulticastPkts_hi || stat.RxMulticastPkts_lo) {
                    sal_printf("  Rx multicast packets %08x%08x                               \r\n", stat.RxMulticastPkts_hi, stat.RxMulticastPkts_lo);
                }
                if (stat.RxBroadcastPkts_hi || stat.RxBroadcastPkts_lo) {
                    sal_printf("  Rx broadcast packets %08x%08x                               \r\n", stat.RxBroadcastPkts_hi, stat.RxBroadcastPkts_lo);
                }
                if (stat.RxPauseFramePkts_hi || stat.RxPauseFramePkts_lo) {
                    sal_printf("  Rx pause frames    %08x%08x                                 \r\n", stat.RxPauseFramePkts_hi, stat.RxPauseFramePkts_lo);
                }
                if (stat.RxOversizePkts_hi || stat.RxOversizePkts_lo) {
                    sal_printf("  Rx oversize frames %08x%08x                                 \r\n", stat.RxOversizePkts_hi, stat.RxOversizePkts_lo);
                }
                if (stat.CRCErrors_hi || stat.CRCErrors_lo) {
                    sal_printf("  Rx FCS Error Frames %08x%08x                                \r\n", stat.CRCErrors_hi, stat.CRCErrors_lo);
                }
                sal_printf("                                                                      \r\n");
                sal_printf("                                                                      \r\n");
           }
       }
   } else {
       if (!sal_strcmp(cmd, "clear")) {
           sal_printf("Clear all non-zero counters\n");
           SAL_UPORT_ITER(uport) {
               board_port_stat_get(uport, &stat);
               if (stat.TxOctets_hi || stat.TxOctets_lo || stat.RxOctets_hi || stat.RxOctets_lo) {
                   board_port_stat_clear(uport);
               }
          }
       } else {
           return BCMA_CLI_CMD_USAGE;
       }
   }

    return BCMA_CLI_CMD_OK;
}

#endif /* CFG_SWITCH_STAT_INCLUDED */
#endif /* CFG_SDKCLI_INCLUDED */
