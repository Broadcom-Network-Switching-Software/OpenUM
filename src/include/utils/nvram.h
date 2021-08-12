/*
 * $Id: nvram.h,v 1.6 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _UTILS_NVRAM_H_
#define _UTILS_NVRAM_H_

/*  *********************************************************************
    *  Data structures.
 */

typedef struct nvram_header_s {
    uint32  magic;
    uint32  chksum;
    uint32  len;
    uint32  crc_ver_init;
} nvram_header_t;

#define NVRAM_VER_SHIFT        0
#define NVRAM_VER_MASK         (0xFF << NVRAM_VER_SHIFT)
#define NVRAM_CRC_SHIFT        8
#define NVRAM_CRC_MASK         (0xFF << NVRAM_CRC_SHIFT)
#define NVRAM_INIT_SHIFT       16
#define NVRAM_INIT_MASK        (0xFFF << NVRAM_INIT_SHIFT)


/*  *********************************************************************
    *  Constants
 */

#define NVRAM_MAGIC            0x48534C46     /* 'FLSH' (little-endian) */
#define NVRAM_VERSION          1

#ifdef CFG_CONFIG_BASE
#define NVRAM_BASE            CFG_CONFIG_BASE
#else
#define NVRAM_BASE            0xBFC0F000        /* The last sector */
#endif

#ifdef CFG_CONFIG_OFFSET
#define NVRAM_OFFSET            CFG_CONFIG_OFFSET
#else
#define NVRAM_OFFSET            0x0        /* The last sector */
#endif

#ifdef CFG_CONFIG_SIZE
#define NVRAM_SPACE            CFG_CONFIG_SIZE
#else
#define NVRAM_SPACE            0x1000         /* Header plus tuples */
#endif

/*  *********************************************************************
    *  Prototypes
 */

/* Initialize from non-volatile storage. */
sys_error_t nvram_init (void);

/* Lookup, add, delete variables. */
const char *nvram_get(const char *name);
sys_error_t nvram_set(const char *name, const char *value);
sys_error_t nvram_unset(const char *name);

/* Commit to non-volatile storage. */
sys_error_t nvram_commit(void);

/* Enumerate all tuples in arbitrary order.  Calls of set and unset
   have unpredictable effect. */
sys_error_t nvram_enum (sys_error_t (*proc)(const char *tuple));
sys_error_t nvram_show_tuple(const char *tuple);

#endif /* _UTILS_NVRAM_H_ */
