/*! \file shr_util.c
 *
 * Common utility routines.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "cli_porting.h"
#include <utils/shr/shr_util.h>

int
shr_util_ffs(uint32_t n)
{
    int i16, i8, i4, i2, i1, i0;

    i16 = !(n & 0xffff) << 4;
    n >>= i16;
    i8 = !(n & 0xff) << 3;
    n >>= i8;
    i4 = !(n & 0xf) << 2;
    n >>= i4;
    i2 = !(n & 0x3) << 1;
    n >>= i2;
    i1 = !(n & 0x1);
    i0 = (n >> i1) & 1 ? 0 : -32;

    return i16 + i8 + i4 + i2 + i1 + i0;
}

int
shr_util_xch2int(int ch)
{
    if (ch >= '0' && ch <= '9') {
        return (ch - '0'     );
    }
    if (ch >= 'a' && ch <= 'f') {
        return (ch - 'a' + 10);
    }
    if (ch >= 'A' && ch <= 'F') {
        return (ch - 'A' + 10);
    }
    return -1;
}
