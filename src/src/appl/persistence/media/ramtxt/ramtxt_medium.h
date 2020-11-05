/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _RAMTXT_MEDIUM_H_
#define _RAMTXT_MEDIUM_H_

#include "appl/medium.h"

/* 
 * NOTE: The parameter of SERIALIZABLE_MEDIUM.initialize function 
 *       in RAMTXT medium is the address of buffer.
 */

typedef struct {
    PERSISTENT_MEDIUM smedium;
    
    /* RAMTXT medium specific fields */
    uint32 index;
    unsigned char *memory;
    const char *item_name;
    unsigned char open_type; /* 0: not-open, 1: read, 2: write */
    unsigned char item_dirty;
    unsigned char item_modified;
    unsigned char item_read;
    uint32 item_index;
    
} MEDIUM_RAMTXT;

BOOL ramtxt_medium_initalize(MEDIUM_RAMTXT *medium, unsigned char *buf);

#endif /* _RAMTXT_MEDIUM_H_ */
