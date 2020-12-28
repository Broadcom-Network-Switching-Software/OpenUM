/*
 * $Id: intr.h,v 1.6 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef INTR_H
#define INTR_H

/**
 * Disable interrupt handling.
 */
extern void
board_intr_handling_disable(void);

/**
 * Enable interrupt handling.
 */
extern void
board_intr_handling_enable(void);

#endif /* INTR_H */
