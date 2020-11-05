/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "soc/mdk_phy.h"
#include "soc/bmd_phy_ctrl.h"
#include "soc/bmd_phy.h"
#include "xgsm_miim.h"


static const uint16 _phy_addr_bcm5354x[] = {
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
    0x01 + CDK_XGSM_MIIM_IBUS(0), /* Port 10 IntBus=0 Addr=0x01 */
    0x02 + CDK_XGSM_MIIM_IBUS(0), /* Port 11 IntBus=0 Addr=0x02 */
    0x03 + CDK_XGSM_MIIM_IBUS(0), /* Port 12 IntBus=0 Addr=0x03 */
    0x04 + CDK_XGSM_MIIM_IBUS(0), /* Port 13 IntBus=0 Addr=0x04 */
    0x05 + CDK_XGSM_MIIM_IBUS(0), /* Port 14 IntBus=0 Addr=0x05 */
    0x06 + CDK_XGSM_MIIM_IBUS(0), /* Port 15 IntBus=0 Addr=0x06 */
    0x07 + CDK_XGSM_MIIM_IBUS(0), /* Port 16 IntBus=0 Addr=0x07 */
    0x08 + CDK_XGSM_MIIM_IBUS(0), /* Port 17 IntBus=0 Addr=0x08 */
    0x0d + CDK_XGSM_MIIM_IBUS(1), /* Port 18 IntBus=1 Addr=0x0d */
    0x0d + CDK_XGSM_MIIM_IBUS(1), /* Port 19 IntBus=1 Addr=0x0d */
    0x0d + CDK_XGSM_MIIM_IBUS(1), /* Port 20 IntBus=1 Addr=0x0d */
    0x0d + CDK_XGSM_MIIM_IBUS(1), /* Port 21 IntBus=1 Addr=0x0d */
    0x0d + CDK_XGSM_MIIM_IBUS(1), /* Port 22 IntBus=1 Addr=0x0d */
    0x0d + CDK_XGSM_MIIM_IBUS(1), /* Port 23 IntBus=1 Addr=0x0d */
    0x0d + CDK_XGSM_MIIM_IBUS(1), /* Port 24 IntBus=1 Addr=0x0d */
    0x0d + CDK_XGSM_MIIM_IBUS(1), /* Port 25 IntBus=1 Addr=0x0d */
    0x09 + CDK_XGSM_MIIM_IBUS(0), /* Port 26 IntBus=0 Addr=0x09 */
    0x0a + CDK_XGSM_MIIM_IBUS(0), /* Port 27 IntBus=0 Addr=0x0a */
    0x0b + CDK_XGSM_MIIM_IBUS(0), /* Port 28 IntBus=0 Addr=0x0b */
    0x0c + CDK_XGSM_MIIM_IBUS(0), /* Port 29 IntBus=0 Addr=0x0c */
    0x0d + CDK_XGSM_MIIM_IBUS(0), /* Port 30 IntBus=0 Addr=0x0d */
    0x0e + CDK_XGSM_MIIM_IBUS(0), /* Port 31 IntBus=0 Addr=0x0e */
    0x0f + CDK_XGSM_MIIM_IBUS(0), /* Port 32 IntBus=0 Addr=0x0f */
    0x10 + CDK_XGSM_MIIM_IBUS(0), /* Port 33 IntBus=0 Addr=0x10 */
	0x01 + CDK_XGSM_MIIM_IBUS(1), /* Port 34 IntBus=1 Addr=0x01 */
	0x01 + CDK_XGSM_MIIM_IBUS(1), /* Port 35 IntBus=1 Addr=0x01 */
	0x01 + CDK_XGSM_MIIM_IBUS(1), /* Port 36 IntBus=1 Addr=0x01 */
	0x01 + CDK_XGSM_MIIM_IBUS(1), /* Port 37 IntBus=1 Addr=0x01 */
	0x05 + CDK_XGSM_MIIM_IBUS(1), /* Port 38 IntBus=1 Addr=0x05 */
	0x05 + CDK_XGSM_MIIM_IBUS(1), /* Port 39 IntBus=1 Addr=0x05 */
	0x05 + CDK_XGSM_MIIM_IBUS(1), /* Port 40 IntBus=1 Addr=0x05 */
	0x05 + CDK_XGSM_MIIM_IBUS(1), /* Port 41 IntBus=1 Addr=0x05 */
};

static uint32_t
_phy_addr(int pport)
{
    if (SOC_PORT_P2L_MAPPING(pport) > BCM5354X_LPORT_MAX) {
        return 0xFF;
    }

    return _phy_addr_bcm5354x[pport];
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
    return pport - 2;
}

phy_bus_t phy_bus_bcm5354x_miim_int = {
    "bcm5354x_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};
