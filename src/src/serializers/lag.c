/*
 * $Id: lag.c,v 1.5 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "appl/persistence.h"

#if CFG_PERSISTENCE_SUPPORT_ENABLED

#ifdef CFG_SWITCH_LAG_INCLUDED

extern int32
lag_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium);

int32
lag_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium)
{
    uint8 i, val8;
    uint8 uplist[MAX_UPLIST_WIDTH];

    sal_memset(uplist, 0, MAX_UPLIST_WIDTH);
    /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(1);
    }

    if (op == SERIALIZE_OP_LOAD) {
        medium->read(&val8, 1);
        board_lag_set(val8);
        for (i = 1 ; i <= BOARD_MAX_NUM_OF_LAG ; i++) {
            medium->read(&val8, 1);        
            medium->read(uplist, MAX_UPLIST_WIDTH);
            board_lag_group_set(i, val8, uplist);
        }        
    } else if (op == SERIALIZE_OP_SAVE) {
        board_lag_get(&val8);
        medium->write(&val8, 1);
        
        for (i = 1 ; i <= BOARD_MAX_NUM_OF_LAG ; i++) {
            board_lag_group_get(i, &val8, uplist);
            medium->write(&val8, 1);
            medium->write(uplist, MAX_UPLIST_WIDTH);
        }                
    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {
        board_lag_set(TRUE);
        for (i = 1 ; i <= BOARD_MAX_NUM_OF_LAG ; i++) {
            board_lag_group_set(i, FALSE, uplist);
        }
        return 0;
    }

    return (1 + ((1 + MAX_UPLIST_WIDTH) * BOARD_MAX_NUM_OF_LAG));
}

#endif /* CFG_SWITCH_LAG_INCLUDED */

#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */
