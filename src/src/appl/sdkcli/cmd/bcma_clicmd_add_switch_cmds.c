/*! \file bcma_clicmd_add_switch_cmds.c
 *
 * CLI switch commands support.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED

#include <appl/sdkcli/bcma_cli.h>

#include <appl/sdkcli/bcma_clicmd_tick.h>
#include <appl/sdkcli/bcma_clicmd_counter.h>
#include <appl/sdkcli/bcma_clicmd_time.h>
#include <appl/sdkcli/bcma_clicmd_gpio.h>
#include <appl/sdkcli/bcma_clicmd.h>

/* CLI switch command set */
static bcma_cli_command_t cmd_tick = {
    .name = "Tick",
    .func = bcma_clicmd_tick,
    .desc = BCMA_CLICMD_TICK_DESC
};

static bcma_cli_command_t cmd_counter = {
    .name = "CounTeR",
    .func = bcma_clicmd_counter,
    .desc = BCMA_CLICMD_COUNTER_DESC
};

#ifdef CFG_SWITCH_SYNCE_INCLUDED
static bcma_cli_command_t cmd_getsynce = {
    .name = "GetSyncE",
    .func = bcma_clicmd_getsynce,
    .desc = BCMA_CLICMD_GETSYNCE_DESC,
    .synop = BCMA_CLICMD_GETSYNCE_SYNOP,
    .help = { BCMA_CLICMD_GETSYNCE_HELP },
    .examples = BCMA_CLICMD_GETSYNCE_EXAMPLES,
};

static bcma_cli_command_t cmd_setsynce = {
    .name = "SetSyncE",
    .func = bcma_clicmd_setsynce,
    .desc = BCMA_CLICMD_SETSYNCE_DESC,
    .synop = BCMA_CLICMD_SETSYNCE_SYNOP,
    .help = { BCMA_CLICMD_SETSYNCE_HELP },
    .examples = BCMA_CLICMD_SETSYNCE_EXAMPLES,
};
#endif /* CFG_SWITCH_SYNCE_INCLUDED */

int
bcma_clicmd_add_switch_cmds(bcma_cli_t *cli)
{
    bcma_cli_add_command(cli, &cmd_tick, 0);
    bcma_cli_add_command(cli, &cmd_counter, 0);
#ifdef CFG_SWITCH_SYNCE_INCLUDED
    bcma_cli_add_command(cli, &cmd_getsynce, 0);
    bcma_cli_add_command(cli, &cmd_setsynce, 0);
#endif /* CFG_SWITCH_SYNCE_INCLUDED */
#ifdef CFG_SDKCLI_GPIO_INCLUDED
    bcma_clicmd_add_gpio_cmd(cli);
#endif

    return 0;
}
#endif /* CFG_SDKCLI_INCLUDED */
