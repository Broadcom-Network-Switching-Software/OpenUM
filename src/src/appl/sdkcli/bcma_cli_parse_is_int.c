/*! \file bcma_cli_parse_is_int.c
 *
 * This module parses a string into an interger.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#ifdef CFG_SDKCLI_INCLUDED
#include "cli_porting.h"
#include <utils/shr/shr_util.h>

#include <appl/sdkcli/bcma_cli_parse.h>

/*
 * Check if string contains a valid integer
 */
int
bcma_cli_parse_is_int(const char *str)
{
    int base, val;

    /* Invalid or empty string */
    if (str == NULL || *str == 0) {
        return FALSE;
    }

    /* Negative value is permitted */
    if (*str == '-') {
        str++;
    }

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

    do {
        val = shr_util_xch2int(*str);
        if (val >= base || val < 0) {
            return FALSE;
        }
    } while (*++str);

    return TRUE;
}
#endif /* CFG_SDKCLI_INCLUDED */
