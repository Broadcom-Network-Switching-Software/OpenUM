/*
 * $Id: persistence.h,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _PERSISTENCE_H_
#define _PERSISTENCE_H_

#include "serialize.h"

/*
 * What to serialize for "ALL"?
 */
#define ALL_CURRENT_SETTINGS    (0xFFFFFFFF)
#define ALL_FACTORY_DEFAULTS    (0xFFFFFFFF)
#define ALL_BACKUP_RESTORE      (0xFFFFFFFF)

/*
 * Basic classification for different categories of settings.
 * 
 * Please follow this rule to assign your group bits.
 */ 
#define SERL_GRP_SWITCH     (0x0000FFFE)
#define SERL_GRP_PROTOCOL   (0x00FF0000)
#define SERL_GRP_KERNEL     (0x0F000000)
#define SERL_GRP_NETWORK    (0xF0000000)

/*
 * Sub-groups
 */
#define SERL_GRP_NETWORK_MAC        (0x80000000)
#define SERL_GRP_NETWORK_IP         (0x40000000)
#define SERL_GRP_KERNEL_LOGGING     (0x02000000)

/*
 * Initialization
 */
void persistence_init(void);


/*
 * Current Settings
 */
 
extern BOOL persistence_load_all_current_settings(void);

extern BOOL persistence_save_all_current_settings(void);

extern BOOL persistence_load_current_settings(const char *name);

extern BOOL persistence_load_group_current_settings(uint32 groups);

extern BOOL persistence_save_current_settings(const char *name);

extern BOOL persistence_save_group_current_settings(uint32 groups);

extern BOOL persistence_validate_current_settings(void);

extern BOOL persistence_backup_to_memory(void *buffer);

extern BOOL persistence_restore_from_memory(void *buffer);

extern BOOL persistence_validate_data_in_memory(void *buffer);

/*
 * Factory Defaults
 */
 
extern BOOL persistence_restore_factory_defaults(void);

extern BOOL persistence_restore_factory_defaults_by_group(uint32 groups);

extern BOOL persistence_restore_factory_defaults_by_name(const char *name);

#endif /* _PERSISTENCE_H_ */
