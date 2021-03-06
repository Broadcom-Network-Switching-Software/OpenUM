/*! \file bcma_cli_prompt.c
 *
 * Customize the prompt prefix of CLI command.
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

int
bcma_cli_prompt_prefix_cb_set(struct bcma_cli_s *cli,
                              bcma_cli_prompt_prefix_f func)
{
    cli->prefix_cb = func;

    return 0;
}

const char *
bcma_cli_prompt_prefix(struct bcma_cli_s *cli)
{
    const char *prefix = NULL;

    if (cli->prefix_cb) {
        prefix = cli->prefix_cb(cli);
    }

    return prefix ? prefix : "";
}

#endif /* CFG_SDKCLI_INCLUDED */
