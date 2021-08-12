/*
 * $Id: pbmp.c,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"


#ifdef _SHR_DEFINE_PBMP_FUNCTIONS

/* returns 1 is the bitmap is empty */
int
_shr_pbmp_bmnull(_shr_pbmp_t *bmp)
{
    int i;

    for (i = 0; i < _SHR_PBMP_WORD_MAX; i++) {
         if (_SHR_PBMP_WORD_GET(*bmp, i) != 0) {
             return 0;
         }
    }
    return 1;
}

/* returns 1 is the two bitmaps are equal */
int
_shr_pbmp_bmeq(_shr_pbmp_t *bmp1, _shr_pbmp_t *bmp2)
{
    int i;

    for (i = 0; i < _SHR_PBMP_WORD_MAX; i++) {
        if (_SHR_PBMP_WORD_GET(*bmp1, i) != _SHR_PBMP_WORD_GET(*bmp2, i)) {
            return 0;
        }
    }
    return 1;
}

#endif /* _SHR_DEFINE_PBMP_FUNCTIONS */

#ifndef CFG_PCM_SUPPORT_INCLUDED
/*
 * _shr_format_integer
 *
 *   Format an integer as a string of ASCII digits.
 *   Used for debugging printf's in the driver.
 */

void
_shr_format_integer(char *buf, unsigned int n, int min_digits, int base)
{
    static char   *digit_char = "0123456789abcdef";
    unsigned int  tmp;
    int     digit, needed_digits = 0;

    for (tmp = n, needed_digits = 0; tmp; needed_digits++) {
         tmp /= base;
    }

    if (needed_digits > min_digits)
        min_digits = needed_digits;
    buf[min_digits] = 0;

    for (digit = min_digits - 1; digit >= 0; digit--) {
         buf[digit] = digit_char[n % base];
         n /= base;
    }    
}
#else 
extern void _shr_format_integer(char *buf, unsigned int n, int min_digits, int base);
#endif /* CFG_PCM_SUPPORT_INCLUDED */
/* format a bitmap into a static buffer suitable for printing */
char *
_shr_pbmp_format(_shr_pbmp_t bmp, char *buf)
{
    int   i;
    char  *bp;

    if (buf == NULL) {
        return buf;
    }
    buf[0] = '0';
    buf[1] = 'x';
    bp = &buf[2];
    for (i = _SHR_PBMP_WORD_MAX-1; i >= 0; i--) {
         _shr_format_integer(bp, _SHR_PBMP_WORD_GET(bmp, i), 8, 16);
         bp += 8;
    }
    return buf;
}

/*
 * decode a string in hex format into a bitmap
 * returns 0 on success, -1 on error
 */
int
_shr_pbmp_decode(char *s, _shr_pbmp_t *bmp)
{
    char  *e;
    uint32  v;
    int   p;

    _SHR_PBMP_CLEAR(*bmp);

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        /* get end of string */
        s += 2;
        for (e = s; *e; e++)
            ;
        e -= 1;
        /* back up to beginning of string, setting ports as we go */
        p = 0;
        while (e >= s) {
               if (*e >= '0' && *e <= '9') {
                   v = *e - '0';
               } else if (*e >= 'a' && *e <= 'f') {
                   v = *e - 'a' + 10;
               } else if (*e >= 'A' && *e <= 'F') {
                   v = *e - 'A' + 10;
               } else {
                   return -1;    /* error: invalid hex digits */
               }
               e -= 1;
               /* now set a nibble's worth of ports */
               if ((v & 1) && p < _SHR_PBMP_PORT_MAX) {
                   _SHR_PBMP_PORT_ADD(*bmp, p);
               }
               p += 1;
               if ((v & 2) && p < _SHR_PBMP_PORT_MAX) {
                   _SHR_PBMP_PORT_ADD(*bmp, p);
               }
               p += 1;
               if ((v & 4) && p < _SHR_PBMP_PORT_MAX) {
                   _SHR_PBMP_PORT_ADD(*bmp, p);
               }
               p += 1;
               if ((v & 8) && p < _SHR_PBMP_PORT_MAX) {
                  _SHR_PBMP_PORT_ADD(*bmp, p);
               }
               p += 1;
        }
    } else {
        v = 0;
        while (*s >= '0' && *s <= '9') {
              v = v * 10 + (*s++ - '0');
        }
        if (*s != '\0') {
            return -1;      /* error: invalid decimal digits */
        }
        p = 0;
        while (v) {
            if ((v & 1) && p < _SHR_PBMP_PORT_MAX) {
                _SHR_PBMP_PORT_ADD(*bmp, p);
            }
            v >>= 1;
            p += 1;
        }
    }
    return 0;
}

