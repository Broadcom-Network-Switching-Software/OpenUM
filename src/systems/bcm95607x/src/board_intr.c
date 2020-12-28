/*
 * $Id: board_intr.c,v 1.6 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include <system.h>
#include <intr.h>

void
board_intr_handling_disable(void)
{
    sys_intr_handling_save_disable(NULL);
}

void
board_intr_handling_enable(void)
{
    sys_intr_handling_restore_enable(NULL);
}
