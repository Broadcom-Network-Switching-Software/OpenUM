/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _UTILS_FACTORY_H_
#define _UTILS_FACTORY_H_

/*  *********************************************************************
    *  Data structures.
 */

typedef struct factory_config_s {
    uint32  magic;
    uint8   mac[6];
    uint32  serial_num_magic;
    uint8   serial_num[20];
} factory_config_t;

/*  *********************************************************************
    *  Constants
 */

#define FACTORY_CONFIG_MAGIC        0x46414354     /* 'FACT' (big-endian) */

#ifdef CFG_FACTORY_CONFIG_BASE
#define FACTORY_CONFIG_BASE_ADDR    CFG_FACTORY_CONFIG_BASE
#define FACTORY_CONFIG_OFFSET       CFG_FACTORY_CONFIG_OFFSET
#else
#define FACTORY_CONFIG_BASE_ADDR    0xBFC00000      /* The first sector */
#define FACTORY_CONFIG_OFFSET       0x20            /* Offset */
#endif

#ifdef CFG_PRODUCT_REGISTRATION_INCLUDED
#define FACTORY_SERIAL_NUMBER_MAGIC 0x53455249      /* 'SERI' */
#endif /* CFG_PRODUCT_REGISTRATION_INCLUDED */


/*  *********************************************************************
    *  Prototypes
 */

sys_error_t factory_config_get(factory_config_t *cfg);
sys_error_t factory_config_set(factory_config_t *cfg);

#endif /* _UTILS_FACTORY_H_ */
