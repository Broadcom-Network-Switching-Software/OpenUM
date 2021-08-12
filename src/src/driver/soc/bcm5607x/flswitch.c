/*
 * $Id: flswitch.c,v 1.28 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"
#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>
#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>
#include "tdm/tdm.h"

#define FL_TDM_DEBUG   0

#if FL_TDM_DEBUG
#define TDM_DBG(x)  do { sal_printf x; } while(0);
#else
#define TDM_DBG(x)
#endif

#define LINKSCAN_INTERVAL        (100000UL)   /* 100 ms */

#define CFG_FLOW_CONTROL_ENABLED_COSQ 1   /* Based on priority <-> cosq mapping */

/* MMU related */
#define FL_MMU_CBP_FULL_SIZE 0x3fff

#define CFG_MMU_DEBUG           (0)
#define MMU_64Q_PPORT_BASE      (58)

/* TDM related */
#define NUM_EXT_PORTS                       80

/* Allocation length of VBS line rate calendar */
#define FL_LR_VBS_LEN 512

typedef struct tdm_config_s {
    int     speed[NUM_EXT_PORTS];
    int     port_state[NUM_EXT_PORTS];

    int idb_tdm_tbl_0[FL_LR_VBS_LEN];
} tdm_config_t;

#define CMIC_LPORT                     0
#define CMIC_PPORT                     0

static sys_error_t
soc_fl_pgw_pll_init(uint8 unit)
{
    int to_usec = 50000;
    int pgw_pll_frefeff_info = 25;
    int pgw_pll_ndiv_int = 228;
    int pgw_pll_pdiv = 2;
    int pgw_pll_ch0_mdiv = 5;
    int pgw_pll_ch5_mdiv = 30;
    TOP_PGW_PLL_OPR_CTRL_2r_t pll_opr_ctrl_2;
    TOP_PGW_PLL_CTRL_0r_t pll_ctrl_0;
    TOP_PGW_PLL_CTRL_1r_t pll_ctrl_1;
    TOP_PGW_PLL_CTRL_2r_t pll_ctrl_2;
    TOP_PGW_PLL_CTRL_3r_t pll_ctrl_3;
    TOP_SOFT_RESET_REG_2r_t soft_reset_reg_2;
    TOP_PGW_PLL_STATUSr_t pll_status;

    SOC_IF_ERROR_RETURN(
        READ_TOP_PGW_PLL_OPR_CTRL_2r(unit, pll_opr_ctrl_2));
    TOP_PGW_PLL_OPR_CTRL_2r_FREFEFF_INFOf_SET(pll_opr_ctrl_2,
                                              pgw_pll_frefeff_info);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_PGW_PLL_OPR_CTRL_2r(unit, pll_opr_ctrl_2));

    SOC_IF_ERROR_RETURN(
        READ_TOP_PGW_PLL_CTRL_3r(unit, pll_ctrl_3));
    TOP_PGW_PLL_CTRL_3r_NDIV_INTf_SET(pll_ctrl_3, pgw_pll_ndiv_int);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_PGW_PLL_CTRL_3r(unit, pll_ctrl_3));

    SOC_IF_ERROR_RETURN(
        READ_TOP_PGW_PLL_CTRL_1r(unit, pll_ctrl_1));
    TOP_PGW_PLL_CTRL_1r_PDIVf_SET(pll_ctrl_1, pgw_pll_pdiv);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_PGW_PLL_CTRL_1r(unit, pll_ctrl_1));

    SOC_IF_ERROR_RETURN(
        READ_TOP_PGW_PLL_CTRL_0r(unit, pll_ctrl_0));
    TOP_PGW_PLL_CTRL_0r_CH0_MDIVf_SET(pll_ctrl_0, pgw_pll_ch0_mdiv);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_PGW_PLL_CTRL_0r(unit, pll_ctrl_0));

    SOC_IF_ERROR_RETURN(
        READ_TOP_PGW_PLL_CTRL_2r(unit, pll_ctrl_2));
    TOP_PGW_PLL_CTRL_2r_CH5_MDIVf_SET(pll_ctrl_2, pgw_pll_ch5_mdiv);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_PGW_PLL_CTRL_2r(unit, pll_ctrl_2));

    /* De-assert the PGW PLL reset */
    SOC_IF_ERROR_RETURN(
        READ_TOP_SOFT_RESET_REG_2r(unit, soft_reset_reg_2));
    TOP_SOFT_RESET_REG_2r_TOP_PGW_PLL_RST_Lf_SET(soft_reset_reg_2, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_SOFT_RESET_REG_2r(unit, soft_reset_reg_2));
    TOP_SOFT_RESET_REG_2r_TOP_PGW_PLL_POST_RST_Lf_SET(soft_reset_reg_2, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_SOFT_RESET_REG_2r(unit, soft_reset_reg_2));

    /* Wait for TOP_PGW_PLL_LOCK done. */
    do {
        int loop_usec = 10;

        SOC_IF_ERROR_RETURN(
            READ_TOP_PGW_PLL_STATUSr(unit, pll_status));
        if (TOP_PGW_PLL_STATUSr_TOP_PGW_PLL_LOCKf_GET(pll_status)) {
            break;
        }
        if (to_usec <= 0) {
            sal_printf("TOP_PGW_PLL_LOCK timeout\n");
            break;
        }
        sal_usleep(loop_usec);
        to_usec -= loop_usec;
    } while (1);

    return SYS_OK;
}

static sys_error_t
soc_fl_macsec_bypass(uint8 unit)
{
    TOP_MACSEC_CTRLr_t macsec_ctrl;

    SOC_IF_ERROR_RETURN(
        READ_TOP_MACSEC_CTRLr(unit, macsec_ctrl));
    TOP_MACSEC_CTRLr_MACSEC_GE_XL_SELf_SET(macsec_ctrl, 0);
    TOP_MACSEC_CTRLr_MACSEC_CL_ENf_SET(macsec_ctrl, 0);
    TOP_MACSEC_CTRLr_MACSEC_PM4X10Q_BYPf_SET(macsec_ctrl, 0x7);
    TOP_MACSEC_CTRLr_MACSEC_PM4X25_BYPf_SET(macsec_ctrl, 0xf);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_MACSEC_CTRLr(unit, macsec_ctrl));

    return SYS_OK;
}

static sys_error_t
soc_reset(uint8 unit)
{
    uint32 to_usec;
    int lport;
    TOP_SOFT_RESET_REGr_t top_soft_reset;
    TOP_SOFT_RESET_REG_2r_t top_soft_reset2;
    CMIC_TOP_SBUS_TIMEOUTr_t cmic_sbus_timeout;
    TOP_CORE_PLL_CTRL3r_t top_core_pll_ctrl3;
    TOP_CORE_PLL_CTRL4r_t top_core_pll_ctrl4;
    TOP_CORE_PLL_CTRL5r_t top_core_pll_ctrl5;
    TOP_CORE_PLL_CTRL6r_t top_core_pll_ctrl6;
    TOP_CORE_PLL_DEBUG_CTRL_2r_t top_core_pll_debug_ctrl_2;
    uint32 core_pll_ndiv;
    uint32 core_pll_ch0;
    uint32 core_pll_ch1;
    uint32 core_pll_ch2;
    uint32 core_pll_ch3;
    uint32 core_pll_ch4;
    TOP_MISC_CONTROL_1r_t top_misc_control_1;
    TOP_MISC_CONTROL_3r_t top_misc_control_3;
    TOP_BS_PLL0_CTRL_0r_t top_bs_pll0_ctrl_0;
    TOP_BS_PLL0_CTRL_1r_t top_bs_pll0_ctrl_1;
    TOP_BS_PLL1_CTRL_0r_t top_bs_pll1_ctrl_0;
    TOP_BS_PLL1_CTRL_1r_t top_bs_pll1_ctrl_1;
    TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_0r_t bs0_lcpll_fbdiv_ctrl_0;
    TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r_t bs0_lcpll_fbdiv_ctrl_1;
    TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_0r_t bs1_lcpll_fbdiv_ctrl_0;
    TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r_t bs1_lcpll_fbdiv_ctrl_1;
    IPROC_NS_TIMESYNC_TS0_INIT_ACCUMULATOR_2r_t ts0_accumaltor_2;
    IPROC_NS_TIMESYNC_TS0_INIT_ACCUMULATOR_1r_t ts0_accumaltor_1;
    IPROC_NS_TIMESYNC_TS0_INIT_ACCUMULATOR_0r_t ts0_accumaltor_0;
    IPROC_NS_TIMESYNC_TS1_INIT_ACCUMULATOR_2r_t ts1_accumaltor_2;
    IPROC_NS_TIMESYNC_TS1_INIT_ACCUMULATOR_1r_t ts1_accumaltor_1;
    IPROC_NS_TIMESYNC_TS1_INIT_ACCUMULATOR_0r_t ts1_accumaltor_0;
    EGR_1588_TIMER_VALUEr_t egr_1588_timer;
    GPORT_TS_TIMER_47_32_REGr_t ts_timer_47_32_reg;
#if CONFIG_EMULATION
    to_usec = 250000;
#else
    to_usec = 10000;
#endif /* CONFIG_EMULATION */

    bcm5607x_write32(0, CMIC_TOP_SBUS_RING_MAP_0_7, 0x01110000);
    bcm5607x_write32(0, CMIC_TOP_SBUS_RING_MAP_8_15, 0x00430070);
    bcm5607x_write32(0, CMIC_TOP_SBUS_RING_MAP_16_23, 0x22226554);
    bcm5607x_write32(0, CMIC_TOP_SBUS_RING_MAP_24_31, 0x00000000);
    bcm5607x_write32(0, CMIC_TOP_SBUS_RING_MAP_32_39, 0x11111111);
    bcm5607x_write32(0, CMIC_TOP_SBUS_RING_MAP_40_47, 0x00001111);

    CMIC_TOP_SBUS_TIMEOUTr_CLR(cmic_sbus_timeout);
    CMIC_TOP_SBUS_TIMEOUTr_TIMEOUT_VALf_SET(cmic_sbus_timeout, 0x7d0);
    WRITE_CMIC_TOP_SBUS_TIMEOUTr(unit,cmic_sbus_timeout);

    /* Skip polling TOP_XG_PLL0_STATUS since we have to keep going anyway */
    sal_usleep(to_usec);

    /* Clear all fl_sw_info structure */
    sal_memset(&fl_sw_info, 0, sizeof(fl_sw_info));

    /* Get chip revision */
    bcm5607x_chip_revision(unit, &fl_sw_info.devid, &fl_sw_info.revid);

#if CFG_CONSOLE_ENABLED
    sal_printf("\ndevid = 0x%x, revid = 0x%x\n", fl_sw_info.devid, fl_sw_info.revid);
#endif /* CFG_CONSOLE_ENABLED */

    SOC_IF_ERROR_RETURN(soc_port_config_init(unit));

    SOC_IF_ERROR_RETURN(
        READ_TOP_CORE_PLL_CTRL4r(unit, top_core_pll_ctrl4));
    SOC_IF_ERROR_RETURN(
        READ_TOP_CORE_PLL_CTRL3r(unit, top_core_pll_ctrl3));
    SOC_IF_ERROR_RETURN(
        READ_TOP_CORE_PLL_CTRL5r(unit, top_core_pll_ctrl5));
    SOC_IF_ERROR_RETURN(
        READ_TOP_CORE_PLL_CTRL6r(unit, top_core_pll_ctrl6));

    switch (sku_port_config->freq)
    {
        case _FL_SYSTEM_FREQ_219:
            core_pll_ndiv = 70;
            core_pll_ch0 = 8;
            core_pll_ch1 = 14;
            core_pll_ch2 = 7;
            core_pll_ch3 = 4;
            core_pll_ch4 = 4;
            break;
        case _FL_SYSTEM_FREQ_344:
            core_pll_ndiv = 110;
            core_pll_ch0 =8;
            core_pll_ch1 = 22;
            core_pll_ch2 = 11;
            core_pll_ch3 = 6;
            core_pll_ch4 = 6;
            break;
        case _FL_SYSTEM_FREQ_469:
            core_pll_ndiv = 150;
            core_pll_ch0 = 8;
            core_pll_ch1 = 30;
            core_pll_ch2 = 15;
            core_pll_ch3 = 8;
            core_pll_ch4 = 8;
            break;
        case _FL_SYSTEM_FREQ_700:
        default:
            core_pll_ndiv = 140;
            core_pll_ch0 = 5;
            core_pll_ch1 = 28;
            core_pll_ch2 = 14;
            core_pll_ch3 = 7;
            core_pll_ch4 = 7;
            break;
    }

    TOP_CORE_PLL_CTRL3r_MSTR_NDIV_INTf_SET(top_core_pll_ctrl3, core_pll_ndiv);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_CORE_PLL_CTRL3r(unit, top_core_pll_ctrl3));

    TOP_CORE_PLL_CTRL4r_MSTR_CH0_MDIVf_SET(top_core_pll_ctrl4, core_pll_ch0);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_CORE_PLL_CTRL4r(unit, top_core_pll_ctrl4));

    TOP_CORE_PLL_CTRL4r_MSTR_CH1_MDIVf_SET(top_core_pll_ctrl4, core_pll_ch1);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_CORE_PLL_CTRL4r(unit, top_core_pll_ctrl4));

    TOP_CORE_PLL_CTRL5r_MSTR_CH2_MDIVf_SET(top_core_pll_ctrl5, core_pll_ch2);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_CORE_PLL_CTRL5r(unit, top_core_pll_ctrl5));

    TOP_CORE_PLL_CTRL6r_MSTR_CH3_MDIVf_SET(top_core_pll_ctrl6, core_pll_ch3);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_CORE_PLL_CTRL6r(unit, top_core_pll_ctrl6));

    TOP_CORE_PLL_DEBUG_CTRL_2r_RESERVEDf_SET(top_core_pll_debug_ctrl_2, core_pll_ch4);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_CORE_PLL_DEBUG_CTRL_2r(unit, top_core_pll_debug_ctrl_2));

    SOC_IF_ERROR_RETURN(
        READ_TOP_MISC_CONTROL_1r(unit, top_misc_control_1));
    TOP_MISC_CONTROL_1r_CMIC_TO_CORE_PLL_LOADf_SET(top_misc_control_1, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_control_1));

    /* configure both BSPLL0 and BSPLL1 */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset));
    TOP_SOFT_RESET_REGr_TOP_LCPLL_SOFT_RESETf_SET(top_soft_reset, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset));

    /* BSPLL0 */
    SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_1r(unit, top_bs_pll0_ctrl_1));
    TOP_BS_PLL0_CTRL_1r_PDIVf_SET(top_bs_pll0_ctrl_1, 2);
    SOC_IF_ERROR_RETURN(WRITE_TOP_BS_PLL0_CTRL_1r(unit, top_bs_pll0_ctrl_1));

    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_0r(unit, bs0_lcpll_fbdiv_ctrl_0));
    TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_0r_BROAD_SYNC0_LCPLL_FBDIV_0f_SET(bs0_lcpll_fbdiv_ctrl_0, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_0r(unit, bs0_lcpll_fbdiv_ctrl_0));

    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r(unit, bs0_lcpll_fbdiv_ctrl_1));
    TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r_BROAD_SYNC0_LCPLL_FBDIV_1f_SET(bs0_lcpll_fbdiv_ctrl_1, 0x1E00);
    SOC_IF_ERROR_RETURN(WRITE_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r(unit, bs0_lcpll_fbdiv_ctrl_1));

    SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_0r(unit, top_bs_pll0_ctrl_0));
    TOP_BS_PLL0_CTRL_0r_CH0_MDIVf_SET(top_bs_pll0_ctrl_0, 0x96);
    SOC_IF_ERROR_RETURN(WRITE_TOP_BS_PLL0_CTRL_0r(unit, top_bs_pll0_ctrl_0));

    SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_3r(unit, top_misc_control_3));
    TOP_MISC_CONTROL_3r_BROAD_SYNC0_LCPLL_HO_BYP_ENABLEf_SET(top_misc_control_3, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_MISC_CONTROL_3r(unit, top_misc_control_3));
    /* BSPLL1 */
    SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL1_CTRL_1r(unit, top_bs_pll1_ctrl_1));
    TOP_BS_PLL1_CTRL_1r_PDIVf_SET(top_bs_pll1_ctrl_1, 2);
    SOC_IF_ERROR_RETURN(WRITE_TOP_BS_PLL1_CTRL_1r(unit, top_bs_pll1_ctrl_1));

    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_0r(unit, bs1_lcpll_fbdiv_ctrl_0));
    TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_0r_BROAD_SYNC1_LCPLL_FBDIV_0f_SET(bs1_lcpll_fbdiv_ctrl_0, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_0r(unit, bs1_lcpll_fbdiv_ctrl_0));

    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r(unit, bs1_lcpll_fbdiv_ctrl_1));
    TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r_BROAD_SYNC1_LCPLL_FBDIV_1f_SET(bs1_lcpll_fbdiv_ctrl_1, 0x1E00);
    SOC_IF_ERROR_RETURN(WRITE_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r(unit, bs1_lcpll_fbdiv_ctrl_1));

    SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL1_CTRL_0r(unit, top_bs_pll1_ctrl_0));
    TOP_BS_PLL1_CTRL_0r_CH0_MDIVf_SET(top_bs_pll1_ctrl_0, 0x96);
    SOC_IF_ERROR_RETURN(WRITE_TOP_BS_PLL1_CTRL_0r(unit, top_bs_pll1_ctrl_0));

    SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_3r(unit, top_misc_control_3));
    TOP_MISC_CONTROL_3r_BROAD_SYNC1_LCPLL_HO_BYP_ENABLEf_SET(top_misc_control_3, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_MISC_CONTROL_3r(unit, top_misc_control_3));

    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset));
    TOP_SOFT_RESET_REGr_TOP_LCPLL_SOFT_RESETf_SET(top_soft_reset, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset));

    /* BSPLL0 */
    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r(unit, bs0_lcpll_fbdiv_ctrl_1));
    TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r_BROAD_SYNC0_LCPLL_FBDIV_1f_SET(bs0_lcpll_fbdiv_ctrl_1, 0x1E00);
    SOC_IF_ERROR_RETURN(WRITE_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r(unit, bs0_lcpll_fbdiv_ctrl_1));

    SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_3r(unit, top_misc_control_3));
    TOP_MISC_CONTROL_3r_BROAD_SYNC0_LCPLL_HO_BYP_ENABLEf_SET(top_misc_control_3, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_MISC_CONTROL_3r(unit, top_misc_control_3));

    /* BSPLL1 */
    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r(unit, bs1_lcpll_fbdiv_ctrl_1));
    TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r_BROAD_SYNC1_LCPLL_FBDIV_1f_SET(bs1_lcpll_fbdiv_ctrl_1, 0x1E00);
    SOC_IF_ERROR_RETURN(WRITE_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r(unit, bs1_lcpll_fbdiv_ctrl_1));

    SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_3r(unit, top_misc_control_3));
    TOP_MISC_CONTROL_3r_BROAD_SYNC1_LCPLL_HO_BYP_ENABLEf_SET(top_misc_control_3, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_MISC_CONTROL_3r(unit, top_misc_control_3));

    SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_1r(unit, top_misc_control_1));
    TOP_MISC_CONTROL_1r_CMIC_TO_BS_PLL0_SW_OVWRf_SET(top_misc_control_1, 0);
    TOP_MISC_CONTROL_1r_CMIC_TO_BS_PLL1_SW_OVWRf_SET(top_misc_control_1, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_MISC_CONTROL_1r(unit, top_misc_control_1));

    /* Initialize PGW PLL */
    SOC_IF_ERROR_RETURN(
        soc_fl_pgw_pll_init(unit));

    /* Bypass MACSEC */
    SOC_IF_ERROR_RETURN(
        soc_fl_macsec_bypass(unit));

    /*
     * Bring port blocks out of reset
     */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset));
    TOP_SOFT_RESET_REGr_TOP_GEP0_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_GEP1_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_GEP2_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP0_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP1_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_XLP2_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP0_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP1_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP2_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_CLP3_RST_Lf_SET(top_soft_reset, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset));
    sal_usleep(to_usec);

    /*
     * Bring port blocks out of reset
     */
    READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset);
    TOP_SOFT_RESET_REGr_TOP_TSCQ0_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_TSCQ1_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_TSCQ2_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_TSCF0_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_TSCF1_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_TSCF2_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_TSCF3_RST_Lf_SET(top_soft_reset, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset));
    sal_usleep(to_usec);

    /* Bring network sync out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset2));
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_RST_Lf_SET(top_soft_reset2, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset2));
    TOP_SOFT_RESET_REG_2r_TOP_TS_PLL_POST_RST_Lf_SET(top_soft_reset2, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, top_soft_reset2));

    /* Bring IP, EP, and MMU blocks out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset));
    TOP_SOFT_RESET_REGr_TOP_EP_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_IP_RST_Lf_SET(top_soft_reset, 1);
    TOP_SOFT_RESET_REGr_TOP_MMU_RST_Lf_SET(top_soft_reset, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset));
    sal_usleep(to_usec);

    /* Initialize cmic, ep, port local timers to same value. */
    IPROC_NS_TIMESYNC_TS0_INIT_ACCUMULATOR_2r_CLR(ts0_accumaltor_2);
    SOC_IF_ERROR_RETURN(
        WRITE_IPROC_NS_TIMESYNC_TS0_INIT_ACCUMULATOR_2r(unit, ts0_accumaltor_2));

    IPROC_NS_TIMESYNC_TS0_INIT_ACCUMULATOR_1r_CLR(ts0_accumaltor_1);
    SOC_IF_ERROR_RETURN(
        WRITE_IPROC_NS_TIMESYNC_TS0_INIT_ACCUMULATOR_1r(unit, ts0_accumaltor_1));

    IPROC_NS_TIMESYNC_TS0_INIT_ACCUMULATOR_0r_CLR(ts0_accumaltor_0);
    SOC_IF_ERROR_RETURN(
        WRITE_IPROC_NS_TIMESYNC_TS0_INIT_ACCUMULATOR_0r(unit, ts0_accumaltor_0));

    IPROC_NS_TIMESYNC_TS1_INIT_ACCUMULATOR_2r_CLR(ts1_accumaltor_2);
    SOC_IF_ERROR_RETURN(
        WRITE_IPROC_NS_TIMESYNC_TS1_INIT_ACCUMULATOR_2r(unit, ts1_accumaltor_2));

    IPROC_NS_TIMESYNC_TS1_INIT_ACCUMULATOR_1r_CLR(ts1_accumaltor_1);
    SOC_IF_ERROR_RETURN(
        WRITE_IPROC_NS_TIMESYNC_TS1_INIT_ACCUMULATOR_1r(unit, ts1_accumaltor_1));

    IPROC_NS_TIMESYNC_TS1_INIT_ACCUMULATOR_0r_CLR(ts1_accumaltor_0);
    SOC_IF_ERROR_RETURN(
        WRITE_IPROC_NS_TIMESYNC_TS1_INIT_ACCUMULATOR_0r(unit, ts1_accumaltor_0));

    /* Reset cmic, ep, port local timers with same value. */
    SOC_LPORT_ITER(lport) {
        if (IS_GX_PORT(lport)) {
            GPORT_TS_TIMER_47_32_REGr_CLR(ts_timer_47_32_reg);
            SOC_IF_ERROR_RETURN(
                WRITE_GPORT_TS_TIMER_47_32_REGr(unit, lport, ts_timer_47_32_reg));
        }
    }

    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset));
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_soft_reset, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset));

    /* Bring network sync out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset));
    TOP_SOFT_RESET_REGr_TOP_TS_RST_Lf_SET(top_soft_reset, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset));

    sal_usleep(to_usec);

    /* Reset egress 1588 timer value. */
    EGR_1588_TIMER_VALUEr_CLR(egr_1588_timer);
    SOC_IF_ERROR_RETURN(WRITE_EGR_1588_TIMER_VALUEr(unit, egr_1588_timer));

    return SYS_OK;
}

soc_chip_type_t
bcm5607x_chip_type(void)
{
    return SOC_TYPE_SWITCH_XGS;
}

uint8
bcm5607x_port_count(uint8 unit)
{
    if (unit > 0) {
        return 0;
    }
    return SOC_PORT_COUNT(unit);
}

sys_error_t
bcm5607x_chip_revision(uint8 unit, uint16 *dev, uint16 *rev)
{
    TOP_DEV_REV_IDr_t top_dev_rev_id;

    if (unit > 0) {
        return SYS_ERR_PARAMETER;
    }

    SOC_IF_ERROR_RETURN(READ_TOP_DEV_REV_IDr(unit,top_dev_rev_id));
    *dev = TOP_DEV_REV_IDr_DEV_IDf_GET(top_dev_rev_id);
    *rev = TOP_DEV_REV_IDr_REV_IDf_GET(top_dev_rev_id);

    return SYS_OK;
}

static sys_error_t
soc_pipe_mem_clear(uint8 unit)
{
    int i;

#if CONFIG_EMULATION
    int         pipe_init_count = 100000;
#else
    int         pipe_init_count = 500;
#endif /* CONFIG_EMULATION */

    ING_HW_RESET_CONTROL_1r_t ing_hw_reset_control_1;
    ING_HW_RESET_CONTROL_2r_t ing_hw_reset_control_2;
    EGR_HW_RESET_CONTROL_0r_t egr_hw_reset_control_0;
    EGR_HW_RESET_CONTROL_1r_t egr_hw_reset_control_1;

    /*
     * Reset the IPIPE and EPIPE block
     */
    ING_HW_RESET_CONTROL_1r_CLR(ing_hw_reset_control_1);
    SOC_IF_ERROR_RETURN(
        WRITE_ING_HW_RESET_CONTROL_1r(unit, ing_hw_reset_control_1));

    /* Set count to # entries in largest IPIPE table */
    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_reset_control_2);
    ING_HW_RESET_CONTROL_2r_RESET_ALLf_SET(ing_hw_reset_control_2, 1);
    ING_HW_RESET_CONTROL_2r_VALIDf_SET(ing_hw_reset_control_2, 1);
    ING_HW_RESET_CONTROL_2r_COUNTf_SET(ing_hw_reset_control_2, 0x8000);
    SOC_IF_ERROR_RETURN(
        WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2));

    EGR_HW_RESET_CONTROL_0r_CLR(egr_hw_reset_control_0);
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_HW_RESET_CONTROL_0r(unit, egr_hw_reset_control_0));

    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_control_1);
    EGR_HW_RESET_CONTROL_1r_RESET_ALLf_SET(egr_hw_reset_control_1, 1);
    EGR_HW_RESET_CONTROL_1r_VALIDf_SET(egr_hw_reset_control_1, 1);
    /* Set count to # entries in largest EPIPE table */
    EGR_HW_RESET_CONTROL_1r_COUNTf_SET(egr_hw_reset_control_1, 0x2000);
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1));

    /* Wait for IPIPE memory initialization done. */
    i = 0;
    do {
        SOC_IF_ERROR_RETURN(
            READ_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2));
        if (ING_HW_RESET_CONTROL_2r_DONEf_GET(ing_hw_reset_control_2)) {
            break;
        }
        i++;
        if (i > pipe_init_count) {
            sal_printf("unit = %d: ING_HW_RESET timeout  \n", unit);
            break;
        }
        sal_usleep(100);
    } while(1);

    /* Wait for EPIPE memory initialization done. */
    i = 0;
    do {
        SOC_IF_ERROR_RETURN(
            READ_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1));
        if (EGR_HW_RESET_CONTROL_1r_DONEf_GET(egr_hw_reset_control_1)) {
            break;
        }
        i++;
        if (i > pipe_init_count) {
            sal_printf("unit = %d: EGR_HW_RESET timeout  \n", unit);
            break;
        }
        sal_usleep(100);
    } while(1);

    ING_HW_RESET_CONTROL_2r_CLR(ing_hw_reset_control_2);
    SOC_IF_ERROR_RETURN(
        WRITE_ING_HW_RESET_CONTROL_2r(unit, ing_hw_reset_control_2));
    EGR_HW_RESET_CONTROL_1r_CLR(egr_hw_reset_control_1);
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_HW_RESET_CONTROL_1r(unit, egr_hw_reset_control_1));

    return SYS_OK;
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
    for (pport = 0; pport <= BCM5607X_PORT_MAX; pport++) {
        ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_CLR(ing_physical_to_logical_port_number_mapping_table);
        if (SOC_PORT_P2L_MAPPING(pport) == -1) {
            ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_LOGICAL_PORT_NUMBERf_SET(ing_physical_to_logical_port_number_mapping_table, 0x7F);
        } else {
            ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm_LOGICAL_PORT_NUMBERf_SET(ing_physical_to_logical_port_number_mapping_table, SOC_PORT_P2L_MAPPING(pport));
        }
        SOC_IF_ERROR_RETURN(
            WRITE_ING_PHYSICAL_TO_LOGICAL_PORT_NUMBER_MAPPING_TABLEm(unit,
                pport ,ing_physical_to_logical_port_number_mapping_table));
    }

    /* Egress logical to physical port mapping, needs a way for maximum logical port? */
    for (lport = 0; lport <= BCM5607X_LPORT_MAX; lport++) {
        EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr_CLR(egr_logical_to_physical_port_number_mapping);
        EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr_PHYSICAL_PORT_NUMBERf_SET(egr_logical_to_physical_port_number_mapping,
                                        ((SOC_PORT_L2P_MAPPING(lport)== -1) ? 0x7F : (SOC_PORT_L2P_MAPPING(lport))));
        SOC_IF_ERROR_RETURN(
            WRITE_EGR_LOGICAL_TO_PHYSICAL_PORT_NUMBER_MAPPINGr(unit,
                lport, egr_logical_to_physical_port_number_mapping));
    }

    /* EGR_TDM_PORT_MAPm */
    EGR_TDM_PORT_MAPm_CLR(egr_tdm_port_map);

    PBMP_CLEAR(pbmp);
    for (lport = 0; lport <= BCM5607X_LPORT_MAX; lport++) {
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
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_TDM_PORT_MAPm(unit, egr_tdm_port_map));

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
        SOC_IF_ERROR_RETURN(
            WRITE_MMU_PORT_TO_PHY_PORT_MAPPINGr(unit,
                mmu_port, mmu_port_to_phy_port_mapping));

        if (lport == -1) {
            lport = 1;
        }

        MMU_PORT_TO_LOGIC_PORT_MAPPINGr_CLR(mmu_port_to_logic_port_mapping);
        MMU_PORT_TO_LOGIC_PORT_MAPPINGr_LOGIC_PORTf_SET(mmu_port_to_logic_port_mapping, lport);
        SOC_IF_ERROR_RETURN(
            WRITE_MMU_PORT_TO_LOGIC_PORT_MAPPINGr(unit,
                mmu_port, mmu_port_to_logic_port_mapping));
    }

    return SYS_OK;
}

static sys_error_t
soc_tsc_xgxs_reset(uint8 unit, uint8 port)
{
#if CONFIG_EMULATION
    int         sleep_usec = 500000;
    int         lcpll = 0;
#else
    int         sleep_usec = 1100;
    int         lcpll = 1;
#endif /* CONFIG_EMULATION */
    PMQ_XGXS0_CTRL_REGr_t pmq_xgxs0_ctrl_reg;
    XLPORT_XGXS0_CTRL_REGr_t xlport_xgxs0_ctrl_reg;
    CLPORT_XGXS0_CTRL_REGr_t clport_xgxs0_ctrl_reg;
    PGW_CTRL_0r_t pgw_ctrl_0;

    if (IS_GX_PORT(port)) {
        /* Turn on PGW for QTCE */
        SOC_IF_ERROR_RETURN(READ_PGW_CTRL_0r(unit, pgw_ctrl_0));
        PGW_CTRL_0r_SW_PM4X10Q_DISABLEf_SET(pgw_ctrl_0,
        PGW_CTRL_0r_SW_PM4X10Q_DISABLEf_GET(pgw_ctrl_0) & ~(1 << QTCE_CORE_NUM_GET(port)));
        SOC_IF_ERROR_RETURN(WRITE_PGW_CTRL_0r(unit, pgw_ctrl_0));

        /*
         * Reference clock selection
         */
        SOC_IF_ERROR_RETURN(
            READ_PMQ_XGXS0_CTRL_REGr(unit, port, pmq_xgxs0_ctrl_reg));
        PMQ_XGXS0_CTRL_REGr_IDDQf_SET(pmq_xgxs0_ctrl_reg, 0);
        SOC_IF_ERROR_RETURN(
            WRITE_PMQ_XGXS0_CTRL_REGr(unit, port, pmq_xgxs0_ctrl_reg));

        SOC_IF_ERROR_RETURN(
            READ_PMQ_XGXS0_CTRL_REGr(unit, port, pmq_xgxs0_ctrl_reg));
        PMQ_XGXS0_CTRL_REGr_REFIN_ENf_SET(pmq_xgxs0_ctrl_reg, lcpll ? 1 : 0);
        SOC_IF_ERROR_RETURN(
            WRITE_PMQ_XGXS0_CTRL_REGr(unit, port, pmq_xgxs0_ctrl_reg));

        /* Deassert power down */
        PMQ_XGXS0_CTRL_REGr_PWRDWNf_SET(pmq_xgxs0_ctrl_reg, 0);
        SOC_IF_ERROR_RETURN(
            WRITE_PMQ_XGXS0_CTRL_REGr(unit, port, pmq_xgxs0_ctrl_reg));
        sal_usleep(sleep_usec);

        /* Reset XGXS */
        PMQ_XGXS0_CTRL_REGr_RSTB_HWf_SET(pmq_xgxs0_ctrl_reg, 0);
        SOC_IF_ERROR_RETURN(
            WRITE_PMQ_XGXS0_CTRL_REGr(unit, port, pmq_xgxs0_ctrl_reg));
        sal_usleep(sleep_usec);

        /* Bring XGXS out of reset */
        PMQ_XGXS0_CTRL_REGr_RSTB_HWf_SET(pmq_xgxs0_ctrl_reg, 1);
        SOC_IF_ERROR_RETURN(
            WRITE_PMQ_XGXS0_CTRL_REGr(unit, port, pmq_xgxs0_ctrl_reg));
        sal_usleep(sleep_usec);

        SOC_IF_ERROR_RETURN(
            READ_PMQ_XGXS0_CTRL_REGr(unit, port, pmq_xgxs0_ctrl_reg));
        PMQ_XGXS0_CTRL_REGr_REFSELf_SET(pmq_xgxs0_ctrl_reg, 5);
        SOC_IF_ERROR_RETURN(
            WRITE_PMQ_XGXS0_CTRL_REGr(unit, port, pmq_xgxs0_ctrl_reg));
    } else if (IS_XL_PORT(port)) {
        SOC_IF_ERROR_RETURN(READ_PGW_CTRL_0r(unit, pgw_ctrl_0));
        PGW_CTRL_0r_SW_PM4X10Q_DISABLEf_SET(pgw_ctrl_0,
        PGW_CTRL_0r_SW_PM4X10Q_DISABLEf_GET(pgw_ctrl_0) & ~(1 << TSCE_CORE_NUM_GET(port)));
        SOC_IF_ERROR_RETURN(WRITE_PGW_CTRL_0r(unit, pgw_ctrl_0));

        /*
         * Reference clock selection
         */
        SOC_IF_ERROR_RETURN(
            READ_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg));
        XLPORT_XGXS0_CTRL_REGr_IDDQf_SET(xlport_xgxs0_ctrl_reg, 0);
        SOC_IF_ERROR_RETURN(
            WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg));

        SOC_IF_ERROR_RETURN(
            READ_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg));
        XLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(xlport_xgxs0_ctrl_reg, lcpll ? 1 : 0);
        SOC_IF_ERROR_RETURN(
            WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg));

        /* Deassert power down */
        XLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(xlport_xgxs0_ctrl_reg, 0);
        SOC_IF_ERROR_RETURN(
            WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg));
        sal_usleep(sleep_usec);

        /* Reset XGXS */
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_xgxs0_ctrl_reg, 0);
        SOC_IF_ERROR_RETURN(
            WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg));
        sal_usleep(sleep_usec);

        /* Bring XGXS out of reset */
        XLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(xlport_xgxs0_ctrl_reg, 1);
        SOC_IF_ERROR_RETURN(
            WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg));
        sal_usleep(sleep_usec);

        SOC_IF_ERROR_RETURN(
            READ_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg));
        XLPORT_XGXS0_CTRL_REGr_REFSELf_SET(xlport_xgxs0_ctrl_reg, 5);
        SOC_IF_ERROR_RETURN(
            WRITE_XLPORT_XGXS0_CTRL_REGr(unit, port, xlport_xgxs0_ctrl_reg));
    } else if (IS_CL_PORT(port)) {
        SOC_IF_ERROR_RETURN(READ_PGW_CTRL_0r(unit, pgw_ctrl_0));
        PGW_CTRL_0r_SW_PM4X25_DISABLEf_SET(pgw_ctrl_0,
        PGW_CTRL_0r_SW_PM4X25_DISABLEf_GET(pgw_ctrl_0) & ~(1 << TSCF_CORE_NUM_GET(port)));
        SOC_IF_ERROR_RETURN(WRITE_PGW_CTRL_0r(unit, pgw_ctrl_0));

        /*
         * Reference clock selection
         */
        SOC_IF_ERROR_RETURN(
            READ_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg));
        CLPORT_XGXS0_CTRL_REGr_IDDQf_SET(clport_xgxs0_ctrl_reg, 0);
        SOC_IF_ERROR_RETURN(
            WRITE_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg));

        SOC_IF_ERROR_RETURN(
            READ_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg));
        CLPORT_XGXS0_CTRL_REGr_REFIN_ENf_SET(clport_xgxs0_ctrl_reg, lcpll ? 1 : 0);
        SOC_IF_ERROR_RETURN(
            WRITE_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg));

        /* Deassert power down */
        CLPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(clport_xgxs0_ctrl_reg, 0);
        SOC_IF_ERROR_RETURN(
            WRITE_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg));
        sal_usleep(sleep_usec);

        /* Reset XGXS */
        CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(clport_xgxs0_ctrl_reg, 0);
        SOC_IF_ERROR_RETURN(
            WRITE_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg));
        sal_usleep(sleep_usec);

        /* Bring XGXS out of reset */
        CLPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(clport_xgxs0_ctrl_reg, 1);
        SOC_IF_ERROR_RETURN(
            WRITE_CLPORT_XGXS0_CTRL_REGr(unit, port, clport_xgxs0_ctrl_reg));
        sal_usleep(sleep_usec);
    }

    return SYS_OK;
}

static sys_error_t
soc_fl_tsc_reset(uint8 unit)
{
    uint8 lport;
    XLPORT_MAC_CONTROLr_t xlport_mac_control;
    CLPORT_MAC_CONTROLr_t clport_mac_control;
    PGW_CTRL_0r_t pgw_ctrl_0;

    /* Disable all serdes cores */
    SOC_IF_ERROR_RETURN(READ_PGW_CTRL_0r(unit, pgw_ctrl_0));
    PGW_CTRL_0r_SW_PM4X10Q_DISABLEf_SET(pgw_ctrl_0, 0x7);
    PGW_CTRL_0r_SW_PM4X25_DISABLEf_SET(pgw_ctrl_0, 0xF);
    SOC_IF_ERROR_RETURN(WRITE_PGW_CTRL_0r(unit, pgw_ctrl_0));

    /* TSC reset */
    SOC_LPORT_ITER(lport) {
        if (IS_GX_PORT(lport)) {
            if (SOC_PMQ_BLOCK_INDEX(lport) == 0) {
                SOC_IF_ERROR_RETURN(
                    soc_tsc_xgxs_reset(unit, lport));
            }
        } else if (SOC_PORT_BLOCK_INDEX(lport) == 0) {
            SOC_IF_ERROR_RETURN(
                soc_tsc_xgxs_reset(unit, lport));
        }
    }

    /* MAC reset */
    SOC_LPORT_ITER(lport) {
        if (IS_XL_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            SOC_IF_ERROR_RETURN(
                READ_XLPORT_MAC_CONTROLr(unit, lport, xlport_mac_control));
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_control, 1);
            SOC_IF_ERROR_RETURN(
                WRITE_XLPORT_MAC_CONTROLr(unit, lport, xlport_mac_control));
            sal_usleep(10);
            XLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(xlport_mac_control, 0);
            SOC_IF_ERROR_RETURN(
                WRITE_XLPORT_MAC_CONTROLr(unit, lport, xlport_mac_control));
        }

        if (IS_CL_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            SOC_IF_ERROR_RETURN(
                READ_CLPORT_MAC_CONTROLr(unit, lport, clport_mac_control));
            CLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(clport_mac_control, 1);
            SOC_IF_ERROR_RETURN(
                WRITE_CLPORT_MAC_CONTROLr(unit, lport, clport_mac_control));
            sal_usleep(10);
            CLPORT_MAC_CONTROLr_XMAC0_RESETf_SET(clport_mac_control, 0);
            CLPORT_MAC_CONTROLr_SYS_16B_INTF_MODEf_SET(clport_mac_control, 1);
            SOC_IF_ERROR_RETURN(
                WRITE_CLPORT_MAC_CONTROLr(unit, lport, clport_mac_control));
        }
    }

    return SYS_OK;
}

static int fl_ceiling_func(uint32 numerators, uint32 denominator)
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

static int fl_floor_func(uint32 numerators, uint32 denominator)
{
    uint32  result;
    if (denominator == 0) {
        return 0xFFFFFFFF;
    }
    result = numerators / denominator;

    return result;
}

/*
 * Function
 *      _uplink_pbmp()
 * Purpose:
 *      Get uplink pbmp from vendor config
 *      "SAL_CONFIG_UPLINK_LOGICAL_PORTS"
 */
static pbmp_t _uplink_pbmp()
{
    pbmp_t pbmp_uplink;
    pbmp_t pbmp_uplink_tmp;
    pbmp_t pbmp_all;

    PBMP_CLEAR(pbmp_uplink);
    PBMP_CLEAR(pbmp_uplink_tmp);
    PBMP_ASSIGN(pbmp_all, BCM5607X_ALL_PORTS_MASK);
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    if (sal_config_pbmp_get(SAL_CONFIG_UPLINK_LOGICAL_PORTS, &pbmp_uplink_tmp)
        == SYS_OK) {
        PBMP_ASSIGN(pbmp_uplink, pbmp_uplink_tmp);
    } else
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */
    {
        /* Set default active_pbmp to all CL ports if no active_pbmp input */
        PBMP_ASSIGN(pbmp_uplink_tmp, SOC_CONTROL(unit).info.cl.bitmap);
        PBMP_AND(pbmp_uplink_tmp, pbmp_all);
        PBMP_ASSIGN(pbmp_uplink, pbmp_uplink_tmp);
    }

    return pbmp_uplink;
}

/*
 * Function
 *      _standby_pbmp()
 * Purpose:
 *      Get standby pbmp from vendor config
 *      "SAL_CONFIG_STANDBY_LOGICAL_PORTS"
 */
static pbmp_t _standby_pbmp()
{
    pbmp_t pbmp_standby;
    pbmp_t pbmp_standby_tmp;

    PBMP_CLEAR(pbmp_standby);
    PBMP_CLEAR(pbmp_standby_tmp);
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    if (sal_config_pbmp_get(SAL_CONFIG_STANDBY_LOGICAL_PORTS, &pbmp_standby_tmp)
        == SYS_OK) {
        PBMP_ASSIGN(pbmp_standby, pbmp_standby_tmp);
    }
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

    return pbmp_standby;
}

/*
 * Function:
 *      _soc_firelight_mmu_init_helper_chassis()
 * Purpose:
 *      MMU init function for chassis mode
 */
static sys_error_t _soc_firelight_mmu_init_helper_chassis(int unit)
{
    uint32 rval;
    int port, phy_port, mport;
    int index;
    pbmp_t pbmp_all;
    pbmp_t pbmp_cpu;
    pbmp_t pbmp_downlink;
    pbmp_t pbmp_uplink_6kxq;
    pbmp_t pbmp_uplink_2kxq;
    pbmp_t pbmp_uplink;
    int check_uplink_speed_type;
    int check_downlink_speed_type;
    int speed = 0;
    int standard_jumbo_frame;
    int cell_size;
    int ethernet_mtu_cell;
    int standard_jumbo_frame_cell;
    int total_cell_memory_for_admission;
    int skidmarker;
    int prefetch;
    int total_cell_memory;
    int total_buffer_space_for_downstream_traffic;
    int total_buffer_space_for_upstream_channelized_fc_message;
    int total_buffer_space_for_upstream_data_high_traffic;
    int total_buffer_space_for_cpu_traffic;
    int total_headroom_space_for_downstream_traffic;
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
    E2ECC_MODEr_t e2ecc_mode;
    E2ECC_HOL_ENr_t e2ecc_hol_en;
    E2ECC_MIN_TX_TIMERr_t e2ecc_min_tx_timer;
    E2ECC_MAX_TX_TIMERr_t e2ecc_max_tx_timer;
    E2ECC_TX_ENABLE_BMPr_t e2ecc_tx_enable_bmp;
    E2ECC_TX_PORTS_NUMr_t e2ecc_tx_ports_num;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    HOLCOSMINXQCNT_QLAYERr_t holcosminxqcnt_qlayer;
    HOLCOSPKTSETLIMIT_QLAYERr_t holcospktsetlimit_qlayer;
    HOLCOSPKTRESETLIMIT_QLAYERr_t holcospktresetlimit_qlayer;
    CNGCOSPKTLIMIT0_QLAYERr_t cngcospktlimit0_qlayer;
    CNGCOSPKTLIMIT1_QLAYERr_t cngcospktlimit1_qlayer;
    CNGPORTPKTLIMIT0r_t cngportpktlimit0;
    CNGPORTPKTLIMIT1r_t cngportpktlimit1;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    LWMCOSCELLSETLIMIT_QLAYERr_t lwmcoscellsetlimit_qlayer;
    HOLCOSCELLMAXLIMIT_QLAYERr_t holcoscellmaxlimit_qlayer;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_EN_QLAYERr_t color_drop_en_qlayer;
    COLOR_DROP_ENr_t color_drop_en;
    HOLCOSPKTSETLIMIT_QGROUPr_t holcospktsetlimit_qgroup;
    HOLCOSPKTRESETLIMIT_QGROUPr_t holcospktresetlimit_qgroup;
    CNGCOSPKTLIMIT0_QGROUPr_t cngcospktlimit0_qgroup;
    CNGCOSPKTLIMIT1_QGROUPr_t cngcospktlimit1_qgroup;
    HOLCOSCELLMAXLIMIT_QGROUPr_t holcoscellmaxlimit_qgroup;
    COLOR_DROP_EN_QGROUPr_t color_drop_en_qgroup;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;
    SHARED_POOL_CTRL_EXT1r_t shared_pool_ctrl_ext1;
    SHARED_POOL_CTRL_EXT2r_t shared_pool_ctrl_ext2;
    E2ECC_PORT_CONFIGr_t e2ecc_port_cfg;
    EARLY_DYNCELLLIMITr_t early_dyncelllimit;
    EARLY_HOLCOSCELLMAXLIMITr_t early_holcoscellmaxlimit;
    int cfapfullsetpoint;
    int total_advertised_cell_memory;
    int number_of_active_uplink_ports;
    int number_of_downlink_ports;
    int headroom_for_backplane_port;
    int num_1g_ports_downlink_ports;
    int num_2dot5g_ports_downlink_ports;
    int num_5g_ports_downlink_ports;
    int num_10g_ports_downlink_ports;
    int num_100g_ports_uplink_ports;
    int num_50g_ports_uplink_ports;
    int num_40g_ports_uplink_ports;
    int num_25g_ports_uplink_ports;
    int num_10g_ports_uplink_ports;
    int num_data_classes;
    int mmu_xoff_pkt_threshold_uplink_ports;
    int mmu_xoff_pkt_threshold_downlink_ports;
    int mmu_xoff_cell_threshold_all_downlink_ports;
    int mmu_xoff_cell_threshold_all_uplink_ports;
    int xoff_cell_threshold_all_downlink_ports;
    int egress_queue_min_reserve_uplink_ports;
    int egress_queue_min_reserve_downlink_ports;
    int egress_queue_min_reserve_cpu_ports;
    int egress_xq_min_reserve_backplane_ports;
    int egress_xq_min_reserve_access_ports;
    int egress_xq_min_reserve_cpu_ports;
    int num_backplane_queues;
    int num_access_queues;
    int num_active_egress_queues_per_access_port;
    int num_cpu_queues;
    int num_cpu_ports;
    int numxqs_per_backplane_ports;
    int numxqs_per_downlink_ports_and_cpu_port;
    int xoff_cell_threshold_all_uplink_ports;
    int xoff_cell_threshold_per_cpu_port;
    int xoff_packet_thresholds_per_port_uplink_port;
    int xoff_packet_thresholds_per_port_downlink_port;
    int discard_limit_per_port_pg_backplane_port;
    int discard_limit_per_port_pg0_access_port;
    int discard_limit_per_port_pg1_access_port;
    int discard_limit_per_port_pg7_access_port;
    int discard_limit_per_queue_access_port;
    int total_reserved_cells_for_uplink_ports;
    int total_reserved_cells_for_downlink_ports;
    int total_reserved_cells_for_cpu_port;
    int total_reserved;
    int shared_space_cells;
    int reserved_xqs_per_6kxq_backplane_port;
    int reserved_xqs_per_2kxq_backplane_port;
    int shared_xqs_per_6kxq_backplane_port;
    int shared_xqs_per_2kxq_backplane_port;
    int shared_xqs_per_access_port;
    int reserved_xqs_per_access_port;
    int gbllimitsetlimit_gblcellsetlimit_up;

    num_cpu_ports = 0;
    num_data_classes = 2;
    num_access_queues = 2;
    total_buffer_space_for_downstream_traffic = 5040;
    number_of_active_uplink_ports = 0;
    number_of_downlink_ports = 0;
    num_1g_ports_downlink_ports = 0;
    num_2dot5g_ports_downlink_ports = 0;
    num_5g_ports_downlink_ports = 0;
    num_10g_ports_downlink_ports = 0;
    num_100g_ports_uplink_ports = 0;
    num_50g_ports_uplink_ports = 0;
    num_40g_ports_uplink_ports = 0;
    num_25g_ports_uplink_ports = 0;
    num_10g_ports_uplink_ports = 0;
    PBMP_ASSIGN(pbmp_all, BCM5607X_ALL_PORTS_MASK);
    PBMP_CLEAR(pbmp_cpu);
    PBMP_CLEAR(pbmp_downlink);
    PBMP_CLEAR(pbmp_uplink_6kxq);
    PBMP_CLEAR(pbmp_uplink_2kxq);
    PBMP_CLEAR(pbmp_uplink);
    pbmp_uplink = _uplink_pbmp();

    for (phy_port = 0; phy_port <= BCM5607X_PORT_MAX; phy_port++) {
        port = SOC_PORT_P2L_MAPPING(phy_port);
        if (port < 0) {
            continue; /* this user port has not been mapping in this sku */
        }
        mport = SOC_PORT_P2M_MAPPING(phy_port);
        if (IS_CPU_PORT(port)) {
            num_cpu_ports++;
            PBMP_PORT_ADD(pbmp_cpu, port);
            continue;
        }
        speed = SOC_PORT_SPEED_MAX(port);
        if (speed == 0) {
            continue; /* this user port has not been mapping in this sku */
        } else if (!PBMP_MEMBER(pbmp_all, port)) {
            continue; /* this user port has been masked out by pbmp_valid */
        }
        if (PBMP_MEMBER(pbmp_uplink, port)) {
            number_of_active_uplink_ports++;
            if (mport >= MMU_64Q_PPORT_BASE) {
                PBMP_PORT_ADD(pbmp_uplink_6kxq, port);
            } else {
                PBMP_PORT_ADD(pbmp_uplink_2kxq, port);
            }
        } else {
            number_of_downlink_ports++;
            PBMP_PORT_ADD(pbmp_downlink, port);
            if (speed > 5000) {
                num_10g_ports_downlink_ports++;
            } else if (speed > 2500) {
                num_5g_ports_downlink_ports++;
            } else if (speed > 1000) {
                num_2dot5g_ports_downlink_ports++;
            } else {
                num_1g_ports_downlink_ports++;
            }
        }
    }

    if (number_of_active_uplink_ports == 0) {
        sal_printf("Wrong config of the active packplane ports\n");
        return SYS_ERR_PARAMETER;
    }

    /* Check all active backplane ports has the same speed */
    PBMP_ITER(pbmp_uplink, port) {
        speed = SOC_PORT_SPEED_MAX(port);
        if (speed > 100000) {
            sal_printf("for backplane port %d,\
                        the max speed cannot exceed 100G (value=%d)\n",
                        port, speed);
            return SYS_ERR_PARAMETER;
        } else if (speed < 10000) {
            PBMP_PORT_REMOVE(pbmp_uplink, port);
        } else if (speed > 50000) {
            num_100g_ports_uplink_ports++;
        } else if (speed > 40000) {
            num_50g_ports_uplink_ports++;
        } else if (speed > 25000) {
            num_40g_ports_uplink_ports++;
        } else if (speed > 10000) {
            num_25g_ports_uplink_ports++;
        } else {
            num_10g_ports_uplink_ports++;
        }
    }

    check_uplink_speed_type = 0;
    check_uplink_speed_type = (num_100g_ports_uplink_ports == 0 ? 0 : 1) +
        (num_50g_ports_uplink_ports == 0 ? 0 : 1) +
        (num_40g_ports_uplink_ports == 0 ? 0 : 1) +
        (num_25g_ports_uplink_ports == 0 ? 0 : 1) +
        (num_10g_ports_uplink_ports == 0 ? 0 : 1);
    if (check_uplink_speed_type > 1) {
        sal_printf("The backplane ports have the different port speed\n");
        return SYS_ERR_PARAMETER;
    }

    check_downlink_speed_type = 0;
    check_downlink_speed_type = (num_10g_ports_downlink_ports == 0 ? 0 : 1) +
        (num_5g_ports_downlink_ports == 0 ? 0 : 1) +
        (num_2dot5g_ports_downlink_ports == 0 ? 0 : 1) +
        (num_1g_ports_downlink_ports == 0 ? 0 : 1);
    if (check_downlink_speed_type > 1) {
        sal_printf("The access ports have the different port speed\n");
        return SYS_ERR_PARAMETER;
    }

    standard_jumbo_frame = 9216;
    cell_size = 144;
    ethernet_mtu_cell = fl_ceiling_func(15 * 1024 / 10, cell_size);
    standard_jumbo_frame_cell =
          fl_ceiling_func(standard_jumbo_frame, cell_size);
    total_cell_memory_for_admission = 14336;
    skidmarker = 7;
    prefetch = 64 + 4;
    total_cell_memory = total_cell_memory_for_admission;
    cfapfullsetpoint = 16222;
    total_advertised_cell_memory = total_cell_memory;
    mmu_xoff_pkt_threshold_uplink_ports = total_advertised_cell_memory;
    mmu_xoff_pkt_threshold_downlink_ports = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_all_uplink_ports = total_advertised_cell_memory;
    mmu_xoff_cell_threshold_all_downlink_ports =
        total_buffer_space_for_downstream_traffic/number_of_active_uplink_ports;
    num_active_egress_queues_per_access_port = num_access_queues;
    egress_queue_min_reserve_uplink_ports = 0;
    egress_queue_min_reserve_downlink_ports = 0;
    egress_queue_min_reserve_cpu_ports = 0;
    egress_xq_min_reserve_backplane_ports = 0;
    egress_xq_min_reserve_access_ports = 0;
    egress_xq_min_reserve_cpu_ports = 0;
    num_backplane_queues = num_data_classes + 1;
    total_buffer_space_for_upstream_channelized_fc_message = 96;
    total_buffer_space_for_upstream_data_high_traffic = 1248;
    total_buffer_space_for_cpu_traffic = 30;
    total_headroom_space_for_downstream_traffic = 1032;
    num_cpu_queues = 8;
    numxqs_per_backplane_ports = 6 * 1024;
    numxqs_per_downlink_ports_and_cpu_port = 2 * 1024;
    headroom_for_backplane_port =
        total_headroom_space_for_downstream_traffic/number_of_active_uplink_ports;
    xoff_cell_threshold_all_downlink_ports =
          mmu_xoff_cell_threshold_all_downlink_ports;
    xoff_cell_threshold_all_uplink_ports =
          mmu_xoff_cell_threshold_all_uplink_ports;
    xoff_cell_threshold_per_cpu_port =
        mmu_xoff_cell_threshold_all_uplink_ports;
    xoff_packet_thresholds_per_port_uplink_port =
          mmu_xoff_pkt_threshold_uplink_ports;
    xoff_packet_thresholds_per_port_downlink_port =
          mmu_xoff_pkt_threshold_downlink_ports;
    discard_limit_per_port_pg_backplane_port =
        headroom_for_backplane_port + xoff_cell_threshold_all_downlink_ports;
    discard_limit_per_port_pg0_access_port = fl_floor_func
        ((total_advertised_cell_memory -
        total_buffer_space_for_downstream_traffic -
        total_headroom_space_for_downstream_traffic -
        total_buffer_space_for_upstream_channelized_fc_message -
        total_buffer_space_for_upstream_data_high_traffic -
        total_buffer_space_for_cpu_traffic), number_of_downlink_ports);
    discard_limit_per_port_pg1_access_port = fl_floor_func
        (total_buffer_space_for_upstream_data_high_traffic,
        number_of_downlink_ports);
    discard_limit_per_port_pg7_access_port = fl_floor_func
        (total_buffer_space_for_upstream_channelized_fc_message,
        number_of_downlink_ports);
    discard_limit_per_queue_access_port = fl_floor_func
        (total_buffer_space_for_downstream_traffic,
        number_of_downlink_ports);
    total_reserved_cells_for_uplink_ports =
          egress_queue_min_reserve_uplink_ports *
          number_of_active_uplink_ports * num_data_classes;
    total_reserved_cells_for_downlink_ports =
          number_of_downlink_ports *
          egress_queue_min_reserve_downlink_ports * num_access_queues;
    total_reserved_cells_for_cpu_port =
          num_cpu_ports * egress_queue_min_reserve_cpu_ports * num_cpu_queues;
    total_reserved =
          total_reserved_cells_for_uplink_ports +
          total_reserved_cells_for_downlink_ports +
          total_reserved_cells_for_cpu_port;
    shared_space_cells = total_advertised_cell_memory - total_reserved;
    reserved_xqs_per_6kxq_backplane_port =
        egress_xq_min_reserve_backplane_ports * num_backplane_queues;
    reserved_xqs_per_2kxq_backplane_port =
        egress_xq_min_reserve_backplane_ports * num_backplane_queues;
    shared_xqs_per_6kxq_backplane_port =
        numxqs_per_backplane_ports - reserved_xqs_per_6kxq_backplane_port;
    shared_xqs_per_2kxq_backplane_port =
        numxqs_per_downlink_ports_and_cpu_port -
        reserved_xqs_per_2kxq_backplane_port;
    reserved_xqs_per_access_port =
        num_active_egress_queues_per_access_port *
        egress_xq_min_reserve_access_ports;
    shared_xqs_per_access_port =
        numxqs_per_downlink_ports_and_cpu_port - reserved_xqs_per_access_port;
    gbllimitsetlimit_gblcellsetlimit_up = total_cell_memory_for_admission;

    /* system-based */
    READ_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);
    CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_SET(cfapfullthreshold,
        cfapfullsetpoint);
    CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_SET(cfapfullthreshold,
                            cfapfullsetpoint - (standard_jumbo_frame_cell * 2));
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    READ_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);
    GBLLIMITSETLIMITr_GBLCELLSETLIMITf_SET(gbllimitsetlimit,
                           total_cell_memory_for_admission);
    WRITE_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);

    READ_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);
    GBLLIMITRESETLIMITr_GBLCELLRESETLIMITf_SET(gbllimitresetlimit,
        gbllimitsetlimit_gblcellsetlimit_up);
    WRITE_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);

    READ_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);
    TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_SET(totaldyncellsetlimit,
        shared_space_cells);
    WRITE_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);

    READ_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_SET(totaldyncellresetlimit,
        shared_space_cells - (standard_jumbo_frame_cell * 2));
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
    MISCCONFIGr_CNG_DROP_ENf_SET(miscconfig, 0);
    MISCCONFIGr_DYN_XQ_ENf_SET(miscconfig, 0);
    MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_SET(miscconfig, 0);
    MISCCONFIGr_DYNAMIC_MEMORY_ENf_SET(miscconfig, 1);
    MISCCONFIGr_SKIDMARKERf_SET(miscconfig, 3);
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

    E2ECC_MODEr_SET(e2ecc_mode, 1);
    WRITE_E2ECC_MODEr(unit, e2ecc_mode);

    E2ECC_HOL_ENr_SET(e2ecc_hol_en, 1);
    WRITE_E2ECC_HOL_ENr(unit, e2ecc_hol_en);

    READ_E2ECC_MIN_TX_TIMERr(unit, e2ecc_min_tx_timer);
    E2ECC_MIN_TX_TIMERr_LGf_SET(e2ecc_min_tx_timer, 0);
    E2ECC_MIN_TX_TIMERr_TIMERf_SET(e2ecc_min_tx_timer, 12);
    WRITE_E2ECC_MIN_TX_TIMERr(unit, e2ecc_min_tx_timer);

    WRITE_E2ECC_MAX_TX_TIMERr(unit, e2ecc_max_tx_timer);
    E2ECC_MAX_TX_TIMERr_LGf_SET(e2ecc_max_tx_timer, 0);
    E2ECC_MAX_TX_TIMERr_TIMERf_SET(e2ecc_max_tx_timer, 12);
    WRITE_E2ECC_MAX_TX_TIMERr(unit, e2ecc_max_tx_timer);

    /* E2ECC_TX_ENABLE_BMPr, index 0 ~ 7 */
    for (index = 0; index <= 7; index++) {
        E2ECC_TX_ENABLE_BMPr_SET(e2ecc_tx_enable_bmp, 15);
        WRITE_E2ECC_TX_ENABLE_BMPr(unit, index, e2ecc_tx_enable_bmp);
    }
    E2ECC_TX_PORTS_NUMr_SET(e2ecc_tx_ports_num,number_of_downlink_ports + 2);
    WRITE_E2ECC_TX_PORTS_NUMr(unit, e2ecc_tx_ports_num);

    /* port-based : 6kxq backplane ports */
    PBMP_ITER(pbmp_uplink_6kxq, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
        /* PG_CTRL0r */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 128);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r */
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

        /* PG2TCr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* PG2TCr, index 7 */
        index = 7;
        READ_PG2TCr(unit, mport, index, pg2tc);
        PG2TCr_PG_BMPf_SET(pg2tc, 128);
        WRITE_PG2TCr(unit, mport, index, pg2tc);

        /* IBPPKTSETLIMITr */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit,
            xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 0);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 1);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 128);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_all_uplink_ports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 7 */
        index = 7;
        READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
            xoff_cell_threshold_all_downlink_ports);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
            xoff_cell_threshold_all_downlink_ports - ethernet_mtu_cell);
        WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
                total_advertised_cell_memory);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* PGDISCARDSETLIMITr, index 7 */
        index = 7;
        READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
            discard_limit_per_port_pg_backplane_port);
        WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_backplane_ports);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer,
                shared_xqs_per_6kxq_backplane_port - skidmarker -
                prefetch + egress_xq_min_reserve_backplane_ports);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer,
                shared_xqs_per_6kxq_backplane_port - skidmarker -
                prefetch + egress_xq_min_reserve_backplane_ports - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                holcospktresetlimit_qlayer);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index,
                cngcospktlimit0_qlayer);
            CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer,
                numxqs_per_backplane_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index,
                cngcospktlimit0_qlayer);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index,
                cngcospktlimit1_qlayer);
            CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer,
                numxqs_per_backplane_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index,
                cngcospktlimit1_qlayer);
        }

        /* CNGPORTPKTLIMIT0r*/
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_backplane_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r*/
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_backplane_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr*/
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport,
            shared_xqs_per_6kxq_backplane_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr*/
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport,
            shared_xqs_per_6kxq_backplane_port - skidmarker - prefetch - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer,
                egress_queue_min_reserve_uplink_ports);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer,
                egress_queue_min_reserve_uplink_ports);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, cfapfullsetpoint);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, cfapfullsetpoint -
                ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                holcoscellmaxlimit_qlayer);
        }

        /* DYNCELLLIMITr */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, cfapfullsetpoint);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            cfapfullsetpoint - (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
        for (index = 0; index <= 1; index++) {
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, index,
                color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, index,
                color_drop_en_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index,
                holcospktsetlimit_qgroup);
            HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(holcospktsetlimit_qgroup,
                numxqs_per_backplane_ports - 1);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index,
                holcospktsetlimit_qgroup);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index,
                holcospktresetlimit_qgroup);
            HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qgroup, 6142);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index,
                holcospktresetlimit_qgroup);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index,
                cngcospktlimit0_qgroup);
            CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qgroup, 6143);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index,
                cngcospktlimit0_qgroup);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index,
                cngcospktlimit1_qgroup);
            CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qgroup, 6143);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index,
            cngcospktlimit1_qgroup);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index,
                holcoscellmaxlimit_qgroup);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qgroup, cfapfullsetpoint);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qgroup,
                cfapfullsetpoint - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index,
                holcoscellmaxlimit_qgroup);
        }

        /* COLOR_DROP_EN_QGROUPr */
        READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* SHARED_POOL_CTRL_EXT1r */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1,
            0);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r */
        READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2,
            0);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);

        /* E2ECC_PORT_CONFIGr */
        READ_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, cfapfullsetpoint);
        WRITE_EARLY_DYNCELLLIMITr(unit, mport, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit,
                cfapfullsetpoint);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mport, index,
                early_holcoscellmaxlimit);
        }
    }

    /* port-based : 2kxq backplane ports */
    PBMP_ITER(pbmp_uplink_2kxq, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
        /* PG_CTRL0r */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 128);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r */
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

        /* PG2TCr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* PG2TCr, index 7 */
        index = 7;
        READ_PG2TCr(unit, mport, index, pg2tc);
        PG2TCr_PG_BMPf_SET(pg2tc, 128);
        WRITE_PG2TCr(unit, mport, index, pg2tc);

        /* IBPPKTSETLIMITr */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit,
            xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 0);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 1);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 128);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_all_uplink_ports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_all_uplink_ports - ethernet_mtu_cell);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 7 */
        index = 7;
        READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
            xoff_cell_threshold_all_downlink_ports);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
            xoff_cell_threshold_all_downlink_ports);
        WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
                total_advertised_cell_memory);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* PGDISCARDSETLIMITr, index 7 */
        index = 7;
        READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
            discard_limit_per_port_pg_backplane_port);
        WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_backplane_ports);
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }

        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit,
                shared_xqs_per_2kxq_backplane_port - skidmarker -
                prefetch + egress_xq_min_reserve_backplane_ports);
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }

        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                shared_xqs_per_2kxq_backplane_port - skidmarker -
                prefetch + egress_xq_min_reserve_backplane_ports - 1);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }

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

        /* CNGPORTPKTLIMIT0r*/
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r*/
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr*/
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport,
            shared_xqs_per_2kxq_backplane_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr*/
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport,
            shared_xqs_per_2kxq_backplane_port - skidmarker - prefetch - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_uplink_ports);
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit,
                cfapfullsetpoint);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                cfapfullsetpoint - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr*/
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, cfapfullsetpoint);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            cfapfullsetpoint - (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_ENr */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* E2ECC_PORT_CONFIGr*/
        READ_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, cfapfullsetpoint);
        WRITE_EARLY_DYNCELLLIMITr(unit, mport, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit,
                cfapfullsetpoint);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mport, index,
                early_holcoscellmaxlimit);
        }
    }

    /* port-based : downlink */
    PBMP_ITER(pbmp_downlink, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* PG_CTRL0r */
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

        /* PG_CTRL1r */
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

        /* IBPPKTSETLIMITr */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit,
            xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 0);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 255);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_all_uplink_ports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_all_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 */
        index = 0;
        READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
            discard_limit_per_port_pg0_access_port);
        WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);

        /* PGDISCARDSETLIMITr, index 1 ~ 6 */
        for (index = 1; index <= 6; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
                discard_limit_per_port_pg1_access_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* PGDISCARDSETLIMITr, index 7 */
        index = 7;
        READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
            discard_limit_per_port_pg7_access_port);
        WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                    egress_xq_min_reserve_access_ports);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer,
                    shared_xqs_per_access_port - skidmarker -
                    prefetch + egress_xq_min_reserve_access_ports);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer,
                    shared_xqs_per_access_port - skidmarker -
                    prefetch + egress_xq_min_reserve_access_ports - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                    holcospktresetlimit_qlayer);
            }

            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index,
                    cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qlayer,
                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index,
                    cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index,
                    cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qlayer,
                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index,
                    cngcospktlimit1_qlayer);
            }
        } else {
            /* HOLCOSMINXQCNTr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                    egress_xq_min_reserve_access_ports);
                WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            }

            /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit,
                    shared_xqs_per_access_port - skidmarker -
                    prefetch + egress_xq_min_reserve_access_ports);
                WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            }

            /* HOLCOSPKTSETLIMITr, index 7 */
            index = 7;
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit,
                shared_xqs_per_access_port - skidmarker -
                prefetch + egress_xq_min_reserve_access_ports);
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);

            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index,
                    holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                    shared_xqs_per_access_port - skidmarker -
                    prefetch + egress_xq_min_reserve_access_ports - 1);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index,
                    holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            index = 7;
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                shared_xqs_per_access_port - skidmarker -
                prefetch + egress_xq_min_reserve_access_ports - 1);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);

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
        }

        /* CNGPORTPKTLIMIT0r*/
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r*/
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr*/
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport,
            shared_xqs_per_access_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr*/
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport,
            shared_xqs_per_access_port - skidmarker - prefetch - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer,
                    egress_queue_min_reserve_downlink_ports);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer,
                    egress_queue_min_reserve_downlink_ports);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, cfapfullsetpoint);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer,
                    cfapfullsetpoint - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                    holcoscellmaxlimit_qlayer);
            }
        } else {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index,
                    lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_downlink_ports);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_downlink_ports);
                WRITE_LWMCOSCELLSETLIMITr(unit, mport, index,
                    lwmcoscellsetlimit);
            }

            /* LWMCOSCELLSETLIMITr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_downlink_ports);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_downlink_ports);
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);

            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index,
                    holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit,
                    discard_limit_per_queue_access_port);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                    discard_limit_per_queue_access_port - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index,
                    holcoscellmaxlimit);
            }
        }

        /* DYNCELLLIMITr */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, cfapfullsetpoint);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            cfapfullsetpoint - (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
            for (index = 0; index <= 1; index++) {
                READ_COLOR_DROP_EN_QLAYERr(unit, mport, index,
                    color_drop_en_qlayer);
                COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer,
                    0);
                WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, index,
                    color_drop_en_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index,
                    holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qgroup,
                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index,
                    holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index,
                    holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qgroup,
                    numxqs_per_downlink_ports_and_cpu_port - 1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index,
                    holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index,
                    cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qgroup,
                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index,
                    cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index,
                    cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qgroup,
                    numxqs_per_downlink_ports_and_cpu_port - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index,
                    cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qgroup, cfapfullsetpoint);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    cfapfullsetpoint - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        } else {
            /* COLOR_DROP_ENr */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
            WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);
        }

        /* SHARED_POOL_CTRLr */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext1, 0);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext2, 0);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        }

        /* E2ECC_PORT_CONFIGr */
        READ_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 255);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr*/
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, cfapfullsetpoint);
        WRITE_EARLY_DYNCELLLIMITr(unit, mport, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        rval = fl_floor_func
            (total_buffer_space_for_upstream_channelized_fc_message +
            total_buffer_space_for_upstream_data_high_traffic,
            number_of_downlink_ports) + 1;
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, rval);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mport, index,
                early_holcoscellmaxlimit);
        }
    }

    /* port-based : cpu port*/
    PBMP_ITER(pbmp_cpu, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* PG_CTRL0r */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 0);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 0);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 0);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 0);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 0);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 0);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 0);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 0);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 0);
        WRITE_PG_CTRL1r(unit, mport, pg_ctrl1);

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mport, index, pg2tc);
        }

        /* IBPPKTSETLIMITr */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit,
            xoff_packet_thresholds_per_port_downlink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 0);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

       /* MMU_FC_RX_ENr */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_per_cpu_port);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_per_cpu_port);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
                total_buffer_space_for_cpu_traffic);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_cpu_ports);
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }

        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit,
                shared_xqs_per_access_port - skidmarker -
                prefetch + egress_xq_min_reserve_cpu_ports);
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }

        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                shared_xqs_per_access_port - skidmarker -
                prefetch + egress_xq_min_reserve_cpu_ports - 1);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }

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

        /* CNGPORTPKTLIMIT0r*/
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r*/
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_downlink_ports_and_cpu_port - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr*/
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport,
            shared_xqs_per_access_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr*/
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport,
            shared_xqs_per_access_port - skidmarker - prefetch - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_cpu_ports);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_cpu_ports);
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit,
                total_buffer_space_for_cpu_traffic);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                total_buffer_space_for_cpu_traffic - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, cfapfullsetpoint);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            cfapfullsetpoint - (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_ENr */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* E2ECC_PORT_CONFIGr */
        READ_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, cfapfullsetpoint);
        WRITE_EARLY_DYNCELLLIMITr(unit, mport, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit,
                cfapfullsetpoint);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mport, index,
                early_holcoscellmaxlimit);
        }
    }

    return SYS_OK;
}
/*
 * Function:
 *      _soc_firelight_mmu_init_helper_custom_chassis()
 * Purpose:
 *      MMU init function for Custom chassis mode
 */
static sys_error_t _soc_firelight_mmu_init_helper_custom_chassis(int unit)
{
    int port, phy_port, mport;
    int index;
    pbmp_t pbmp_all;
    pbmp_t pbmp_cpu;
    pbmp_t pbmp_downlink_6kxq;
    pbmp_t pbmp_downlink_2kxq;
    pbmp_t pbmp_uplink_6kxq;
    pbmp_t pbmp_uplink_2kxq;
    pbmp_t pbmp_uplink;
    int speed = 0;
    int cell_size;
    int ethernet_mtu_cell;
    int max_packet_size;
    int cfapfullsetpoint;
    int reserved_for_cfap;
    int num_of_used_memory_banks;
    int standard_jumbo_frame;
    int standard_jumbo_frame_cell;
    int total_cell_memory_for_admission;
    int total_cell_memory;
    int total_advertised_cell_memory;
    int mmu_xoff_pkt_threshold_uplink_ports;
    int xoff_packet_thresholds_per_port_uplink_port;
    int xoff_cell_threshold_uplink_ports;
    int xoff_cell_threshold_downlink_ports;
    int discard_limit_per_port_pg_uplink_port;
    int discard_limit_per_port_pg_downlink_port;
    int egress_xq_min_reserve_lossy_uplink_ports;
    int num_lossy_queues_sp_traffic;
    int num_lossy_queues_wdrr_traffic;
    int egress_xq_min_reserve_lossless_uplink_ports;
    int egress_xq_min_reserve_lossless_downlink_ports;
    int num_lossless_queues;
    int reserved_xqs_per_uplink_port;
    int shared_xqs_per_6kxq_uplink_port;
    int shared_xqs_per_2kxq_uplink_port;
    int reserved_xqs_per_downlink_port;
    int shared_xqs_per_6kxq_downlink_port;
    int shared_xqs_per_2kxq_downlink_port;
    int skidmarker;
    int prefetch;
    int numxqs_6kxq_uplink_ports;
    int numxqs_2kxq_uplink_ports;
    int numxqs_6kxq_downlink_ports;
    int numxqs_2kxq_downlink_ports;
    int buffer_space_for_upstream_traffic;
    int wrr_reserve;
    int shared_space_cells;
    int port_shared_limit_downlink_ports;
    int port_shared_limit_cpu_ports;
    int total_reserved_for_uplink_ports;
    int total_reserved_for_downlink_ports;
    int total_reserved_for_cpu_port;
    int total_reserved;
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
    E2ECC_MODEr_t e2ecc_mode;
    E2ECC_HOL_ENr_t e2ecc_hol_en;
    E2ECC_MIN_TX_TIMERr_t e2ecc_min_tx_timer;
    E2ECC_MAX_TX_TIMERr_t e2ecc_max_tx_timer;
    E2ECC_TX_ENABLE_BMPr_t e2ecc_tx_enable_bmp;
    E2ECC_TX_PORTS_NUMr_t e2ecc_tx_ports_num;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    CNGPORTPKTLIMIT0r_t cngportpktlimit0;
    CNGPORTPKTLIMIT1r_t cngportpktlimit1;
    HOLCOSMINXQCNT_QLAYERr_t holcosminxqcnt_qlayer;
    HOLCOSPKTSETLIMIT_QLAYERr_t holcospktsetlimit_qlayer;
    HOLCOSPKTRESETLIMIT_QLAYERr_t holcospktresetlimit_qlayer;
    CNGCOSPKTLIMIT0_QLAYERr_t cngcospktlimit0_qlayer;
    CNGCOSPKTLIMIT1_QLAYERr_t cngcospktlimit1_qlayer;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    LWMCOSCELLSETLIMIT_QLAYERr_t lwmcoscellsetlimit_qlayer;
    HOLCOSCELLMAXLIMIT_QLAYERr_t holcoscellmaxlimit_qlayer;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_EN_QLAYERr_t color_drop_en_qlayer;
    COLOR_DROP_ENr_t color_drop_en;
    HOLCOSPKTSETLIMIT_QGROUPr_t holcospktsetlimit_qgroup;
    HOLCOSPKTRESETLIMIT_QGROUPr_t holcospktresetlimit_qgroup;
    CNGCOSPKTLIMIT0_QGROUPr_t cngcospktlimit0_qgroup;
    CNGCOSPKTLIMIT1_QGROUPr_t cngcospktlimit1_qgroup;
    HOLCOSCELLMAXLIMIT_QGROUPr_t holcoscellmaxlimit_qgroup;
    COLOR_DROP_EN_QGROUPr_t color_drop_en_qgroup;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;
    SHARED_POOL_CTRL_EXT1r_t shared_pool_ctrl_ext1;
    SHARED_POOL_CTRL_EXT2r_t shared_pool_ctrl_ext2;
    E2ECC_PORT_CONFIGr_t e2ecc_port_cfg;
    EARLY_DYNCELLLIMITr_t early_dyncelllimit;
    EARLY_HOLCOSCELLMAXLIMITr_t early_holcoscellmaxlimit;
    int number_of_uplink_ports;
    int number_of_downlink_ports;
    int num_cpu_ports;
    number_of_uplink_ports = 0;
    number_of_downlink_ports = 0;
    num_cpu_ports = 0;
    PBMP_ASSIGN(pbmp_all, BCM5607X_ALL_PORTS_MASK);
    PBMP_CLEAR(pbmp_cpu);
    PBMP_CLEAR(pbmp_downlink_6kxq);
    PBMP_CLEAR(pbmp_downlink_2kxq);
    PBMP_CLEAR(pbmp_uplink_6kxq);
    PBMP_CLEAR(pbmp_uplink_2kxq);
    PBMP_CLEAR(pbmp_uplink);
    pbmp_uplink = _uplink_pbmp();

    for (phy_port = 0; phy_port <= BCM5607X_PORT_MAX; phy_port++) {
        port = SOC_PORT_P2L_MAPPING(phy_port);
        if (port < 0) {
            continue; /* this user port has not been mapping in this sku */
        }
        mport = SOC_PORT_P2M_MAPPING(phy_port);
        if (IS_CPU_PORT(port)) {
            num_cpu_ports++;
            PBMP_PORT_ADD(pbmp_cpu, port);
            continue;
        }
        speed = SOC_PORT_SPEED_MAX(port);
        if (speed == 0) {
            continue; /* this user port has not been mapping in this sku */
        } else if (!PBMP_MEMBER(pbmp_all, port)) {
            continue; /* this user port has been masked out by pbmp_valid */
        }
        if (PBMP_MEMBER(pbmp_uplink, port)) {
            number_of_uplink_ports++;
            if (mport >= MMU_64Q_PPORT_BASE) {
                PBMP_PORT_ADD(pbmp_uplink_6kxq, port);
            } else {
                PBMP_PORT_ADD(pbmp_uplink_2kxq, port);
            }
        } else {
            number_of_downlink_ports++;
            if (mport >= MMU_64Q_PPORT_BASE) {
                PBMP_PORT_ADD(pbmp_downlink_6kxq, port);
            } else {
                PBMP_PORT_ADD(pbmp_downlink_2kxq, port);
            }
        }
    }


    /* Parameters */
    total_cell_memory_for_admission = 14 * 1024;
    num_of_used_memory_banks = 8;
    reserved_for_cfap = 65 * 2 + num_of_used_memory_banks * 4;
    cfapfullsetpoint = 16 * 1024- reserved_for_cfap;
    standard_jumbo_frame = 9216;
    cell_size = 144;
    ethernet_mtu_cell = fl_ceiling_func(1.5 * 1024, cell_size);
    max_packet_size = fl_ceiling_func(12 * 1024, cell_size);
    standard_jumbo_frame_cell =
          fl_ceiling_func(standard_jumbo_frame, cell_size);
    total_cell_memory = total_cell_memory_for_admission;
    total_advertised_cell_memory = total_cell_memory;
    mmu_xoff_pkt_threshold_uplink_ports = total_advertised_cell_memory;
    xoff_packet_thresholds_per_port_uplink_port = mmu_xoff_pkt_threshold_uplink_ports;
    xoff_cell_threshold_uplink_ports = total_advertised_cell_memory;
    xoff_cell_threshold_downlink_ports = total_advertised_cell_memory;
    discard_limit_per_port_pg_uplink_port = total_advertised_cell_memory;
    discard_limit_per_port_pg_downlink_port = max_packet_size;
    egress_xq_min_reserve_lossy_uplink_ports = ethernet_mtu_cell;
    num_lossy_queues_sp_traffic = 3;
    num_lossy_queues_wdrr_traffic = 5;
    egress_xq_min_reserve_lossless_uplink_ports = 0;
    egress_xq_min_reserve_lossless_downlink_ports = 0;
    num_lossless_queues = 0;
    reserved_xqs_per_uplink_port =
        egress_xq_min_reserve_lossy_uplink_ports * (num_lossy_queues_sp_traffic +
                                                    num_lossy_queues_wdrr_traffic)
        + egress_xq_min_reserve_lossless_uplink_ports * num_lossless_queues;
    shared_xqs_per_6kxq_uplink_port = 6 * 1024 - reserved_xqs_per_uplink_port;
    shared_xqs_per_2kxq_uplink_port = 2 * 1024 - reserved_xqs_per_uplink_port;
    reserved_xqs_per_downlink_port =
        egress_xq_min_reserve_lossy_uplink_ports * (num_lossy_queues_sp_traffic +
                                                    num_lossy_queues_wdrr_traffic)
        + egress_xq_min_reserve_lossless_downlink_ports * num_lossless_queues;
    shared_xqs_per_6kxq_downlink_port = 6 * 1024 - reserved_xqs_per_downlink_port;
    shared_xqs_per_2kxq_downlink_port = 2 * 1024 - reserved_xqs_per_downlink_port;
    skidmarker = 7;
    prefetch = 68;
    numxqs_6kxq_uplink_ports = 6 * 1024;
    numxqs_2kxq_uplink_ports = 2 * 1024;
    numxqs_6kxq_downlink_ports = 6 * 1024;
    numxqs_2kxq_downlink_ports = 2 * 1024;
    port_shared_limit_cpu_ports = max_packet_size;
    buffer_space_for_upstream_traffic =
        discard_limit_per_port_pg_downlink_port * number_of_downlink_ports;
    wrr_reserve = ethernet_mtu_cell * 4;
    total_reserved_for_uplink_ports = 0 * number_of_uplink_ports * num_lossy_queues_sp_traffic
        + number_of_uplink_ports * 0 * 0;
    total_reserved_for_downlink_ports = number_of_downlink_ports
        * (port_shared_limit_cpu_ports * 2) * num_lossy_queues_sp_traffic
        + number_of_downlink_ports * wrr_reserve * num_lossy_queues_wdrr_traffic
        + number_of_downlink_ports * 0 * num_lossless_queues;
    total_reserved_for_cpu_port = 1 * ethernet_mtu_cell * 8;
    total_reserved = total_reserved_for_uplink_ports
                     + total_reserved_for_downlink_ports
                     + total_reserved_for_cpu_port;
    shared_space_cells = xoff_cell_threshold_uplink_ports - total_reserved;


    /* Device Wide Registers */
    READ_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);
    CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_SET(cfapfullthreshold,
        cfapfullsetpoint);
    CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_SET(cfapfullthreshold,
                            cfapfullsetpoint - (standard_jumbo_frame_cell * 2));
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    READ_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);
    GBLLIMITSETLIMITr_GBLCELLSETLIMITf_SET(gbllimitsetlimit,
                           total_cell_memory_for_admission);
    WRITE_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);

    READ_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);
    GBLLIMITRESETLIMITr_GBLCELLRESETLIMITf_SET(gbllimitresetlimit,
        total_cell_memory_for_admission);
    WRITE_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);

    /* Egress Thresholds */
    READ_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);
    TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_SET(totaldyncellsetlimit,
        shared_space_cells);
    WRITE_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);

    READ_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_SET(totaldyncellresetlimit,
        shared_space_cells - standard_jumbo_frame_cell * 2);
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
    MISCCONFIGr_CNG_DROP_ENf_SET(miscconfig, 0);
    MISCCONFIGr_DYN_XQ_ENf_SET(miscconfig, 1);
    MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_SET(miscconfig, 1);
    MISCCONFIGr_DYNAMIC_MEMORY_ENf_SET(miscconfig, 1);
    MISCCONFIGr_SKIDMARKERf_SET(miscconfig, 3);
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

    E2ECC_MODEr_SET(e2ecc_mode, 0);
    WRITE_E2ECC_MODEr(unit, e2ecc_mode);

    E2ECC_HOL_ENr_SET(e2ecc_hol_en, 0);
    WRITE_E2ECC_HOL_ENr(unit, e2ecc_hol_en);

    READ_E2ECC_MIN_TX_TIMERr(unit, e2ecc_min_tx_timer);
    E2ECC_MIN_TX_TIMERr_LGf_SET(e2ecc_min_tx_timer, 0);
    E2ECC_MIN_TX_TIMERr_TIMERf_SET(e2ecc_min_tx_timer, 0);
    WRITE_E2ECC_MIN_TX_TIMERr(unit, e2ecc_min_tx_timer);

    WRITE_E2ECC_MAX_TX_TIMERr(unit, e2ecc_max_tx_timer);
    E2ECC_MAX_TX_TIMERr_LGf_SET(e2ecc_max_tx_timer, 0);
    E2ECC_MAX_TX_TIMERr_TIMERf_SET(e2ecc_max_tx_timer, 0);
    WRITE_E2ECC_MAX_TX_TIMERr(unit, e2ecc_max_tx_timer);

    /* E2ECC_TX_ENABLE_BMPr, index 0 ~ 7 */
    for (index = 0; index <= 7; index++) {
        E2ECC_TX_ENABLE_BMPr_SET(e2ecc_tx_enable_bmp, 0);
        WRITE_E2ECC_TX_ENABLE_BMPr(unit, index, e2ecc_tx_enable_bmp);
    }
    E2ECC_TX_PORTS_NUMr_SET(e2ecc_tx_ports_num, 0);
    WRITE_E2ECC_TX_PORTS_NUMr(unit, e2ecc_tx_ports_num);

    /* port-based : 6kxq uplink ports */
    PBMP_ITER(pbmp_uplink_6kxq, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* PG_CTRL0r */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r */
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

        /* IBPPKTSETLIMITr */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit,
            xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_uplink_ports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }
        index = 7;
        READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
            total_advertised_cell_memory);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
            total_advertised_cell_memory);
        WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
                discard_limit_per_port_pg_uplink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossy_uplink_ports);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                holcosminxqcnt_qlayer);
        }
        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer,
                shared_xqs_per_6kxq_uplink_port - skidmarker - prefetch
                + egress_xq_min_reserve_lossy_uplink_ports);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                holcospktsetlimit_qlayer);
        }
        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer,
                shared_xqs_per_6kxq_uplink_port - skidmarker - prefetch);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer,
                shared_xqs_per_6kxq_uplink_port - skidmarker - prefetch
                + egress_xq_min_reserve_lossy_uplink_ports - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                holcospktresetlimit_qlayer);
        }
        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer,
                shared_xqs_per_6kxq_uplink_port - skidmarker - prefetch - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                holcospktresetlimit_qlayer);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index,
                cngcospktlimit0_qlayer);
            CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer,
                numxqs_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index,
                cngcospktlimit0_qlayer);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index,
                cngcospktlimit1_qlayer);
            CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer,
                numxqs_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index,
                cngcospktlimit1_qlayer);
        }

        /* CNGPORTPKTLIMIT0r*/
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r*/
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr*/
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport,
            shared_xqs_per_6kxq_uplink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr*/
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport,
            shared_xqs_per_6kxq_uplink_port - skidmarker - prefetch - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, 0);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, 0);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer,
                fl_ceiling_func(buffer_space_for_upstream_traffic, 1));
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer,
                fl_ceiling_func(buffer_space_for_upstream_traffic, 1) -
                ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                holcoscellmaxlimit_qlayer);
        }

        /* DYNCELLLIMITr */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit,
            buffer_space_for_upstream_traffic);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            buffer_space_for_upstream_traffic - (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
        for (index = 0; index <= 1; index++) {
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, index,
                color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, index,
                color_drop_en_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index,
                holcospktsetlimit_qgroup);
            HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(holcospktsetlimit_qgroup,
                numxqs_6kxq_uplink_ports - 1);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index,
                holcospktsetlimit_qgroup);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index,
                holcospktresetlimit_qgroup);
            HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qgroup, numxqs_6kxq_uplink_ports - 1 - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index,
                holcospktresetlimit_qgroup);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index,
                cngcospktlimit0_qgroup);
            CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qgroup, numxqs_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index,
                cngcospktlimit0_qgroup);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index,
                cngcospktlimit1_qgroup);
            CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qgroup, numxqs_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index,
            cngcospktlimit1_qgroup);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index,
                holcoscellmaxlimit_qgroup);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qgroup, total_advertised_cell_memory - 1);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qgroup,
                total_advertised_cell_memory - 1 - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index,
                holcoscellmaxlimit_qgroup);
        }

        /* COLOR_DROP_EN_QGROUPr */
        READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* SHARED_POOL_CTRL_EXT1r */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1,
            0xFFFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r */
        READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2,
            0xFFFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);

        /* E2ECC_PORT_CONFIGr */
        READ_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mport, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mport, index,
                early_holcoscellmaxlimit);
        }
    }

    /* port-based : 2kxq uplink ports */
    PBMP_ITER(pbmp_uplink_2kxq, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* PG_CTRL0r */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r */
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

        /* IBPPKTSETLIMITr */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit,
            xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_uplink_ports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }
        index = 7;
        READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
            total_advertised_cell_memory);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
            total_advertised_cell_memory);
        WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
                discard_limit_per_port_pg_uplink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossy_uplink_ports);
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }

        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit,
                shared_xqs_per_2kxq_uplink_port - skidmarker -
                prefetch + egress_xq_min_reserve_lossy_uplink_ports);
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }

        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                shared_xqs_per_2kxq_uplink_port - skidmarker -
                prefetch + egress_xq_min_reserve_lossy_uplink_ports - 1);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                numxqs_2kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                numxqs_2kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r*/
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r*/
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr*/
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport,
            shared_xqs_per_2kxq_uplink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr*/
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport,
            shared_xqs_per_2kxq_uplink_port - skidmarker - prefetch - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, 0);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, 0);
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit,
                fl_ceiling_func(buffer_space_for_upstream_traffic, 1));
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                fl_ceiling_func(buffer_space_for_upstream_traffic, 1)
                - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr*/
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit,
            buffer_space_for_upstream_traffic);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            buffer_space_for_upstream_traffic - (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_ENr */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* E2ECC_PORT_CONFIGr*/
        READ_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mport, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mport, index,
                early_holcoscellmaxlimit);
        }
    }

    /* port-based : 6kxq downlink ports */
    PBMP_ITER(pbmp_downlink_6kxq, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* PG_CTRL0r */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r */
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

        /* IBPPKTSETLIMITr */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit,
            xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_uplink_ports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_uplink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }
        index = 7;
        READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
            total_advertised_cell_memory);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
            total_advertised_cell_memory);
        WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
                discard_limit_per_port_pg_downlink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossy_uplink_ports);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                holcosminxqcnt_qlayer);
        }
        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer,
                shared_xqs_per_6kxq_downlink_port - skidmarker - prefetch
                + egress_xq_min_reserve_lossy_uplink_ports);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                holcospktsetlimit_qlayer);
        }
        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(holcospktsetlimit_qlayer,
                shared_xqs_per_6kxq_downlink_port - skidmarker - prefetch);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer,
                shared_xqs_per_6kxq_downlink_port - skidmarker - prefetch
                + egress_xq_min_reserve_lossy_uplink_ports - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                holcospktresetlimit_qlayer);
        }
        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer,
                shared_xqs_per_6kxq_downlink_port - skidmarker - prefetch - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                holcospktresetlimit_qlayer);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index,
                cngcospktlimit0_qlayer);
            CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(cngcospktlimit0_qlayer,
                numxqs_6kxq_downlink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index,
                cngcospktlimit0_qlayer);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index,
                cngcospktlimit1_qlayer);
            CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(cngcospktlimit1_qlayer,
                numxqs_6kxq_downlink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index,
                cngcospktlimit1_qlayer);
        }

        /* CNGPORTPKTLIMIT0r*/
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_6kxq_downlink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r*/
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_6kxq_downlink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr*/
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport,
            shared_xqs_per_6kxq_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr*/
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport,
            shared_xqs_per_6kxq_downlink_port - skidmarker - prefetch - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 4 */
        for (index = 0; index <= 4; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, wrr_reserve);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, wrr_reserve);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                lwmcoscellsetlimit_qlayer);
        }
        /* LWMCOSCELLSETLIMIT_QLAYERr, index 5 ~ 7 */
        for (index = 5; index <= 7; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, max_packet_size * 2);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, max_packet_size * 2);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                lwmcoscellsetlimit_qlayer);
        }
        /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, 0);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, 0);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                lwmcoscellsetlimit_qlayer);
        }
        port_shared_limit_downlink_ports = fl_floor_func(
            (shared_space_cells -
             buffer_space_for_upstream_traffic-max_packet_size),
            number_of_downlink_ports);
        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 4 */
        for (index = 0; index <= 4; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer,
                fl_ceiling_func((port_shared_limit_downlink_ports
                                 + wrr_reserve), 1));
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer,
                fl_ceiling_func((port_shared_limit_downlink_ports
                                 + wrr_reserve), 1) - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                holcoscellmaxlimit_qlayer);
        }
        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 5 ~ 7 */
        for (index = 5; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer,
                fl_ceiling_func((port_shared_limit_downlink_ports
                                 + max_packet_size * 2), 1));
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer,
                fl_ceiling_func((port_shared_limit_downlink_ports
                                 + max_packet_size * 2), 1) - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                holcoscellmaxlimit_qlayer);
        }
        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer,
                fl_ceiling_func(port_shared_limit_downlink_ports, 1));
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer,
                fl_ceiling_func(port_shared_limit_downlink_ports, 1)-
                ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                holcoscellmaxlimit_qlayer);
        }

        /* DYNCELLLIMITr */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit,
            port_shared_limit_downlink_ports);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            port_shared_limit_downlink_ports - (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
        for (index = 0; index <= 1; index++) {
            READ_COLOR_DROP_EN_QLAYERr(unit, mport, index,
                color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mport, index,
                color_drop_en_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index,
                holcospktsetlimit_qgroup);
            HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(holcospktsetlimit_qgroup,
                numxqs_6kxq_downlink_ports - 1);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index,
                holcospktsetlimit_qgroup);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index,
                holcospktresetlimit_qgroup);
            HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qgroup, numxqs_6kxq_downlink_ports - 1 - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index,
                holcospktresetlimit_qgroup);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index,
                cngcospktlimit0_qgroup);
            CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qgroup, numxqs_6kxq_downlink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index,
                cngcospktlimit0_qgroup);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index,
                cngcospktlimit1_qgroup);
            CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qgroup, numxqs_6kxq_downlink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index,
            cngcospktlimit1_qgroup);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index,
                holcoscellmaxlimit_qgroup);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qgroup, total_advertised_cell_memory - 1);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qgroup,
                total_advertised_cell_memory - 1 - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index,
                holcoscellmaxlimit_qgroup);
        }

        /* COLOR_DROP_EN_QGROUPr */
        READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
        COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* SHARED_POOL_CTRL_EXT1r */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext1,
            0xFFFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r */
        READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
        SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl_ext2,
            0xFFFFFFFF);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);

        /* E2ECC_PORT_CONFIGr */
        READ_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mport, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mport, index,
                early_holcoscellmaxlimit);
        }
    }

    /* port-based : 2kxq downlink ports */
    PBMP_ITER(pbmp_downlink_2kxq, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* PG_CTRL0r */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r */
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

        /* IBPPKTSETLIMITr */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit,
            xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_downlink_ports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_downlink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }
        index = 7;
        READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
            total_advertised_cell_memory);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
            total_advertised_cell_memory);
        WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
                discard_limit_per_port_pg_downlink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossy_uplink_ports);
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }

        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit,
                shared_xqs_per_2kxq_downlink_port - skidmarker -
                prefetch + egress_xq_min_reserve_lossy_uplink_ports);
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }

        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                shared_xqs_per_2kxq_downlink_port - skidmarker -
                prefetch + egress_xq_min_reserve_lossy_uplink_ports - 1);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                numxqs_2kxq_downlink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                numxqs_2kxq_downlink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r*/
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_2kxq_downlink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r*/
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_2kxq_downlink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr*/
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport,
            shared_xqs_per_2kxq_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr*/
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport,
            shared_xqs_per_2kxq_downlink_port - skidmarker - prefetch - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 4 */
        for (index = 0; index <= 4; index++) {
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, wrr_reserve);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, wrr_reserve);
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }
        /* LWMCOSCELLSETLIMITr, index 5 ~ 7 */
        for (index = 5; index <= 7; index++) {
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, max_packet_size * 2);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, max_packet_size * 2);
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 4 */
        for (index = 0; index <= 4; index++) {
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit,
                fl_ceiling_func((port_shared_limit_downlink_ports + wrr_reserve), 1));
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                fl_ceiling_func((port_shared_limit_downlink_ports + wrr_reserve), 1)
                - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }
        /* HOLCOSCELLMAXLIMITr, index 5 ~ 7 */
        for (index = 5; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit,
                fl_ceiling_func((port_shared_limit_downlink_ports + max_packet_size * 2), 1));
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                fl_ceiling_func((port_shared_limit_downlink_ports + max_packet_size * 2), 1)
                - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr*/
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit,
            port_shared_limit_downlink_ports);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            port_shared_limit_downlink_ports - (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_ENr */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* E2ECC_PORT_CONFIGr*/
        READ_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mport, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mport, index,
                early_holcoscellmaxlimit);
        }
    }

    /* port-based : cpu port*/
    PBMP_ITER(pbmp_cpu, port) {
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* PG_CTRL0r */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 7);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mport, pg_ctrl0);

        /* PG_CTRL1r */
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

        /* IBPPKTSETLIMITr */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit,
            xoff_packet_thresholds_per_port_uplink_port);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);

        /* MMU_FC_RX_ENr */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, 0);
        WRITE_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, 0);
        WRITE_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_downlink_ports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_downlink_ports);
            WRITE_PGCELLLIMITr(unit, mport, index, pgcelllimit);
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit,
                discard_limit_per_port_pg_uplink_port);
            WRITE_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
        }

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                ethernet_mtu_cell);
            WRITE_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
        }

        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit,
                shared_xqs_per_2kxq_downlink_port - skidmarker -
                prefetch + ethernet_mtu_cell);
            WRITE_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
        }

        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                shared_xqs_per_2kxq_downlink_port - skidmarker -
                prefetch + ethernet_mtu_cell - 1);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mport, index, holcospktresetlimit);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                numxqs_2kxq_downlink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                numxqs_2kxq_downlink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r*/
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_2kxq_downlink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r*/
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_2kxq_downlink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);

        /* DYNXQCNTPORTr*/
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport,
            shared_xqs_per_2kxq_downlink_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mport, dynxqcntport);

        /* DYNRESETLIMPORTr*/
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport,
            shared_xqs_per_2kxq_downlink_port - skidmarker - prefetch - 2);
        WRITE_DYNRESETLIMPORTr(unit, mport, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit, ethernet_mtu_cell);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit, ethernet_mtu_cell);
            WRITE_LWMCOSCELLSETLIMITr(unit, mport, index, lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit,
                fl_ceiling_func((port_shared_limit_cpu_ports + ethernet_mtu_cell), 1));
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                fl_ceiling_func((port_shared_limit_cpu_ports + ethernet_mtu_cell), 1)
                - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mport, index, holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr*/
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit,
            port_shared_limit_cpu_ports);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            port_shared_limit_cpu_ports - (2 * ethernet_mtu_cell));
        WRITE_DYNCELLLIMITr(unit, mport, dyncelllimit);

        /* COLOR_DROP_ENr */
        READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
        WRITE_COLOR_DROP_ENr(unit, mport, color_drop_en);

        /* SHARED_POOL_CTRLr */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
        SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
        SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
        WRITE_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);

        /* E2ECC_PORT_CONFIGr*/
        READ_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mport, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mport, index,
                early_holcoscellmaxlimit);
        }

    }

    return SYS_OK;
}


STATIC int _soc_firelight_mmu_init_bump_on_wire(int unit)
{
    uint32 val;
    int port, phy_port, mmu_port;
    int index;
    pbmp_t pbmp_cpu;
    pbmp_t pbmp_uplink;
    pbmp_t pbmp_standby;
    pbmp_t pbmp_6kxq;
    pbmp_t pbmp_2kxq;
    pbmp_t pbmp_downlink;
    pbmp_t pbmp_all;
    pbmp_t pbmp_1g;
    pbmp_t pbmp_2dot5g;
    pbmp_t pbmp_10g_6kxq;
    pbmp_t pbmp_10g_2kxq;
    pbmp_t pbmp_20g_6kxq;
    pbmp_t pbmp_20g_2kxq;
    pbmp_t pbmp_25g_6kxq;
    pbmp_t pbmp_25g_2kxq;
    pbmp_t pbmp_40g_6kxq;
    pbmp_t pbmp_40g_2kxq;
    pbmp_t pbmp_50g_6kxq;
    pbmp_t pbmp_50g_2kxq;
    pbmp_t pbmp_100g_6kxq;
    pbmp_t pbmp_100g_2kxq;
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
    E2ECC_MODEr_t e2ecc_mode;
    E2ECC_HOL_ENr_t e2ecc_hol_en;
    E2ECC_MIN_TX_TIMERr_t e2ecc_min_tx_timer;
    E2ECC_MAX_TX_TIMERr_t e2ecc_max_tx_timer;
    E2ECC_TX_ENABLE_BMPr_t e2ecc_tx_enable_bmp;
    E2ECC_TX_PORTS_NUMr_t e2ecc_tx_ports_num;
    PG_CTRL0r_t pg_ctrl0;
    PG_CTRL1r_t pg_ctrl1;
    PG2TCr_t pg2tc;
    IBPPKTSETLIMITr_t ibppktsetlimit;
    MMU_FC_RX_ENr_t mmu_fc_rx_en;
    MMU_FC_TX_ENr_t mmu_fc_tx_en;
    PGCELLLIMITr_t pgcelllimit;
    PGDISCARDSETLIMITr_t pgdiscardsetlimit;
    HOLCOSMINXQCNTr_t holcosminxqcnt;
    HOLCOSPKTSETLIMITr_t holcospktsetlimit;
    HOLCOSPKTRESETLIMITr_t holcospktresetlimit;
    CNGCOSPKTLIMIT0r_t cngcospktlimit0;
    CNGCOSPKTLIMIT1r_t cngcospktlimit1;
    HOLCOSMINXQCNT_QLAYERr_t holcosminxqcnt_qlayer;
    HOLCOSPKTSETLIMIT_QLAYERr_t holcospktsetlimit_qlayer;
    HOLCOSPKTRESETLIMIT_QLAYERr_t holcospktresetlimit_qlayer;
    CNGCOSPKTLIMIT0_QLAYERr_t cngcospktlimit0_qlayer;
    CNGCOSPKTLIMIT1_QLAYERr_t cngcospktlimit1_qlayer;
    CNGPORTPKTLIMIT0r_t cngportpktlimit0;
    CNGPORTPKTLIMIT1r_t cngportpktlimit1;
    DYNXQCNTPORTr_t dynxqcntport;
    DYNRESETLIMPORTr_t dynresetlimport;
    LWMCOSCELLSETLIMITr_t lwmcoscellsetlimit;
    HOLCOSCELLMAXLIMITr_t holcoscellmaxlimit;
    LWMCOSCELLSETLIMIT_QLAYERr_t lwmcoscellsetlimit_qlayer;
    HOLCOSCELLMAXLIMIT_QLAYERr_t holcoscellmaxlimit_qlayer;
    DYNCELLLIMITr_t dyncelllimit;
    COLOR_DROP_EN_QLAYERr_t color_drop_en_qlayer;
    COLOR_DROP_ENr_t color_drop_en;
    HOLCOSPKTSETLIMIT_QGROUPr_t holcospktsetlimit_qgroup;
    HOLCOSPKTRESETLIMIT_QGROUPr_t holcospktresetlimit_qgroup;
    CNGCOSPKTLIMIT0_QGROUPr_t cngcospktlimit0_qgroup;
    CNGCOSPKTLIMIT1_QGROUPr_t cngcospktlimit1_qgroup;
    HOLCOSCELLMAXLIMIT_QGROUPr_t holcoscellmaxlimit_qgroup;
    COLOR_DROP_EN_QGROUPr_t color_drop_en_qgroup;
    SHARED_POOL_CTRLr_t shared_pool_ctrl;
    SHARED_POOL_CTRL_EXT1r_t shared_pool_ctrl_ext1;
    SHARED_POOL_CTRL_EXT2r_t shared_pool_ctrl_ext2;
    E2ECC_PORT_CONFIGr_t e2ecc_port_cfg;
    EARLY_DYNCELLLIMITr_t early_dyncelllimit;
    EARLY_HOLCOSCELLMAXLIMITr_t early_holcoscellmaxlimit;
    int speed;
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
    int num_1g_ports_uplink_ports;
    int num_2dot5g_ports_uplink_ports;
    int num_10g_ports_uplink_ports;
    int num_20g_ports_uplink_ports;
    int num_25g_ports_uplink_ports;
    int num_40g_ports_uplink_ports;
    int num_50g_ports_uplink_ports;
    int num_100g_ports_uplink_ports;
    int num_1g_ports_downlink_ports;
    int num_2dot5g_ports_downlink_ports;
    int num_10g_ports_downlink_ports;
    int num_20g_ports_downlink_ports;
    int num_25g_ports_downlink_ports;
    int num_40g_ports_downlink_ports;
    int num_50g_ports_downlink_ports;
    int num_100g_ports_downlink_ports;
    int queue_port_limit_ratio;
    int egress_queue_min_reserve_lossy;
    int egress_queue_min_reserve_lossless_1g_port;
    int egress_queue_min_reserve_lossless_2dot5g_port;
    int egress_queue_min_reserve_lossless_10g_port;
    int egress_queue_min_reserve_lossless_20g_port;
    int egress_queue_min_reserve_lossless_25g_port;
    int egress_queue_min_reserve_lossless_40g_port;
    int egress_queue_min_reserve_lossless_50g_port;
    int egress_queue_min_reserve_lossless_100g_port;
    int egress_queue_min_reserve_cpu_ports;
    int egress_xq_min_reserve_lossy;
    int egress_xq_min_reserve_lossless_1g_port;
    int egress_xq_min_reserve_lossless_2dot5g_port;
    int egress_xq_min_reserve_lossless_10g_port;
    int egress_xq_min_reserve_lossless_20g_port;
    int egress_xq_min_reserve_lossless_25g_port;
    int egress_xq_min_reserve_lossless_40g_port;
    int egress_xq_min_reserve_lossless_50g_port;
    int egress_xq_min_reserve_lossless_100g_port;
    int num_lossy_queues;
    int num_lossless_queues;
    int mmu_xoff_pkt_threshold_uplink_ports;
    int mmu_xoff_pkt_threshold_downlink_ports;
    int mmu_xoff_cell_threshold_1g_port;
    int mmu_xoff_cell_threshold_2dot5g_port;
    int mmu_xoff_cell_threshold_10g_port;
    int mmu_xoff_cell_threshold_20g_port;
    int mmu_xoff_cell_threshold_25g_port;
    int mmu_xoff_cell_threshold_40g_port;
    int mmu_xoff_cell_threshold_50g_port;
    int mmu_xoff_cell_threshold_100g_port;
    int num_cpu_queues;
    int num_cpu_ports;
    int numxqs_per_6kxq_uplink_ports;
    int numxqs_per_2kxq_uplink_ports;
    int headroom_for_1g_port;
    int headroom_for_2dot5g_port;
    int headroom_for_10g_port;
    int headroom_for_20g_port;
    int headroom_for_25g_port;
    int headroom_for_40g_port;
    int headroom_for_50g_port;
    int headroom_for_100g_port;
    int total_required_pg_headroom;
    int xoff_cell_thresholds_per_port_1g_port;
    int xoff_cell_thresholds_per_port_2dot5g_port;
    int xoff_cell_thresholds_per_port_10g_port;
    int xoff_cell_thresholds_per_port_20g_port;
    int xoff_cell_thresholds_per_port_25g_port;
    int xoff_cell_thresholds_per_port_40g_port;
    int xoff_cell_thresholds_per_port_50g_port;
    int xoff_cell_thresholds_per_port_100g_port;
    int xoff_cell_threshold_downlink_ports;
    int xoff_packet_thresholds_per_port_uplink_port;
    int xoff_packet_thresholds_per_port_downlink_port;
    int discard_limit_per_port_pg_uplink_port;
    int discard_limit_per_port_pg_downlink_port;
    int discard_limit_per_port_pg_1g_port;
    int discard_limit_per_port_pg_2dot5g_port;
    int discard_limit_per_port_pg_10g_port;
    int discard_limit_per_port_pg_20g_port;
    int discard_limit_per_port_pg_25g_port;
    int discard_limit_per_port_pg_40g_port;
    int discard_limit_per_port_pg_50g_port;
    int discard_limit_per_port_pg_100g_port;
    int total_reserved_cells_for_uplink_ports;
    int total_reserved_cells_for_downlink_ports;
    int total_reserved_cells_for_cpu_port;
    int total_reserved;
    int shared_space_cells;
    int reserved_xqs_per_1g_port;
    int reserved_xqs_per_2dot5g_port;
    int reserved_xqs_per_10g_port;
    int reserved_xqs_per_20g_port;
    int reserved_xqs_per_25g_port;
    int reserved_xqs_per_40g_port;
    int reserved_xqs_per_50g_port;
    int reserved_xqs_per_100g_port;
    int reserved_xqs_per_cpu_port;
    int shared_xqs_per_2kxq_port_1g;
    int shared_xqs_per_2kxq_port_2dot5g;
    int shared_xqs_per_2kxq_port_10g;
    int shared_xqs_per_2kxq_port_20g;
    int shared_xqs_per_2kxq_port_25g;
    int shared_xqs_per_2kxq_port_40g;
    int shared_xqs_per_2kxq_port_50g;
    int shared_xqs_per_2kxq_port_100g;
    int shared_xqs_per_6kxq_port_10g;
    int shared_xqs_per_6kxq_port_20g;
    int shared_xqs_per_6kxq_port_25g;
    int shared_xqs_per_6kxq_port_40g;
    int shared_xqs_per_6kxq_port_50g;
    int shared_xqs_per_6kxq_port_100g;
    int shared_xqs_cpu_port;

    /* setup port bitmap according the port max speed for lossy
     *   TSC/TSCF    : uplink port
     *   QGMII/SGMII : downlink port
     */
    num_cpu_ports = 0;
    number_of_uplink_ports = 0;
    number_of_downlink_ports = 0;
    num_1g_ports_uplink_ports = 0;
    num_2dot5g_ports_uplink_ports = 0;
    num_10g_ports_uplink_ports = 0;
    num_20g_ports_uplink_ports = 0;
    num_25g_ports_uplink_ports = 0;
    num_40g_ports_uplink_ports = 0;
    num_50g_ports_uplink_ports = 0;
    num_100g_ports_uplink_ports = 0;
    num_1g_ports_downlink_ports = 0;
    num_2dot5g_ports_downlink_ports = 0;
    num_10g_ports_downlink_ports = 0;
    num_20g_ports_downlink_ports = 0;
    num_25g_ports_downlink_ports = 0;
    num_40g_ports_downlink_ports = 0;
    num_50g_ports_downlink_ports = 0;
    num_100g_ports_downlink_ports = 0;
    PBMP_CLEAR(pbmp_cpu);
    PBMP_CLEAR(pbmp_uplink);
    PBMP_CLEAR(pbmp_standby);
    PBMP_CLEAR(pbmp_6kxq);
    PBMP_CLEAR(pbmp_2kxq);
    PBMP_CLEAR(pbmp_downlink);
    PBMP_CLEAR(pbmp_all);
    PBMP_CLEAR(pbmp_1g);
    PBMP_CLEAR(pbmp_2dot5g);
    PBMP_CLEAR(pbmp_10g_6kxq);
    PBMP_CLEAR(pbmp_10g_2kxq);
    PBMP_CLEAR(pbmp_20g_6kxq);
    PBMP_CLEAR(pbmp_20g_2kxq);
    PBMP_CLEAR(pbmp_25g_6kxq);
    PBMP_CLEAR(pbmp_25g_2kxq);
    PBMP_CLEAR(pbmp_40g_6kxq);
    PBMP_CLEAR(pbmp_40g_2kxq);
    PBMP_CLEAR(pbmp_50g_6kxq);
    PBMP_CLEAR(pbmp_50g_2kxq);
    PBMP_CLEAR(pbmp_100g_6kxq);
    PBMP_CLEAR(pbmp_100g_2kxq);
    PBMP_ASSIGN(pbmp_all, BCM5607X_ALL_PORTS_MASK);
    pbmp_uplink = _uplink_pbmp();
    pbmp_standby = _standby_pbmp();

    sal_printf("BumpOnWire MMU....\n");

    for (phy_port = 0; phy_port < BCM5607X_PORT_MAX; phy_port++) {
        port = SOC_PORT_P2L_MAPPING(phy_port);
        if (port < 0) {
            continue;
        }
        if (IS_CPU_PORT(port)) {
            num_cpu_ports++;
            PBMP_PORT_ADD(pbmp_cpu, port);
            continue;
        }
        speed = SOC_PORT_SPEED_MAX(port);
        mmu_port = SOC_PORT_P2M_MAPPING(phy_port);
        if (speed <= 0) {
            continue; /* this user port has not been mapping in this sku */
        } else if (!PBMP_MEMBER(pbmp_all, port)) {
            continue; /* this user port has been masked out by pbmp_valid */
        }

        if (PBMP_MEMBER(pbmp_uplink, port)) {
            if (PBMP_MEMBER(pbmp_standby, port)) {
                PBMP_PORT_REMOVE(pbmp_uplink, port);
            } else {
                number_of_uplink_ports++;
            }
        } else {
            PBMP_PORT_ADD(pbmp_downlink, port);
            number_of_downlink_ports++;
        }
        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            PBMP_PORT_ADD(pbmp_6kxq, port);
        } else {
            PBMP_PORT_ADD(pbmp_2kxq, port);
        }
    }

    if (number_of_uplink_ports != number_of_downlink_ports) {
        sal_printf("Num of Uplink and Downlink ports should be equal\n");
        return SYS_ERR_PARAMETER;
    }

    PBMP_OR(pbmp_all, pbmp_cpu);
    PBMP_OR(pbmp_all, pbmp_uplink);
    PBMP_OR(pbmp_all, pbmp_downlink);

    PBMP_ITER(pbmp_all, port) {
        speed = SOC_PORT_SPEED_INIT(port);
        if (speed > 100000) {
            sal_printf("for uplink port %d,\
                     the max speed cannot exceed 100G (value=%d)\n",
                     port, speed);
            return SOC_E_PARAM;
        } else if (speed > 50000) {
            if (PBMP_MEMBER(pbmp_6kxq, port)) {
                PBMP_PORT_ADD(pbmp_100g_6kxq, port);
            } else if (PBMP_MEMBER(pbmp_2kxq, port)) {
                PBMP_PORT_ADD(pbmp_100g_2kxq, port);
            }
            if (PBMP_MEMBER(pbmp_uplink, port)) {
                num_100g_ports_uplink_ports++;
            } else if (PBMP_MEMBER(pbmp_downlink, port)) {
                num_100g_ports_downlink_ports++;
            }
        } else if (speed > 40000) {
            if (PBMP_MEMBER(pbmp_6kxq, port)) {
                PBMP_PORT_ADD(pbmp_50g_6kxq, port);
            } else if (PBMP_MEMBER(pbmp_2kxq, port)) {
                PBMP_PORT_ADD(pbmp_50g_2kxq, port);
            }
            if (PBMP_MEMBER(pbmp_uplink, port)) {
                num_50g_ports_uplink_ports++;
            } else if (PBMP_MEMBER(pbmp_downlink, port)) {
                num_50g_ports_downlink_ports++;
            }
        } else if (speed > 25000) {
            if (PBMP_MEMBER(pbmp_6kxq, port)) {
                PBMP_PORT_ADD(pbmp_40g_6kxq, port);
            } else if (PBMP_MEMBER(pbmp_2kxq, port)) {
                PBMP_PORT_ADD(pbmp_40g_2kxq, port);
            }
            if (PBMP_MEMBER(pbmp_uplink, port)) {
                num_40g_ports_uplink_ports++;
            } else if (PBMP_MEMBER(pbmp_downlink, port)) {
                num_40g_ports_downlink_ports++;
            }
        } else if (speed > 20000) {
            if (PBMP_MEMBER(pbmp_6kxq, port)) {
                PBMP_PORT_ADD(pbmp_25g_6kxq, port);
            } else if (PBMP_MEMBER(pbmp_2kxq, port)) {
                PBMP_PORT_ADD(pbmp_25g_2kxq, port);
            }
            if (PBMP_MEMBER(pbmp_uplink, port)) {
                num_25g_ports_uplink_ports++;
            } else if (PBMP_MEMBER(pbmp_downlink, port)) {
                num_25g_ports_downlink_ports++;
            }
        } else if (speed > 10000) {
            if (PBMP_MEMBER(pbmp_6kxq, port)) {
                PBMP_PORT_ADD(pbmp_20g_6kxq, port);
            } else if (PBMP_MEMBER(pbmp_2kxq, port)) {
                PBMP_PORT_ADD(pbmp_20g_2kxq, port);
            }
            if (PBMP_MEMBER(pbmp_uplink, port)) {
                num_20g_ports_uplink_ports++;
            } else if (PBMP_MEMBER(pbmp_downlink, port)) {
                num_20g_ports_downlink_ports++;
            }
        } else if (speed > 2500) {
            if (PBMP_MEMBER(pbmp_6kxq, port)) {
                PBMP_PORT_ADD(pbmp_10g_6kxq, port);
            } else if (PBMP_MEMBER(pbmp_2kxq, port)) {
                PBMP_PORT_ADD(pbmp_10g_2kxq, port);
            }
            if (PBMP_MEMBER(pbmp_uplink, port)) {
                num_10g_ports_uplink_ports++;
            } else if (PBMP_MEMBER(pbmp_downlink, port)) {
                num_10g_ports_downlink_ports++;
            }
        } else if (speed > 1000) {
            PBMP_PORT_ADD(pbmp_2dot5g, port);
            if (PBMP_MEMBER(pbmp_uplink, port)) {
                num_2dot5g_ports_uplink_ports++;
            } else if (PBMP_MEMBER(pbmp_downlink, port)) {
                num_2dot5g_ports_downlink_ports++;
            }
        } else {
            PBMP_PORT_ADD(pbmp_1g, port);
            if (PBMP_MEMBER(pbmp_uplink, port)) {
                num_1g_ports_uplink_ports++;
            } else if (PBMP_MEMBER(pbmp_downlink, port)) {
                num_1g_ports_downlink_ports++;
            }
        }
    }

    standard_jumbo_frame = 9216;
    cell_size = 144;
    standard_jumbo_frame_cell = fl_ceiling_func(standard_jumbo_frame, cell_size);
    ethernet_mtu_cell = fl_ceiling_func(15 * 1024 / 10, cell_size);
    total_cell_memory_for_admission = 14 * 1024;
    total_physical_memory = 16 * 1024;
    number_of_used_memory_banks = 8;
    reserved_for_cfap = 65 * 2 + number_of_used_memory_banks * 4;
    cfapfullsetpoint = total_physical_memory - reserved_for_cfap;
    total_cell_memory = total_cell_memory_for_admission;
    total_advertised_cell_memory = total_cell_memory;
    skidmarker = 7;
    prefetch = 64 + 4;
    queue_port_limit_ratio = 1;
    egress_queue_min_reserve_lossy = 0;
    egress_queue_min_reserve_cpu_ports = ethernet_mtu_cell;
    num_lossless_queues = 1;
    num_lossy_queues = 7;
    num_cpu_queues = 8;
    numxqs_per_6kxq_uplink_ports = 6 * 1024;
    numxqs_per_2kxq_uplink_ports = 2 * 1024;
    headroom_for_1g_port = 118;
    headroom_for_2dot5g_port = 120;
    headroom_for_10g_port = 160;
    headroom_for_20g_port = 198;
    headroom_for_25g_port = 256;
    headroom_for_40g_port = 274;
    headroom_for_50g_port = 352;
    headroom_for_100g_port = 574;
    mmu_xoff_pkt_threshold_uplink_ports = total_advertised_cell_memory;
    mmu_xoff_pkt_threshold_downlink_ports = total_advertised_cell_memory;
    xoff_cell_threshold_downlink_ports = total_advertised_cell_memory;
    xoff_packet_thresholds_per_port_uplink_port
          = mmu_xoff_pkt_threshold_uplink_ports;
    xoff_packet_thresholds_per_port_downlink_port
          = mmu_xoff_pkt_threshold_downlink_ports;
    discard_limit_per_port_pg_uplink_port = total_advertised_cell_memory;
    discard_limit_per_port_pg_downlink_port = total_advertised_cell_memory;
    total_required_pg_headroom
        = headroom_for_1g_port *
          (num_1g_ports_uplink_ports + num_1g_ports_downlink_ports) +
          headroom_for_2dot5g_port *
          (num_2dot5g_ports_uplink_ports + num_2dot5g_ports_downlink_ports) +
          headroom_for_10g_port *
          (num_10g_ports_uplink_ports + num_10g_ports_downlink_ports) +
          headroom_for_20g_port *
          (num_20g_ports_uplink_ports + num_20g_ports_downlink_ports) +
          headroom_for_25g_port *
          (num_25g_ports_uplink_ports + num_25g_ports_downlink_ports) +
          headroom_for_40g_port *
          (num_40g_ports_uplink_ports + num_40g_ports_downlink_ports) +
          headroom_for_50g_port *
          (num_50g_ports_uplink_ports + num_50g_ports_downlink_ports) +
          headroom_for_100g_port *
          (num_100g_ports_uplink_ports + num_100g_ports_downlink_ports);
    if ((num_1g_ports_uplink_ports + num_1g_ports_downlink_ports) == 0) {
        mmu_xoff_cell_threshold_1g_port = headroom_for_1g_port;
    } else {
        val = (total_cell_memory_for_admission - total_required_pg_headroom -
              (number_of_uplink_ports + number_of_downlink_ports) * 72 - 88) *
              headroom_for_1g_port / headroom_for_100g_port;
        if (headroom_for_1g_port < val) {
            val = headroom_for_1g_port;
        }
        mmu_xoff_cell_threshold_1g_port = fl_floor_func(val, 1);
    }
    if ((num_2dot5g_ports_uplink_ports + num_2dot5g_ports_downlink_ports) == 0) {
        mmu_xoff_cell_threshold_2dot5g_port = headroom_for_2dot5g_port;
    } else {
        val = (total_cell_memory_for_admission - total_required_pg_headroom -
              (number_of_uplink_ports + number_of_downlink_ports) * 72 - 88) *
              headroom_for_2dot5g_port / total_required_pg_headroom;
        if (headroom_for_2dot5g_port < val) {
            val = headroom_for_2dot5g_port;
        }
        mmu_xoff_cell_threshold_2dot5g_port = fl_floor_func(val, 1);
    }
    if ((num_10g_ports_uplink_ports + num_10g_ports_downlink_ports) == 0) {
        mmu_xoff_cell_threshold_10g_port = headroom_for_10g_port;
    } else {
        val = (total_cell_memory_for_admission - total_required_pg_headroom -
              (number_of_uplink_ports + number_of_downlink_ports) * 72 - 88) *
              headroom_for_10g_port / total_required_pg_headroom;
        if (headroom_for_10g_port < val) {
            val = headroom_for_10g_port;
        }
        mmu_xoff_cell_threshold_10g_port = fl_floor_func(val, 1);
    }
    if ((num_20g_ports_uplink_ports + num_20g_ports_downlink_ports) == 0) {
        mmu_xoff_cell_threshold_20g_port = headroom_for_20g_port;
    } else {
        val = (total_cell_memory_for_admission - total_required_pg_headroom -
              (number_of_uplink_ports + number_of_downlink_ports) * 72 - 88) *
              headroom_for_20g_port / total_required_pg_headroom;
        if (headroom_for_20g_port < val) {
            val = headroom_for_20g_port;
        }
        mmu_xoff_cell_threshold_20g_port = fl_floor_func(val, 1);
    }
    if ((num_25g_ports_uplink_ports + num_25g_ports_downlink_ports) == 0) {
        mmu_xoff_cell_threshold_25g_port = headroom_for_25g_port;
    } else {
        val = (total_cell_memory_for_admission - total_required_pg_headroom -
              (number_of_uplink_ports + number_of_downlink_ports) * 72 - 88) *
              headroom_for_25g_port / total_required_pg_headroom;
        if (headroom_for_25g_port < val) {
            val = headroom_for_25g_port;
        }
        mmu_xoff_cell_threshold_25g_port = fl_floor_func(val, 1);
    }
    if ((num_40g_ports_uplink_ports + num_40g_ports_downlink_ports) == 0) {
        mmu_xoff_cell_threshold_40g_port = headroom_for_40g_port;
    } else {
        val = (total_cell_memory_for_admission - total_required_pg_headroom -
              (number_of_uplink_ports + number_of_downlink_ports) * 72 - 88) *
              headroom_for_40g_port / total_required_pg_headroom;
        if (headroom_for_40g_port < val) {
            val = headroom_for_40g_port;
        }
        mmu_xoff_cell_threshold_40g_port = fl_floor_func(val, 1);
    }
    if ((num_50g_ports_uplink_ports + num_50g_ports_downlink_ports) == 0) {
        mmu_xoff_cell_threshold_50g_port = headroom_for_50g_port;
    } else {
        val = (total_cell_memory_for_admission - total_required_pg_headroom -
              (number_of_uplink_ports + number_of_downlink_ports) * 72 - 88) *
              headroom_for_50g_port / total_required_pg_headroom;
        if (headroom_for_50g_port < val) {
            val = headroom_for_50g_port;
        }
        mmu_xoff_cell_threshold_50g_port = fl_floor_func(val, 1);
    }
    if ((num_100g_ports_uplink_ports + num_100g_ports_downlink_ports) == 0) {
        mmu_xoff_cell_threshold_100g_port = headroom_for_100g_port;
    } else {
        val = (total_cell_memory_for_admission - total_required_pg_headroom -
              (number_of_uplink_ports + number_of_downlink_ports) * 72 - 88) *
              headroom_for_100g_port / total_required_pg_headroom;
        if (headroom_for_100g_port < val) {
            val = headroom_for_100g_port;
        }
        mmu_xoff_cell_threshold_100g_port = fl_floor_func(val, 1);
    }
    xoff_cell_thresholds_per_port_1g_port
          = mmu_xoff_cell_threshold_1g_port;
    xoff_cell_thresholds_per_port_2dot5g_port
          = mmu_xoff_cell_threshold_2dot5g_port;
    xoff_cell_thresholds_per_port_10g_port
          = mmu_xoff_cell_threshold_10g_port;
    xoff_cell_thresholds_per_port_20g_port
          = mmu_xoff_cell_threshold_20g_port;
    xoff_cell_thresholds_per_port_25g_port
          = mmu_xoff_cell_threshold_25g_port;
    xoff_cell_thresholds_per_port_40g_port
          = mmu_xoff_cell_threshold_40g_port;
    xoff_cell_thresholds_per_port_50g_port
          = mmu_xoff_cell_threshold_50g_port;
    xoff_cell_thresholds_per_port_100g_port
          = mmu_xoff_cell_threshold_100g_port;
    discard_limit_per_port_pg_1g_port
        = xoff_cell_thresholds_per_port_1g_port + headroom_for_1g_port;
    discard_limit_per_port_pg_2dot5g_port
        = xoff_cell_thresholds_per_port_2dot5g_port
          + headroom_for_2dot5g_port;
    discard_limit_per_port_pg_10g_port
        = xoff_cell_thresholds_per_port_10g_port
          + headroom_for_10g_port;
    discard_limit_per_port_pg_20g_port
        = xoff_cell_thresholds_per_port_20g_port
          + headroom_for_20g_port;
    discard_limit_per_port_pg_25g_port
        = xoff_cell_thresholds_per_port_25g_port
          + headroom_for_25g_port;
    discard_limit_per_port_pg_40g_port
        = xoff_cell_thresholds_per_port_40g_port
          + headroom_for_40g_port;
    discard_limit_per_port_pg_50g_port
        = xoff_cell_thresholds_per_port_50g_port
          + headroom_for_50g_port;
    discard_limit_per_port_pg_100g_port
        = xoff_cell_thresholds_per_port_100g_port
          + headroom_for_100g_port;
    egress_xq_min_reserve_lossy = 0;
    egress_xq_min_reserve_lossless_1g_port
        = mmu_xoff_cell_threshold_1g_port + headroom_for_1g_port;
    egress_queue_min_reserve_lossless_1g_port
        = egress_xq_min_reserve_lossless_1g_port;
    egress_xq_min_reserve_lossless_2dot5g_port
        = mmu_xoff_cell_threshold_2dot5g_port + headroom_for_2dot5g_port;
    egress_queue_min_reserve_lossless_2dot5g_port
        = egress_xq_min_reserve_lossless_2dot5g_port;
    egress_xq_min_reserve_lossless_10g_port
        = mmu_xoff_cell_threshold_10g_port + headroom_for_10g_port;
    egress_queue_min_reserve_lossless_10g_port
        = egress_xq_min_reserve_lossless_10g_port;
    egress_xq_min_reserve_lossless_20g_port
        = mmu_xoff_cell_threshold_20g_port + headroom_for_20g_port;
    egress_queue_min_reserve_lossless_20g_port
        = egress_xq_min_reserve_lossless_20g_port;
    egress_xq_min_reserve_lossless_25g_port
        = mmu_xoff_cell_threshold_25g_port + headroom_for_25g_port;
    egress_queue_min_reserve_lossless_25g_port
        = egress_xq_min_reserve_lossless_25g_port;
    egress_xq_min_reserve_lossless_40g_port
        = mmu_xoff_cell_threshold_40g_port + headroom_for_40g_port;
    egress_queue_min_reserve_lossless_40g_port
        = egress_xq_min_reserve_lossless_40g_port;
    egress_xq_min_reserve_lossless_50g_port
        = mmu_xoff_cell_threshold_50g_port + headroom_for_50g_port;
    egress_queue_min_reserve_lossless_50g_port
        = egress_xq_min_reserve_lossless_50g_port;
    egress_xq_min_reserve_lossless_100g_port
        = mmu_xoff_cell_threshold_100g_port + headroom_for_100g_port;
    egress_queue_min_reserve_lossless_100g_port
        = egress_xq_min_reserve_lossless_100g_port;
    reserved_xqs_per_1g_port
        = egress_xq_min_reserve_lossy * num_lossy_queues
          + egress_xq_min_reserve_lossless_1g_port*num_lossless_queues;
    reserved_xqs_per_2dot5g_port
        = egress_xq_min_reserve_lossy * num_lossy_queues
          + num_lossless_queues * egress_xq_min_reserve_lossless_2dot5g_port;
    reserved_xqs_per_10g_port
        = egress_xq_min_reserve_lossy * num_lossy_queues
          + egress_xq_min_reserve_lossless_10g_port*num_lossless_queues;
    reserved_xqs_per_20g_port
        = egress_xq_min_reserve_lossy * num_lossy_queues
          + egress_xq_min_reserve_lossless_20g_port*num_lossless_queues;
    reserved_xqs_per_25g_port
        = egress_xq_min_reserve_lossy * num_lossy_queues
          + egress_xq_min_reserve_lossless_25g_port*num_lossless_queues;
    reserved_xqs_per_40g_port
        = egress_xq_min_reserve_lossy * num_lossy_queues
          + egress_xq_min_reserve_lossless_40g_port*num_lossless_queues;
    reserved_xqs_per_50g_port
        = egress_xq_min_reserve_lossy * num_lossy_queues
          + egress_xq_min_reserve_lossless_50g_port*num_lossless_queues;
    reserved_xqs_per_100g_port
        = egress_xq_min_reserve_lossy * num_lossy_queues
          + egress_xq_min_reserve_lossless_100g_port*num_lossless_queues;
    reserved_xqs_per_cpu_port
        = num_lossless_queues * egress_queue_min_reserve_cpu_ports
          + num_lossy_queues * egress_queue_min_reserve_cpu_ports;
    shared_xqs_per_2kxq_port_1g
        = numxqs_per_2kxq_uplink_ports - reserved_xqs_per_1g_port;
    shared_xqs_per_2kxq_port_2dot5g
        = numxqs_per_2kxq_uplink_ports - reserved_xqs_per_2dot5g_port;
    shared_xqs_per_2kxq_port_10g
        = numxqs_per_2kxq_uplink_ports - reserved_xqs_per_10g_port;
    shared_xqs_per_2kxq_port_20g
        = numxqs_per_2kxq_uplink_ports - reserved_xqs_per_20g_port;
    shared_xqs_per_2kxq_port_25g
        = numxqs_per_2kxq_uplink_ports - reserved_xqs_per_25g_port;
    shared_xqs_per_2kxq_port_40g
        = numxqs_per_2kxq_uplink_ports - reserved_xqs_per_40g_port;
    shared_xqs_per_2kxq_port_50g
        = numxqs_per_2kxq_uplink_ports - reserved_xqs_per_50g_port;
    shared_xqs_per_2kxq_port_100g
        = numxqs_per_2kxq_uplink_ports - reserved_xqs_per_100g_port;
    shared_xqs_per_6kxq_port_10g
        = numxqs_per_6kxq_uplink_ports - reserved_xqs_per_10g_port;
    shared_xqs_per_6kxq_port_20g
        = numxqs_per_6kxq_uplink_ports - reserved_xqs_per_20g_port;
    shared_xqs_per_6kxq_port_25g
        = numxqs_per_6kxq_uplink_ports - reserved_xqs_per_25g_port;
    shared_xqs_per_6kxq_port_40g
        = numxqs_per_6kxq_uplink_ports - reserved_xqs_per_40g_port;
    shared_xqs_per_6kxq_port_50g
        = numxqs_per_6kxq_uplink_ports - reserved_xqs_per_50g_port;
    shared_xqs_per_6kxq_port_100g
        = numxqs_per_6kxq_uplink_ports - reserved_xqs_per_100g_port;
    shared_xqs_cpu_port
        = numxqs_per_2kxq_uplink_ports - reserved_xqs_per_cpu_port;
    total_reserved_cells_for_uplink_ports
        = egress_queue_min_reserve_lossy
          * number_of_uplink_ports * num_lossy_queues
          + num_1g_ports_uplink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_1g_port
          + num_2dot5g_ports_uplink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_2dot5g_port
          + num_10g_ports_uplink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_10g_port
          + num_20g_ports_uplink_ports
          * egress_queue_min_reserve_lossless_20g_port * num_lossless_queues
          + num_25g_ports_uplink_ports
          * egress_queue_min_reserve_lossless_25g_port * num_lossless_queues
          + num_40g_ports_uplink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_40g_port
          + num_50g_ports_uplink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_50g_port
          + num_100g_ports_uplink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_100g_port;
    total_reserved_cells_for_downlink_ports
        = number_of_downlink_ports * egress_queue_min_reserve_lossy
          * num_lossy_queues + num_1g_ports_downlink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_1g_port
          + num_2dot5g_ports_downlink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_2dot5g_port
          + num_10g_ports_downlink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_10g_port
          + num_20g_ports_downlink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_20g_port
          + num_25g_ports_downlink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_25g_port
          + num_40g_ports_downlink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_40g_port
          + num_50g_ports_downlink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_50g_port
          + num_100g_ports_downlink_ports * num_lossless_queues
          * egress_queue_min_reserve_lossless_100g_port;
    total_reserved_cells_for_cpu_port
        = num_cpu_ports * egress_queue_min_reserve_cpu_ports
          * num_cpu_queues;
    total_reserved
        = total_reserved_cells_for_uplink_ports
          + total_reserved_cells_for_downlink_ports
          + total_reserved_cells_for_cpu_port;
    shared_space_cells = total_advertised_cell_memory - total_reserved;

    if ((shared_space_cells * cell_size)/1024 <= 500) {
        sal_printf("Shared Pool Is Small,\
                 should be larger than 500 (value=%d)\n",
                 (shared_space_cells * cell_size)/1024);
        return SOC_E_PARAM;
    }

    /* system-based */
    READ_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);
    CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_SET(cfapfullthreshold,
        cfapfullsetpoint);
    CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_SET(cfapfullthreshold,
                            cfapfullsetpoint - (standard_jumbo_frame_cell * 2));
    WRITE_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);

    READ_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);
    GBLLIMITSETLIMITr_GBLCELLSETLIMITf_SET(gbllimitsetlimit,
        total_cell_memory_for_admission);
    WRITE_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);

    READ_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);
    GBLLIMITRESETLIMITr_GBLCELLRESETLIMITf_SET(gbllimitresetlimit,
        total_cell_memory_for_admission);
    WRITE_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);

    READ_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);
    TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_SET(totaldyncellsetlimit,
                            shared_space_cells);
    WRITE_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);

    READ_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_SET(totaldyncellresetlimit,
                            shared_space_cells - standard_jumbo_frame_cell * 2);
    WRITE_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);

    PBMP_ITER(pbmp_all, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            READ_TWO_LAYER_SCH_MODEr(unit, mmu_port, two_layer_sch_mode);
            TWO_LAYER_SCH_MODEr_SCH_MODEf_SET(two_layer_sch_mode, 0);
            WRITE_TWO_LAYER_SCH_MODEr(unit, mmu_port, two_layer_sch_mode);
        }
    }

    READ_MISCCONFIGr(unit, miscconfig);
    MISCCONFIGr_MULTIPLE_ACCOUNTING_FIX_ENf_SET(miscconfig, 1);
    MISCCONFIGr_CNG_DROP_ENf_SET(miscconfig, 0);
    MISCCONFIGr_DYN_XQ_ENf_SET(miscconfig, 1);
    MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_SET(miscconfig, 1);
    MISCCONFIGr_DYNAMIC_MEMORY_ENf_SET(miscconfig, 1);
    MISCCONFIGr_SKIDMARKERf_SET(miscconfig, 3);
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

    E2ECC_MODEr_SET(e2ecc_mode, 0);
    WRITE_E2ECC_MODEr(unit, e2ecc_mode);

    E2ECC_HOL_ENr_SET(e2ecc_hol_en, 0);
    WRITE_E2ECC_HOL_ENr(unit, e2ecc_hol_en);

    READ_E2ECC_MIN_TX_TIMERr(unit, e2ecc_min_tx_timer);
    E2ECC_MIN_TX_TIMERr_LGf_SET(e2ecc_min_tx_timer, 0);
    E2ECC_MIN_TX_TIMERr_TIMERf_SET(e2ecc_min_tx_timer, 0);
    WRITE_E2ECC_MIN_TX_TIMERr(unit, e2ecc_min_tx_timer);

    WRITE_E2ECC_MAX_TX_TIMERr(unit, e2ecc_max_tx_timer);
    E2ECC_MAX_TX_TIMERr_LGf_SET(e2ecc_max_tx_timer, 0);
    E2ECC_MAX_TX_TIMERr_TIMERf_SET(e2ecc_max_tx_timer, 0);
    WRITE_E2ECC_MAX_TX_TIMERr(unit, e2ecc_max_tx_timer);

    /* E2ECC_TX_ENABLE_BMPr, index 0 ~ 7 */
    for (index = 0; index <= 7; index++) {
        E2ECC_TX_ENABLE_BMPr_SET(e2ecc_tx_enable_bmp, 0);
        WRITE_E2ECC_TX_ENABLE_BMPr(unit, index, e2ecc_tx_enable_bmp);
    }
    E2ECC_TX_PORTS_NUMr_SET(e2ecc_tx_ports_num, 0);
    WRITE_E2ECC_TX_PORTS_NUMr(unit, e2ecc_tx_ports_num);

    /* port-based : uplink downlink*/
    PBMP_ITER(pbmp_all, port) {
        speed = SOC_PORT_SPEED_INIT(port);
        /* PG_CTRL0r, index 0 */
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
        val = (IS_CPU_PORT(port)) ? 0x0 : 0x80;

        READ_PG_CTRL0r(unit, mmu_port, pg_ctrl0);
        PG_CTRL0r_PPFC_PG_ENf_SET(pg_ctrl0, val);
        PG_CTRL0r_PRI0_GRPf_SET(pg_ctrl0, 0);
        PG_CTRL0r_PRI1_GRPf_SET(pg_ctrl0, 1);
        PG_CTRL0r_PRI2_GRPf_SET(pg_ctrl0, 2);
        PG_CTRL0r_PRI3_GRPf_SET(pg_ctrl0, 3);
        PG_CTRL0r_PRI4_GRPf_SET(pg_ctrl0, 4);
        PG_CTRL0r_PRI5_GRPf_SET(pg_ctrl0, 5);
        PG_CTRL0r_PRI6_GRPf_SET(pg_ctrl0, 6);
        PG_CTRL0r_PRI7_GRPf_SET(pg_ctrl0, 7);
        WRITE_PG_CTRL0r(unit, mmu_port, pg_ctrl0);

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mmu_port, pg_ctrl1);
        PG_CTRL1r_PRI8_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI9_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI10_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI11_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI12_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI13_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI14_GRPf_SET(pg_ctrl1, 7);
        PG_CTRL1r_PRI15_GRPf_SET(pg_ctrl1, 7);
        WRITE_PG_CTRL1r(unit, mmu_port, pg_ctrl1);

        /* PG2TCr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PG2TCr(unit, mmu_port, index, pg2tc);
            PG2TCr_PG_BMPf_SET(pg2tc, 0);
            WRITE_PG2TCr(unit, mmu_port, index, pg2tc);
        }

        /* PG2TCr, index 7 */
        index = 7;
        val = (IS_CPU_PORT(port)) ? 0x0 : 0x80;
        READ_PG2TCr(unit, mmu_port, index, pg2tc);
        PG2TCr_PG_BMPf_SET(pg2tc, val);
        WRITE_PG2TCr(unit, mmu_port, index, pg2tc);

        /* IBPPKTSETLIMITr, index 0 */
        val = (IS_CPU_PORT(port)) ?
              xoff_packet_thresholds_per_port_uplink_port :
              xoff_packet_thresholds_per_port_downlink_port;
        READ_IBPPKTSETLIMITr(unit, mmu_port, ibppktsetlimit);
        IBPPKTSETLIMITr_PKTSETLIMITf_SET(ibppktsetlimit, val);
        IBPPKTSETLIMITr_RESETLIMITSELf_SET(ibppktsetlimit, 3);
        WRITE_IBPPKTSETLIMITr(unit, mmu_port, ibppktsetlimit);

        /* MMU_FC_RX_ENr, index 0 */
        val = (IS_CPU_PORT(port)) ? 0x0 : 0x80;
        READ_MMU_FC_RX_ENr(unit, mmu_port, mmu_fc_rx_en);
        MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_SET(mmu_fc_rx_en, val);
        WRITE_MMU_FC_RX_ENr(unit, mmu_port, mmu_fc_rx_en);

        /* MMU_FC_TX_ENr, index 0 */
        val = (IS_CPU_PORT(port)) ? 0x0 : 0x80;
        READ_MMU_FC_TX_ENr(unit, mmu_port, mmu_fc_tx_en);
        MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_SET(mmu_fc_tx_en, val);
        WRITE_MMU_FC_TX_ENr(unit, mmu_port, mmu_fc_tx_en);

        /* PGCELLLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_PGCELLLIMITr(unit, mmu_port, index, pgcelllimit);
            PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_downlink_ports);
            PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit,
                xoff_cell_threshold_downlink_ports);
            WRITE_PGCELLLIMITr(unit, mmu_port, index, pgcelllimit);
        }

        /* PGCELLLIMITr, index 7 */
        index = 7;
        if (IS_CPU_PORT(port)) {
            val = xoff_cell_threshold_downlink_ports;
        } else if (speed == 1000) {
            val = xoff_cell_thresholds_per_port_1g_port;
        } else if (speed == 2500) {
            val = xoff_cell_thresholds_per_port_2dot5g_port;
        } else if (speed == 10000) {
            val = xoff_cell_thresholds_per_port_10g_port;
        } else if (speed == 20000) {
            val = xoff_cell_thresholds_per_port_20g_port;
        } else if (speed == 25000) {
            val = xoff_cell_thresholds_per_port_25g_port;
        } else if (speed == 40000) {
            val = xoff_cell_thresholds_per_port_40g_port;
        } else if (speed == 50000) {
            val = xoff_cell_thresholds_per_port_50g_port;
        } else {
            val = xoff_cell_thresholds_per_port_100g_port;
        }
        READ_PGCELLLIMITr(unit, mmu_port, index, pgcelllimit);
        PGCELLLIMITr_CELLSETLIMITf_SET(pgcelllimit, val);
        PGCELLLIMITr_CELLRESETLIMITf_SET(pgcelllimit, val);
        WRITE_PGCELLLIMITr(unit, mmu_port, index, pgcelllimit);

        /* PGDISCARDSETLIMITr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            val = (speed <= 40000) ? discard_limit_per_port_pg_uplink_port :
                                     discard_limit_per_port_pg_downlink_port;

            READ_PGDISCARDSETLIMITr(unit, mmu_port, index, pgdiscardsetlimit);
            PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, val);
            WRITE_PGDISCARDSETLIMITr(unit, mmu_port, index, pgdiscardsetlimit);
        }

        /* PGDISCARDSETLIMITr, index 7 */
        index = 7;
        if (IS_CPU_PORT(port)) {
            val = discard_limit_per_port_pg_downlink_port;
        } else if (speed == 1000) {
            val = discard_limit_per_port_pg_1g_port;
        } else if (speed == 2500) {
            val = discard_limit_per_port_pg_2dot5g_port;
        } else if (speed == 10000) {
            val = discard_limit_per_port_pg_10g_port;
        } else if (speed == 20000) {
            val = discard_limit_per_port_pg_20g_port;
        } else if (speed == 25000) {
            val = discard_limit_per_port_pg_25g_port;
        } else if (speed == 40000) {
            val = discard_limit_per_port_pg_40g_port;
        } else if (speed == 50000) {
            val = discard_limit_per_port_pg_50g_port;
        } else {
            val = discard_limit_per_port_pg_100g_port;
        }
        READ_PGDISCARDSETLIMITr(unit, mmu_port, index, pgdiscardsetlimit);
        PGDISCARDSETLIMITr_DISCARDSETLIMITf_SET(pgdiscardsetlimit, val);
        WRITE_PGDISCARDSETLIMITr(unit, mmu_port, index, pgdiscardsetlimit);
    }

    PBMP_ITER(pbmp_1g, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                    holcosminxqcnt_qlayer, egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                holcosminxqcnt_qlayer, egress_xq_min_reserve_lossless_1g_port);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                    holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch
                    + egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch
                            + egress_xq_min_reserve_lossless_1g_port;
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch
                    + egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch
                            + egress_xq_min_reserve_lossless_1g_port - 1;
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch - 1;
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
            }
        } else {
            /* HOLCOSMINXQCNTr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            }
            /* HOLCOSMINXQCNTr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossless_1g_port);
            WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);

            /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch
                    + egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
                WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            }

            /* HOLCOSPKTSETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch
                            + egress_xq_min_reserve_lossless_1g_port;
            READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
            WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);

            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch
                    + egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index, holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, val);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index, holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_1g_port - 1;
            READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, val);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index, holcospktresetlimit);

            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_2kxq_port_1g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_1g_port);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_1g_port);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                  egress_queue_min_reserve_lossless_1g_port;
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio);
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }
        } else {
           /* LWMCOSCELLSETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
            }

            /* LWMCOSCELLSETLIMITr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index, lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_1g_port);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_1g_port);
            WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index, lwmcoscellsetlimit);

            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                    val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_1g_port;
            READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
            for (index = 0; index <= 1; index++) {
                READ_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                    color_drop_en_qlayer);
                COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
                WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                    color_drop_en_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1 - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        } else {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
            WRITE_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
        }

        /* SHARED_POOL_CTRLr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext1, 0xffffff);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext2, 0xffffffff);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        }

        /* E2ECC_PORT_CONFIGr, index 0 */
        E2ECC_PORT_CONFIGr_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_2dot5g, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossless_2dot5g_port);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                    holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch
                    + egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch
                    + egress_xq_min_reserve_lossless_2dot5g_port;
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch
                    + egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch +
                  egress_xq_min_reserve_lossless_2dot5g_port - 1;
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch - 1;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
            }
        } else {
            /* HOLCOSMINXQCNTr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            }

            /* HOLCOSMINXQCNTr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossless_2dot5g_port);
            WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);

            /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch
                    + egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
                WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            }

            /* HOLCOSPKTSETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_2dot5g_port;
            READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
            WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);

            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, val);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_2dot5g_port - 1;
            READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, val);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);

            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_2kxq_port_2dot5g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer,
                egress_queue_min_reserve_lossless_2dot5g_port);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer,
                egress_queue_min_reserve_lossless_2dot5g_port);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_2dot5g_port;
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio);
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }
        } else {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
            }
            /* LWMCOSCELLSETLIMITr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_2dot5g_port);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_2dot5g_port);
            WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);

            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                    val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_2dot5g_port;
            READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
            for (index = 0; index <= 1; index++) {
                COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
                WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                    color_drop_en_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1 - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        } else {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
            WRITE_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
        }

        /* SHARED_POOL_CTRLr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext1, 0xffffff);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext2, 0xffffffff);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        }

        /* E2ECC_PORT_CONFIGr, index 0 */
        E2ECC_PORT_CONFIGr_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_10g_6kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossy);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }
        /* HOLCOSMINXQCNT_QLAYERr, index 7 */
        index = 7;
        READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index, holcosminxqcnt_qlayer);
        HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
            egress_xq_min_reserve_lossless_10g_port);
        WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);
        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                holcosminxqcnt_qlayer, 0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_10g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }
        /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_10g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_10g_port;
        READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);
        HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
            holcospktsetlimit_qlayer, val);
        WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_10g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_10g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_10g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_10g_port - 1;
        READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);
        HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
            holcospktresetlimit_qlayer, val);
        WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_10g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
            CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
            CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_6kxq_port_10g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
        index = 7;
        READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
            lwmcoscellsetlimit_qlayer,
            egress_queue_min_reserve_lossless_10g_port);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
            lwmcoscellsetlimit_qlayer,
            egress_queue_min_reserve_lossless_10g_port);
        WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
        index = 7;
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                              egress_queue_min_reserve_lossless_10g_port;
        READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
            holcoscellmaxlimit_qlayer, val);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
            holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
        WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
        for (index = 0; index <= 1; index++) {
            READ_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
            HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                holcospktsetlimit_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
            HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qgroup,
                numxqs_per_6kxq_uplink_ports - 1 - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
            CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
            CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qgroup, total_advertised_cell_memory - 1);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qgroup,
                total_advertised_cell_memory - 1 - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
        }

        /* COLOR_DROP_EN_QGROUPr, index 0 */
        READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr, index 0-7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        /* SHARED_POOL_CTRL_EXT1r */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext1, 0xffffff);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r */
        READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext2, 0xffffffff);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);

        /* E2ECC_PORT_CONFIGr, index 0 */
        READ_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_10g_2kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            }

            /* HOLCOSMINXQCNTr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossless_10g_port);
            WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt, 0);
                WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_10g_port;
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_10g_port - 1;
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch - 1;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 63; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
            }
        } else {
            /* HOLCOSMINXQCNTr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            }
            /* HOLCOSMINXQCNTr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossless_10g_port);
            WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);

            /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
                WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            }
            /* HOLCOSPKTSETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_10g_port;
            READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
            WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);

            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                    val);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_10g_port - 1;
            READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, val);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index, holcospktresetlimit);

            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_2kxq_port_10g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer,
                egress_queue_min_reserve_lossless_10g_port);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer,
                egress_queue_min_reserve_lossless_10g_port);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_10g_port;
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio);
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }
        } else {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
            }

            /* LWMCOSCELLSETLIMITr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_10g_port);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_10g_port);
            WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);

            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                    val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_10g_port;
            READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
            for (index = 0; index <= 1; index++) {
                COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
                WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                    color_drop_en_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1 - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        } else {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
            WRITE_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
        }

        /* SHARED_POOL_CTRLr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext1, 0xffffff);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext2, 0xffffffff);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        }

        /* E2ECC_PORT_CONFIGr, index 0 */
        E2ECC_PORT_CONFIGr_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_20g_6kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossy);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSMINXQCNT_QLAYERr, index 7 */
        index = 7;
        READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);
        HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
            egress_xq_min_reserve_lossless_20g_port);
        WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);

        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                holcosminxqcnt_qlayer, 0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_20g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_20g - skidmarker - prefetch
              + egress_xq_min_reserve_lossless_20g_port;
        READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);
        HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
            holcospktsetlimit_qlayer, val);
        WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_20g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_20g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_20g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_20g_port - 1;
        READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);
        HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
            holcospktresetlimit_qlayer, val);
        WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_20g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
            CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
            CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_6kxq_port_20g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
        index = 7;
        READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
            lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_20g_port);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
            lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_20g_port);
        WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index, lwmcoscellsetlimit_qlayer);


        /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
        index = 7;
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossless_20g_port;
        READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
            holcoscellmaxlimit_qlayer, val);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
            holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
        WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
        for (index = 0; index <= 1; index++) {
            READ_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
            HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                holcospktsetlimit_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
            HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qgroup,
                numxqs_per_6kxq_uplink_ports - 1 - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
            CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
            CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qgroup, total_advertised_cell_memory - 1);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qgroup,
                total_advertised_cell_memory - 1 - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
        }

        /* COLOR_DROP_EN_QGROUPr, index 0 */
        READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr, index 0-7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        /* SHARED_POOL_CTRL_EXT1r */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext1, 0xffffff);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r */
        READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext2, 0xffffffff);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);

        /* E2ECC_PORT_CONFIGr, index 0 */
        READ_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_20g_2kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossless_20g_port);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                    holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_20g_port;
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_20g_port - 1;
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch - 1;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
            }
        } else {
            /* HOLCOSMINXQCNTr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            }

            /* HOLCOSMINXQCNTr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossless_20g_port);
            WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);

            /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
                WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            }

            /* HOLCOSPKTSETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_20g_port;
            READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
            WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);

            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                    val);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_20g_port - 1;
            READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                val);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);

            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_2kxq_port_20g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_20g_port);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_20g_port);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_20g_port;
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio);
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }
        } else {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
            }

            /* LWMCOSCELLSETLIMITr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_20g_port);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_20g_port);
            WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);

            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                    val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_20g_port;
            READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
            for (index = 0; index <= 1; index++) {
                COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
                WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                    color_drop_en_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1 - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        } else {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
            WRITE_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
        }

        /* SHARED_POOL_CTRLr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext1, 0xffffff);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext2, 0xffffffff);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        }

        /* E2ECC_PORT_CONFIGr, index 0 */
        E2ECC_PORT_CONFIGr_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_25g_6kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossy);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }
        /* HOLCOSMINXQCNT_QLAYERr, index 7 */
        index = 7;
        READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);
        HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
            egress_xq_min_reserve_lossless_25g_port);
        WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);

        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                holcosminxqcnt_qlayer, 0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_25g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_25g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_25g_port;
        READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);
        HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
            holcospktsetlimit_qlayer, val);
        WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_25g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_25g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_25g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_25g_port - 1;
        READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);
        HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
            holcospktresetlimit_qlayer, val);
        WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_25g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
            CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
            CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_6kxq_port_25g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
        index = 7;
        READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
            lwmcoscellsetlimit_qlayer,
            egress_queue_min_reserve_lossless_25g_port);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
            lwmcoscellsetlimit_qlayer,
            egress_queue_min_reserve_lossless_25g_port);
        WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
        index = 7;
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossless_25g_port;
        READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
            holcoscellmaxlimit_qlayer, val);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
            holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
        WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
        for (index = 0; index <= 1; index++) {
            READ_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
            HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                holcospktsetlimit_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
            HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qgroup,
                numxqs_per_6kxq_uplink_ports - 1 - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
            CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
            CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qgroup, total_advertised_cell_memory - 1);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qgroup,
                total_advertised_cell_memory - 1 - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
        }

        /* COLOR_DROP_EN_QGROUPr, index 0 */
        READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr, index 0-7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        /* SHARED_POOL_CTRL_EXT1r */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext1, 0xffffff);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r */
        READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext2, 0xffffffff);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);

        /* E2ECC_PORT_CONFIGr, index 0 */
        READ_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_25g_2kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossless_25g_port);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                    holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_25g_port;
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_25g_port - 1;
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch - 1;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
            }
        } else {
            /* HOLCOSMINXQCNTr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            }
            /* HOLCOSMINXQCNTr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossless_25g_port);
            WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);

            /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
                WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            }
            /* HOLCOSPKTSETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_25g_port;
            READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
            WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);

            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                    val);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_25g_port - 1;
            READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                val);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);

            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_2kxq_port_25g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer,
                egress_queue_min_reserve_lossless_25g_port);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer,
                egress_queue_min_reserve_lossless_25g_port);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_25g_port;
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio);
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }
        } else {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
            }
            /* LWMCOSCELLSETLIMITr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_25g_port);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_25g_port);
            WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);

            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                    val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_25g_port;
            READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
            for (index = 0; index <= 1; index++) {
                COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
                WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                    color_drop_en_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1 - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        } else {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
            WRITE_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
        }

        /* SHARED_POOL_CTRLr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext1, 0xffffff);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext2, 0xffffffff);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        }

        /* E2ECC_PORT_CONFIGr, index 0 */
        E2ECC_PORT_CONFIGr_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_40g_6kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossy);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSMINXQCNT_QLAYERr, index 7 */
        index = 7;
        READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);
        HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
            egress_xq_min_reserve_lossless_40g_port);
        WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);

        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                holcosminxqcnt_qlayer, 0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_40g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_40g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_40g_port;
        READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);
        HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
            holcospktsetlimit_qlayer, val);
        WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_40g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_40g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_40g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_40g_port - 1;
        READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);
        HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
            holcospktresetlimit_qlayer, val);
        WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_40g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
            CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
            CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_6kxq_port_40g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
        index = 7;
        READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
            lwmcoscellsetlimit_qlayer,
            egress_queue_min_reserve_lossless_40g_port);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
            lwmcoscellsetlimit_qlayer,
            egress_queue_min_reserve_lossless_40g_port);
        WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
        index = 7;
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossless_40g_port;
        READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
            holcoscellmaxlimit_qlayer, val);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
            holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
        WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
        for (index = 0; index <= 1; index++) {
            READ_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
            HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                holcospktsetlimit_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
            HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qgroup,
                numxqs_per_6kxq_uplink_ports - 1 - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
            CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
            CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qgroup, total_advertised_cell_memory - 1);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qgroup,
                total_advertised_cell_memory - 1 - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
        }

        /* COLOR_DROP_EN_QGROUPr, index 0 */
        READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr, index 0-7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        /* SHARED_POOL_CTRL_EXT1r */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext1, 0xffffff);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r */
        READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext2, 0xffffffff);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);

        /* E2ECC_PORT_CONFIGr, index 0 */
        READ_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_40g_2kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossless_40g_port);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                    holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_40g_port;
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch +
                  egress_xq_min_reserve_lossless_40g_port - 1;
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch - 1;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
            }
        } else {
            /* HOLCOSMINXQCNTr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            }

            /* HOLCOSMINXQCNTr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossless_40g_port);
            WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);

            /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
                WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            }

            /* HOLCOSPKTSETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_40g_port;
            READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
            WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);

            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                    val);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_40g_port - 1;
            READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                val);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);

            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            }

            /* CNGPORTPKTLIMIT0r, index 0 */
            READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
            CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
                numxqs_per_2kxq_uplink_ports - 1);
            WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_2kxq_port_40g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_40g_port);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_40g_port);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_40g_port;
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            val = fl_ceiling_func(shared_space_cells,
                    queue_port_limit_ratio);
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }
        } else {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
            }

            /* LWMCOSCELLSETLIMITr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_40g_port);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_40g_port);
            WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);

            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                    val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_40g_port;
            READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
            for (index = 0; index <= 1; index++) {
                COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
                WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                    color_drop_en_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1 - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        } else {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
            WRITE_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
        }

        /* SHARED_POOL_CTRLr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext1, 0xffffff);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext2, 0xffffffff);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        }

        /* E2ECC_PORT_CONFIGr, index 0 */
        E2ECC_PORT_CONFIGr_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_50g_6kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossy);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }
        /* HOLCOSMINXQCNT_QLAYERr, index 7 */
        index = 7;
        READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);
        HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
            egress_xq_min_reserve_lossless_50g_port);
        WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);

        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                holcosminxqcnt_qlayer, 0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_50g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_50g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_50g_port;
        READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);
        HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
            holcospktsetlimit_qlayer, val);
        WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_50g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_50g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
         for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }
        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_50g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_50g_port - 1;
        READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);
        HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
            holcospktresetlimit_qlayer, val);
        WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_50g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
            CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
            CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_6kxq_port_50g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
        index = 7;
        READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
            lwmcoscellsetlimit_qlayer,
            egress_queue_min_reserve_lossless_50g_port);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
            lwmcoscellsetlimit_qlayer,
            egress_queue_min_reserve_lossless_50g_port);
        WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
        index = 7;
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossless_50g_port;
        READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
            holcoscellmaxlimit_qlayer, val);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
            holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
        WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
        for (index = 0; index <= 1; index++) {
            READ_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
            HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                holcospktsetlimit_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
            HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qgroup,
                numxqs_per_6kxq_uplink_ports - 1 - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
            CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
            CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qgroup, total_advertised_cell_memory - 1);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qgroup,
                total_advertised_cell_memory - 1 - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
        }

        /* COLOR_DROP_EN_QGROUPr, index 0 */
        READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr, index 0-7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        /* SHARED_POOL_CTRL_EXT1r */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext1, 0xffffff);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r */
        READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext2, 0xffffffff);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);

        /* E2ECC_PORT_CONFIGr, index 0 */
        READ_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_50g_2kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossless_50g_port);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                    holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_50g_port;
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);

            val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_50g_port - 1;
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch - 1;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
            }
        } else {
            /* HOLCOSMINXQCNTr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            }
            /* HOLCOSMINXQCNTr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossless_50g_port);
            WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);

            /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
                WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            }

            /* HOLCOSPKTSETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_50g_port;
            READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
            WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);

            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                    val);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_50g_port - 1;
            READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                val);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);

            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_2kxq_port_50g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_50g_port);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_50g_port);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_50g_port;
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio);
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }
        } else {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
            }

            /* LWMCOSCELLSETLIMITr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_50g_port);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_50g_port);
            WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);

            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                        egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                    val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_50g_port;
            READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
            for (index = 0; index <= 1; index++) {
                COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
                WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                    color_drop_en_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1 - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        } else {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
            WRITE_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
        }

        /* SHARED_POOL_CTRLr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext1, 0xffffff);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext2, 0xffffffff);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        }

        /* E2ECC_PORT_CONFIGr, index 0 */
        E2ECC_PORT_CONFIGr_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_100g_6kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossy);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }
        /* HOLCOSMINXQCNT_QLAYERr, index 7 */
        index = 7;
        READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);
        HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
            egress_xq_min_reserve_lossless_100g_port);
        WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
            holcosminxqcnt_qlayer);

        /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                holcosminxqcnt_qlayer, 0);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_100g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
       for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_100g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_100g_port;
        READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);
        HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
            holcospktsetlimit_qlayer, val);
        WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktsetlimit_qlayer);

        /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_100g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
        }

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
        val = shared_xqs_per_6kxq_port_100g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }
        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
        index = 7;
        val = shared_xqs_per_6kxq_port_100g - skidmarker - prefetch +
                egress_xq_min_reserve_lossless_100g_port - 1;
        READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);
        HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
            holcospktresetlimit_qlayer, val);
        WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
            holcospktresetlimit_qlayer);

        /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
        val = shared_xqs_per_6kxq_port_100g - skidmarker - prefetch +
                egress_xq_min_reserve_lossy - 1;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
        }

        /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
            CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                cngcospktlimit0_qlayer);
        }

        /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
        for (index = 0; index <= 63; index++) {
            READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
            CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qlayer, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                cngcospktlimit1_qlayer);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_6kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        val = shared_xqs_per_6kxq_port_100g - skidmarker - prefetch;
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
        for (index = 0; index <= 6; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
        index = 7;
        READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
            lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_100g_port);
        LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
            lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_100g_port);
        WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
            lwmcoscellsetlimit_qlayer);

        /* LWMCOSCELLSETLIMIT_QLAYERr, index 8 ~ 63 */
        for (index = 8; index <= 63; index++) {
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 0; index <= 6; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
        index = 7;
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossless_100g_port;
        READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
            holcoscellmaxlimit_qlayer, val);
        HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
            holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
        WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
            holcoscellmaxlimit_qlayer);

        /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_lossy;
        for (index = 8; index <= 63; index++) {
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
        for (index = 0; index <= 1; index++) {
            READ_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
            COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
            WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                color_drop_en_qlayer);
        }

        /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
            HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                holcospktsetlimit_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktsetlimit_qgroup);
        }

        /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
            HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qgroup,
                numxqs_per_6kxq_uplink_ports - 1 - 1);
            WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                holcospktresetlimit_qgroup);
        }

        /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
            CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                cngcospktlimit0_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                cngcospktlimit0_qgroup);
        }

        /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
            CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                cngcospktlimit1_qgroup, numxqs_per_6kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                cngcospktlimit1_qgroup);
        }

        /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qgroup, total_advertised_cell_memory - 1);
            HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qgroup,
                total_advertised_cell_memory - 1 - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                holcoscellmaxlimit_qgroup);
        }

        /* COLOR_DROP_EN_QGROUPr, index 0 */
        READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
        WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);

        /* SHARED_POOL_CTRLr, index 0-7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        /* SHARED_POOL_CTRL_EXT1r */
        READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
        SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext1, 0xffffff);
        WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

        /* SHARED_POOL_CTRL_EXT2r */
        READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
            shared_pool_ctrl_ext2, 0xffffffff);
        WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);

        /* E2ECC_PORT_CONFIGr, index 0 */
        READ_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);
        E2ECC_PORT_CONFIGr_COS_CELL_ENf_SET(e2ecc_port_cfg, 0);
        E2ECC_PORT_CONFIGr_COS_PKT_ENf_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    PBMP_ITER(pbmp_100g_2kxq, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSMINXQCNT_QLAYERr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);
            HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(holcosminxqcnt_qlayer,
                egress_xq_min_reserve_lossless_100g_port);
            WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                holcosminxqcnt_qlayer);

            /* HOLCOSMINXQCNT_QLAYERr, index 8 ~ 63 */
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
                HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_SET(
                    holcosminxqcnt_qlayer, 0);
                WRITE_HOLCOSMINXQCNT_QLAYERr(unit, mmu_port, index,
                    holcosminxqcnt_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch +
                  egress_xq_min_reserve_lossless_100g_port;
            READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);
            HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                holcospktsetlimit_qlayer, val);
            WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktsetlimit_qlayer);

            /* HOLCOSPKTSETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
                HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qlayer, val);
                WRITE_HOLCOSPKTSETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktsetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_100g_port - 1;
            READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);
            HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                holcospktresetlimit_qlayer, val);
            WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                holcospktresetlimit_qlayer);

            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 8 ~ 63 */
            val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch- 1;
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
                HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qlayer, val);
                WRITE_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mmu_port, index,
                    holcospktresetlimit_qlayer);
            }

            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
                CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit0_qlayer);
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
                CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qlayer, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QLAYERr(unit, mmu_port, index,
                    cngcospktlimit1_qlayer);
            }
        } else {
            /* HOLCOSMINXQCNTr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
                HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                    egress_xq_min_reserve_lossy);
                WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            }
            /* HOLCOSMINXQCNTr, index 7 */
            index = 7;
            READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_xq_min_reserve_lossless_100g_port);
            WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);

            /* HOLCOSPKTSETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
                HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
                WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            }

            /* HOLCOSPKTSETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_100g_port;
            READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
            WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);

            /* HOLCOSPKTRESETLIMITr, index 0 ~ 6 */
            val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossy - 1;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
                HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                    val);
                WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                    holcospktresetlimit);
            }

            /* HOLCOSPKTRESETLIMITr, index 7 */
            index = 7;
            val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch +
                    egress_xq_min_reserve_lossless_100g_port - 1;
            READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit,
                val);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index,
                holcospktresetlimit);

            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
                CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
                CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        val = shared_xqs_per_2kxq_port_100g - skidmarker - prefetch;
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport, val);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport, val - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* LWMCOSCELLSETLIMIT_QLAYERr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_100g_port);
            LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                lwmcoscellsetlimit_qlayer, egress_queue_min_reserve_lossless_100g_port);
            WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                lwmcoscellsetlimit_qlayer);

            for (index = 8; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_SET(
                    lwmcoscellsetlimit_qlayer, 0);
                WRITE_LWMCOSCELLSETLIMIT_QLAYERr(unit, mmu_port, index,
                    lwmcoscellsetlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_100g_port;
            READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                holcoscellmaxlimit_qlayer, val);
            HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                holcoscellmaxlimit_qlayer);

            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 8 ~ 63 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio);
            for (index = 8; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qlayer, val);
                HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qlayer, val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mmu_port, index,
                    holcoscellmaxlimit_qlayer);
            }
        } else {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 6 */
            for (index = 0; index <= 6; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
                LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                    egress_queue_min_reserve_lossy);
                WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                    lwmcoscellsetlimit);
            }

            /* LWMCOSCELLSETLIMITr, index 7 */
            index = 7;
            READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_100g_port);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_lossless_100g_port);
            WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);

            /* HOLCOSCELLMAXLIMITr, index 0 ~ 6 */
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossy;
            for (index = 0; index <= 6; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
                HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
                HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                    val - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                    holcoscellmaxlimit);
            }

            /* HOLCOSCELLMAXLIMITr, index 7 */
            index = 7;
            val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                    egress_queue_min_reserve_lossless_100g_port;
            READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1 */
            for (index = 0; index <= 1; index++) {
                COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_SET(color_drop_en_qlayer, 0);
                WRITE_COLOR_DROP_EN_QLAYERr(unit, mmu_port, index,
                    color_drop_en_qlayer);
            }

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
                HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_SET(
                    holcospktsetlimit_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_HOLCOSPKTSETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktsetlimit_qgroup);
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
                HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_SET(
                    holcospktresetlimit_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1 - 1);
                WRITE_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mmu_port, index,
                    holcospktresetlimit_qgroup);
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
                CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_SET(
                    cngcospktlimit0_qgroup, numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT0_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit0_qgroup);
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_SET(
                    cngcospktlimit1_qgroup,
                    numxqs_per_2kxq_uplink_ports - 1);
                WRITE_CNGCOSPKTLIMIT1_QGROUPr(unit, mmu_port, index,
                    cngcospktlimit1_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
                HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_SET(
                    holcoscellmaxlimit_qgroup,
                    total_advertised_cell_memory - 1 - ethernet_mtu_cell);
                WRITE_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mmu_port, index,
                    holcoscellmaxlimit_qgroup);
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
            COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_SET(color_drop_en_qgroup, 0);
            WRITE_COLOR_DROP_EN_QGROUPr(unit, mmu_port, color_drop_en_qgroup);
        } else {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
            COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
            WRITE_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
        }

        /* SHARED_POOL_CTRLr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        if (mmu_port >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);
            SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext1, 0xffffff);
            WRITE_SHARED_POOL_CTRL_EXT1r(unit, mmu_port, shared_pool_ctrl_ext1);

            /* SHARED_POOL_CTRL_EXT2r */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
            SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_SET(
                shared_pool_ctrl_ext2, 0xffffffff);
            WRITE_SHARED_POOL_CTRL_EXT2r(unit, mmu_port, shared_pool_ctrl_ext2);
        }

        /* E2ECC_PORT_CONFIGr, index 0 */
        E2ECC_PORT_CONFIGr_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    /* port-based : cpu port*/
    PBMP_ITER(pbmp_cpu, port) {
        mmu_port = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));

        /* HOLCOSMINXQCNTr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
            HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_SET(holcosminxqcnt,
                egress_queue_min_reserve_cpu_ports);
            WRITE_HOLCOSMINXQCNTr(unit, mmu_port, index, holcosminxqcnt);
        }

        /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
        val = shared_xqs_cpu_port - skidmarker - prefetch +
                egress_queue_min_reserve_cpu_ports;
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
            HOLCOSPKTSETLIMITr_PKTSETLIMITf_SET(holcospktsetlimit, val);
            WRITE_HOLCOSPKTSETLIMITr(unit, mmu_port, index, holcospktsetlimit);
        }

        /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
        val = shared_xqs_cpu_port - skidmarker - prefetch +
                egress_queue_min_reserve_cpu_ports - 1;
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSPKTRESETLIMITr(unit, mmu_port, index, holcospktresetlimit);
            HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_SET(holcospktresetlimit, val);
            WRITE_HOLCOSPKTRESETLIMITr(unit, mmu_port, index, holcospktresetlimit);
        }

        /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
            CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_SET(cngcospktlimit0,
                numxqs_per_2kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT0r(unit, mmu_port, index, cngcospktlimit0);
        }

        /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
            CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_SET(cngcospktlimit1,
                numxqs_per_2kxq_uplink_ports - 1);
            WRITE_CNGCOSPKTLIMIT1r(unit, mmu_port, index, cngcospktlimit1);
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);
        CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_SET(cngportpktlimit0,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT0r(unit, mmu_port, cngportpktlimit0);

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);
        CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_SET(cngportpktlimit1,
            numxqs_per_2kxq_uplink_ports - 1);
        WRITE_CNGPORTPKTLIMIT1r(unit, mmu_port, cngportpktlimit1);

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);
        DYNXQCNTPORTr_DYNXQCNTPORTf_SET(dynxqcntport,
            shared_xqs_cpu_port - skidmarker - prefetch);
        WRITE_DYNXQCNTPORTr(unit, mmu_port, dynxqcntport);

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);
        DYNRESETLIMPORTr_DYNRESETLIMPORTf_SET(dynresetlimport,
            shared_xqs_cpu_port - skidmarker - prefetch - 2);
        WRITE_DYNRESETLIMPORTr(unit, mmu_port, dynresetlimport);

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLSETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_cpu_ports);
            WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
        }

        /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
            LWMCOSCELLSETLIMITr_CELLRESETLIMITf_SET(lwmcoscellsetlimit,
                egress_queue_min_reserve_cpu_ports);
            WRITE_LWMCOSCELLSETLIMITr(unit, mmu_port, index,
                lwmcoscellsetlimit);
        }

        /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        val = fl_ceiling_func(shared_space_cells, queue_port_limit_ratio) +
                egress_queue_min_reserve_cpu_ports;
        for (index = 0; index <= 7; index++) {
            READ_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
            HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_SET(holcoscellmaxlimit, val);
            HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_SET(holcoscellmaxlimit,
                val - ethernet_mtu_cell);
            WRITE_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                holcoscellmaxlimit);
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);
        DYNCELLLIMITr_DYNCELLSETLIMITf_SET(dyncelllimit, shared_space_cells);
        DYNCELLLIMITr_DYNCELLRESETLIMITf_SET(dyncelllimit,
            shared_space_cells - ethernet_mtu_cell * 2);
        WRITE_DYNCELLLIMITr(unit, mmu_port, dyncelllimit);

        /* COLOR_DROP_ENr, index 0 */
        READ_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);
        COLOR_DROP_ENr_COLOR_DROP_ENf_SET(color_drop_en, 0);
        WRITE_COLOR_DROP_ENr(unit, mmu_port, color_drop_en);

        /* SHARED_POOL_CTRLr, index 0 ~ 7*/
        for (index = 0; index <= 7; index++) {
            READ_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_SET(shared_pool_ctrl, 255);
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_SET(shared_pool_ctrl, 0);
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_SET(shared_pool_ctrl, 0);
            WRITE_SHARED_POOL_CTRLr(unit, mmu_port, shared_pool_ctrl);
        }

        /* E2ECC_PORT_CONFIGr, index 0 */
        E2ECC_PORT_CONFIGr_SET(e2ecc_port_cfg, 0);
        WRITE_E2ECC_PORT_CONFIGr(unit, mmu_port, e2ecc_port_cfg);

        /* EARLY_DYNCELLLIMITr, index 0 */
        EARLY_DYNCELLLIMITr_SET(early_dyncelllimit, 0);
        WRITE_EARLY_DYNCELLLIMITr(unit, mmu_port, early_dyncelllimit);

        /* EARLY_HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            EARLY_HOLCOSCELLMAXLIMITr_SET(early_holcoscellmaxlimit, 0);
            WRITE_EARLY_HOLCOSCELLMAXLIMITr(unit, mmu_port, index,
                early_holcoscellmaxlimit);
        }
    }

    return SOC_E_NONE;
}

#if CFG_MMU_DEBUG
/*
 * Function:
 *      _soc_firelight_mmu_init_debug()
 * Purpose:
 *      TO print out mmu values for debuging purpose.
 */
static void _soc_firelight_mmu_init_debug(int unit)
{
    int port, mport;
    int index;
    pbmp_t pbmp_all;
    CFAPCONFIGr_t cfapconfig;
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
    E2ECC_MODEr_t e2ecc_mode;
    E2ECC_HOL_ENr_t e2ecc_hol_en;
    E2ECC_MIN_TX_TIMERr_t e2ecc_min_tx_timer;
    E2ECC_MAX_TX_TIMERr_t e2ecc_max_tx_timer;
    E2ECC_TX_ENABLE_BMPr_t e2ecc_tx_enable_bmp;
    E2ECC_TX_PORTS_NUMr_t e2ecc_tx_ports_num;
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
    E2ECC_PORT_CONFIGr_t e2ecc_port_cfg;
    EARLY_DYNCELLLIMITr_t early_dyncelllimit;
    EARLY_HOLCOSCELLMAXLIMITr_t early_holcoscellmaxlimit;;

    PBMP_ASSIGN(pbmp_all, BCM5607X_ALL_PORTS_MASK);

    sal_printf("\n########################### MMU SETTING #################\n");

    READ_CFAPCONFIGr(unit, cfapconfig);
    sal_printf("CFAPCONFIG.CFAPPOOLSIZE 0x%x\n",
        CFAPCONFIGr_CFAPPOOLSIZEf_GET(cfapconfig));

    /* system-based */
    sal_printf("\nSystem-Based ==>\n");
    READ_CFAPFULLTHRESHOLDr(unit, cfapfullthreshold);
    sal_printf("CFAPFULLTHRESHOLD.CFAPFULLSETPOINT 0x%x\n",
        CFAPFULLTHRESHOLDr_CFAPFULLSETPOINTf_GET(cfapfullthreshold));
    sal_printf("CFAPFULLTHRESHOLD.CFAPFULLRESETPOINT 0x%x\nx",
        CFAPFULLTHRESHOLDr_CFAPFULLRESETPOINTf_GET(cfapfullthreshold));
    READ_GBLLIMITSETLIMITr(unit, gbllimitsetlimit);
    sal_printf("GBLLIMITSETLIMIT.GBLCELLSETLIMIT 0x%x\n",
        GBLLIMITSETLIMITr_GBLCELLSETLIMITf_GET(gbllimitsetlimit));
    READ_GBLLIMITRESETLIMITr(unit, gbllimitresetlimit);
    sal_printf("GBLLIMITRESETLIMIT.GBLCELLRESETLIMIT 0x%x\n",
        GBLLIMITRESETLIMITr_GBLCELLRESETLIMITf_GET(gbllimitresetlimit));
    READ_TOTALDYNCELLSETLIMITr(unit, totaldyncellsetlimit);
    sal_printf("TOTALDYNCELLSETLIMIT.TOTALDYNCELLSETLIMIT 0x%x\n",
        TOTALDYNCELLSETLIMITr_TOTALDYNCELLSETLIMITf_GET(totaldyncellsetlimit));
    READ_TOTALDYNCELLRESETLIMITr(unit, totaldyncellresetlimit);
    sal_printf("TOTALDYNCELLRESETLIMIT.TOTALDYNCELLRESETLIMIT 0x%x\n",
        TOTALDYNCELLRESETLIMITr_TOTALDYNCELLRESETLIMITf_GET
        (totaldyncellresetlimit));

    PBMP_ITER(pbmp_all, port) {
        if (SOC_PORT_BLOCK_INDEX(port) != 0x0) {
            continue;
        }
        mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
        if (mport >= MMU_64Q_PPORT_BASE) {
            mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
            READ_TWO_LAYER_SCH_MODEr(unit, mport, two_layer_sch_mode);
            sal_printf("port %d mport %d TWO_LAYER_SCH_MODE.SCH_MODE 0x%x\n",
            port, mport, TWO_LAYER_SCH_MODEr_SCH_MODEf_GET(two_layer_sch_mode));
        }
    }

    READ_MISCCONFIGr(unit, miscconfig);
    sal_printf("MISCCONFIG.MULTIPLE_ACCOUNTING_FIX_EN 0x%x\n",
        MISCCONFIGr_MULTIPLE_ACCOUNTING_FIX_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.CNG_DROP_EN 0x%x\n",
        MISCCONFIGr_CNG_DROP_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.DYN_XQ_EN 0x%x\n",
        MISCCONFIGr_DYN_XQ_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.HOL_CELL_SOP_DROP_EN 0x%x\n",
        MISCCONFIGr_HOL_CELL_SOP_DROP_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.DYNAMIC_MEMORY_EN 0x%x\n",
        MISCCONFIGr_DYNAMIC_MEMORY_ENf_GET(miscconfig));
    sal_printf("MISCCONFIG.SKIDMARKER 0x%x\n",
        MISCCONFIGr_SKIDMARKERf_GET(miscconfig));

    READ_MMUPORTTXENABLE_0r(unit, mmuporttxenable_0);
    sal_printf("MMUPORTTXENABLE_0.MMUPORTTXENABLE 0x%x\n",
        MMUPORTTXENABLE_0r_MMUPORTTXENABLEf_GET(mmuporttxenable_0));

    READ_MMUPORTTXENABLE_1r(unit, mmuporttxenable_1);
    sal_printf("MMUPORTTXENABLE_1.MMUPORTTXENABLE 0x%x\n",
        MMUPORTTXENABLE_1r_MMUPORTTXENABLEf_GET(mmuporttxenable_1));

    READ_MMUPORTTXENABLE_2r(unit, mmuporttxenable_2);
    sal_printf("MMUPORTTXENABLE_2.MMUPORTTXENABLE 0x%x\n",
        MMUPORTTXENABLE_2r_MMUPORTTXENABLEf_GET(mmuporttxenable_2));

    READ_E2ECC_MODEr(unit, e2ecc_mode);
    sal_printf("E2ECC_MODE.EN 0x%x\n", E2ECC_MODEr_ENf_GET(e2ecc_mode));

    READ_E2ECC_HOL_ENr(unit, e2ecc_hol_en);
    sal_printf("E2ECC_HOL_EN.EN 0x%x\n", E2ECC_HOL_ENr_ENf_GET(e2ecc_hol_en));

    READ_E2ECC_MIN_TX_TIMERr(unit, e2ecc_min_tx_timer);
    sal_printf("E2ECC_MIN_TX_TIMER.LG 0x%x\n",
        E2ECC_MIN_TX_TIMERr_LGf_GET(e2ecc_min_tx_timer));
    sal_printf("E2ECC_MIN_TX_TIMER.TIMER 0x%x\n",
        E2ECC_MIN_TX_TIMERr_TIMERf_GET(e2ecc_min_tx_timer));

    READ_E2ECC_MAX_TX_TIMERr(unit, e2ecc_max_tx_timer);
    sal_printf("E2ECC_MAX_TX_TIMER.LG 0x%x\n",
        E2ECC_MAX_TX_TIMERr_LGf_GET(e2ecc_max_tx_timer));
    sal_printf("E2ECC_MAX_TX_TIMER.TIMER 0x%x\n",
        E2ECC_MAX_TX_TIMERr_TIMERf_GET(e2ecc_max_tx_timer));

    for (index = 0; index <= 7; index++) {
        READ_E2ECC_TX_ENABLE_BMPr(unit, index, e2ecc_tx_enable_bmp);
        sal_printf("E2ECC_TX_ENABLE_BMP_index%d 0x%x\n", index,
            E2ECC_TX_ENABLE_BMPr_GET(e2ecc_tx_enable_bmp));
    }

    READ_E2ECC_TX_PORTS_NUMr(unit, e2ecc_tx_ports_num);
    sal_printf("E2ECC_TX_PORTS_NUM.LG 0x%x\n",
        E2ECC_TX_PORTS_NUMr_GET(e2ecc_tx_ports_num));
    sal_printf("\n");

    PBMP_PORT_ADD(pbmp_all, BCM5607X_PORT_CMIC);
    PBMP_ITER(pbmp_all, port) {
        if (IS_CPU_PORT(port)) {
            mport = 0;
            sal_printf("\nPort-Based (cpu port) ==>\n");
        } else {
            if (SOC_PORT_BLOCK_INDEX(port) != 0x0) {
                continue;
            }

            mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(port));
            sal_printf("\nPort-Based (port %d mport %d speed %d) ==>\n", port,
                mport, SOC_PORT_SPEED_MAX(port));
        }

        /* PG_CTRL0r, index 0 */
        READ_PG_CTRL0r(unit, mport, pg_ctrl0);
        sal_printf("PG_CTRL0.PPFC_PG_EN 0x%x\n",
            PG_CTRL0r_PPFC_PG_ENf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI0_GRP 0x%x\n",
            PG_CTRL0r_PRI0_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI1_GRP 0x%x\n",
            PG_CTRL0r_PRI1_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI2_GRP 0x%x\n",
            PG_CTRL0r_PRI2_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI3_GRP 0x%x\n",
            PG_CTRL0r_PRI3_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI4_GRP 0x%x\n",
            PG_CTRL0r_PRI4_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI5_GRP 0x%x\n",
            PG_CTRL0r_PRI5_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI6_GRP 0x%x\n",
            PG_CTRL0r_PRI6_GRPf_GET(pg_ctrl0));
        sal_printf("PG_CTRL0.PRI7_GRP 0x%x\n",
            PG_CTRL0r_PRI7_GRPf_GET(pg_ctrl0));

        /* PG_CTRL1r, index 0 */
        READ_PG_CTRL1r(unit, mport, pg_ctrl1);
        sal_printf("PG_CTRL1.PRI8_GRP 0x%x\n",
            PG_CTRL1r_PRI8_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI9_GRP 0x%x\n",
            PG_CTRL1r_PRI9_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI10_GRP 0x%x\n",
            PG_CTRL1r_PRI10_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI11_GRP 0x%x\n",
            PG_CTRL1r_PRI11_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI12_GRP 0x%x\n",
            PG_CTRL1r_PRI12_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI13_GRP 0x%x\n",
            PG_CTRL1r_PRI13_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI14_GRP 0x%x\n",
            PG_CTRL1r_PRI14_GRPf_GET(pg_ctrl1));
        sal_printf("PG_CTRL1.PRI15_GRP 0x%x\n",
            PG_CTRL1r_PRI15_GRPf_GET(pg_ctrl1));

        /* PG2TCr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PG2TCr(unit, mport, index, pg2tc);
            sal_printf("COSQ %d PG2TC.PG_BMP 0x%x\n", index,
                PG2TCr_PG_BMPf_GET(pg2tc));
        }

        /* IBPPKTSETLIMITr, index 0 */
        READ_IBPPKTSETLIMITr(unit, mport, ibppktsetlimit);
        sal_printf("IBPPKTSETLIMIT.PKTSETLIMIT 0x%x\n",
            IBPPKTSETLIMITr_PKTSETLIMITf_GET(ibppktsetlimit));
        sal_printf("IBPPKTSETLIMIT.RESETLIMITSEL 0x%x\n",
            IBPPKTSETLIMITr_RESETLIMITSELf_GET(ibppktsetlimit));

        /* MMU_FC_RX_ENr, index 0 */
        READ_MMU_FC_RX_ENr(unit, mport, mmu_fc_rx_en);
        sal_printf("MMU_FC_RX_EN.MMU_FC_RX_ENABLE 0x%x\n",
            MMU_FC_RX_ENr_MMU_FC_RX_ENABLEf_GET(mmu_fc_rx_en));

        /* MMU_FC_TX_ENr, index 0 */
        READ_MMU_FC_TX_ENr(unit, mport, mmu_fc_tx_en);
        sal_printf("MMU_FC_TX_EN.MMU_FC_TX_ENABLE 0x%x\n",
            MMU_FC_TX_ENr_MMU_FC_TX_ENABLEf_GET(mmu_fc_tx_en));

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            sal_printf("COSQ %d PGCELLLIMIT.CELLSETLIMIT 0x%x\n", index,
                PGCELLLIMITr_CELLSETLIMITf_GET(pgcelllimit));
        }

        /* PGCELLLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGCELLLIMITr(unit, mport, index, pgcelllimit);
            sal_printf("COSQ %d PGCELLLIMIT.CELLRESETLIMIT 0x%x\n", index,
                PGCELLLIMITr_CELLRESETLIMITf_GET(pgcelllimit));
        }

        /* PGDISCARDSETLIMITr, index 0 ~ 7 */
        for (index = 0; index <= 7; index++) {
            READ_PGDISCARDSETLIMITr(unit, mport, index, pgdiscardsetlimit);
            sal_printf("COSQ %d PGDISCARDSETLIMIT.DISCARDSETLIMIT 0x%x\n",
                index,
                PGDISCARDSETLIMITr_DISCARDSETLIMITf_GET(pgdiscardsetlimit));
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSMINXQCNTr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSMINXQCNTr(unit, mport, index, holcosminxqcnt);
                sal_printf("COSQ %d HOLCOSMINXQCNT.HOLCOSMINXQCNT 0x%x\n",
                    index,
                    HOLCOSMINXQCNTr_HOLCOSMINXQCNTf_GET(holcosminxqcnt));
            }
        } else {
            /* HOLCOSMINXQCNT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_HOLCOSMINXQCNT_QLAYERr(unit, mport, index,
                    holcosminxqcnt_qlayer);
                sal_printf("COSQ %d HOLCOSMINXQCNT_QLAYER.HOLCOSMINXQCNT \
                    0x%x\n", index, HOLCOSMINXQCNT_QLAYERr_HOLCOSMINXQCNTf_GET(
                    holcosminxqcnt_qlayer));
            }
        }
        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMITr(unit, mport, index, holcospktsetlimit);
                sal_printf("COSQ %d HOLCOSPKTSETLIMIT.PKTSETLIMIT 0x%x\n",
                    index,
                    HOLCOSPKTSETLIMITr_PKTSETLIMITf_GET(holcospktsetlimit));
            }
        } else {
            /* HOLCOSPKTSETLIMIT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_HOLCOSPKTSETLIMIT_QLAYERr(unit, mport, index,
                    holcospktsetlimit_qlayer);
                sal_printf("COSQ %d HOLCOSPKTSETLIMIT_QLAYER.PKTSETLIMIT \
                    0x%x\n", index, HOLCOSPKTSETLIMIT_QLAYERr_PKTSETLIMITf_GET(
                    holcospktsetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSPKTRESETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMITr(unit, mport, index,
                    holcospktresetlimit);
                sal_printf("COSQ %d HOLCOSPKTRESETLIMIT.PKTRESETLIMIT 0x%x\n",
                    index,
                    HOLCOSPKTRESETLIMITr_PKTRESETLIMITf_GET(holcospktresetlimit)
                    );
            }
        } else {
            /* HOLCOSPKTRESETLIMIT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_HOLCOSPKTRESETLIMIT_QLAYERr(unit, mport, index,
                    holcospktresetlimit_qlayer);
                sal_printf("COSQ %d HOLCOSPKTRESETLIMIT_QLAYER.PKTRESETLIMIT \
                    0x%x\n", index,
                    HOLCOSPKTRESETLIMIT_QLAYERr_PKTRESETLIMITf_GET(
                    holcospktresetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* CNGCOSPKTLIMIT0r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0r(unit, mport, index, cngcospktlimit0);
                sal_printf("COSQ %d CNGCOSPKTLIMIT0.CNGPKTSETLIMIT0 0x%x\n",
                    index, CNGCOSPKTLIMIT0r_CNGPKTSETLIMIT0f_GET(
                    cngcospktlimit0));
            }

            /* CNGCOSPKTLIMIT1r, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1r(unit, mport, index, cngcospktlimit1);
                sal_printf("COSQ %d CNGCOSPKTLIMIT1.CNGPKTSETLIMIT1 0x%x\n",
                index, CNGCOSPKTLIMIT1r_CNGPKTSETLIMIT1f_GET(cngcospktlimit1));
            }
        } else {
            /* CNGCOSPKTLIMIT0_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_CNGCOSPKTLIMIT0_QLAYERr(unit, mport, index,
                    cngcospktlimit0_qlayer);
                sal_printf("COSQ %d CNGCOSPKTLIMIT0_QLAYER.CNGPKTSETLIMIT0 \
                    0x%x\n",
                    index, CNGCOSPKTLIMIT0_QLAYERr_CNGPKTSETLIMIT0f_GET(
                    cngcospktlimit0_qlayer));
            }

            /* CNGCOSPKTLIMIT1_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63 ; index++) {
                READ_CNGCOSPKTLIMIT1_QLAYERr(unit, mport, index,
                    cngcospktlimit1_qlayer);
                sal_printf("COSQ %d CNGCOSPKTLIMIT1_QLAYER.CNGPKTSETLIMIT1 \
                    0x%x\n", index,
                    CNGCOSPKTLIMIT1_QLAYERr_CNGPKTSETLIMIT1f_GET(
                    cngcospktlimit1_qlayer));
            }
        }

        /* CNGPORTPKTLIMIT0r, index 0 */
        READ_CNGPORTPKTLIMIT0r(unit, mport, cngportpktlimit0);
        sal_printf("CNGPORTPKTLIMIT0.CNGPORTPKTLIMIT0 0x%x\n",
            CNGPORTPKTLIMIT0r_CNGPORTPKTLIMIT0f_GET(cngportpktlimit0));

        /* CNGPORTPKTLIMIT1r, index 0 */
        READ_CNGPORTPKTLIMIT1r(unit, mport, cngportpktlimit1);
        sal_printf("CNGPORTPKTLIMIT1.CNGPORTPKTLIMIT1 0x%x\n",
            CNGPORTPKTLIMIT1r_CNGPORTPKTLIMIT1f_GET(cngportpktlimit1));

        /* DYNXQCNTPORTr, index 0 */
        READ_DYNXQCNTPORTr(unit, mport, dynxqcntport);
        sal_printf("DYNXQCNTPORT.DYNXQCNTPORT 0x%x\n",
            DYNXQCNTPORTr_DYNXQCNTPORTf_GET(dynxqcntport));

        /* DYNRESETLIMPORTr, index 0 */
        READ_DYNRESETLIMPORTr(unit, mport, dynresetlimport);
        sal_printf("DYNRESETLIMPORT.DYNRESETLIMPORT 0x%x\n",
            DYNRESETLIMPORTr_DYNRESETLIMPORTf_GET(dynresetlimport));

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index,
                    lwmcoscellsetlimit);
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT.CELLSETLIMIT 0x%x\n",
                    index,
                    LWMCOSCELLSETLIMITr_CELLSETLIMITf_GET(lwmcoscellsetlimit));
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                    lwmcoscellsetlimit_qlayer);
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT_QLAYER.CELLSETLIMIT \
                    0x%x\n", index,
                    LWMCOSCELLSETLIMIT_QLAYERr_CELLSETLIMITf_GET(
                    lwmcoscellsetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* LWMCOSCELLSETLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_LWMCOSCELLSETLIMITr(unit, mport, index,
                    lwmcoscellsetlimit);
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT.CELLRESETLIMIT \
                    0x%x\n", index,
                    LWMCOSCELLSETLIMITr_CELLRESETLIMITf_GET(
                    lwmcoscellsetlimit));
            }
        } else {
            /* LWMCOSCELLSETLIMIT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_LWMCOSCELLSETLIMIT_QLAYERr(unit, mport, index,
                    lwmcoscellsetlimit_qlayer);
                sal_printf("COSQ %d LWMCOSCELLSETLIMIT_QLAYER.CELLRESETLIMIT \
                0x%x\n", index,
                LWMCOSCELLSETLIMIT_QLAYERr_CELLRESETLIMITf_GET(
                lwmcoscellsetlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index,
                    holcoscellmaxlimit);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT.CELLMAXLIMIT 0x%x\n",
                    index, HOLCOSCELLMAXLIMITr_CELLMAXLIMITf_GET(
                    holcoscellmaxlimit));
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                    holcoscellmaxlimit_qlayer);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QLAYER.CELLMAXLIMIT \
                    0x%x\n", index,
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXLIMITf_GET(
                    holcoscellmaxlimit_qlayer));
            }
        }

        if (mport < MMU_64Q_PPORT_BASE) {
            /* HOLCOSCELLMAXLIMITr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMITr(unit, mport, index,
                    holcoscellmaxlimit);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT.CELLMAXRESUMELIMIT \
                    0x%x\n", index,
                    HOLCOSCELLMAXLIMITr_CELLMAXRESUMELIMITf_GET(
                    holcoscellmaxlimit));
            }
        } else {
            /* HOLCOSCELLMAXLIMIT_QLAYERr, index 0 ~ 63 */
            for (index = 0; index <= 63; index++) {
                READ_HOLCOSCELLMAXLIMIT_QLAYERr(unit, mport, index,
                    holcoscellmaxlimit_qlayer);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QLAYER.\
                    CELLMAXRESUMELIMIT 0x%x\n", index,
                    HOLCOSCELLMAXLIMIT_QLAYERr_CELLMAXRESUMELIMITf_GET(
                    holcoscellmaxlimit_qlayer));
            }
        }

        /* DYNCELLLIMITr, index 0 */
        READ_DYNCELLLIMITr(unit, mport, dyncelllimit);
        sal_printf("DYNCELLLIMIT.DYNCELLSETLIMIT 0x%x\n",
            DYNCELLLIMITr_DYNCELLSETLIMITf_GET(dyncelllimit));
        sal_printf("DYNCELLLIMIT.DYNCELLRESETLIMIT 0x%x\n",
            DYNCELLLIMITr_DYNCELLRESETLIMITf_GET(dyncelllimit));

        if (mport < MMU_64Q_PPORT_BASE) {
            /* COLOR_DROP_ENr, index 0 */
            READ_COLOR_DROP_ENr(unit, mport, color_drop_en);
            sal_printf("COLOR_DROP_EN.COLOR_DROP_EN 0x%x\n",
                COLOR_DROP_ENr_COLOR_DROP_ENf_GET(color_drop_en));
        } else {
            /* COLOR_DROP_EN_QLAYERr, index 0 ~ 1*/
            for (index = 0; index <= 1; index++) {
                READ_COLOR_DROP_EN_QLAYERr(unit, mport, index, color_drop_en_qlayer);
                sal_printf("COLOR_DROP_EN_QLAYER.COLOR_DROP_EN 0x%x\n",
                    COLOR_DROP_EN_QLAYERr_COLOR_DROP_ENf_GET(color_drop_en_qlayer));
            }

            /* HOLCOSPKTSETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTSETLIMIT_QGROUPr(unit, mport, index,
                    holcospktsetlimit_qgroup);
                sal_printf("COSQ %d HOLCOSPKTSETLIMIT_QGROUP.PKTSETLIMIT \
                    0x%x\n", index,
                    HOLCOSPKTSETLIMIT_QGROUPr_PKTSETLIMITf_GET(
                    holcospktsetlimit_qgroup));
            }

            /* HOLCOSPKTRESETLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSPKTRESETLIMIT_QGROUPr(unit, mport, index,
                    holcospktresetlimit_qgroup);
                sal_printf("COSQ %d HOLCOSPKTRESETLIMIT_QGROUP.PKTRESETLIMIT \
                    0x%x\n", index,
                    HOLCOSPKTRESETLIMIT_QGROUPr_PKTRESETLIMITf_GET(
                    holcospktresetlimit_qgroup));
            }

            /* CNGCOSPKTLIMIT0_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT0_QGROUPr(unit, mport, index,
                    cngcospktlimit0_qgroup);
                sal_printf("COSQ %d CNGCOSPKTLIMIT0_QGROUP.CNGPKTSETLIMIT0 \
                    0x%x\n", index,
                    CNGCOSPKTLIMIT0_QGROUPr_CNGPKTSETLIMIT0f_GET(
                    cngcospktlimit0_qgroup));
            }

            /* CNGCOSPKTLIMIT1_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_CNGCOSPKTLIMIT1_QGROUPr(unit, mport, index,
                    cngcospktlimit1_qgroup);
                sal_printf("COSQ %d CNGCOSPKTLIMIT1_QGROUP.NGPKTSETLIMIT1 \
                0x%x\n", index,
                CNGCOSPKTLIMIT1_QGROUPr_CNGPKTSETLIMIT1f_GET(
                cngcospktlimit1_qgroup));
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index,
                    holcoscellmaxlimit_qgroup);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QGROUP.CELLMAXLIMIT \
                    0x%x\n", index,
                    HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXLIMITf_GET(
                    holcoscellmaxlimit_qgroup));
            }

            /* HOLCOSCELLMAXLIMIT_QGROUPr, index 0 ~ 7 */
            for (index = 0; index <= 7; index++) {
                READ_HOLCOSCELLMAXLIMIT_QGROUPr(unit, mport, index,
                    holcoscellmaxlimit_qgroup);
                sal_printf("COSQ %d HOLCOSCELLMAXLIMIT_QGROUP.\
                    CELLMAXRESUMELIMIT 0x%x\n", index,
                    HOLCOSCELLMAXLIMIT_QGROUPr_CELLMAXRESUMELIMITf_GET(
                    holcoscellmaxlimit_qgroup));
            }

            /* COLOR_DROP_EN_QGROUPr, index 0 */
            READ_COLOR_DROP_EN_QGROUPr(unit, mport, color_drop_en_qgroup);
            sal_printf("COLOR_DROP_EN_QGROUP.COLOR_DROP_EN 0x%x\n",
                COLOR_DROP_EN_QGROUPr_COLOR_DROP_ENf_GET(color_drop_en_qgroup));
        }

        /* SHARED_POOL_CTRLr, index 0 */
        READ_SHARED_POOL_CTRLr(unit, mport, shared_pool_ctrl);
        sal_printf("SHARED_POOL_CTRL.DYNAMIC_COS_DROP_EN 0x%x\n",
            SHARED_POOL_CTRLr_DYNAMIC_COS_DROP_ENf_GET(shared_pool_ctrl));
        sal_printf("SHARED_POOL_CTRL.SHARED_POOL_DISCARD_EN 0x%x\n",
            SHARED_POOL_CTRLr_SHARED_POOL_DISCARD_ENf_GET(shared_pool_ctrl));
        sal_printf("SHARED_POOL_CTRL.SHARED_POOL_XOFF_EN 0x%x\n",
            SHARED_POOL_CTRLr_SHARED_POOL_XOFF_ENf_GET(shared_pool_ctrl));

        if (mport >= MMU_64Q_PPORT_BASE) {
            /* SHARED_POOL_CTRL_EXT1r, index 0 */
            READ_SHARED_POOL_CTRL_EXT1r(unit, mport, shared_pool_ctrl_ext1);
            sal_printf("SHARED_POOL_CTRL_EXT1.DYNAMIC_COS_DROP_EN 0x%x\n",
                SHARED_POOL_CTRL_EXT1r_DYNAMIC_COS_DROP_ENf_GET(
                shared_pool_ctrl_ext1));

            /* SHARED_POOL_CTRL_EXT2r, index 0 */
            READ_SHARED_POOL_CTRL_EXT2r(unit, mport, shared_pool_ctrl_ext2);
            sal_printf("SHARED_POOL_CTRL_EXT2.DYNAMIC_COS_DROP_EN 0x%x\n",
                SHARED_POOL_CTRL_EXT2r_DYNAMIC_COS_DROP_ENf_GET(
                shared_pool_ctrl_ext2));
        }

        /* E2ECC_PORT_CONFIGr, index 0 */
        READ_E2ECC_PORT_CONFIGr(unit, mport, e2ecc_port_cfg);
        sal_printf("E2ECC_PORT_CONFIG.COS_CELL_EN 0x%x\n",
            E2ECC_PORT_CONFIGr_COS_CELL_ENf_GET(e2ecc_port_cfg));
        sal_printf("E2ECC_PORT_CONFIG.COS_PKT_EN 0x%x\n",
            E2ECC_PORT_CONFIGr_COS_PKT_ENf_GET(e2ecc_port_cfg));

        /* EARLY_DYNCELLLIMITr, index 0 */
        READ_EARLY_DYNCELLLIMITr(unit, mport, early_dyncelllimit);
        sal_printf("EARLY_DYNCELLLIMIT.EARLY_DYNCELLSETLIMIT 0x%x\n",
            EARLY_DYNCELLLIMITr_EARLY_DYNCELLSETLIMITf_GET(early_dyncelllimit));

        for (index = 0; index <= 7; index++) {
            READ_EARLY_HOLCOSCELLMAXLIMITr(unit, mport, index,
                early_holcoscellmaxlimit);
            sal_printf("EARLY_HOLCOSCELLMAXLIMIT.EARLY_CELLMAXLIMIT 0x%x\n",
                EARLY_HOLCOSCELLMAXLIMITr_EARLY_CELLMAXLIMITf_GET(
                early_holcoscellmaxlimit));
        }
    }
}
#endif /* CFG_MMU_DEBUG */

static sys_error_t
soc_misc_init(uint8 unit)
{
#define NUM_XLPORT 4
    int i, lport, pport;

    GPORT_CONFIGr_t gport_config;
    MISCCONFIGr_t miscconfig;
    L2_AUX_HASH_CONTROLr_t l2_aux_hash_control;
    L3_AUX_HASH_CONTROLr_t l3_aux_hash_control;
    EGR_ENABLEm_t egr_enable;
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
    SOC_IF_ERROR_RETURN(soc_pipe_mem_clear(unit));

    SOC_IF_ERROR_RETURN(soc_init_port_mapping(unit));

    /* Reset XLPORT and CLPORT MIB counter */
    SOC_LPORT_ITER(lport) {
        if (IS_XL_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            XLPORT_MIB_RESETr_CLR(xlport_mib_reset);
            XLPORT_MIB_RESETr_CLR_CNTf_SET(xlport_mib_reset, 0xf);
            SOC_IF_ERROR_RETURN(
                WRITE_XLPORT_MIB_RESETr(unit, lport, xlport_mib_reset));
            XLPORT_MIB_RESETr_CLR_CNTf_SET(xlport_mib_reset, 0);
            SOC_IF_ERROR_RETURN(
                WRITE_XLPORT_MIB_RESETr(unit, lport, xlport_mib_reset));
        }

        if (IS_CL_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            CLPORT_MIB_RESETr_CLR(clport_mib_reset);
            CLPORT_MIB_RESETr_CLR_CNTf_SET(clport_mib_reset, 0xf);
            SOC_IF_ERROR_RETURN(
                WRITE_CLPORT_MIB_RESETr(unit, lport, clport_mib_reset));
            CLPORT_MIB_RESETr_CLR_CNTf_SET(clport_mib_reset, 0);
            SOC_IF_ERROR_RETURN(
                WRITE_CLPORT_MIB_RESETr(unit, lport, clport_mib_reset));
        }
    }

    /* GMAC init */
    GPORT_CONFIGr_CLR(gport_config);
    GPORT_CONFIGr_CLR_CNTf_SET(gport_config, 1);
    GPORT_CONFIGr_GPORT_ENf_SET(gport_config, 1);
    SOC_LPORT_ITER(lport) {
        if (IS_GX_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            /* Clear counter and enable gport */
            SOC_IF_ERROR_RETURN(
                WRITE_GPORT_CONFIGr(unit, lport, gport_config));
        }
    }

    GPORT_CONFIGr_CLR_CNTf_SET(gport_config, 0);
    SOC_LPORT_ITER(lport) {
        if (IS_GX_PORT(lport) && (SOC_PORT_BLOCK_INDEX(lport) == 0)) {
            /* Enable gport */
            SOC_IF_ERROR_RETURN(
                WRITE_GPORT_CONFIGr(unit, lport, gport_config));
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
            SOC_IF_ERROR_RETURN(
                WRITE_XLPORT_SOFT_RESETr(unit, lport, xlport_sreset));

            XLPORT_MODE_REGr_CLR(xlport_mode);
            block_start_port = pport - SOC_PORT_BLOCK_INDEX(lport);
            if (pport == block_start_port) {
                port_mode = SOC_PORT_MODE(lport);
                XLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(xlport_mode, port_mode);
                XLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(xlport_mode, port_mode);
                SOC_IF_ERROR_RETURN(
                    WRITE_XLPORT_MODE_REGr(unit, lport, xlport_mode));
            }


            /* De-assert XLPORT soft reset */
            XLPORT_SOFT_RESETr_CLR(xlport_sreset);
            SOC_IF_ERROR_RETURN(
                WRITE_XLPORT_SOFT_RESETr(unit, lport, xlport_sreset));

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
            SOC_IF_ERROR_RETURN(
                WRITE_XLPORT_ENABLE_REGr(unit, lport, xlport_enable));
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
            SOC_IF_ERROR_RETURN(
                WRITE_CLPORT_SOFT_RESETr(unit, lport, clport_sreset));

            CLPORT_MODE_REGr_CLR(clport_mode);
            block_start_port = pport - SOC_PORT_BLOCK_INDEX(lport);
            if (SOC_PORT_BLOCK_INDEX(lport) == 0) {
                port_mode = SOC_PORT_MODE(lport);
                CLPORT_MODE_REGr_XPORT0_PHY_PORT_MODEf_SET(clport_mode, port_mode);
                CLPORT_MODE_REGr_XPORT0_CORE_PORT_MODEf_SET(clport_mode, port_mode);
                SOC_IF_ERROR_RETURN(
                    WRITE_CLPORT_MODE_REGr(unit, lport, clport_mode));
            }


            /* De-assert XLPORT soft reset */
            CLPORT_SOFT_RESETr_CLR(clport_sreset);
            SOC_IF_ERROR_RETURN(
                WRITE_CLPORT_SOFT_RESETr(unit, lport, clport_sreset));

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
            SOC_IF_ERROR_RETURN(
                WRITE_CLPORT_ENABLE_REGr(unit, lport, clport_enable));
        }
    }

    SOC_IF_ERROR_RETURN(
        READ_MISCCONFIGr(unit, miscconfig));
    MISCCONFIGr_METERING_CLK_ENf_SET(miscconfig, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_MISCCONFIGr(unit, miscconfig));

    /* Enable dual hash on L2 and L3 tables with CRC32_LOWER (2) */
    L2_AUX_HASH_CONTROLr_CLR(l2_aux_hash_control);
    L2_AUX_HASH_CONTROLr_ENABLEf_SET(l2_aux_hash_control, 1);
    L2_AUX_HASH_CONTROLr_HASH_SELECTf_SET(l2_aux_hash_control, 2);
    L2_AUX_HASH_CONTROLr_INSERT_LEAST_FULL_HALFf_SET(l2_aux_hash_control, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_L2_AUX_HASH_CONTROLr(unit, l2_aux_hash_control));

    L3_AUX_HASH_CONTROLr_CLR(l3_aux_hash_control);
    L3_AUX_HASH_CONTROLr_ENABLEf_SET(l3_aux_hash_control, 1);
    L3_AUX_HASH_CONTROLr_HASH_SELECTf_SET(l3_aux_hash_control, 2);
    L3_AUX_HASH_CONTROLr_INSERT_LEAST_FULL_HALFf_SET(l3_aux_hash_control, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_L3_AUX_HASH_CONTROLr(unit, l3_aux_hash_control));

    /* Egress Enable */
    EGR_ENABLEm_CLR(egr_enable);
    EGR_ENABLEm_PRT_ENABLEf_SET(egr_enable, 1);
    SOC_LPORT_ITER(lport) {
        SOC_IF_ERROR_RETURN(
            WRITE_EGR_ENABLEm(unit, SOC_PORT_L2P_MAPPING(lport), egr_enable));
    }

    /* The HW defaults for EGR_VLAN_CONTROL_1.VT_MISS_UNTAG == 1, which
     * causes the outer tag to be removed from packets that don't have
     * a hit in the egress vlan tranlation table. Set to 0 to disable this.
     */
    EGR_VLAN_CONTROL_1r_CLR(egr_vlan_control_1);
    SOC_LPORT_ITER(lport) {
        SOC_IF_ERROR_RETURN(
            WRITE_EGR_VLAN_CONTROL_1r(unit, lport, egr_vlan_control_1));
    }

    /* Enable SRC_REMOVAL_EN and LAG_RES_EN */
    SW2_FP_DST_ACTION_CONTROLr_CLR(sw2_fp_dst_action_control);
    SW2_FP_DST_ACTION_CONTROLr_SRC_REMOVAL_ENf_SET(sw2_fp_dst_action_control, 1);
    SW2_FP_DST_ACTION_CONTROLr_LAG_RES_ENf_SET(sw2_fp_dst_action_control, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_SW2_FP_DST_ACTION_CONTROLr(unit, sw2_fp_dst_action_control));

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
    int i, tdm_size, length;
    uint32 val;
    uint32 speed_sum = 0;

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

        tcfg->port_state[phy_port] = PORT_STATE__LINERATE;
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
    chip_pkg->state[CMIC_PPORT] = PORT_STATE__LINERATE;
    for (idx = 0; idx < NUM_EXT_PORTS; idx ++) {
        chip_pkg->speed[idx] = tcfg->speed[idx];
        if (idx != CMIC_PPORT) {
            /* Don't count the speed of CPU port in speed_sum */
            speed_sum = speed_sum + tcfg->speed[idx];
        }
    }

    /* chip_pkg->soc_vars.fl.mgmt_pm_hg = 0; */

    chip_pkg->soc_vars.fl.tdm_chk_en = 1;

    if (speed_sum > 420000) {
        /* Don't need to support HG if speed_sum greater than 420Gbps */
        chip_pkg->soc_vars.fl.cal_hg_en = 0;
    } else {
        chip_pkg->soc_vars.fl.cal_hg_en = 1;
    }

    bcm5607x_a0_sel_tdm(chip_pkg, tdm_pkg);
    if (NULL == bcm5607x_a0_set_tdm_tbl(tdm_pkg)) {
        sal_printf("bcm5607x_a0_init[%d]: Unable to configure TDM\n", unit);
        sal_free(tdm_pkg);
        return SYS_ERR;
    }
    tdm_size = tdm_pkg->_chip_data.cal_0.cal_len;
    sal_memcpy(tcfg->idb_tdm_tbl_0, tdm_pkg->_chip_data.cal_0.cal_main, \
                   sizeof(int)*FL_LR_VBS_LEN);

    for (length = tdm_size; length > 0; length--) {
        if (tcfg->idb_tdm_tbl_0[length - 1] != FL_NUM_EXT_PORTS) {
            break;
        }
    }

    TDM_DBG(("\n"));
    TDM_DBG(("tdm size: %d", length));
    TDM_DBG(("\n"));
    TDM_DBG(("tdm table:"));
    for (idx = 0; idx < length; idx++) {
        if (idx % 16 == 0) {
            TDM_DBG(("\n    "));
        }
        TDM_DBG(("  %d", (tcfg->idb_tdm_tbl_0[idx]<0) ? 127 : tcfg->idb_tdm_tbl_0[idx]));
    }
    TDM_DBG(("\n"));

    /* DISABLE = 1, TDM_WRAP_PTR = length-1 */
    SOC_IF_ERROR_RETURN(
        READ_IARB_TDM_CONTROLr(unit, iarb_tdm_control));
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_control, 1);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_control, (length-1));
    SOC_IF_ERROR_RETURN(
        WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_control));

    for (i = 0; i < length; i++) {
        IARB_TDM_TABLEm_CLR(iarb_tdm_table);
        IARB_TDM_TABLEm_PORT_NUMf_SET(iarb_tdm_table, tcfg->idb_tdm_tbl_0[i]);
        SOC_IF_ERROR_RETURN(
            WRITE_IARB_TDM_TABLEm(unit, i, iarb_tdm_table));

        if (tcfg->idb_tdm_tbl_0[i] == -1) {
            /* Convert IDLE slot definition */
            tcfg->idb_tdm_tbl_0[i] = 127;
        }

        val = (tcfg->idb_tdm_tbl_0[i] != 127) ? \
              SOC_PORT_P2M_MAPPING(tcfg->idb_tdm_tbl_0[i]) : 127;
        MMU_ARB_TDM_TABLEm_CLR(mmu_arb_tdm_table);
        MMU_ARB_TDM_TABLEm_PORT_NUMf_SET(mmu_arb_tdm_table, val);
        if (i == (length - 1)) {
            /* WRAP_EN = 1 */
            MMU_ARB_TDM_TABLEm_WRAP_ENf_SET(mmu_arb_tdm_table, 1);
        }
        SOC_IF_ERROR_RETURN(
            WRITE_MMU_ARB_TDM_TABLEm(unit, i, mmu_arb_tdm_table));
    }

    /* DISABLE = 0, TDM_WRAP_PTR = length-1 */
    IARB_TDM_CONTROLr_DISABLEf_SET(iarb_tdm_control, 0);
    IARB_TDM_CONTROLr_TDM_WRAP_PTRf_SET(iarb_tdm_control, (length - 1));
    SOC_IF_ERROR_RETURN(
        WRITE_IARB_TDM_CONTROLr(unit, iarb_tdm_control));

    if (tdm_pkg != NULL) {
        tdm_fl_main_free(tdm_pkg);
        sal_free(tdm_pkg);
    }

    return ioerr;
}

static sys_error_t
soc_mmu_init(uint8 unit)
{
    sys_error_t rv = SYS_OK;
    int pport;
    pbmp_t lpbmp, mmu_lpbmp;
    uint32 val, oval, cfap_max_idx;
    CFAPCONFIGr_t cfapconfig;
    MMUPORTENABLE_0r_t mmuportenable_0;
    MMUPORTENABLE_1r_t mmuportenable_1;
    MMUPORTENABLE_2r_t mmuportenable_2;

    cfap_max_idx = FL_MMU_CBP_FULL_SIZE;

    SOC_IF_ERROR_RETURN(READ_CFAPCONFIGr(unit, cfapconfig));
    oval = CFAPCONFIGr_GET(cfapconfig);
    CFAPCONFIGr_CFAPPOOLSIZEf_SET(cfapconfig, cfap_max_idx);
    val = CFAPCONFIGr_GET(cfapconfig);
    if (oval != val) {
        SOC_IF_ERROR_RETURN(WRITE_CFAPCONFIGr(unit, cfapconfig));
    }

    /* default is custom chassis mode, if specify mmu_mode=chassis_brcm
       it will goes to standard chassis mode.
     */
    const char *str_val;
    str_val = sal_config_get(SAL_CONFIG_MMU_MODE);
    if (str_val != NULL && (sal_strcmp((const char *)str_val, "chassis_brcm") == 0)) {
        rv = _soc_firelight_mmu_init_helper_chassis(unit);
    } else if (str_val != NULL && (sal_strcmp((const char *)str_val, "passthru") == 0)) {
        rv = _soc_firelight_mmu_init_bump_on_wire(unit);
    } else {
        rv = _soc_firelight_mmu_init_helper_custom_chassis(unit);
    }

#if CFG_MMU_DEBUG
    _soc_firelight_mmu_init_debug(unit);
#endif /* CFG_MMU_DEBUG */

    /* Port enable */
    PBMP_ASSIGN(lpbmp, BCM5607X_ALL_PORTS_MASK);
#if CFG_RXTX_SUPPORT_ENABLED
    PBMP_PORT_ADD(lpbmp, 0);
#endif  /* CFG_RXTX_SUPPORT_ENABLED */
    PBMP_CLEAR(mmu_lpbmp);
    PBMP_ITER(lpbmp, pport) {
         PBMP_PORT_ADD(mmu_lpbmp, SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(pport)));
    }
    /* Add CPU port */
    MMUPORTENABLE_0r_CLR(mmuportenable_0);
    MMUPORTENABLE_0r_MMUPORTENABLEf_SET(mmuportenable_0,
                                        PBMP_WORD_GET(mmu_lpbmp, 0));
    SOC_IF_ERROR_RETURN(WRITE_MMUPORTENABLE_0r(unit, mmuportenable_0));

    MMUPORTENABLE_1r_CLR(mmuportenable_1);
    MMUPORTENABLE_1r_MMUPORTENABLEf_SET(mmuportenable_1,
                                        PBMP_WORD_GET(mmu_lpbmp, 1));
    SOC_IF_ERROR_RETURN(WRITE_MMUPORTENABLE_1r(unit, mmuportenable_1));

    MMUPORTENABLE_2r_CLR(mmuportenable_2);
    MMUPORTENABLE_2r_MMUPORTENABLEf_SET(mmuportenable_2,
                                        PBMP_WORD_GET(mmu_lpbmp, 2));
    SOC_IF_ERROR_RETURN(WRITE_MMUPORTENABLE_2r(unit, mmuportenable_2));

    return rv;
}

static void
config_schedule_mode(uint8 unit)
{
    int lport, pport, mmu_port, cos_queue, weight_val;
    pbmp_t pbmp_uplink;
    int wrr_weight[8]={2, 2, 2, 3, 3, 0, 0, 0};

    XQCOSARBSELr_t xqcosarbsel;
    WRRWEIGHT_COS0r_t wrrweight_cos0;
    WRRWEIGHT_COS1r_t wrrweight_cos1;
    WRRWEIGHT_COS2r_t wrrweight_cos2;
    WRRWEIGHT_COS3r_t wrrweight_cos3;
    WRRWEIGHT_COS4r_t wrrweight_cos4;
    WRRWEIGHT_COS5r_t wrrweight_cos5;
    WRRWEIGHT_COS6r_t wrrweight_cos6;
    WRRWEIGHT_COS7r_t wrrweight_cos7;
    XQCOSARBSEL_QLAYERr_t xqcosarbsel_qlayer;
    WRRWEIGHT_COS_QLAYERr_t wrrweight_cos_qlayer;
    MMU_MAX_BUCKET_QLAYERm_t mmu_max_bucket_qlayer;

    pbmp_uplink = _uplink_pbmp();
    /* MAX_THD_SEL = 0 : Disable MAX shaper */
    MMU_MAX_BUCKET_QLAYERm_CLR(mmu_max_bucket_qlayer);

    SOC_LPORT_ITER(lport) {
        /* MMU to Physical  port */
        pport = SOC_PORT_L2P_MAPPING(lport);
        /* Physical to Logical port */
        mmu_port = SOC_PORT_P2M_MAPPING(pport);
        if (mmu_port < MMU_64Q_PPORT_BASE) {
            /* Legacy 8x1 scheduling */

            
            WRRWEIGHT_COS0r_CLR(wrrweight_cos0);
            WRRWEIGHT_COS0r_ENABLEf_SET(wrrweight_cos0, 1);
            if (PBMP_MEMBER(pbmp_uplink, lport)) {
                WRRWEIGHT_COS0r_WEIGHTf_SET(wrrweight_cos0, 1);
            } else {
                WRRWEIGHT_COS0r_WEIGHTf_SET(wrrweight_cos0, wrr_weight[0]);
            }
            WRITE_WRRWEIGHT_COS0r(unit, mmu_port, wrrweight_cos0);

            WRRWEIGHT_COS1r_CLR(wrrweight_cos1);
            WRRWEIGHT_COS1r_ENABLEf_SET(wrrweight_cos1, 1);
            if (PBMP_MEMBER(pbmp_uplink, lport)) {
                WRRWEIGHT_COS1r_WEIGHTf_SET(wrrweight_cos1, 1);
            } else {
                WRRWEIGHT_COS1r_WEIGHTf_SET(wrrweight_cos1, wrr_weight[1]);
            }
            WRITE_WRRWEIGHT_COS1r(unit, mmu_port, wrrweight_cos1);

            WRRWEIGHT_COS2r_CLR(wrrweight_cos2);
            WRRWEIGHT_COS2r_ENABLEf_SET(wrrweight_cos2, 1);
            if (PBMP_MEMBER(pbmp_uplink, lport)) {
                WRRWEIGHT_COS2r_WEIGHTf_SET(wrrweight_cos2, 1);
            } else {
                WRRWEIGHT_COS2r_WEIGHTf_SET(wrrweight_cos2, wrr_weight[2]);
            }
            WRITE_WRRWEIGHT_COS2r(unit, mmu_port, wrrweight_cos2);

            WRRWEIGHT_COS3r_CLR(wrrweight_cos3);
            WRRWEIGHT_COS3r_ENABLEf_SET(wrrweight_cos3, 1);
            if (PBMP_MEMBER(pbmp_uplink, lport)) {
                WRRWEIGHT_COS3r_WEIGHTf_SET(wrrweight_cos3, 1);
            } else {
                WRRWEIGHT_COS3r_WEIGHTf_SET(wrrweight_cos3, wrr_weight[3]);
            }
            WRITE_WRRWEIGHT_COS3r(unit, mmu_port, wrrweight_cos3);

            WRRWEIGHT_COS4r_CLR(wrrweight_cos4);
            WRRWEIGHT_COS4r_ENABLEf_SET(wrrweight_cos4, 1);
            if (PBMP_MEMBER(pbmp_uplink, lport)) {
                WRRWEIGHT_COS4r_WEIGHTf_SET(wrrweight_cos4, 1);
            } else {
                WRRWEIGHT_COS4r_WEIGHTf_SET(wrrweight_cos4, wrr_weight[4]);
            }
            WRITE_WRRWEIGHT_COS4r(unit, mmu_port, wrrweight_cos4);

            WRRWEIGHT_COS5r_CLR(wrrweight_cos5);
            WRRWEIGHT_COS5r_ENABLEf_SET(wrrweight_cos5, 1);
            if (PBMP_MEMBER(pbmp_uplink, lport)) {
                WRRWEIGHT_COS5r_WEIGHTf_SET(wrrweight_cos5, 1);
            } else {
                WRRWEIGHT_COS5r_WEIGHTf_SET(wrrweight_cos5, wrr_weight[5]);
            }
            WRITE_WRRWEIGHT_COS5r(unit, mmu_port, wrrweight_cos5);

            WRRWEIGHT_COS6r_CLR(wrrweight_cos6);
            WRRWEIGHT_COS6r_ENABLEf_SET(wrrweight_cos6, 1);
            if (PBMP_MEMBER(pbmp_uplink, lport)) {
                WRRWEIGHT_COS6r_WEIGHTf_SET(wrrweight_cos6, 1);
            } else {
                WRRWEIGHT_COS6r_WEIGHTf_SET(wrrweight_cos6, wrr_weight[6]);
            }
            WRITE_WRRWEIGHT_COS6r(unit, mmu_port, wrrweight_cos6);

            WRRWEIGHT_COS7r_CLR(wrrweight_cos7);
            WRRWEIGHT_COS7r_ENABLEf_SET(wrrweight_cos7, 1);
            if (PBMP_MEMBER(pbmp_uplink, lport)) {
                WRRWEIGHT_COS7r_WEIGHTf_SET(wrrweight_cos7, 1);
            } else {
                WRRWEIGHT_COS7r_WEIGHTf_SET(wrrweight_cos7, wrr_weight[7]);
            }
            WRITE_WRRWEIGHT_COS7r(unit, mmu_port, wrrweight_cos7);

            XQCOSARBSELr_CLR(xqcosarbsel);
            XQCOSARBSELr_COSARBf_SET(xqcosarbsel, 3);
            XQCOSARBSELr_MTU_QUANTA_SELECTf_SET(xqcosarbsel, 3);
            WRITE_XQCOSARBSELr(unit, mmu_port, xqcosarbsel);

            for (cos_queue = 0; cos_queue < COS_QUEUE_NUM; cos_queue++) {
                WRITE_MMU_MAX_BUCKET_QLAYERm(unit, (mmu_port * 8) + cos_queue, mmu_max_bucket_qlayer);
            }
        } else {
            /* Use 8x1 scheduling for high speed port (with 64 queues) */

            
            for (cos_queue = 0; cos_queue < COS_QUEUE_NUM; cos_queue++) {
                WRRWEIGHT_COS_QLAYERr_CLR(wrrweight_cos_qlayer);
                WRRWEIGHT_COS_QLAYERr_ENABLEf_SET(wrrweight_cos_qlayer, 1);
                weight_val = 1;
                if (!PBMP_MEMBER(pbmp_uplink, lport)) {
                     weight_val = wrr_weight[cos_queue];
                }
                WRRWEIGHT_COS_QLAYERr_WEIGHTf_SET(wrrweight_cos_qlayer, weight_val);
                WRITE_WRRWEIGHT_COS_QLAYERr(unit, mmu_port, cos_queue, wrrweight_cos_qlayer);
            }

            XQCOSARBSEL_QLAYERr_CLR(xqcosarbsel_qlayer);
            XQCOSARBSEL_QLAYERr_COSARBf_SET(xqcosarbsel_qlayer, 3);
            XQCOSARBSEL_QLAYERr_MTU_QUANTA_SELECTf_SET(xqcosarbsel_qlayer, 3);
            for (cos_queue = 0; cos_queue < COS_QUEUE_NUM; cos_queue++) {
                WRITE_XQCOSARBSEL_QLAYERr(unit, mmu_port, cos_queue, xqcosarbsel_qlayer);
            }

            for (cos_queue = 0; cos_queue < COS_QUEUE_NUM; cos_queue++) {
                WRITE_MMU_MAX_BUCKET_QLAYERm(unit, MMU_64Q_PPORT_BASE * 8 + ((mmu_port - MMU_64Q_PPORT_BASE) * 64) + cos_queue, mmu_max_bucket_qlayer);
            }
        }
    }
}

static void
fl_cosq_perq_xmt_counter_base_init(uint8 unit)
{
    PERQ_XMT_COUNTER_BASEr_t    perq_xmt_counter_base;
    int lport, pport, mmu_port;

    SOC_LPORT_ITER(lport) {
        PERQ_XMT_COUNTER_BASEr_CLR(perq_xmt_counter_base);
        /* MMU to Physical  port */
        pport = SOC_PORT_L2P_MAPPING(lport);
        /* Physical to Logical port */
        mmu_port = SOC_PORT_P2M_MAPPING(pport);
        /* MMU port >= 58 got 64 queues */
        if (mmu_port > 58) {
            PERQ_XMT_COUNTER_BASEr_INDEXf_SET(perq_xmt_counter_base,
                (58 * 8) + ((mmu_port - 58) * 64));
        } else {
            PERQ_XMT_COUNTER_BASEr_INDEXf_SET(perq_xmt_counter_base,
                (mmu_port * 8));
        }

        WRITE_PERQ_XMT_COUNTER_BASEr(unit, lport , perq_xmt_counter_base);
    }
}


static sys_error_t
bcm5607x_system_init(uint8 unit)
{
    int i, j, lport;
    PORTm_t port_entry;

    uint32 dot1pmap[16] = {
        0x00000000, 0x00000001, 0x00000004, 0x00000005, 0x00000008, 0x00000009, 0x0000000c, 0x0000000d,
        0x00000010, 0x00000011, 0x00000014, 0x00000015, 0x00000018, 0x00000019, 0x0000001c, 0x0000001d
    };
#ifndef CFG_COE_INCLUDED
    uint32 drop_mac_addr1[2] = { 0xC2000001,   0x0180 };
    uint32 drop_mac_addr2[2] = { 0xC2000002,   0x0180 };
    uint32 drop_mac_addr3[2] = { 0xC200000E,   0x0180 };
    uint32 drop_mac_addr_mask[2] = { 0xFFFFFFFF,   0xFFFF };
#endif

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
#ifndef CFG_COE_INCLUDED
    L2_USER_ENTRYm_t l2_user_entry;
#endif
#ifdef CFG_SWITCH_DOS_INCLUDED
    DOS_CONTROLr_t dos_control;
    DOS_CONTROL2r_t dos_control2;
#endif /* CFG_SWITCH_DOS_INCLUDED */
    AUX_ARB_CONTROL_2r_t aux_arb_control_2;
    ING_CONFIG_64r_t ing_config_64;
    VLAN_PROFILEm_t vlan_profile;
    EGR_PORT_64r_t egr_port_64;
    L2_AGE_TIMERr_t l2_age_timer;
#if CFG_RXTX_SUPPORT_ENABLED
    EGR_VLAN_STGm_t egr_vlan_stg;
    VLAN_STGm_t vlan_stg;
    PROTOCOL_PKT_CONTROLr_t protocol_pkt_control;
    EGR_VLANm_t egr_vlan;
    VLANm_t vlan;
    pbmp_t lpbmp;
#endif

    /* ING_OUTER_TPID is allowed outer TPID values */
    SYSTEM_CONFIG_TABLEm_CLR(system_config_table);
    SYSTEM_CONFIG_TABLEm_OUTER_TPID_ENABLEf_SET(system_config_table, 1);

    SOURCE_TRUNK_MAPm_CLR(source_trunk_map);
#ifdef CFG_SWITCH_VLAN_UNAWARE_INCLUDED
    /* DISABLE_VLAN_CHECKS = 1, PACKET_MODIFICATION_DISABLE = 1 */
    SOURCE_TRUNK_MAPm_DISABLE_VLAN_CHECKSf_SET(source_trunk_map, 1);
    SOURCE_TRUNK_MAPm_PACKET_MODIFICATION_DISABLEf_SET(source_trunk_map, 1);
#endif /* CFG_SWITCH_VLAN_UNAWARE_INCLUDED */

    /* Default port_entry */
    PORTm_CLR(port_entry);
    PORTm_PORT_VIDf_SET(port_entry, 1);
    PORTm_TRUST_OUTER_DOT1Pf_SET(port_entry, 1);
    PORTm_OUTER_TPID_ENABLEf_SET(port_entry, 1);
    PORTm_TRUST_INCOMING_VIDf_SET(port_entry, 1);
#ifndef CFG_COE_INCLUDED
    /* Disable L2 learn when COE enabled */
    PORTm_CML_FLAGS_NEWf_SET(port_entry, 8);
#endif
    PORTm_CML_FLAGS_MOVEf_SET(port_entry, 8);

    /* Clear Unknown Unicast Block Mask. */
    UNKNOWN_UCAST_BLOCK_MASK_LO_64r_CLR(unknown_ucast_block_mask_lo_64);
    UNKNOWN_UCAST_BLOCK_MASK_HI_64r_CLR(unknown_ucast_block_mask_hi_64);

    /* Clear ingress block mask. */
    ING_EGRMSKBMAP_LOr_CLR(ing_egrmskbmap_lo);
    ING_EGRMSKBMAP_HIr_CLR(ing_egrmskbmap_hi);

    /* Configurations to guarantee no packet modifications */
    SOC_LPORT_ITER(lport) {
        SOC_IF_ERROR_RETURN(
            WRITE_SYSTEM_CONFIG_TABLEm(unit,lport, system_config_table));

        SOC_IF_ERROR_RETURN(
            WRITE_SOURCE_TRUNK_MAPm(unit, lport, source_trunk_map));

        SOC_IF_ERROR_RETURN(
            WRITE_PORTm(unit, lport, port_entry));

        SOC_IF_ERROR_RETURN(
            WRITE_UNKNOWN_UCAST_BLOCK_MASK_LO_64r(unit, lport, unknown_ucast_block_mask_lo_64));
        SOC_IF_ERROR_RETURN(
            WRITE_UNKNOWN_UCAST_BLOCK_MASK_HI_64r(unit, lport, unknown_ucast_block_mask_hi_64));

        SOC_IF_ERROR_RETURN(
            WRITE_ING_EGRMSKBMAP_LOr(unit, lport, ing_egrmskbmap_lo));
        SOC_IF_ERROR_RETURN(
            WRITE_ING_EGRMSKBMAP_HIr(unit, lport, ing_egrmskbmap_hi));
    }

    for (lport = 0 ; lport <= BCM5607X_LPORT_MAX ; lport++) {
        if (-1 == SOC_PORT_L2P_MAPPING(lport)) {
            continue;
        }

        /*
         * ING_PRI_CNG_MAP: Unity priority mapping and CNG = 0 or 1
         */
        for (j = 0; j < 16; j++) {
            ING_PRI_CNG_MAPm_SET(ing_pri_cng_map, dot1pmap[j]);
            SOC_IF_ERROR_RETURN(
                WRITE_ING_PRI_CNG_MAPm(unit, lport*16+j, ing_pri_cng_map));
        }
    }

    /* TRUNK32_CONFIG_TABLE: OUTER_TPID_ENABLE = 0x1 */
    TRUNK32_CONFIG_TABLEm_CLR(trunk32_config_table);
    TRUNK32_CONFIG_TABLEm_OUTER_TPID_ENABLEf_SET(trunk32_config_table, 1);

    /*
     * TRUNK32_PORT_TABLE:
     * DISABLE_VLAN_CHECKS = 1, PACKET_MODIFICATION_DISABLE = 1
     */
    TRUNK32_PORT_TABLEm_CLR(trunk32_port_table);
#ifdef CFG_SWITCH_VLAN_UNAWARE_INCLUDED
    TRUNK32_PORT_TABLEm_DISABLE_VLAN_CHECKSf_SET(trunk32_port_table, 1);
    TRUNK32_PORT_TABLEm_PACKET_MODIFICATION_DISABLEf_SET(trunk32_port_table, 1);
#endif /* CFG_SWITCH_VLAN_UNAWARE_INCLUDED */

    for (i = 0; i < 32; i++) {
        SOC_IF_ERROR_RETURN(
            WRITE_TRUNK32_CONFIG_TABLEm(unit, i, trunk32_config_table));
        SOC_IF_ERROR_RETURN(
            WRITE_TRUNK32_PORT_TABLEm(unit, i, trunk32_port_table));
    }

    config_schedule_mode(unit);
    fl_cosq_perq_xmt_counter_base_init(unit);

    /*
     * VLAN_DEFAULT_PBM is used as defalut VLAN members for those ports
     * that disable VLAN checks.
     */
    VLAN_DEFAULT_PBM_LOr_CLR(vlan_default_pbm_lo);
    VLAN_DEFAULT_PBM_LOr_PORT_BITMAPf_SET(vlan_default_pbm_lo, SOC_PBMP(BCM5607X_ALL_PORTS_MASK));
    SOC_IF_ERROR_RETURN(
        WRITE_VLAN_DEFAULT_PBM_LOr(0, vlan_default_pbm_lo));

    VLAN_DEFAULT_PBM_HIr_CLR(vlan_default_pbm_hi);
    VLAN_DEFAULT_PBM_HIr_PORT_BITMAPf_SET(vlan_default_pbm_hi, PBMP_WORD_GET(BCM5607X_ALL_PORTS_MASK, 2));
    SOC_IF_ERROR_RETURN(
        WRITE_VLAN_DEFAULT_PBM_HIr(0, vlan_default_pbm_hi));

    /* ING_VLAN_TAG_ACTION_PROFILE:
     * UT_OTAG_ACTION = 0x1
     * SIT_OTAG_ACTION = 0x0
     * SOT_POTAG_ACTION = 0x2
     * SOT_OTAG_ACTION = 0x0
     * DT_POTAG_ACTION = 0x2
     */
    ING_VLAN_TAG_ACTION_PROFILEm_CLR(ing_vlan_tag_action_profile);
    ING_VLAN_TAG_ACTION_PROFILEm_UT_OTAG_ACTIONf_SET(ing_vlan_tag_action_profile, 1);
    ING_VLAN_TAG_ACTION_PROFILEm_SIT_OTAG_ACTIONf_SET(ing_vlan_tag_action_profile, 0);
    ING_VLAN_TAG_ACTION_PROFILEm_SOT_POTAG_ACTIONf_SET(ing_vlan_tag_action_profile, 2);
    ING_VLAN_TAG_ACTION_PROFILEm_SOT_OTAG_ACTIONf_SET(ing_vlan_tag_action_profile, 0);
    ING_VLAN_TAG_ACTION_PROFILEm_DT_POTAG_ACTIONf_SET(ing_vlan_tag_action_profile, 2);
    SOC_IF_ERROR_RETURN(
        WRITE_ING_VLAN_TAG_ACTION_PROFILEm(unit, 0, ing_vlan_tag_action_profile));

#ifndef CFG_COE_INCLUDED
    /*
     * Program l2 user entry table to drop below MAC addresses:
     * 0x0180C2000001, 0x0180C2000002 and 0x0180C200000E
     */

    /* VALID = 1 */
    L2_USER_ENTRYm_CLR(l2_user_entry);
    L2_USER_ENTRYm_VALIDf_SET(l2_user_entry, 1);
    L2_USER_ENTRYm_DST_DISCARDf_SET(l2_user_entry, 1);
    L2_USER_ENTRYm_KEY_TYPEf_SET(l2_user_entry, 0);
    L2_USER_ENTRYm_BPDUf_SET(l2_user_entry, 1);
    L2_USER_ENTRYm_KEY_TYPE_MASKf_SET(l2_user_entry, 1);
    L2_USER_ENTRYm_MAC_ADDRf_SET(l2_user_entry, drop_mac_addr1);
    L2_USER_ENTRYm_MAC_ADDR_MASKf_SET(l2_user_entry, drop_mac_addr_mask);
    SOC_IF_ERROR_RETURN(
        WRITE_L2_USER_ENTRYm(unit, 0, l2_user_entry));

    L2_USER_ENTRYm_MAC_ADDRf_SET(l2_user_entry, drop_mac_addr2);
    SOC_IF_ERROR_RETURN(
        WRITE_L2_USER_ENTRYm(unit, 1, l2_user_entry));

    L2_USER_ENTRYm_MAC_ADDRf_SET(l2_user_entry, drop_mac_addr3);
    SOC_IF_ERROR_RETURN(
        WRITE_L2_USER_ENTRYm(unit, 2, l2_user_entry));
#endif

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
    SOC_IF_ERROR_RETURN(
        WRITE_DOS_CONTROLr(unit, dos_control));

    DOS_CONTROL2r_CLR(dos_control2);
    DOS_CONTROL2r_TCP_HDR_OFFSET_EQ1_ENABLEf_SET(dos_control2, 1);
    DOS_CONTROL2r_TCP_HDR_PARTIAL_ENABLEf_SET(dos_control2, 1);
    DOS_CONTROL2r_ICMP_V4_PING_SIZE_ENABLEf_SET(dos_control2, 1);
    DOS_CONTROL2r_ICMP_FRAG_PKTS_ENABLEf_SET(dos_control2, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_DOS_CONTROL2r(unit, dos_control2));

#endif /* CFG_SWITCH_DOS_INCLUDED */

    /* enable FP_REFRESH_ENABLE */
    SOC_IF_ERROR_RETURN(
        READ_AUX_ARB_CONTROL_2r(unit, aux_arb_control_2));
    AUX_ARB_CONTROL_2r_FP_REFRESH_ENABLEf_SET(aux_arb_control_2, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_AUX_ARB_CONTROL_2r(unit, aux_arb_control_2));

    /*
     * Enable IPV4_RESERVED_MC_ADDR_IGMP_ENABLE, APPLY_EGR_MASK_ON_L3
     * and APPLY_EGR_MASK_ON_L2
     * Disable L2DST_HIT_ENABLE
     */
    SOC_IF_ERROR_RETURN(
        READ_ING_CONFIG_64r(unit, ing_config_64));
    ING_CONFIG_64r_IPV4_RESERVED_MC_ADDR_IGMP_ENABLEf_SET(ing_config_64, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L3f_SET(ing_config_64, 1);
    ING_CONFIG_64r_APPLY_EGR_MASK_ON_L2f_SET(ing_config_64, 1);
    ING_CONFIG_64r_L2DST_HIT_ENABLEf_SET(ing_config_64, 0);
    SOC_IF_ERROR_RETURN(
        WRITE_ING_CONFIG_64r(unit, ing_config_64));

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
    SOC_IF_ERROR_RETURN(
        WRITE_VLAN_PROFILEm(unit,0,vlan_profile));

    /* Do VLAN Membership check EN_EFILTER for the outgoing port */
    SOC_LPORT_ITER(lport) {
        SOC_IF_ERROR_RETURN(
            READ_EGR_PORT_64r(unit, lport, egr_port_64));
        EGR_PORT_64r_EN_EFILTERf_SET(egr_port_64, 1);
        SOC_IF_ERROR_RETURN(
            WRITE_EGR_PORT_64r(unit, lport, egr_port_64));
    }

#if CFG_RXTX_SUPPORT_ENABLED
    /*
     * Use VLAN 0 for CPU to transmit packets
     * All ports are untagged members, with STG=1 and VLAN_PROFILE_PTR=0
     */
    PBMP_ASSIGN(lpbmp, BCM5607X_ALL_PORTS_MASK);
    PBMP_PORT_ADD(lpbmp, 0);
    VLANm_CLR(vlan);
    VLANm_VALIDf_SET(vlan, 1);
    VLANm_STGf_SET(vlan, 1);
    VLANm_VLAN_PROFILE_PTRf_SET(vlan, 0);
    VLANm_PORT_BITMAPf_SET(vlan, SOC_PBMP(lpbmp));
    SOC_IF_ERROR_RETURN(
        WRITE_VLANm(unit, 0, vlan));

    EGR_VLANm_CLR(egr_vlan);
    EGR_VLANm_PORT_BITMAPf_SET(egr_vlan,SOC_PBMP(lpbmp));
    PBMP_PORT_REMOVE(lpbmp, 0);
    EGR_VLANm_UT_BITMAPf_SET(egr_vlan, SOC_PBMP(lpbmp));
    EGR_VLANm_STGf_SET(egr_vlan, 1);
    EGR_VLANm_VALIDf_SET(egr_vlan, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_VLANm(unit, 0, egr_vlan));

#ifdef __BOOTLOADER__
    /* Default VLAN 1 with STG=1 and VLAN_PROFILE_PTR=0 for bootloader */
    PBMP_ASSIGN(lpbmp, BCM5607X_ALL_PORTS_MASK);
    VLANm_CLR(vlan);
    VLANm_STGf_SET(vlan, 1);
    VLANm_VALIDf_SET(vlan, 1);
    VLANm_VLAN_PROFILE_PTRf_SET(vlan, 0);
    VLANm_PORT_BITMAPf_SET(vlan, SOC_PBMP(lpbmp));
    SOC_IF_ERROR_RETURN(
        WRITE_VLANm(unit, 1, vlan));

    EGR_VLANm_CLR(egr_vlan);
    EGR_VLANm_UT_BITMAPf_SET(egr_vlan, SOC_PBMP(lpbmp));
    EGR_VLANm_PORT_BITMAPf_SET(egr_vlan,SOC_PBMP(lpbmp));
    EGR_VLANm_STGf_SET(egr_vlan, 1);
    EGR_VLANm_VALIDf_SET(egr_vlan, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_VLANm(unit, 1, egr_vlan));
#endif /* __BOOTLOADER__ */

    /* Set VLAN_STG and EGR_VLAN_STG */
    VLAN_STGm_CLR(vlan_stg);
    VLAN_STGm_SET(vlan_stg,0,0xfffffff0);
    VLAN_STGm_SET(vlan_stg,1,0xffffffff);
    VLAN_STGm_SET(vlan_stg,2,0xffffffff);
    VLAN_STGm_SET(vlan_stg,3,0xffffffff);
    VLAN_STGm_SET(vlan_stg,4,0x0000000f);
    SOC_IF_ERROR_RETURN(
        WRITE_VLAN_STGm(unit, 1, vlan_stg));

    EGR_VLAN_STGm_CLR(egr_vlan_stg);
    EGR_VLAN_STGm_SET(egr_vlan_stg,0,0xfffffff0);
    EGR_VLAN_STGm_SET(egr_vlan_stg,1,0xffffffff);
    EGR_VLAN_STGm_SET(egr_vlan_stg,2,0xffffffff);
    EGR_VLAN_STGm_SET(egr_vlan_stg,3,0xffffffff);
    EGR_VLAN_STGm_SET(egr_vlan_stg,4,0x0000000f);
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_VLAN_STGm(unit, 1, egr_vlan_stg));

    /* Make PORT_VID = 0 for CPU port */
    SOC_IF_ERROR_RETURN(
        READ_PORTm(unit, 0, port_entry));
    PORTm_PORT_VIDf_SET(port_entry, 0);
    SOC_IF_ERROR_RETURN(
        WRITE_PORTm(unit, 0, port_entry));

    /*
     * Trap DHCP and ARP packets to CPU.
     * Note ARP reply is copied to CPU ONLY when l2 dst is hit.
     */
    PROTOCOL_PKT_CONTROLr_CLR(protocol_pkt_control);
    PROTOCOL_PKT_CONTROLr_DHCP_PKT_TO_CPUf_SET(protocol_pkt_control, 1);
    PROTOCOL_PKT_CONTROLr_ARP_REPLY_TO_CPUf_SET(protocol_pkt_control, 1);
    PROTOCOL_PKT_CONTROLr_ARP_REQUEST_TO_CPUf_SET(protocol_pkt_control, 1);

    SOC_LPORT_ITER(lport) {
        SOC_IF_ERROR_RETURN(
            WRITE_PROTOCOL_PKT_CONTROLr(unit, lport, protocol_pkt_control));
    }
#endif /* CFG_RXTX_SUPPORT_ENABLED */

    /* Enable aging timer */
    SOC_IF_ERROR_RETURN(
        READ_L2_AGE_TIMERr(unit, l2_age_timer));
#ifndef CFG_COE_INCLUDED
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 1);
#else
    L2_AGE_TIMERr_AGE_ENAf_SET(l2_age_timer, 0);
#endif
    SOC_IF_ERROR_RETURN(
        WRITE_L2_AGE_TIMERr(unit, l2_age_timer));

    return SYS_OK;
}

#ifdef __LINUX__
#define FSM_IDLE        0
#define FSM_ACTIVE      1
#define FSM_WAIT_IDLE   2
#define FSM_DUMMY_RESP  3
static int
pcie_hotswap_recover(int unit)
{
    int ioerr = 0;
    int rv;
    PAXB_0_PAXB_HOTSWAP_CTRLr_t hswp_ctrl;
    PAXB_0_PAXB_HOTSWAP_STATr_t hswp_stat;
    PAXB_0_PAXB_INTR_ENr_t intr_en;
    PAXB_0_PAXB_INTR_STATUSr_t intr_stat;
    int to;
    int hswp_cause, fsm_state;
    uint32 intr_mask;

    /* Enable hotswap manager to handle CPU hotswap */
    ioerr += READ_PAXB_0_PAXB_HOTSWAP_CTRLr(unit, hswp_ctrl);
    PAXB_0_PAXB_HOTSWAP_CTRLr_ENABLEf_SET(hswp_ctrl, 1);
    ioerr += WRITE_PAXB_0_PAXB_HOTSWAP_CTRLr(unit, hswp_ctrl);

    /* Hot Swap status before Recovery */
    ioerr += READ_PAXB_0_PAXB_HOTSWAP_STATr(unit, hswp_stat);
    hswp_cause = PAXB_0_PAXB_HOTSWAP_STATr_HOTSWAP_CAUSEf_GET(hswp_stat);
    fsm_state = PAXB_0_PAXB_HOTSWAP_STATr_FSM_STATEf_GET(hswp_stat);

    /* Determine if hotswap event has happened */
    if ((fsm_state == 0) && (hswp_cause == 0)) {
        /* No hotswap events detected */
        return SYS_OK;
    }

    /* Save PAXB interrupt enable mask and disable all interrupts */
    ioerr += READ_PAXB_0_PAXB_INTR_ENr(unit, intr_en);
    intr_mask = PAXB_0_PAXB_INTR_ENr_GET(intr_en);
    PAXB_0_PAXB_INTR_ENr_CLR(intr_en);
    ioerr += WRITE_PAXB_0_PAXB_INTR_ENr(unit, intr_en);

    /* Clear any active PAXB interrupts */
    ioerr += READ_PAXB_0_PAXB_INTR_STATUSr(unit, intr_stat);
    ioerr += WRITE_PAXB_0_PAXB_INTR_STATUSr(unit, intr_stat);

    /* Wait for idle state */
    to = 100;
    rv = SYS_ERR_TIMEOUT;
    while (to) {
        ioerr += READ_PAXB_0_PAXB_HOTSWAP_STATr(unit, hswp_stat);
        fsm_state = PAXB_0_PAXB_HOTSWAP_STATr_FSM_STATEf_GET(hswp_stat);
        if (fsm_state == FSM_IDLE || fsm_state == FSM_DUMMY_RESP) {
            rv = SYS_OK;
            break;
        }
        sal_usleep(10);
        to -= 10;
    }
    if (rv == SYS_ERR_TIMEOUT) {
        sal_printf("Hot Swap Wait Time Out for IDLE/DUMMYRESP\n");
        return rv;
    }

    /* Signal that hotswap processing complete */
    ioerr += READ_PAXB_0_PAXB_HOTSWAP_CTRLr(unit, hswp_ctrl);
    PAXB_0_PAXB_HOTSWAP_CTRLr_SERVICE_COMPLETEf_SET(hswp_ctrl, 1);
    ioerr += WRITE_PAXB_0_PAXB_HOTSWAP_CTRLr(unit, hswp_ctrl);

    /* Wait for PAXB ready state */
    to = 100;
    rv = SYS_ERR_TIMEOUT;
    while (to) {
        ioerr += READ_PAXB_0_PAXB_HOTSWAP_STATr(unit, hswp_stat);
        if (PAXB_0_PAXB_HOTSWAP_STATr_PAXB_RDYf_GET(hswp_stat)) {
            rv = SYS_OK;
            break;
        }
        sal_usleep(10);
        to -= 10;
    }
    if (rv == SYS_ERR_TIMEOUT) {
        sal_printf("Hot Swap Wait Time Out for READY\n");
        return rv;
    }

    /* Clear hotswap status */
    ioerr += READ_PAXB_0_PAXB_HOTSWAP_STATr(unit, hswp_stat);
    ioerr += WRITE_PAXB_0_PAXB_HOTSWAP_STATr(unit, hswp_stat);

    /* Clear any active PAXB interrupts */
    ioerr += READ_PAXB_0_PAXB_INTR_STATUSr(unit, intr_stat);
    ioerr += WRITE_PAXB_0_PAXB_INTR_STATUSr(unit, intr_stat);

    /* Restore PAXB interrupt enable mask */
    PAXB_0_PAXB_INTR_ENr_SET(intr_en, intr_mask);
    ioerr += WRITE_PAXB_0_PAXB_INTR_ENr(unit, intr_en);

    if (ioerr) {
        return SYS_ERR;
    }
    return SYS_OK;
}
#endif /* __LINUX__ */

/* Function:
 *   bcm5607x_sw_init
 * Description:
 *   Perform chip specific initialization.
 *   This will be called by board_init()
 * Parameters:
 *   None
 * Returns:
 *   None
 */
sys_error_t
bcm5607x_sw_init(void)
{
    uint8 unit = 0x0;
    pbmp_t okay_pbmp;
    sys_error_t rv = SYS_OK;
    MHOST_0_MHOST_DEBUG_CTRLr_t mhost_0_mhost_debug_ctrl;
    DMU_CRU_RESETr_t dmu_cru_reset;

    /* To enable ICE */
    MHOST_0_MHOST_DEBUG_CTRLr_CLR(mhost_0_mhost_debug_ctrl);
    MHOST_0_MHOST_DEBUG_CTRLr_SET(mhost_0_mhost_debug_ctrl, 0xFFFFFFFF);
    WRITE_MHOST_0_MHOST_DEBUG_CTRLr(0, mhost_0_mhost_debug_ctrl);

    /* To maintian compatibility with all
     * previous devices, write '0' to
     * SW_RESET_N of DMU_CRU_RESET
     */
    DMU_CRU_RESETr_SET(dmu_cru_reset, 2);
    WRITE_DMU_CRU_RESETr(unit, dmu_cru_reset);

#if CONFIG_EMULATION
    sal_usleep(250000);
#else
    sal_usleep(1000);
#endif

#ifdef __LINUX__
    rv = pcie_hotswap_recover(unit);
    if (rv != SYS_OK) {
        sal_printf("pcie_hotswap_recover return %d\n", rv);
        return rv;
    }
#endif

    rv = soc_reset(unit);
    if (rv != SYS_OK) {
        sal_printf("soc_reset return %d\n", rv);
        return rv;
    }

#ifdef CFG_INTR_INCLUDED
    rv = bcm5607x_intr_init(unit);
    if (rv != SYS_OK) {
        sal_printf("bcm5607x_intr_init return %d\n", rv);
        return rv;
    }
#endif

    rv = soc_fl_tsc_reset(unit);
    if (rv != SYS_OK) {
        sal_printf("soc_fl_tsc_reset return %d\n", rv);
        return rv;
    }

    rv = soc_misc_init(unit);
    if (rv != SYS_OK) {
        sal_printf("soc_misc_init return %d\n", rv);
        return rv;
    }

    rv = soc_tdm_calculation(unit);
    if (rv != SYS_OK) {
        sal_printf("soc_tdm_calculation return %d\n", rv);
        return rv;
    }

    rv = soc_mmu_init(unit);
    if (rv != SYS_OK) {
        sal_printf("soc_mmu_init return %d\n", rv);
        return rv;
    }

    rv = pcm_software_init(unit);
    if (rv != SYS_OK) {
        sal_printf("pcm_software_init return %d\n", rv);
        return rv;
    }

    rv = pcm_port_probe_init(unit, BCM5607X_ALL_PORTS_MASK, &okay_pbmp);
    if (rv != SYS_OK) {
        sal_printf("pcm_port_probe_init return %d\n", rv);
        return rv;
    }

    rv = bcm5607x_system_init(unit);
    if (rv != SYS_OK) {
        sal_printf("bcm5607x_system_init return %d\n", rv);
        return rv;
    }

    rv = bcm5607x_linkscan_init(LINKSCAN_INTERVAL);
    if (rv != SYS_OK) {
        sal_printf("bcm5607x_linkscan_init return %d\n", rv);
        return rv;
    }

#ifdef CFG_SWITCH_TIMESYNC_INCLUDED
    rv = bcm5607x_timesync_init(unit);
    if (rv != SYS_OK) {
        sal_printf("bcm5607x_timesync_init return %d\n", rv);
        return rv;
    }
#endif

    return SYS_OK;
}

soc_switch_t soc_switch_bcm5607x =
{
    bcm5607x_chip_type,
    NULL,
    bcm5607x_port_count,
    NULL,
    NULL,
#if CFG_RXTX_SUPPORT_ENABLED
    bcm5607x_rx_set_handler,
    bcm5607x_rx_fill_buffer,
    bcm5607x_tx,
#endif /* CFG_RXTX_SUPPORT_ENABLED */

    bcm5607x_link_status,
    bcm5607x_chip_revision,
    bcm5607x_reg_get,
    bcm5607x_reg_set,
    bcm5607x_reg64_get,
    bcm5607x_reg64_set,
    bcm5607x_mem_get,
    bcm5607x_mem_set,
    bcm5607x_tcam_mem_get,
    bcm5607x_tcam_mem_set,
    bcm5607x_read32,
    bcm5607x_write32,
#ifdef CFG_SWITCH_VLAN_INCLUDED
    bcm5607x_pvlan_egress_set,
    bcm5607x_pvlan_egress_get,
    bcm5607x_qvlan_port_set,
    bcm5607x_qvlan_port_get,
    bcm5607x_vlan_create,
    bcm5607x_vlan_destroy,
    bcm5607x_vlan_type_set,
    bcm5607x_vlan_reset,
#endif  /* CFG_SWITCH_VLAN_INCLUDED */
    bcm5607x_phy_reg_get,
    bcm5607x_phy_reg_set,
};

