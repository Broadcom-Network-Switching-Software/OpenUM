/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#include "utils/net.h"
#include <shared/bslenum.h>


bcm5354x_sw_info_t wh2_sw_info;

extern int matched_devid_idx;
extern wolfhound2_sku_info_t wh2_sku_port_config[];


static void
soc_wh2_tsc_reset(uint8 unit){
    
    soc_wolfhound2_port_reset(unit);
}

 
static void
soc_reset(uint8 unit)
{
    uint32 to_usec;
    uint32 rval;
    CMIC_SBUS_RING_MAPr_t ring_map;
    CMIC_SBUS_TIMEOUTr_t cmic_sbus_timeout;
    TOP_SOFT_RESET_REGr_t top_soft_reset_reg;
    TOP_SOFT_RESET_REG_2r_t top_soft_reset_reg_2;
    TOP_CORE_PLL_CTRL4r_t   top_core_pll_ctrl4;
    TOP_MISC_CONTROL_1r_t   top_misc_control_1;    
#if (CFG_UM_BCMSIM==1) || (CONFIG_EMULATION==1)
#else
    uint32          value;
#endif
    uint32          strap_sts_1, qgphy_core_map;

    wolfhound2_sku_info_t  *matched_port_config = NULL;
    uint8           qgphy5_lane, sgmii_4p0_lane;
    int             option;    
    TOP_STRAP_STATUS_1r_t    top_strap_status_1;

#if CONFIG_EMULATION
    to_usec = 250000;
#else
    to_usec = 10000;
#endif /* CONFIG_EMULATION */

    
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x00000000);  /* block 7  - 0 */
    WRITE_CMIC_SBUS_RING_MAPr(unit, 0, ring_map);
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x00430000);  /* block 15 - 8 */
    WRITE_CMIC_SBUS_RING_MAPr(unit, 1, ring_map);
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x00005064);  /* block 23 - 16 */
    WRITE_CMIC_SBUS_RING_MAPr(unit, 2, ring_map);
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x00000000);  /* block 31 - 24 */
    WRITE_CMIC_SBUS_RING_MAPr(unit, 3, ring_map);
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x77772222);  /* block 39 - 32 */
    WRITE_CMIC_SBUS_RING_MAPr(unit, 4, ring_map);
    CMIC_SBUS_RING_MAPr_SET(ring_map, 0x00000111);  /* block 37 - 40 */
    WRITE_CMIC_SBUS_RING_MAPr(unit, 5, ring_map);

    
    CMIC_SBUS_TIMEOUTr_CLR(cmic_sbus_timeout);
    CMIC_SBUS_TIMEOUTr_TIMEOUT_VALf_SET(cmic_sbus_timeout, 0x7d0);
    WRITE_CMIC_SBUS_TIMEOUTr(unit,cmic_sbus_timeout);

    
    sal_usleep(to_usec);
    

    bcm5354x_chip_revision(unit, &wh2_sw_info.devid, &wh2_sw_info.revid);
    //sal_printf("\ndevid = 0x%x, revid = 0x%x\n", wh2_sw_info.devid, wh2_sw_info.revid);

    soc_port_config_init(unit);
       
    /* set core clock to 62.5MHz */
    if (sku_port_config->freq == 62) {
        rval = 0x28;
    /* set core clock to 41.67MHz */
    } else if (sku_port_config->freq == 41) {
        rval = 0x3c;
    /* set core clock to 104.17MHz */
    } else {
        rval = 0x18;
    }

    READ_TOP_CORE_PLL_CTRL4r(unit, top_core_pll_ctrl4);
    TOP_CORE_PLL_CTRL4r_MSTR_CH0_MDIVf_SET(top_core_pll_ctrl4, rval);
    WRITE_TOP_CORE_PLL_CTRL4r(unit, top_core_pll_ctrl4);
    
    READ_TOP_MISC_CONTROL_1r(unit, top_misc_control_1);
    TOP_MISC_CONTROL_1r_CMIC_TO_CORE_PLL_LOADf_SET(top_misc_control_1, 1);
    WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_control_1);
    
    /* Check option and strap pin status to do the proper initialization */
    matched_port_config = &wh2_sku_port_config[matched_devid_idx];
    option = matched_port_config->config_op;
    qgphy_core_map = matched_port_config->qgphy_core_map;
    qgphy5_lane = matched_port_config->qgphy5_lane;
    sgmii_4p0_lane = matched_port_config->sgmii_4p0_lane;

#define WH2_STRAP_GPHY_SGMII_SEL_0  (1 << 0) 
#define WH2_STRAP_GPHY_SGMII_SEL_1  (1 << 1)
    strap_sts_1 = 0;
    option = option % 6;
    READ_TOP_STRAP_STATUS_1r(unit, top_strap_status_1);
    strap_sts_1= TOP_STRAP_STATUS_1r_STRAP_STATUSf_GET(top_strap_status_1);
    sal_printf("STRAP_STATUS=0x%08x\n", strap_sts_1);
    
    /*
    *   			                                            QGPHY5			SGMII_4P0			
    *   option#	strap_gphy_sgmii_sel[0]	strap_gphy_sgmii_sel[1]	0	1	2	3	0	1	2	3
    *   1	                        0	                    0				C	S	S	S	S
    *   2	                        1	                    1	S	S	S	S				C
    *   3	                        1	                    0	S	S		C			S	S
    *   4	                        1	                    0	S	S		C			S	S
    *   5	                        0	                    0				C	S	S	S	S
    *   6	                        1	                    1	S	S	S	S				C
    *   NA	                        0	                    1			S	S	S	S		C
    */						
    
#if (CFG_UM_BCMSIM!=1) && (CONFIG_EMULATION!=1) //if (!SAL_BOOT_SIMULATION) 
    /* check strap v.s. option */
    value = (strap_sts_1 >> 1) & 0x3;
    {
        switch (option) {
            case 0:
            case 2:
                if (value != 3) {
                    sal_printf("%s..line=%d:Invalid option :option=0x%08x value=%d\n", __func__, __LINE__, option, value);
                }
                break;
            case 3:
            case 4:
                if (value != 1) {
                    sal_printf("%s..line=%d:Invalid option :option=0x%08x value=%d\n", __func__, __LINE__, option, value);
                }
                break;
            case 1:
            case 5:
                if (value != 0) {
                    sal_printf("%s..line=%d:Invalid option :option=0x%08x value=%d\n", __func__, __LINE__, option, value);
                }
                break;
        }

        if (value & WH2_STRAP_GPHY_SGMII_SEL_0) {
            /* front ports select GPHY */
            if (!(qgphy_core_map & 0x20) || !(qgphy5_lane & 0x3)) {
                sal_printf("%s..line=%d:Invalid option :option=0x%08x value=%d\n", __func__, __LINE__, option, value);
            }
        }
        if (value & WH2_STRAP_GPHY_SGMII_SEL_1) {
            /* front ports select GPHY, OOB port select SGMII */
            if (!(qgphy_core_map & 0x20) || !(qgphy5_lane & 0xC)) {
                sal_printf("%s..:Invalid option (unmatch with strap pin)_5\n", __func__);
            }
        }
    }
#endif//if (CFG_UM_BCMSIM!=1) && (CONFIG_EMULATION!=1) //if (!SAL_BOOT_SIMULATION) 


    /* re-ordering the port sequence if required*/
    /* Bring port block out of reset */

    /* GPHY or SGMII configuration need to be considered here */
    /* 
    * soc_wh2_sgmii_init(int unit, int sgmii_inst)
    * soc_wh2_sgmii_init(int unit, 0): sgmii_inst==0, reset the TOP_SGMII_CTRL_REGr
    * soc_wh2_sgmii_init(int unit, 1): sgmii_inst==1, reset the GPORT_SGMII0_CTRL_REGr
    */
    if (sgmii_4p0_lane) {
        soc_wh2_sgmii_init_top(unit, 0);
    }
    soc_wh2_qgphy_init(unit, qgphy_core_map, qgphy5_lane);
        
    READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
    TOP_SOFT_RESET_REGr_TOP_GXP0_RST_Lf_SET(top_soft_reset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_GXP1_RST_Lf_SET(top_soft_reset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_GXP2_RST_Lf_SET(top_soft_reset_reg, 1);
    WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
    sal_usleep(to_usec);    
    
    /*
     * Bring network sync out of reset
     * TOP_TS_RST_Lf [Bit 15]
     */
    READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_soft_reset_reg, 1);
    WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
    sal_usleep(to_usec);
    

    /*
     * Bring network sync PLL out of reset
     * TOP_TS_PLL_RST_Lf [Bit 8] and then TOP_TS_PLL_POST_RST_Lf [Bit 9]
     */
    READ_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset_reg_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_soft_reset_reg_2, 1);
    WRITE_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset_reg_2);
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_soft_reset_reg_2, 1);
    WRITE_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset_reg_2);
    sal_usleep(to_usec);    


    /*
     * Bring IP, EP, and MMU blocks out of reset
     * TOP_IP_RST_L [Bit 0], TOP_EP_RST_Lf [Bit 1], TOP_MMU_RST_Lf [Bit 2]
     */
    READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_soft_reset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_soft_reset_reg, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_soft_reset_reg, 1);
    WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
    sal_usleep(to_usec);    
}

soc_chip_type_t
bcm5354x_chip_type(void)
{
    return SOC_TYPE_SWITCH_XGS;
}

uint8
bcm5354x_port_count(uint8 unit)
{
    if (unit > 0) {
        return 0;
    }
    return SOC_PORT_COUNT(unit);
}


sys_error_t
bcm5354x_chip_revision(uint8 unit, uint16 *dev, uint16 *rev)
{

    CMIC_DEV_REV_IDr_t cmic_dev_rev_id;

    if (unit > 0) {
        return -1;
    }

    READ_CMIC_DEV_REV_IDr(unit,cmic_dev_rev_id);
    wh2_sw_info.devid = CMIC_DEV_REV_IDr_DEV_IDf_GET(cmic_dev_rev_id);
    wh2_sw_info.revid = CMIC_DEV_REV_IDr_REV_IDf_GET(cmic_dev_rev_id);

    return 0;
}


#define JUMBO_FRM_SIZE (9216)

static void
enable_jumbo_frame(uint8 unit)
{
    int lport;
    FRM_LENGTHr_t frm_length;

    FRM_LENGTHr_CLR(frm_length);
    FRM_LENGTHr_MAXFRf_SET(frm_length, JUMBO_FRM_SIZE);

    SOC_LPORT_ITER(lport) {
        /* All the port block type is PORT_BLOCK_TYPE_GXPORT, assigned in soc_port_block_info_get() */
        if (IS_GX_PORT(lport)) {
            WRITE_FRM_LENGTHr(unit, lport, frm_length);
        }
    }

}

static void
soc_pipe_mem_clear(uint8 unit)
{

    ING_HW_RESET_CONTROL_1r_t ing_hw_reset_control_1;
    ING_HW_RESET_CONTROL_2r_t ing_hw_reset_control_2;
    EGR_HW_RESET_CONTROL_0r_t egr_hw_reset_control_0;
    EGR_HW_RESET_CONTROL_1r_t egr_hw_reset_control_1;

    /*
     * Reset the IPIPE and EPIPE block
     */
    ING_HW_RESET_CONTROL_1r_CLR(ing_hw_reset_control_1);
    WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_hw_reset_control_1);

    /*
     * Set COUNT[Bit 15-0] to # entries in largest IPIPE table, L2_ENTRYm 0x4000
     * RESET_ALL [Bit 16] = 1, VALID [Bit 17] = 1
     */
    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_reset_control_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_hw_reset_control_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_hw_reset_control_2, 1);
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_hw_reset_control_2, L2_ENTRYm_MAX+1);
    WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2);

    EGR_HW_RESET_CONTROL_0r_CLR(egr_hw_reset_control_0);
    WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_hw_reset_control_0);

    /*
     * Set COUNT[Bit 15-0] to # entries in largest EPIPE table, EGR_VLAN 0x1000
     * RESET_ALL [Bit 16] = 1, VALID [Bit 17] = 1
     */

    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_control_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_hw_reset_control_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_hw_reset_control_1, 1);
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_hw_reset_control_1, EGR_VLANm_MAX + 1);
    WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1);

    
    /* Wait for IPIPE memory initialization done. */
    do {
        /* Polling DONE[Bit 18] */
        READ_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2);        
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_hw_reset_control_2)) {
            break;
        }

    } while (1);

    /* Wait for EPIPE memory initialization done. */
    do {
        /* Polling DONE[Bit 18] */
        READ_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1);        
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_hw_reset_control_1)) {
            break;
        }

    } while (1);

    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_reset_control_2);
    WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2);

    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_control_1);
    WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1);

}


static void
soc_pgw_ge_tdm_config(int unit){
    int pport, bindex;
    int pgw_ge0_block_ge0, pgw_ge0_block_ge1, pgw_ge1_block_ge0, pgw_ge1_block_ge1, pgw_ge2_block_ge0, pgw_ge2_block_ge1;
    int ge0_block_first_lport, ge1_block_first_lport, ge2_block_first_lport;
    PGW_GE0_MODE_REGr_t     pgw_ge0_mode_reg;
    PGW_GE1_MODE_REGr_t     pgw_ge1_mode_reg;
    wolfhound2_sku_info_t  *matched_port_config = &wh2_sku_port_config[matched_devid_idx];
    int speed;

    
    pgw_ge0_block_ge0 = pgw_ge0_block_ge1 = pgw_ge1_block_ge0 = pgw_ge1_block_ge1 = pgw_ge2_block_ge0 = pgw_ge2_block_ge1 = 0;
    ge0_block_first_lport = ge1_block_first_lport = ge2_block_first_lport = 0;

    SOC_PPORT_ITER(pport) {
        speed = matched_port_config->speed_max[pport];
        
        if(bcm5354x_pgw_ge_pport_to_blockid[pport] == PGW_GE0_BLOCK_ID){
            
            bindex = bcm5354x_pgw_ge_lport_to_index_in_block[SOC_PORT_P2L_MAPPING(pport)];
            if (bindex==0)
                ge0_block_first_lport = SOC_PORT_P2L_MAPPING(pport);
            
            if(speed != -1){    /* valid P port */
                if (bindex < 8){
                    /* index 0-7 controlled by PGW_GE0_MODE_REG */
                    pgw_ge0_block_ge0++;
                } else {
                    /* index 8-15 controlled by PGW_GE1_MODE_REG */
                    pgw_ge0_block_ge1++;
                }
            }
        }
        
        if(bcm5354x_pgw_ge_pport_to_blockid[pport] == PGW_GE1_BLOCK_ID){
            bindex = bcm5354x_pgw_ge_lport_to_index_in_block[SOC_PORT_P2L_MAPPING(pport)];
            if (bindex==0)
                ge1_block_first_lport = SOC_PORT_P2L_MAPPING(pport);
                
            if(speed != -1){    /* valid P port */
                if (bindex < 8){
                    /* index 0-7 controlled by PGW_GE0_MODE_REG */
                    pgw_ge1_block_ge0++;
                } else {
                    /* index 8-15 controlled by PGW_GE1_MODE_REG */
                    pgw_ge1_block_ge1++;
                }
            }
        }
        
        if(bcm5354x_pgw_ge_pport_to_blockid[pport] == PGW_GE2_BLOCK_ID){

            bindex = bcm5354x_pgw_ge_lport_to_index_in_block[SOC_PORT_P2L_MAPPING(pport)];
            if (bindex==0)
                ge2_block_first_lport = SOC_PORT_P2L_MAPPING(pport);
                
            if(speed != -1){    /* valid P port */
                if (bindex < 8){
                    /* index 0-7 controlled by PGW_GE0_MODE_REG */
                    pgw_ge2_block_ge0++;
                } else {
                    /* index 8-15 controlled by PGW_GE1_MODE_REG */
                    pgw_ge2_block_ge1++;
                }
            }
        }
    }    
    
#if UM_DEBUG
    sal_printf("%s..:pgw_ge0_block_ge0=%d\n", __func__, pgw_ge0_block_ge0);
    sal_printf("%s..:pgw_ge0_block_ge1=%d\n", __func__, pgw_ge0_block_ge1);
    sal_printf("%s..:pgw_ge1_block_ge0=%d\n", __func__, pgw_ge1_block_ge0);
    sal_printf("%s..:pgw_ge1_block_ge1=%d\n", __func__, pgw_ge1_block_ge1);
    sal_printf("%s..:pgw_ge2_block_ge0=%d\n", __func__, pgw_ge2_block_ge0);
    sal_printf("%s..:pgw_ge2_block_ge1=%d\n", __func__, pgw_ge2_block_ge1);
    
    sal_printf("%s..:ge0_block_first_lport=%d\n", __func__, ge0_block_first_lport);
    sal_printf("%s..:ge1_block_first_lport=%d\n", __func__, ge1_block_first_lport);
    sal_printf("%s..:ge2_block_first_lport=%d\n", __func__, ge2_block_first_lport);
#endif
    
    if(wh2_sw_info.devid != 0x8549 && (ge0_block_first_lport !=0) ){
        
        /* For 53547 and 53548, set both PGW_GE0_MODE_REG and PGW_GE1_MODE_REG */
        READ_PGW_GE0_MODE_REGr(unit, ge0_block_first_lport, pgw_ge0_mode_reg);
        PGW_GE0_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge0_mode_reg, pgw_ge0_block_ge0);
        WRITE_PGW_GE0_MODE_REGr(unit, ge0_block_first_lport, pgw_ge0_mode_reg);
        READ_PGW_GE1_MODE_REGr(unit, ge0_block_first_lport, pgw_ge1_mode_reg);
        PGW_GE1_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge1_mode_reg, pgw_ge0_block_ge1);
        WRITE_PGW_GE1_MODE_REGr(unit, ge0_block_first_lport, pgw_ge1_mode_reg);
    }else{
        
        /* For 53549, ge0_block is not used. set both PGW_GE0_MODE_REG and PGW_GE1_MODE_REG to 0 with raw access 
        *   due to bcm5354x_pgw_ge_lport_to_blockid[ge0_block_first_lport=0] would get -1
        */
        bcm5354x_reg_get(unit, PGW_GE0_BLOCK_ID, R_PGW_GE0_MODE_REG, &(pgw_ge0_mode_reg._pgw_ge0_mode_reg));
        PGW_GE0_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge0_mode_reg, pgw_ge0_block_ge0);
        bcm5354x_reg_set(unit, PGW_GE0_BLOCK_ID, R_PGW_GE0_MODE_REG, (pgw_ge0_mode_reg._pgw_ge0_mode_reg));
        
        bcm5354x_reg_get(unit, PGW_GE0_BLOCK_ID, R_PGW_GE1_MODE_REG, &(pgw_ge1_mode_reg._pgw_ge1_mode_reg));
        PGW_GE1_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge1_mode_reg, pgw_ge0_block_ge1);
        bcm5354x_reg_set(unit, PGW_GE0_BLOCK_ID, R_PGW_GE1_MODE_REG, (pgw_ge1_mode_reg._pgw_ge1_mode_reg));
    }
    
    READ_PGW_GE0_MODE_REGr(unit, ge1_block_first_lport, pgw_ge0_mode_reg);
    PGW_GE0_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge0_mode_reg, pgw_ge1_block_ge0);
    WRITE_PGW_GE0_MODE_REGr(unit, ge1_block_first_lport, pgw_ge0_mode_reg);
    READ_PGW_GE1_MODE_REGr(unit, ge1_block_first_lport, pgw_ge1_mode_reg);
    PGW_GE1_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge1_mode_reg, pgw_ge1_block_ge1);
    WRITE_PGW_GE1_MODE_REGr(unit, ge1_block_first_lport, pgw_ge1_mode_reg);
    
    READ_PGW_GE0_MODE_REGr(unit, ge2_block_first_lport, pgw_ge0_mode_reg);
    PGW_GE0_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge0_mode_reg, pgw_ge2_block_ge0);
    WRITE_PGW_GE0_MODE_REGr(unit, ge2_block_first_lport, pgw_ge0_mode_reg);
    READ_PGW_GE1_MODE_REGr(unit, ge2_block_first_lport, pgw_ge1_mode_reg);
    PGW_GE1_MODE_REGr_GP0_TDM_MODEf_SET(pgw_ge1_mode_reg, pgw_ge2_block_ge1);
    WRITE_PGW_GE1_MODE_REGr(unit, ge2_block_first_lport, pgw_ge1_mode_reg);
}

static void
soc_init_port_mapping(uint8 unit)
{
    int i;
    uint32 val;

    ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_t ing_physical_to_logical_port_number_mapping_table;
    EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr_t egr_logical_to_physical_port_number_mapping;
    LOG_TO_PHY_PORT_MAPPINGr_t log_to_phy_port_mapping;


    /* Ingress physical to logical port mapping */
    for (i = 0; i <= BCM5354X_PORT_MAX; i++) {
        val = (SOC_PORT_P2L_MAPPING(i) == -1) ? 0x3F: SOC_PORT_P2L_MAPPING(i);

        ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_CLR(ing_physical_to_logical_port_number_mapping_table);
        ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_LOGICAL_PORT_NUMBERf_SET(ing_physical_to_logical_port_number_mapping_table, val);
        
        
        WRITE_ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm(unit,i, ing_physical_to_logical_port_number_mapping_table);
        ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_CLR(ing_physical_to_logical_port_number_mapping_table);        
        READ_ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm(unit,i, ing_physical_to_logical_port_number_mapping_table);

    }

    /* Egress logical to physical port mapping, needs a way for maximum logical port? */
    for (i = 0; i <= BCM5354X_LPORT_MAX; i++) {
        val = (SOC_PORT_L2P_MAPPING(i) == -1) ? 0x3F : SOC_PORT_L2P_MAPPING(i);
        EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr_CLR(egr_logical_to_physical_port_number_mapping);
        EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr_PHYSICAL_PORT_NUMBERf_SET(egr_logical_to_physical_port_number_mapping, val);
        WRITE_EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr(unit, i, egr_logical_to_physical_port_number_mapping);
        /* MMU logical to physical port mapping
        * (Here, Same as Egress logical to physical port mapping)
        */
        if (val != 0x3F) {
            LOG_TO_PHY_PORT_MAPPINGr_CLR(log_to_phy_port_mapping);
            LOG_TO_PHY_PORT_MAPPINGr_PHY_PORTf_SET(log_to_phy_port_mapping, val);
            WRITE_LOG_TO_PHY_PORT_MAPPINGr(unit, i, log_to_phy_port_mapping);
        }
    }

}


static void
soc_misc_init(uint8 unit)
{
    int lport;

    GPORT_CONFIGr_t gport_config;
    EGR_ENABLEm_t egr_enable;
    CMIC_RATE_ADJUST_EXT_MDIOr_t cmic_rate_adjust_ext_mdio;
    CMIC_RATE_ADJUST_INT_MDIOr_t cmic_rate_adjust_int_mdio;
    EGR_VLAN_CONTROL_1r_t egr_vlan_control_1;
#if !CONFIG_WOLFHOUND2_ROMCODE
    MISCCONFIGr_t miscconfig;
    L2_AUX_HASH_CONTROLr_t l2_aux_hash_control;
    L3_AUX_HASH_CONTROLr_t l3_aux_hash_control;
    SW2_FP_DST_ACTION_CONTROLr_t sw2_fp_dst_action_control;
#endif

    /* HR3-1325 : enable GPHY1 and GPHY2 */
    soc_pipe_mem_clear(unit);
    soc_pgw_ge_tdm_config(unit);
    soc_init_port_mapping(unit);

    

    /* GMAC init */
    SOC_LPORT_ITER(lport) {
        GPORT_CONFIGr_CLR(gport_config);
        GPORT_CONFIGr_CLR_CNTf_SET(gport_config, 1);
        GPORT_CONFIGr_GPORT_ENf_SET(gport_config, 1);
        if (IS_GX_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            /* Clear counter and enable gport */
            WRITE_GPORT_CONFIGr(unit, lport, gport_config);
        }
    }

    SOC_LPORT_ITER(lport) {
        GPORT_CONFIGr_CLR(gport_config);
        GPORT_CONFIGr_GPORT_ENf_SET(gport_config, 1);
        if (IS_GX_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            /* Enable gport */
            WRITE_GPORT_CONFIGr(unit, lport, gport_config);
        }
    }

#if !CONFIG_WOLFHOUND2_ROMCODE
    /* Metering Clock [Bit 5] */
    READ_MISCCONFIGr(unit, miscconfig);
    MISCCONFIGr_METERING_CLK_ENf_SET(miscconfig, 1);
    WRITE_MISCCONFIGr(unit, miscconfig);

    /* Enable dual hash on L2 and L3 tables */
    /* HASH_SELECT[Bit3:1] = FB_HASH_CRC32_LOWER(2), INSERT_LEAST_FULL_HALF[Bit 0] = 1 */
    L2_AUX_HASH_CONTROLr_CLR(l2_aux_hash_control);
    L2_AUX_HASH_CONTROLr_HASH_SELECTf_SET(l2_aux_hash_control, 2);
    L2_AUX_HASH_CONTROLr_INSERT_LEAST_FULL_HALFf_SET(l2_aux_hash_control, 1);
    L2_AUX_HASH_CONTROLr_ENABLEf_SET(l2_aux_hash_control, 1);
    WRITE_L2_AUX_HASH_CONTROLr(unit, l2_aux_hash_control);

    L3_AUX_HASH_CONTROLr_CLR(l3_aux_hash_control);
    L3_AUX_HASH_CONTROLr_HASH_SELECTf_SET(l3_aux_hash_control, 2);
    L3_AUX_HASH_CONTROLr_INSERT_LEAST_FULL_HALFf_SET(l3_aux_hash_control, 1);
    L3_AUX_HASH_CONTROLr_ENABLEf_SET(l3_aux_hash_control, 1);
    WRITE_L3_AUX_HASH_CONTROLr(unit, l3_aux_hash_control);
#endif /* !CONFIG_WOLFHOUND2_ROMCODE */

    /* Egress Enable */
    EGR_ENABLEm_CLR(egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    SOC_LPORT_ITER(lport) {
        WRITE_EGR_ENABLEm(unit, SOC_PORT_L2P_MAPPING(lport), egr_enable);
    }
    

    /*
     * Set reference clock (based on 200MHz core clock)
     * to be 200MHz * (1/40) = 5MHz
     */
    CMIC_RATE_ADJUST_EXT_MDIOr_CLR(cmic_rate_adjust_ext_mdio);
    CMIC_RATE_ADJUST_EXT_MDIOr_DIVISORf_SET(cmic_rate_adjust_ext_mdio, 0x28);
    CMIC_RATE_ADJUST_EXT_MDIOr_DIVIDENDf_SET(cmic_rate_adjust_ext_mdio, 0x1);
    WRITE_CMIC_RATE_ADJUST_EXT_MDIOr(unit, cmic_rate_adjust_ext_mdio);

    /* Match the Internal MDC freq with above for External MDC */
    CMIC_RATE_ADJUST_INT_MDIOr_CLR(cmic_rate_adjust_int_mdio);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVISORf_SET(cmic_rate_adjust_int_mdio, 0x28);
    CMIC_RATE_ADJUST_INT_MDIOr_DIVIDENDf_SET(cmic_rate_adjust_int_mdio, 0x1);
    WRITE_CMIC_RATE_ADJUST_INT_MDIOr(unit, cmic_rate_adjust_int_mdio);


    /* The HW defaults for EGR_VLAN_CONTROL_1.VT_MISS_UNTAG == 1, which
     * causes the outer tag to be removed from packets that don't have
     * a hit in the egress vlan tranlation table. Set to 0 to disable this.
     */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_control_1);
    SOC_LPORT_ITER(lport) {
        WRITE_EGR_VLAN_CONTROL_1r(unit, lport, egr_vlan_control_1);
    }

#if !CONFIG_WOLFHOUND2_ROMCODE
    /* Enable SRC_REMOVAL_EN[Bit 0] and LAG_RES_EN[Bit2] */
    SW2_FP_DST_ACTION_CONTROLr_CLR(sw2_fp_dst_action_control);
    SW2_FP_DST_ACTION_CONTROLr_SRC_REMOVAL_ENf_SET(sw2_fp_dst_action_control, 1);
    SW2_FP_DST_ACTION_CONTROLr_LAG_RES_ENf_SET(sw2_fp_dst_action_control, 1);
    WRITE_SW2_FP_DST_ACTION_CONTROLr(unit, sw2_fp_dst_action_control);
#endif /* !CONFIG_WOLFHOUND2_ROMCODE */

    
}


static void
config_schedule_mode(uint8 unit)
{
    int lport, cos_queue;

    XQCOSARBSELr_t xqcosarbsel;
#if !CONFIG_WOLFHOUND2_ROMCODE
    WRRWEIGHT_COS0r_t wrrweight_cos0;
    WRRWEIGHT_COS1r_t wrrweight_cos1;
    WRRWEIGHT_COS2r_t wrrweight_cos2;
    WRRWEIGHT_COS3r_t wrrweight_cos3;
#endif
    MAXBUCKETCONFIGr_t maxbucketconfig;

    SOC_LPORT_ITER(lport) {
#if CONFIG_WOLFHOUND2_ROMCODE
        /* Strict Priority Mode[Bit 0-1] = 0x0, MTU_Quanta_Select[Bit 2-3]=0x3 */
        XQCOSARBSELr_CLR(xqcosarbsel);
        XQCOSARBSELr_COSARBf_SET(xqcosarbsel, 0);
        XQCOSARBSELr_MTU_QUANTA_SELECTf_SET(xqcosarbsel, 3);
        WRITE_XQCOSARBSELr(unit, lport, xqcosarbsel);
#else
        
        WRRWEIGHT_COS0r_CLR(wrrweight_cos0);
        WRRWEIGHT_COS0r_ENABLEf_SET(wrrweight_cos0, 1);
        WRRWEIGHT_COS0r_WEIGHTf_SET(wrrweight_cos0, 1);
        WRITE_WRRWEIGHT_COS0r(unit, lport, wrrweight_cos0);

        WRRWEIGHT_COS1r_CLR(wrrweight_cos1);
        WRRWEIGHT_COS1r_ENABLEf_SET(wrrweight_cos1, 1);
        WRRWEIGHT_COS1r_WEIGHTf_SET(wrrweight_cos1, 2);
        WRITE_WRRWEIGHT_COS1r(unit, lport, wrrweight_cos1);

        WRRWEIGHT_COS2r_CLR(wrrweight_cos2);
        WRRWEIGHT_COS2r_ENABLEf_SET(wrrweight_cos2, 1);
        WRRWEIGHT_COS2r_WEIGHTf_SET(wrrweight_cos2, 4);
        WRITE_WRRWEIGHT_COS2r(unit, lport, wrrweight_cos2);

        WRRWEIGHT_COS3r_CLR(wrrweight_cos3);
        WRRWEIGHT_COS3r_ENABLEf_SET(wrrweight_cos3, 1);
        WRRWEIGHT_COS3r_WEIGHTf_SET(wrrweight_cos3, 8);
        WRITE_WRRWEIGHT_COS3r(unit, lport, wrrweight_cos3);

        XQCOSARBSELr_CLR(xqcosarbsel);
        XQCOSARBSELr_COSARBf_SET(xqcosarbsel, 2);
        XQCOSARBSELr_MTU_QUANTA_SELECTf_SET(xqcosarbsel, 3);
        WRITE_XQCOSARBSELr(unit, lport, xqcosarbsel);
#endif /* CONFIG_WOLFHOUND2_ROMCODE */
        /* MAX_THD_SEL = 0 : Disable MAX shaper */
        MAXBUCKETCONFIGr_CLR(maxbucketconfig);
        for (cos_queue = 0; cos_queue < COS_QUEUE_NUM; cos_queue++) {
             WRITE_MAXBUCKETCONFIGr(unit,lport,cos_queue, maxbucketconfig);
        }
    }
}

 
static void
bcm5354x_system_init(uint8 unit)
{
    int i, j, lport;
    PORTm_t port_entry;

#if CONFIG_WOLFHOUND2_ROMCODE
    uint32 cos_map_default[8] = { 0, 0, 1, 1, 2, 2, 3, 3} ;
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
    VLAN_DEFAULT_PBMr_t vlan_default_pbm;
    SYSTEM_CONFIG_TABLEm_t system_config_table;
    SOURCE_TRUNK_MAPm_t source_trunk_map;
    UNKNOWN_UCAST_BLOCK_MASK_64r_t unknown_ucast_block_mask_64;
    ING_EGRMSKBMAP_64r_t ing_egrmskbmap_64;
    ING_PRI_CNG_MAPm_t ing_pri_cng_map;
    TRUNK32_CONFIG_TABLEm_t trunk32_config_table;
    TRUNK32_PORT_TABLEm_t trunk32_port_table;
    ING_VLAN_TAG_ACTION_PROFILEm_t ing_vlan_tag_action_profile;
#if CONFIG_WOLFHOUND2_ROMCODE
    COS_MAPm_t cos_map;
#endif
    L2_USER_ENTRYm_t l2_user_entry;
    EGR_PORT_64r_t egr_port_64;
    VLANm_t vlan;
    EGR_VLANm_t egr_vlan;
    L2_AGE_TIMERr_t l2_age_timer;
    VLAN_STGm_t vlan_stg;
    EGR_VLAN_STGm_t egr_vlan_stg;
#if !CONFIG_WOLFHOUND2_ROMCODE
    AUX_ARB_CONTROL_2r_t aux_arb_control;
    PROTOCOL_PKT_CONTROLr_t protocol_pkt_control;
    VLAN_PROFILEm_t vlan_profile;
    ING_CONFIG_64r_t ing_config_64;
#ifdef CFG_SWITCH_DOS_INCLUDED
    DOS_CONTROLr_t dos_control;
    DOS_CONTROL2r_t dos_control2;
#endif

#endif

    /* Default port_entry */
    PORTm_CLR(port_entry);
    PORTm_PORT_VIDf_SET(port_entry, 1);
    PORTm_TRUST_OUTER_DOT1Pf_SET(port_entry, 1);
    PORTm_OUTER_TPID_ENABLEf_SET(port_entry, 1);
    PORTm_TRUST_INCOMING_VIDf_SET(port_entry, 1);
    PORTm_CML_FLAGS_NEWf_SET(port_entry, 8);
    PORTm_CML_FLAGS_MOVEf_SET(port_entry, 8);

    /* Configurations to guarantee no packet modifications */
    SOC_LPORT_ITER(lport) {
        /* ING_OUTER_TPID[0] is allowed outer TPID values */
        SYSTEM_CONFIG_TABLEm_CLR(system_config_table);
        SYSTEM_CONFIG_TABLEm_OUTER_TPID_ENABLEf_SET(system_config_table, 1);
        WRITE_SYSTEM_CONFIG_TABLEm(unit,lport, system_config_table);

        SOURCE_TRUNK_MAPm_CLR(source_trunk_map);
#ifdef CFG_SWITCH_VLAN_UNAWARE_INCLUDED
        /* DISABLE_VLAN_CHECKS[Bit 63] = 1, PACKET_MODIFICATION_DISABLE[Bit 62] = 1 */
        SOURCE_TRUNK_MAPm_DISABLE_VLAN_CHECKSf_SET(source_trunk_map, 1);
        SOURCE_TRUNK_MAPm_PACKET_MODIFICATION_DISABLEf_SET(source_trunk_map, 1);
#endif /* CFG_SWITCH_VLAN_UNAWARE_INCLUDED */
        WRITE_SOURCE_TRUNK_MAPm(unit, lport, source_trunk_map);
            
        
        WRITE_PORTm(unit, lport, port_entry);

        /* Clear Unknown Unicast Block Mask. */                           
        UNKNOWN_UCAST_BLOCK_MASK_64r_CLR(unknown_ucast_block_mask_64);
        WRITE_UNKNOWN_UCAST_BLOCK_MASK_64r(unit, lport, unknown_ucast_block_mask_64);

        /* Clear ingress block mask. */
        ING_EGRMSKBMAP_64r_CLR(ing_egrmskbmap_64);
        WRITE_ING_EGRMSKBMAP_64r(unit, lport, ing_egrmskbmap_64);
    }

    for (lport = 0; lport <= BCM5354X_LPORT_MAX; lport++) {
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

#if CONFIG_WOLFHOUND2_ROMCODE
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

#endif /* CONFIG_WOLFHOUND2_ROMCODE */

    enable_jumbo_frame(unit);
    config_schedule_mode(unit);

    /*
     * VLAN_DEFAULT_PBM is used as defalut VLAN members for those ports
     * that disable VLAN checks.
     */
    VLAN_DEFAULT_PBMr_CLR(vlan_default_pbm);
    VLAN_DEFAULT_PBMr_PORT_BITMAPf_SET(vlan_default_pbm, SOC_PBMP(BCM5354X_ALL_PORTS_MASK));
    WRITE_VLAN_DEFAULT_PBMr(0, vlan_default_pbm);

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
    //L2_USER_ENTRYm_BPDUf_SET(l2_user_entry, 1);
    L2_USER_ENTRYm_Tf_SET(l2_user_entry, 1);
    L2_USER_ENTRYm_KEY_TYPE_MASKf_SET(l2_user_entry, 1);
    L2_USER_ENTRYm_MAC_ADDRf_SET(l2_user_entry, drop_mac_addr1);
    L2_USER_ENTRYm_MAC_ADDR_MASKf_SET(l2_user_entry, drop_mac_addr_mask);
    WRITE_L2_USER_ENTRYm(unit, 0, l2_user_entry);

    L2_USER_ENTRYm_MAC_ADDRf_SET(l2_user_entry, drop_mac_addr2);
    WRITE_L2_USER_ENTRYm(unit, 1, l2_user_entry);

    L2_USER_ENTRYm_MAC_ADDRf_SET(l2_user_entry, drop_mac_addr3);
    WRITE_L2_USER_ENTRYm(unit, 2, l2_user_entry);


#if !CONFIG_WOLFHOUND2_ROMCODE

#ifdef CFG_SWITCH_DOS_INCLUDED
    /*
     * Enable following Denial of Service protections:
     * DROP_IF_SIP_EQUALS_DIP
     * MIN_TCPHDR_SIZE = 0x14 (Default)
     * IP_FIRST_FRAG_CHECK_ENABLE
     * TCP_HDR_OFFSET_EQ1_ENABLE
     * TCP_HDR_PARTIAL_ENABLE
     * ICMP_V4_PING_SIZE_ENABLE, TCP_HDR_PARTIAL_ENABLE, 
     * TCP_HDR_OFFSET_EQ1_ENABLE, ICMP_FRAG_PKTS_ENABLE 
     */
    DOS_CONTROLr_CLR(dos_control);
    DOS_CONTROLr_DROP_IF_SIP_EQUALS_DIPf_SET(dos_control, 1);
    DOS_CONTROLr_MIN_TCPHDR_SIZEf_SET(dos_control, 0x14);
    DOS_CONTROLr_IP_FIRST_FRAG_CHECK_ENABLEf_SET(dos_control, 1);
    DOS_CONTROLr_BIG_ICMP_PKT_SIZEf_SET(dos_control, 0x208);
    DOS_CONTROLr_IPV4_FIRST_FRAG_CHECK_ENABLEf_SET(dos_control, 1);
    WRITE_DOS_CONTROLr(unit, dos_control);

    DOS_CONTROL2r_CLR(dos_control2);
    DOS_CONTROL2r_TCP_HDR_OFFSET_EQ1_ENABLEf_SET(dos_control2, 1);
    DOS_CONTROL2r_TCP_HDR_PARTIAL_ENABLEf_SET(dos_control2, 1);
    DOS_CONTROL2r_ICMP_V4_PING_SIZE_ENABLEf_SET(dos_control2, 1);
    DOS_CONTROL2r_TCP_HDR_PARTIAL_ENABLEf_SET(dos_control2, 1);
    DOS_CONTROL2r_ICMP_FRAG_PKTS_ENABLEf_SET(dos_control2, 1);
    WRITE_DOS_CONTROL2r(unit, dos_control2);
#endif

    /* enable FP_REFRESH_ENABLE [Bit 26] */
    READ_AUX_ARB_CONTROL_2r(unit, aux_arb_control);
    AUX_ARB_CONTROL_2r_FP_REFRESH_ENABLEf_SET(aux_arb_control, 1);
    WRITE_AUX_ARB_CONTROL_2r(unit, aux_arb_control);

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

#endif /* !CONFIG_WOLFHOUND2_ROMCODE */

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
    PBMP_ASSIGN(lpbmp, BCM5354X_ALL_PORTS_MASK);
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
    PBMP_ASSIGN(lpbmp, BCM5354X_ALL_PORTS_MASK);
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
    VLAN_STGm_SET(vlan_stg,2,0);
    WRITE_VLAN_STGm(unit, 1, vlan_stg);

    EGR_VLAN_STGm_CLR(egr_vlan_stg);
    EGR_VLAN_STGm_SET(egr_vlan_stg,0,0xfffffff0);
    EGR_VLAN_STGm_SET(egr_vlan_stg,1,0xffffffff);
    EGR_VLAN_STGm_SET(egr_vlan_stg,2,0);
    WRITE_EGR_VLAN_STGm(unit, 1, egr_vlan_stg);


    /* Make PORT_VID[Bit 35:24] = 0 for CPU port */
    READ_PORTm(unit, 0, port_entry);
    PORTm_PORT_VIDf_SET(port_entry, 0);
    WRITE_PORTm(unit, 0, port_entry);
#if !CONFIG_WOLFHOUND2_ROMCODE
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
#endif /* !CONFIG_WOLFHOUND2_ROMCODE */
#endif /* CFG_RXTX_SUPPORT_ENABLED */

    /* Enable aging timer */
    READ_L2_AGE_TIMERr(unit, l2_age_timer);
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 1);
    WRITE_L2_AGE_TIMERr(unit, l2_age_timer);

}

/* Function:
 *   bcm5354x_sw_init
 * Description:
 *   Perform chip specific initialization.
 *   This will be called by board_init()
 * Parameters:
 *   None
 * Returns:
 *   None
 */
sys_error_t
bcm5354x_sw_init(void)
{
    int   rv = 0;
    uint8 unit = 0;
    pbmp_t okay_pbmp;
    uint8 lport;    
    CMIC_CPS_RESETr_t cmic_cps_reset;
#if UM_DEBUG
    soc_info_t *si = &SOC_INFO(0);
#endif

    /* Get chip revision */
    bcm5354x_chip_revision(unit, &wh2_sw_info.devid, &wh2_sw_info.revid);
#if CFG_CONSOLE_ENABLED
    //sal_printf("\ndevid = 0x%x, revid = 0x%x\n", wh2_sw_info.devid, wh2_sw_info.revid);
    sal_printf("devid = 0x%x\n", wh2_sw_info.devid);
    sal_printf("revid = 0x%x\n", wh2_sw_info.revid);
#endif /* CFG_CONSOLE_ENABLED */

#if UM_DEBUG
  sal_printf("si = %p si->port_p2l_mapping=%p\n", si, si->port_p2l_mapping);
  sal_printf("SOC_MAX_NUM_PORTS = %x SOC_MAX_NUM_BLKS=%x sizeof(pbmp_t)=%x\n", SOC_MAX_NUM_PORTS, SOC_MAX_NUM_BLKS, sizeof(pbmp_t));
#endif

   /* CPS reset complete SWITCH and CMICd */
   CMIC_CPS_RESETr_CLR(cmic_cps_reset);
   CMIC_CPS_RESETr_CPS_RESETf_SET(cmic_cps_reset, 1);
   WRITE_CMIC_CPS_RESETr(unit, cmic_cps_reset);

#if CONFIG_EMULATION
    sal_usleep(250000);
#else
    sal_usleep(1000);
#endif /* CONFIG_EMULATION */
     
    soc_reset(unit);

    soc_wh2_tsc_reset(unit);
 
    soc_misc_init(unit);

    soc_mmu_init(unit);
    
    bcm5354x_pcm_software_init(unit);

    pcm_port_probe_init(unit, BCM5354X_ALL_PORTS_MASK, &okay_pbmp);

    bcm5354x_system_init(unit);
    
    SOC_LPORT_ITER(lport) {
		pcm_port_update(unit, lport, 0);
		wh2_sw_info.link[lport] = PORT_LINK_DOWN;
#if defined(CFG_SWITCH_EEE_INCLUDED)
		wh2_sw_info.need_process_for_eee_1s[lport] = FALSE;
#endif /*  CFG_SWITCH_EEE_INCLUDED */
	}

    
    bcm5354x_load_led_program(unit);

#if CONFIG_EMULATION
    SOC_LPORT_ITER(lport) {
        link_qt[lport] = PORT_LINK_UP;
    }
    bcm5354x_linkscan_task(NULL);
    sal_printf("all ports up!\n");

    /* Register background process for handling link status */
    timer_add(bcm5354x_linkscan_task, NULL, LINKSCAN_INTERVAL);
#else
    bcm5354x_linkscan_init(LINKSCAN_INTERVAL);
#endif /* CONFIG_EMULATION */
    
    if (rv) {
        return SYS_ERR;
    } else {
        return SYS_OK;
    }

}


soc_switch_t soc_switch_bcm5354x =
{
    bcm5354x_chip_type,
    NULL,
    bcm5354x_port_count,
    NULL,
    NULL,
#if CFG_RXTX_SUPPORT_ENABLED
    bcm5354x_rx_set_handler,
    bcm5354x_rx_fill_buffer,
    bcm5354x_tx,
#endif /* CFG_RXTX_SUPPORT_ENABLED */

    bcm5354x_link_status,
    bcm5354x_chip_revision,
    bcm5354x_reg_get,
    bcm5354x_reg_set,
    bcm5354x_mem_get,
    bcm5354x_mem_set,
#ifdef CFG_SWITCH_VLAN_INCLUDED
    bcm5354x_pvlan_egress_set,
    bcm5354x_pvlan_egress_get,
    bcm5354x_qvlan_port_set,
    bcm5354x_qvlan_port_get,
    bcm5354x_vlan_create,
    bcm5354x_vlan_destroy,
    bcm5354x_vlan_type_set,
    bcm5354x_vlan_reset,
#endif  /* CFG_SWITCH_VLAN_INCLUDED */
    bcm5354x_phy_reg_get,
    bcm5354x_phy_reg_set,
};

