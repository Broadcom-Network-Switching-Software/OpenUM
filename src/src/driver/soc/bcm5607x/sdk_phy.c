/*! \file sdk_phy.c
 *
 * SDK PHY interface.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#define __int8_t_defined
#define __uint32_t_defined

#include <system.h>
#include <cmicx_miim.h>

#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>   /* for SOC_E_XXXX */
#include <utils/net.h>   /* for htol32 */

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>
#include <soc/property.h>
#include <soc/phyreg.h>


/*******************************************************************************
 * Internal PHY address
 */

static const uint16 _soc_fl_int_phy_addr[] = {
    0x00, /* Port  0 (cmic)             N/A */
    0x00, /* Port  1 (N/A)              */
    0xc1, /* Port  2 (QTC0-QSGMII)      IntBus=2 Addr=0x01 */
    0xc1, /* Port  3 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port  4 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port  5 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port  6 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port  7 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port  8 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port  9 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 10 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 11 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 12 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 13 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 14 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 15 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 16 (QTC0-QSGMII)      Share the same Addr */
    0xc1, /* Port 17 (QTC0-QSGMII)      Share the same Addr */
    0xc5, /* Port 18 (QTC1-QSGMII)      IntBus=2 Addr=0x5 */
    0xc5, /* Port 19 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 20 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 21 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 22 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 23 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 24 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 25 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 26 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 27 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 28 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 29 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 30 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 31 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 32 (QTC1-QSGMII)      Share the same Addr */
    0xc5, /* Port 33 (QTC1-QSGMII)      Share the same Addr */
    0xc9, /* Port 34 (QTC2-QSGMII)      IntBus=2 Addr=0x9 */
    0xc9, /* Port 35 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 36 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 37 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 38 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 39 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 40 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 41 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 42 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 43 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 44 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 45 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 46 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 47 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 48 (QTC2-QSGMII)      Share the same Addr */
    0xc9, /* Port 49 (QTC2-QSGMII)      Share the same Addr */
    0xc1, /* Port 50 (TSCE0)            IntBus=2 Addr=0x01 */
    0xc1, /* Port 51 (TSCE0)            Share the same Addr */
    0xc1, /* Port 52 (TSCE0)            Share the same Addr */
    0xc1, /* Port 53 (TSCE0)            Share the same Addr */
    0xc5, /* Port 54 (TSCE1)            IntBus=2 Addr=0x05 */
    0xc5, /* Port 55 (TSCE1)            Share the same Addr */
    0xc5, /* Port 56 (TSCE1)            Share the same Addr */
    0xc5, /* Port 57 (TSCE1)            Share the same Addr */
    0xc9, /* Port 58 (TSCE2)            IntBus=2 Addr=0x09 */
    0xc9, /* Port 59 (TSCE2)            Share the same Addr */
    0xc9, /* Port 60 (TSCE2)            Share the same Addr */
    0xc9, /* Port 61 (TSCE2)            Share the same Addr */
    0xa1, /* Port 62 (TSCF0)            IntBus=1 Addr=0x01 */
    0xa1, /* Port 63 (TSCF0)            Share the same Addr */
    0xa1, /* Port 64 (TSCF0)            Share the same Addr */
    0xa1, /* Port 65 (TSCF0)            Share the same Addr */
    0xa5, /* Port 66 (TSCF1)            IntBus=1 Addr=0x05 */
    0xa5, /* Port 67 (TSCF1)            Share the same Addr */
    0xa5, /* Port 68 (TSCF1)            Share the same Addr */
    0xa5, /* Port 69 (TSCF1)            Share the same Addr */
    0xa9, /* Port 70 (TSCF2)            IntBus=1 Addr=0x09 */
    0xa9, /* Port 71 (TSCF2)            Share the same Addr */
    0xa9, /* Port 72 (TSCF2)            Share the same Addr */
    0xa9, /* Port 73 (TSCF2)            Share the same Addr */
    0xad, /* Port 74 (TSCF3)            IntBus=1 Addr=0x0d */
    0xad, /* Port 75 (TSCF3)            Share the same Addr */
    0xad, /* Port 76 (TSCF3)            Share the same Addr */
    0xad, /* Port 77 (TSCF3)            Share the same Addr */
};

void
soc_phy_addr_default(int unit, int lport,
                     uint16 *phy_addr, uint16 *phy_addr_int)
{
    int phy_port = SOC_PORT_L2P_MAPPING(lport);
    *phy_addr = 0xFF;
    *phy_addr_int = _soc_fl_int_phy_addr[phy_port];
}

static int
mdio_addr_to_port(uint32 phy_addr_int)
{
    int bus, mdio_addr;

    /* Must be internal MDIO bus */
    if ((phy_addr_int & 0x80) == 0) {
        return -1;
    }

    bus = PCM_PHY_ID_BUS_NUM(phy_addr_int);
    mdio_addr = phy_addr_int & 0x1f;

    if (bus == 1) {
        switch (mdio_addr) {
        case 0x1:
            return 62;
        case 0x5:
            return 66;
        case 0x9:
            return 70;
        case 0xd:
            return 74;
        default:
            return -1;
        }
    } else if (bus == 2) {
        switch (mdio_addr) {
        case 0x1:
            return 50;
        case 0x5:
            return 54;
        case 0x9:
            return 58;
        default:
            return -1;
        }
    }

    return -1;
}

/*******************************************************************************
 * soc_functions_t driver
 */

#define FW_ALIGN_BYTES  16
#define FW_ALIGN_MASK   (FW_ALIGN_BYTES - 1)

static int
tsce_firmware_helper(int unit, int port, uint8 *fw_data, int fw_size)
{
    int ioerr = 0;
    XLPORT_WC_UCMEM_CTRLr_t ucmem_ctrl;
    XLPORT_WC_UCMEM_DATAm_t ucmem_data;
    uint8_t buf[FW_ALIGN_BYTES];
    const uint8_t *data;
    uint32_t val;
    uint32_t size, idx, wdx, bdx;
    uint32_t ucmem_idx, ucmem_idx_max = XLPORT_WC_UCMEM_DATAm_MAX;

    if (fw_size == 0) {
        return SOC_E_PARAM;
    }

    /* Aligned firmware size */
    size = (fw_size + FW_ALIGN_MASK) & ~FW_ALIGN_MASK;

    /* Enable parallel bus access */
    ioerr += READ_XLPORT_WC_UCMEM_CTRLr(unit, port, ucmem_ctrl);
    XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 1);
    ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, port, ucmem_ctrl);

    for (idx = 0; idx < size; idx += 16) {
        if (idx + 15 < fw_size) {
            data = fw_data + idx;
        } else {
            /* Use staging buffer for modulo bytes */
            sal_memset(buf, 0, sizeof(buf));
            sal_memcpy(buf, fw_data + idx, FW_ALIGN_BYTES - (size - fw_size));
            data = buf;
        }
        for (wdx = 0; wdx < 4; wdx++) {
            val = 0;
            for (bdx = 0; bdx < 4; bdx++) {
                val |= data[bdx + wdx * 4] << (bdx * 8);
            }
            XLPORT_WC_UCMEM_DATAm_SET(ucmem_data, wdx, val);
        }
        ucmem_idx = idx >> 4;
        if (ucmem_idx > ucmem_idx_max) {
            ucmem_idx = ucmem_idx % ucmem_idx_max;
        }
        ioerr += WRITE_XLPORT_WC_UCMEM_DATAm(unit, port, ucmem_idx,
                                             ucmem_data);
    }

    /* Disable parallel bus access */
    XLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 0);
    ioerr += WRITE_XLPORT_WC_UCMEM_CTRLr(unit, port, ucmem_ctrl);

    return ioerr ? SOC_E_FAIL : SOC_E_NONE;
}

static int
tscf_firmware_helper(int unit, int port, uint8 *fw_data, int fw_size)
{
    int ioerr = 0;
    CLPORT_WC_UCMEM_CTRLr_t ucmem_ctrl;
    CLPORT_WC_UCMEM_DATAm_t ucmem_data;
    uint8_t buf[FW_ALIGN_BYTES];
    const uint8_t *data;
    uint32_t val;
    uint32_t size, idx, wdx, bdx;
    uint32_t ucmem_idx, ucmem_idx_max = CLPORT_WC_UCMEM_DATAm_MAX;

    if (fw_size == 0) {
        return SOC_E_PARAM;
    }

    /* Aligned firmware size */
    size = (fw_size + FW_ALIGN_MASK) & ~FW_ALIGN_MASK;

    /* Enable parallel bus access */
    ioerr += READ_CLPORT_WC_UCMEM_CTRLr(unit, port, ucmem_ctrl);
    CLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 1);
    ioerr += WRITE_CLPORT_WC_UCMEM_CTRLr(unit, port, ucmem_ctrl);

    for (idx = 0; idx < size; idx += 16) {
        if (idx + 15 < fw_size) {
            data = fw_data + idx;
        } else {
            /* Use staging buffer for modulo bytes */
            sal_memset(buf, 0, sizeof(buf));
            sal_memcpy(buf, fw_data + idx, FW_ALIGN_BYTES - (size - fw_size));
            data = buf;
        }
        for (wdx = 0; wdx < 4; wdx++) {
            val = 0;
            for (bdx = 0; bdx < 4; bdx++) {
                val |= data[bdx + wdx * 4] << (bdx * 8);
            }
            CLPORT_WC_UCMEM_DATAm_SET(ucmem_data, wdx, val);
        }
        ucmem_idx = idx >> 4;
        if (ucmem_idx > ucmem_idx_max) {
            ucmem_idx = ucmem_idx % ucmem_idx_max;
        }
        ioerr += WRITE_CLPORT_WC_UCMEM_DATAm(unit, port, ucmem_idx,
                                             ucmem_data);
    }

    /* Disable parallel bus access */
    CLPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(ucmem_ctrl, 0);
    ioerr += WRITE_CLPORT_WC_UCMEM_CTRLr(unit, port, ucmem_ctrl);

    return ioerr ? SOC_E_FAIL : SOC_E_NONE;
}

static int
bcm95607x_firmware_set(int unit, int port, uint8 *data, int size)
{
    int blk_pport, blk_lport;

    blk_pport = SOC_PORT_L2P_MAPPING(port) - SOC_PORT_BLOCK_INDEX(port);
    blk_lport = SOC_PORT_P2L_MAPPING(blk_pport);

    if (IS_CL_PORT(blk_lport)) {
        return tscf_firmware_helper(unit, blk_lport, data, size);
    } else if (IS_XL_PORT(blk_lport)) {
        return tsce_firmware_helper(unit, blk_lport, data, size);
    }

    return SOC_E_FAIL;
}

#define TSC_REG_ADDR_TSCID_SET(_phy_reg, _phyad)    \
            ((_phy_reg) |= ((_phyad) & 0x1f) << 19)

static int
tsce_reg_read(int unit, uint32_t phy_addr,
              uint32_t reg_addr, uint32_t *val)
{
    int rv, idx, lport, pport;
    uint32_t data[16];
    XLPORT_WC_UCMEM_DATAm_t ucmem_data;

    pport = mdio_addr_to_port(phy_addr);
    if (pport < 0) {
        return SOC_E_PARAM;
    }
    lport = SOC_PORT_P2L_MAPPING(pport);

    TSC_REG_ADDR_TSCID_SET(reg_addr, phy_addr);

    sal_memset(data, 0, sizeof(data));
    data[0] = reg_addr & 0xffffffff;
    data[2] = 0; /* for TSC register READ */

    for (idx = 0; idx < 4; idx++) {
        XLPORT_WC_UCMEM_DATAm_SET(ucmem_data, idx, data[idx]);
    }
    rv = WRITE_XLPORT_WC_UCMEM_DATAm(unit, lport, 0, ucmem_data);

    if (rv == SYS_OK) {
        rv = READ_XLPORT_WC_UCMEM_DATAm(unit, lport, 0, ucmem_data);
    }

    /*
     * The read data returned will be stored in bits [47:32] of
     * XLPORT_WC_UCMEM_DATA.
     */
    *val = XLPORT_WC_UCMEM_DATAm_GET(ucmem_data, 1);

    return rv == SYS_OK ? SOC_E_NONE : SOC_E_FAIL;
}

static int
tsce_reg_write(int unit, uint32_t phy_addr,
               uint32_t reg_addr, uint32_t val)
{
    int rv, idx, lport, pport;
    uint32_t data[16];
    XLPORT_WC_UCMEM_DATAm_t ucmem_data;

    pport = mdio_addr_to_port(phy_addr);
    if (pport < 0) {
        return SOC_E_PARAM;
    }
    lport = SOC_PORT_P2L_MAPPING(pport);

    TSC_REG_ADDR_TSCID_SET(reg_addr, phy_addr);

    if ((val & 0xffff0000) == 0) {
        val |= 0xffff0000;
    }

    sal_memset(data, 0, sizeof(data));
    data[0] = reg_addr & 0xffffffff;
    data[1] = ((val & 0xffff) << 16) |
              ((~val & 0xffff0000) >> 16);
    data[2] = 1; /* for TSC register write */

    for (idx = 0; idx < 4; idx++) {
        XLPORT_WC_UCMEM_DATAm_SET(ucmem_data, idx, data[idx]);
    }
    rv = WRITE_XLPORT_WC_UCMEM_DATAm(unit, lport, 0, ucmem_data);

    return rv == SYS_OK ? SOC_E_NONE : SOC_E_FAIL;
}

static int
tscf_reg_read(int unit, uint32_t phy_addr,
              uint32_t reg_addr, uint32_t *val)
{
    int rv, idx, lport, pport;
    uint32_t data[16];
    CLPORT_WC_UCMEM_DATAm_t ucmem_data;

    pport = mdio_addr_to_port(phy_addr);
    if (pport < 0) {
        return SOC_E_PARAM;
    }
    lport = SOC_PORT_P2L_MAPPING(pport);

    TSC_REG_ADDR_TSCID_SET(reg_addr, phy_addr);

    sal_memset(data, 0, sizeof(data));
    data[0] = reg_addr & 0xffffffff;
    data[2] = 0; /* for TSC register READ */

    for (idx = 0; idx < 4; idx++) {
        CLPORT_WC_UCMEM_DATAm_SET(ucmem_data, idx, data[idx]);
    }
    rv = WRITE_CLPORT_WC_UCMEM_DATAm(unit, lport, 0, ucmem_data);

    if (rv == SYS_OK) {
        rv = READ_CLPORT_WC_UCMEM_DATAm(unit, lport, 0, ucmem_data);
    }

    /*
     * The read data returned will be stored in bits [47:32] of
     * CLPORT_WC_UCMEM_DATA.
     */
    *val = CLPORT_WC_UCMEM_DATAm_GET(ucmem_data, 1);

    return rv == SYS_OK ? SOC_E_NONE : SOC_E_FAIL;
}

static int
tscf_reg_write(int unit, uint32_t phy_addr,
               uint32_t reg_addr, uint32_t val)
{
    int rv, idx, lport, pport;
    uint32_t data[16];
    CLPORT_WC_UCMEM_DATAm_t ucmem_data;

    pport = mdio_addr_to_port(phy_addr);
    if (pport < 0) {
        return SOC_E_PARAM;
    }
    lport = SOC_PORT_P2L_MAPPING(pport);

    TSC_REG_ADDR_TSCID_SET(reg_addr, phy_addr);

    if ((val & 0xffff0000) == 0) {
        val |= 0xffff0000;
    }

    sal_memset(data, 0, sizeof(data));
    data[0] = reg_addr & 0xffffffff;
    data[1] = ((val & 0xffff) << 16) |
              ((~val & 0xffff0000) >> 16);
    data[2] = 1; /* for TSC register write */

    for (idx = 0; idx < 4; idx++) {
        CLPORT_WC_UCMEM_DATAm_SET(ucmem_data, idx, data[idx]);
    }
    rv = WRITE_CLPORT_WC_UCMEM_DATAm(unit, lport, 0, ucmem_data);

    return rv == SYS_OK ? SOC_E_NONE : SOC_E_FAIL;
}

static int
bcm95607x_tsc_reg_read(int unit, uint32 phy_addr,
                       uint32 phy_reg, uint32_t *phy_data)
{
    int lport, pport;

    pport = mdio_addr_to_port(phy_addr);
    if (pport < 0) {
        return SOC_E_PARAM;
    }
    lport = SOC_PORT_P2L_MAPPING(pport);

    if (IS_CL_PORT(lport)) {
        return tscf_reg_read(unit, phy_addr, phy_reg, phy_data);
    } else if (IS_XL_PORT(lport)) {
        return tsce_reg_read(unit, phy_addr, phy_reg, phy_data);
    }

    return SOC_E_FAIL;
}

static int
bcm95607x_tsc_reg_write(int unit, uint32 phy_addr,
                        uint32 phy_reg, uint32_t phy_data)
{
    int lport, pport;

    pport = mdio_addr_to_port(phy_addr);
    if (pport < 0) {
        return SOC_E_PARAM;
    }
    lport = SOC_PORT_P2L_MAPPING(pport);

    if (IS_CL_PORT(lport)) {
        return tscf_reg_write(unit, phy_addr, phy_reg, phy_data);
    } else if (IS_XL_PORT(lport)) {
        return tsce_reg_write(unit, phy_addr, phy_reg, phy_data);
    }

    return SOC_E_FAIL;
}

static soc_functions_t bcm95607x_soc_funs = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    bcm95607x_firmware_set,
    bcm95607x_tsc_reg_read,
    bcm95607x_tsc_reg_write,
    NULL
};

soc_functions_t *
soc_chip_drv_funs_find(uint16 dev_id, uint8 rev_id)
{
    return &bcm95607x_soc_funs;
}

/*******************************************************************************
 * SoC CM driver
 */

/*
 * Function:    soc_cm_get_name
 * Purpose:     returns the symbolic name of the device.
 *
 * Parameters:  dev - device handle
 *
 * Returns:     symbolic name of this device.
 *
 * Note:        The return value is symbolic name of this device.
 *              String values are valid, and should not be interpreted as error.
 *              Negative values indicate an error.
 */
const char *
soc_cm_get_name(int dev)
{
   return "FIRELIGHT";
}

const char *
soc_cm_config_var_get(int dev, const char *name)
{
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
     return sal_config_get(name);
#else
     return NULL;
#endif
}

int
soc_cm_get_id(int unit, uint16 *dev_id, uint8 *rev_id) {

    uint16 devid, rev;

    bcm5607x_chip_revision(unit, &devid, &rev);
    *dev_id = devid;
    *rev_id = rev;

    return SOC_E_NONE;
}

/*******************************************************************************
 * MIIM driver
 */

/*
 * Function:
 *      soc_miim_write
 * Purpose:
 *      Write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */
int
soc_miim_write(int unit, uint16 phy_id,
               uint8 phy_reg_addr, uint16 phy_wr_data)
{
    uint32_t data = phy_wr_data;
    cmicx_miim_op_t miim_op, *op = &miim_op;

    sal_memset(op, 0, sizeof(*op));

    op->opcode = CMICX_MIIM_OPC_CL22_WRITE;
    op->internal = PCM_PHY_ID_IS_INTERNAL_BUS(phy_id);
    op->busno = PCM_PHY_ID_BUS_NUM(phy_id);
    op->phyad = PCM_PHY_ID_PHY_ADDR(phy_id);
    op->regad = phy_reg_addr & 0x1f;

    return cmicx_miim_op(unit, op, &data);
}

/*
 * Function:
 *      soc_miim_read
 * Purpose:
 *      Read a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */
int
soc_miim_read(int unit, uint16 phy_id,
              uint8 phy_reg_addr, uint16 *phy_rd_data)
{
    int rv;
    uint32_t data = 0;
    cmicx_miim_op_t miim_op, *op = &miim_op;

    sal_memset(op, 0, sizeof(*op));

    op->opcode = CMICX_MIIM_OPC_CL22_READ;
    op->internal = PCM_PHY_ID_IS_INTERNAL_BUS(phy_id);
    op->busno = PCM_PHY_ID_BUS_NUM(phy_id);
    op->phyad = PCM_PHY_ID_PHY_ADDR(phy_id);
    op->regad = phy_reg_addr & 0x1f;

    rv = cmicx_miim_op(unit, op, &data);
    *phy_rd_data = data & 0xffff;

    return rv;
}

/*
 * Function:
 *      soc_esw_miim_write
 * Purpose:
 *      New interface to write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 * Added to have the same interface for PHY register access among
 * ESW, ROBO, SBX
 */
int
soc_esw_miim_write(int unit, uint32 phy_id,
                   uint32 phy_reg_addr, uint16 phy_wr_data)
{
    return soc_miim_write(unit, (uint16)phy_id,
                          (uint16)phy_reg_addr, phy_wr_data);
}

/*
 * Function:
 *      soc_esw_miim_read
 * Purpose:
 *      New interface to read a value from a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - Data read.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 * Added to have the same interface for PHY register access among
 * ESW, ROBO, SBX.
 */
int
soc_esw_miim_read(int unit, uint32 phy_id,
                  uint32 phy_reg_addr, uint16 *phy_rd_data)
{
    return soc_miim_read(unit, (uint16)phy_id,
                         (uint16)phy_reg_addr, phy_rd_data);
}

/*
 * Function:
 *      soc_miimc45_write
 * Purpose:
 *      Write a value to a MIIM register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_wr_data - Data to write.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously. This
 *      is a wrapper around __soc_miimc45_write that should be used for
 *      oridinary clause 45 writes that are disabled during warmboot.
 */
int
soc_miimc45_write(int unit, uint16 phy_id, uint8 phy_devad,
                  uint16 phy_reg_addr, uint16 phy_wr_data)
{
    uint32_t data = phy_wr_data;
    cmicx_miim_op_t miim_op, *op = &miim_op;

    sal_memset(op, 0, sizeof(*op));

    op->opcode = CMICX_MIIM_OPC_CL45_WRITE;
    op->internal = PCM_PHY_ID_IS_INTERNAL_BUS(phy_id);
    op->busno = PCM_PHY_ID_BUS_NUM(phy_id);
    op->phyad = PCM_PHY_ID_PHY_ADDR(phy_id);
    op->regad = phy_reg_addr;
    op->devad = phy_devad;

    return cmicx_miim_op(unit, op, &data);
}

/*
 * Function:
 *      soc_miimc45_read
 * Purpose:
 *      Read a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_reg_addr - PHY register to write
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Temporarily disables auto link scan if it was enabled.  The MIIM
 *      registers are locked during the operation to prevent multiple
 *      tasks from trying to access PHY registers simultaneously.
 */

int
soc_miimc45_read(int unit, uint16 phy_id, uint8 phy_devad,
                 uint16 phy_reg_addr, uint16 *phy_rd_data)
{
    int rv;
    uint32_t data = 0;
    cmicx_miim_op_t miim_op, *op = &miim_op;

    sal_memset(op, 0, sizeof(*op));

    op->opcode = CMICX_MIIM_OPC_CL45_READ;
    op->internal = PCM_PHY_ID_IS_INTERNAL_BUS(phy_id);
    op->busno = PCM_PHY_ID_BUS_NUM(phy_id);
    op->phyad = PCM_PHY_ID_PHY_ADDR(phy_id);
    op->regad = phy_reg_addr;
    op->devad = phy_devad;

    rv = cmicx_miim_op(unit, op, &data);
    *phy_rd_data = data & 0xffff;

    return rv;
}

/*
 * Function:
 *      soc_esw_miimc45_write
 * Purpose:
 *      Write a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write (Encoded with PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_write(int unit, uint32 phy_id,
                      uint32 phy_reg_addr, uint16 phy_wr_data)
{
    uint8 dev_addr;
    uint16 reg_addr;

    dev_addr = SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr);
    reg_addr = SOC_PHY_CLAUSE45_REGAD(phy_reg_addr);

    return soc_miimc45_write(unit, (uint16)phy_id, dev_addr,
                             reg_addr, phy_wr_data);
}

/*
 * Function:
 *      soc_esw_miimc45_read
 * Purpose:
 *      Read a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write (Encoded with PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_read(int unit, uint32 phy_id,
                     uint32 phy_reg_addr, uint16 *phy_rd_data)
{
    uint8 dev_addr;
    uint16 reg_addr;

    dev_addr = SOC_PHY_CLAUSE45_DEVAD(phy_reg_addr);
    reg_addr = SOC_PHY_CLAUSE45_REGAD(phy_reg_addr);

    return soc_miimc45_read(unit, (uint16)phy_id, dev_addr,
                            reg_addr, phy_rd_data);
}

/*
 * Function:
 *      soc_esw_miimc45_data_write
 * Purpose:
 *      Write a value to the present MII register without a address cycle.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_wr_data - 16 bit data for write
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_data_write(int unit, uint32 phy_id,
                           uint8 phy_devad, uint16 phy_wr_data)
{
    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *      soc_esw_miimc45_data_read
 * Purpose:
 *      Read a value from the present MII register without a address cycle.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - point to 16 bit data buffer
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_data_read(int unit, uint32 phy_id,
                          uint8 phy_devad, uint16 *phy_rd_data)
{
    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *      soc_esw_miimc45_addr_write
 * Purpose:
 *      Write the address register with a new address to access.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_devad - Device type (PMA/PMD, PCS, PHY XS)
 *      phy_rd_data - 16 bit register address
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
int
soc_esw_miimc45_addr_write(int unit, uint32 phy_id,
                           uint8 phy_devad, uint16 phy_ad_data)
{
    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *      soc_esw_miimc45_wb_write
 * Purpose:
 *      Write a value from an MII register.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      phy_id - Phy ID to write (MIIM address)
 *      phy_reg_addr - PHY register to write (Encoded with PMA/PMD, PCS, PHY XS)
 *      phy_wr_data - 16bit data to write into
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      This is a wrapper around __soc_miimc45_write that should be used for
 *      clause 45 writes that should succeed during warmboot.
 */
int
soc_esw_miimc45_wb_write(int unit, uint32 phy_id,
                     uint32 phy_reg_addr, uint16 phy_wr_data)
{
    return SOC_E_UNAVAIL;
}

/*******************************************************************************
 * MAC functions
 */

int
soc_mac_probe(int unit, uint8 port, mac_driver_t **macdp)
{
    if (IS_CL_PORT(port)) {
        *macdp = &soc_mac_cl;
    } else if (IS_XL_PORT(port)) {
        *macdp = &soc_mac_xl;
    } else {
        return SOC_E_FAIL;
    }

    return SOC_E_NONE;
}

/*******************************************************************************
 * Misc
 */

int
soc_firelight_sbus_tsc_block(int unit, int pport, int *blk)
{
    if (pport < 50 || pport > 77) {
        return SOC_E_PORT;
    }

    return SOC_E_NONE;
}

/*
 * This function is used to pass the compiling only.
 *
 * Firelight would define both BCM_FIRELIGHT_SUPPORT and BCM_GREYHOUND2_SUPPORT.
 * soc_greyhound2_sbus_tsc_block is required by phyctrl.c when
 * BCM_GREYHOUND2_SUPPORT is defined.
 */
int
soc_greyhound2_sbus_tsc_block(int unit, int pport, int *blk)
{
    return SOC_E_NONE;
}

int
soc_counter_port_pbmp_add(int unit, int port)
{
    return SOC_E_NONE;
}

int
soc_link_mask2_set(int unit, pbmp_t mask)
{
    return 0;
}

int
soc_link_mask2_get(int unit, pbmp_t *mask)
{
    return 0;
}

int
_bcm_esw_link_force(int unit, uint32 flags, int port,
                    int force, int link)
{
    return 0;
}
