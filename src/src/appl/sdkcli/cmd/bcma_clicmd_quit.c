/*! \file bcma_clicmd_quit.c
 *
 * CLI 'quit' command.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#ifdef __LINUX__
#include "cli_porting.h"

#include <appl/sdkcli/bcma_cli.h>

#include <appl/sdkcli/bcma_clicmd_quit.h>

int
bcma_clicmd_quit(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    COMPILER_REFERENCE(cli);
    COMPILER_REFERENCE(args);

    sal_reset(0);
    return BCMA_CLI_CMD_EXIT;
}
#endif /* __LINUX__ */
#endif /* CFG_SDKCLI_INCLUDED */
