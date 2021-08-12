/*! \file util_symbols_iter.c
 *
 * The related APIs to traverse the symbol table
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_CHIP_SYMBOLS_INCLUDED
#include <utils/sym/util_symbols_iter.h>
#include <appl/sdkcli/bcma_cli_token.h>

static int
symbols_get(const symbols_t *symbols, uint32_t sindex, symbol_t *rsym)
{
    if (symbols) {
        if (sindex < symbols->size) {
            /* Index is within the symbol table */
            *rsym = symbols->symbols[sindex];
            return 0;
        }
    }
    return -1;
}

static bool
sym_flag_info_match(util_symbols_iter_t *iter, const symbol_t *s)
{
    /* Check flags which must be present */
    if (iter->pflags && ((s->flags & iter->pflags) != iter->pflags)) {
        /* Symbol does not match */
        return false;
    }

    /* Check flags which must be absent */
    if (iter->aflags && ((s->flags & iter->aflags) != 0)) {
        /* Symbol does not match */
        return false;
    }

    return true;
}

static int
sym_match(util_symbols_iter_t *iter, const char *sym_name)
{
    switch(iter->matching_mode) {
        case UTIL_SYMBOLS_ITER_MODE_EXACT:
            if (sal_strcasecmp(sym_name, iter->name) == 0) {
                /* Name matches */
                return 1;
            }
            break;

        case UTIL_SYMBOLS_ITER_MODE_START:
            if (sal_strncasecmp(sym_name, iter->name,
                                sal_strlen(iter->name)) == 0) {
                /* Name matches */
                return 1;
            }
            break;

        case UTIL_SYMBOLS_ITER_MODE_STRSTR:
            if (sal_strstr(sym_name, iter->name) != NULL) {
                /* Name matches */
                return 1;
            }
            break;

        default:
            break;
    }
    return 0;
}

int
util_symbols_iter(util_symbols_iter_t *iter)
{
    int count = 0;
    int rc;
    int match;
    uint32_t idx;
    symbol_t s;
    bool is_wildcard;
    bcma_cli_tokens_t tokens;

    if (iter == NULL || iter->symbols == NULL) {
        return 0;
    }

    is_wildcard = (sal_strstr(iter->name, "*") != NULL)? true: false;
    if (is_wildcard == true &&
        bcma_cli_tokenize(iter->name, &tokens, "*") < 0) {
        return -1;
    }

    for (idx = 0; symbols_get(iter->symbols, idx, &s) >= 0; idx++) {

        if (s.name == 0) {
            /* Last */
            continue;
        }

        if (!sym_flag_info_match(iter, &s)) {
            continue;
        }

        /* Check the name */
        match = 0;
        if (!is_wildcard) {
            /* Not wildcarded */
            if (sym_match(iter, s.name)) {
                match = 1;
            }
#if UTIL_CONFIG_INCLUDE_ALIAS_NAMES == 1
            else if (s.ufname && sym_match(iter, s.ufname)) {
                match = 1;
            }
            else if (s.alias && sym_match(iter, s.alias)) {
                match = 1;
            }
#endif
        } else {
            /* Include wildcarded */
            const char *last_pos = s.name;
            int tokens_i = 0;

            for (tokens_i = 0; tokens_i < tokens.argc; tokens_i++) {
                const char *cur_pos = sal_strcasestr(last_pos,
                                                     tokens.argv[tokens_i]);
                if (cur_pos == NULL) {
                    break;
                }
                last_pos = cur_pos;
            }
            if (tokens_i == tokens.argc) {
                match = 1;
            }
        }
        if (!match) {
            continue;
        }

        /* Whew, name is okay */
        count++;

        if ((rc = iter->function(&s, iter->vptr)) < 0) {
            return rc;
        }
    }
    return count;
}
#endif /* CFG_CHIP_SYMBOLS_INCLUDED */
