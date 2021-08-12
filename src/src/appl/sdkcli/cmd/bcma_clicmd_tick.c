/*! \file bcma_clicmd_tick.c
 *
 * CLI 'tick' command.
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
bcma_clicmd_tick(bcma_cli_t *cli, bcma_cli_args_t *args)
{
    COMPILER_REFERENCE(cli);
    COMPILER_REFERENCE(args);

    sal_printf("System ticks: %lu / 0x%lX\n",
               (uint32)sal_get_ticks(), (uint32)sal_get_ticks());
    sal_printf("  - Every tick is %lu micro seconds\n",
                sal_get_us_per_tick());

    return BCMA_CLI_CMD_OK;
}
#endif /* CFG_SDKCLI_INCLUDED */
