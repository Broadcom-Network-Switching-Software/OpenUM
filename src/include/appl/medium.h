/*
 * $Id: medium.h,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _MEDIUM_H_
#define _MEDIUM_H_

/*
 * Capabilities/properties of a persistent medium
 */
enum {
    /* Compact form; no extra meta; no naming but can be seekable */
    SMEDIUM_PROP_COMPACT = 0x01,
    
    /* Can get (data) size of individual item */
    SMEDIUM_PROP_GET_ITEM_DATA_SIZE = 0x02,
    
    /* Can get (data) summed size of all items */
    SMEDIUM_PROP_GET_TOTAL_DATA_SIZE = 0x04,

    /* Can get serializer version of individual item */
    SMEDIUM_PROP_GET_ITEM_VERSION = 0x08
};

typedef struct _smedium_s {
    
    int32 (*open)(BOOL write, struct _smedium_s *ps) REENTRANT;
    int32 (*close)(struct _smedium_s *ps) REENTRANT;
    
    BOOL (*item_begin)(const char *name, struct _smedium_s *ps, int32 version) REENTRANT;
    BOOL (*item_end)(struct _smedium_s *ps) REENTRANT;
    int32 (*item_size)(struct _smedium_s *ps) REENTRANT;
    int32 (*item_version)(struct _smedium_s *ps) REENTRANT;
    
    int32 (*read)(uint32 len, uint8 *buf, struct _smedium_s *ps) REENTRANT;
    int32 (*write)(uint32 len, uint8 *buf, struct _smedium_s *ps) REENTRANT;
    
#ifdef CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED
    const char * (*read_string)(struct _smedium_s *ps) REENTRANT;
    BOOL (*write_string)(const char *str, struct _smedium_s *ps) REENTRANT;
#endif /* CFG_PERSISTENCE_STRING_SUPPORT_INCLUDED */
    
    BOOL (*seek)(uint32 pos, struct _smedium_s *ps) REENTRANT;
    
    const char * (*get_medium_identification)(struct _smedium_s *ps) REENTRANT;
    uint32 (*medium_properties)(struct _smedium_s *ps) REENTRANT;
    int32 (*get_total_data_size)(struct _smedium_s *ps) REENTRANT;
    uint32 (*get_valid_groups)(struct _smedium_s *ps) REENTRANT;
    
} PERSISTENT_MEDIUM;

#endif /* _MEDIUM_H_ */
