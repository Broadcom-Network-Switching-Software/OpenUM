/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>
#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>
#include "tdm/tdm.h"

#define GH2_TDM_DEBUG   0

#if GH2_TDM_DEBUG
#define TDM_DBG(x)  do { sal_printf x; } while(0);
#else
#define TDM_DBG(x)
#endif

#define LINKSCAN_INTERVAL        (100000UL)   /* 100 ms */

#define CFG_FLOW_CONTROL_ENABLED_COSQ 1   /* Based on priority <-> cosq mapping */

/* MMU related */
#define GH2_MMU_CBP_FULL_SIZE 0x7fff
#define GH2_MMU_CBP_HALF_SIZE 0x5fff
#define GH2_MMU_CBP_QUARTER_SIZE 0x3fff

#define CFG_MMU_DEBUG           (0)
#define MMU_64Q_PPORT_BASE      (58)
#define CFG_SET_TOP_CORE_PLL    (1)

/* TDM related */
#define NUM_EXT_PORTS                       136

/* Allocation length of VBS line rate calendar */
#define GH2_LR_VBS_LEN 512

typedef struct tdm_config_s {
    int     speed[NUM_EXT_PORTS];
    int     port_state[NUM_EXT_PORTS];

    int idb_tdm_tbl_0[GH2_LR_VBS_LEN];
} tdm_config_t;

#define CMIC_LPORT                     0
#define CMIC_PPORT                     0

/*
 * Function:
 *  _soc_greyhound2_gpio_set
 * Purpose:
 *  Write value to the specified GPIO pin
 */
STATIC int
_soc_greyhound2_gpio_set(int unit, int pin, int output, int val)
{

    uint32 fval = 0;
    uint8 mask = 0xFF;
    CMIC_GP_OUT_ENr_t cmic_gp_out_en;
    CMIC_GP_DATA_OUTr_t cmic_gp_data_out;
    mask &= ~(1 << pin);

    /* coverity[result_independent_of_operands] */
    READ_CMIC_GP_OUT_ENr(unit, cmic_gp_out_en);
    fval = CMIC_GP_OUT_ENr_OUT_ENABLEf_GET(cmic_gp_out_en);
    if (output) {
        fval |= 1 << pin;
    } else {
        fval &= ~(1 << pin);
    }
    CMIC_GP_OUT_ENr_OUT_ENABLEf_SET(cmic_gp_out_en, fval);

    /* coverity[result_independent_of_operands] */
    WRITE_CMIC_GP_OUT_ENr(unit, cmic_gp_out_en);

    if (output) {
        /* coverity[result_independent_of_operands] */
        READ_CMIC_GP_DATA_OUTr(unit, cmic_gp_data_out);
        fval = CMIC_GP_DATA_OUTr_DATA_OUTf_GET(cmic_gp_data_out);
        if (val) {
            fval |= (val << pin);
        } else {
            fval &= ~(1 << pin);
        }
        CMIC_GP_DATA_OUTr_DATA_OUTf_SET(cmic_gp_data_out, fval);
        /* coverity[result_independent_of_operands] */
        WRITE_CMIC_GP_DATA_OUTr(unit, cmic_gp_data_out);
    }

    return SOC_E_NONE;
}


static void
soc_reset(uint8 unit)
{
    uint32 to_usec;

    TOP_SOFT_RESET_REGr_t top_soft_reset; 
    TOP_SOFT_RESET_REG_2r_t top_soft_reset2;
    CMIC_SBUS_TIMEOUTr_t cmic_sbus_timeout;
    CMIC_SBUS_RING_MAPr_t ring_map;
#if CFG_SET_TOP_CORE_PLL	
    TOP_CORE_PLL_CTRL3r_t top_core_pll_ctrl3;
    TOP_CORE_PLL_CTRL4r_t top_core_pll_ctrl4;
    TOP_CORE_PLL_CTRL5r_t top_core_pll_ctrl5;
    TOP_CORE_PLL_CTRL6r_t top_core_pll_ctrl6;
    TOP_MISC_CONTROL_1r_t top_misc_control_1;
    uint32 core_pll_ndiv;
    uint32 core_pll_ch0;
    uint32 core_pll_ch1;
    uint32 core_pll_ch2;
    uint32 core_pll_ch3;
#endif

#if CONFIG_EMULATION
    to_usec = 250000;
#else
    to_usec = 10000;
#endif /* CONFIG_EMULATION */


    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x11110100);
    WRITE_CMIC_SBUS_RING_MAPr(unit, 0, ring_map); /* block 0  - 7 */
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x00430000);
    WRITE_CMIC_SBUS_RING_MAPr(unit, 1, ring_map); /* block 8 - 15 */
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x00005064);
    WRITE_CMIC_SBUS_RING_MAPr(unit, 2, ring_map); /* block 16 - 23 */
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x02220111);
    WRITE_CMIC_SBUS_RING_MAPr(unit, 3, ring_map); /* block 24 - 31 */
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x02220222);
    WRITE_CMIC_SBUS_RING_MAPr(unit, 4, ring_map); /* block 32 - 39 */
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x22222222);
    WRITE_CMIC_SBUS_RING_MAPr(unit, 5, ring_map); /* block 40 - 47 */

    CMIC_SBUS_TIMEOUTr_CLR(cmic_sbus_timeout);
    CMIC_SBUS_TIMEOUTr_TIMEOUT_VALf_SET(cmic_sbus_timeout, 0x7d0);
    WRITE_CMIC_SBUS_TIMEOUTr(unit,cmic_sbus_timeout);

    /* Skip polling TOP_XG_PLL0_STATUS since we have to keep going anyway */
    sal_usleep(to_usec);
    
#if 0
     uint32 val32;
     if (sal_config_uint32_get("pci_delay_val", &val32) == SYS_OK) {
         char name_str[16];
        
         TOP_SW_BOND_OVERRIDE_CODE_REGr_t top_sw_bond_override_code_reg;
         TOP_SW_BOND_OVERRIDE_REGr_t top_sw_bond_override_reg;
         int i;        
         for (i = 0; i < 16; i++) {
              sal_sprintf(name_str, "pci_delay_val%d", i);
              val32 = 0;
              if (sal_config_uint32_get(name_str, &val32) == SYS_OK) {
                  SAL_DEBUGF(("%s = %x\n", name_str, val32));
              };
              TOP_SW_BOND_OVERRIDE_REGr_SET(top_sw_bond_override_reg, val32);
              WRITE_TOP_SW_BOND_OVERRIDE_REGr(unit, i, top_sw_bond_override_reg);            
         }
         sal_config_uint32_get("pci_delay_val", &val32);
        
         TOP_SW_BOND_OVERRIDE_CODE_REGr_SET(top_sw_bond_override_code_reg, val32);
         WRITE_TOP_SW_BOND_OVERRIDE_CODE_REGr(unit, top_sw_bond_override_code_reg);
        
         bcm5357x_chip_revision(unit, &gh2_sw_info.devid, &gh2_sw_info.revid);
         sal_printf("\nOverride devid = 0x%x, revid = 0x%x\n", gh2_sw_info.devid, gh2_sw_info.revid);
        
    }
#endif /* VENDOR_BROADCOM */
    

    soc_port_config_init(unit);
#if CFG_SET_TOP_CORE_PLL
    READ_TOP_CORE_PLL_CTRL4r(unit, top_core_pll_ctrl4);
    READ_TOP_CORE_PLL_CTRL3r(unit, top_core_pll_ctrl3);
    READ_TOP_CORE_PLL_CTRL5r(unit, top_core_pll_ctrl5);    
    READ_TOP_CORE_PLL_CTRL6r(unit, top_core_pll_ctrl6);    

    switch (sku_port_config->freq) 
    {
        case _GH2_SYSTEM_FREQ_125:
            core_pll_ndiv = 140;
            core_pll_ch0 = 28;
            core_pll_ch1 = 28;
            core_pll_ch2 = 14;
            core_pll_ch3 = 7;
            break;
        case _GH2_SYSTEM_FREQ_375:
            core_pll_ndiv = 120;
            core_pll_ch0 = 8;
            core_pll_ch1 = 24;
            core_pll_ch2 = 12;
            core_pll_ch3 = 6;
            break;
        case _GH2_SYSTEM_FREQ_389:
            core_pll_ndiv = 140;
            core_pll_ch0 = 9;
            core_pll_ch1 = 28;
            core_pll_ch2 = 14;
            core_pll_ch3 = 7;
            break;
        case _GH2_SYSTEM_FREQ_392:
            core_pll_ndiv = 110;
            core_pll_ch0 = 7;
            core_pll_ch1 = 22;
            core_pll_ch2 = 11;
            core_pll_ch3 = 6;
            sal_timer_init(458300000, TRUE);
            break;
        case _GH2_SYSTEM_FREQ_437:
            core_pll_ndiv = 140;
            core_pll_ch0 = 8;
            core_pll_ch1 = 28;
            core_pll_ch2 = 14;
            core_pll_ch3 = 7;
            break;
        case _GH2_SYSTEM_FREQ_450:
            core_pll_ndiv = 90;
            core_pll_ch0 = 5;
            core_pll_ch1 = 18;
            core_pll_ch2 = 9;
            core_pll_ch3 = 5;
            sal_timer_init(450000000, TRUE);
            break;
        case _GH2_SYSTEM_FREQ_500:
            core_pll_ndiv = 140;
            core_pll_ch0 = 7;
            core_pll_ch1 = 28;
            core_pll_ch2 = 14;
            core_pll_ch3 = 7;
            break;
        case _GH2_SYSTEM_FREQ_583:
        default:
            core_pll_ndiv = 140;
            core_pll_ch0 = 6;
            core_pll_ch1 = 28;
            core_pll_ch2 = 14;
            core_pll_ch3 = 7;
            break;
    }

    TOP_CORE_PLL_CTRL3r_MSTR_NDIV_INTf_SET(top_core_pll_ctrl3, core_pll_ndiv);
    WRITE_TOP_CORE_PLL_CTRL3r(unit, top_core_pll_ctrl3);

    TOP_CORE_PLL_CTRL4r_MSTR_CH0_MDIVf_SET(top_core_pll_ctrl4, core_pll_ch0);
    WRITE_TOP_CORE_PLL_CTRL4r(unit, top_core_pll_ctrl4);

    TOP_CORE_PLL_CTRL4r_MSTR_CH1_MDIVf_SET(top_core_pll_ctrl4, core_pll_ch1);
    WRITE_TOP_CORE_PLL_CTRL4r(unit, top_core_pll_ctrl4);

    TOP_CORE_PLL_CTRL5r_MSTR_CH2_MDIVf_SET(top_core_pll_ctrl5, core_pll_ch2);
    WRITE_TOP_CORE_PLL_CTRL5r(unit, top_core_pll_ctrl5);

    TOP_CORE_PLL_CTRL6r_MSTR_CH3_MDIVf_SET(top_core_pll_ctrl6, core_pll_ch3);
    WRITE_TOP_CORE_PLL_CTRL6r(unit, top_core_pll_ctrl6);


    READ_TOP_MISC_CONTROL_1r(unit, top_misc_control_1);
    TOP_MISC_CONTROL_1r_CMIC_TO_CORE_PLL_LOADf_SET(top_misc_control_1, 1);
    WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_control_1);
#endif

    /*
     * Select LCPLL0 as external PHY reference clock and output 125MHz
     */
    if (sal_strcmp(board_name(), "BCM953570K") == 0) {
        /* Pull GPIO3 low to reset the ext. PHY */
        _soc_greyhound2_gpio_set(unit, 3, TRUE, 0);
    }
   
    TOP_XG_PLL0_CTRL_0r_t top_xg_pll0_ctrl_0;
    TOP_XG_PLL0_CTRL_6r_t top_xg_pll0_ctrl_6; 

    READ_TOP_XG_PLL0_CTRL_0r(unit, top_xg_pll0_ctrl_0);
    TOP_XG_PLL0_CTRL_0r_CH5_MDIVf_SET(top_xg_pll0_ctrl_0, 0x19);
    WRITE_TOP_XG_PLL0_CTRL_0r(unit, top_xg_pll0_ctrl_0);

    READ_TOP_XG_PLL0_CTRL_6r(unit, top_xg_pll0_ctrl_6);
    TOP_XG_PLL0_CTRL_6r_MSC_CTRLf_SET(top_xg_pll0_ctrl_6, 0x71a2);
    WRITE_TOP_XG_PLL0_CTRL_6r(unit, top_xg_pll0_ctrl_6);
    sal_usleep(to_usec);

    if (sal_strcmp(board_name(), "BCM953570K") == 0) {
        /* Pull  GPIO high to leave the reset state */
        _soc_greyhound2_gpio_set(unit, 3, TRUE, 1);
    }

    /*
     * Bring port blocks out of reset
     */
    READ_TOP_SOFT_RESET_REGr(unit,  top_soft_reset);
    TOP_SOFT_RESET_REGr_TOP_GE8P_RST_Lf_SET(top_soft_reset, 0x7);
    TOP_SOFT_RESET_REGr_TOP_GEP0_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_GEP1_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP0_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP1_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP2_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP3_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP4_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP5_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP6_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP0_RST_Lf_SET(top_soft_reset, 1);
    WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset);  
    sal_usleep(to_usec);


    /* Bring network sync out of reset */
    READ_TOP_SOFT_RESET_REGr(unit,  top_soft_reset);  
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_soft_reset,1);
    WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset);
    sal_usleep(to_usec);


    READ_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_soft_reset2, 1);
    WRITE_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_soft_reset2, 1);
    WRITE_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset2);


    /* Bring IP, EP, and MMU blocks out of reset */
    READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_soft_reset, 1); 
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_soft_reset, 1);    
    WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset);
    sal_usleep(to_usec);
    

    
}

soc_chip_type_t
bcm5357x_chip_type(void)
{
    return SOC_TYPE_SWITCH_XGS;
}

uint8
bcm5357x_port_count(uint8 unit)
{
    if (unit > 0) {
        return 0;
    }
    return SOC_PORT_COUNT(unit);
}

sys_error_t
bcm5357x_chip_revision(uint8 unit, uint16 *dev, uint16 *rev)
{
    CMIC_DEV_REV_IDr_t cmic_dev_rev_id;

    if (unit > 0) {
        return -1;
    }

    READ_CMIC_DEV_REV_IDr(unit,cmic_dev_rev_id);
    *dev = CMIC_DEV_REV_IDr_DEV_IDf_GET(cmic_dev_rev_id);
    *rev = CMIC_DEV_REV_IDr_REV_IDf_GET(cmic_dev_rev_id);

    return 0;
}

#define JUMBO_FRM_SIZE (9216)


static void
enable_jumbo_frame(uint8 unit)
{
    int lport;
    FRM_LENGTHr_t frm_length;
    XLMAC_RX_MAX_SIZEr_t xlmac_rx_max_size;
    CLMAC_RX_MAX_SIZEr_t clmac_rx_max_size;

    FRM_LENGTHr_CLR(frm_length);
    FRM_LENGTHr_MAXFRf_SET(frm_length, JUMBO_FRM_SIZE);

    XLMAC_RX_MAX_SIZEr_CLR(xlmac_rx_max_size);
    XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(xlmac_rx_max_size, JUMBO_FRM_SIZE);

    CLMAC_RX_MAX_SIZEr_CLR(clmac_rx_max_size);
    CLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(clmac_rx_max_size, JUMBO_FRM_SIZE);

    SOC_LPORT_ITER(lport) {
        if (IS_GX_PORT(lport)) {
            WRITE_FRM_LENGTHr(unit, lport, frm_length);
        } else if (IS_XL_PORT(lport)) {
            WRITE_XLMAC_RX_MAX_SIZEr(unit, lport, xlmac_rx_max_size);
        } else if (IS_CL_PORT(lport)) {
            WRITE_CLMAC_RX_MAX_SIZEr(unit, lport, clmac_rx_max_size);
        }
    }
}

#define PIPE_INIT_COUNT 50000  

static void
soc_pipe_mem_clear(uint8 unit)
{
    int i;

    ING_HW_RESET_CONTROL_1r_t ing_hw_reset_control_1;
    ING_HW_RESET_CONTROL_2r_t ing_hw_reset_control_2;
    EGR_HW_RESET_CONTROL_0r_t egr_hw_reset_control_0;
    EGR_HW_RESET_CONTROL_1r_t egr_hw_reset_control_1;

    /*
     * Reset the IPIPE and EPIPE block
     */
    ING_HW_RESET_CONTROL_1r_CLR(ing_hw_reset_control_1);
    WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_hw_reset_control_1);

    /* Set count to # entries in largest IPIPE table, L2_ENTRYm */
    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_reset_control_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_hw_reset_control_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_hw_reset_control_2, 1);  
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_hw_reset_control_2, L2_ENTRYm_MAX+1);  
    WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2);  

    EGR_HW_RESET_CONTROL_0r_CLR(egr_hw_reset_control_0);
    WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_hw_reset_control_0);

    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_control_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_hw_reset_control_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_hw_reset_control_1, 1);
    /* Set count to # entries in largest EPIPE table, EGR_VLANm */
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_hw_reset_control_1, EGR_VLANm_MAX + 1);
    WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1);

    /* Wait for IPIPE memory initialization done. */
    i = 0;
    do {
        READ_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2);
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_hw_reset_control_2)) {
            break;
        }
        i++;
        if (i > PIPE_INIT_COUNT) {
            sal_printf("unit = %d: ING_HW_RESET timeout  \n", unit);
            break;
        }       
        sal_usleep(100);
    } while(1);

    /* Wait for EPIPE memory initialization done. */
    i = 0;
    do {
        READ_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1);
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_hw_reset_control_1)) {
            break;
        }
        i++;
        if (i > PIPE_INIT_COUNT) {
            sal_printf("unit = %d: EGR_HW_RESET timeout  \n", unit);
            break;
        }       
        sal_usleep(100);
    } while(1);

    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_reset_control_2);
    WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2);
    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_control_1);
    WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1);
}



static sys_error_t
soc_init_port_mapping(uint8 unit)
{
    int lport, pport, mmu_port;
    pbmp_t pbmp;

    ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_t ing_physical_to_logical_port_number_mapping_table;
    EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr_t egr_logical_to_physical_port_number_mapping;
    EGR_TDM_PORT_MAPm_t egr_tdm_port_map;
    MMU_PORT_TO_PHY_PORT_MAPPINGr_t mmu_port_to_phy_port_mapping;
    MMU_PORT_TO_LOGIC_PORT_MAPPINGr_t mmu_port_to_logic_port_mapping;

  
    /* Ingress physical to logical port mapping */
    for (pport = 0; pport <= BCM5357X_PORT_MAX; pport++) {
        ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_CLR(ing_physical_to_logical_port_number_mapping_table);
    if (SOC_PORT_P2L_MAPPING(pport) == -1) {
        ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_LOGICAL_PORT_NUMBERf_SET(ing_physical_to_logical_port_number_mapping_table, 0x7F);
    } else {
        ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_LOGICAL_PORT_NUMBERf_SET(ing_physical_to_logical_port_number_mapping_table, SOC_PORT_P2L_MAPPING(pport));
    }
        WRITE_ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm(unit, pport ,ing_physical_to_logical_port_number_mapping_table);   
    }

    /* Egress logical to physical port mapping, needs a way for maximum logical port? */
    for (lport = 0; lport <= BCM5357X_LPORT_MAX; lport++) {
        EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr_CLR(egr_logical_to_physical_port_number_mapping);    
        EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr_PHYSICAL_PORT_NUMBERf_SET(egr_logical_to_physical_port_number_mapping,
                                        ((SOC_PORT_L2P_MAPPING(lport)== -1) ? 0x7F : (SOC_PORT_L2P_MAPPING(lport))));
        WRITE_EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr(unit, lport, egr_logical_to_physical_port_number_mapping);
    }

    /* EGR_TDM_PORT_MAPm */
    EGR_TDM_PORT_MAPm_CLR(egr_tdm_port_map);

    PBMP_CLEAR(pbmp); 
    for (lport = 0; lport <= BCM5357X_LPORT_MAX; lport++) {
        pport = SOC_PORT_L2P_MAPPING(lport);
        /*
               * Physical port of loopback port(1) is assigned -1.
               * The port should be programmed on physical port 1.
               */
        if (lport == 1) {
            pport = 1;
        }          
        if (pport < 0) continue;

        PBMP_PORT_ADD(pbmp, pport);   
    }
    EGR_TDM_PORT_MAPm_BITMAPf_SET(egr_tdm_port_map, SOC_PBMP(pbmp));
    WRITE_EGR_TDM_PORT_MAPm(unit, egr_tdm_port_map);

    /* MMU to physical port mapping and MMU to logical port mapping */
    for (mmu_port = 0; mmu_port <= SOC_MAX_MMU_PORTS; mmu_port++) {
        /* MMU to physical port */
        pport = SOC_PORT_M2P_MAPPING(mmu_port);
        /* Physical to logical port */
        lport = SOC_PORT_P2L_MAPPING(pport);

        if (pport == 1) {
            /* skip loopback port */
            continue;
        }

        if (pport == -1) {
            /* skip not mapped mmu port */
            continue;
        }
        
        MMU_PORT_TO_PHY_PORT_MAPPINGr_CLR(mmu_port_to_phy_port_mapping);
        MMU_PORT_TO_PHY_PORT_MAPPINGr_PHY_PORTf_SET(mmu_port_to_phy_port_mapping, pport);
        
        WRITE_MMU_PORT_TO_PHY_PORT_MAPPINGr(unit, mmu_port, mmu_port_to_phy_port_mapping);

        if (lport == -1) {
            lport = 1;
        }

        MMU_PORT_TO_LOGIC_PORT_MAPPINGr_CLR(mmu_port_to_logic_port_mapping);  
        MMU_PORT_TO_LOGIC_PORT_MAPPINGr_LOGIC_PORTf_SET(mmu_port_to_logic_port_mapping, lport);
        
        WRITE_MMU_PORT_TO_LOGIC_PORT_MAPPINGr(unit, mmu_port, mmu_port_to_logic_port_mapping);
    }

    return SYS_OK;
}

static void
soc_tsc_xgxs_reset(uint8 unit, uint8 port)
{
#if CONFIG_EMULATION
    int         sleep_usec = 500000;
    int         lcpll = 0;
#else
    int         sleep_usec = 1100;
    int         lcpll = 1;
#endif /* CONFIG_EMULATION */
    GPORT_XGXS0_CTRL_REGr_t gport_xgxs0_ctrl_reg;
    XLPORT_XGXS0_CTRL_REGr_t xlport_xgxs0_ctrl_reg;
    CLPORT_XGXS0_CTRL_REGr_t clport_xgxs0_ctrl_reg;
    PGW_CTRL_0r_t pgw_ctrl_0;
    GPORT_SGMII0_CTRL_REGr_t gport_sgmii0_ctrl_reg; 
    GPORT_SGMII1_CTRL_REGr_t gport_sgmii1_ctrl_reg;

    
    if (IS_GX_PORT(port)) {
        
        /*
         * Reference clock selection
         */
        
        READ_GPORT_SGMII0_CTRL_REGr(unit, port, gport_sgmii0_ctrl_reg); 
        GPORT_SGMII0_CTRL_REGr_IDDQf_SET(gport_sgmii0_ctrl_reg, 1);
        GPORT_SGMII0_CTRL_REGr_PWRDWNf_SET(gport_sgmii0_ctrl_reg, 1);  
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, port, gport_sgmii0_ctrl_reg); 
        sal_usleep(sleep_usec);
        
        /* Analog section powered */
        GPORT_SGMII0_CTRL_REGr_IDDQf_SET(gport_sgmii0_ctrl_reg, 0);
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, port, gport_sgmii0_ctrl_reg); 
        
        /* Deassert power down */
        GPORT_SGMII0_CTRL_REGr_PWRDWNf_SET(gport_sgmii0_ctrl_reg, 0);
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, port, gport_sgmii0_ctrl_reg); 
        sal_usleep(sleep_usec);
        
        /* Bring SGMII out of reset */
        GPORT_SGMII0_CTRL_REGr_RSTB_HWf_SET(gport_sgmii0_ctrl_reg, 1);
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, port, gport_sgmii0_ctrl_reg); 
        sal_usleep(sleep_usec);
        
        /* Activate MDIO on SGMII */
        GPORT_SGMII0_CTRL_REGr_RSTB_MDIOREGSf_SET(gport_sgmii0_ctrl_reg, 1);
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, port, gport_sgmii0_ctrl_reg); 
        sal_usleep(sleep_usec);
        
        /* Activate clocks */
        GPORT_SGMII0_CTRL_REGr_RSTB_PLLf_SET(gport_sgmii0_ctrl_reg, 1);
        WRITE_GPORT_SGMII0_CTRL_REGr(unit, port, gport_sgmii0_ctrl_reg); 
        sal_usleep(sleep_usec);
        
        READ_GPORT_SGMII1_CTRL_REGr(unit, port, gport_sgmii1_ctrl_reg); 
        GPORT_SGMII1_CTRL_REGr_IDDQf_SET(gport_sgmii1_ctrl_reg, 1);
        GPORT_SGMII1_CTRL_REGr_PWRDWNf_SET(gport_sgmii1_ctrl_reg, 1);  
        WRITE_GPORT_SGMII1_CTRL_REGr(unit, port, gport_sgmii1_ctrl_reg); 
        sal_usleep(sleep_usec);
        
        /* Analog section powered */
        GPORT_SGMII1_CTRL_REGr_IDDQf_SET(gport_sgmii1_ctrl_reg, 0);
        WRITE_GPORT_SGMII1_CTRL_REGr(unit, port, gport_sgmii1_ctrl_reg); 
        
        /* Deassert power down */
        GPORT_SGMII1_CTRL_REGr_PWRDWNf_SET(gport_sgmii1_ctrl_reg, 0);
        WRITE_GPORT_SGMII1_CTRL_REGr(unit, port, gport_sgmii1_ctrl_reg); 
        sal_usleep(sleep_usec);
        
        /* Bring SGMII out of reset */
        GPORT_SGMII1_CTRL_REGr_RSTB_HWf_SET(gport_sgmii1_ctrl_reg, 1);
        WRITE_GPORT_SGMII1_CTRL_REGr(unit, port, gport_sgmii1_ctrl_reg); 
        sal_usleep(sleep_usec);
        
        /* Activate MDIO on SGMII */
        GPORT_SGMII1_CTRL_REGr_RSTB_MDIOREGSf_SET(gport_sgmii1_ctrl_reg, 1);
        WRITE_GPORT_SGMII1_CTRL_REGr(unit, port, gport_sgmii1_ctrl_reg); 
        sal_usleep(sleep_usec);
        
        /* Activate clocks */
        GPORT_SGMII1_CTRL_REGr_RSTB_PLLf_SET(gport_sgmii1_ctrl_reg, 1);
        WRITE_GPORT_SGMII1_CTRL_REGr(unit, port, gport_sgmii1_ctrl_reg); 
        sal_usleep(sleep_usec);

        if (IS_QTCE_PORT(port)) {
            /* Turn on PGW for QTCE */
            READ_PGW_CTRL_0r(unit, pgw_ctrl_0);
            PGW_CTRL_0r_SW_QTC_DISABLEf_SET(pgw_ctrl_0, 
            PGW_CTRL_0r_SW_QTC_DISABLEf_GET(pgw_ctrl_0) & ~(1 << QTCE_CORE_NUM_GET(port)));
            WRITE_PGW_CTRL_0r(unit, pgw_ctrl_0);
        } else if (IS_SGMIIPX4_PORT(port)) {
             /* TBD */
        }
        
        /*
         * Reference clock selection
         */
        READ_GPORT_XGXS0_CTRL_REGr(unit, port, gport_xgxs0_ctrl_reg);
        GPORT_XGXS0_CTRL_REGr_IDDQf_SET(gport_xgxs0_ctrl_reg, 0);
        WRITE_GPORT_XGXS0_CTRL_REGr(unit, port, gport_xgxs0_ctrl_reg);
            

        READ_GPORT_XGXS0_CTRL_REGr(unit, port, gport_xgxs0_ctrl_reg);
        GPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(gport_xgxs0_ctrl_reg, lcpll ? 1 : 0);
        WRITE_GPORT_XGXS0_CTRL_REGr(unit, port, gport_xgxs0_ctrl_reg);

        /* Deassert power down */
        GPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(gport_xgxs0_ctrl_reg, 0);
        WRITE_GPORT_XGXS0_CTRL_REGr(unit, port, gport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);

        /* Reset XGXS */
        GPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(gport_xgxs0_ctrl_reg, 0);
        WRITE_GPORT_XGXS0_CTRL_REGr(unit, port, gport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);

        /* Bring XGXS out of reset */
        GPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(gport_xgxs0_ctrl_reg, 1);
        WRITE_GPORT_XGXS0_CTRL_REGr(unit, port, gport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);

        /* Bring reference clock out reset */
        GPORT_XGXS0_CTRL_REGr_RSTB_REFCLKf_SET(gport_xgxs0_ctrl_reg, 1);
        WRITE_GPORT_XGXS0_CTRL_REGr(unit, port, gport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);

        /* Activate clocks */  
        GPORT_XGXS0_CTRL_REGr_RSTB_PLLf_SET(gport_xgxs0_ctrl_reg, 1);
        WRITE_GPORT_XGXS0_CTRL_REGr(unit, port, gport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);

    } else if (IS_XL_PORT(port)) {
        /*
         * Reference clock selection
         */

        if (IS_TSCE_PORT(port)) {
            READ_PGW_CTRL_0r(unit, pgw_ctrl_0);
            PGW_CTRL_0r_SW_PM4X10_DISABLEf_SET(pgw_ctrl_0, 
            PGW_CTRL_0r_SW_PM4X10_DISABLEf_GET(pgw_ctrl_0) & ~(1 << TSCE_CORE_NUM_GET(port)));
            WRITE_PGW_CTRL_0r(unit, pgw_ctrl_0);
        }

        /*
         * Reference clock selection
         */
        READ_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg);
        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlport_xgxs0_ctrl_reg, 0);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg);

        READ_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg);
        XLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(xlport_xgxs0_ctrl_reg, lcpll ? 1 : 0);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg);

        /* Deassert power down */
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlport_xgxs0_ctrl_reg, 0);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);

        /* Reset XGXS */
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_xgxs0_ctrl_reg, 0);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);

        /* Bring XGXS out of reset */
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_xgxs0_ctrl_reg, 1);
        WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);

    } else if (IS_CL_PORT(port)) {
    /*
         * Reference clock selection
         */
        if (IS_TSCF_PORT(port)) {
            READ_PGW_CTRL_0r(unit, pgw_ctrl_0);
            PGW_CTRL_0r_SW_PM4X25_DISABLEf_SET(pgw_ctrl_0, 
            PGW_CTRL_0r_SW_PM4X25_DISABLEf_GET(pgw_ctrl_0) & ~(1 << TSCF_CORE_NUM_GET(port)));
            WRITE_PGW_CTRL_0r(unit, pgw_ctrl_0);
        }

        /*
         * Reference clock selection
         */
        READ_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg);
        CLPORT_XGXS0_CTRL_REGr_IDDQf_SET(clport_xgxs0_ctrl_reg, 0);
        WRITE_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg);
  
        READ_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg);
        CLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(clport_xgxs0_ctrl_reg, lcpll ? 1 : 0);
        WRITE_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg);

        /* Deassert power down */
        CLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(clport_xgxs0_ctrl_reg, 0);
        WRITE_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);

        /* Reset XGXS */
        CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(clport_xgxs0_ctrl_reg, 0);
        WRITE_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);

        /* Bring XGXS out of reset */
        CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(clport_xgxs0_ctrl_reg, 1);
        WRITE_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg);
        sal_usleep(sleep_usec);
    }
}

static void
soc_gh2_tsc_reset(uint8 unit)
{
    uint8 lport;
    XLPORT_MAC_CONTROLr_t xlport_mac_control;
    CLPORT_MAC_CONTROLr_t clport_mac_control;
    PGW_CTRL_0r_t pgw_ctrl_0;

    /* Disable all serdes cores */
    READ_PGW_CTRL_0r(unit, pgw_ctrl_0);    
    
    PGW_CTRL_0r_SW_QTC_DISABLEf_SET(pgw_ctrl_0, 0x3);
    PGW_CTRL_0r_SW_PM4X10_DISABLEf_SET(pgw_ctrl_0, 0x7F);
    PGW_CTRL_0r_SW_PM4X25_DISABLEf_SET(pgw_ctrl_0, 0x1);
    WRITE_PGW_CTRL_0r(unit, pgw_ctrl_0);

    /* TSC reset */
    SOC_LPORT_ITER(lport) {
        if (IS_GX_PORT(lport)) {
            if (SOC_PMQ_BLOCK_INDEX(lport) == 0) {
                soc_tsc_xgxs_reset(unit, lport);
            }
        } else if (SOC_PORT_BLOCK_INDEX(lport) == 0) {
            soc_tsc_xgxs_reset(unit, lport);
        }
    }

    /* MAC reset */
    SOC_LPORT_ITER(lport) {
        if (IS_XL_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            READ_XLPORT_MAC_CONTROLr(unit, lport, xlport_mac_control);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_control, 1);
            WRITE_XLPORT_MAC_CONTROLr(unit, lport, xlport_mac_control);
            sal_usleep(10);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_control, 0);
            WRITE_XLPORT_MAC_CONTROLr(unit, lport, xlport_mac_control);
        }

        if (IS_CL_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            READ_CLPORT_MAC_CONTROLr(unit, lport, clport_mac_control);
            CLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(clport_mac_control, 1);
            WRITE_CLPORT_MAC_CONTROLr(unit, lport, clport_mac_control);
            sal_usleep(10);
            CLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(clport_mac_control, 0);
            WRITE_CLPORT_MAC_CONTROLr(unit, lport, clport_mac_control);
        }
    }
}

static int ceiling_func(uint32 numerators, uint32 denominator)
{
    uint32  result;
    if (denominator == 0) {
        return 0xFFFFFFFF;
    }
    result = numerators / denominator;
    if (numerators % denominator != 0) {
        result++;
    }
    return result;
}

static sys_error_t _soc_greyhound2_mmu_init_helper_lossy(int unit)
{
    int port, phy_port, mport;
    int index;
    pbmp_t pbmp_cpu;
    pbmp_t pbmp_uplink;
    pbmp_t pbmp_downlink_1g;
    pbmp_t pbmp_downlink_2dot5g;
    pbmp_t pbmp_all;

    int standard_jumbo_frame;
    int cell_size;
    int ethernet_mtu_cell;
    int standard_jumbo_frame_cell;
    int total_physical_memory;
    int total_cell_memory_for_admission;
    int number_of_used_memory_banks;
    int reserved_for_cfap;
    int skidmarker;
    int prefetch;
    int total_cell_memory;
    int cfapfullsetpoint;
    int total_advertised_cell_memory;
    int number_of_uplink_ports;
    int number_of_downlink_ports;
    int queue_port_limit_ratio;
    int egress_queue_min_reserve_uplink_ports_lossy;
    int egress_queue_min_reserve_downlink_ports_lossy;
    int egress_queue_min_reserve_uplink_ports_lossless;
    int egress_queue_min_reserve_downlink_ports_lossless;
    int egress_queue_min_reserve_cpu_ports;
    int egress_xq_min_reserve_lossy_ports;
    int egress_xq_min_reserve_lossless_uplink_ports;
    int egress_xq_min_reserve_lossless_downlink_ports;
    int num_active_pri_group_lossless;
    int num_lossy_queues;
    int mmu_xoff_pkt_threshold_uplink_ports;
    int mmu_xoff_pkt_threshold_downlink_ports;
    int mmu_xoff_cell_threshold_1g_port_downlink_ports;
    int mmu_xoff_cell_threshold_2dot5g_port_downlink_ports;
    int mmu_xoff_cell_threshold_all_uplink_ports;
    int num_cpu_queues;
    int num_cpu_ports;
    int numxqs_per_uplink_ports;
    int numxqs_per_downlink_ports_and_cpu_port;
    int headroom_for_1g_port;
    int headroom_for_2dot5g_port;
    int xoff_cell_thresholds_per_port_1g_port_downlink_ports;
    int xoff_cell_thresholds_per_port_2dot5g_downlink_ports;
    int xoff_cell_threshold_all_uplink_ports;
    int xoff_packet_thresholds_per_port_uplink_port;
    int xoff_packet_thresholds_per_port_downlink_port;
    int discard_limit_per_port_pg_uplink_1g_port;
    int discard_limit_per_port_pg_uplink_2dot5g_port;
    int discard_limit_per_port_pg_downlink_port;
    int total_reserved_cells_for_uplink_ports;
    int total_reserved_cells_for_downlink_ports;
    int total_reserved_cells_for_cpu_port;
    int total_reserved;
    int shared_space_cells;
    int reserved_xqs_per_uplink_port;
    int shared_xqs_per_uplink_port;
    int reserved_xqs_per_downlink_port;
    int shared_xqs_per_downlink_port;
    int cfapfullthreshold_cfapfullsetpoint_up;
    int gbllimitsetlimit_gblcellsetlimit_up;
    int totaldyncellsetlimit_totaldyncellsetlimit_up;
    int holcosminxqcnt_qlayer8_holcosminxqcnt_up;
    int holcospktsetlimit0_pktsetlimit_up;
    int holcospktsetlimit7_pktsetlimit_up;
    int holcospktsetlimit_qlayer0_pktsetlimit_up;
    int holcospktsetlimit_qlayer7_pktsetlimit_up;
    int holcospktsetlimit_qlayer8_pktsetlimit_up;
    int dynxqcntport_dynxqcntport_up;
    int lwmcoscellsetlimit0_cellsetlimit_up;
    int lwmcoscellsetlimit7_cellsetlimit_up;
    int lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
    int lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
    int lwmcoscellsetlimit_qlayer8_cellsetlimit_up;
    int holcoscellmaxlimit0_cellmaxlimit_up;
    int holcoscellmaxlimit7_cellmaxlimit_up;
    int holcoscellmaxlimit_qlayer0_cellmaxlimit_up;
    int holcoscellmaxlimit_qlayer7_cellmaxlimit_up;
    int holcoscellmaxlimit_qlayer8_cellmaxlimit_up;
    int dyncelllimit_dyncellsetlimit_up;
    int holcospktsetlimit_qgroup0_pktsetlimit_up;
    int holcoscellmaxlimit_qgroup0_cellmaxlimit_up;
    int holcosminxqcnt_qlayer8_holcosminxqcnt_down_1;
    int holcospktsetlimit0_pktsetlimit_down_1;
    int holcospktsetlimit7_pktsetlimit_down_1;
    int holcospktsetlimit_qlayer0_pktsetlimit_down_1;
    int holcospktsetlimit_qlayer7_pktsetlimit_down_1;
    int holcospktsetlimit_qlayer8_pktsetlimit_down_1;
    int dynxqcntport_dynxqcntport_down_1;
    int lwmcoscellsetlimit0_cellsetlimit_down_1;
    int lwmcoscellsetlimit7_cellsetlimit_down_1;
    int lwmcoscellsetlimit_qlayer0_cellsetlimit_down_1;
    int lwmcoscellsetlimit_qlayer7_cellsetlimit_down_1;
    int lwmcoscellsetlimit_qlayer8_cellsetlimit_down_1;
    int holcoscellmaxlimit0_cellmaxlimit_down_1;
    int holcoscellmaxlimit7_cellmaxlimit_down_1;
    int holcoscellmaxlimit_qlayer0_cellmaxlimit_down_1;
    int holcoscellmaxlimit_qlayer7_cellmaxlimit_down_1;
    int holcoscellmaxlimit_qlayer8_cellmaxlimit_down_1;
    int dyncelllimit_dyncellsetlimit_down_1;
    int holcospktsetlimit_qgroup0_pktsetlimit_down_1;
    int holcoscellmaxlimit_qgroup0_cellmaxlimit_down_1;
    int holcospktsetlimit0_pktsetlimit_down_2dot5;
    int holcospktsetlimit7_pktsetlimit_down_2dot5;
    int holcospktsetlimit_qlayer0_pktsetlimit_down_2dot5;
    int holcospktsetlimit_qlayer7_pktsetlimit_down_2dot5;
    int holcospktsetlimit_qlayer8_pktsetlimit_down_2dot5;
    int dynxqcntport_dynxqcntport_down_2dot5;
    int lwmcoscellsetlimit0_cellsetlimit_down_2dot5;
    int lwmcoscellsetlimit7_cellsetlimit_down_2dot5;
    int lwmcoscellsetlimit_qlayer0_cellsetlimit_down_2dot5;
    int lwmcoscellsetlimit_qlayer7_cellsetlimit_down_2dot5;
    int lwmcoscellsetlimit_qlayer8_cellsetlimit_down_2dot5;
    int holcoscellmaxlimit0_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit7_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit_qlayer0_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit_qlayer7_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit_qlayer8_cellmaxlimit_down_2dot5;
    int dyncelllimit_dyncellsetlimit_down_2dot5;
    int holcospktsetlimit_qgroup0_pktsetlimit_down_2dot5;
    int holcoscellmaxlimit_qgroup0_cellmaxlimit_down_2dot5;
    int holcosminxqcnt0_holcosminxqcnt_cpu;
    int holcosminxqcnt7_holcosminxqcnt_cpu;
    int holcospktsetlimit0_pktsetlimit_cpu;
    int holcospktsetlimit7_pktsetlimit_cpu;
    int dynxqcntport_dynxqcntport_cpu;
    int lwmcoscellsetlimit0_cellsetlimit_cpu;
    int lwmcoscellsetlimit7_cellsetlimit_cpu;
    int holcoscellmaxlimit0_cellmaxlimit_cpu;
    int holcoscellmaxlimit7_cellmaxlimit_cpu;
    int dyncelllimit_dyncellsetlimit_cpu;

    CFAPFULLTHRESHOLDr_t cfapfullthreshold;
    GBLLIMITSETLIMITr_t gbllimitsetlimit;
    GBLLIMITRESETLIMITr_t gbllimitresetlimit;
    TOTALDYNCELLSETLIMITr_t totaldyncellsetlimit;
    TOTALDYNCELLRESETLIMITr_t totaldyncellresetlimit;
    TWO_LAYER_SCH_MODEr_t two_layer_sch_mode;
    MISCCONFIGr_t miscconfig;
    MMUPORTTXENABLE_0r_t mmuporttxenable_0;
    MMUPORTTXENABLE_1r_t mmuporttxenable_1;
    MMUPORTTXENABLE_2r_t mmuporttxenable_2;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;    
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSMINXQCNT_QLAYERr_t holcosminxqcnt_qlayer;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTSETLIMIT_QLAYERr_t holcospktsetlimit_qlayer;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    HOLCOSPKTRESETLIMIT_QLAYERr_t holcospktresetlimit_qlayer;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    CNGCOSPKTLIMIT0_QLAYERr_t cngcospktlimit0_qlayer;
    CNGCOSPKTLIMIT1_QLAYERr_t cngcospktlimit1_qlayer;
    CNGPORTPKTLIMIT0r_t cngportpktlimit0;
    CNGPORTPKTLIMIT1r_t cngportpktlimit1;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    LWMCOSCELLSETLIMIT_QLAYERr_t lwmcoscellsetlimit_qlayer;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    HOLCOSCELLMAXLIMIT_QLAYERr_t holcoscellmaxlimit_qlayer;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_ENr_t color_drop_en;
    COLOR_DROP_EN_QLAYERr_t color_drop_en_qlayer;
    HOLCOSPKTSETLIMIT_QGROUPr_t holcospktsetlimit_qgroup;
    HOLCOSPKTRESETLIMIT_QGROUPr_t holcospktresetlimit_qgroup;
    CNGCOSPKTLIMIT0_QGROUPr_t cngcospktlimit0_qgroup;
    CNGCOSPKTLIMIT1_QGROUPr_t cngcospktlimit1_qgroup;
    HOLCOSCELLMAXLIMIT_QGROUPr_t holcoscellmaxlimit_qgroup;
    COLOR_DROP_EN_QGROUPr_t color_drop_en_qgroup;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;
    SHARED_POOL_CTRL_EXT1r_t shared_pool_ctrl_ext1;
    SHARED_POOL_CTRL_EXT2r_t shared_pool_ctrl_ext2;

    /* setup port bitmap according the port max speed for lossy
     *   TSC/TSCF    : uplink port
     *   QGMII/SGMII : downlink port
     */
    num_cpu_ports = 0;
    number_of_uplink_ports = 0;
    number_of_downlink_ports = 0;
    PBMP_CLEAR(pbmp_cpu);
    PBMP_CLEAR(pbmp_uplink);
    PBMP_CLEAR(pbmp_downlink_1g);
    PBMP_CLEAR(pbmp_downlink_2dot5g);
    PBMP_ASSIGN(pbmp_all, BCM5357X_ALL_PORTS_MASK);
    for (phy_port = 0; phy_port <= BCM5357X_PORT_MAX; phy_port++) {
        port = SOC_PORT_P2L_MAPPING(phy_port);
        if ((port == -1) || (SOC_PORT_L2P_MAPPING(port) == -1)) {
            continue;
        }
        if (IS_CPU_PORT(port)) {
            num_cpu_ports++;
            PBMP_PORT_ADD(pbmp_cpu, port);
        } else if ((IS_XL_PORT(port) && (SOC_PORT_SPEED_MAX(port) > 2500)) || IS_CL_PORT(port)) {
            number_of_uplink_ports++;
            PBMP_PORT_ADD(pbmp_uplink, port);
        } else if (IS_GE_PORT(port) || (IS_XL_PORT(port) && (SOC_PORT_SPEED_MAX(port) <= 2500))) {
            number_of_downlink_ports++;
            if (SOC_PORT_SPEED_MAX(port) > 1000) {
                PBMP_PORT_ADD(pbmp_downlink_2dot5g, port);
            } else {
                PBMP_PORT_ADD(pbmp_downlink_1g, port);
            }
        }
    }

    standard_jumbo_frame = 9216;
    cell_size = 144;
    ethernet_mtu_cell = ceiling_func(15 * 1024 / 10, cell_size);
    standard_jumbo_frame_cell = ceiling_func(standard_jumbo_frame, cell_size);
    if (gh2_sw_info.cbp_buffer_size == 4 * 1024 * 1024) {
        total_physical_memory = GH2_MMU_CBP_FULL_SIZE + 1;
    } else if (gh2_sw_info.cbp_buffer_size == 2 * 1024 * 1024) {
        total_physical_memory = GH2_MMU_CBP_HALF_SIZE + 1;
    } else if (gh2_sw_info.cbp_buffer_size == 1 * 1024 * 1024) {
        total_physical_memory = GH2_MMU_CBP_QUARTER_SIZE + 1;
    }
#ifdef GREYHOUND2_CHIP_A0 
    total_physical_memory = 24 * 1024; 
#endif
    total_cell_memory_for_admission = 225 * 1024 / 10;
    number_of_used_memory_banks = 8;
    reserved_for_cfap = (65) * 2 + number_of_used_memory_banks * 4;
    skidmarker = 7;
    prefetch = 64 + 4;
    total_cell_memory = total_cell_memory_for_admission;
    cfapfullsetpoint = total_physical_memory - reserved_for_cfap;
    total_advertised_cell_memory = total_cell_memory;
    queue_port_limit_ratio = 8;
    egress_queue_min_reserve_uplink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_downlink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_uplink_ports_lossless = 0;
    egress_queue_min_reserve_downlink_ports_lossless = 0;
    egress_queue_min_reserve_cpu_ports = ethernet_mtu_cell;
    egress_xq_min_reserve_lossy_ports
          = ethernet_mtu_cell;
    egress_xq_min_reserve_lossless_uplink_ports = 0;
    egress_xq_min_reserve_lossless_downlink_ports = 0;
    num_active_pri_group_lossless = 0;
    num_lossy_queues = 8;
    mmu_xoff_pkt_threshold_uplink_ports = total_advertised_cell_memory;
    mmu_xoff_pkt_threshold_downlink_ports = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_1g_port_downlink_ports
          = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_2dot5g_port_downlink_ports
          = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_all_uplink_ports = total_advertised_cell_memory;
    num_cpu_queues = 8;
    num_cpu_ports = 1;
    numxqs_per_uplink_ports = 6 * 1024;
    numxqs_per_downlink_ports_and_cpu_port = 2 * 1024;
    headroom_for_1g_port = 0;
    headroom_for_2dot5g_port = 0;
    xoff_cell_thresholds_per_port_1g_port_downlink_ports
          = mmu_xoff_cell_threshold_1g_port_downlink_ports;
    xoff_cell_thresholds_per_port_2dot5g_downlink_ports
          = mmu_xoff_cell_threshold_2dot5g_port_downlink_ports;
    xoff_cell_threshold_all_uplink_ports
          = mmu_xoff_cell_threshold_all_uplink_ports;
    xoff_packet_thresholds_per_port_uplink_port
          = mmu_xoff_pkt_threshold_uplink_ports;
    xoff_packet_thresholds_per_port_downlink_port
          = mmu_xoff_pkt_threshold_downlink_ports;
    discard_limit_per_port_pg_uplink_1g_port
        = xoff_cell_thresholds_per_port_1g_port_downlink_ports
          + headroom_for_1g_port;
    discard_limit_per_port_pg_uplink_2dot5g_port
        = xoff_cell_thresholds_per_port_2dot5g_downlink_ports
          + headroom_for_2dot5g_port;
    discard_limit_per_port_pg_downlink_port = total_advertised_cell_memory;
    total_reserved_cells_for_uplink_ports
        = egress_queue_min_reserve_uplink_ports_lossy
          * number_of_uplink_ports * num_lossy_queues
          + number_of_uplink_ports
          * egress_queue_min_reserve_uplink_ports_lossless
          * num_active_pri_group_lossless;
    total_reserved_cells_for_downlink_ports
        = number_of_downlink_ports
          * egress_queue_min_reserve_downlink_ports_lossy
          * (num_lossy_queues) + number_of_downlink_ports
          * egress_queue_min_reserve_downlink_ports_lossless
          * num_active_pri_group_lossless;
    total_reserved_cells_for_cpu_port
        = num_cpu_ports * egress_queue_min_reserve_cpu_ports
          * num_cpu_queues;
    total_reserved
        = total_reserved_cells_for_uplink_ports
          + total_reserved_cells_for_downlink_ports
          + total_reserved_cells_for_cpu_port;
    shared_space_cells = total_advertised_cell_memory - total_reserved;
    reserved_xqs_per_uplink_port
        = egress_xq_min_reserve_lossy_ports
          * num_lossy_queues + egress_xq_min_reserve_lossless_uplink_ports
          * num_active_pri_group_lossless;
    shared_xqs_per_uplink_port
          = numxqs_per_uplink_ports - reserved_xqs_per_uplink_port;
    reserved_xqs_per_downlink_port
        = egress_xq_min_reserve_lossy_ports
          * num_lossy_queues + egress_xq_min_reserve_lossless_downlink_ports
          * num_active_pri_group_lossless;
    shared_xqs_per_downlink_port
        = numxqs_per_downlink_ports_and_cpu_port
          - reserved_xqs_per_downlink_port;
    cfapfullthreshold_cfapfullsetpoint_up = cfapfullsetpoint;
    gbllimitsetlimit_gblcellsetlimit_up = total_cell_memory_for_admission;
    totaldyncellsetlimit_totaldyncellsetlimit_up = shared_space_cells;
    holcosminxqcnt_qlayer8_holcosminxqcnt_up = 0;
    holcospktsetlimit0_pktsetlimit_up
        = shared_xqs_per_uplink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit7_pktsetlimit_up
        = shared_xqs_per_uplink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer0_pktsetlimit_up
        = shared_xqs_per_uplink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer7_pktsetlimit_up
        = shared_xqs_per_uplink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer8_pktsetlimit_up
        = shared_xqs_per_uplink_port
          + holcosminxqcnt_qlayer8_holcosminxqcnt_up;
    dynxqcntport_dynxqcntport_up
          = shared_xqs_per_uplink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;
    lwmcoscellsetlimit7_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;
    lwmcoscellsetlimit_qlayer0_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;
    lwmcoscellsetlimit_qlayer7_cellsetlimit_up
          = egress_queue_min_reserve_uplink_ports_lossy;
    lwmcoscellsetlimit_qlayer8_cellsetlimit_up = 0;
    holcoscellmaxlimit0_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_up;
    holcoscellmaxlimit7_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit7_cellsetlimit_up;
    holcoscellmaxlimit_qlayer0_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
    holcoscellmaxlimit_qlayer7_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
    holcoscellmaxlimit_qlayer8_cellmaxlimit_up
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer8_cellsetlimit_up;
    dyncelllimit_dyncellsetlimit_up = shared_space_cells;
    holcospktsetlimit_qgroup0_pktsetlimit_up = numxqs_per_uplink_ports - 1;
    holcoscellmaxlimit_qgroup0_cellmaxlimit_up
        = ceiling_func(total_advertised_cell_memory, queue_port_limit_ratio);
    holcosminxqcnt_qlayer8_holcosminxqcnt_down_1 = 0;
    holcospktsetlimit0_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit7_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer0_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer7_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer8_pktsetlimit_down_1
        = shared_xqs_per_downlink_port
          + holcosminxqcnt_qlayer8_holcosminxqcnt_down_1;
    dynxqcntport_dynxqcntport_down_1
          = shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit7_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit_qlayer0_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit_qlayer7_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit_qlayer8_cellsetlimit_down_1 = 0;
    holcoscellmaxlimit0_cellmaxlimit_down_1
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_down_1;
    holcoscellmaxlimit7_cellmaxlimit_down_1
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit7_cellsetlimit_down_1;
    holcoscellmaxlimit_qlayer0_cellmaxlimit_down_1
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer0_cellsetlimit_down_1;
    holcoscellmaxlimit_qlayer7_cellmaxlimit_down_1
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer7_cellsetlimit_down_1;
    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_1
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer8_cellsetlimit_down_1;
    dyncelllimit_dyncellsetlimit_down_1 = shared_space_cells;
    holcospktsetlimit_qgroup0_pktsetlimit_down_1
          = numxqs_per_downlink_ports_and_cpu_port - 1;
    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_1
        = ceiling_func(total_advertised_cell_memory, queue_port_limit_ratio);
    holcospktsetlimit0_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit7_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer0_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer7_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer8_pktsetlimit_down_2dot5
        = shared_xqs_per_downlink_port
          + holcosminxqcnt_qlayer8_holcosminxqcnt_down_1;
    dynxqcntport_dynxqcntport_down_2dot5
          = shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit7_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit_qlayer0_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit_qlayer7_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit_qlayer8_cellsetlimit_down_2dot5 = 0;
    holcoscellmaxlimit0_cellmaxlimit_down_2dot5
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit0_cellsetlimit_down_2dot5;
    holcoscellmaxlimit7_cellmaxlimit_down_2dot5
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit7_cellsetlimit_down_2dot5;
    holcoscellmaxlimit_qlayer0_cellmaxlimit_down_2dot5
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer0_cellsetlimit_down_2dot5;
    holcoscellmaxlimit_qlayer7_cellmaxlimit_down_2dot5
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer7_cellsetlimit_down_2dot5;
    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_2dot5
        = ceiling_func(shared_space_cells, queue_port_limit_ratio)
          + lwmcoscellsetlimit_qlayer8_cellsetlimit_down_2dot5;
    dyncelllimit_dyncellsetlimit_down_2dot5 = shared_space_cells;
    holcospktsetlimit_qgroup0_pktsetlimit_down_2dot5
          = numxqs_per_downlink_ports_and_cpu_port - 1;
    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_2dot5
        = ceiling_func(total_advertised_cell_memory, queue_port_limit_ratio);
    holcosminxqcnt0_holcosminxqcnt_cpu = egress_queue_min_reserve_cpu_ports;
    holcosminxqcnt7_holcosminxqcnt_cpu = egress_queue_min_reserve_cpu_ports;
    holcospktsetlimit0_pktsetlimit_cpu =
              shared_xqs_per_downlink_port + holcosminxqcnt0_holcosminxqcnt_cpu;
    holcospktsetlimit7_pktsetlimit_cpu =
              shared_xqs_per_downlink_port + holcosminxqcnt7_holcosminxqcnt_cpu;
    dynxqcntport_dynxqcntport_cpu =
              shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_cpu = egress_queue_min_reserve_cpu_ports;
    lwmcoscellsetlimit7_cellsetlimit_cpu = egress_queue_min_reserve_cpu_ports;
    holcoscellmaxlimit0_cellmaxlimit_cpu =
              ceiling_func(shared_space_cells, queue_port_limit_ratio) +
              lwmcoscellsetlimit0_cellsetlimit_cpu;
    holcoscellmaxlimit7_cellmaxlimit_cpu =
              ceiling_func(shared_space_cells, queue_port_limit_ratio) +
              lwmcoscellsetlimit7_cellsetlimit_cpu;
    dyncelllimit_dyncellsetlimit_cpu = shared_space_cells;

    if ((shared_space_cells * cell_size)/1024 <= 800) {
        sal_printf("ERROR : Shared Pool Is Small, should be larger than 800 (value=%d)\n",
                 (shared_space_cells * cell_size)/1024);
        return SYS_ERR_PARAMETER;
    }

    /* system-based */
    READ_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);
    CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_SET(cfapfullthreshold, cfapfullsetpoint);
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_SET(cfapfullthreshold, 
                            cfapfullthreshold_cfapfullsetpoint_up -
                            (standard_jumbo_frame_cell * 2));
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    READ_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);
    GBLLIMITSETLIMITr_GBLCELLSETLIMITf_SET(gbllimitsetlimit, total_cell_memory_for_admission);
    WRITE_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);

    READ_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);
    GBLLIMITRESETLIMITr_GBLCELLRESETLIMITf_SET(gbllimitresetlimit, gbllimitsetlimit_gblcellsetlimit_up);
    WRITE_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);

    READ_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);
    TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_SET(totaldyncellsetlimit, shared_space_cells);
    WRITE_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);

    READ_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_SET(totaldyncellresetlimit, 
                            totaldyncellsetlimit_totaldyncellsetlimit_up -
                            (standard_jumbo_frame_cell * 2));
    WRITE_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);

    PBMP_ITER(pbmp_all, port) {        
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
        if (mport >= MMU_64Q_PPORT_BASE) {
            READ_TWO_LAYER_SCH_MODEr(unit, mport, two_layer_sch_mode);
            TWO_LAYER_SCH_MODEr_SCH_MODEf_SET(two_layer_sch_mode, 0);
            WRITE_TWO_LAYER_SCH_MODEr(unit, mport, two_layer_sch_mode);
        }
    }

    READ_MISCCONFIGr(unit, miscconfig);
    MISCCONFIGr_MULTIPLE_ACCOUNTING_FIX_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    MISCCONFIGr_CNG_DROP_ENf_SET(miscconfig, 0);
    WRITE_MISCCONFIGr(unit, miscconfig);

    MISCCONFIGr_DYN_XQ_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    MISCCONFIGr_DYNAMIC_MEMORY_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    MISCCONFIGr_SKIDMARKERf_SET(miscconfig, 3); /* 3 for skidmarker=7 */
    WRITE_MISCCONFIGr(unit, miscconfig);

    READ_MMUPORTTXENABLE_0r(unit, mmuporttxenable_0);
    MMUPORTTXENABLE_0r_MMUPORTTXENABLEf_SET(mmuporttxenable_0, 0xFFFFFFFF);
    WRITE_MMUPORTTXENABLE_0r(unit, mmuporttxenable_0);

    READ_MMUPORTTXENABLE_1r(unit, mmuporttxenable_1);
    MMUPORTTXENABLE_1r_MMUPORTTXENABLEf_SET(mmuporttxenable_1, 0xFFFFFFFF);
    WRITE_MMUPORTTXENABLE_1r(unit, mmuporttxenable_1);

    READ_MMUPORTTXENABLE_2r(unit, mmuporttxenable_2);
    MMUPORTTXENABLE_2r_MMUPORTTXENABLEf_SET(mmuporttxenable_2, 3);
    WRITE_MMUPORTTXENABLE_2r(unit, mmuporttxenable_2);

    /* port-based : uplink */
    PBMP_ITER(pbmp_uplink, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        if (mport < MMU_64Q_PPORT_BASE) {
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            }
        } else {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                    shared_xqs_per_uplink_port +
                                    egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            }
        } else {
            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                    shared_xqs_per_uplink_port +
                                    egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                    shared_xqs_per_uplink_port +
                                    holcosminxqcnt_qlayer8_holcosminxqcnt_up);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                    holcospktsetlimit0_pktsetlimit_up - 1);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            READ_HOLCOSPKTRESETLIMITr(unit, mport, 7, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                    holcospktsetlimit7_pktsetlimit_up - 1);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, 7, holcospktresetlimit);
        } else {
            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                    holcospktsetlimit_qlayer0_pktsetlimit_up - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, 7, holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                holcospktsetlimit_qlayer7_pktsetlimit_up - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, 7, holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                    holcospktsetlimit_qlayer8_pktsetlimit_up - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            }
        } else {
            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                             numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_uplink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_up - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                    egress_queue_min_reserve_uplink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    egress_queue_min_reserve_uplink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                    egress_queue_min_reserve_uplink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);            
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    egress_queue_min_reserve_uplink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit0_cellsetlimit_up);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            READ_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                ceiling_func(shared_space_cells,
                                             queue_port_limit_ratio) +
                                lwmcoscellsetlimit7_cellsetlimit_up);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit_qlayer0_cellsetlimit_up);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                ceiling_func(shared_space_cells,
                                             queue_port_limit_ratio) +
                                lwmcoscellsetlimit_qlayer7_cellsetlimit_up);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit_qlayer8_cellsetlimit_up);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                    holcoscellmaxlimit0_cellmaxlimit_up -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            READ_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit7_cellmaxlimit_up -
                                ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    holcoscellmaxlimit_qlayer0_cellmaxlimit_up -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                holcoscellmaxlimit_qlayer7_cellmaxlimit_up -
                                ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    holcoscellmaxlimit_qlayer8_cellmaxlimit_up -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_up -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                                0);
            WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);
        } else {
            /* COLOR_DROP_EN_QLAYERr, index 0 */
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 
                                0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(holcospktsetlimit_qgroup, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(holcospktresetlimit_qgroup, 
                                    holcospktsetlimit_qgroup0_pktsetlimit_up - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qgroup, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qgroup, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    ceiling_func(total_advertised_cell_memory,
                                                 queue_port_limit_ratio));
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    holcoscellmaxlimit_qgroup0_cellmaxlimit_up -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);

            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 
                                0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        }

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r, index 0 */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1, 
                                0xFFFFFF);        
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r, index 0 */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2, 
                                0xFFFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        }
    }

    /* port-based : downlink 1G */
    PBMP_ITER(pbmp_downlink_1g, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
    
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 7 */
        READ_PGCELLLIMITr(unit, mport, 7, pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_1g_port_downlink_ports);
        WRITE_PGCELLLIMITr(unit, mport, 7, pgcelllimit);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 7 */
        READ_PGCELLLIMITr(unit, mport, 7, pgcelllimit);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_1g_port_downlink_ports);
        WRITE_PGCELLLIMITr(unit, mport, 7, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* PGDISCARDSETLIMITr, index 7 */
        READ_PGDISCARDSETLIMITr(unit, mport, 7, pgdiscardsetlimit);
        PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_uplink_1g_port);
        WRITE_PGDISCARDSETLIMITr(unit, mport, 7, pgdiscardsetlimit);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNTr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            }
        } else {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                    shared_xqs_per_downlink_port +
                                    egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            }
        } else {
            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                    shared_xqs_per_downlink_port +
                                    egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                    shared_xqs_per_downlink_port +
                                    holcosminxqcnt_qlayer8_holcosminxqcnt_down_1);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                    holcospktsetlimit0_pktsetlimit_down_1 - 1);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);

            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            READ_HOLCOSPKTRESETLIMITr(unit, mport, 7, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                    holcospktsetlimit7_pktsetlimit_down_1 - 1);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, 7, holcospktresetlimit);
        } else {
            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                    holcospktsetlimit_qlayer0_pktsetlimit_down_1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, 7, holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                holcospktsetlimit_qlayer7_pktsetlimit_down_1 - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, 7, holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                    holcospktsetlimit_qlayer8_pktsetlimit_down_1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            }
        } else {
            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_down_1 - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                    egress_queue_min_reserve_downlink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    egress_queue_min_reserve_downlink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                    egress_queue_min_reserve_downlink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);            
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                     egress_queue_min_reserve_downlink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit0_cellsetlimit_down_1);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            READ_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                ceiling_func(shared_space_cells,
                                             queue_port_limit_ratio) +
                                lwmcoscellsetlimit7_cellsetlimit_down_1);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit_qlayer0_cellsetlimit_down_1);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                ceiling_func(shared_space_cells,
                                             queue_port_limit_ratio) +
                                lwmcoscellsetlimit_qlayer7_cellsetlimit_down_1);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit_qlayer8_cellsetlimit_down_1);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                    holcoscellmaxlimit0_cellmaxlimit_down_1 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            READ_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit7_cellmaxlimit_down_1 -
                                ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    holcoscellmaxlimit_qlayer0_cellmaxlimit_down_1 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                holcoscellmaxlimit_qlayer7_cellmaxlimit_down_1 -
                                ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_1 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_down_1 -
                            (ethernet_mtu_cell * 2));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                                0);
            WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);
        } else {
            /* COLOR_DROP_EN_QLAYERr, index 0 */
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 
                                0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(holcospktsetlimit_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(holcospktresetlimit_qgroup, 
                                    holcospktsetlimit_qgroup0_pktsetlimit_down_1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    ceiling_func(total_advertised_cell_memory,
                                                 queue_port_limit_ratio));
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_1 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 
                                0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        }

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r, index 0 */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1, 
                                0xFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r, index 0 */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2, 
                                0xFFFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        }
    }

    /* port-based : downlink 2.5G */
    PBMP_ITER(pbmp_downlink_2dot5g, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
    
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 7 */
        READ_PGCELLLIMITr(unit, mport, 7, pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_2dot5g_downlink_ports);
        WRITE_PGCELLLIMITr(unit, mport, 7, pgcelllimit);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 7 */
        READ_PGCELLLIMITr(unit, mport, 7, pgcelllimit);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_2dot5g_downlink_ports);
        WRITE_PGCELLLIMITr(unit, mport, 7, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* PGDISCARDSETLIMITr, index 7 */
        READ_PGDISCARDSETLIMITr(unit, mport, 7, pgdiscardsetlimit);
        PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_uplink_2dot5g_port);
        WRITE_PGDISCARDSETLIMITr(unit, mport, 7, pgdiscardsetlimit);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNTr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            }
        } else {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                    shared_xqs_per_downlink_port +
                                    egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            }
        } else {
            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                    shared_xqs_per_downlink_port +
                                    egress_xq_min_reserve_lossy_ports);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                    shared_xqs_per_downlink_port +
                                    holcosminxqcnt_qlayer8_holcosminxqcnt_down_1);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                    holcospktsetlimit0_pktsetlimit_down_2dot5 - 1);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            READ_HOLCOSPKTRESETLIMITr(unit, mport, 7, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                    holcospktsetlimit7_pktsetlimit_down_2dot5 - 1);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, 7, holcospktresetlimit);
        } else {
            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                    holcospktsetlimit_qlayer0_pktsetlimit_down_2dot5 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, 7, holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                holcospktsetlimit_qlayer7_pktsetlimit_down_2dot5 - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, 7, holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                    holcospktsetlimit_qlayer8_pktsetlimit_down_2dot5 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            }
        } else {
            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_down_2dot5 - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                    egress_queue_min_reserve_downlink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    egress_queue_min_reserve_downlink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                    egress_queue_min_reserve_downlink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);            
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    egress_queue_min_reserve_downlink_ports_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit0_cellsetlimit_down_2dot5);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            READ_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                ceiling_func(shared_space_cells,
                                             queue_port_limit_ratio) +
                                lwmcoscellsetlimit7_cellsetlimit_down_2dot5);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit_qlayer0_cellsetlimit_down_2dot5);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                ceiling_func(shared_space_cells,
                                             queue_port_limit_ratio) +
                                lwmcoscellsetlimit_qlayer7_cellsetlimit_down_2dot5);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit_qlayer8_cellsetlimit_down_2dot5);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                    holcoscellmaxlimit0_cellmaxlimit_down_2dot5 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            READ_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                holcoscellmaxlimit7_cellmaxlimit_down_2dot5 -
                                ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, 7, holcoscellmaxlimit);
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    holcoscellmaxlimit_qlayer0_cellmaxlimit_down_2dot5 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                holcoscellmaxlimit_qlayer7_cellmaxlimit_down_2dot5 -
                                ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, 7, holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_2dot5 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_down_2dot5 -
                            (ethernet_mtu_cell * 2));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                                0);
            WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);
        } else {
            /* COLOR_DROP_EN_QLAYERr, index 0 */
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 
                                0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(holcospktsetlimit_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(holcospktresetlimit_qgroup, 
                                    holcospktsetlimit_qgroup0_pktsetlimit_down_2dot5 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    ceiling_func(total_advertised_cell_memory,
                                                 queue_port_limit_ratio));
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_2dot5 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 
                                0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        }

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r, index 0 */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1, 
                                0xFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r, index 0 */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2, 
                                0xFFFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        }
    }

    /* port-based : cpu port (lport=pport=mport=0) */
    PBMP_ITER(pbmp_cpu, port) {
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, port, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, port, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, port, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, port, pg_ctrl1);

       /* PG2TCr, index 0 ~ 7 */
       for (index = 0; index <= 7; index++) {
           READ_PG2TCr(unit, port, index, pg2tc);
           PG2TCr_PG_BMPf_SET(pg2tc, 0);
           WRITE_PG2TCr(unit, port, index, pg2tc);
       }
       
       /* IBPPKTSETLIMITr, index 0 */
       READ_IBPPKTSETLIMITr(unit, port, ibppktsetlimit);
       IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_uplink_port);
       IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
       WRITE_IBPPKTSETLIMITr(unit, port, ibppktsetlimit);
       
       /* MMU_FC_RX_ENr, index 0 */
       READ_MMU_FC_RX_ENr(unit, port, mmu_fc_rx_en);
       MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
       WRITE_MMU_FC_RX_ENr(unit, port, mmu_fc_rx_en);
       
       /* MMU_FC_TX_ENr, index 0 */
       READ_MMU_FC_TX_ENr(unit, port, mmu_fc_tx_en);
       MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
       WRITE_MMU_FC_TX_ENr(unit, port, mmu_fc_tx_en);

       /* PGCELLLIMITr, index 0 ~ 7 */
       for (index = 0; index <= 7; index++) {
           READ_PGCELLLIMITr(unit, port, index, pgcelllimit);
           PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
           WRITE_PGCELLLIMITr(unit, port, index, pgcelllimit);
       }
       
       /* PGCELLLIMITr, index 0 ~ 7 */
       for (index = 0; index <= 7; index++) {
           READ_PGCELLLIMITr(unit, port, index, pgcelllimit);
           PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
           WRITE_PGCELLLIMITr(unit, port, index, pgcelllimit);
       }
       
       /* PGDISCARDSETLIMITr, index 0 ~ 7 */
       for (index = 0; index <= 7; index++) {
           READ_PGDISCARDSETLIMITr(unit, port, index, pgdiscardsetlimit);
           PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
           WRITE_PGDISCARDSETLIMITr(unit, port, index, pgdiscardsetlimit);
       }

       /* HOLCOSMINXQCNTr, index 0 ~ 7 */
       for (index = 0; index <= 7; index++) {
           READ_HOLCOSMINXQCNTr(unit, port, index, holcosminxqcnt);
           HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_queue_min_reserve_cpu_ports);
           WRITE_HOLCOSMINXQCNTr(unit, port, index, holcosminxqcnt);
       }
       
       /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
       for (index = 0; index <= 6; index++) {
           READ_HOLCOSPKTSETLIMITr(unit, port, index, holcospktsetlimit);
           HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                               shared_xqs_per_downlink_port +
                               holcosminxqcnt0_holcosminxqcnt_cpu);
           WRITE_HOLCOSPKTSETLIMITr(unit, port, index, holcospktsetlimit);
       }
       
       /* HOLCOSPKTSETLIMITr, index 7 */
       READ_HOLCOSPKTSETLIMITr(unit, port, 7, holcospktsetlimit);
       HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                               shared_xqs_per_downlink_port +
                               holcosminxqcnt7_holcosminxqcnt_cpu);
       WRITE_HOLCOSPKTSETLIMITr(unit, port, 7, holcospktsetlimit);

       /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
       for (index = 0; index <= 6; index++) {
           READ_HOLCOSPKTRESETLIMITr(unit, port, index, holcospktresetlimit);
           HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                               holcospktsetlimit0_pktsetlimit_cpu - 1);
           WRITE_HOLCOSPKTRESETLIMITr(unit, port, index, holcospktresetlimit);
       }
       
       /* HOLCOSPKTRESETLIMITr, index 7 */
       READ_HOLCOSPKTRESETLIMITr(unit, port, 7, holcospktresetlimit);
       HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                               holcospktsetlimit7_pktsetlimit_cpu - 1);
       WRITE_HOLCOSPKTRESETLIMITr(unit, port, 7, holcospktresetlimit);
       
       /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
       for (index = 0; index <= 7; index++) {
           READ_CNGCOSPKTLIMIT0r(unit, port, index, cngcospktlimit0);
           CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                               numxqs_per_downlink_ports_and_cpu_port - 1);
           WRITE_CNGCOSPKTLIMIT0r(unit, port, index, cngcospktlimit0);
       }
       
       /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
       for (index = 0; index <= 7; index++) {
           READ_CNGCOSPKTLIMIT1r(unit, port, index, cngcospktlimit1);
           CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                               numxqs_per_downlink_ports_and_cpu_port - 1);
           WRITE_CNGCOSPKTLIMIT1r(unit, port, index, cngcospktlimit1);
       }

#if 0 /* QLAYER is no need for cpu port */
       /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
       for (index = 0; index <= 63; index++) {
           READ_CNGCOSPKTLIMIT0_QLAYERr(unit, port, index, cngcospktlimit0_qlayer);
           CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer, 
                               numxqs_per_downlink_ports_and_cpu_port - 1);
           WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, port, index, cngcospktlimit0_qlayer);
       }
       
       /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
       for (index = 0; index <= 63; index++) {
           READ_CNGCOSPKTLIMIT1_QLAYERr(unit, port, index, cngcospktlimit1_qlayer);
           CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer, 
                               numxqs_per_downlink_ports_and_cpu_port - 1);
           WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, port, index, cngcospktlimit1_qlayer);
       }
#endif
       
       /* CNGPORTPKTLIMIT0r, index 0 */
       READ_CNGPORTPKTLIMIT0r(unit, port, cngportpktlimit0);
       CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                           numxqs_per_downlink_ports_and_cpu_port - 1);
       WRITE_CNGPORTPKTLIMIT0r(unit, port, cngportpktlimit0);
       
       /* CNGPORTPKTLIMIT1r, index 0 */
       READ_CNGPORTPKTLIMIT1r(unit, port, cngportpktlimit1);
       CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                           numxqs_per_downlink_ports_and_cpu_port - 1);
       WRITE_CNGPORTPKTLIMIT1r(unit, port, cngportpktlimit1);

       /* DYNXQCNTPORTr, index 0 */
       READ_DYNXQCNTPORTr(unit, port, dynxqcntport);
       DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                           shared_xqs_per_downlink_port - skidmarker - prefetch);
       WRITE_DYNXQCNTPORTr(unit, port, dynxqcntport);
       
       /* DYNRESETLIMPORTr, index 0 */
       READ_DYNRESETLIMPORTr(unit, port, dynresetlimport);
       DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                           dynxqcntport_dynxqcntport_cpu - 2);
       WRITE_DYNRESETLIMPORTr(unit, port, dynresetlimport);
       
       /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
       for (index = 0; index <= 7; index++) {
           READ_LWMCOSCELLSETLIMITr(unit, port, index, lwmcoscellsetlimit);
           LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                               egress_queue_min_reserve_cpu_ports);
           WRITE_LWMCOSCELLSETLIMITr(unit, port, index, lwmcoscellsetlimit);            
       }
       
       /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
       for (index = 0; index <= 7; index++) {
           READ_LWMCOSCELLSETLIMITr(unit, port, index, lwmcoscellsetlimit);
           LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                               egress_queue_min_reserve_cpu_ports);
           WRITE_LWMCOSCELLSETLIMITr(unit, port, index, lwmcoscellsetlimit);            
       }

       /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
       for (index = 0; index <= 6; index++) {
           READ_HOLCOSCELLMAXLIMITr(unit, port, index, holcoscellmaxlimit);
           HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                               ceiling_func(shared_space_cells,
                                            queue_port_limit_ratio) +
                               lwmcoscellsetlimit0_cellsetlimit_cpu);
           WRITE_HOLCOSCELLMAXLIMITr(unit, port, index, holcoscellmaxlimit);
       }
       
       /* HOLCOSCELLMAXLIMITr, index 7 */
       READ_HOLCOSCELLMAXLIMITr(unit, port, 7, holcoscellmaxlimit);
       HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                           ceiling_func(shared_space_cells,
                                      queue_port_limit_ratio) +
                           lwmcoscellsetlimit7_cellsetlimit_cpu);
       WRITE_HOLCOSCELLMAXLIMITr(unit, port, 7, holcoscellmaxlimit);

       /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
       for (index = 0; index <= 6; index++) {
           READ_HOLCOSCELLMAXLIMITr(unit, port, index, holcoscellmaxlimit);
           HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                               holcoscellmaxlimit0_cellmaxlimit_cpu -
                               ethernet_mtu_cell);
           WRITE_HOLCOSCELLMAXLIMITr(unit, port, index, holcoscellmaxlimit);
       }
       
       /* HOLCOSCELLMAXLIMITr, index 7 */
       READ_HOLCOSCELLMAXLIMITr(unit, port, 7, holcoscellmaxlimit);
       HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                           holcoscellmaxlimit7_cellmaxlimit_cpu -
                           ethernet_mtu_cell);
       WRITE_HOLCOSCELLMAXLIMITr(unit, port, 7, holcoscellmaxlimit);

       /* DYNCELLLIMITr, index 0 */
       READ_DYNCELLLIMITr(unit, port, dyncelllimit);
       DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                           shared_space_cells);
       DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                           dyncelllimit_dyncellsetlimit_cpu -
                           ethernet_mtu_cell * 2);
       WRITE_DYNCELLLIMITr(unit, port, dyncelllimit);
       
       /* COLOR_DROP_ENr, index 0 */
       READ_COLOR_DROP_ENr(unit, port, color_drop_en);
       COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                           0);
       WRITE_COLOR_DROP_ENr(unit, port, color_drop_en);
       
#if 0 /* QLAYER is no need for cpu port */
       /* COLOR_DROP_EN_QLAYERr, index 0 */
       READ_COLOR_DROP_EN_QLAYERr(unit, port, 0, color_drop_en_qlayer);
       COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 
                           0);
       WRITE_COLOR_DROP_EN_QLAYERr(unit, port, 0, color_drop_en_qlayer);
       
       /* COLOR_DROP_EN_QGROUPr, index 0 */
       READ_COLOR_DROP_EN_QGROUPr(unit, port, color_drop_en_qgroup);
       COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 
                           0);
       WRITE_COLOR_DROP_EN_QGROUPr(unit, port, color_drop_en_qgroup);
#endif

       /* SHARED_POOL_CTRLr, index 0 */
       READ_SHARED_POOL_CTRLr(unit, port, shared_pool_ctrl);
       SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                           255);
       SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                           255);
       SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                           0);
       WRITE_SHARED_POOL_CTRLr(unit, port, shared_pool_ctrl);
       
#if 0 /* QLAYER is no need for cpu port */
       /* SHARED_POOL_CTRL_EXT1r, index 0 */
       READ_SHARED_POOL_CTRL_EXT1r(unit, port, shared_pool_ctrl_ext1);
       SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1, 
                           0xFFFFFF);
       WRITE_SHARED_POOL_CTRL_EXT1r(unit, port, shared_pool_ctrl_ext1);
       
       /* SHARED_POOL_CTRL_EXT2r, index 0 */
       READ_SHARED_POOL_CTRL_EXT2r(unit, port, shared_pool_ctrl_ext2);
       SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2, 
                           0xFFFFFFFF);
       WRITE_SHARED_POOL_CTRL_EXT2r(unit, port, shared_pool_ctrl_ext2);
#endif
    }
    return SYS_OK;
}

static sys_error_t _soc_greyhound2_mmu_init_helper_lossless(int unit)
{
    int port, phy_port, mport;
    int index;
    pbmp_t pbmp_cpu;
    pbmp_t pbmp_uplink;
    pbmp_t pbmp_downlink_1g;
    pbmp_t pbmp_downlink_2dot5g;
    pbmp_t pbmp_downlink_10g;
    pbmp_t pbmp_all;

    int standard_jumbo_frame;
    int cell_size;
    int ethernet_mtu_cell;
    int standard_jumbo_frame_cell;
    int total_physical_memory;
    int total_cell_memory_for_admission;
    int number_of_used_memory_banks;
    int reserved_for_cfap;
    int skidmarker;
    int prefetch;
    int total_cell_memory;
    int cfapfullsetpoint;
    int total_advertised_cell_memory;
    int number_of_uplink_ports;
    int number_of_downlink_ports;
    int flow_control_type_pause_0_pfc_1;
    int queue_port_limit_ratio;
    int headroom_for_1g_port;
    int headroom_for_2dot5g_port;
    int headroom_for_10g_port;
    int headroom_for_20g_port;
    int headroom_for_25g_port;
    int headroom_for_40g_port;
    int num_1g_ports_downlink_ports;
    int num_2dot5g_ports_downlink_ports;
    int num_10g_ports_downlink_ports;
    int num_20g_ports_downlink_ports;
    int num_25g_ports_downlink_ports;
    int num_40g_ports_downlink_ports;
    int mmu_xoff_pkt_threshold_uplink_ports;
    int mmu_xoff_pkt_threshold_downlink_ports;
    int mmu_xoff_cell_threshold_1g_port_downlink_ports;
    int mmu_xoff_cell_threshold_2dot5g_port_downlink_ports;
    int mmu_xoff_cell_threshold_10g_port_downlink_ports;
    int mmu_xoff_cell_threshold_20g_port_downlink_ports;
    int mmu_xoff_cell_threshold_25g_port_downlink_ports;
    int mmu_xoff_cell_threshold_40g_port_downlink_ports;
    int mmu_xoff_cell_threshold_all_uplink_ports;
    int egress_xq_min_reserve_lossless_uplink_ports;
    int egress_xq_min_reserve_lossless_downlink_ports;
    int egress_xq_min_reserve_lossy_up_downlink_ports;
    int egress_queue_min_reserve_uplink_ports_lossy;
    int egress_queue_min_reserve_downlink_ports_lossy;
    int egress_queue_min_reserve_uplink_ports_lossless;
    int egress_queue_min_reserve_downlink_ports_lossless;
    int egress_queue_min_reserve_cpu_ports;
    int egress_xq_min_reserve_lossy_ports;
    int num_active_pri_group_lossless;
    int num_lossy_queues;
    int num_cpu_queues;
    int num_cpu_ports;
    int numxqs_per_uplink_ports;
    int numxqs_per_downlink_ports_and_cpu_port;
    int xoff_cell_thresholds_per_port_1g_port_downlink_ports;
    int xoff_cell_thresholds_per_port_2dot5g_downlink_ports;
    int xoff_cell_thresholds_per_port_10g_port_downlink_ports;
    int xoff_cell_threshold_all_uplink_ports;
    int xoff_packet_thresholds_per_port_uplink_port;
    int xoff_packet_thresholds_per_port_downlink_port;
    int discard_limit_per_port_pg_uplink_1g_port;
    int discard_limit_per_port_pg_uplink_2dot5g_port;
    int discard_limit_per_port_pg_uplink_10g_port;
    int discard_limit_per_port_pg_downlink_port;
    int total_reserved_cells_for_uplink_ports;
    int total_reserved_cells_for_downlink_ports;
    int total_reserved_cells_for_cpu_port;
    int total_reserved;
    int shared_space_cells;
    int reserved_xqs_per_uplink_port;
    int shared_xqs_per_uplink_port;
    int reserved_xqs_per_downlink_port;
    int shared_xqs_per_downlink_port;
    int cfapfullthreshold_cfapfullsetpoint_up;
    int gbllimitsetlimit_gblcellsetlimit_up;
    int totaldyncellsetlimit_totaldyncellsetlimit_up;
    int holcosminxqcnt_qlayer8_holcosminxqcnt_up;
    int holcospktsetlimit0_pktsetlimit_up;
    int holcospktsetlimit7_pktsetlimit_up;
    int holcospktsetlimit_qlayer0_pktsetlimit_up;
    int holcospktsetlimit_qlayer7_pktsetlimit_up;
    int holcospktsetlimit_qlayer8_pktsetlimit_up;
    int dynxqcntport_dynxqcntport_up;
    int lwmcoscellsetlimit0_cellsetlimit_up;
    int lwmcoscellsetlimit7_cellsetlimit_up;
    int lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
    int lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
    int lwmcoscellsetlimit_qlayer8_cellsetlimit_up;
    int holcoscellmaxlimit0_cellmaxlimit_up;
    int holcoscellmaxlimit7_cellmaxlimit_up;
    int holcoscellmaxlimit_qlayer0_cellmaxlimit_up;
    int holcoscellmaxlimit_qlayer7_cellmaxlimit_up;
    int holcoscellmaxlimit_qlayer8_cellmaxlimit_up;
    int dyncelllimit_dyncellsetlimit_up;
    int holcospktsetlimit_qgroup0_pktsetlimit_up;
    int holcoscellmaxlimit_qgroup0_cellmaxlimit_up;
    int holcosminxqcnt_qlayer8_holcosminxqcnt_down_1;
    int holcospktsetlimit0_pktsetlimit_down_1;
    int holcospktsetlimit7_pktsetlimit_down_1;
    int holcospktsetlimit_qlayer0_pktsetlimit_down_1;
    int holcospktsetlimit_qlayer7_pktsetlimit_down_1;
    int holcospktsetlimit_qlayer8_pktsetlimit_down_1;
    int dynxqcntport_dynxqcntport_down_1;
    int lwmcoscellsetlimit0_cellsetlimit_down_1;
    int lwmcoscellsetlimit7_cellsetlimit_down_1;
    int lwmcoscellsetlimit_qlayer0_cellsetlimit_down_1;
    int lwmcoscellsetlimit_qlayer7_cellsetlimit_down_1;
    int lwmcoscellsetlimit_qlayer8_cellsetlimit_down_1;
    int holcoscellmaxlimit0_cellmaxlimit_down_1;
    int holcoscellmaxlimit7_cellmaxlimit_down_1;
    int holcoscellmaxlimit_qlayer0_cellmaxlimit_down_1;
    int holcoscellmaxlimit_qlayer7_cellmaxlimit_down_1;
    int holcoscellmaxlimit_qlayer8_cellmaxlimit_down_1;
    int dyncelllimit_dyncellsetlimit_down_1;
    int holcospktsetlimit_qgroup0_pktsetlimit_down_1;
    int holcoscellmaxlimit_qgroup0_cellmaxlimit_down_1;
    int holcosminxqcnt_qlayer8_holcosminxqcnt_down_2dot5;
    int holcospktsetlimit0_pktsetlimit_down_2dot5;
    int holcospktsetlimit7_pktsetlimit_down_2dot5;
    int holcospktsetlimit_qlayer0_pktsetlimit_down_2dot5;
    int holcospktsetlimit_qlayer7_pktsetlimit_down_2dot5;
    int holcospktsetlimit_qlayer8_pktsetlimit_down_2dot5;
    int dynxqcntport_dynxqcntport_down_2dot5;
    int lwmcoscellsetlimit0_cellsetlimit_down_2dot5;
    int lwmcoscellsetlimit7_cellsetlimit_down_2dot5;
    int lwmcoscellsetlimit_qlayer0_cellsetlimit_down_2dot5;
    int lwmcoscellsetlimit_qlayer7_cellsetlimit_down_2dot5;
    int lwmcoscellsetlimit_qlayer8_cellsetlimit_down_2dot5;
    int holcoscellmaxlimit0_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit7_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit_qlayer0_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit_qlayer7_cellmaxlimit_down_2dot5;
    int holcoscellmaxlimit_qlayer8_cellmaxlimit_down_2dot5;
    int dyncelllimit_dyncellsetlimit_down_2dot5;
    int holcospktsetlimit_qgroup0_pktsetlimit_down_2dot5;
    int holcoscellmaxlimit_qgroup0_cellmaxlimit_down_2dot5;
    int holcosminxqcnt_qlayer8_holcosminxqcnt_down_10;
    int holcospktsetlimit0_pktsetlimit_down_10;
    int holcospktsetlimit7_pktsetlimit_down_10;
    int holcospktsetlimit_qlayer0_pktsetlimit_down_10;
    int holcospktsetlimit_qlayer7_pktsetlimit_down_10;
    int holcospktsetlimit_qlayer8_pktsetlimit_down_10;
    int dynxqcntport_dynxqcntport_down_10;
    int lwmcoscellsetlimit0_cellsetlimit_down_10;
    int lwmcoscellsetlimit7_cellsetlimit_down_10;
    int lwmcoscellsetlimit_qlayer0_cellsetlimit_down_10;
    int lwmcoscellsetlimit_qlayer7_cellsetlimit_down_10;
    int lwmcoscellsetlimit_qlayer8_cellsetlimit_down_10;
    int holcoscellmaxlimit0_cellmaxlimit_down_10;
    int holcoscellmaxlimit7_cellmaxlimit_down_10;
    int holcoscellmaxlimit_qlayer0_cellmaxlimit_down_10;
    int holcoscellmaxlimit_qlayer7_cellmaxlimit_down_10;
    int holcoscellmaxlimit_qlayer8_cellmaxlimit_down_10;
    int dyncelllimit_dyncellsetlimit_down_10;
    int holcospktsetlimit_qgroup0_pktsetlimit_down_10;
    int holcoscellmaxlimit_qgroup0_cellmaxlimit_down_10;
    int holcospktsetlimit0_pktsetlimit_cpu;
    int holcospktsetlimit7_pktsetlimit_cpu;
    int dynxqcntport_dynxqcntport_cpu;
    int lwmcoscellsetlimit0_cellsetlimit_cpu;
    int lwmcoscellsetlimit7_cellsetlimit_cpu;
    int holcoscellmaxlimit0_cellmaxlimit_cpu;
    int holcoscellmaxlimit7_cellmaxlimit_cpu;
    int dyncelllimit_dyncellsetlimit_cpu;

    CFAPFULLTHRESHOLDr_t cfapfullthreshold;
    GBLLIMITSETLIMITr_t gbllimitsetlimit;
    GBLLIMITRESETLIMITr_t gbllimitresetlimit;
    TOTALDYNCELLSETLIMITr_t totaldyncellsetlimit;
    TOTALDYNCELLRESETLIMITr_t totaldyncellresetlimit;
    TWO_LAYER_SCH_MODEr_t two_layer_sch_mode;
    MISCCONFIGr_t miscconfig;
    MMUPORTTXENABLE_0r_t mmuporttxenable_0;
    MMUPORTTXENABLE_1r_t mmuporttxenable_1;
    MMUPORTTXENABLE_2r_t mmuporttxenable_2;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;    
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSMINXQCNT_QLAYERr_t holcosminxqcnt_qlayer;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTSETLIMIT_QLAYERr_t holcospktsetlimit_qlayer;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    HOLCOSPKTRESETLIMIT_QLAYERr_t holcospktresetlimit_qlayer;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    CNGCOSPKTLIMIT0_QLAYERr_t cngcospktlimit0_qlayer;
    CNGCOSPKTLIMIT1_QLAYERr_t cngcospktlimit1_qlayer;
    CNGPORTPKTLIMIT0r_t cngportpktlimit0;
    CNGPORTPKTLIMIT1r_t cngportpktlimit1;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    LWMCOSCELLSETLIMIT_QLAYERr_t lwmcoscellsetlimit_qlayer;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    HOLCOSCELLMAXLIMIT_QLAYERr_t holcoscellmaxlimit_qlayer;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_ENr_t color_drop_en;
    COLOR_DROP_EN_QLAYERr_t color_drop_en_qlayer;
    HOLCOSPKTSETLIMIT_QGROUPr_t holcospktsetlimit_qgroup;
    HOLCOSPKTRESETLIMIT_QGROUPr_t holcospktresetlimit_qgroup;
    CNGCOSPKTLIMIT0_QGROUPr_t cngcospktlimit0_qgroup;
    CNGCOSPKTLIMIT1_QGROUPr_t cngcospktlimit1_qgroup;
    HOLCOSCELLMAXLIMIT_QGROUPr_t holcoscellmaxlimit_qgroup;
    COLOR_DROP_EN_QGROUPr_t color_drop_en_qgroup;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;
    SHARED_POOL_CTRL_EXT1r_t shared_pool_ctrl_ext1;
    SHARED_POOL_CTRL_EXT2r_t shared_pool_ctrl_ext2;

    /* setup port bitmap according the port max speed for lossless
     *   TSC4/TSC6/TSCF    : uplink port
     *   TSC0~3/TSC5/QGMII/SGMII : downlink port
     *     Note.
     *       1. the number of uplink port should not exceed 4
     *       2. cannot support downlink port with speed > 10G
     */
    num_cpu_ports = 0;
    number_of_uplink_ports = 0;
    number_of_downlink_ports = 0;
    num_1g_ports_downlink_ports = 0;
    num_2dot5g_ports_downlink_ports = 0;
    num_10g_ports_downlink_ports = 0;
    num_20g_ports_downlink_ports = 0;
    num_25g_ports_downlink_ports = 0;
    num_40g_ports_downlink_ports = 0;
    PBMP_CLEAR(pbmp_cpu);
    PBMP_CLEAR(pbmp_uplink);
    PBMP_CLEAR(pbmp_downlink_1g);
    PBMP_CLEAR(pbmp_downlink_2dot5g);
    PBMP_CLEAR(pbmp_downlink_10g);
    PBMP_ASSIGN(pbmp_all, BCM5357X_ALL_PORTS_MASK);
    for (phy_port = 0; phy_port <= BCM5357X_PORT_MAX; phy_port++) {
        port = SOC_PORT_P2L_MAPPING(phy_port);
        if ((port == -1) || (SOC_PORT_L2P_MAPPING(port) == -1)) {
            continue;
        }
        if (IS_CPU_PORT(port)) {
            num_cpu_ports++;
            PBMP_PORT_ADD(pbmp_cpu, port);
        } else if (IS_CL_PORT(port) ||
            (74 <= phy_port && phy_port <= 77) ||
            (82 <= phy_port && phy_port <= 85)) {
            number_of_uplink_ports++;
            PBMP_PORT_ADD(pbmp_uplink, port);
        } else if (IS_GE_PORT(port) ||
                   (58 <= phy_port && phy_port <= 73) ||
                   (78 <= phy_port && phy_port <= 81)) {
            number_of_downlink_ports++;
            if (SOC_PORT_SPEED_MAX(port) > 10000) {
                 sal_printf("ERROR : for downlink port %d, the max speed cannot exceed 10G (value=%d)\n",
                    port, SOC_PORT_SPEED_MAX(port));
                return SYS_ERR_PARAMETER;
            } else if (SOC_PORT_SPEED_MAX(port) > 2500) {
                num_10g_ports_downlink_ports++;
                PBMP_PORT_ADD(pbmp_downlink_10g, port);
            } else if (SOC_PORT_SPEED_MAX(port) > 1000) {
                num_2dot5g_ports_downlink_ports++;
                PBMP_PORT_ADD(pbmp_downlink_2dot5g, port);
            } else {
                num_1g_ports_downlink_ports++;
                PBMP_PORT_ADD(pbmp_downlink_1g, port);
            }
        }
    }
    if (number_of_uplink_ports > 4) {
         sal_printf("ERROR : Num of UplinkPorts should be less than or equal to 4 (value=%d)\n",
                 number_of_uplink_ports);
        return SYS_ERR_PARAMETER;
    }

    standard_jumbo_frame = 9216;
    cell_size = 144;
    ethernet_mtu_cell = ceiling_func(15 * 1024 / 10, cell_size);
    standard_jumbo_frame_cell =
          ceiling_func(standard_jumbo_frame, cell_size);
    if (gh2_sw_info.cbp_buffer_size == 4 * 1024 * 1024) {
        total_physical_memory = GH2_MMU_CBP_FULL_SIZE + 1;
    } else if (gh2_sw_info.cbp_buffer_size == 2 * 1024 * 1024) {
        total_physical_memory = GH2_MMU_CBP_HALF_SIZE + 1;
    } else if (gh2_sw_info.cbp_buffer_size == 1 * 1024 * 1024) {
        total_physical_memory = GH2_MMU_CBP_QUARTER_SIZE + 1;
    } 
#ifdef CHIP_GREYHOUND2_A0    
    total_physical_memory = 24 * 1024;
#endif    
    total_cell_memory_for_admission = 225 * 1024 / 10;;
    number_of_used_memory_banks = 8;
    reserved_for_cfap = 65 * 2 + number_of_used_memory_banks * 4;
    skidmarker = 7;
    prefetch = 64 + 4;
    total_cell_memory = total_cell_memory_for_admission;
    cfapfullsetpoint = total_physical_memory - reserved_for_cfap;
    total_advertised_cell_memory = total_cell_memory;
    flow_control_type_pause_0_pfc_1 = 0;
    queue_port_limit_ratio = 8;
    headroom_for_1g_port = 30;
    headroom_for_2dot5g_port = 34;
    headroom_for_10g_port = 74;
    headroom_for_20g_port = 92;
    headroom_for_25g_port = 100;
    headroom_for_40g_port = 186;
    mmu_xoff_pkt_threshold_uplink_ports = total_advertised_cell_memory;
    mmu_xoff_pkt_threshold_downlink_ports = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_1g_port_downlink_ports = headroom_for_1g_port;
    mmu_xoff_cell_threshold_2dot5g_port_downlink_ports =
          headroom_for_2dot5g_port;
    mmu_xoff_cell_threshold_10g_port_downlink_ports = headroom_for_10g_port;
    mmu_xoff_cell_threshold_20g_port_downlink_ports = headroom_for_20g_port;
    mmu_xoff_cell_threshold_25g_port_downlink_ports = headroom_for_25g_port;
    mmu_xoff_cell_threshold_40g_port_downlink_ports = headroom_for_40g_port;
    mmu_xoff_cell_threshold_all_uplink_ports = total_advertised_cell_memory;
    egress_xq_min_reserve_lossless_uplink_ports =
          (headroom_for_1g_port +
          mmu_xoff_cell_threshold_1g_port_downlink_ports) *
          num_1g_ports_downlink_ports + (headroom_for_2dot5g_port +
          mmu_xoff_cell_threshold_2dot5g_port_downlink_ports) *
          num_2dot5g_ports_downlink_ports + (headroom_for_10g_port +
          mmu_xoff_cell_threshold_10g_port_downlink_ports) *
          num_10g_ports_downlink_ports + (headroom_for_20g_port +
          mmu_xoff_cell_threshold_20g_port_downlink_ports) *
          num_20g_ports_downlink_ports + (headroom_for_25g_port +
          mmu_xoff_cell_threshold_25g_port_downlink_ports) *
          num_25g_ports_downlink_ports + (headroom_for_40g_port +
          mmu_xoff_cell_threshold_40g_port_downlink_ports) *
          num_40g_ports_downlink_ports;
    egress_xq_min_reserve_lossless_downlink_ports = 0;
    egress_xq_min_reserve_lossy_up_downlink_ports = ethernet_mtu_cell;
    egress_queue_min_reserve_uplink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_downlink_ports_lossy = ethernet_mtu_cell;
    egress_queue_min_reserve_uplink_ports_lossless =
          egress_xq_min_reserve_lossless_uplink_ports;
    egress_queue_min_reserve_downlink_ports_lossless = 0;
    egress_queue_min_reserve_cpu_ports = ethernet_mtu_cell;
    egress_xq_min_reserve_lossy_ports =
          ethernet_mtu_cell;
    num_active_pri_group_lossless = 1;
    num_lossy_queues = 7;
    num_cpu_queues = 8;
    num_cpu_ports = 1;
    numxqs_per_uplink_ports = 6 * 1024;
    numxqs_per_downlink_ports_and_cpu_port = 2 * 1024;
    xoff_cell_thresholds_per_port_1g_port_downlink_ports =
          mmu_xoff_cell_threshold_1g_port_downlink_ports;
    xoff_cell_thresholds_per_port_2dot5g_downlink_ports =
          mmu_xoff_cell_threshold_2dot5g_port_downlink_ports;
    xoff_cell_thresholds_per_port_10g_port_downlink_ports =
          mmu_xoff_cell_threshold_10g_port_downlink_ports;
    xoff_cell_threshold_all_uplink_ports =
          mmu_xoff_cell_threshold_all_uplink_ports;
    xoff_packet_thresholds_per_port_uplink_port =
          mmu_xoff_pkt_threshold_uplink_ports;
    xoff_packet_thresholds_per_port_downlink_port =
          mmu_xoff_pkt_threshold_downlink_ports;
    discard_limit_per_port_pg_uplink_1g_port =
          xoff_cell_thresholds_per_port_1g_port_downlink_ports +
          headroom_for_1g_port;
    discard_limit_per_port_pg_uplink_2dot5g_port =
          xoff_cell_thresholds_per_port_2dot5g_downlink_ports +
          headroom_for_2dot5g_port;
    discard_limit_per_port_pg_uplink_10g_port =
          xoff_cell_thresholds_per_port_10g_port_downlink_ports +
          headroom_for_10g_port;
    discard_limit_per_port_pg_downlink_port = total_advertised_cell_memory;
    total_reserved_cells_for_uplink_ports =
          egress_queue_min_reserve_uplink_ports_lossy *
          number_of_uplink_ports * num_lossy_queues + number_of_uplink_ports *
          egress_queue_min_reserve_uplink_ports_lossless *
          num_active_pri_group_lossless;
    total_reserved_cells_for_downlink_ports =
          number_of_downlink_ports *
          egress_queue_min_reserve_downlink_ports_lossy * (num_lossy_queues) +
          number_of_downlink_ports *
          egress_queue_min_reserve_downlink_ports_lossless;
    total_reserved_cells_for_cpu_port =
          num_cpu_ports * egress_queue_min_reserve_cpu_ports * num_cpu_queues;
    total_reserved =
          total_reserved_cells_for_uplink_ports +
          total_reserved_cells_for_downlink_ports +
          total_reserved_cells_for_cpu_port;
    shared_space_cells = total_advertised_cell_memory - total_reserved;
    reserved_xqs_per_uplink_port =
          egress_xq_min_reserve_lossy_ports *
          num_lossy_queues + egress_xq_min_reserve_lossless_uplink_ports;
    shared_xqs_per_uplink_port =
          numxqs_per_uplink_ports - reserved_xqs_per_uplink_port;
    reserved_xqs_per_downlink_port =
          egress_xq_min_reserve_lossy_ports *
          num_lossy_queues + egress_xq_min_reserve_lossless_downlink_ports;
    shared_xqs_per_downlink_port =
          numxqs_per_downlink_ports_and_cpu_port -
          reserved_xqs_per_downlink_port;
    cfapfullthreshold_cfapfullsetpoint_up = cfapfullsetpoint;
    gbllimitsetlimit_gblcellsetlimit_up = total_cell_memory_for_admission;
    totaldyncellsetlimit_totaldyncellsetlimit_up = shared_space_cells;
    holcosminxqcnt_qlayer8_holcosminxqcnt_up = 0;
    holcospktsetlimit0_pktsetlimit_up =
          shared_xqs_per_uplink_port +
          egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit7_pktsetlimit_up =
          shared_xqs_per_uplink_port +
          egress_xq_min_reserve_lossless_uplink_ports;
    holcospktsetlimit_qlayer0_pktsetlimit_up =
          shared_xqs_per_uplink_port +
          egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer7_pktsetlimit_up =
          shared_xqs_per_uplink_port +
          egress_xq_min_reserve_lossless_uplink_ports;
    holcospktsetlimit_qlayer8_pktsetlimit_up =
          shared_xqs_per_uplink_port +
          holcosminxqcnt_qlayer8_holcosminxqcnt_up;
    dynxqcntport_dynxqcntport_up =
          shared_xqs_per_uplink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_up =
          egress_queue_min_reserve_uplink_ports_lossy;
    lwmcoscellsetlimit7_cellsetlimit_up =
          egress_queue_min_reserve_uplink_ports_lossless;
    lwmcoscellsetlimit_qlayer0_cellsetlimit_up =
          egress_queue_min_reserve_uplink_ports_lossy;
    lwmcoscellsetlimit_qlayer7_cellsetlimit_up =
          egress_queue_min_reserve_uplink_ports_lossless;
    lwmcoscellsetlimit_qlayer8_cellsetlimit_up = 0;
    holcoscellmaxlimit0_cellmaxlimit_up =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit0_cellsetlimit_up;
    holcoscellmaxlimit7_cellmaxlimit_up =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit7_cellsetlimit_up;
    holcoscellmaxlimit_qlayer0_cellmaxlimit_up =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer0_cellsetlimit_up;
    holcoscellmaxlimit_qlayer7_cellmaxlimit_up =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer7_cellsetlimit_up;
    holcoscellmaxlimit_qlayer8_cellmaxlimit_up =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer8_cellsetlimit_up;
    dyncelllimit_dyncellsetlimit_up = shared_space_cells;
    holcospktsetlimit_qgroup0_pktsetlimit_up = numxqs_per_uplink_ports - 1;
    holcoscellmaxlimit_qgroup0_cellmaxlimit_up =
          ceiling_func(total_advertised_cell_memory, queue_port_limit_ratio);
    holcosminxqcnt_qlayer8_holcosminxqcnt_down_1 = 0;
    holcospktsetlimit0_pktsetlimit_down_1 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit7_pktsetlimit_down_1 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossless_downlink_ports;
    holcospktsetlimit_qlayer0_pktsetlimit_down_1 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer7_pktsetlimit_down_1 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossless_downlink_ports;
    holcospktsetlimit_qlayer8_pktsetlimit_down_1 =
          shared_xqs_per_downlink_port +
          holcosminxqcnt_qlayer8_holcosminxqcnt_down_1;
    dynxqcntport_dynxqcntport_down_1 =
          shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit7_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossless;
    lwmcoscellsetlimit_qlayer0_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit_qlayer7_cellsetlimit_down_1 =
              egress_queue_min_reserve_downlink_ports_lossless;
    lwmcoscellsetlimit_qlayer8_cellsetlimit_down_1 = 0;
    holcoscellmaxlimit0_cellmaxlimit_down_1 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit0_cellsetlimit_down_1;
    holcoscellmaxlimit7_cellmaxlimit_down_1 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit7_cellsetlimit_down_1;
    holcoscellmaxlimit_qlayer0_cellmaxlimit_down_1 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer0_cellsetlimit_down_1;
    holcoscellmaxlimit_qlayer7_cellmaxlimit_down_1 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer7_cellsetlimit_down_1;
    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_1 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer8_cellsetlimit_down_1;
    dyncelllimit_dyncellsetlimit_down_1 = shared_space_cells;
    holcospktsetlimit_qgroup0_pktsetlimit_down_1 =
          numxqs_per_downlink_ports_and_cpu_port - 1;
    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_1 =
          ceiling_func(total_advertised_cell_memory, queue_port_limit_ratio);
    holcosminxqcnt_qlayer8_holcosminxqcnt_down_2dot5 = 0;
    holcospktsetlimit0_pktsetlimit_down_2dot5 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit7_pktsetlimit_down_2dot5 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossless_downlink_ports;
    holcospktsetlimit_qlayer0_pktsetlimit_down_2dot5 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer7_pktsetlimit_down_2dot5 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossless_downlink_ports;
    holcospktsetlimit_qlayer8_pktsetlimit_down_2dot5 =
          shared_xqs_per_downlink_port +
          holcosminxqcnt_qlayer8_holcosminxqcnt_down_2dot5;
    dynxqcntport_dynxqcntport_down_2dot5 =
          shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit7_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossless;
    lwmcoscellsetlimit_qlayer0_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit_qlayer7_cellsetlimit_down_2dot5 =
              egress_queue_min_reserve_downlink_ports_lossless;
    lwmcoscellsetlimit_qlayer8_cellsetlimit_down_2dot5 = 0;
    holcoscellmaxlimit0_cellmaxlimit_down_2dot5 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit0_cellsetlimit_down_2dot5;
    holcoscellmaxlimit7_cellmaxlimit_down_2dot5 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit7_cellsetlimit_down_2dot5;
    holcoscellmaxlimit_qlayer0_cellmaxlimit_down_2dot5 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer0_cellsetlimit_down_2dot5;
    holcoscellmaxlimit_qlayer7_cellmaxlimit_down_2dot5 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer7_cellsetlimit_down_2dot5;
    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_2dot5 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer8_cellsetlimit_down_2dot5;
    dyncelllimit_dyncellsetlimit_down_2dot5 = shared_space_cells;
    holcospktsetlimit_qgroup0_pktsetlimit_down_2dot5 =
          numxqs_per_downlink_ports_and_cpu_port - 1;
    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_2dot5 =
          ceiling_func(total_advertised_cell_memory, queue_port_limit_ratio);
    holcosminxqcnt_qlayer8_holcosminxqcnt_down_10 = 0;
    holcospktsetlimit0_pktsetlimit_down_10 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit7_pktsetlimit_down_10 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossless_downlink_ports;
    holcospktsetlimit_qlayer0_pktsetlimit_down_10 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit_qlayer7_pktsetlimit_down_10 =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossless_downlink_ports;
    holcospktsetlimit_qlayer8_pktsetlimit_down_10 =
          shared_xqs_per_downlink_port +
          holcosminxqcnt_qlayer8_holcosminxqcnt_down_10;
    dynxqcntport_dynxqcntport_down_10 =
          shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_down_10 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit7_cellsetlimit_down_10 =
              egress_queue_min_reserve_downlink_ports_lossless;
    lwmcoscellsetlimit_qlayer0_cellsetlimit_down_10 =
              egress_queue_min_reserve_downlink_ports_lossy;
    lwmcoscellsetlimit_qlayer7_cellsetlimit_down_10 =
              egress_queue_min_reserve_downlink_ports_lossless;
    lwmcoscellsetlimit_qlayer8_cellsetlimit_down_10 = 0;
    holcoscellmaxlimit0_cellmaxlimit_down_10 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit0_cellsetlimit_down_10;
    holcoscellmaxlimit7_cellmaxlimit_down_10 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit7_cellsetlimit_down_10;
    holcoscellmaxlimit_qlayer0_cellmaxlimit_down_10 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer0_cellsetlimit_down_10;
    holcoscellmaxlimit_qlayer7_cellmaxlimit_down_10 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer7_cellsetlimit_down_10;
    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_10 =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit_qlayer8_cellsetlimit_down_10;
    dyncelllimit_dyncellsetlimit_down_10 = shared_space_cells;
    holcospktsetlimit_qgroup0_pktsetlimit_down_10 =
          numxqs_per_downlink_ports_and_cpu_port - 1;
    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_10 =
          ceiling_func(total_advertised_cell_memory, queue_port_limit_ratio);
    holcospktsetlimit0_pktsetlimit_cpu =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossy_ports;
    holcospktsetlimit7_pktsetlimit_cpu =
          shared_xqs_per_downlink_port +
          egress_xq_min_reserve_lossless_downlink_ports;
    dynxqcntport_dynxqcntport_cpu =
          shared_xqs_per_downlink_port - skidmarker - prefetch;
    lwmcoscellsetlimit0_cellsetlimit_cpu = egress_queue_min_reserve_cpu_ports;
    lwmcoscellsetlimit7_cellsetlimit_cpu = egress_queue_min_reserve_cpu_ports;
    holcoscellmaxlimit0_cellmaxlimit_cpu =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit0_cellsetlimit_cpu;
    holcoscellmaxlimit7_cellmaxlimit_cpu =
          ceiling_func(shared_space_cells, queue_port_limit_ratio) +
          lwmcoscellsetlimit7_cellsetlimit_cpu;
    dyncelllimit_dyncellsetlimit_cpu = shared_space_cells;

    if ((shared_space_cells * cell_size)/1024 <= 800) {
        sal_printf("ERROR : Shared Pool Is Small, should be larger than 800 (value=%d)\n",
                 (shared_space_cells * cell_size)/1024);
        return SYS_ERR_PARAMETER;
    }

    /* system-based */
    READ_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);
    CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_SET(cfapfullthreshold, cfapfullsetpoint);
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_SET(cfapfullthreshold, 
                            cfapfullthreshold_cfapfullsetpoint_up -
                            (standard_jumbo_frame_cell * 2));
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    READ_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);
    GBLLIMITSETLIMITr_GBLCELLSETLIMITf_SET(gbllimitsetlimit, total_cell_memory_for_admission);
    WRITE_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);

    READ_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);
    GBLLIMITRESETLIMITr_GBLCELLRESETLIMITf_SET(gbllimitresetlimit, gbllimitsetlimit_gblcellsetlimit_up);
    WRITE_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);

    READ_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);
    TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_SET(totaldyncellsetlimit, shared_space_cells);
    WRITE_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);

    READ_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_SET(totaldyncellresetlimit, 
                            totaldyncellsetlimit_totaldyncellsetlimit_up -
                            (standard_jumbo_frame_cell * 2));
    WRITE_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);

    PBMP_ITER(pbmp_all, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
        if (mport >= MMU_64Q_PPORT_BASE) {
            READ_TWO_LAYER_SCH_MODEr(unit, mport, two_layer_sch_mode);
            TWO_LAYER_SCH_MODEr_SCH_MODEf_SET(two_layer_sch_mode, 0);
            WRITE_TWO_LAYER_SCH_MODEr(unit, mport, two_layer_sch_mode);
        }
    }

    READ_MISCCONFIGr(unit, miscconfig);
    MISCCONFIGr_MULTIPLE_ACCOUNTING_FIX_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    MISCCONFIGr_CNG_DROP_ENf_SET(miscconfig, 0);
    WRITE_MISCCONFIGr(unit, miscconfig);

    MISCCONFIGr_DYN_XQ_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    MISCCONFIGr_DYNAMIC_MEMORY_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    MISCCONFIGr_SKIDMARKERf_SET(miscconfig, 3); /* 3 for skidmarker=7 */
    WRITE_MISCCONFIGr(unit, miscconfig);

    READ_MMUPORTTXENABLE_0r(unit, mmuporttxenable_0);
    MMUPORTTXENABLE_0r_MMUPORTTXENABLEf_SET(mmuporttxenable_0, 0xFFFFFFFF);
    WRITE_MMUPORTTXENABLE_0r(unit, mmuporttxenable_0);

    READ_MMUPORTTXENABLE_1r(unit, mmuporttxenable_1);
    MMUPORTTXENABLE_1r_MMUPORTTXENABLEf_SET(mmuporttxenable_1, 0xFFFFFFFF);
    WRITE_MMUPORTTXENABLE_1r(unit, mmuporttxenable_1);

    READ_MMUPORTTXENABLE_2r(unit, mmuporttxenable_2);
    MMUPORTTXENABLE_2r_MMUPORTTXENABLEf_SET(mmuporttxenable_2, 3);
    WRITE_MMUPORTTXENABLE_2r(unit, mmuporttxenable_2);

    /* port-based : uplink */
    PBMP_ITER(pbmp_uplink, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
    
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?0:7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?1:7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?2:7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?3:7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?4:7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?5:7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?6:7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?7:7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNTr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossless_uplink_ports);
                } else {
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            }
        } else {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossless_uplink_ports);
                } else {
                    HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                            shared_xqs_per_uplink_port +
                                            egress_xq_min_reserve_lossless_uplink_ports);
                } else {
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                        shared_xqs_per_uplink_port +
                                        egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            }
        } else {
            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                            shared_xqs_per_uplink_port +
                                            egress_xq_min_reserve_lossless_uplink_ports);
                } else {
                    HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                        shared_xqs_per_uplink_port +
                                        egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                    shared_xqs_per_uplink_port +
                                    holcosminxqcnt_qlayer8_holcosminxqcnt_up);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                            holcospktsetlimit7_pktsetlimit_up - 1);
                } else {
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                        holcospktsetlimit0_pktsetlimit_up - 1);
                }
                WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            }
        } else {
            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                        holcospktsetlimit_qlayer7_pktsetlimit_up - 1);
                } else {
                    HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                        holcospktsetlimit_qlayer0_pktsetlimit_up - 1);
                }
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                    holcospktsetlimit_qlayer8_pktsetlimit_up - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            }
        } else {
            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                            numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_uplink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_up - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_uplink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_uplink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);            
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_uplink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_uplink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_uplink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_uplink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);            
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_uplink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_uplink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                        ceiling_func(shared_space_cells,
                                                   queue_port_limit_ratio) +
                                        lwmcoscellsetlimit7_cellsetlimit_up);
                } else {
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit0_cellsetlimit_up);
                }
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit_qlayer7_cellsetlimit_up);
                } else {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit_qlayer0_cellsetlimit_up);
                }
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    ceiling_func(shared_space_cells,
                                                queue_port_limit_ratio) +
                                    lwmcoscellsetlimit_qlayer8_cellsetlimit_up);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                        holcoscellmaxlimit7_cellmaxlimit_up -
                                        ethernet_mtu_cell);
                } else {
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                        holcoscellmaxlimit0_cellmaxlimit_up -
                                        ethernet_mtu_cell);
                }
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        holcoscellmaxlimit_qlayer7_cellmaxlimit_up -
                                        ethernet_mtu_cell);
                } else {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        holcoscellmaxlimit_qlayer0_cellmaxlimit_up -
                                        ethernet_mtu_cell);
                }
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    holcoscellmaxlimit_qlayer8_cellmaxlimit_up -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_up -
                            (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                                0);
            WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);
        } else {
            /* COLOR_DROP_EN_QLAYERr, index 0 */
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 
                                0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(holcospktsetlimit_qgroup, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(holcospktresetlimit_qgroup, 
                                    holcospktsetlimit_qgroup0_pktsetlimit_up - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qgroup, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qgroup, 
                                    numxqs_per_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    ceiling_func(total_advertised_cell_memory,
                                               queue_port_limit_ratio));
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    holcoscellmaxlimit_qgroup0_cellmaxlimit_up -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);

            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 
                                0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        }

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            127);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r, index 0 */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1, 
                                0xFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r, index 0 */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2, 
                                0xFFFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        }
    }

    /* port-based : downlink 1G */
    PBMP_ITER(pbmp_downlink_1g, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
    
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 128);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?0:7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?1:7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?2:7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?3:7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?4:7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?5:7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?6:7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?7:7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            if (index == 7) {
                PG2TCr_PG_BMPf_SET(pg2tc, flow_control_type_pause_0_pfc_1?128:255);
            } else {
                PG2TCr_PG_BMPf_SET(pg2tc, 0);
            }
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 128);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 128);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if (index == 7) {
                PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_1g_port_downlink_ports);
            } else {
                PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if (index == 7) {
                PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_1g_port_downlink_ports);
            } else {
                PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            if (index == 7) {
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_uplink_1g_port);
            } else {
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            }
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNTr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            }
        } else {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                            shared_xqs_per_downlink_port +
                                            egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                        shared_xqs_per_downlink_port +
                                        egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            }
        } else {
            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                            shared_xqs_per_downlink_port +
                                            egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                        shared_xqs_per_downlink_port +
                                        egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                    shared_xqs_per_downlink_port +
                                    holcosminxqcnt_qlayer8_holcosminxqcnt_down_1);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                            holcospktsetlimit7_pktsetlimit_down_1 - 1);
                } else {
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                        holcospktsetlimit0_pktsetlimit_down_1 - 1);
                }
                WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);

            }
        } else {
            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                        holcospktsetlimit_qlayer7_pktsetlimit_down_1 - 1);
                } else {
                    HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                        holcospktsetlimit_qlayer0_pktsetlimit_down_1 - 1);
                }
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                    holcospktsetlimit_qlayer8_pktsetlimit_down_1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            }
        } else {
            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_down_1 - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);            
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit7_cellsetlimit_down_1);
                } else {
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit0_cellsetlimit_down_1);
                }
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit_qlayer7_cellsetlimit_down_1);
                } else {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit_qlayer0_cellsetlimit_down_1);
                }
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    ceiling_func(shared_space_cells,
                                                queue_port_limit_ratio) +
                                    lwmcoscellsetlimit_qlayer8_cellsetlimit_down_1);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                        holcoscellmaxlimit7_cellmaxlimit_down_1 -
                                        ethernet_mtu_cell);
                } else {
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                        holcoscellmaxlimit0_cellmaxlimit_down_1 -
                                        ethernet_mtu_cell);
                }
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        holcoscellmaxlimit_qlayer7_cellmaxlimit_down_1 -
                                        ethernet_mtu_cell);
                } else {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        holcoscellmaxlimit_qlayer0_cellmaxlimit_down_1 -
                                        ethernet_mtu_cell);
                }
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_1 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_down_1 -
                            (ethernet_mtu_cell * 2));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                                0);
            WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);
        } else {
            /* COLOR_DROP_EN_QLAYERr, index 0 */
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 
                                0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(holcospktsetlimit_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(holcospktresetlimit_qgroup, 
                                    holcospktsetlimit_qgroup0_pktsetlimit_down_1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    ceiling_func(total_advertised_cell_memory,
                                                queue_port_limit_ratio));
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_1 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 
                                0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        }

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            127);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r, index 0 */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1, 
                                0xFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r, index 0 */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2, 
                                0xFFFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        }
    }

    /* port-based : downlink 2.5G */
    PBMP_ITER(pbmp_downlink_2dot5g, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
    
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 128);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?0:7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?1:7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?2:7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?3:7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?4:7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?5:7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?6:7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?7:7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            if (index == 7) {
                PG2TCr_PG_BMPf_SET(pg2tc, flow_control_type_pause_0_pfc_1?128:255);
            } else {
                PG2TCr_PG_BMPf_SET(pg2tc, 0);
            }
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 128);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 128);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if (index == 7) {
                PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_2dot5g_downlink_ports);
            } else {
                PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if (index == 7) {
                PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_2dot5g_downlink_ports);
            } else {
                PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            if (index == 7) {
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_uplink_2dot5g_port);
            } else {
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            }
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNTr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            }
        } else {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                            shared_xqs_per_downlink_port +
                                            egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                        shared_xqs_per_downlink_port +
                                        egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            }
        } else {
            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                            shared_xqs_per_downlink_port +
                                            egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                        shared_xqs_per_downlink_port +
                                        egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                    shared_xqs_per_downlink_port +
                                    holcosminxqcnt_qlayer8_holcosminxqcnt_down_2dot5);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                            holcospktsetlimit7_pktsetlimit_down_2dot5 - 1);
                } else {
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                        holcospktsetlimit0_pktsetlimit_down_2dot5 - 1);
                }
                WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            }
        } else {
            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                        holcospktsetlimit_qlayer7_pktsetlimit_down_2dot5 - 1);
                } else {
                    HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                        holcospktsetlimit_qlayer0_pktsetlimit_down_2dot5 - 1);
                }
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                    holcospktsetlimit_qlayer8_pktsetlimit_down_2dot5 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            }
        } else {
            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_down_2dot5 - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);            
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit7_cellsetlimit_down_2dot5);
                } else {
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit0_cellsetlimit_down_2dot5);
                }
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit_qlayer7_cellsetlimit_down_2dot5);
                } else {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit_qlayer0_cellsetlimit_down_2dot5);
                }
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    ceiling_func(shared_space_cells,
                                                queue_port_limit_ratio) +
                                    lwmcoscellsetlimit_qlayer8_cellsetlimit_down_2dot5);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                        holcoscellmaxlimit7_cellmaxlimit_down_2dot5 -
                                        ethernet_mtu_cell);
                } else {
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                        holcoscellmaxlimit0_cellmaxlimit_down_2dot5 -
                                        ethernet_mtu_cell);
                }
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        holcoscellmaxlimit_qlayer7_cellmaxlimit_down_2dot5 -
                                        ethernet_mtu_cell);
                } else {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        holcoscellmaxlimit_qlayer0_cellmaxlimit_down_2dot5 -
                                        ethernet_mtu_cell);
                }
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_2dot5 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_down_2dot5 -
                            (ethernet_mtu_cell * 2));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                                0);
            WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);
        } else {
            /* COLOR_DROP_EN_QLAYERr, index 0 */
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 
                                0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(holcospktsetlimit_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(holcospktresetlimit_qgroup, 
                                    holcospktsetlimit_qgroup0_pktsetlimit_down_2dot5 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    ceiling_func(total_advertised_cell_memory,
                                                queue_port_limit_ratio));
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_2dot5 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 
                                0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        }

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            127);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r, index 0 */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1, 
                                0xFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r, index 0 */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2, 
                                0xFFFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        }
    }

    /* port-based : downlink 10G */
    PBMP_ITER(pbmp_downlink_10g, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
    
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 128);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?0:7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?1:7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?2:7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?3:7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?4:7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?5:7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?6:7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, flow_control_type_pause_0_pfc_1?7:7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, flow_control_type_pause_0_pfc_1?7:7);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            if (index == 7) {
                PG2TCr_PG_BMPf_SET(pg2tc, flow_control_type_pause_0_pfc_1?128:255);
            } else {
                PG2TCr_PG_BMPf_SET(pg2tc, 0);
            }
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 128);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 128);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if (index == 7) {
                PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_10g_port_downlink_ports);
            } else {
                PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            if (index == 7) {
                PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_thresholds_per_port_10g_port_downlink_ports);
            } else {
                PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            }
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            if (index == 7) {
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_uplink_10g_port);
            } else {
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            }
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNTr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            }
        } else {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
             }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                            shared_xqs_per_downlink_port +
                                            egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                        shared_xqs_per_downlink_port +
                                        egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            }
        } else {
            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                            shared_xqs_per_downlink_port +
                                            egress_xq_min_reserve_lossless_downlink_ports);
                } else {
                    HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                        shared_xqs_per_downlink_port +
                                        egress_xq_min_reserve_lossy_ports);
                }
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer, 
                                    shared_xqs_per_downlink_port +
                                    holcosminxqcnt_qlayer8_holcosminxqcnt_down_10);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                            holcospktsetlimit7_pktsetlimit_down_10 - 1);
                } else {
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                        holcospktsetlimit0_pktsetlimit_down_10 - 1);
                }
                WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            }
        } else {
            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                        holcospktsetlimit_qlayer7_pktsetlimit_down_10 - 1);
                } else {
                    HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                        holcospktsetlimit_qlayer0_pktsetlimit_down_10 - 1);
                }
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(holcospktresetlimit_qlayer, 
                                    holcospktsetlimit_qlayer8_pktsetlimit_down_10 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            }
        } else {
            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_down_10 - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);            
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossless);
                } else {
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                        egress_queue_min_reserve_downlink_ports_lossy);
                }
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(lwmcoscellsetlimit_qlayer, 
                                    0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                        ceiling_func(shared_space_cells,
                                        queue_port_limit_ratio) +
                                        lwmcoscellsetlimit7_cellsetlimit_down_10);
                } else {
                    HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                        ceiling_func(shared_space_cells,
                                        queue_port_limit_ratio) +
                                        lwmcoscellsetlimit0_cellsetlimit_down_10);
                }
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit_qlayer7_cellsetlimit_down_10);
                } else {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        ceiling_func(shared_space_cells,
                                                    queue_port_limit_ratio) +
                                        lwmcoscellsetlimit_qlayer0_cellsetlimit_down_10);
                }
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    ceiling_func(shared_space_cells,
                                                queue_port_limit_ratio) +
                                    lwmcoscellsetlimit_qlayer8_cellsetlimit_down_10);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                        holcoscellmaxlimit7_cellmaxlimit_down_10 -
                                        ethernet_mtu_cell);
                } else {
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                        holcoscellmaxlimit0_cellmaxlimit_down_10 -
                                        ethernet_mtu_cell);
                }
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        holcoscellmaxlimit_qlayer7_cellmaxlimit_down_10 -
                                        ethernet_mtu_cell);
                } else {
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                        holcoscellmaxlimit_qlayer0_cellmaxlimit_down_10 -
                                        ethernet_mtu_cell);
                }
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qlayer, 
                                    holcoscellmaxlimit_qlayer8_cellmaxlimit_down_10 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
            }
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_down_10 -
                            (ethernet_mtu_cell * 2));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        if (mport < MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                                0);
            WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);
        } else {
            /* COLOR_DROP_EN_QLAYERr, index 0 */
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 
                                0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(holcospktsetlimit_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(holcospktresetlimit_qgroup, 
                                    holcospktsetlimit_qgroup0_pktsetlimit_down_10 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qgroup, 
                                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    ceiling_func(total_advertised_cell_memory,
                                                queue_port_limit_ratio));
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit_qgroup, 
                                    holcoscellmaxlimit_qgroup0_cellmaxlimit_down_10 -
                                    ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 
                                0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        }

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            127);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r, index 0 */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1, 
                                0xFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r, index 0 */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2, 
                                0xFFFFFFFF);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        }
    }

    /* port-based : cpu port (lport=pport=mport=0)*/
    PBMP_ITER(pbmp_cpu, port) {
        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, port, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, port, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, port, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, port, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, port, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, port, index, pg2tc);
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, port, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, port, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, port, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, port, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, port, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, port, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, port, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, port, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, port, index, pgcelllimit);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, port, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, port, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, discard_limit_per_port_pg_downlink_port);
            WRITE_PGDISCARDSETLIMITr(unit, port, index, pgdiscardsetlimit);
        }

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSMINXQCNTr(unit, port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, egress_queue_min_reserve_cpu_ports);
            WRITE_HOLCOSMINXQCNTr(unit, port, index, holcosminxqcnt);
        }

        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMITr(unit, port, index, holcospktsetlimit);
            if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                        shared_xqs_per_downlink_port +
                                        egress_xq_min_reserve_lossless_downlink_ports);
            } else {
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, 
                                    shared_xqs_per_downlink_port +
                                    egress_xq_min_reserve_lossy_up_downlink_ports);
            }
            WRITE_HOLCOSPKTSETLIMITr(unit, port, index, holcospktsetlimit);
        }

        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMITr(unit, port, index, holcospktresetlimit);
            if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                        holcospktsetlimit7_pktsetlimit_cpu - 1);
            } else {
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, 
                                    holcospktsetlimit0_pktsetlimit_cpu - 1);
            }
            WRITE_HOLCOSPKTRESETLIMITr(unit, port, index, holcospktresetlimit);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, port, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, port, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, port, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, port, index, cngcospktlimit1);
        }

#if 0 /* QLAYER is no need for cpu port */
        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT0_QLAYERr(unit, port, index, cngcospktlimit0_qlayer);
            CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, port, index, cngcospktlimit0_qlayer);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT1_QLAYERr(unit, port, index, cngcospktlimit1_qlayer);
            CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer, 
                                numxqs_per_downlink_ports_and_cpu_port - 1);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, port, index, cngcospktlimit1_qlayer);
        }
#endif

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1, 
                            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, 
                            shared_xqs_per_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, 
                            dynxqcntport_dynxqcntport_cpu - 2);
        WRITE_DYNRESETLIMPORTr(unit, port, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_LWMCOSCELLSETLIMITr(unit, port, index, lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_cpu_ports);
            WRITE_LWMCOSCELLSETLIMITr(unit, port, index, lwmcoscellsetlimit);            
        }

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_LWMCOSCELLSETLIMITr(unit, port, index, lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 
                                egress_queue_min_reserve_cpu_ports);
            WRITE_LWMCOSCELLSETLIMITr(unit, port, index, lwmcoscellsetlimit);            
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMITr(unit, port, index, holcoscellmaxlimit);
            if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit7_cellsetlimit_cpu);
            } else {
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, 
                                    ceiling_func(shared_space_cells,
                                                 queue_port_limit_ratio) +
                                    lwmcoscellsetlimit0_cellsetlimit_cpu);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, port, index, holcoscellmaxlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMITr(unit, port, index, holcoscellmaxlimit);
            if (index == CFG_FLOW_CONTROL_ENABLED_COSQ) {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                    holcoscellmaxlimit7_cellmaxlimit_cpu -
                                    ethernet_mtu_cell);
            } else {
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit, 
                                    holcoscellmaxlimit0_cellmaxlimit_cpu -
                                    ethernet_mtu_cell);
            }
            WRITE_HOLCOSCELLMAXLIMITr(unit, port, index, holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, 
                            shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit, 
                            dyncelllimit_dyncellsetlimit_cpu -
                            ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, port, dyncelllimit);

        /* COLOR_DROP_ENr, index 0 */
        READ_COLOR_DROP_ENr(unit, port, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 
                            0);
        WRITE_COLOR_DROP_ENr(unit, port, color_drop_en);

#if 0 /* QLAYER is no need for cpu port */
        /* COLOR_DROP_EN_QLAYERr, index 0 */
        READ_COLOR_DROP_EN_QLAYERr(unit, port, 0, color_drop_en_qlayer);
        COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 
                            0);
        WRITE_COLOR_DROP_EN_QLAYERr(unit, port, 0, color_drop_en_qlayer);

        /* COLOR_DROP_EN_QGROUPr, index 0 */
        READ_COLOR_DROP_EN_QGROUPr(unit, port, color_drop_en_qgroup);
        COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 
                            0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, port, color_drop_en_qgroup);
#endif

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, port, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 
                            255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 
                            127);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 
                            0);
        WRITE_SHARED_POOL_CTRLr(unit, port, shared_pool_ctrl);

#if 0 /* QLAYER is no need for cpu port */
        /* SHARED_POOL_CTRL_EXT1r, index 0 */
        READ_SHARED_POOL_CTRL_EXT1r(unit, port, shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1, 
                            0xFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, port, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r, index 0 */
        READ_SHARED_POOL_CTRL_EXT2r(unit, port, shared_pool_ctrl_ext2);
        SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2, 
                            0xFFFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, port, shared_pool_ctrl_ext2);
#endif
    }

    return SYS_OK;
}

#if CFG_MMU_DEBUG
static void _soc_greyhound2_mmu_init_debug(int unit)
{
    int port, mport;
    int index;
    pbmp_t pbmp_all;
    
    CFAPFULLTHRESHOLDr_t cfapfullthreshold;
    GBLLIMITSETLIMITr_t gbllimitsetlimit;
    GBLLIMITRESETLIMITr_t gbllimitresetlimit;
    TOTALDYNCELLSETLIMITr_t totaldyncellsetlimit;
    TOTALDYNCELLRESETLIMITr_t totaldyncellresetlimit;
    TWO_LAYER_SCH_MODEr_t two_layer_sch_mode;
    MISCCONFIGr_t miscconfig;
    MMUPORTTXENABLE_0r_t mmuporttxenable_0;
    MMUPORTTXENABLE_1r_t mmuporttxenable_1;
    MMUPORTTXENABLE_2r_t mmuporttxenable_2;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;    
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSMINXQCNT_QLAYERr_t holcosminxqcnt_qlayer;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTSETLIMIT_QLAYERr_t holcospktsetlimit_qlayer;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    HOLCOSPKTRESETLIMIT_QLAYERr_t holcospktresetlimit_qlayer;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    CNGCOSPKTLIMIT0_QLAYERr_t cngcospktlimit0_qlayer;
    CNGCOSPKTLIMIT1_QLAYERr_t cngcospktlimit1_qlayer;
    CNGPORTPKTLIMIT0r_t cngportpktlimit0;
    CNGPORTPKTLIMIT1r_t cngportpktlimit1;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    LWMCOSCELLSETLIMIT_QLAYERr_t lwmcoscellsetlimit_qlayer;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    HOLCOSCELLMAXLIMIT_QLAYERr_t holcoscellmaxlimit_qlayer;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_ENr_t color_drop_en;
    COLOR_DROP_EN_QLAYERr_t color_drop_en_qlayer;
    HOLCOSPKTSETLIMIT_QGROUPr_t holcospktsetlimit_qgroup;
    HOLCOSPKTRESETLIMIT_QGROUPr_t holcospktresetlimit_qgroup;
    CNGCOSPKTLIMIT0_QGROUPr_t cngcospktlimit0_qgroup;
    CNGCOSPKTLIMIT1_QGROUPr_t cngcospktlimit1_qgroup;
    HOLCOSCELLMAXLIMIT_QGROUPr_t holcoscellmaxlimit_qgroup;
    COLOR_DROP_EN_QGROUPr_t color_drop_en_qgroup;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;
    SHARED_POOL_CTRL_EXT1r_t shared_pool_ctrl_ext1;
    SHARED_POOL_CTRL_EXT2r_t shared_pool_ctrl_ext2;

    PBMP_ASSIGN(pbmp_all, BCM5357X_ALL_PORTS_MASK);

    sal_printf("\n########################### MMU SETTING #############################\n");

    /* system-based */
    sal_printf("\nSystem-Based ==>\n");
    READ_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);
    sal_printf("CFAPFULLTHRESHOLD.CFAPFULLSETPOINT 0x%x\n", CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_GET(cfapfullthreshold));
    sal_printf("CFAPFULLTHRESHOLD.CFAPFULLRESETPOINT 0x%x\nx", CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_GET(cfapfullthreshold));
    READ_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);
    sal_printf("GBLLIMITSETLIMIT.GBLCELLSETLIMIT 0x%x\n", GBLLIMITSETLIMITr_GBLCELLSETLIMITf_GET(gbllimitsetlimit));
    READ_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);
    sal_printf("GBLLIMITRESETLIMIT.GBLCELLRESETLIMIT 0x%x\n", GBLLIMITRESETLIMITr_GBLCELLRESETLIMITf_GET(gbllimitresetlimit));
    READ_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);
    sal_printf("TOTALDYNCELLSETLIMIT.TOTALDYNCELLSETLIMIT 0x%x\n", TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_GET(totaldyncellsetlimit));
    READ_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    sal_printf("TOTALDYNCELLRESETLIMIT.TOTALDYNCELLRESETLIMIT 0x%x\n", TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_GET(totaldyncellresetlimit));

    PBMP_ITER(pbmp_all, port) {
        if (SOC_PORT_BLOCK_INDEX(port) != 0x0) {
            continue;
        }
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
        if (mport >= MMU_64Q_PPORT_BASE) {
            mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
            READ_TWO_LAYER_SCH_MODEr(unit, mport, two_layer_sch_mode);
            sal_printf("port %d mport %d TWO_LAYER_SCH_MODE.SCH_MODE 0x%x\n", port, mport, TWO_LAYER_SCH_MODEr_SCH_MODEf_GET(two_layer_sch_mode));
        }
    }

    READ_MISCCONFIGr(unit, miscconfig);
    sal_printf("MISCCONFIG.MULTIPLE_ACCOUNTING_FIX_EN 0x%x\n", MISCCONFIGr_MULTIPLE_ACCOUNTING_FIX_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.CNG_DROP_EN 0x%x\n", MISCCONFIGr_CNG_DROP_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.DYN_XQ_EN 0x%x\n", MISCCONFIGr_DYN_XQ_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.HOL_CELL_SOP_DROP_EN 0x%x\n", MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.DYNAMIC_MEMORY_EN 0x%x\n", MISCCONFIGr_DYNAMIC_MEMORY_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.SKIDMARKER 0x%x\n", MISCCONFIGr_SKIDMARKERf_GET(miscconfig));

    READ_MMUPORTTXENABLE_0r(unit, mmuporttxenable_0);
    sal_printf("MMUPORTTXENABLE_0.MMUPORTTXENABLE 0x%x\n", MMUPORTTXENABLE_0r_MMUPORTTXENABLEf_GET(mmuporttxenable_0));

    READ_MMUPORTTXENABLE_1r(unit, mmuporttxenable_1);
    sal_printf("MMUPORTTXENABLE_1.MMUPORTTXENABLE 0x%x\n", MMUPORTTXENABLE_1r_MMUPORTTXENABLEf_GET(mmuporttxenable_1));

    READ_MMUPORTTXENABLE_2r(unit, mmuporttxenable_2);
    sal_printf("MMUPORTTXENABLE_2.MMUPORTTXENABLE 0x%x\n", MMUPORTTXENABLE_2r_MMUPORTTXENABLEf_GET(mmuporttxenable_2));
    sal_printf("\n");

    PBMP_PORT_ADD(pbmp_all, BCM5357X_PORT_CMIC);
    PBMP_ITER(pbmp_all, port) {
        if (IS_CPU_PORT(port)) {
            mport = 0;
            sal_printf("\nPort-Based (cpu port) ==>\n");
        } else {
            if (SOC_PORT_BLOCK_INDEX(port) != 0x0) {
                continue;
            }

            mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
            sal_printf("\nPort-Based (port %d mport %d speed %d) ==>\n", port, mport, SOC_PORT_SPEED_MAX(port));
        }

        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        sal_printf("PG_CTRL0.PPFC_PG_EN 0x%x\n", PG_CTRL0r_PPFC_PG_ENf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI0_GRP 0x%x\n", PG_CTRL0r_PRI0_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI1_GRP 0x%x\n", PG_CTRL0r_PRI1_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI2_GRP 0x%x\n", PG_CTRL0r_PRI2_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI3_GRP 0x%x\n", PG_CTRL0r_PRI3_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI4_GRP 0x%x\n", PG_CTRL0r_PRI4_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI5_GRP 0x%x\n", PG_CTRL0r_PRI5_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI6_GRP 0x%x\n", PG_CTRL0r_PRI6_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI7_GRP 0x%x\n", PG_CTRL0r_PRI7_GRPf_GET(pg_ctrl0));

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        sal_printf("PG_CTRL1.PRI8_GRP 0x%x\n", PG_CTRL1r_PRI8_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI9_GRP 0x%x\n", PG_CTRL1r_PRI9_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI10_GRP 0x%x\n", PG_CTRL1r_PRI10_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI11_GRP 0x%x\n", PG_CTRL1r_PRI11_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI12_GRP 0x%x\n", PG_CTRL1r_PRI12_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI13_GRP 0x%x\n", PG_CTRL1r_PRI13_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI14_GRP 0x%x\n", PG_CTRL1r_PRI14_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI15_GRP 0x%x\n", PG_CTRL1r_PRI15_GRPf_GET(pg_ctrl1));

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            sal_printf("COSQ %d PG2TC.PG_BMP 0x%x\n", index, PG2TCr_PG_BMPf_GET(pg2tc));
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        sal_printf("IBPPKTSETLIMIT.PKTSETLIMIT 0x%x\n", IBPPKTSETLIMITr_PKTSETLIMITf_GET(ibppktsetlimit));
        sal_printf("IBPPKTSETLIMIT.RESETLIMITSEL 0x%x\n", IBPPKTSETLIMITr_RESETLIMITSELf_GET(ibppktsetlimit));

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        sal_printf("MMU_FC_RX_EN.MMU_FC_RX_ENABLE 0x%x\n", MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_GET(mmu_fc_rx_en));

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        sal_printf("MMU_FC_TX_EN.MMU_FC_TX_ENABLE 0x%x\n", MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_GET(mmu_fc_tx_en));

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            sal_printf("COSQ %d PGCELLLIMIT.CELLSETLIMIT 0x%x\n", index, PGCELLLIMITr_CELLSETLIMITf_GET(pgcelllimit));
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            sal_printf("COSQ %d PGCELLLIMIT.CELLRESETLIMIT 0x%x\n", index, PGCELLLIMITr_CELLRESETLIMITf_GET(pgcelllimit));
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            sal_printf("COSQ %d PGDISCARDSETLIMIT.DISCARDSETLIMIT 0x%x\n", index, PGDISCARDSETLIMITr_DISCARDSETLIMITf_GET(pgdiscardsetlimit));
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNTr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                sal_printf("COSQ %d HOLCOSMINXQCNT.HOLCOSMINXQCNT 0x%x\n", index, HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_GET(holcosminxqcnt));
            }
        } else {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index, holcosminxqcnt_qlayer);
                sal_printf("COSQ %d HOLCOSMINXQCNT_QLAYER.HOLCOSMINXQCNT 0x%x\n", index, HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_GET(holcosminxqcnt_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                sal_printf("COSQ %d HOLCOSPKTSETLIMIT.PKTSETLIMIT 0x%x\n", index, HOLCOSPKTSETLIMITr_PKTSETLIMITf_GET(holcospktsetlimit));
            }
        } else {
            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index, holcospktsetlimit_qlayer);
                sal_printf("COSQ %d HOLCOSPKTSETLIMIT_QLAYER.PKTSETLIMIT 0x%x\n", index, HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_GET(holcospktsetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
                sal_printf("COSQ %d HOLCOSPKTRESETLIMIT.PKTRESETLIMIT 0x%x\n", index, HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_GET(holcospktresetlimit));
            }
        } else {
            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index, holcospktresetlimit_qlayer);
                sal_printf("COSQ %d HOLCOSPKTRESETLIMIT_QLAYER.PKTRESETLIMIT 0x%x\n", index, HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_GET(holcospktresetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
                sal_printf("COSQ %d CNGCOSPKTLIMIT0.CNGPKTSETLIMIT0 0x%x\n", index, CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_GET(cngcospktlimit0));
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
                sal_printf("COSQ %d CNGCOSPKTLIMIT1.CNGPKTSETLIMIT1 0x%x\n", index, CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_GET(cngcospktlimit1));
            }
        } else {
            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index, cngcospktlimit0_qlayer);
                sal_printf("COSQ %d CNGCOSPKTLIMIT0_QLAYER.CNGPKTSETLIMIT0 0x%x\n", index, CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_GET(cngcospktlimit0_qlayer));
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <=8 ; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index, cngcospktlimit1_qlayer);
                sal_printf("COSQ %d CNGCOSPKTLIMIT1_QLAYER.CNGPKTSETLIMIT1 0x%x\n", index, CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_GET(cngcospktlimit1_qlayer));
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        sal_printf("CNGPORTPKTLIMIT0.CNGPORTPKTLIMIT0 0x%x\n", CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_GET(cngportpktlimit0));

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        sal_printf("CNGPORTPKTLIMIT1.CNGPORTPKTLIMIT1 0x%x\n", CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_GET(cngportpktlimit1));

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        sal_printf("DYNXQCNTPORT.DYNXQCNTPORT 0x%x\n", DYNXQCNTPORTr_DYNXQCNTPORTf_GET(dynxqcntport));

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        sal_printf("DYNRESETLIMPORT.DYNRESETLIMPORT 0x%x\n", DYNRESETLIMPORTr_DYNRESETLIMPORTf_GET(dynresetlimport));

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT.CELLSETLIMIT 0x%x\n", index, LWMCOSCELLSETLIMITr_CELLSETLIMITf_GET(lwmcoscellsetlimit));
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);            
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT_QLAYER.CELLSETLIMIT 0x%x\n", index, LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_GET(lwmcoscellsetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT.CELLRESETLIMIT 0x%x\n", index, LWMCOSCELLSETLIMITr_CELLRESETLIMITf_GET(lwmcoscellsetlimit));
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index, lwmcoscellsetlimit_qlayer);
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT_QLAYER.CELLRESETLIMIT 0x%x\n", index, LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_GET(lwmcoscellsetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT.CELLMAXLIMIT 0x%x\n", index, HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_GET(holcoscellmaxlimit));
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QLAYER.CELLMAXLIMIT 0x%x\n", index, HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_GET(holcoscellmaxlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT.CELLMAXRESUMELIMIT 0x%x\n", index, HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_GET(holcoscellmaxlimit));
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 8 */
            for (index = 0; index <= 8; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index, holcoscellmaxlimit_qlayer);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QLAYER.CELLMAXRESUMELIMIT 0x%x\n", index, HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_GET(holcoscellmaxlimit_qlayer));
            }
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        sal_printf("DYNCELLLIMIT.DYNCELLSETLIMIT 0x%x\n", DYNCELLLIMITr_DYNCELLSETLIMITf_GET(dyncelllimit));
        sal_printf("DYNCELLLIMIT.DYNCELLRESETLIMIT 0x%x\n", DYNCELLLIMITr_DYNCELLRESETLIMITf_GET(dyncelllimit));

        if (mport < MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            sal_printf("COLOR_DROP_EN.COLOR_DROP_EN 0x%x\n", COLOR_DROP_ENr_COLOR_DROP_ENf_GET(color_drop_en));
        } else {
            /* COLOR_DROP_EN_QLAYERr, index 0 */
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, 0, color_drop_en_qlayer);
            sal_printf("COLOR_DROP_EN_QLAYER.COLOR_DROP_EN 0x%x\n", COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_GET(color_drop_en_qlayer));

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index, holcospktsetlimit_qgroup);
                sal_printf("COSQ %d HOLCOSPKTSETLIMIT_QGROUP.PKTSETLIMIT 0x%x\n", index, HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_GET(holcospktsetlimit_qgroup));
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index, holcospktresetlimit_qgroup);
                sal_printf("COSQ %d HOLCOSPKTRESETLIMIT_QGROUP.PKTRESETLIMIT 0x%x\n", index, HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_GET(holcospktresetlimit_qgroup));
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index, cngcospktlimit0_qgroup);
                sal_printf("COSQ %d CNGCOSPKTLIMIT0_QGROUP.CNGPKTSETLIMIT0 0x%x\n", index, CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_GET(cngcospktlimit0_qgroup));
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index, cngcospktlimit1_qgroup);
                sal_printf("COSQ %d CNGCOSPKTLIMIT1_QGROUP.NGPKTSETLIMIT1 0x%x\n", index, CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_GET(cngcospktlimit1_qgroup));
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QGROUP.CELLMAXLIMIT 0x%x\n", index, HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_GET(holcoscellmaxlimit_qgroup));
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index, holcoscellmaxlimit_qgroup);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QGROUP.CELLMAXRESUMELIMIT 0x%x\n", index, HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_GET(holcoscellmaxlimit_qgroup));
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            sal_printf("COLOR_DROP_EN_QGROUP.COLOR_DROP_EN 0x%x\n", COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_GET(color_drop_en_qgroup));
        }

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        sal_printf("SHARED_POOL_CTRL.DYNAMIC_COS_DROP_EN 0x%x\n", SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_GET(shared_pool_ctrl));
        sal_printf("SHARED_POOL_CTRL.SHARED_POOL_DISCARD_EN 0x%x\n", SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_GET(shared_pool_ctrl));
        sal_printf("SHARED_POOL_CTRL.SHARED_POOL_XOFF_EN 0x%x\n", SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_GET(shared_pool_ctrl));

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r, index 0 */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            sal_printf("SHARED_POOL_CTRL_EXT1.DYNAMIC_COS_DROP_EN 0x%x\n", SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_GET(shared_pool_ctrl_ext1));

            /* SHARED_POOL_CTRL_EXT2r, index 0 */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            sal_printf("SHARED_POOL_CTRL_EXT2.DYNAMIC_COS_DROP_EN 0x%x\n", SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_GET(shared_pool_ctrl_ext2));
        }
    }
}
#endif /* CFG_MMU_DEBUG */

static void
soc_misc_init(uint8 unit)
{
#define NUM_XLPORT 4
    int i, lport, pport;
#if CFG_SET_TOP_CORE_PLL
    int freq, target_freq, divisor, dividend;
#endif

#if 0
    int bindex;
    uint32 val;
#endif
    GPORT_CONFIGr_t gport_config;
    MISCCONFIGr_t miscconfig;
    L2_AUX_HASH_CONTROLr_t l2_aux_hash_control;
    L3_AUX_HASH_CONTROLr_t l3_aux_hash_control;
    EGR_ENABLEm_t egr_enable;
    CMIC_RATE_ADJUST_EXT_MDIOr_t cmic_rate_adjust_ext_mdio;
    CMIC_RATE_ADJUST_INT_MDIOr_t cmic_rate_adjust_int_mdio;
    EGR_VLAN_CONTROL_1r_t egr_vlan_control_1;
    SW2_FP_DST_ACTION_CONTROLr_t sw2_fp_dst_action_control;
    XLPORT_MIB_RESETr_t xlport_mib_reset;
    CLPORT_MIB_RESETr_t clport_mib_reset;
    XLPORT_SOFT_RESETr_t xlport_sreset;
    XLPORT_MODE_REGr_t xlport_mode;
    XLPORT_ENABLE_REGr_t xlport_enable;
    CLPORT_SOFT_RESETr_t clport_sreset;
    CLPORT_MODE_REGr_t clport_mode;
    CLPORT_ENABLE_REGr_t clport_enable;
    uint8 port_mode;
    int block_start_port;
    soc_pipe_mem_clear(unit);

    soc_init_port_mapping(unit);

    /* Reset XLPORT and CLPORT MIB counter */
    SOC_LPORT_ITER(lport) {
        if (IS_XL_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            XLPORT_MIB_RESETr_CLR(xlport_mib_reset);
            XLPORT_MIB_RESETr_CLR_CNTf_SET(xlport_mib_reset, 0xf);
            WRITE_XLPORT_MIB_RESETr(unit, lport, xlport_mib_reset);
            XLPORT_MIB_RESETr_CLR_CNTf_SET(xlport_mib_reset, 0);
            WRITE_XLPORT_MIB_RESETr(unit, lport, xlport_mib_reset);
        }

        if (IS_CL_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            CLPORT_MIB_RESETr_CLR(clport_mib_reset);
            CLPORT_MIB_RESETr_CLR_CNTf_SET(clport_mib_reset, 0xf);
            WRITE_CLPORT_MIB_RESETr(unit, lport, clport_mib_reset);
            CLPORT_MIB_RESETr_CLR_CNTf_SET(clport_mib_reset, 0);
            WRITE_CLPORT_MIB_RESETr(unit, lport, clport_mib_reset);
        }
    }

    

    /* GMAC init */
    GPORT_CONFIGr_CLR(gport_config);
    GPORT_CONFIGr_CLR_CNTf_SET(gport_config, 1);
    GPORT_CONFIGr_GPORT_ENf_SET(gport_config, 1);
    SOC_LPORT_ITER(lport) {
        if (IS_GX_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            /* Clear counter and enable gport */
            WRITE_GPORT_CONFIGr(unit, lport, gport_config);
        }
    }

    GPORT_CONFIGr_CLR_CNTf_SET(gport_config, 0);
    SOC_LPORT_ITER(lport) {
        if (IS_GX_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            /* Enable gport */
            WRITE_GPORT_CONFIGr(unit, lport, gport_config);
        }
    }

    

    SOC_LPORT_ITER(lport) {
        if (IS_XL_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            XLPORT_SOFT_RESETr_CLR(xlport_sreset);
            pport = SOC_PORT_L2P_MAPPING(lport);
            for (i = 0; i <= 3; i++) {
                if (SOC_PORT_P2L_MAPPING(pport + i) == -1) {
                    continue;
                }
            
                if (i == 0) {
                    XLPORT_SOFT_RESETr_PORT0f_SET(xlport_sreset, 1);
                } else if (i == 1) {
                    XLPORT_SOFT_RESETr_PORT1f_SET(xlport_sreset, 1);
                } else if (i == 2) {
                    XLPORT_SOFT_RESETr_PORT2f_SET(xlport_sreset, 1);
                } else if (i == 3) {
                    XLPORT_SOFT_RESETr_PORT3f_SET(xlport_sreset, 1);
                }
            }
            WRITE_XLPORT_SOFT_RESETr(unit, lport, xlport_sreset);
            
            XLPORT_MODE_REGr_CLR(xlport_mode);
            block_start_port = pport - SOC_PORT_BLOCK_INDEX(lport);
            if (pport == block_start_port) {
                port_mode = SOC_PORT_MODE(lport);
                XLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(xlport_mode, port_mode);
                XLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(xlport_mode, port_mode);
                WRITE_XLPORT_MODE_REGr(unit, lport, xlport_mode);
            }


            /* De-assert XLPORT soft reset */
            XLPORT_SOFT_RESETr_CLR(xlport_sreset);
            WRITE_XLPORT_SOFT_RESETr(unit, lport, xlport_sreset);
    
            /* Enable XLPORT */
            XLPORT_ENABLE_REGr_CLR(xlport_enable);
            for (i = 0; i <= 3; i++) {
                if (SOC_PORT_P2L_MAPPING(pport + i) == -1) {
                    continue;
                }
                
                if (i == 0) {
                    XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable, 1);
                } else if (i == 1) {
                    XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable, 1);
                } else if (i == 2) {
                    XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable, 1);
                } else if (i == 3) {
                    XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable, 1);
                }
            }
            WRITE_XLPORT_ENABLE_REGr(unit, lport, xlport_enable);
        } else if (IS_CL_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            CLPORT_SOFT_RESETr_CLR(clport_sreset);
            pport = SOC_PORT_L2P_MAPPING(lport);
            for (i = 0; i <= 3; i++) {
                if (SOC_PORT_P2L_MAPPING(pport + i) == -1) {
                    continue;
                }
            
                if (i == 0) {
                    CLPORT_SOFT_RESETr_PORT0f_SET(clport_sreset, 1);
                } else if (i == 1) {
                    CLPORT_SOFT_RESETr_PORT1f_SET(clport_sreset, 1);
                } else if (i == 2) {
                    CLPORT_SOFT_RESETr_PORT2f_SET(clport_sreset, 1);
                } else if (i == 3) {
                    CLPORT_SOFT_RESETr_PORT3f_SET(clport_sreset, 1);
                }
            }
            WRITE_CLPORT_SOFT_RESETr(unit, lport, clport_sreset);
            
            CLPORT_MODE_REGr_CLR(clport_mode);
            block_start_port = pport - SOC_PORT_BLOCK_INDEX(lport);
            if (SOC_PORT_BLOCK_INDEX(lport) == 0) {
                port_mode = SOC_PORT_MODE(lport);           
                CLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(clport_mode, port_mode);
                CLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(clport_mode, port_mode);
                WRITE_CLPORT_MODE_REGr(unit, lport, clport_mode);
            }


            /* De-assert XLPORT soft reset */
            CLPORT_SOFT_RESETr_CLR(clport_sreset);
            WRITE_CLPORT_SOFT_RESETr(unit, lport, clport_sreset);
    
            /* Enable XLPORT */
            CLPORT_ENABLE_REGr_CLR(clport_enable);
            for (i = 0; i <= 3; i++) {
                if (SOC_PORT_P2L_MAPPING(pport + i) == -1) {
                    continue;
                }
                
                if (i == 0) {
                    CLPORT_ENABLE_REGr_PORT0f_SET(clport_enable, 1);
                } else if (i == 1) {
                    CLPORT_ENABLE_REGr_PORT1f_SET(clport_enable, 1);
                } else if (i == 2) {
                    CLPORT_ENABLE_REGr_PORT2f_SET(clport_enable, 1);
                } else if (i == 3) {
                    CLPORT_ENABLE_REGr_PORT3f_SET(clport_enable, 1);
                }
            }
            WRITE_CLPORT_ENABLE_REGr(unit, lport, clport_enable);
        }
    }

#if !CONFIG_GREYHOUND2_ROMCODE
    READ_MISCCONFIGr(unit, miscconfig);
    MISCCONFIGr_METERING_CLK_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    /* Enable dual hash on L2 and L3 tables with CRC32_LOWER (2) */
    L2_AUX_HASH_CONTROLr_CLR(l2_aux_hash_control);
    L2_AUX_HASH_CONTROLr_ENABLEf_SET(l2_aux_hash_control, 1);
    L2_AUX_HASH_CONTROLr_HASH_SELECTf_SET(l2_aux_hash_control, 2);
    L2_AUX_HASH_CONTROLr_INSERT_LEAST_FULL_HALFf_SET(l2_aux_hash_control, 1);
    WRITE_L2_AUX_HASH_CONTROLr(unit, l2_aux_hash_control);

    L3_AUX_HASH_CONTROLr_CLR(l3_aux_hash_control);
    L3_AUX_HASH_CONTROLr_ENABLEf_SET(l3_aux_hash_control, 1);
    L3_AUX_HASH_CONTROLr_HASH_SELECTf_SET(l3_aux_hash_control, 2);
    L3_AUX_HASH_CONTROLr_INSERT_LEAST_FULL_HALFf_SET(l3_aux_hash_control, 1);
    WRITE_L3_AUX_HASH_CONTROLr(unit, l3_aux_hash_control);
#endif /* !CONFIG_GREYHOUND2_ROMCODE */

    /* Egress Enable */
    EGR_ENABLEm_CLR(egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    SOC_LPORT_ITER(lport) {
        WRITE_EGR_ENABLEm(unit, SOC_PORT_L2P_MAPPING(lport), egr_enable);
    }

    
#if CFG_SET_TOP_CORE_PLL
    /*
     * Set external MDIO freq to around 10MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     */

    freq = sku_port_config->freq;
    target_freq = 12;
    divisor = (freq + (target_freq * 2 - 1)) / (target_freq * 2);
    dividend = 1;

    CMIC_RATE_ADJUST_EXT_MDIOr_CLR(cmic_rate_adjust_ext_mdio);
    CMIC_RATE_ADJUST_EXT_MDIOr_DIVISORf_SET(cmic_rate_adjust_ext_mdio, divisor);
    CMIC_RATE_ADJUST_EXT_MDIOr_DIVIDENDf_SET(cmic_rate_adjust_ext_mdio, dividend);
    WRITE_CMIC_RATE_ADJUST_EXT_MDIOr(unit, cmic_rate_adjust_ext_mdio);

    /*
     * Set internal MDIO freq to around 10MHz
     * Valid range is from 2.5MHz to 12.5MHz
     * target_freq = core_clock_freq * DIVIDEND / DIVISOR / 2
     * or
     * DIVISOR = core_clock_freq * DIVIDENT / (target_freq * 2)
     */
    target_freq = 12;
    divisor = (freq + (target_freq * 2 - 1)) / (target_freq * 2);
    dividend = 1;
    CMIC_RATE_ADJUST_INT_MDIOr_CLR(cmic_rate_adjust_int_mdio);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(cmic_rate_adjust_int_mdio, divisor);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(cmic_rate_adjust_int_mdio, dividend);
    WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, cmic_rate_adjust_int_mdio);
#else
    /*
       * Set reference clock (based on 200MHz core clock)
       * to be 200MHz * (1/40) = 5MHz
       */
    CMIC_RATE_ADJUST_EXT_MDIOr_CLR(cmic_rate_adjust_ext_mdio);
    CMIC_RATE_ADJUST_EXT_MDIOr_DIVISORf_SET(cmic_rate_adjust_ext_mdio, 0x1e);
    CMIC_RATE_ADJUST_EXT_MDIOr_DIVIDENDf_SET(cmic_rate_adjust_ext_mdio, 0x1);
    WRITE_CMIC_RATE_ADJUST_EXT_MDIOr(unit, cmic_rate_adjust_ext_mdio);

    /* Match the Internal MDC freq with above for External MDC */
    CMIC_RATE_ADJUST_INT_MDIOr_CLR(cmic_rate_adjust_int_mdio);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(cmic_rate_adjust_int_mdio, 0x1e);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(cmic_rate_adjust_int_mdio, 0x1);
    WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, cmic_rate_adjust_int_mdio);
#endif
    /* The HW defaults for EGR_VLAN_CONTROL_1.VT_MISS_UNTAG == 1, which
     * causes the outer tag to be removed from packets that don't have
     * a hit in the egress vlan tranlation table. Set to 0 to disable this.
     */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_control_1);
    SOC_LPORT_ITER(lport) {
        WRITE_EGR_VLAN_CONTROL_1r(unit, lport, egr_vlan_control_1);
    }

#if !CONFIG_GREYHOUND2_ROMCODE
    /* Enable SRC_REMOVAL_EN[Bit 0] and LAG_RES_EN[Bit2] */
    SW2_FP_DST_ACTION_CONTROLr_CLR(sw2_fp_dst_action_control);
    SW2_FP_DST_ACTION_CONTROLr_SRC_REMOVAL_ENf_SET(sw2_fp_dst_action_control, 1);
    SW2_FP_DST_ACTION_CONTROLr_LAG_RES_ENf_SET(sw2_fp_dst_action_control, 1);
    WRITE_SW2_FP_DST_ACTION_CONTROLr(unit, sw2_fp_dst_action_control);
#endif

    

#if 0 
    /*
     * (GH-1622) XQ IPMC group table controllers only initialize address
     *  0~63 of group table.
     */
#define MMU_IPMC_GROUP_TBL_MAXIDX 256
    val = 0;
    for (lport = BCM5357X_LPORT_MIN; lport <= BCM5357X_LPORT_MAX; lport++) {
        for (bindex = 0; bindex < MMU_IPMC_GROUP_TBL_MAXIDX; bindex++)  {
            bcm5357x_mem_set(unit, M_MMU_IPMC_GROUP_TBL(lport,bindex), &val, 1);
        }
    }

    if ((BCM53405_DEVICE_ID == gh2_sw_info.devid) ||
        (BCM53415_DEVICE_ID == gh2_sw_info.devid)) {
        /* Shutdown GP,XP0~3(Bit[1-5]) */
        bcm5357x_reg_set(unit, R_EGR_PORT_BUFFER_CLK_SHUTDOWN, 0x3E);
    }
#endif  
}

static sys_error_t soc_tdm_init(uint8 unit) {
    const uint32        *arr = NULL;
    int                 tdm_size;
    int i;
    uint32 val;

    IARB_TDM_CONTROLr_t iarb_tdm_control;
    IARB_TDM_TABLEm_t iarb_tdm_table;
    MMU_ARB_TDM_TABLEm_t mmu_arb_tdm_table;

    arr = sku_port_config->tdm_table;
    tdm_size = sku_port_config->tdm_table_size;

    /* DISABLE [Bit 0] = 1, TDM_WRAP_PTR = TDM_SIZE-1 */
    READ_IARB_TDM_CONTROLr(unit, iarb_tdm_control);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_control, 1);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_control, (tdm_size-1));  
    WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_control);

    if (arr == NULL) {
        return SYS_ERR_PARAMETER;
    }

    for (i = 0; i < tdm_size; i++) {
        IARB_TDM_TABLEm_CLR(iarb_tdm_table);
        IARB_TDM_TABLEm_PORT_NUMf_SET(iarb_tdm_table, arr[i]);
        WRITE_IARB_TDM_TABLEm(unit, i, iarb_tdm_table);

        val = (arr[i] != 127) ? SOC_PORT_P2M_MAPPING(arr[i]) : 127;
        MMU_ARB_TDM_TABLEm_CLR(mmu_arb_tdm_table);
        MMU_ARB_TDM_TABLEm_PORT_NUMf_SET(mmu_arb_tdm_table, val);
        if (i == (tdm_size - 1)) {
            /* WRAP_EN = 1 */
            MMU_ARB_TDM_TABLEm_WRAP_ENf_SET(mmu_arb_tdm_table, 1);
        }
        WRITE_MMU_ARB_TDM_TABLEm(unit, i, mmu_arb_tdm_table);
    }

    /* DISABLE = 0, TDM_WRAP_PTR = TDM_SIZE-1 */
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_control, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_control, (tdm_size - 1));
    WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_control);
    return SYS_OK;  
}

static sys_error_t soc_tdm_calculation(uint8 unit) {

    sys_error_t ioerr = SYS_OK;
    int phy_port, port;
    int idx;
    int core_freq = sku_port_config->freq;
    int chip_state[NUM_EXT_PORTS], chip_speed[NUM_EXT_PORTS];
    pbmp_t allpbmp;
    tdm_config_t tdm_config, *tcfg = &tdm_config;
    tdm_soc_t tdm_soc, *chip_pkg = &tdm_soc;
    tdm_mod_t *tdm_pkg = NULL ;
    IARB_TDM_CONTROLr_t iarb_tdm_control;
    IARB_TDM_TABLEm_t iarb_tdm_table;
    MMU_ARB_TDM_TABLEm_t mmu_arb_tdm_table;
    int i, tdm_size = 0;
    uint32 val;

    tdm_pkg = (tdm_mod_t *)sal_malloc(sizeof(tdm_mod_t));
    if (tdm_pkg == NULL) {
        sal_printf("Failed to allocate memory in soc_tdm_calculation() !\n");
        return SYS_ERR_OUT_OF_RESOURCE;
    }

    sal_memset(chip_state, 0, sizeof(int) * NUM_EXT_PORTS);
    sal_memset(chip_speed, 0, sizeof(int) * NUM_EXT_PORTS);
    sal_memset(tcfg, 0, sizeof(tdm_config_t));
    sal_memset(chip_pkg, 0, sizeof(tdm_soc_t));
    sal_memset(tdm_pkg, 0, sizeof(tdm_mod_t));
    
    PBMP_CLEAR(allpbmp);
    SOC_PPORT_ITER(phy_port) {
        PBMP_PORT_ADD(allpbmp, SOC_PORT_P2L_MAPPING(phy_port));
    }
    PBMP_PORT_ADD(allpbmp, CMIC_LPORT);

    SOC_PPORT_ITER(phy_port) {
        port = SOC_PORT_P2L_MAPPING(phy_port);
        if (SOC_PORT_SPEED_MAX(port) == 0) {
            continue;
        }

        if ((SOC_PORT_SPEED_MAX(port) > 0) && \
            (SOC_PORT_SPEED_MAX(port) < 1000)) {
            tcfg->speed[phy_port] = 1000;
        } else {
            tcfg->speed[phy_port] = SOC_PORT_SPEED_MAX(port);
        }

        tcfg->port_state[phy_port] = PORT_STATE_LINERATE;
    }

    tcfg->speed[CMIC_PPORT] = 1000; 
    TDM_DBG(("port speed:"));
    for (idx = 0; idx < NUM_EXT_PORTS; idx++) {
        if (idx % 8 == 0) {
            TDM_DBG(("\n    "));
        }
        TDM_DBG((" %6d", tcfg->speed[idx]));
    }
    TDM_DBG(("\n"));
    TDM_DBG(("port state map:"));
    for (idx = 0; idx < NUM_EXT_PORTS; idx++) {
        if (idx % 16 == 0) {
            TDM_DBG(("\n    "));
        }
        if (idx == 0 || idx == (NUM_EXT_PORTS - 1)) {
            TDM_DBG((" ---"));
        } else {
            TDM_DBG((" %3d", tcfg->port_state[idx]));
        }
    }
    TDM_DBG(("\n"));
    
    chip_pkg->unit = unit;
    chip_pkg->num_ext_ports = NUM_EXT_PORTS;
    chip_pkg->clk_freq = core_freq;
    chip_pkg->state = (enum port_state_e *)chip_state;
    chip_pkg->speed = (enum port_speed_e *)chip_speed;
    for (idx = 1; idx < NUM_EXT_PORTS; idx++) {
        /* Shift port state to left one position; required by C_TDM */
        chip_pkg->state[idx - 1] = tcfg->port_state[idx];
    }
    /* enable cpu port */
    chip_pkg->state[CMIC_PPORT] = PORT_STATE_LINERATE;
    for (idx = 0; idx < NUM_EXT_PORTS; idx ++) {
        chip_pkg->speed[idx] = tcfg->speed[idx];
    }
    chip_pkg->soc_vars.gh2.mgmt_pm_hg = 0;

    chip_pkg->soc_vars.gh2.tdm_chk_en = 1;

    bcm5357x_a0_sel_tdm(chip_pkg, tdm_pkg);
    if (NULL == bcm5357x_a0_set_tdm_tbl(tdm_pkg)) {
        sal_printf("bcm5357x_a0_init[%d]: Unable to configure TDM\n", unit);
        sal_free(tdm_pkg);
        return SYS_ERR;
    }
    tdm_size = tdm_pkg->_chip_data.cal_0.cal_len;
    sal_memcpy(tcfg->idb_tdm_tbl_0, tdm_pkg->_chip_data.cal_0.cal_main, \
                   sizeof(int)*GH2_LR_VBS_LEN);
    TDM_DBG(("\n"));
    TDM_DBG(("tdm size: %d", tdm_size));
    TDM_DBG(("\n"));
    TDM_DBG(("tdm table:"));
    for (idx = 0; idx < tdm_size; idx++) {
        if (idx % 16 == 0) {
            TDM_DBG(("\n    "));
        }
        TDM_DBG(("  %d", (tcfg->idb_tdm_tbl_0[idx]<0) ? 127 : tcfg->idb_tdm_tbl_0[idx]));
    }
    TDM_DBG(("\n"));

    /* DISABLE [Bit 0] = 1, TDM_WRAP_PTR = TDM_SIZE-1 */
    READ_IARB_TDM_CONTROLr(unit, iarb_tdm_control);
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_control, 1);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_control, (tdm_size-1));  
    WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_control);

    for (i = 0; i < tdm_size; i++) {
        IARB_TDM_TABLEm_CLR(iarb_tdm_table);
        IARB_TDM_TABLEm_PORT_NUMf_SET(iarb_tdm_table, tcfg->idb_tdm_tbl_0[i]);
        WRITE_IARB_TDM_TABLEm(unit, i, iarb_tdm_table);

        if (tcfg->idb_tdm_tbl_0[i] == -1) {
            /* Convert IDLE slot definition */
            tcfg->idb_tdm_tbl_0[i] = 127;
        }

        val = (tcfg->idb_tdm_tbl_0[i] != 127) ? \
              SOC_PORT_P2M_MAPPING(tcfg->idb_tdm_tbl_0[i]) : 127;
        MMU_ARB_TDM_TABLEm_CLR(mmu_arb_tdm_table);
        MMU_ARB_TDM_TABLEm_PORT_NUMf_SET(mmu_arb_tdm_table, val);
        if (i == (tdm_size - 1)) {
            /* WRAP_EN = 1 */
            MMU_ARB_TDM_TABLEm_WRAP_ENf_SET(mmu_arb_tdm_table, 1);
        }
        WRITE_MMU_ARB_TDM_TABLEm(unit, i, mmu_arb_tdm_table);
    }

    /* DISABLE = 0, TDM_WRAP_PTR = TDM_SIZE-1 */
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_control, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_control, (tdm_size - 1));
    WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_control);

    tdm_gh2_free(tdm_pkg);
    sal_free(tdm_pkg);

    return ioerr;
}

static void
soc_mmu_init(uint8 unit)
{
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    sys_error_t rv = SYS_OK;  
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */
    uint8 mmu_lossless = 0;
    int pport;
    pbmp_t lpbmp, mmu_lpbmp;
#ifndef GREYHOUND2_CHIP_A0    
    uint32 val, oval, cfap_max_idx;
    CFAPCONFIGr_t cfapconfig;
#endif
    MMUPORTENABLE_0r_t mmuportenable_0;
    MMUPORTENABLE_1r_t mmuportenable_1;
    MMUPORTENABLE_2r_t mmuportenable_2;

    if (sku_port_config->tdm_table_size > 0) {
        soc_tdm_init(unit);
    } else {
        soc_tdm_calculation(unit);
    }
#ifndef GREYHOUND2_CHIP_A0
    if (gh2_sw_info.cbp_buffer_size == 4 * 1024 * 1024) {
        cfap_max_idx = GH2_MMU_CBP_FULL_SIZE;
    } else if (gh2_sw_info.cbp_buffer_size == 2 * 1024 * 1024) {
        cfap_max_idx = GH2_MMU_CBP_HALF_SIZE;
    } else if (gh2_sw_info.cbp_buffer_size == 1 * 1024 * 1024) {
        cfap_max_idx = GH2_MMU_CBP_QUARTER_SIZE;
    } else {
        cfap_max_idx = 0;
        sal_printf("ERROR : gh2_sw_info.cbp_buffer_size %d is incorrect!\n", gh2_sw_info.cbp_buffer_size);
        return;
    }
    
    READ_CFAPCONFIGr(unit, cfapconfig);
    oval = CFAPCONFIGr_GET(cfapconfig);
    CFAPCONFIGr_CFAPPOOLSIZEf_SET(cfapconfig, cfap_max_idx);
    val = CFAPCONFIGr_GET(cfapconfig);
    if (oval != val) {
        WRITE_CFAPCONFIGr(unit, cfapconfig);
    }
#endif

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    rv = sal_config_uint8_get(SAL_CONFIG_MMU_LOSSLESS, &mmu_lossless);
    if ((rv == SYS_OK) && mmu_lossless) {
        sal_printf("Vendor Config : Change to enable the lossless MMU setting.\n");
    }
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

    if (mmu_lossless) {
        rv = _soc_greyhound2_mmu_init_helper_lossless(unit);
    } else {
        rv = _soc_greyhound2_mmu_init_helper_lossy(unit);
    }

#if CFG_MMU_DEBUG
    _soc_greyhound2_mmu_init_debug(unit);
#endif /* CFG_MMU_DEBUG */

    /* Port enable */
    PBMP_ASSIGN(lpbmp, BCM5357X_ALL_PORTS_MASK);
#if CFG_RXTX_SUPPORT_ENABLED
    PBMP_PORT_ADD(lpbmp, 0);
#endif  /* CFG_RXTX_SUPPORT_ENABLED */
    PBMP_CLEAR(mmu_lpbmp);
    PBMP_ITER(lpbmp, pport) {          
         PBMP_PORT_ADD(mmu_lpbmp, SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(pport)));
    }
    /* Add CPU port */
    MMUPORTENABLE_0r_CLR(mmuportenable_0);
    MMUPORTENABLE_0r_MMUPORTENABLEf_SET(mmuportenable_0, PBMP_WORD_GET(mmu_lpbmp, 0));
    WRITE_MMUPORTENABLE_0r(unit, mmuportenable_0);

    MMUPORTENABLE_1r_CLR(mmuportenable_1);
    MMUPORTENABLE_1r_MMUPORTENABLEf_SET(mmuportenable_1, PBMP_WORD_GET(mmu_lpbmp, 1));
    WRITE_MMUPORTENABLE_1r(unit, mmuportenable_1);

    MMUPORTENABLE_2r_CLR(mmuportenable_2);
    MMUPORTENABLE_2r_MMUPORTENABLEf_SET(mmuportenable_2, PBMP_WORD_GET(mmu_lpbmp, 2));
    WRITE_MMUPORTENABLE_2r(unit, mmuportenable_2);
}

static void
config_schedule_mode(uint8 unit)
{
    int lport, pport, mmu_port, cos_queue, weight_val;

    XQCOSARBSELr_t xqcosarbsel;
    WRRWEIGHT_COS0r_t wrrweight_cos0;
    WRRWEIGHT_COS1r_t wrrweight_cos1;
    WRRWEIGHT_COS2r_t wrrweight_cos2;
    WRRWEIGHT_COS3r_t wrrweight_cos3;
    XQCOSARBSEL_QLAYERr_t xqcosarbsel_qlayer;
    WRRWEIGHT_COS_QLAYERr_t wrrweight_cos_qlayer;
    MMU_MAX_BUCKET_QLAYERm_t mmu_max_bucket_qlayer;

    /* MAX_THD_SEL = 0 : Disable MAX shaper */
    MMU_MAX_BUCKET_QLAYERm_CLR(mmu_max_bucket_qlayer);

    SOC_LPORT_ITER(lport) {
        /* MMU to Physical  port */
        pport = SOC_PORT_L2P_MAPPING(lport);
        /* Physical to Logical port */
        mmu_port = SOC_PORT_P2M_MAPPING(pport);

        if (mmu_port < MMU_64Q_PPORT_BASE) {
            /* Legacy 8x1 scheduling */

#if CONFIG_GREYHOUND2_ROMCODE
            /* Strict Priority Mode[Bit 0-1] = 0x0, MTU_Quanta_Select[Bit 2-3]=0x3 */
            XQCOSARBSELr_CLR(xqcosarbsel);
            XQCOSARBSELr_COSARBf_SET(xqcosarbsel, 0);
            XQCOSARBSELr_MTU_QUANTA_SELECTf_SET(xqcosarbsel, 3);
            WRITE_XQCOSARBSELr(unit, mmu_port, xqcosarbsel);
#else
            
            WRRWEIGHT_COS0r_CLR(wrrweight_cos0);
            WRRWEIGHT_COS0r_ENABLEf_SET(wrrweight_cos0, 1);
            WRRWEIGHT_COS0r_WEIGHTf_SET(wrrweight_cos0, 1);
            WRITE_WRRWEIGHT_COS0r(unit, mmu_port, wrrweight_cos0);

            WRRWEIGHT_COS1r_CLR(wrrweight_cos1);
            WRRWEIGHT_COS1r_ENABLEf_SET(wrrweight_cos1, 1);
            WRRWEIGHT_COS1r_WEIGHTf_SET(wrrweight_cos1, 2);
            WRITE_WRRWEIGHT_COS1r(unit, mmu_port, wrrweight_cos1);

            WRRWEIGHT_COS2r_CLR(wrrweight_cos2);
            WRRWEIGHT_COS2r_ENABLEf_SET(wrrweight_cos2, 1);
            WRRWEIGHT_COS2r_WEIGHTf_SET(wrrweight_cos2, 4);
            WRITE_WRRWEIGHT_COS2r(unit, mmu_port, wrrweight_cos2);

            WRRWEIGHT_COS3r_CLR(wrrweight_cos3);
            WRRWEIGHT_COS3r_ENABLEf_SET(wrrweight_cos3, 1);
            WRRWEIGHT_COS3r_WEIGHTf_SET(wrrweight_cos3, 8);
            WRITE_WRRWEIGHT_COS3r(unit, mmu_port, wrrweight_cos3);

            XQCOSARBSELr_CLR(xqcosarbsel);
            XQCOSARBSELr_COSARBf_SET(xqcosarbsel, 2);
            XQCOSARBSELr_MTU_QUANTA_SELECTf_SET(xqcosarbsel, 3);
            WRITE_XQCOSARBSELr(unit, mmu_port, xqcosarbsel);
#endif /* CONFIG_GREYHOUND2_ROMCODE */

            for (cos_queue = 0; cos_queue < COS_QUEUE_NUM; cos_queue++) {
                WRITE_MMU_MAX_BUCKET_QLAYERm(unit, (mmu_port * 8) + cos_queue, mmu_max_bucket_qlayer);
            } 
        } else {
            /* Use 8x1 scheduling for high speed port (with 64 queues) */

            
            for (cos_queue = 0; cos_queue < COS_QUEUE_NUM; cos_queue++) {
                WRRWEIGHT_COS_QLAYERr_CLR(wrrweight_cos_qlayer);
                WRRWEIGHT_COS_QLAYERr_ENABLEf_SET(wrrweight_cos_qlayer, 1);
                if (cos_queue == 0) {
                    weight_val = 1;
                } else if (cos_queue == 1) {
                    weight_val = 2;
                } else if (cos_queue == 2) {
                    weight_val = 4;
                } else if (cos_queue == 3) {
                    weight_val = 8;
                }
                WRRWEIGHT_COS_QLAYERr_WEIGHTf_SET(wrrweight_cos_qlayer, weight_val);
                WRITE_WRRWEIGHT_COS_QLAYERr(unit, mmu_port, cos_queue, wrrweight_cos_qlayer);
            }

            XQCOSARBSEL_QLAYERr_CLR(xqcosarbsel_qlayer);
            XQCOSARBSEL_QLAYERr_COSARBf_SET(xqcosarbsel_qlayer, 2);
            XQCOSARBSEL_QLAYERr_MTU_QUANTA_SELECTf_SET(xqcosarbsel_qlayer, 3);
            WRITE_XQCOSARBSEL_QLAYERr(unit, mmu_port, 0, xqcosarbsel_qlayer);

            for (cos_queue = 0; cos_queue < COS_QUEUE_NUM; cos_queue++) {
                WRITE_MMU_MAX_BUCKET_QLAYERm(unit, MMU_64Q_PPORT_BASE * 8 + ((mmu_port - MMU_64Q_PPORT_BASE) * 64) + cos_queue, mmu_max_bucket_qlayer);
            } 
        }
    }
}

static void
bcm5357x_system_init(uint8 unit)
{
    int i, j, lport;   
    PORTm_t port_entry;

#if CONFIG_GREYHOUND2_ROMCODE
    uint32 cos_map_default[8] = { 0, 0, 1, 1, 2, 2, 3, 3};
#endif
    uint32 dot1pmap[16] = {
        0x00000000, 0x00000001, 0x00000004, 0x00000005, 0x00000008, 0x00000009, 0x0000000c, 0x0000000d,
        0x00000010, 0x00000011, 0x00000014, 0x00000015, 0x00000018, 0x00000019, 0x0000001c, 0x0000001d 
    };

    uint32 drop_mac_addr1[2] = { 0xC2000001,   0x0180 };
    uint32 drop_mac_addr2[2] = { 0xC2000002,   0x0180 };
    uint32 drop_mac_addr3[2] = { 0xC200000E,   0x0180 };
    uint32 drop_mac_addr_mask[2] = { 0xFFFFFFFF,   0xFFFF };
    
    pbmp_t lpbmp;  
    VLAN_DEFAULT_PBM_LOr_t vlan_default_pbm_lo;
    VLAN_DEFAULT_PBM_HIr_t vlan_default_pbm_hi;
    SYSTEM_CONFIG_TABLEm_t system_config_table;
    SOURCE_TRUNK_MAPm_t source_trunk_map;
    UNKNOWN_UCAST_BLOCK_MASK_LO_64r_t unknown_ucast_block_mask_lo_64;
    UNKNOWN_UCAST_BLOCK_MASK_HI_64r_t unknown_ucast_block_mask_hi_64;

    ING_EGRMSKBMAP_LOr_t ing_egrmskbmap_lo;
    ING_EGRMSKBMAP_HIr_t ing_egrmskbmap_hi;

    ING_PRI_CNG_MAPm_t ing_pri_cng_map;
    TRUNK32_CONFIG_TABLEm_t trunk32_config_table;
    TRUNK32_PORT_TABLEm_t trunk32_port_table;
    ING_VLAN_TAG_ACTION_PROFILEm_t ing_vlan_tag_action_profile;
#if CONFIG_GREYHOUND2_ROMCODE
    COS_MAPm_t cos_map;
#endif
    L2_USER_ENTRYm_t l2_user_entry;
#ifdef CFG_SWITCH_DOS_INCLUDED
    DOS_CONTROLr_t dos_control;
    DOS_CONTROL2r_t dos_control2;
#endif /* CFG_SWITCH_DOS_INCLUDED */
    AUX_ARB_CONTROL_2r_t aux_arb_control_2;
    ING_CONFIG_64r_t ing_config_64;
    VLAN_PROFILEm_t vlan_profile;
    EGR_PORT_64r_t egr_port_64;
    VLANm_t vlan;
    EGR_VLANm_t egr_vlan;
    PROTOCOL_PKT_CONTROLr_t protocol_pkt_control;
    L2_AGE_TIMERr_t l2_age_timer;
    VLAN_STGm_t vlan_stg;
    EGR_VLAN_STGm_t egr_vlan_stg;

    /* ING_OUTER_TPID[0] is allowed outer TPID values */
    SYSTEM_CONFIG_TABLEm_CLR(system_config_table);
    SYSTEM_CONFIG_TABLEm_OUTER_TPID_ENABLEf_SET(system_config_table, 1);

    SOURCE_TRUNK_MAPm_CLR(source_trunk_map);
#ifdef CFG_SWITCH_VLAN_UNAWARE_INCLUDED
    /* DISABLE_VLAN_CHECKS[Bit 63] = 1, PACKET_MODIFICATION_DISABLE[Bit 62] = 1 */
    SOURCE_TRUNK_MAPm_DISABLE_VLAN_CHECKSf_SET(source_trunk_map, 1);
    SOURCE_TRUNK_MAPm_PACKET_MODIFICATION_DISABLEf_SET(source_trunk_map, 1);
#endif /* CFG_SWITCH_VLAN_UNAWARE_INCLUDED */

    /* Default port_entry */
    PORTm_CLR(port_entry);
    PORTm_PORT_VIDf_SET(port_entry, 1);
    PORTm_TRUST_OUTER_DOT1Pf_SET(port_entry, 1);
    PORTm_OUTER_TPID_ENABLEf_SET(port_entry, 1);
    PORTm_TRUST_INCOMING_VIDf_SET(port_entry, 1);
    PORTm_CML_FLAGS_NEWf_SET(port_entry, 8);
    PORTm_CML_FLAGS_MOVEf_SET(port_entry, 8);

    /* Clear Unknown Unicast Block Mask. */                           
    UNKNOWN_UCAST_BLOCK_MASK_LO_64r_CLR(unknown_ucast_block_mask_lo_64);
    UNKNOWN_UCAST_BLOCK_MASK_HI_64r_CLR(unknown_ucast_block_mask_hi_64);

    /* Clear ingress block mask. */
    ING_EGRMSKBMAP_LOr_CLR(ing_egrmskbmap_lo);
    ING_EGRMSKBMAP_HIr_CLR(ing_egrmskbmap_hi);

    /* Configurations to guarantee no packet modifications */
    SOC_LPORT_ITER(lport) {
        WRITE_SYSTEM_CONFIG_TABLEm(unit,lport, system_config_table);

        WRITE_SOURCE_TRUNK_MAPm(unit, lport, source_trunk_map);            

        WRITE_PORTm(unit, lport, port_entry);

        WRITE_UNKNOWN_UCAST_BLOCK_MASK_LO_64r(unit, lport, unknown_ucast_block_mask_lo_64);
        WRITE_UNKNOWN_UCAST_BLOCK_MASK_HI_64r(unit, lport, unknown_ucast_block_mask_hi_64);

        WRITE_ING_EGRMSKBMAP_LOr(unit, lport, ing_egrmskbmap_lo);
        WRITE_ING_EGRMSKBMAP_HIr(unit, lport, ing_egrmskbmap_hi);
    }

    for (lport = 0 ; lport <= BCM5357X_LPORT_MAX ; lport++) {
        if (-1 == SOC_PORT_L2P_MAPPING(lport)) {
            continue;
        }

        /*
         * ING_PRI_CNG_MAP: Unity priority mapping and CNG = 0 or 1
         */
        for (j = 0; j < 16; j++) {
            ING_PRI_CNG_MAPm_SET(ing_pri_cng_map, dot1pmap[j]);
            WRITE_ING_PRI_CNG_MAPm(unit, lport*16+j, ing_pri_cng_map);
        }
    }

    /* TRUNK32_CONFIG_TABLE: OUTER_TPID_ENABLE[3:0] (Bit 0-3) = 0x1 */
    TRUNK32_CONFIG_TABLEm_CLR(trunk32_config_table);
    TRUNK32_CONFIG_TABLEm_OUTER_TPID_ENABLEf_SET(trunk32_config_table, 1);

    /*
     * TRUNK32_PORT_TABLE:
     * DISABLE_VLAN_CHECKS[Bit 31] = 1, PACKET_MODIFICATION_DISABLE[Bit 30] = 1
     */
    TRUNK32_PORT_TABLEm_CLR(trunk32_port_table);
#ifdef CFG_SWITCH_VLAN_UNAWARE_INCLUDED
    TRUNK32_PORT_TABLEm_DISABLE_VLAN_CHECKSf_SET(trunk32_port_table, 1);
    TRUNK32_PORT_TABLEm_PACKET_MODIFICATION_DISABLEf_SET(trunk32_port_table, 1);
#endif /* CFG_SWITCH_VLAN_UNAWARE_INCLUDED */

    for (i = 0; i < 32; i++) {
        WRITE_TRUNK32_CONFIG_TABLEm(unit, i, trunk32_config_table);
        WRITE_TRUNK32_PORT_TABLEm(unit, i, trunk32_port_table);
    }

#if CONFIG_GREYHOUND2_ROMCODE
    /*
     * Assign 1p priority mapping:
     *  pri_0/1 ==> low    (COS0)
     *  pri_2/3 ==> normal (COS1)
     *  pri_4/5 ==> medium (COS2)
     *  pri_6/7 ==> high   (COS3)
     */
#define INT_PRI_MAX  16 
    for (i = 0; i < INT_PRI_MAX ; i++) {
        if (i < 8) {
            COS_MAPm_SET(cos_map, cos_map_default[i]);
        } else {  
            COS_MAPm_SET(cos_map, cos_map_default[7]);    
        }   
        WRITE_COS_MAPm(unit, i, cos_map);
    }
#endif /* CONFIG_GREYHOUND2_ROMCODE */

    enable_jumbo_frame(unit);
    config_schedule_mode(unit);

    /*
     * VLAN_DEFAULT_PBM is used as defalut VLAN members for those ports
     * that disable VLAN checks.
     */
    VLAN_DEFAULT_PBM_LOr_CLR(vlan_default_pbm_lo);
    VLAN_DEFAULT_PBM_LOr_PORT_BITMAPf_SET(vlan_default_pbm_lo, SOC_PBMP(BCM5357X_ALL_PORTS_MASK));
    WRITE_VLAN_DEFAULT_PBM_LOr(0, vlan_default_pbm_lo);
    
    VLAN_DEFAULT_PBM_HIr_CLR(vlan_default_pbm_hi);
    VLAN_DEFAULT_PBM_HIr_PORT_BITMAPf_SET(vlan_default_pbm_hi, PBMP_WORD_GET(BCM5357X_ALL_PORTS_MASK, 2));
    WRITE_VLAN_DEFAULT_PBM_HIr(0, vlan_default_pbm_hi);

    /* ING_VLAN_TAG_ACTION_PROFILE:
     * UT_OTAG_ACTION (Bit 2-3) = 0x1
     * SIT_OTAG_ACTION (Bit 8-9) = 0x0
     * SOT_POTAG_ACTION (Bit 12-13) = 0x2
     * SOT_OTAG_ACTION (Bit 14-15) = 0x0
     * DT_POTAG_ACTION (Bit 20-21) = 0x2
     */
    ING_VLAN_TAG_ACTION_PROFILEm_CLR(ing_vlan_tag_action_profile);
    ING_VLAN_TAG_ACTION_PROFILEm_UT_OTAG_ACTIONf_SET(ing_vlan_tag_action_profile, 1);
    ING_VLAN_TAG_ACTION_PROFILEm_SIT_OTAG_ACTIONf_SET(ing_vlan_tag_action_profile, 0);
    ING_VLAN_TAG_ACTION_PROFILEm_SOT_POTAG_ACTIONf_SET(ing_vlan_tag_action_profile, 2);
    ING_VLAN_TAG_ACTION_PROFILEm_SOT_OTAG_ACTIONf_SET(ing_vlan_tag_action_profile, 0);
    ING_VLAN_TAG_ACTION_PROFILEm_DT_POTAG_ACTIONf_SET(ing_vlan_tag_action_profile, 2);
    WRITE_ING_VLAN_TAG_ACTION_PROFILEm(unit, 0, ing_vlan_tag_action_profile);
  
    /*
     * Program l2 user entry table to drop below MAC addresses:
     * 0x0180C2000001, 0x0180C2000002 and 0x0180C200000E
     */

    /* VALID[Bit 0] = 1 */
    L2_USER_ENTRYm_CLR(l2_user_entry);
    L2_USER_ENTRYm_VALIDf_SET(l2_user_entry, 1);
    L2_USER_ENTRYm_DST_DISCARDf_SET(l2_user_entry, 1);
    L2_USER_ENTRYm_KEY_TYPEf_SET(l2_user_entry, 0);
    L2_USER_ENTRYm_BPDUf_SET(l2_user_entry, 1);
    L2_USER_ENTRYm_KEY_TYPE_MASKf_SET(l2_user_entry, 1);   
    L2_USER_ENTRYm_MAC_ADDRf_SET(l2_user_entry, drop_mac_addr1);
    L2_USER_ENTRYm_MAC_ADDR_MASKf_SET(l2_user_entry, drop_mac_addr_mask);   
    WRITE_L2_USER_ENTRYm(unit, 0, l2_user_entry);

    L2_USER_ENTRYm_MAC_ADDRf_SET(l2_user_entry, drop_mac_addr2);
    WRITE_L2_USER_ENTRYm(unit, 1, l2_user_entry);

    L2_USER_ENTRYm_MAC_ADDRf_SET(l2_user_entry, drop_mac_addr3);
    WRITE_L2_USER_ENTRYm(unit, 2, l2_user_entry);

#if !CONFIG_GREYHOUND2_ROMCODE
#ifdef CFG_SWITCH_DOS_INCLUDED
    /*
     * Enable following Denial of Service protections:
     * DROP_IF_SIP_EQUALS_DIP
     * MIN_TCPHDR_SIZE = 0x14 (Default)
     * IP_FIRST_FRAG_CHECK_ENABLE
     * BIG_ICMP_PKT_SIZE = 0x208
     * TCP_HDR_OFFSET_EQ1_ENABLE
     * TCP_HDR_PARTIAL_ENABLE
     * ICMP_V4_PING_SIZE_ENABLE
     * ICMP_FRAG_PKTS_ENABLE 
     */
    DOS_CONTROLr_CLR(dos_control);
    DOS_CONTROLr_DROP_IF_SIP_EQUALS_DIPf_SET(dos_control, 1);
    DOS_CONTROLr_MIN_TCPHDR_SIZEf_SET(dos_control, 0x14);
    DOS_CONTROLr_IP_FIRST_FRAG_CHECK_ENABLEf_SET(dos_control, 1);
    DOS_CONTROLr_BIG_ICMP_PKT_SIZEf_SET(dos_control, 0x208);
    WRITE_DOS_CONTROLr(unit, dos_control);

    DOS_CONTROL2r_CLR(dos_control2);
    DOS_CONTROL2r_TCP_HDR_OFFSET_EQ1_ENABLEf_SET(dos_control2, 1);
    DOS_CONTROL2r_TCP_HDR_PARTIAL_ENABLEf_SET(dos_control2, 1);
    DOS_CONTROL2r_ICMP_V4_PING_SIZE_ENABLEf_SET(dos_control2, 1);
    DOS_CONTROL2r_ICMP_FRAG_PKTS_ENABLEf_SET(dos_control2, 1);
    WRITE_DOS_CONTROL2r(unit, dos_control2);

#endif /* CFG_SWITCH_DOS_INCLUDED */

    /* enable FP_REFRESH_ENABLE [Bit 26] */
    READ_AUX_ARB_CONTROL_2r(unit, aux_arb_control_2);
    AUX_ARB_CONTROL_2r_FP_REFRESH_ENABLEf_SET(aux_arb_control_2, 1);
    WRITE_AUX_ARB_CONTROL_2r(unit, aux_arb_control_2);

    /*
     * Enable IPV4_RESERVED_MC_ADDR_IGMP_ENABLE[Bit 31], APPLY_EGR_MASK_ON_L3[Bit 13]
     * and APPLY_EGR_MASK_ON_L2[Bit 12]
     * Disable L2DST_HIT_ENABLE[Bit 2]
     */
    READ_ING_CONFIG_64r(unit, ing_config_64);
    ING_CONFIG_64r_IPV4_RESERVED_MC_ADDR_IGMP_ENABLEf_SET(ing_config_64, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L3f_SET(ing_config_64, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L2f_SET(ing_config_64, 1);
    ING_CONFIG_64r_L2DST_HIT_ENABLEf_SET(ing_config_64, 0);
    WRITE_ING_CONFIG_64r(unit, ing_config_64); 

    /*
     * L3_IPV6_PFM=1, L3_IPV4_PFM=1, L2_PFM=1, IPV6L3_ENABLE=1, IPV4L3_ENABLE=1
     * IPMCV6_L2_ENABLE=1, IPMCV6_ENABLE=1, IPMCV4_L2_ENABLE=1, IPMCV4_ENABLE=1
     */
    VLAN_PROFILEm_CLR(vlan_profile);
    VLAN_PROFILEm_L3_IPV6_PFMf_SET(vlan_profile, 1);
    VLAN_PROFILEm_L3_IPV4_PFMf_SET(vlan_profile, 1);
    VLAN_PROFILEm_L2_PFMf_SET(vlan_profile, 1);
    VLAN_PROFILEm_IPV6L3_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILEm_IPV4L3_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILEm_IPMCV6_L2_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILEm_IPMCV6_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILEm_IPMCV4_L2_ENABLEf_SET(vlan_profile, 1);
    VLAN_PROFILEm_IPMCV4_ENABLEf_SET(vlan_profile, 1);
    WRITE_VLAN_PROFILEm(unit,0,vlan_profile);

#endif /* !CONFIG_GREYHOUND2_ROMCODE */

    /* Do VLAN Membership check EN_EFILTER[Bit 3] for the outgoing port */
    SOC_LPORT_ITER(lport) {
        READ_EGR_PORT_64r(unit, lport, egr_port_64);
        EGR_PORT_64r_EN_EFILTERf_SET(egr_port_64, 1);
        WRITE_EGR_PORT_64r(unit, lport, egr_port_64);
    }

#if CFG_RXTX_SUPPORT_ENABLED
    /*
     * Use VLAN 0 for CPU to transmit packets
     * All ports are untagged members, with STG=1 and VLAN_PROFILE_PTR=0
     */
    PBMP_ASSIGN(lpbmp, BCM5357X_ALL_PORTS_MASK);
    PBMP_PORT_ADD(lpbmp, 0);
    VLANm_CLR(vlan);
    VLANm_VALIDf_SET(vlan, 1);
    VLANm_STGf_SET(vlan, 1);
    VLANm_VLAN_PROFILE_PTRf_SET(vlan, 0);
    VLANm_PORT_BITMAPf_SET(vlan, SOC_PBMP(lpbmp)); 
    WRITE_VLANm(unit, 0, vlan);

    EGR_VLANm_CLR(egr_vlan);
    EGR_VLANm_PORT_BITMAPf_SET(egr_vlan,SOC_PBMP(lpbmp)); 
    PBMP_PORT_REMOVE(lpbmp, 0);
    EGR_VLANm_UT_BITMAPf_SET(egr_vlan, SOC_PBMP(lpbmp));
    EGR_VLANm_STGf_SET(egr_vlan, 1);
    EGR_VLANm_VALIDf_SET(egr_vlan, 1);
    WRITE_EGR_VLANm(unit, 0, egr_vlan);

#ifdef __BOOTLOADER__
    /* Default VLAN 1 with STG=1 and VLAN_PROFILE_PTR=0 for bootloader */
    PBMP_ASSIGN(lpbmp, BCM5357X_ALL_PORTS_MASK);
    VLANm_CLR(vlan);
    VLANm_STGf_SET(vlan, 1);
    VLANm_VALIDf_SET(vlan, 1);
    VLANm_VLAN_PROFILE_PTRf_SET(vlan, 0);
    VLANm_PORT_BITMAPf_SET(vlan, SOC_PBMP(lpbmp)); 
    WRITE_VLANm(unit, 1, vlan);

    EGR_VLANm_CLR(egr_vlan);
    EGR_VLANm_UT_BITMAPf_SET(egr_vlan, SOC_PBMP(lpbmp));
    EGR_VLANm_PORT_BITMAPf_SET(egr_vlan,SOC_PBMP(lpbmp)); 
    EGR_VLANm_STGf_SET(egr_vlan, 1);
    EGR_VLANm_VALIDf_SET(egr_vlan, 1);
    WRITE_EGR_VLANm(unit, 1, egr_vlan);
#endif /* __BOOTLOADER__ */

    /* Set VLAN_STG and EGR_VLAN_STG */
    VLAN_STGm_CLR(vlan_stg);
    VLAN_STGm_SET(vlan_stg,0,0xfffffff0);
    VLAN_STGm_SET(vlan_stg,1,0xffffffff);
    VLAN_STGm_SET(vlan_stg,2,0xffffffff);
    VLAN_STGm_SET(vlan_stg,3,0xffffffff);
    VLAN_STGm_SET(vlan_stg,4,0x0000000f);
    WRITE_VLAN_STGm(unit, 1, vlan_stg);

    EGR_VLAN_STGm_CLR(egr_vlan_stg);
    EGR_VLAN_STGm_SET(egr_vlan_stg,0,0xfffffff0);
    EGR_VLAN_STGm_SET(egr_vlan_stg,1,0xffffffff);
    EGR_VLAN_STGm_SET(egr_vlan_stg,2,0xffffffff);
    EGR_VLAN_STGm_SET(egr_vlan_stg,3,0xffffffff);
    EGR_VLAN_STGm_SET(egr_vlan_stg,4,0x0000000f);
    WRITE_EGR_VLAN_STGm(unit, 1, egr_vlan_stg);

    /* Make PORT_VID[Bit 35:24] = 0 for CPU port */
    READ_PORTm(unit, 0, port_entry);
    PORTm_PORT_VIDf_SET(port_entry, 0);
    WRITE_PORTm(unit, 0, port_entry);
#if !CONFIG_GREYHOUND2_ROMCODE
    /*
     * Trap DHCP[Bit 0] and ARP packets[Bit 4, 6] to CPU.
     * Note ARP reply is copied to CPU ONLY when l2 dst is hit.
     */
    PROTOCOL_PKT_CONTROLr_CLR(protocol_pkt_control);
    PROTOCOL_PKT_CONTROLr_DHCP_PKT_TO_CPUf_SET(protocol_pkt_control, 1);
    PROTOCOL_PKT_CONTROLr_ARP_REPLY_TO_CPUf_SET(protocol_pkt_control, 1);
    PROTOCOL_PKT_CONTROLr_ARP_REQUEST_TO_CPUf_SET(protocol_pkt_control, 1);

    SOC_LPORT_ITER(lport) {        
        WRITE_PROTOCOL_PKT_CONTROLr(unit, lport, protocol_pkt_control);
    }
#endif
#endif /* CFG_RXTX_SUPPORT_ENABLED */

    /* Enable aging timer */
    READ_L2_AGE_TIMERr(unit, l2_age_timer);
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 1);
    WRITE_L2_AGE_TIMERr(unit, l2_age_timer);
}

#if 0 
int _firmware_helper(void *ctx, uint32 offset, uint32 size, void *data) {
    return SYS_OK;
}

extern int
bmd_phy_fw_helper_set(int unit, int port,
                      int (*fw_helper)(void *, uint32, uint32, void *));
#endif



/* Function:
 *   bcm5357x_sw_init
 * Description:
 *   Perform chip specific initialization.
 *   This will be called by board_init()
 * Parameters:
 *   None
 * Returns:
 *   None
 */
sys_error_t
bcm5357x_sw_init(void)
{
    uint8 unit = 0x0;

    pbmp_t okay_pbmp;

    sys_error_t rv = SYS_OK;
    CMIC_CPS_RESETr_t cmic_cps_reset;
    MHOST_0_MHOST_DEBUG_CTRLr_t mhost_0_mhost_debug_ctrl;
    /* Clear all gh2_sw_info structure */   
    sal_memset(&gh2_sw_info, 0, sizeof(gh2_sw_info));

    /* Get chip revision */
    bcm5357x_chip_revision(unit, &gh2_sw_info.devid, &gh2_sw_info.revid);

    /* To enable ICE */
    MHOST_0_MHOST_DEBUG_CTRLr_CLR(mhost_0_mhost_debug_ctrl);
    MHOST_0_MHOST_DEBUG_CTRLr_SET(mhost_0_mhost_debug_ctrl, 0xFFFFFFFF);
    WRITE_MHOST_0_MHOST_DEBUG_CTRLr(0, mhost_0_mhost_debug_ctrl);

#if CFG_CONSOLE_ENABLED
    sal_printf("\ndevid = 0x%x, revid = 0x%x\n", gh2_sw_info.devid, gh2_sw_info.revid);
#endif /* CFG_CONSOLE_ENABLED */

    /* CPS reset complete SWITCH and CMICd */
    CMIC_CPS_RESETr_CLR(cmic_cps_reset);
    CMIC_CPS_RESETr_CPS_RESETf_SET(cmic_cps_reset, 1);
    WRITE_CMIC_CPS_RESETr(unit, cmic_cps_reset);

#if CONFIG_EMULATION
    sal_usleep(250000);
#else
    sal_usleep(1000);
#endif

    soc_reset(unit);

    soc_gh2_tsc_reset(unit);

    soc_misc_init(unit);

    soc_mmu_init(unit);

    bcm5357x_pcm_software_init(unit);

    pcm_port_probe_init(unit, BCM5357X_ALL_PORTS_MASK, &okay_pbmp);

    bcm5357x_system_init(unit);

    bcm5357x_linkscan_init(LINKSCAN_INTERVAL);

    if (rv) {
        return SYS_ERR;
    } else {
        return SYS_OK;
    }
}

soc_switch_t soc_switch_bcm5357x =
{
    bcm5357x_chip_type,
    NULL,
    bcm5357x_port_count,
    NULL,
    NULL,
#if CFG_RXTX_SUPPORT_ENABLED
    bcm5357x_rx_set_handler,
    bcm5357x_rx_fill_buffer,
    bcm5357x_tx,
#endif /* CFG_RXTX_SUPPORT_ENABLED */

    bcm5357x_link_status,
    bcm5357x_chip_revision,
    bcm5357x_reg_get,
    bcm5357x_reg_set,
    bcm5357x_mem_get,
    bcm5357x_mem_set,
#ifdef CFG_SWITCH_VLAN_INCLUDED
    bcm5357x_pvlan_egress_set,
    bcm5357x_pvlan_egress_get,
    bcm5357x_qvlan_port_set,
    bcm5357x_qvlan_port_get,
    bcm5357x_vlan_create,
    bcm5357x_vlan_destroy,
    bcm5357x_vlan_type_set,
    bcm5357x_vlan_reset,
#endif  /* CFG_SWITCH_VLAN_INCLUDED */
    bcm5357x_phy_reg_get,
    bcm5357x_phy_reg_set,
};

