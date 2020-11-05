/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.f
 */

#include "system.h"

/*
 * These are the possible debug types/flags for cdk_debug_level (below).
 */
#define CDK_DBG_ERR       (1 << 0)    /* Print errors */
#define CDK_DBG_WARN      (1 << 1)    /* Print warnings */
#define CDK_DBG_VERBOSE   (1 << 2)    /* General verbose output */
#define CDK_DBG_VVERBOSE  (1 << 3)    /* Very verbose output */
#define CDK_DBG_DEV       (1 << 4)    /* Device access */
#define CDK_DBG_REG       (1 << 5)    /* Register access */
#define CDK_DBG_MEM       (1 << 6)    /* Memory access */
#define CDK_DBG_SCHAN     (1 << 7)    /* S-channel operations */
#define CDK_DBG_MIIM      (1 << 8)    /* MII managment access */
#define CDK_DBG_DMA       (1 << 9)    /* DMA operations */
#define CDK_DBG_HIGIG     (1 << 10)   /* HiGig information */
#define CDK_DBG_PACKET    (1 << 11)   /* Packet data */

uint32 cdk_debug_level = CDK_DBG_ERR | CDK_DBG_WARN;
int (*cdk_debug_printf)(const char *format,...) = cdk_printf;
