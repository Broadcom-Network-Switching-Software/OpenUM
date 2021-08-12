/*
 * $Id: qos.c,v 1.13 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "appl/persistence.h"

#if CFG_PERSISTENCE_SUPPORT_ENABLED

extern int32
qos_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;

extern int32
rate_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;

extern int32
storm_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;

#ifdef CFG_SWITCH_QOS_INCLUDED

int32
qos_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT
{
    uint8 val8;
    qos_type_t type;
    uint16 uport;
    /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(1);
    }

    if (op == SERIALIZE_OP_LOAD) {
        medium->read(&val8, 1);
        type = (qos_type_t)val8;
        board_qos_type_set(type);
        SAL_UPORT_ITER(uport) {
            medium->read(&val8, 1);
            if (type == QT_PORT_BASED) {
                board_untagged_priority_set(uport, val8);
            }
        }
    } else if (op == SERIALIZE_OP_SAVE) {
        board_qos_type_get(&type);
        val8 = (uint8)type;
        medium->write(&val8, 1);
        SAL_UPORT_ITER(uport) {
            board_untagged_priority_get(uport, &val8);
            medium->write(&val8, 1);
        }
    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {
        board_qos_type_set(QT_DOT1P_PRIORITY);
        return 0;
    }

    return board_uport_count()+1;
}

#endif /* CFG_SWITCH_QOS_INCLUDED */

#ifdef CFG_SWITCH_RATE_INCLUDED

int32
rate_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT
{
    uint32 rate;
    uint16 uport;
    /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(1);
    }

    if (op == SERIALIZE_OP_LOAD) {
        SAL_UPORT_ITER(uport) {
            medium->read_uint32(&rate);
            board_port_rate_ingress_set(uport, rate);
            medium->read_uint32(&rate);
            board_port_rate_egress_set(uport, rate);
        }
    } else if (op == SERIALIZE_OP_SAVE) {
        SAL_UPORT_ITER(uport) {
            board_port_rate_ingress_get(uport, &rate);
            medium->write_uint32(rate);
            board_port_rate_egress_get(uport, &rate);
            medium->write_uint32(rate);
        }
    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {
        SAL_UPORT_ITER(uport) {
            board_port_rate_ingress_set(uport, 0);
            board_port_rate_egress_set(uport, 0);
        }
        return 0;
    }

    return board_uport_count()*8;
}

int32
storm_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT
{
    uint8 val8;
    uint32 val32;
    uint16 uport;
    /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(1);
    }

    if (op == SERIALIZE_OP_LOAD) {
        medium->read_uint32(&val32);
        board_rate_type_set((uint8)val32);
        SAL_UPORT_ITER(uport) {
            medium->read_uint32(&val32);
            board_rate_set(uport, val32);
        }
    } else if (op == SERIALIZE_OP_SAVE) {
        board_rate_type_get(&val8);
        medium->write_uint32((uint32)val8);
        SAL_UPORT_ITER(uport) {
            board_rate_get(uport, &val32);
            medium->write_uint32(val32);
        }
    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {
        board_rate_type_set((uint8)STORM_RATE_NONE);
        SAL_UPORT_ITER(uport) {
            board_rate_set(uport, 0);
        }
        return 0;
    }

    return 4+board_uport_count()*4;
}

#endif /* CFG_SWITCH_RATE_INCLUDED */

#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */
