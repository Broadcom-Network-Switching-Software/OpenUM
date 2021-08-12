/*
 * $Id: utgpio_internal.h,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#ifndef __UTGPIO_INTERNAL_H__

/* Enable local debug information. */
#define TEST_GPIO                 4

/* Define debug printf. */
#ifdef DEBUG
#define DEBUG_PRINT(args ...)     sal_printf(args)
#else
#define DEBUG_PRINT(args ...)
#endif

/* Test log helper macros. */
#define TEST_HEADER(x)                                 \
sal_printf("=====================================\n"); \
sal_printf(x); \
sal_printf("\n=====================================\n");

/* Selected GPIO number. */
extern int utgpio;

/* GPIO interrupt counter. */
extern int utgpio_intr_cnt;

/* GPIO interrupt type. */
extern int utgpio_intr_type;

/**
 * Interrupt handler for GPIO unittest.
 *
 * \param param Param for GPIO interrupt handler.
 */
extern void
utgpio_intr_handler(uint32 param);

#endif /* __UTGPIO_INTERNAL_H__ */
