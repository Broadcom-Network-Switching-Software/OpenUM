/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _NVRAM_MEDIUM_H_
#define _NVRAM_MEDIUM_H_

#include "appl/medium.h"

/* 
 * NOTE: The parameter of SERIALIZABLE_MEDIUM.initialize function 
 *       in NVRAM medium is the prefix (char *) of variable names.
 */

typedef struct {
    PERSISTENT_MEDIUM smedium;
    
    /* NVRAM medium specific fields */
    const char *prefix;
    const char *item_name;
    unsigned char *buf;
    int32 item_index;
    int32 item_length;
    int32 item_version;
    unsigned char open_type; /* 0: not-open, 1: read, 2: write */
    unsigned char item_dirty;
    uint32 bits_buf;
    uint32 nbits;
    uint32 groups;
    
} MEDIUM_NVRAM;

extern BOOL nvram_medium_initialize(
        MEDIUM_NVRAM *medium, const char *prefix, uint32 groups) REENTRANT;

#endif /* _NVRAM_MEDIUM_H_ */
