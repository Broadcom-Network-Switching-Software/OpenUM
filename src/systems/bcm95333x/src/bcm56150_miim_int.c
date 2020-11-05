/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */


#include "system.h"
#include "xgsm_miim.h"
#include "soc/bcm5333x.h"

/*
 *Internal phy address:
 * CMICd chain 0 :   (QGPHY and QSGMII2X will not coexist in same package, either one will be disabled)
 *               QGPHY inst 0  : 1 to 5
 *               QGPHY inst 1 : 6 to 10
 *               QGPHY inst 2 : 11 to 15
 *               QGPHY inst 3 : 16 to 20
 *               QSGMII2X inst 0 : 1
 *               QSGMII2X inst 1 : 9
 *
 * CMICd chain 1 :
 *               TSC4 inst 0 :   1
 *               TSC4 inst 1 :   5
 *               QSGMII2X inst 2 : 11
 */


static const uint16 _phy_addr_bcm5333x[] = {
    0xFF,  /* Port  0 (cmic) N/A */
    0xFF,  /* Port  1        N/A */
    0x09 + CDK_XGSM_MIIM_IBUS(0), /* Port  2 IntBus=0 Addr=0x09 */
    0x08 + CDK_XGSM_MIIM_IBUS(0), /* Port  3 IntBus=0 Addr=0x08 */
    0x07 + CDK_XGSM_MIIM_IBUS(0), /* Port  4 IntBus=0 Addr=0x07 */
    0x06 + CDK_XGSM_MIIM_IBUS(0), /* Port  5 IntBus=0 Addr=0x06 */
    0x04 + CDK_XGSM_MIIM_IBUS(0), /* Port  6 IntBus=0 Addr=0x04 */
    0x03 + CDK_XGSM_MIIM_IBUS(0), /* Port  7 IntBus=0 Addr=0x03 */
    0x02 + CDK_XGSM_MIIM_IBUS(0), /* Port  8 IntBus=0 Addr=0x02 */
    0x01 + CDK_XGSM_MIIM_IBUS(0), /* Port  9 IntBus=0 Addr=0x01 */
    0x0B + CDK_XGSM_MIIM_IBUS(0), /* Port 10 IntBus=0 Addr=0x0B */
    0x0C + CDK_XGSM_MIIM_IBUS(0), /* Port 11 IntBus=0 Addr=0x0C */
    0x0D + CDK_XGSM_MIIM_IBUS(0), /* Port 12 IntBus=0 Addr=0x0D */
    0x0E + CDK_XGSM_MIIM_IBUS(0), /* Port 13 IntBus=0 Addr=0x0E */
    0x10 + CDK_XGSM_MIIM_IBUS(0), /* Port 14 IntBus=0 Addr=0x10 */
    0x11 + CDK_XGSM_MIIM_IBUS(0), /* Port 15 IntBus=0 Addr=0x11 */
    0x12 + CDK_XGSM_MIIM_IBUS(0), /* Port 16 IntBus=0 Addr=0x12 */
    0x13 + CDK_XGSM_MIIM_IBUS(0), /* Port 17 IntBus=0 Addr=0x13 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 18 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 19 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 20 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 21 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 22 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 23 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 24 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1) /* Port 25 IntBus=1 Addr=0x11 */
};

static const uint16 _phy_addr_bcm5334x[] = {
    0xFF,  /* Port  0 (cmic) N/A */
    0xFF,  /* Port  1        N/A */
    0x09 + CDK_XGSM_MIIM_IBUS(0), /* Port  2 IntBus=0 Addr=0x09 */
    0x08 + CDK_XGSM_MIIM_IBUS(0), /* Port  3 IntBus=0 Addr=0x08 */
    0x07 + CDK_XGSM_MIIM_IBUS(0), /* Port  4 IntBus=0 Addr=0x07 */
    0x06 + CDK_XGSM_MIIM_IBUS(0), /* Port  5 IntBus=0 Addr=0x06 */
    0x04 + CDK_XGSM_MIIM_IBUS(0), /* Port  6 IntBus=0 Addr=0x04 */
    0x03 + CDK_XGSM_MIIM_IBUS(0), /* Port  7 IntBus=0 Addr=0x03 */
    0x02 + CDK_XGSM_MIIM_IBUS(0), /* Port  8 IntBus=0 Addr=0x02 */
    0x01 + CDK_XGSM_MIIM_IBUS(0), /* Port  9 IntBus=0 Addr=0x01 */
    0x0B + CDK_XGSM_MIIM_IBUS(0), /* Port 10 IntBus=0 Addr=0x0B */
    0x0C + CDK_XGSM_MIIM_IBUS(0), /* Port 11 IntBus=0 Addr=0x0C */
    0x0D + CDK_XGSM_MIIM_IBUS(0), /* Port 12 IntBus=0 Addr=0x0D */
    0x0E + CDK_XGSM_MIIM_IBUS(0), /* Port 13 IntBus=0 Addr=0x0E */
    0x10 + CDK_XGSM_MIIM_IBUS(0), /* Port 14 IntBus=0 Addr=0x10 */
    0x11 + CDK_XGSM_MIIM_IBUS(0), /* Port 15 IntBus=0 Addr=0x11 */
    0x12 + CDK_XGSM_MIIM_IBUS(0), /* Port 16 IntBus=0 Addr=0x12 */
    0x13 + CDK_XGSM_MIIM_IBUS(0), /* Port 17 IntBus=0 Addr=0x13 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 18 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 19 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 20 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 21 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 22 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 23 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 24 IntBus=1 Addr=0x11 */
    0x11 + CDK_XGSM_MIIM_IBUS(1), /* Port 25 IntBus=1 Addr=0x11 */
    0x01 + CDK_XGSM_MIIM_IBUS(1), /* Port 26 IntBus=1 Addr=0x01 */
    0x02 + CDK_XGSM_MIIM_IBUS(1), /* Port 27 IntBus=1 Addr=0x02 */
    0x03 + CDK_XGSM_MIIM_IBUS(1), /* Port 28 IntBus=1 Addr=0x03 */
    0x04 + CDK_XGSM_MIIM_IBUS(1), /* Port 29 IntBus=1 Addr=0x04 */
    0x05 + CDK_XGSM_MIIM_IBUS(1), /* Port 30 IntBus=1 Addr=0x05 */
    0x06 + CDK_XGSM_MIIM_IBUS(1), /* Port 31 IntBus=1 Addr=0x06 */
    0x07 + CDK_XGSM_MIIM_IBUS(1), /* Port 32 IntBus=1 Addr=0x07 */
    0x08 + CDK_XGSM_MIIM_IBUS(1)  /* Port 33 IntBus=1 Addr=0x08 */
};

static const uint16 _phy_addr_bcm5339x[] = {
    0xFF,  /* Port  0 (cmic) N/A */
    0xFF,  /* Port  1        N/A */
    0x01 + CDK_XGSM_MIIM_IBUS(0), /* Port  2 IntBus=0 Addr=0x01 */
    0xFF, /* Port  3        N/A */
    0xFF, /* Port  4        N/A */
    0xFF, /* Port  5        N/A */
    0x01 + CDK_XGSM_MIIM_IBUS(0), /* Port  6 IntBus=0 Addr=0x01 */
    0xFF, /* Port  7        N/A */
    0xFF, /* Port  8        N/A */
    0xFF, /* Port  9        N/A */
    0x09 + CDK_XGSM_MIIM_IBUS(0), /* Port 10 IntBus=0 Addr=0x09 */
    0xFF, /* Port  11        N/A */
    0xFF, /* Port  12        N/A */
    0xFF, /* Port  13        N/A */
    0x09 + CDK_XGSM_MIIM_IBUS(0), /* Port 14 IntBus=0 Addr=0x09 */
    0xFF, /* Port  15        N/A */
    0xFF, /* Port  16        N/A */
    0xFF, /* Port  17        N/A */
    0x1F + CDK_XGSM_MIIM_IBUS(1), /* Port 18 IntBus=1 Addr=0x1F */
    0xFF, /* Port  19        N/A */
    0xFF, /* Port  20        N/A */
    0xFF, /* Port  21        N/A */
    0x1F + CDK_XGSM_MIIM_IBUS(1), /* Port 22 IntBus=1 Addr=0x1F */
    0xFF, /* Port  23        N/A */
    0xFF, /* Port  24        N/A */
    0xFF, /* Port  25        N/A */
    0x01 + CDK_XGSM_MIIM_IBUS(1), /* Port 26 IntBus=1 Addr=0x01 */
    0x02 + CDK_XGSM_MIIM_IBUS(1), /* Port 27 IntBus=1 Addr=0x02 */
    0x03 + CDK_XGSM_MIIM_IBUS(1), /* Port 28 IntBus=1 Addr=0x03 */
    0x04 + CDK_XGSM_MIIM_IBUS(1), /* Port 29 IntBus=1 Addr=0x04 */
    0x05 + CDK_XGSM_MIIM_IBUS(1), /* Port 30 IntBus=1 Addr=0x05 */
    0x06 + CDK_XGSM_MIIM_IBUS(1), /* Port 31 IntBus=1 Addr=0x06 */
    0x07 + CDK_XGSM_MIIM_IBUS(1), /* Port 32 IntBus=1 Addr=0x07 */
    0x08 + CDK_XGSM_MIIM_IBUS(1)  /* Port 33 IntBus=1 Addr=0x08 */
};

static uint32_t
_phy_addr(int pport)
{
    if (pport <= BCM5333X_PORT_MAX) {
        switch (hr2_sw_info.devid) {
            case BCM53333_DEVICE_ID:
            case BCM53334_DEVICE_ID:
                if (pport < sizeof(_phy_addr_bcm5333x)) {
                    return _phy_addr_bcm5333x[pport];
                } else {
                    return 0xFF;
                }
                break;
            case BCM53344_DEVICE_ID:
            case BCM53346_DEVICE_ID:
                if (pport < sizeof(_phy_addr_bcm5334x)) {
                    return _phy_addr_bcm5334x[pport];
                } else {
                    return 0xFF;
                }
                break;
            case BCM53393_DEVICE_ID:
            case BCM53394_DEVICE_ID:
                if (pport < sizeof(_phy_addr_bcm5339x)) {
                    return _phy_addr_bcm5339x[pport];
                } else {
                    return 0xFF;
                }
                break;
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
_phy_inst(int pport)
{
    if ((pport >= BCM5333X_PORT_MIN) && (pport <= BCM5333X_PORT_MAX)) {
        switch (hr2_sw_info.devid) {
            case BCM53333_DEVICE_ID:
            case BCM53334_DEVICE_ID:
            case BCM53344_DEVICE_ID:
            case BCM53346_DEVICE_ID:
                /* 56150 type with internal GPHY */
                if (pport < 10) {
                    return 9 - pport;
                } else {
                    return pport - 2;
                }
            break;
            case BCM53393_DEVICE_ID:
            case BCM53394_DEVICE_ID:
                /* 56151 type without internal GPHY */
                return pport - 2;
            break;
            default :
                break;
        }
    }
    /* Should not get here */
    return 0;
}

phy_bus_t phy_bus_bcm56150_miim_int = {
    "bcm56150_miim_int",
    _phy_addr,
    _read,
    _write,
    _phy_inst
};


