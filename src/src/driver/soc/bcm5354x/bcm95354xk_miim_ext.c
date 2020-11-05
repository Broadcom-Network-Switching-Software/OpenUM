/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "xgsm_miim.h"
#include "soc/mdk_phy.h"
#include "soc/bmd_phy_ctrl.h"
#include "soc/bmd_phy.h"

extern uint8 qtc_interface;

static const uint16 _phy_addr_bcm5354x[] = {
    0xFF,  /* Port 0 (cmic)  N/A */
    0xFF,  /* Port 1         N/A */
    0xFF,  /* Port 2         N/A */
    0xFF,  /* Port 3         N/A */
    0xFF,  /* Port 4         N/A */
    0xFF,  /* Port 5         N/A */
    0xFF,  /* Port 6         N/A */
    0xFF,  /* Port 7         N/A */
    0xFF,  /* Port 8         N/A */
    0xFF,  /* Port 9         N/A */
    0xFF,  /* Port 10        N/A */
    0xFF,  /* Port 11        N/A */
    0xFF,  /* Port 12        N/A */
    0xFF,  /* Port 13        N/A */
    0xFF,  /* Port 14        N/A */
    0xFF,  /* Port 15        N/A */
    0xFF,  /* Port 16        N/A */
    0xFF,  /* Port 17        N/A */
    0x0A + CDK_XGSM_MIIM_EBUS(0), /* Port 18 ExtBus=0 Addr=0x0A*/
    0x0B + CDK_XGSM_MIIM_EBUS(0), /* Port 19 ExtBus=0 Addr=0x0B*/
    0x0C + CDK_XGSM_MIIM_EBUS(0), /* Port 20 ExtBus=0 Addr=0x0C*/
    0x0D + CDK_XGSM_MIIM_EBUS(0), /* Port 21 ExtBus=0 Addr=0x0D*/
    0x0E + CDK_XGSM_MIIM_EBUS(0), /* Port 22 ExtBus=0 Addr=0x0E*/
    0x0F + CDK_XGSM_MIIM_EBUS(0), /* Port 23 ExtBus=0 Addr=0x0F*/
    0x10 + CDK_XGSM_MIIM_EBUS(0), /* Port 24 ExtBus=0 Addr=0x10*/
    0x11 + CDK_XGSM_MIIM_EBUS(0), /* Port 25 ExtBus=0 Addr=0x11*/
    0xFF,  /* Port 26        N/A */
    0xFF,  /* Port 27        N/A */
    0xFF,  /* Port 28        N/A */
    0xFF,  /* Port 29        N/A */
    0xFF,  /* Port 30        N/A */
    0xFF,  /* Port 31        N/A */
    0xFF,  /* Port 32        N/A */
    0xFF,  /* Port 33        N/A */

};

static uint32_t
_phy_addr(int pport)
{
    if (SOC_PORT_P2L_MAPPING(pport) > BCM5354X_LPORT_MAX) {
        return 0xFF;
    }

    if (qtc_interface == QTC_INTERFACE_QSGMII) {
        return _phy_addr_bcm5354x[pport];
    } else {
        /* SGMII mode or Fiber mode */
        return 0xFF;
    }    
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

phy_bus_t phy_bus_bcm95354xk_miim_ext = {
    "bcm95354xk_miim_ext",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};

