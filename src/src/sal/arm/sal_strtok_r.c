/*! \file sal_strtok_r.c
 *
 * Split string into tokens
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

char *
sal_strtok_r(char *s1, const char *delim, char **s2)
{
    char *ret_str;
    char *p;
    int len = 0;

    if (s1 == NULL) {
        s1 = *s2;
        if (s1 == NULL) {
            return NULL;
        }
    }

    p = s1;
    /* skip initial delimiters */
    while ((*p) && (sal_strchr(delim, *p++) != NULL)) {
       len++;
    }

    s1 += len;

    if (*s1 == '\0') {
        return NULL;
    }

    ret_str = s1;

    while (*s1) {
       if (sal_strchr(delim, *s1) != NULL) {
           break;
       }
       s1++;
    }

    if (*s1) {
        *s1++ = '\0';
    }

    if (s2 != NULL) {
        *s2 = s1;
    }

    return ret_str;
}
