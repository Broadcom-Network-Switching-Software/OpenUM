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
#include <utils/net.h>


extern uint8 qtc_interface;
extern uint8 tsce_interface;

pbmp_t phy_external_mode = {{0}};

#define CDK_CONFIG_MIIM_MAX_POLLS   100000
#define MIIM_PARAM_ID_OFFSET 		16
#define MIIM_PARAM_REG_ADDR_OFFSET	24

extern phy_bus_t phy_bus_bcm5354x_miim_int;
extern phy_bus_t phy_bus_bcm95354xk_miim_ext;
extern phy_driver_t bcm54282_drv;
extern phy_driver_t bcm56160_drv;
extern phy_driver_t bcmi_qtce_xgxs_drv;
extern phy_driver_t bcmi_tsce_xgxs_drv;

static phy_bus_t *bcm5354x_phy_bus[] = {
    &phy_bus_bcm5354x_miim_int,
    &phy_bus_bcm95354xk_miim_ext,
    NULL
};

phy_driver_t *bmd_ext_phy_drv_list[] = {
#ifdef INCLUDE_PHY_56160
    &bcm56160_drv,
#endif
#ifdef INCLUDE_PHY_54282
    &bcm54282_drv,
#endif
    NULL
};

phy_driver_t *bmd_int_phy_drv_list[] = {
#ifdef INCLUDE_PHY_QTCE
    &bcmi_qtce_xgxs_drv,
#endif
#ifdef INCLUDE_PHY_TSCE
    &bcmi_tsce_xgxs_drv,
#endif
    NULL
};

bmd_phy_info_t bmd_phy_info[BOARD_NUM_OF_UNITS];

#define PHY_CTRL_NUM_MAX  (BCM5354X_LPORT_MAX*2)
/* 
 * We do not want to rely on dynamic memory allocation,
 * so we allocate phy_ctrl blocks from a static pool.
 * The 'bus' member of the structure indicates whether
 * the block is free or in use.
 */
static phy_ctrl_t _phy_ctrl[PHY_CTRL_NUM_MAX];

int (*phy_reset_cb)(phy_ctrl_t *pc);
int (*phy_init_cb)(phy_ctrl_t *pc);

/*
 * Register PHY init callback function
 */
int 
bmd_phy_init_cb_register(int (*init_cb)(phy_ctrl_t *pc))
{
    phy_init_cb = init_cb;

    return CDK_E_NONE;
}

/*
 * Register PHY reset callback function
 */
int 
bmd_phy_reset_cb_register(int (*reset_cb)(phy_ctrl_t *pc))
{
    phy_reset_cb = reset_cb;

    return CDK_E_NONE;
}

int
bmd_phy_add(int unit, int lport, phy_ctrl_t *pc)
{
    pc->next = BMD_PORT_PHY_CTRL(unit, lport);
    BMD_PORT_PHY_CTRL(unit, lport) = pc;
    return CDK_E_NONE;
}

static phy_ctrl_t *
phy_ctrl_alloc(void)
{
    int idx;
    phy_ctrl_t *pc;

    for (idx = 0, pc = &_phy_ctrl[0]; idx < PHY_CTRL_NUM_MAX; idx++, pc++) {
        if (pc->bus == 0) {
            return pc;
        }
    }
    return NULL;
}

static void
phy_ctrl_free(phy_ctrl_t *pc)
{
    pc->bus = 0;
}


phy_ctrl_t *
bmd_phy_del(int unit, int lport)
{
    phy_ctrl_t *pc;

    if ((pc = BMD_PORT_PHY_CTRL(unit, lport)) != 0) {
        BMD_PORT_PHY_CTRL(unit, lport) = pc->next;
        phy_ctrl_free(pc);
    }
    return pc;
}


/*
 * Probe all PHY buses associated with BMD device
 */
int 
bmd_phy_probe_default(int unit, int lport, phy_driver_t **phy_drv)
{
    phy_bus_t **bus;
    phy_driver_t **drv;
    phy_ctrl_t pc_probe;
    phy_ctrl_t *pc;
    int rv; 
    int found = 0;

    /* Bail if not PHY driver list is provided */
    if (phy_drv == NULL) {
        return CDK_E_NONE;
    }

    /* Check that we have PHY bus list */
    bus = BMD_PORT_PHY_BUS(unit, lport);
    if (bus == NULL) {
        return CDK_E_CONFIG;
    }

    /* Loop over PHY buses for this lport */
    while (*bus != NULL) {
        drv = phy_drv;
        /* Probe all PHY drivers on this bus */
        while (*drv != NULL) {
            /* Initialize PHY control used for probing */
            pc = &pc_probe;
            sal_memset(pc, 0, sizeof(*pc));
            pc->unit = unit;
            pc->port = SOC_PORT_L2P_MAPPING(lport);
            pc->bus = *bus;
            pc->drv = *drv;
            pc->lane_num = SOC_PORT_LANE_NUMBER(lport);
            pc->blk_id = SOC_PORT_BLOCK(lport);
            pc->blk_idx = SOC_PORT_BLOCK_INDEX(lport);
            if (CDK_SUCCESS(PHY_PROBE(pc))) {
                /* Found known PHY on bus */
                pc = phy_ctrl_alloc();
                if (pc == NULL) {
                    return CDK_E_MEMORY;
                }
                sal_memcpy(pc, &pc_probe, sizeof(*pc));
                /* Install PHY */
                rv = bmd_phy_add(unit, lport, pc);
                if (CDK_FAILURE(rv)) {
                    return rv;
                }
                found = 1;
                /* Move to next bus */
                break;
            }
            drv++;
        }
        bus++;
    }
    if (!found) {
        return CDK_E_NOT_FOUND;
    }
    return CDK_E_NONE;
}


int
cdk_xgsm_miim_read(int unit, uint32_t phy_addr, uint32_t reg, uint32_t *val)
{
    int rv = CDK_E_NONE; 

    uint32 polls; 
    CMIC_CMC0_MIIM_PARAMr_t cmic_cmc1_miim_param;
    CMIC_CMC0_MIIM_ADDRESSr_t cmic_cmc1_miim_address;
    CMIC_CMC0_MIIM_CTRLr_t cmic_cmc1_miim_ctrl;
    CMIC_CMC0_MIIM_STATr_t cmic_cmc1_miim_stat;
    CMIC_CMC0_MIIM_READ_DATAr_t cmic_cmc1_miim_read_data;

    MIIM_LOCK(unit);
    /*
     * Use clause 45 access if DEVAD specified.
     * Note that DEVAD 32 (0x20) can be used to access special DEVAD 0.
     */
    /* Write parameter registers */
    CMIC_CMC0_MIIM_PARAMr_SET(cmic_cmc1_miim_param, (phy_addr << MIIM_PARAM_ID_OFFSET));
    if (reg & 0x003f0000) {
        CMIC_CMC0_MIIM_PARAMr_C45_SELf_SET(cmic_cmc1_miim_param, 1);
        reg &= 0x001fffff;
    }
	WRITE_CMIC_CMC0_MIIM_PARAMr(unit, cmic_cmc1_miim_param);

    /* Write address registers */
    CMIC_CMC0_MIIM_ADDRESSr_CLR(cmic_cmc1_miim_address);
    CMIC_CMC0_MIIM_ADDRESSr_CLAUSE_45_REGADRf_SET(cmic_cmc1_miim_address, reg);
    CMIC_CMC0_MIIM_ADDRESSr_CLAUSE_45_DTYPEf_SET(cmic_cmc1_miim_address, (reg >> 16));	
    WRITE_CMIC_CMC0_MIIM_ADDRESSr(unit, cmic_cmc1_miim_address);


    /* Tell CMIC to start */
	CMIC_CMC0_MIIM_CTRLr_CLR(cmic_cmc1_miim_ctrl);
	CMIC_CMC0_MIIM_CTRLr_MIIM_RD_STARTf_SET(cmic_cmc1_miim_ctrl, 1);
	WRITE_CMIC_CMC0_MIIM_CTRLr(unit, cmic_cmc1_miim_ctrl);

    /* Poll for completion */
    for (polls = 0; polls < CDK_CONFIG_MIIM_MAX_POLLS; polls++) {
        READ_CMIC_CMC0_MIIM_STATr(unit, cmic_cmc1_miim_stat);
        if (CMIC_CMC0_MIIM_STATr_MIIM_OPN_DONEf_GET(cmic_cmc1_miim_stat)) {
            break; 
	    }
    }
    
    /* Check for timeout and error conditions */
    if (polls == CDK_CONFIG_MIIM_MAX_POLLS) {
        rv = -1; 
        CDK_DEBUG_MIIM
            (("cdk_xgsm_miim_read[%d]: Timeout at phy_addr=0x%08x"
              "reg_addr=%08x\n",
              unit, phy_addr, reg));
    }

	CMIC_CMC0_MIIM_CTRLr_CLR(cmic_cmc1_miim_ctrl);
	WRITE_CMIC_CMC0_MIIM_CTRLr(unit, cmic_cmc1_miim_ctrl);

    if (rv == CDK_E_NONE) {
        READ_CMIC_CMC0_MIIM_READ_DATAr(unit, cmic_cmc1_miim_read_data);
        *val = CMIC_CMC0_MIIM_READ_DATAr_DATAf_GET(cmic_cmc1_miim_read_data);
        CDK_DEBUG_MIIM
            (("cdk_xgsm_miim_read[%d]: phy_addr=0x%08x"
              "reg_addr=%08x data: 0x%08x\n",
              unit, phy_addr, reg, *val));
    }
    MIIM_UNLOCK(unit);
	
    return rv;
}

int 
cdk_xgsm_miim_write(int unit, uint32_t phy_addr, uint32_t reg, uint32_t val)
{
    int rv = CDK_E_NONE; 
    uint32 polls; 

	CMIC_CMC0_MIIM_PARAMr_t cmic_cmc1_miim_param;
	CMIC_CMC0_MIIM_ADDRESSr_t cmic_cmc1_miim_address;
    CMIC_CMC0_MIIM_CTRLr_t cmic_cmc1_miim_ctrl;
	CMIC_CMC0_MIIM_STATr_t cmic_cmc1_miim_stat;	


    MIIM_LOCK(unit);
    /*
     * Use clause 45 access if DEVAD specified.
     * Note that DEVAD 32 (0x20) can be used to access special DEVAD 0.
     */
    /* Write parameter registers */
    CMIC_CMC0_MIIM_PARAMr_SET(cmic_cmc1_miim_param, (phy_addr << MIIM_PARAM_ID_OFFSET) | val);
    if (reg & 0x003f0000) {
        CMIC_CMC0_MIIM_PARAMr_C45_SELf_SET(cmic_cmc1_miim_param, 1);
        reg &= 0x001fffff;
    }
	WRITE_CMIC_CMC0_MIIM_PARAMr(unit, cmic_cmc1_miim_param);


    CMIC_CMC0_MIIM_ADDRESSr_CLR(cmic_cmc1_miim_address);
	CMIC_CMC0_MIIM_ADDRESSr_CLAUSE_45_REGADRf_SET(cmic_cmc1_miim_address, reg);
	CMIC_CMC0_MIIM_ADDRESSr_CLAUSE_45_DTYPEf_SET(cmic_cmc1_miim_address, (reg >> 16));	
    WRITE_CMIC_CMC0_MIIM_ADDRESSr(unit, cmic_cmc1_miim_address);


    /* Tell CMIC to start */
	CMIC_CMC0_MIIM_CTRLr_CLR(cmic_cmc1_miim_ctrl);
	CMIC_CMC0_MIIM_CTRLr_MIIM_WR_STARTf_SET(cmic_cmc1_miim_ctrl, 1);
	WRITE_CMIC_CMC0_MIIM_CTRLr(unit, cmic_cmc1_miim_ctrl);


    /* Poll for completion */
    for (polls = 0; polls < CDK_CONFIG_MIIM_MAX_POLLS; polls++) {
        READ_CMIC_CMC0_MIIM_STATr(unit, cmic_cmc1_miim_stat);
        if (CMIC_CMC0_MIIM_STATr_MIIM_OPN_DONEf_GET(cmic_cmc1_miim_stat)) {
            break; 
	    }
    }

    /* Check for timeout and error conditions */
    if (polls == CDK_CONFIG_MIIM_MAX_POLLS) {
        rv = -1; 
        CDK_DEBUG_MIIM
            (("cdk_xgsm_miim_read[%d]: Timeout at phy_addr=0x%08x"
              "reg_addr=%08x\n",
              unit, phy_addr, reg));
    }

	CMIC_CMC0_MIIM_CTRLr_CLR(cmic_cmc1_miim_ctrl);
	WRITE_CMIC_CMC0_MIIM_CTRLr(unit, cmic_cmc1_miim_ctrl);

    MIIM_UNLOCK(unit);
    return rv;
}

/***********************************************************************
 *
 * HELPER FUNCTIONS FOR SBUS MDIO ACCESS
 */
static int _mdio_addr_to_port(uint32 phy_addr){
	if ((phy_addr & 0xF) == 0xd) {
		return PHY_GPORT2_BASE;
	} else {
     	return PHY_XLPORT0_BASE;	
	}
}
int
cdk_xgsm_sbus_read(int unit, uint32_t phy_addr, 
                         uint32_t phy_reg, uint32_t *phy_data)
{
    int rv = CDK_E_NONE;
    uint32_t xlport_wc_ucmem_data[4];
    uint8_t block_id;
    uint32_t addr;
    SAL_DEBUGF(("soc_sbus_tsc_reg_read (%d,%d,%d,0x%x,0x%08x,*phy_data)..\n", unit, 10, 14, phy_addr, phy_reg));

    MIIM_SCHAN_LOCK(unit);

    /* TSCE sbus access */
    xlport_wc_ucmem_data[0] = (phy_reg | ((phy_addr & 0x1f) << 19)) & 0xffffffff;
    xlport_wc_ucmem_data[1] = 0x0;
    xlport_wc_ucmem_data[2] = 0x0;
    xlport_wc_ucmem_data[3] = 0x0;


    if (_mdio_addr_to_port(phy_addr) == PHY_GPORT2_BASE) {
		addr = M_GPORT_WC_UCMEM_DATA(0);
		block_id = PMQ1_BLOCK_ID;
    } else {
		addr = M_XLPORT_WC_UCMEM_DATA(0);
		block_id = XLPORT0_BLOCK_ID;
    }



    SAL_DEBUGF(("ucmem_data_entry[95:64-63:32-31:0]= 0x%08x-0x%08x-0x%08x\n", xlport_wc_ucmem_data[0], xlport_wc_ucmem_data[1], xlport_wc_ucmem_data[2]));

    rv = bcm5354x_mem_set(unit, block_id, addr, xlport_wc_ucmem_data, 4);

    if (CDK_SUCCESS(rv)) {
        rv = bcm5354x_mem_get(unit, block_id, addr, xlport_wc_ucmem_data, 4);            
    }
    *phy_data = xlport_wc_ucmem_data[1];

    SAL_DEBUGF(("soc_sbus_tsc_reg_read: *phy_data=0x%04x,rv=%d\n", *phy_data, rv));

    MIIM_SCHAN_UNLOCK(unit);
    return rv;
}


int
cdk_xgsm_sbus_write(int unit, uint32_t phy_addr,
                          uint32_t phy_reg, uint32_t phy_data)
{
    int rv = CDK_E_NONE;
    uint32_t xlport_wc_ucmem_data[4];
    uint8_t block_id;
    uint32_t addr;

    SAL_DEBUGF(("soc_sbus_tsc_reg_write (%d,%d,%d,0x%x,0x%08x,0x%04x)..\n", unit, 10, 14, phy_addr, phy_reg, phy_data));    

    MIIM_SCHAN_LOCK(unit);

    /* TSCE sbus access */
    if ((phy_data & 0xffff0000) == 0) {
        phy_data |= 0xffff0000;
    }

    xlport_wc_ucmem_data[0] = (phy_reg | ((phy_addr & 0x1f) << 19)) & 0xffffffff;;
    xlport_wc_ucmem_data[1] = ((phy_data & 0xffff) << 16) | 
              ((~phy_data & 0xffff0000) >> 16);
    xlport_wc_ucmem_data[2] = 1; /* for TSC register write */
    xlport_wc_ucmem_data[3] = 0x0;
    if (_mdio_addr_to_port(phy_addr) == PHY_GPORT2_BASE) {
		addr = M_GPORT_WC_UCMEM_DATA(0);
		block_id = PMQ1_BLOCK_ID;
    } else {
		addr = M_XLPORT_WC_UCMEM_DATA(0);
		block_id = XLPORT0_BLOCK_ID;
    }

    SAL_DEBUGF(("  ucmem_data_entry[95:64-63:32-31:0]=0x%08x-0x%08x-0x%08x\n", xlport_wc_ucmem_data[0], xlport_wc_ucmem_data[1], xlport_wc_ucmem_data[2]));

    rv = bcm5354x_mem_set(unit, block_id, addr, xlport_wc_ucmem_data, 4);

    SAL_DEBUGF(("soc_sbus_tsc_reg_write : rv=%d\n",rv));

    MIIM_SCHAN_UNLOCK(unit);
    return rv;
}

int 
bmd_phy_init(int unit, int lport)
{
    int rv = CDK_E_NONE;
    
    if (BMD_PORT_PHY_CTRL(unit, lport)) {
        rv = PHY_RESET(BMD_PORT_PHY_CTRL(unit, lport));
        if (CDK_SUCCESS(rv) && phy_reset_cb) {
            rv = phy_reset_cb(BMD_PORT_PHY_CTRL(unit, lport));
        }
        if (CDK_SUCCESS(rv)) {
            rv = PHY_INIT(BMD_PORT_PHY_CTRL(unit, lport));
        }
        if (CDK_SUCCESS(rv) && phy_init_cb) {
            rv = phy_init_cb(BMD_PORT_PHY_CTRL(unit, lport));
        }
    }        
    return rv;
}

int
bmd_phy_init_without_reset(int unit, int lport)
{
    int rv = CDK_E_NONE;
    
    if (BMD_PORT_PHY_CTRL(unit, lport)) {
        rv = PHY_INIT(BMD_PORT_PHY_CTRL(unit, lport));
        if (CDK_SUCCESS(rv) && phy_init_cb) {
            rv = phy_init_cb(BMD_PORT_PHY_CTRL(unit, lport));
        }
    }

    return rv;
}

int 
bmd_phy_probe(int unit, int lport)
{
    int rv;
    BMD_PORT_PHY_BUS(unit, lport) = bcm5354x_phy_bus;
 
    bmd_phy_probe_default(unit, lport, bmd_int_phy_drv_list);
	rv = bmd_phy_probe_default(unit, lport, bmd_ext_phy_drv_list);
	if (rv == CDK_E_NONE) {
		 PBMP_PORT_ADD(phy_external_mode, lport);
	}
    return CDK_E_NONE;
}

int 
bmd_phy_mode_set(int unit, int lport, char *name, int mode, int enable)
{
    int rv = CDK_E_NONE;

    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, lport);

    while (pc != NULL) {
        if (name && pc->drv && pc->drv->drv_name &&
            (sal_strcmp(pc->drv->drv_name, name) != 0)) {
            pc = pc->next;
            continue;
        }
        switch (mode) {
        case BMD_PHY_MODE_WAN:
            rv = PHY_CONFIG_SET(pc, PhyConfig_Mode,
                                enable ? PHY_MODE_WAN : PHY_MODE_LAN, NULL);
            if (!enable && rv == CDK_E_UNAVAIL) {
                rv = CDK_E_NONE;
            }
            break;
        case BMD_PHY_MODE_2LANE:
            if (enable) {
                PHY_CTRL_FLAGS(pc) |= PHY_F_2LANE_MODE;
            } else {
                PHY_CTRL_FLAGS(pc) &= ~PHY_F_2LANE_MODE;
            }
            break;
        case BMD_PHY_MODE_SERDES:
            if (enable) {
                PHY_CTRL_FLAGS(pc) |= PHY_F_SERDES_MODE;
            } else {
                PHY_CTRL_FLAGS(pc) &= ~PHY_F_SERDES_MODE;
            }
            break;
        case BMD_PHY_MODE_FIBER:
            if (enable) {
                PHY_CTRL_FLAGS(pc) |= PHY_F_FIBER_MODE;
            } else {
                PHY_CTRL_FLAGS(pc) &= ~PHY_F_FIBER_MODE;
            }
            break;
        default:
            rv = CDK_E_PARAM;
            break;
        }
        break;
    }
    return rv;
}


int 
bmd_phy_fw_helper_set(int unit, int lport,
                      int (*fw_helper)(void *, uint32_t, uint32_t, void *))
{
    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, lport);

    while (pc != NULL) {
        PHY_CTRL_FW_HELPER(pc) = fw_helper;
        pc = pc->next;
    }
    return CDK_E_NONE;
}

int 
bmd_phy_line_interface_set(int unit, int lport, int intf)
{
    int pref_intf;

    if (BMD_PORT_PHY_CTRL(unit, lport)) {
        switch (intf) {
        case BMD_PHY_IF_XFI:
            pref_intf = PHY_IF_XFI;
            break;
        case BMD_PHY_IF_SFI:
            pref_intf = PHY_IF_SFI;
            break;
        case BMD_PHY_IF_KR:
            pref_intf = PHY_IF_KR;
            break;
        case BMD_PHY_IF_CR:
            pref_intf = PHY_IF_CR;
            break;
        case BMD_PHY_IF_HIGIG:
            pref_intf = PHY_IF_HIGIG;
            break;
        default:
            pref_intf = 0;
            break;
        }
        PHY_CTRL_LINE_INTF(BMD_PORT_PHY_CTRL(unit, lport)) = pref_intf;
    }
    return CDK_E_NONE;
}

int 
bmd_phy_external_mode_get(int unit, int lport)
{
    return (PBMP_MEMBER(phy_external_mode, lport));
}


int 
bmd_phy_ability_set(int unit, int lport, char *name, int ability)
{
    int rv = CDK_E_NONE;

    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, lport);
    int phy_abil = 0;
    
    while (pc != NULL) {
        if (name && pc->drv && pc->drv->drv_name &&
            (sal_strcmp(pc->drv->drv_name, name) != 0)) {
            pc = pc->next;
            continue;
        }

        if (ability & BMD_PHY_ABIL_10MB_HD)
            phy_abil |= PHY_ABIL_10MB_HD;
        if (ability & BMD_PHY_ABIL_10MB_FD)
            phy_abil |= PHY_ABIL_10MB_FD;
        if (ability & BMD_PHY_ABIL_100MB_HD)
            phy_abil |= PHY_ABIL_100MB_HD;
        if (ability & BMD_PHY_ABIL_100MB_FD)
            phy_abil |= PHY_ABIL_100MB_FD;
        if (ability & BMD_PHY_ABIL_1000MB_HD)
            phy_abil |= PHY_ABIL_1000MB_HD;
        if (ability & BMD_PHY_ABIL_1000MB_FD)
            phy_abil |= PHY_ABIL_1000MB_FD;
        if (ability & BMD_PHY_ABIL_2500MB)
            phy_abil |= PHY_ABIL_2500MB;
        if (ability & BMD_PHY_ABIL_3000MB)
            phy_abil |= PHY_ABIL_3000MB;
        if (ability & BMD_PHY_ABIL_10GB)
            phy_abil |= PHY_ABIL_10GB;
        if (ability & BMD_PHY_ABIL_13GB)
            phy_abil |= PHY_ABIL_13GB;
        if (ability & BMD_PHY_ABIL_16GB)
            phy_abil |= PHY_ABIL_16GB;
        if (ability & BMD_PHY_ABIL_21GB)
            phy_abil |= PHY_ABIL_21GB;
        if (ability & BMD_PHY_ABIL_25GB)
            phy_abil |= PHY_ABIL_25GB;
        if (ability & BMD_PHY_ABIL_30GB)
            phy_abil |= PHY_ABIL_30GB;
        if (ability & BMD_PHY_ABIL_40GB)
            phy_abil |= PHY_ABIL_40GB;

        phy_abil |= (PHY_ABIL_PAUSE_TX | PHY_ABIL_PAUSE_RX);

        rv = PHY_CONFIG_SET(pc, PhyConfig_AdvLocal, phy_abil, NULL);
        if (rv == CDK_E_UNAVAIL) {
            rv = CDK_E_NONE;
        }
        break;
    }
    return rv;
}

#define BCM54282_MII_ANPr             (0x00000005)
#undef SOC_PA_PAUSE_TX
#undef SOC_PA_PAUSE_RX

#define SOC_PA_PAUSE_TX        (1 << 0)       /* TX pause capable */
#define SOC_PA_PAUSE_RX        (1 << 1)       /* RX pause capable */

int
phy_pause_get(uint8 unit, uint8 lport, BOOL *tx_pause, BOOL *rx_pause)
{
    int rv = 0;
    phy_ctrl_t *pc = BMD_PORT_PHY_CTRL(unit, lport);
    uint32_t remote_advert, local_advert = (SOC_PA_PAUSE_RX | SOC_PA_PAUSE_TX);
    uint32_t ability;
    PHY_CTRL_CHECK(pc);

    if ((PBMP_MEMBER(phy_external_mode, lport))) {
        if (!sal_strcmp(pc->drv->drv_name,"bcm5428(9)2")) {
            rv = PHY_CONFIG_GET(BMD_PORT_PHY_CTRL(unit, lport), 
                               PhyConfig_AdvRemote, &ability, NULL);

        } else if (!sal_strcmp(pc->drv->drv_name,"bcm56160")) {
            rv = PHY_CONFIG_GET(BMD_PORT_PHY_CTRL(unit, lport), 
                               PhyConfig_AdvRemote, &ability, NULL);

        }
        else {
            return SYS_ERR_PARAMETER;
        }
    } else {
        if (!sal_strcmp(pc->drv->drv_name,"bcmi_qtce_xgxs") || !sal_strcmp(pc->drv->drv_name,"bcmi_tsce_xgxs")) {
            /* Assume CL73 AN Bit[11:10] */
            rv = PHY_CONFIG_GET(BMD_PORT_PHY_CTRL(unit, lport), 
                               PhyConfig_AdvRemote, &ability, NULL);

        } else {
            return SYS_ERR_PARAMETER;
        }
    }

    if (CDK_FAILURE(rv)) {
        PHY_WARN(pc, ("read remote ability failed!\n"));
        return rv;
    }

    remote_advert = 0;
    if (ability & PHY_ABIL_PAUSE_TX) {
        remote_advert |= SOC_PA_PAUSE_TX;
    }
    if (ability & PHY_ABIL_PAUSE_RX) {
        remote_advert |= SOC_PA_PAUSE_RX;
    }

    /*
     * IEEE 802.3 Flow Control Resolution.
     * Please see $SDK/doc/pause-resolution.txt for more information.
     * Assume local always advertises PASUE and ASYM PAUSE.
     */
    *tx_pause =
            ((remote_advert & SOC_PA_PAUSE_RX) &&     
             (local_advert & SOC_PA_PAUSE_RX)) ||     
             ((remote_advert & SOC_PA_PAUSE_RX) &&     
             !(remote_advert & SOC_PA_PAUSE_TX) &&    
             (local_advert & SOC_PA_PAUSE_TX));   

    *rx_pause =
             ((remote_advert & SOC_PA_PAUSE_RX) &&     
             (local_advert & SOC_PA_PAUSE_RX)) ||     
             ((local_advert & SOC_PA_PAUSE_RX) &&      
             (remote_advert & SOC_PA_PAUSE_TX) &&     
             !(local_advert & SOC_PA_PAUSE_TX));

    return(rv);
}


void
usleep(uint32 usec) {
    sal_usleep(usec);
}

#define FW_ALIGN_BYTES                  16
#define FW_ALIGN_MASK                   (FW_ALIGN_BYTES - 1)
int
_firmware_helper(void *ctx, uint32 offset, uint32 size, void *data)
{
    int ioerr = 0;
    uint32 val;
    uint32 wbuf[4], ucmem_data[4];
    uint32 *fw_data;
    uint32 *fw_entry;
    uint32 fw_size;
    uint32 idx, wdx;
    uint8 lport;

    phy_ctrl_t *pc = (phy_ctrl_t *)ctx;
    /* Check if PHY driver requests optimized MDC clock */
    if (data == NULL) {
        uint32 rate_adjust;
        val = 1;

        /* Offset value is MDC clock in kHz (or zero for default) */
        if (offset) {
            val = offset / 1500;
        }

        rate_adjust = READCSR(CMIC_RATE_ADJUST_EXT_MDIO);
        rate_adjust &= ~ (0xFFFF0000);
        rate_adjust |= val << 16;
        ioerr += WRITECSR(CMIC_RATE_ADJUST_EXT_MDIO, val);

        return ioerr ? SYS_ERR_STATE : SYS_OK;
    }

    if (size == 0) {
        return SYS_ERR_PARAMETER;
    }

    /* Aligned firmware size */
    fw_size = (size + FW_ALIGN_MASK) & ~FW_ALIGN_MASK;

    lport = SOC_PORT_P2L_MAPPING(pc->port);    
    /* Enable parallel bus access, ACCESS_MODE [Bit 0] = 0x1 */
    if (IS_XL_PORT(lport)) {
        bcm5354x_reg_set(pc->unit, SOC_PORT_BLOCK(lport), R_XLPORT_WC_UCMEM_CTRL, 0x1);    
    } else {
        bcm5354x_reg_set(pc->unit, PMQ1_BLOCK_ID, R_GPORT_WC_UCMEM_CTRL, 0x1);    
    }

    /* DMA buffer needs 32-bit words in little endian order */
    fw_data = (uint32 *)data;
    for (idx = 0; idx < fw_size; idx += 16) {
        if (idx + 15 < size) {
            fw_entry = &fw_data[idx >> 2];
        } else {
            /* Use staging buffer for modulo bytes */
            sal_memset(wbuf, 0, sizeof(wbuf));
            sal_memcpy(wbuf, &fw_data[idx >> 2], 16 - (fw_size - size));
            fw_entry = wbuf;
        }
        for (wdx = 0; wdx < 4; wdx++) {
            ucmem_data[wdx] = htol32(fw_entry[wdx]);
        }
		if (IS_XL_PORT(lport)) {
            bcm5354x_mem_set(pc->unit, SOC_PORT_BLOCK(lport),
                         M_XLPORT_WC_UCMEM_DATA(idx >> 4), ucmem_data, 4);
        } else {
            bcm5354x_mem_set(pc->unit, PMQ1_BLOCK_ID,
                         M_GPORT_WC_UCMEM_DATA(idx >> 4), ucmem_data, 4);
        }
    }

    /* Disable parallel bus access */
    if (IS_XL_PORT(lport)) {
        bcm5354x_reg_set(pc->unit, SOC_PORT_BLOCK(lport), R_XLPORT_WC_UCMEM_CTRL, 0x0);      
    } else {
		bcm5354x_reg_set(pc->unit, PMQ1_BLOCK_ID, R_GPORT_WC_UCMEM_CTRL, 0x0);
    }

    return ioerr ? SYS_ERR_STATE : SYS_OK;
}


#define FW_ALIGN_BYTES                  16
#define FW_ALIGN_MASK                   (FW_ALIGN_BYTES - 1)
int
_qtc_firmware_helper(void *ctx, uint32 offset, uint32 size, void *data)
{
    int ioerr = 0;
    uint32 val;
    uint32 wbuf[4];
    uint32 *fw_data;
    uint32 *fw_entry;
    uint32 fw_size;
    uint32 idx, wdx;
    phy_ctrl_t *pc = (phy_ctrl_t *)ctx;

    CMIC_RATE_ADJUST_EXT_MDIOr_t cmic_rate_adjust_ext_mdio;
    GPORT_WC_UCMEM_CTRLr_t gport_wc_ucmem_ctrl;
    GPORT_WC_UCMEM_DATAm_t gport_wc_ucmem_data;

    /* Check if PHY driver requests optimized MDC clock */
    if (data == NULL) {
        val = 1;

        /* Offset value is MDC clock in kHz (or zero for default) */
        if (offset) {
            val = offset / 1500;
        }

        ioerr = READ_CMIC_RATE_ADJUST_EXT_MDIOr(pc->unit, cmic_rate_adjust_ext_mdio);
        CMIC_RATE_ADJUST_EXT_MDIOr_DIVIDENDf_SET(cmic_rate_adjust_ext_mdio, val);
        ioerr += WRITE_CMIC_RATE_ADJUST_EXT_MDIOr(pc->unit, cmic_rate_adjust_ext_mdio);

        return ioerr ? SYS_ERR_STATE : SYS_OK;
    }
    if (sal_strcmp(pc->drv->drv_name, "bcmi_qtce_xgxs") != 0) {
        return CDK_E_NONE;
    }
    if (size == 0) {
        return SYS_ERR_PARAMETER;
    }

    /* Aligned firmware size */
    fw_size = (size + FW_ALIGN_MASK) & ~FW_ALIGN_MASK;

    /* Enable parallel bus access, ACCESS_MODE [Bit 0] = 0x1 */
    GPORT_WC_UCMEM_CTRLr_CLR(gport_wc_ucmem_ctrl);
    GPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(gport_wc_ucmem_ctrl, 1);
    WRITE_GPORT_WC_UCMEM_CTRLr(pc->unit, SOC_PORT_P2L_MAPPING(PHY_GPORT2_BASE), gport_wc_ucmem_ctrl);

    /* DMA buffer needs 32-bit words in little endian order */
    fw_data = (uint32 *)data;
    for (idx = 0; idx < fw_size; idx += 16) {
        if (idx + 15 < size) {
            fw_entry = &fw_data[idx >> 2];
        } else {
            /* Use staging buffer for modulo bytes */
            sal_memset(wbuf, 0, sizeof(wbuf));
            sal_memcpy(wbuf, &fw_data[idx >> 2], 16 - (fw_size - size));
            fw_entry = wbuf;
        }
        GPORT_WC_UCMEM_DATAm_CLR(gport_wc_ucmem_data);
        for (wdx = 0; wdx < 4; wdx++) {
            GPORT_WC_UCMEM_DATAm_SET(gport_wc_ucmem_data, wdx, htol32(fw_entry[wdx]));
        }
        WRITE_GPORT_WC_UCMEM_DATAm(pc->unit, SOC_PORT_P2L_MAPPING(PHY_GPORT2_BASE), (idx >> 4), gport_wc_ucmem_data);
    }

    /* Disable parallel bus access */
    GPORT_WC_UCMEM_CTRLr_ACCESS_MODEf_SET(gport_wc_ucmem_ctrl, 0);
    WRITE_GPORT_WC_UCMEM_CTRLr(pc->unit, SOC_PORT_P2L_MAPPING(PHY_GPORT2_BASE), gport_wc_ucmem_ctrl);


    return ioerr ? SYS_ERR_STATE : SYS_OK;
}





