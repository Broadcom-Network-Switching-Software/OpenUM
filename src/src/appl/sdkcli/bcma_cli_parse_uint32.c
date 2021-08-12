/*! \file bcma_cli_parse_uint32.c
 *
 * This module parses a string into an unsigned interger.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#include "cli_porting.h"

#include <appl/sdkcli/bcma_cli_parse.h>

int
bcma_cli_parse_uint32(const char *str, uint32_t *val)
{
    int base = 0;

    if (!bcma_cli_parse_is_int(str)) {
        return -1;
    }
    if (val) {
        if (*str == '0') {
            if (str[1] == 'b' || str[1] == 'B') {
                base = 2;
                str += 2;
            } else if (str[1] == 'x' || str[1] == 'X') {
                base = 16;
                str += 2;
            } else
                base = 8;
        } else {
            base = 10;
        }
        *val = (uint32_t)sal_strtoul(str, NULL, base);
    }
    return 0;
}
#endif /* CFG_SDKCLI_INCLUDED */
