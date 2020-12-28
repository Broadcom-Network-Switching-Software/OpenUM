/*
 * $Id: system.c,v 1.3 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "utils/system.h"
#include "appl/persistence.h"

#if CFG_PERSISTENCE_SUPPORT_ENABLED
extern int32
system_name_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;

extern int32
port_desc_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;

extern int32
port_enable_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;

#ifdef CFG_PRODUCT_REGISTRATION_INCLUDED
extern int32
registration_status_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT;
#endif /* CFG_PRODUCT_REGISTRATION_INCLUDED */

int32
system_name_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT
{
    unsigned char buf[MAX_SYSTEM_NAME_LEN + 1];

    /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(1);
    }

    if (op == SERIALIZE_OP_LOAD) {

        medium->read(buf, MAX_SYSTEM_NAME_LEN);
        /*
         * Make sure it's null terminated
         */
        buf[MAX_SYSTEM_NAME_LEN] = 0;

        set_system_name((char *)buf);

    } else if (op == SERIALIZE_OP_SAVE) {

        get_system_name((char *)buf, sizeof(buf));
        medium->write(buf, MAX_SYSTEM_NAME_LEN);

    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {

        set_system_name((char *)DEFAULT_SYSTEM_NAME);
        return 0;
    }

    return MAX_SYSTEM_NAME_LEN;
}

int32
port_desc_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT
{
    unsigned char buf[WEB_PORT_DESC_LEN + 1];
    uint16 uport;

    /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(1);
    }

    if (op == SERIALIZE_OP_LOAD) {

        SAL_UPORT_ITER(uport) {
            sal_memset(buf, 0, sizeof(buf));
            medium->read(buf, WEB_PORT_DESC_LEN);
            /*
             * Make sure it's null terminated
             */
            buf[WEB_PORT_DESC_LEN] = 0; 
            set_port_desc(uport, (char *)buf);
        }
    } else if (op == SERIALIZE_OP_SAVE) {

        SAL_UPORT_ITER(uport) {
            sal_memset(buf, 0, sizeof(buf)); /* Get_port_desc may fail, clear buf before get_port_desc()*/
            get_port_desc(uport, (char *)buf, WEB_PORT_DESC_LEN);                             
            medium->write(buf, WEB_PORT_DESC_LEN);
        }
    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {

        SAL_UPORT_ITER(uport) {          
            set_port_desc(uport, "");                     
        }
        
        return 0;
    }

    return (WEB_PORT_DESC_LEN * board_uport_count());

}

int32
port_enable_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT
{
    uint16 uport;
    BOOL porten;
    uint16 port_en;

    /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(1);
    }

    if (op == SERIALIZE_OP_LOAD) {

        SAL_UPORT_ITER(uport) {
        
            medium->read_uint16(&port_en);
            porten = 0;
            if (port_en) {
                porten = 1;
            }
            board_port_enable_set(uport, porten);
        }
    } else if (op == SERIALIZE_OP_SAVE) {

        SAL_UPORT_ITER(uport) {
            board_port_enable_get(uport, &porten);
            medium->write_uint16(porten);
        }
    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {
        SAL_UPORT_ITER(uport) {
            board_port_enable_set(uport, 1);
        }

        return 0;
    }

    return (board_uport_count() * 2);

}

#ifdef CFG_PRODUCT_REGISTRATION_INCLUDED
int32
registration_status_serializer(SERIALIZE_OP op, SERL_MEDIUM_BIN *medium) REENTRANT
{
    unsigned char reg_status;

    /*
     * NOTE: Update the version number if serializer code changes.
     */
    if (op == SERIALIZE_OP_VERSION) {
        return MAKE_SERIALIZER_VERSION(1);
    }

    if (op == SERIALIZE_OP_LOAD) {

        medium->read(&reg_status, MAX_REG_STATUS_LEN);

        set_registration_status(reg_status);

    } else if (op == SERIALIZE_OP_SAVE) {

        get_registration_status(&reg_status);
        medium->write(&reg_status, MAX_REG_STATUS_LEN);

    } else if (op == SERIALIZE_OP_LOAD_DEFAULTS) {

        reg_status = DEFAULT_REGISTRATION_STATUS;
        set_registration_status(reg_status);
        return 0;
    }

    return MAX_REG_STATUS_LEN;
}
#endif /* CFG_PRODUCT_REGISTRATION_INCLUDED */
#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */
