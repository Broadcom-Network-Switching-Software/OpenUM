/*
 * $Id: mirror.c,v 1.10 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "appl/persistence.h"

#if CFG_PERSISTENCE_SUPPORT_ENABLED

#ifdef CFG_SWITCH_MIRROR_INCLUDED

extern int32
mirror_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;

int32
mirror_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT
{
    BOOL mirror_enabled = FALSE;
    uint8 val8;
    uint16 uport;

    /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(1);
    }

    if (op == SERIALIZE_OP_LOAD) {
        SAL_UPORT_ITER(uport) {
             medium->read(&val8, 1);
             board_mirror_port_set(uport, val8);
             if (val8) {
                 mirror_enabled = TRUE;
             }
        }
        medium->read_uint16(&uport);
        if (mirror_enabled) {
            board_mirror_to_set(uport);
        }
    } else if (op == SERIALIZE_OP_SAVE) {
        SAL_UPORT_ITER(uport) {
            board_mirror_port_get(uport, &val8);
            medium->write(&val8, 1);
        }
        board_mirror_to_get(&uport);
        medium->write_uint16(uport);
    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {
        SAL_UPORT_ITER(uport) {
            board_mirror_port_set(uport, FALSE);
        }
        board_mirror_to_set(0);
        return 0;
    }

    return board_uport_count()+2;
}

#endif /* CFG_SWITCH_MIRROR_INCLUDED */

#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */
