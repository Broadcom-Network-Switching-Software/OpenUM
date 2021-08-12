/*! \file bcma_clicmd_add_pktdma_cmds.c
 *
 * CLI tx,rx and pkttest commands support.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED

#include <appl/sdkcli/bcma_cli.h>

#include <appl/sdkcli/bcma_clicmd.h>
#include <appl/sdkcli/bcma_clicmd_tx.h>
#include <appl/sdkcli/bcma_clicmd_rx.h>
#include <appl/sdkcli/bcma_clicmd_pkttest.h>

/* CLI pktdma command set */
#if CFG_RXTX_SUPPORT_ENABLED

static bcma_cli_command_t cmd_pkttx = {
    .name = "tx",
    .func = bcma_bcmpktcmd_tx,
    .desc = BCMA_BCMPKTCMD_TX_DESC,
    .synop = BCMA_BCMPKTCMD_TX_SYNOP,
    .help = { BCMA_BCMPKTCMD_TX_HELP },
    .examples = BCMA_BCMPKTCMD_TX_EXAMPLES
};

static bcma_cli_command_t cmd_pktrx = {
    .name = "rx",
    .func = bcma_bcmpktcmd_rx,
    .desc = BCMA_BCMPKTCMD_RX_DESC,
    .synop = BCMA_BCMPKTCMD_RX_SYNOP,
    .help = { BCMA_BCMPKTCMD_RX_HELP },
    .examples = BCMA_BCMPKTCMD_RX_EXAMPLES
};

static bcma_cli_command_t cmd_pkttest = {
    .name = "pkttest",
    .func = bcma_bcmpktcmd_pkttest,
    .desc = BCMA_BCMPKTCMD_PKTTEST_DESC,
    .synop = BCMA_BCMPKTCMD_PKTTEST_SYNOP,
    .help = { BCMA_BCMPKTCMD_PKTTEST_HELP },
    .examples = BCMA_BCMPKTCMD_PKTTEST_EXAMPLES
};
#endif

int
bcma_clicmd_add_pktdma_cmds(bcma_cli_t *cli)
{
#if CFG_RXTX_SUPPORT_ENABLED
    bcma_cli_add_command(cli, &cmd_pkttx, 0);
    bcma_cli_add_command(cli, &cmd_pktrx, 0);
    bcma_cli_add_command(cli, &cmd_pkttest, 0);
#endif /* CFG_RXTX_SUPPORT_ENABLED */
    return 0;
}
#endif /* CFG_SDKCLI_INCLUDED */
