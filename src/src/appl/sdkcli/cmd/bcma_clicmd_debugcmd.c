/*! \file bcma_clicmd_debugcmd.c
 *
 * CLI 'debugcmd' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_SDKCLI_INCLUDED
#if CFG_CLI_ENABLED
#include "appl/cli.h"
#include <appl/sdkcli/bcma_cli.h>

int
bcma_clicmd_debugcmd(bcma_cli_t *cli_s, bcma_cli_args_t *args)
{
    COMPILER_REFERENCE(cli_s);
    COMPILER_REFERENCE(args);

    cli();

    return BCMA_CLI_CMD_OK;
}
#endif /* CFG_CLI_ENABLED */
#endif /* CFG_SDKCLI_INCLUDED */
