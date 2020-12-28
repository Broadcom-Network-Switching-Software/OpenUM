/*! \file bcma_cli_path.c
 *
 *  Functions for retrieving and parsing the CLI 'path' variable.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#include "cli_porting.h"

#include <appl/sdkcli/bcma_cli_var.h>
#include <appl/sdkcli/bcma_cli_path.h>

char *
bcma_cli_path_tok_get(bcma_cli_t *cli, char **s, char **s2)
{
    const char *path;

    path = bcma_cli_var_get(cli, BCMA_CLI_VAR_PATH);
    if (path == NULL) {
        path = ".";
    }

    if (s != NULL) {
        *s = sal_strdup(path);
        if (*s == NULL) {
            return NULL;
        }
    }

    /* Each path is separated by space or colon in $BCMA_CLI_VAR_PATH  */
    return sal_strtok_r((s != NULL) ? *s : NULL, " :", s2);
}
#endif /* CFG_SDKCLI_INCLUDED */
