/*! \file bcma_clicmd_add_basic_cmds.c
 *
 * CLI basic commands support.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#include "cli_porting.h"
#include <appl/sdkcli/bcma_cli.h>

#include <appl/sdkcli/bcma_clicmd_quit.h>
#include <appl/sdkcli/bcma_clicmd_help.h>
#include <appl/sdkcli/bcma_clicmd_bhelp.h>
#include <appl/sdkcli/bcma_clicmd_debugcmd.h>
#include <appl/sdkcli/bcma_clicmd.h>

#ifdef __LINUX__
/* CLI default command set */
static bcma_cli_command_t cmd_quit = {
    .name = "Quit",
    .func = bcma_clicmd_quit,
    .desc = BCMA_CLICMD_QUIT_DESC
};

static bcma_cli_command_t cmd_exit = {
    .name = "Exit",
    .func = bcma_clicmd_quit,
    .desc = BCMA_CLICMD_QUIT_DESC
};
#endif /* __LINUX__ */

static bcma_cli_command_t cmd_help = {
    .name = "Help",
    .func = bcma_clicmd_help,
    .desc = BCMA_CLICMD_HELP_DESC,
    .synop = BCMA_CLICMD_HELP_SYNOP
};

static bcma_cli_command_t cmd_bhelp = {
    .name = "?",
    .func = bcma_clicmd_bhelp,
    .desc = BCMA_CLICMD_BHELP_DESC
};

#if CFG_CLI_ENABLED
static bcma_cli_command_t cmd_debugcmd = {
    .name = "DebugCmd",
    .func = bcma_clicmd_debugcmd,
    .desc = BCMA_CLICMD_DEBUGCMD_DESC
};
#endif /* CFG_CLI_ENABLED */

int
bcma_clicmd_add_basic_cmds(bcma_cli_t *cli)
{
#ifdef __LINUX__
    bcma_cli_add_command(cli, &cmd_quit, 0);
    bcma_cli_add_command(cli, &cmd_exit, 0);
#endif /* __LINUX__ */
    bcma_cli_add_command(cli, &cmd_help, 0);
    bcma_cli_add_command(cli, &cmd_bhelp, 0);
#if CFG_CLI_ENABLED
    bcma_cli_add_command(cli, &cmd_debugcmd, 0);
#endif /* CFG_CLI_ENABLED */

    return 0;
}
#endif /* CFG_SDKCLI_INCLUDED */
