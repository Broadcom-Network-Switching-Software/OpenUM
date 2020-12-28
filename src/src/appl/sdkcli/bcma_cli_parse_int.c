/*! \file bcma_cli_parse_int.c
 *
 * This module parses a string into an interger.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#include "cli_porting.h"

#include <appl/sdkcli/bcma_cli_parse.h>

int
bcma_cli_parse_int(const char *str, int *val)
{
    if (!bcma_cli_parse_is_int(str)) {
        return -1;
    }
    if (val) {
        *val = sal_ctoi(str, NULL);
    }
    return 0;
}
#endif /* CFG_SDKCLI_INCLUDED */
