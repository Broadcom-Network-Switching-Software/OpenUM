/*! \file bcma_clicmd_bhelp.c
 *
 * CLI '?' command.
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

#include <appl/sdkcli/bcma_clicmd_bhelp.h>

int
bcma_clicmd_bhelp(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    if (bcma_cli_show_cmds_avail(cli, NULL) != 0) {
        return BCMA_CLI_CMD_FAIL;
    }
    return BCMA_CLI_CMD_OK;
}
#endif /* CFG_SDKCLI_INCLUDED */
