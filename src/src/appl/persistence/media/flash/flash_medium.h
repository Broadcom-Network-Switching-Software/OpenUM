/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _FLASH_MEDIUM_H_
#define _FLASH_MEDIUM_H_

#include "appl/medium.h"

/* Flash medium header */
typedef struct med_header_s {
    uint8               magic[4];
    uint16              size;           /* Header not included */
    uint16              checksum;       /* Header not included */
} med_header_t;

/* Item header */
typedef struct med_item_hdr_s {
    uint16              version;
    uint16              datalen;
    char                name[1];
} med_item_hdr_t;

/* Item holder */
typedef struct med_item_s {
    med_item_hdr_t    * header;
} med_item_t;

typedef struct {
    PERSISTENT_MEDIUM smedium;
    
    /* Flash medium specific fields */
    uint32            groups;
    hsaddr_t          start;
    uint8             open_type; /* 0: not-open, 1: read, 2: write */
    med_header_t    * header;
    med_item_t      * items;
    med_item_hdr_t  * item;
    uint16            item_index;
    uint8           * item_ptr;
    uint8           * databuf;
    uint8           * newbuf;
    uint16            nbuf_len;
    
} MEDIUM_FLASH;

extern BOOL flash_medium_initialize(MEDIUM_FLASH *medium, hsaddr_t addr, uint32 groups) REENTRANT;

#endif /* _FLASH_MEDIUM_H_ */
