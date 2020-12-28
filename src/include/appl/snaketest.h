/*
 * $Id: snaketest.h,v 1.7 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _SNAKETEST_H_
#define _SNAKETEST_H_
#include "system.h"

typedef enum snaketest_type_s {
    SNAKETEST_TYPE_INT_MAC = 0,
    SNAKETEST_TYPE_INT_PHY,
    SNAKETEST_TYPE_EXT,
    SNAKETEST_TYPE_PORT_PAIR,
    SNAKETEST_TYPE_PKT_GEN,
    SNAKETEST_TYPE_COUNT
} snaketest_type_t;

extern void snaketest(uint8 mode, uint8 min_uport, uint8 max_uport, int duration);

#endif /* _SNAKETEST_H_ */
