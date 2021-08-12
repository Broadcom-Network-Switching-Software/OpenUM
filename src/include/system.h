/*
 * $Id: system.h,v 1.20 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <types.h>
#include "error.h"
#include "config.h"
#include "mcu.h"
#include <sal.h>
#include "board.h"
#include "soc/pbmp.h"
#include "soc/soc.h"
#include "soc/symbol.h"
#include "soc/chip.h"
#include "sal_config.h"
#if CFG_XGS_CHIP
#ifndef CFG_PCM_SUPPORT_INCLUDED
#include "soc/mdk_phy.h"
#include "soc/bmd_phy_ctrl.h"
#include "soc/bmd_phy.h"
#else
#include <pcm.h>
#endif
#else /* !CFG_XGS_CHIP */
#include "soc/phy.h"
#endif /* CFG_XGS_CHIP */
#include "pkt.h"
#if CFG_FLASH_SUPPORT_ENABLED
#include "flash.h"
#endif /* CFG_FLASH_SUPPORT_ENABLED */
#include "boardapi.h"
#include "kernel.h"
#include BOOT_SOC_INCLUDE_FILE
#include "utils/ports.h"

/* Default high priority task if device doesn't define. */
#ifndef POLLED_IRQ
#define POLLED_IRQ()
#endif

#endif /* _SYSTEM_H_ */
