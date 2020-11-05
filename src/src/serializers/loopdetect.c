/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "appl/persistence.h"

#if CFG_PERSISTENCE_SUPPORT_ENABLED

#ifdef CFG_SWITCH_LOOPDETECT_INCLUDED

extern int32
loopdetect_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium);

int32
loopdetect_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium)
{
    uint8 val8;
    /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(1);
    }

    if (op == SERIALIZE_OP_LOAD) {
        medium->read(&val8, 1);
        board_loop_detect_enable(val8);
    } else if (op == SERIALIZE_OP_SAVE) {
        val8 = board_loop_detect_status_get();
        medium->write(&val8, 1);
    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {
        board_loop_detect_enable(0);
        return 0;
    }

    return 1;
}
#endif /* CFG_SWITCH_LOOPDETECT_INCLUDED */

#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */
