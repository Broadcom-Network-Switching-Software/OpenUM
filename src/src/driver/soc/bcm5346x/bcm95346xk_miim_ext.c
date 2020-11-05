/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "xgsm_miim.h"
#include "soc/bcm5346x.h"

static const uint16 _phy_addr_bcm5346x[] = {
    0xFF,  /* Port  0 (cmic) N/A */
    0xFF,  /* Port  1        N/A */
    0xFF,  /* Port  2        N/A */
    0xFF,  /* Port  3        N/A */
    0xFF,  /* Port  4        N/A */
    0xFF,  /* Port  5        N/A */
    0xFF,  /* Port  6        N/A */
    0xFF,  /* Port  7        N/A */
    0xFF,  /* Port  8        N/A */
    0xFF,  /* Port  9        N/A */
    0xFF,  /* Port  10        N/A */
    0xFF,  /* Port  11        N/A */
    0xFF,  /* Port  12       N/A */

};

static uint32_t
_phy_addr(int pport)
{
    if (SOC_PORT_P2L_MAPPING(pport) > BCM5346X_LPORT_MAX) {
        return 0;
    }

    return _phy_addr_bcm5346x[SOC_PORT_P2L_MAPPING(pport)];
}

static int 
_read(int unit, uint32_t addr, uint32_t reg, uint32_t *val)
{
    return cdk_xgsm_miim_read(unit, addr, reg, val);
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    return cdk_xgsm_miim_write(unit, addr, reg, val);
}

static int
_phy_inst(int port)
{
    return port - 2;
}

phy_bus_t phy_bus_bcm95346xk_miim_ext = {
    "bcm95346xk_miim_ext",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

