/*! \file bcma_clicmd_add_coe_cmds.c
 *
 * CLI COE commands support.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED

#include <appl/sdkcli/bcma_cli.h>

#include <appl/sdkcli/bcma_clicmd_coe.h>
#include <appl/sdkcli/bcma_clicmd.h>

#ifdef CFG_COE_INCLUDED
static bcma_cli_command_t cmd_coe = {
    .name = "coe",
    .func = bcma_clicmd_coe,
    .desc = BCMA_BCMCOECMD_DESC,
    .synop = BCMA_BCMCOECMD_SYNOP,
    .help = { BCMA_BCMCOECMD_HELP },
    .examples = BCMA_BCMCOECMD_EXAMPLES
};
#endif /* CFG_COE_INCLUDED */

int
bcma_clicmd_add_coe_cmds(bcma_cli_t *cli)
{
#ifdef CFG_SDKCLI_COE_INCLUDED
    bcma_cli_add_command(cli, &cmd_coe, 0);
#endif /* CFG_SDKCLI_COE_INCLUDED */
    return 0;
}
#endif /* CFG_SDKCLI_INCLUDED */
