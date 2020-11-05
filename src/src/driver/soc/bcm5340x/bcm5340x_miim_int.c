/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "xgsm_miim.h"
#include "soc/bcm5340x.h"

static uint32_t
_phy_addr(int pport)
{
    if (pport >= 2 && pport < 18) {
        return ((pport - 2) & ~0xf) + 1 + CDK_XGSM_MIIM_IBUS(0);
    } else if (pport >= 18 && pport < 38) {
        return ((pport - 18) & ~0x3) + 1 + CDK_XGSM_MIIM_IBUS(1);
    }
    /* Should not get here */
    return 0xFF;
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return cdk_xgsm_sbus_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return cdk_xgsm_sbus_write(unit, addr, reg, val);
}

static int
_phy_inst(int pport)
{
    return pport - 2;
}

phy_bus_t phy_bus_bcm5340x_miim_int = {
    "bcm5340x_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};
