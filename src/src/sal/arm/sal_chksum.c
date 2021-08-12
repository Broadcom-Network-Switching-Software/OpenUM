/*
 * $Id: sal_chksum.c,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

uint16
sal_checksum(uint16 sum, const void *data_p, uint16 len) REENTRANT
{
    uint16 t;
    const uint8 *dataptr;
    const uint8 *last_byte;

    dataptr = (const uint8 *)data_p;
    last_byte = (const uint8 *)data_p + len - 1;

    while(dataptr < last_byte) {    /* At least two more bytes */
        t = (dataptr[0] << 8) + dataptr[1];
        sum += t;
        if(sum < t) {
            sum++;      /* carry */
        }
        dataptr += 2;
    }

    if(dataptr == last_byte) {
        t = (dataptr[0] << 8) + 0;
        sum += t;
        if(sum < t) {
            sum++;      /* carry */
        }
    }

    /* Return sum in host byte order. */
    return sum;
}
