/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "appl/persistence.h"
#include "appl/igmpsnoop.h"

#if CFG_PERSISTENCE_SUPPORT_ENABLED

extern int32
mcast_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;

extern int32
igmpsnoop_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;

#ifdef CFG_SWITCH_MCAST_INCLUDED
int32
mcast_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT
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
        board_block_unknown_mcast_set(val8);
    } else if (op == SERIALIZE_OP_SAVE) {
        board_block_unknown_mcast_get(&val8);
        medium->write(&val8, 1);
    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {
        /* Default disabled */
        board_block_unknown_mcast_set(0);
        return 0;
    }

    return 1;
}



int32
igmpsnoop_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT
{
    uint8 val8;
    uint16 val16;
   /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(2);
    }

    if (op == SERIALIZE_OP_LOAD) {
        medium->read(&val8, 1);
        igmpsnoop_enable_set(val8);
        medium->read_uint16(&val16);
        igmpsnoop_vid_set(val16);
    } else if (op == SERIALIZE_OP_SAVE) {
        igmpsnoop_enable_get(&val8);
        medium->write(&val8, 1);
        igmpsnoop_vid_get(&val16);
        medium->write_uint16(val16);
    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {
        igmpsnoop_enable_set(1);
        igmpsnoop_vid_set(1);
        return 0;
    }

    return 3;
}
#endif /* CFG_SWITCH_MCAST_INCLUDED */


#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */
