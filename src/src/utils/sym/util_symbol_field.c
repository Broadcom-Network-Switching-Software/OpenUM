/*! \file util_symbol_field.c
 *
 * The related APIs to handle the field of the symbol
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_CHIP_SYMBOLS_FIELD_INCLUDED

/* get info or size in this field */
static void
field_info_get(uint32_t *fp, symbol_field_info_t *finfo, int *fd_sz,
               const char **fnames_arr)
{
    SAL_ASSERT(fp != NULL);

    if (finfo) {
        sal_memset(finfo, 0, sizeof(*finfo));
    }

    if (SYMBOL_FIELD_EXT(*fp)) {
        /* Double Word */
        if (finfo) {
            finfo->fid = SYMBOL_FIELD_EXT_ID_GET(*fp);
            finfo->minbit = SYMBOL_FIELD_EXT_MIN_GET(*fp);
            finfo->maxbit = SYMBOL_FIELD_EXT_MAX_GET(*fp);
            finfo->name = fnames_arr? fnames_arr[finfo->fid]: NULL;
        }
        if (fd_sz) {
            *fd_sz = 2;
        }
    } else {
        /* Single Word */
        if (finfo) {
            finfo->fid = SYMBOL_FIELD_ID_GET(*fp);
            finfo->minbit = SYMBOL_FIELD_MIN_GET(*fp);
            finfo->maxbit = SYMBOL_FIELD_MAX_GET(*fp);
            finfo->name = fnames_arr? fnames_arr[finfo->fid]: NULL;
        }
        if (fd_sz) {
            *fd_sz = 1;
        }
    }
}

/* Operate in SYMBOL_FIELDS_ITER_BEGIN iteration */
uint32_t* symbol_field_info_decode(uint32_t* fp, symbol_field_info_t* finfo,
                                   const char** fnames_arr)
{
    int fd_sz = 1;
    SAL_ASSERT(fp != NULL);

    field_info_get(fp, finfo, &fd_sz, fnames_arr);

    /* Return pointer to next field descriptor */
    if (SYMBOL_FIELD_LAST(*fp)) {
        return NULL;
    } else {
        return fp + fd_sz;
    }
}

int
symbol_field_value_get(const uint32_t *buf, const symbol_field_info_t *finfo,
                       uint32_t *fbuf, uint32_t buf_wsize)
{
    int i, wp, bp;
    int sbit, ebit, len;
    uint32_t mask;

    if (!buf || !fbuf || !finfo) {
        return -1;
    }

    sbit = finfo->minbit;
    ebit = finfo->maxbit;
    len = ebit - sbit + 1;
    if (len <= 0  || len > buf_wsize * 32) {
        return -1;
    }

    sal_memset(fbuf, 0, sizeof(uint32_t) * buf_wsize);
    wp = sbit / 32;
    bp = sbit % 32;
    i = 0;

    for (; len > 0; len -= 32, i++) {
        if (wp >= buf_wsize) {
            return -1;
        }
        if (len < 32) {
            mask = (1 << len) - 1;
        } else {
            mask = ~0;
        }
        if (bp) {
            fbuf[i] = (buf[wp++] >> bp);
            if (len > (32 - bp)) {
                fbuf[i] |= buf[wp] << (32 - bp);
            }
        } else {
            fbuf[i] = buf[wp++];
        }
        fbuf[i] &= mask;
    }

    return 0;
}

int
symbol_field_value_set(uint32_t *buf, const symbol_field_info_t *finfo,
                       const uint32_t *fbuf, uint32_t buf_wsize)
{
    int i, wp, bp;
    int sbit, ebit, len;
    uint32_t mask;

    if (!buf || !fbuf || !finfo) {
        return -1;
    }

    sbit = finfo->minbit;
    ebit = finfo->maxbit;
    len = ebit - sbit + 1;
    if (len <= 0  || len > buf_wsize * 32) {
        return -1;
    }

    wp = sbit / 32;
    bp = sbit % 32;
    i = 0;

    for (; len > 0; len -= 32, i++) {
        if (wp >= buf_wsize) {
            return -1;
        }
        if (len < 32) {
            mask = (1 << len) - 1;
        } else {
            mask = ~0;
        }
        if (bp) {
            buf[wp] &= ~(mask << bp);
            buf[wp++] |= (fbuf[i] & mask) << bp;
            if (len > (32 - bp)) {
                buf[wp] &= ~(mask >> (32 - bp));
                buf[wp] |= (fbuf[i] & mask) >> (32 - bp);
            }
        } else {
            buf[wp] &= ~mask;
            buf[wp++] |= (fbuf[i] & mask);
        }
    }

    return 0;
}

int
symbol_field_value_get32(const uint32_t *buf, const symbol_field_info_t *finfo,
                         uint32_t *ret_val)
{
    return symbol_field_value_get(buf, finfo, ret_val, 1);
}
#endif /* CFG_CHIP_SYMBOLS_FIELD_INCLUDED */
