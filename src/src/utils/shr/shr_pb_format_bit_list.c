/*! \file shr_pb_format_bit_list.c
 *
 * Format a bit list as a set of ranges into print buffer.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "cli_porting.h"
#include <utils/shr/shr_pb_format.h>

const char *
shr_pb_format_bit_list(shr_pb_t *pb, const SHR_BITDCL *bits, int count)
{
    int idx, start, end;
    char *sep = "";

    /* Mark range as inactive */
    start = end = count;

    SHR_BIT_ITER(bits, count, idx) {
        if (idx != (end + 1)) {
            /* If the range is interrupted, terminate it */
            if (end > start) {
                shr_pb_printf(pb, "-%d", end);
            }
            /* Mark range as inactive */
            start = end = count;
        }
        if (start == count) {
            /* If no active range, start new */
            shr_pb_printf(pb, "%s%d", sep, idx);
            start = idx;
            sep = ",";
        }
        end = idx;
    }
    /* If active range, terminate it */
    if (end > start) {
        shr_pb_printf(pb, "-%d", end);
    }

    return shr_pb_str(pb);
}
