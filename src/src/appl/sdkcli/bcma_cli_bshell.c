/*! \file bcma_cli_bshell.c
 *
 * The bshell API provides a way to execute CLI commands programmatically.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#include "cli_porting.h"

#include <appl/sdkcli/bcma_cli_bshell.h>

/*
 * A system may have multiple CLI instances, but only one can be
 * associated with the bshell feature at a time.
 */
static bcma_cli_t *bshell_cli;

void
bcma_cli_bshell_init(bcma_cli_t *cli)
{
    bshell_cli = cli;
}

int
bcma_cli_bshell(int unit, char *cmd)
{
    static bool bshell_active;
    int cur_unit;
    int rv;

    if (bshell_cli == NULL) {
        return BCMA_CLI_CMD_FAIL;
    }

    if (bshell_active) {
        cli_out("%sbshell cannot be called recursively\n",
                BCMA_CLI_CONFIG_ERROR_STR);
        return BCMA_CLI_CMD_FAIL;
    }
    bshell_active = true;

    /* Save current unit of the CLI */
    cur_unit = bshell_cli->cur_unit;
    bshell_cli->cur_unit = unit;

    rv = bcma_cli_cmd_process(bshell_cli, cmd);

    /* Restore current unit of the CLI */
    bshell_cli->cur_unit = cur_unit;

    bshell_active = false;

    return rv;
}
#endif /* CFG_SDKCLI_INCLUDED */
