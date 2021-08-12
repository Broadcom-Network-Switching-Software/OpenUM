/*! \file bcma_clicmd_add_switch_cmds.c
 *
 * CLI switch commands support.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED

#include <appl/sdkcli/bcma_cli.h>

#include <appl/sdkcli/bcma_clicmd_tick.h>
#include <appl/sdkcli/bcma_clicmd_counter.h>
#include <appl/sdkcli/bcma_clicmd_portmap.h>
#include <appl/sdkcli/bcma_clicmd_phy.h>
#include <appl/sdkcli/bcma_clicmd_time.h>
#include <appl/sdkcli/bcma_clicmd_gpio.h>
#include <appl/sdkcli/bcma_clicmd_led.h>
#include <appl/sdkcli/bcma_clicmd_port.h>
#include <appl/sdkcli/bcma_clicmd_setget.h>
#include <appl/sdkcli/bcma_clicmd_bs.h>
#include <appl/sdkcli/bcma_clicmd_temp.h>
#include <appl/sdkcli/bcma_clicmd_porttest.h>
#include <appl/sdkcli/bcma_clicmd.h>

/* CLI switch command set */
static bcma_cli_command_t cmd_tick = {
    .name = "Tick",
    .func = bcma_clicmd_tick,
    .desc = BCMA_CLICMD_TICK_DESC
};

#ifdef CFG_SWITCH_STAT_INCLUDED
static bcma_cli_command_t cmd_counter = {
    .name = "CounTeR",
    .func = bcma_clicmd_counter,
    .desc = BCMA_CLICMD_COUNTER_DESC,
    .synop = BCMA_CLICMD_COUNTER_SYNOP,
};
#endif /* CFG_SWITCH_STAT_INCLUDED */

static bcma_cli_command_t cmd_portmap = {
    .name = "PortMap",
    .func = bcma_clicmd_portmap,
    .desc = BCMA_CLICMD_PORTMAP_DESC
};

#ifdef CFG_BROADSYNC_INCLUDED
static bcma_cli_command_t cmd_bs = {
    .name = "BroadSync",
    .func = bcma_clicmd_bs,
    .desc = BCMA_CLICMD_BS_DESC,
    .synop = BCMA_CLICMD_BS_SYNOP,
    .help = { BCMA_CLICMD_BS_HELP },
    .examples = BCMA_CLICMD_BS_EXAMPLES
};
#endif

#ifdef CFG_TEMP_MONITOR_INCLUDED
static bcma_cli_command_t cmd_temp = {
    .name = "TempQuery",
    .func = bcma_clicmd_temp,
    .desc = BCMA_CLICMD_TEMP_DESC
};
#endif /* CFG_TEMP_MONITOR_INCLUDED */

#ifdef CFG_SWITCH_SYNCE_INCLUDED
static bcma_cli_command_t cmd_synce = {
    .name = "SyncE",
    .func = bcma_clicmd_synce,
    .desc = BCMA_CLICMD_SYNCE_DESC,
    .synop = BCMA_CLICMD_SYNCE_SYNOP,
    .help = { BCMA_CLICMD_SYNCE_HELP },
    .examples = BCMA_CLICMD_SYNCE_EXAMPLES,
};
#endif /* CFG_SWITCH_SYNCE_INCLUDED */

#ifdef CFG_SWITCH_TIMESYNC_INCLUDED
static bcma_cli_command_t cmd_timesync = {
    .name = "TimeSync",
    .func = bcma_clicmd_timesync,
    .desc = BCMA_CLICMD_TIMESYNC_DESC,
    .synop = BCMA_CLICMD_TIMESYNC_SYNOP,
    .help = { BCMA_CLICMD_TIMESYNC_HELP },
    .examples = BCMA_CLICMD_TIMESYNC_EXAMPLES
};
static bcma_cli_command_t cmd_timesync_test = {
    .name = "tstest",
    .func = bcma_clicmd_timesync_test,
    .desc = BCMA_CLICMD_TIMESYNCTEST_DESC,
    .synop = BCMA_CLICMD_TIMESYNCTEST_SYNOP,
    .help = { BCMA_CLICMD_TIMESYNCTEST_HELP },
    .examples = BCMA_CLICMD_TIMESYNCTEST_EXAMPLES
};
#endif /* CFG_SWITCH_TIMESYNC_INCLUDED */

#ifdef CFG_SDKCLI_PORT_INCLUDED
static bcma_cli_command_t cmd_portstatus = {
    .name = "PortStatus",
    .func = bcma_clicmd_portstatus,
    .desc = BCMA_CLICMD_PORTSTATUS_DESC,
    .synop = BCMA_CLICMD_PORTSTATUS_SYNOP,
    .help = { BCMA_CLICMD_PORTSTATUS_HELP },
    .examples = BCMA_CLICMD_PORTSTATUS_EXAMPLES
};

static bcma_cli_command_t cmd_portreinit = {
    .name = "PortReinit",
    .func = bcma_clicmd_portreinit,
    .desc = BCMA_CLICMD_PORTREINIT_DESC,
    .synop = BCMA_CLICMD_PORTREINIT_SYNOP,
    .help = { BCMA_CLICMD_PORTREINIT_HELP },
    .examples = BCMA_CLICMD_PORTREINIT_EXAMPLES
};

static bcma_cli_command_t cmd_portclass = {
    .name = "PortClass",
    .func = bcma_clicmd_portclass,
    .desc = BCMA_CLICMD_PORTCLASS_DESC,
    .synop = BCMA_CLICMD_PORTCLASS_SYNOP,
    .help = { BCMA_CLICMD_PORTCLASS_HELP },
    .examples = BCMA_CLICMD_PORTCLASS_EXAMPLES
};

static bcma_cli_command_t cmd_port = {
    .name = "Port",
    .func = bcma_clicmd_port,
    .desc = BCMA_CLICMD_PORT_DESC,
    .synop = BCMA_CLICMD_PORT_SYNOP,
    .help = { BCMA_CLICMD_PORT_HELP },
    .examples = BCMA_CLICMD_PORT_EXAMPLES,
};
#endif /* CFG_SDKCLI_PORT_INCLUDED */

#ifdef CFG_SDKCLI_PHY_INCLUDED
static bcma_cli_command_t cmd_phy = {
    .name = "phy",
    .func = bcma_clicmd_phy,
    .desc = BCMA_CLICMD_PHY_DESC,
    .synop = BCMA_CLICMD_PHY_SYNOP,
    .help = { BCMA_CLICMD_PHY_HELP },
    .examples = BCMA_CLICMD_PHY_EXAMPLES
};

static bcma_cli_command_t cmd_porttest = {
    .name = "porttest",
    .func = bcma_clicmd_porttest,
    .desc = BCMA_CLICMD_PORTTEST_DESC,
    .synop = BCMA_CLICMD_PORTTEST_SYNOP,
    .help = { BCMA_CLICMD_PORTTEST_HELP },
    .examples = BCMA_CLICMD_PORTTEST_EXAMPLES
};
#endif /* CFG_SDKCLI_PHY_INCLUDED */

static bcma_cli_command_t cmd_get = {
    .name = "Get",
    .func = bcma_clicmd_get,
    .desc = BCMA_CLICMD_GET_DESC,
    .synop = BCMA_CLICMD_GET_SYNOP,
    .help = {BCMA_CLICMD_GET_HELP},
    .examples = BCMA_CLICMD_GET_EXAMPLES
};

static bcma_cli_command_t cmd_set = {
    .name = "Set",
    .func = bcma_clicmd_set,
    .desc = BCMA_CLICMD_SET_DESC,
    .synop = BCMA_CLICMD_SET_SYNOP,
    .help = {BCMA_CLICMD_SET_HELP},
    .examples = BCMA_CLICMD_SET_EXAMPLES
};

int
bcma_clicmd_add_switch_cmds(bcma_cli_t *cli)
{
    bcma_cli_add_command(cli, &cmd_tick, 0);
#ifdef CFG_SWITCH_STAT_INCLUDED
    bcma_cli_add_command(cli, &cmd_counter, 0);
#endif /* CFG_SWITCH_STAT_INCLUDED */
    bcma_cli_add_command(cli, &cmd_portmap, 0);
#ifdef CFG_BROADSYNC_INCLUDED
    bcma_cli_add_command(cli, &cmd_bs, 0);
#endif /* CFG_BROADSYNC_INCLUDED */
#ifdef CFG_SDKCLI_PHY_INCLUDED
    bcma_cli_add_command(cli, &cmd_phy, 0);
    bcma_cli_add_command(cli, &cmd_porttest, 0);
#endif /* CFG_SDKCLI_PHY_INCLUDED */
#ifdef CFG_SWITCH_SYNCE_INCLUDED
    bcma_cli_add_command(cli, &cmd_synce, 0);
#endif /* CFG_SWITCH_SYNCE_INCLUDED */
#ifdef CFG_SWITCH_TIMESYNC_INCLUDED
    bcma_cli_add_command(cli, &cmd_timesync, 0);
    bcma_cli_add_command(cli, &cmd_timesync_test, 0);
#endif /* CFG_SWITCH_TIMESYNC_INCLUDED */
#ifdef CFG_SDKCLI_GPIO_INCLUDED
    bcma_clicmd_add_gpio_cmd(cli);
#endif
#ifdef CFG_SDKCLI_LED_INCLUDED
    bcma_clicmd_add_led_cmd(cli);
#endif
#ifdef CFG_SDKCLI_PORT_INCLUDED
    bcma_cli_add_command(cli, &cmd_portstatus, 0);
    bcma_cli_add_command(cli, &cmd_portreinit, 0);
        bcma_cli_add_command(cli, &cmd_portclass, 0);
    bcma_cli_add_command(cli, &cmd_port, 0);
#endif
#ifdef CFG_TEMP_MONITOR_INCLUDED
    bcma_cli_add_command(cli, &cmd_temp, 0);
#endif
    bcma_cli_add_command(cli, &cmd_get, 0);
    bcma_cli_add_command(cli, &cmd_set, 0);

    return 0;
}
#endif /* CFG_SDKCLI_INCLUDED */
