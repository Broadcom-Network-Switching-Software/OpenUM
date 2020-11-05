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
    0x01 + CDK_XGSM_MIIM_IBUS(0), /* Port  1 ( ge0) IntBus=0 Addr=0x01*/
    0x01 + CDK_XGSM_MIIM_IBUS(0), /* Port  2 ( ge1) IntBus=0 Addr=0x02*/
    0x01 + CDK_XGSM_MIIM_IBUS(0), /* Port  3 ( ge2) IntBus=0 Addr=0x03*/
    0x01 + CDK_XGSM_MIIM_IBUS(0), /* Port  4 ( ge3) IntBus=0 Addr=0x04*/
    0x01 + CDK_XGSM_MIIM_IBUS(1), /* Port  5 ( ge4) IntBus=1 Addr=0x01*/
    0x01 + CDK_XGSM_MIIM_IBUS(1), /* Port  6 ( ge5) IntBus=1 Addr=0x01*/
    0x01 + CDK_XGSM_MIIM_IBUS(1), /* Port  7 ( ge6) IntBus=1 Addr=0x01*/
    0x01 + CDK_XGSM_MIIM_IBUS(1), /* Port  8 ( ge7) IntBus=1 Addr=0x01*/
    0x05 + CDK_XGSM_MIIM_IBUS(1), /* Port  9 ( ge8) IntBus=1 Addr=0x05*/
    0x05 + CDK_XGSM_MIIM_IBUS(1), /* Port 10 ( ge9) IntBus=1 Addr=0x05*/
    0x05 + CDK_XGSM_MIIM_IBUS(1), /* Port 11 ( ge10) IntBus=1 Addr=0x05*/
    0x05 + CDK_XGSM_MIIM_IBUS(1), /* Port 12 ( ge11) IntBus=1 Addr=0x05*/

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
    if (CDK_XGSM_MIIM_IBUS_NUM(addr) == 1) {
        return cdk_xgsm_sbus_read(unit, addr, reg, val);
    } else {
        return cdk_xgsm_miim_read(unit, addr, reg, val);
    }
}

static int 
_write(int unit, uint32_t addr, uint32_t reg, uint32_t val)
{
    if (CDK_XGSM_MIIM_IBUS_NUM(addr) == 1) {
        return cdk_xgsm_sbus_write(unit, addr, reg, val);
    } else {
        return cdk_xgsm_miim_write(unit, addr, reg, val);
    }
}

static int
_phy_inst(int pport)
{
    return (pport - 1) % 4;
}

phy_bus_t phy_bus_bcm5346x_miim_int = {
    "bcm5346x_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};
