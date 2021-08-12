/*
 * $Id: persistence.c,v 1.9 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "board.h"
#include "appl/serialize.h"
#include "serialize/serialization.h" 
#include "appl/persistence.h"

#include "media/flash/flash_medium.h"
#include "media/ramtxt/ramtxt_medium.h"

#if CFG_PERSISTENCE_SUPPORT_ENABLED

/* Persistent-data-compatible versions */
const char * CODE um_compatible_versions[] = { "all", NULL };

static MEDIUM_FLASH medium_current;

/* We use this definition to keep the medium flexible */
#define MED_CURRENT_SETTING  ((PERSISTENT_MEDIUM *)&medium_current)

void
persistence_init(void)
{
    extern void serializers_init(void);
    
#ifdef CFG_NVRAM_SUPPORT_INCLUDED
//    nvram_medium_initialize(&medium_current, NULL, ALL_CURRENT_SETTINGS);
#endif

    flash_medium_initialize(&medium_current, 
                            MEDIUM_FLASH_START_ADDRESS, ALL_CURRENT_SETTINGS);


    serializers_init();
}

BOOL
persistence_load_all_current_settings(void)
{
    return persistence_load_group_current_settings(ALL_CURRENT_SETTINGS);
}

BOOL
persistence_save_all_current_settings(void)
{
    return persistence_save_group_current_settings(ALL_CURRENT_SETTINGS);
}

BOOL
persistence_load_current_settings(const char *name)
{
    if (serialize_by_name(name, SERIALIZE_OP_LOAD, MED_CURRENT_SETTING)) {
        return TRUE;
    }
    return FALSE;
}

BOOL
persistence_load_group_current_settings(uint32 groups)
{
    if (serialize_by_groups(
            groups, 
            SERIALIZE_OP_LOAD, 
            MED_CURRENT_SETTING) < 0) {
        return FALSE;
    }

    return TRUE;
}

BOOL
persistence_save_current_settings(const char *name)
{
    if (serialize_by_name(name, SERIALIZE_OP_SAVE, MED_CURRENT_SETTING)) {
        return TRUE;
    }
    
    return FALSE;
}

BOOL
persistence_save_group_current_settings(uint32 groups)
{
    if (serialize_by_groups(
            groups, 
            SERIALIZE_OP_SAVE, 
            MED_CURRENT_SETTING) < 0) {

        return FALSE;
    }
    
    return TRUE;
}

BOOL
persistence_validate_current_settings(void)
{
    if (serialize_medium_for_validation(MED_CURRENT_SETTING, FALSE) < 0) {
        return FALSE;
    }
    
    return TRUE;
}

BOOL
persistence_restore_factory_defaults(void)
{
    if (serialize_by_groups(
            ALL_FACTORY_DEFAULTS, 
            SERIALIZE_OP_LOAD_DEFAULTS, 
            MED_CURRENT_SETTING) < 0) {
        return FALSE;
    }
         
    return TRUE;
}

BOOL
persistence_restore_factory_defaults_by_name(const char *name)
{
    if (serialize_by_name(
                name, 
                SERIALIZE_OP_LOAD_DEFAULTS, 
                MED_CURRENT_SETTING)) {
        return TRUE;
    }
    
    return FALSE;
}

BOOL
persistence_restore_factory_defaults_by_group(uint32 groups)
{
    if (serialize_by_groups(
            groups, 
            SERIALIZE_OP_LOAD_DEFAULTS, 
            MED_CURRENT_SETTING) < 0) {
        return FALSE;
    }
    
    return TRUE;
}

BOOL
persistence_backup_to_memory(void *buffer)
{
    MEDIUM_RAMTXT medium;

    if (buffer == NULL) {
        return FALSE;
    }

    ramtxt_medium_initalize(&medium, buffer);
    if (serialize_by_groups(
            ALL_BACKUP_RESTORE, 
            SERIALIZE_OP_SAVE, 
            (PERSISTENT_MEDIUM *)&medium) < 0) {
        return FALSE;
    }

    return TRUE;
}

BOOL
persistence_restore_from_memory(void *buffer)
{
    MEDIUM_RAMTXT medium;

    if (buffer == NULL) {
        return FALSE;
    }

    ramtxt_medium_initalize(&medium, buffer);
    if (serialize_by_groups(
            ALL_BACKUP_RESTORE, 
            SERIALIZE_OP_LOAD, 
            (PERSISTENT_MEDIUM *)&medium) < 0) {
        return FALSE;
    }

    return TRUE;
}

BOOL
persistence_validate_data_in_memory(void *buffer)
{
    MEDIUM_RAMTXT medium;

    if (buffer == NULL) {
        return FALSE;
    }

    ramtxt_medium_initalize(&medium, buffer);
    if (serialize_medium_for_validation((PERSISTENT_MEDIUM *)&medium, FALSE) < 0) {
        return FALSE;
    }

    return TRUE;
}

#endif /* CFG_PERSISTENCE_SUPPORT_ENABLED */
