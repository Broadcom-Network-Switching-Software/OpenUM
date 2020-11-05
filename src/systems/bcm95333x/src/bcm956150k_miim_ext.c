/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */


#include "system.h"
#include "xgsm_miim.h"
#include "soc/bcm5333x.h"

static const uint16 _phy_addr_bcm5339x[] = {
    0xFF,  /* Port  0 (cmic) N/A */
    0xFF,  /* Port  1        N/A */
    0x02 + CDK_XGSM_MIIM_EBUS(0), /* Port  2 ExtBus=0 Addr=0x02*/
    0xFF, /* Port  3        N/A */
    0xFF, /* Port  4        N/A */
    0xFF, /* Port  5        N/A */
    0x03 + CDK_XGSM_MIIM_EBUS(0), /* Port  6 ExtBus=0 Addr=0x03*/
    0xFF, /* Port  7        N/A */
    0xFF, /* Port  8        N/A */
    0xFF, /* Port  9        N/A */
    0x04 + CDK_XGSM_MIIM_EBUS(0), /* Port 10 ExtBus=0 Addr=0x04*/
    0xFF, /* Port  11        N/A */
    0xFF, /* Port  12        N/A */
    0xFF, /* Port  13        N/A */
    0x05 + CDK_XGSM_MIIM_EBUS(0), /* Port 14  ExtBus=0 Addr=0x05*/
    0xFF, /* Port  15        N/A */
    0xFF, /* Port  16        N/A */
    0xFF, /* Port  17        N/A */
    0x06 + CDK_XGSM_MIIM_EBUS(0), /* Port 18  ExtBus=0 Addr=0x06*/
    0xFF, /* Port  19        N/A */
    0xFF, /* Port  20        N/A */
    0xFF, /* Port  21        N/A */
    0x07 + CDK_XGSM_MIIM_EBUS(0), /* Port 22 ExtBus=0 Addr=0x07*/
    0xFF, /* Port  23        N/A */
    0xFF, /* Port  24        N/A */
    0xFF, /* Port  25        N/A */
    0xFF, /* Port  26        N/A */
    0xFF, /* Port  27        N/A */
    0xFF, /* Port  28        N/A */
    0xFF, /* Port  29        N/A */
    0xFF, /* Port  30        N/A */
    0xFF, /* Port  31        N/A */
    0xFF, /* Port  32        N/A */
    0xFF /* Port  33        N/A */
};

static uint32_t
_phy_addr(int pport)
{
    if (pport < BCM5333X_PORT_MAX) {
        switch (hr2_sw_info.devid) {
            case BCM53333_DEVICE_ID:
            case BCM53334_DEVICE_ID:
            case BCM53346_DEVICE_ID:
                if (pport >= 2 && pport < 18) {
                    return 0xFF;
                } else if (pport < 26) {
                    return pport + ((pport - 0x2) >> 3) + CDK_XGSM_MIIM_EBUS(0);
                } else if (pport < 34) {
                    return 0x01 + (pport - 26) + CDK_XGSM_MIIM_EBUS(1);
                }
                break;
            case BCM53344_DEVICE_ID:
                /* BCM953344R */
                if (pport >= 2 && pport < 18) {
                    return 0xFF;
                } else if (pport < 26) {
                    return (pport - 18) + CDK_XGSM_MIIM_EBUS(0);
                } else if (pport < 34) {
                    return 0x01 + (pport - 26) + CDK_XGSM_MIIM_EBUS(1);
                }
                break;
            case BCM53393_DEVICE_ID:
            case BCM53394_DEVICE_ID:
                 return _phy_addr_bcm5339x[pport];
            default :
                break;
        }
    }
    /* Should not get here */
    return 0xFF;
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

phy_bus_t phy_bus_bcm956150k_miim_ext = {
    "bcm956150k_miim_ext",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};


