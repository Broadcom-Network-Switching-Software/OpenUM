/*
 * $Id: util_symbols.h,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 *
 * Chip symbol table definitions.
 */

#ifndef __UTIL_SYMBOLS_ITER_H__
#define __UTIL_SYMBOLS_ITER_H__

/*! Matching modes */
#define UTIL_SYMBOLS_ITER_MODE_EXACT     0
#define UTIL_SYMBOLS_ITER_MODE_START     1
#define UTIL_SYMBOLS_ITER_MODE_STRSTR    2

typedef struct util_symbols_iter_s {

    const char* name;   /* String to match */

    int matching_mode;  /* Defines valid matches */

    uint32_t pflags;    /* Flags that must be present */
    uint32_t aflags;    /* Flags that must be absent */

    const symbols_t *symbols;

    int (*function)(const symbol_t *sym, void *vptr);
    void* vptr;

} util_symbols_iter_t;

extern int util_symbols_iter(util_symbols_iter_t *iter);
#endif /* __UTIL_SYMBOLS_ITER_H__ */
