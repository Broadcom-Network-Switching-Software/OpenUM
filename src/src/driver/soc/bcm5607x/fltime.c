/*
 * $Id: fltime.c,v 1.2 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"

#ifdef CFG_SWITCH_SYNCE_INCLUDED
/* SyncE Stage0 Mode. */
typedef enum bcm_time_synce_stage0_mode_e {
    bcmTimeSynceModeBypass = 0,     /* Stage0 Mode Bypass */
    bcmTimeSynceModeGap45 = 1,      /* Stage0 Mode GAP 4/5 */
    bcmTimeSynceModeSDMFracDiv = 2  /* Stage0 Mode SDM Frac Div */
} bcm_time_synce_stage0_mode_t;

/* bcm_time_synce_stage1_div_e */
typedef enum bcm_time_synce_stage1_div_e {
    bcmTimeSynceStage1Div1 = 0, /* Stage1 Div1 */
    bcmTimeSynceStage1Div7 = 1, /* Stage1 Div7 */
    bcmTimeSynceStage1Div11 = 2 /* Stage1 Div11 */
} bcm_time_synce_stage1_div_t;

/* Time Synce Divisor setting structure. */
typedef struct bcm_time_synce_divisor_setting_s {
    bcm_time_synce_input_src_type_t input_src; /* Input source type */
    uint32 index;                       /* Logical port number or PLL index */
    bcm_time_synce_stage0_mode_t stage0_mode; /* Stage0 Mode bcmTimeSynceMode* */
    uint32 stage0_sdm_whole;            /* Stage0 SDM whole value */
    uint32 stage0_sdm_frac;             /* Stage0 SDM frac value */
    bcm_time_synce_stage1_div_t stage1_div; /* Stage1 Div bcmTimeSynceStage1Div* */
} bcm_time_synce_divisor_setting_t;

static void
bcm_time_synce_divisor_setting_t_init(bcm_time_synce_divisor_setting_t *divisor);

static int
_bcm5607x_time_phy_port_get(uint8 unit, int port_sel);

static int
bcm5607x_time_synce_clock_set_by_port(int unit,
    bcm_time_synce_clock_src_type_t clk_src,
    bcm_time_synce_divisor_setting_t *divisor);

static int
_bcm5607x_time_synce_clock_source_squelch_get(uint8 unit,
    bcm_time_synce_clock_src_type_t clk_src, int *squelch);

static int
_bcm5607x_time_synce_clock_source_squelch_set(int unit,
    bcm_time_synce_clock_src_type_t clk_src, int squelch);

static int
_bcm5607x_time_synce_clock_source_frequency_get(uint8 unit,
    bcm_time_synce_clock_source_config_t *clk_src_config, int *frequency);

static int
_bcm5607x_time_synce_clock_source_frequency_set(int unit,
    bcm_time_synce_clock_source_config_t *clk_src_config, int frequency);

#if 1

/*!
 * @enum phymod_dispatch_type_e
 * @brief Supported Drivers
 */
typedef enum phymod_dispatch_type_e {
    phymodDispatchTypeTsce16,
    phymodDispatchTypeNull,
    phymodDispatchTypeInvalid,
    phymodDispatchTypeTscf16_gen3,
    phymodDispatchTypeCount
} phymod_dispatch_type_t;
#endif


#if 1
static phymod_dispatch_type_t
dummy_bcm_time_synce_tsc_phymod_dispatch_type_get(int unit, int lport);
#endif
#endif /* CFG_SWITCH_SYNCE_INCLUDED */

#ifdef CFG_SWITCH_SYNCE_INCLUDED
/*
 * Function:
 *      bcm_time_synce_divisor_setting_t_init
 * Purpose:
 *      Initialize bcm_time_synce_divisor_setting_t struct
 * Parameters:
        divisor - (OUT) Divisor setting
 * Returns:
 * Notes:
 */
static void
bcm_time_synce_divisor_setting_t_init(
    bcm_time_synce_divisor_setting_t *divisor)
{
    sal_memset(divisor, 0, sizeof (bcm_time_synce_divisor_setting_t));
    return;
}

/*
 * Function:
 *      _bcm5607x_time_phy_port_get
 * Purpose:
 *      Get physical port from L1 recovery clock port select
 * Parameters:
 *      unit    - (IN)  Unit number.
 *      port_sel - (IN) port
 * Returns:
 *      port    - physical port
 * Notes:
 */
static int
_bcm5607x_time_phy_port_get(uint8 unit, int port_sel)
{
    if (port_sel >= 8 && port_sel <= 19) {
        int pport = (50 + (port_sel - 8));

        /* no qmode checking */
        return pport;
    }
    else if (port_sel >= 24 && port_sel <= 39) {
        /* Falcon : TSCF0, TSCF1, TSCF2, TSCF3 */
        return (port_sel - 24 + 62);
    }
    else {
        return SYS_ERR_FALSE;
    }
}

/*
 * Function:
 *      bcm5607x_time_synce_clock_get
 * Purpose:
 *      Get current setting of syncE clock divisor
 * Parameters:
 *      unit    - (IN)  Unit number.
 *      clk_src - (IN) clock source type (Primary, Secondary)
 *      divisor - (OUT) Divisor setting
 * Returns:
 *      SYS_OK/SYS_ERR_xxx
 * Notes:
 */
int
bcm5607x_time_synce_clock_get(uint8 unit,
                             bcm_time_synce_clock_src_type_t clk_src,
                             bcm_time_synce_divisor_setting_t *divisor)
{
    uint32 val;
    int lport;
    int phy_port = 0;
    TOP_MISC_CONTROL_2r_t top_misc_control_2;

    if (NULL == divisor) {
        return SYS_ERR_PARAMETER;
    }

    /* input source port */
    READ_TOP_MISC_CONTROL_2r(unit, top_misc_control_2);
    if (clk_src == bcmTimeSynceClockSourceSecondary) {
        val = TOP_MISC_CONTROL_2r_L1_RCVD_BKUP_CLK_PORT_SELf_GET(top_misc_control_2);
    } else {
        val = TOP_MISC_CONTROL_2r_L1_RCVD_PRI_CLK_PORT_SELf_GET(top_misc_control_2);
    }

    /* source is port, get the physical port */
    phy_port = _bcm5607x_time_phy_port_get(unit, val);

    /* Check if invalid physical port */
    if (-1 == phy_port) {
        return SYS_ERR_FALSE;
    }

    divisor->input_src = bcmTimeSynceInputSourceTypePort;
    divisor->index = SOC_PORT_P2L_MAPPING(phy_port);

    if (divisor->input_src == bcmTimeSynceInputSourceTypePort) {
        /* READ the SDM divisors from TSC */
        lport = divisor->index;
        if ((lport < 0) || (SOC_PORT_SPEED_MAX(lport) == 0)) {
            return SYS_ERR_PARAMETER;
        }

#if 0
        if (SOC_PORT_USE_PORTCTRL(unit, lport)) {
            portmod_port_synce_clk_ctrl_t config;
            portmod_port_synce_clk_ctrl_t_init(unit, &config);

            SOC_IF_ERROR_RETURN(
                portmod_port_synce_clk_ctrl_get(unit, lport, &config));

            /* stage0_mode */
            divisor->stage0_mode = config.stg0_mode;
            sdm_val = config.sdm_val;
            /* stage0_whole */
            divisor->stage0_sdm_whole  = ((sdm_val & 0xff00) >> 8);
            /* stage0_frac */
            divisor->stage0_sdm_frac = sdm_val & 0xff ;
            /* stage1_div  */
            divisor->stage1_div = config.stg1_mode;
        }
#endif /* PORTMOD_SUPPORT */
    }
    return SYS_OK;
}

/*
 * Port sel |         Clock source            | Phy port
 *   0      | LCPLL0 channel 0                |    x
 *   1      | N/A                             |    x
 *   2      | External phy recovered clock 0  |    x
 *   3      | External phy recovered clock 1  |    x
 *  7:4     | 4'h0                            |    x
 * 23:20    | 4'h0                            |    x
 *
 * PM4x10Q x3:
 *
 * 1) 48x2.5G:
 * 11:8     | pgw_ge0 lane [3, 2, 1, 0]       |   2-17
 * 15:12    | pgw_ge1 lane [3, 2, 1, 0]       |  18-33
 * 19:16    | pgw_ge2 lane [3, 2, 1, 0]       |  34-49
 *
 * 2) 12x10G:
 * 11:8     | pgw_xl0 lane [3, 2, 1, 0]       |  50-53
 * 15:12    | pgw_xl1 lane [3, 2, 1, 0]       |  54-57
 * 19:16    | pgw_xl2 lane [3, 2, 1, 0]       |  58-61
 *
 * PM4x25 x4:
 * 27:24    | pgw_cl0 lane [3, 2, 1, 0]       |  62-65
 * 31:28    | pgw_cl1 lane [3, 2, 1, 0]       |  66-69
 * 35:32    | pgw_cl2 lane [3, 2, 1, 0]       |  70-73
 * 39:36    | pgw_cl3 lane [3, 2, 1, 0]       |  74-77
 */

static int
bcm5607x_time_synce_L1_rcvd_clk_port_get(int phy_port)
{
    if (phy_port >= 2 && phy_port <= 49) {
        /* PM4x10Q with Q mode, TSCE0_0, TSCE0_1, ..., TSCE2_3. */
        return ((phy_port - 2)/4 + 8);
    }
    else if (phy_port >= 50 && phy_port <= 61) {
        /* PM4x10Q with eth mode, TSCE0, TSCE1, TSCE2 */
        return (phy_port - 50 + 8);
    }
    else if (phy_port >= 62 && phy_port <= 77) {
        /* Falcon : TSCF0, TSCF1, TSCF2, TSCF3 */
        return (phy_port - 62 + 24);
    }
    else {
        return SYS_ERR;
    }
}

#define BCM_TIME_SYNCE_CLK_SRC_VALID(x) \
    (((x) == bcmTimeSynceClockSourcePrimary) || \
     ((x) == bcmTimeSynceClockSourceSecondary) )

static int
bcm5607x_time_synce_clock_set_by_port(
    int unit,
    bcm_time_synce_clock_src_type_t clk_src,
    bcm_time_synce_divisor_setting_t *divisor)
{
    int lport;
    int phy_port;
    int port_sel = 0;

    TOP_MISC_CONTROL_2r_t top_misc_control_2;
    TOP_L1_RCVD_CLK_CONTROLr_t top_l1_rcvd_clk_control;

    if (!BCM_TIME_SYNCE_CLK_SRC_VALID(clk_src)|| NULL == divisor) {
        return SYS_ERR_PARAMETER;
    }

    /* Assumes input index is logical port number */
    lport = divisor->index;
    phy_port = SOC_PORT_L2P_MAPPING(lport);

    if ((lport < 0) || (SOC_PORT_SPEED_MAX(lport) == 0)) {
        return SYS_ERR_PARAMETER;
    }

    port_sel = bcm5607x_time_synce_L1_rcvd_clk_port_get(phy_port);

    if (-1 == port_sel) {
        return SYS_ERR_FALSE;
    }

#ifdef PORTMOD_SUPPORT
    if ((SOC_PORT_USE_PORTCTRL(unit, lport))) {
        portmod_port_synce_clk_ctrl_t config;
        portmod_port_synce_clk_ctrl_t_init(unit, &config);
        /* Set TSCF SDM divisors */
        /* Set SDM stage 1 mode to bypass */
        sdm_val = ((divisor->stage0_sdm_whole & 0xff) << 8 |
                   (divisor->stage0_sdm_frac & 0xff));

        config.stg0_mode = divisor->stage0_mode;
        config.stg1_mode = 0x0;
        config.sdm_val = sdm_val;
        SOC_IF_ERROR_RETURN(portmod_port_synce_clk_ctrl_set(unit, lport, &config));
    }
#endif /* PORTMOD_SUPPORT */

    /* serdes port configuration */
    if (clk_src == bcmTimeSynceClockSourceSecondary) {
        READ_TOP_MISC_CONTROL_2r(unit, top_misc_control_2);
        TOP_MISC_CONTROL_2r_L1_RCVD_BKUP_CLK_PORT_SELf_SET(
            top_misc_control_2, port_sel);
        TOP_MISC_CONTROL_2r_L1_RCVD_BKUP_LNKSTS_PORT_SELf_SET(
            top_misc_control_2, port_sel);
        WRITE_TOP_MISC_CONTROL_2r(unit, top_misc_control_2);

        READ_TOP_L1_RCVD_CLK_CONTROLr(unit, top_l1_rcvd_clk_control);
        TOP_L1_RCVD_CLK_CONTROLr_L1_RCVD_CLK_BKUP_RSTNf_SET(
            top_l1_rcvd_clk_control, 1);
        WRITE_TOP_L1_RCVD_CLK_CONTROLr(unit, top_l1_rcvd_clk_control);
    } else {
        READ_TOP_MISC_CONTROL_2r(unit, top_misc_control_2);
        TOP_MISC_CONTROL_2r_L1_RCVD_PRI_CLK_PORT_SELf_SET(
            top_misc_control_2, port_sel);
        TOP_MISC_CONTROL_2r_L1_RCVD_PRI_LNKSTS_PORT_SELf_SET(
            top_misc_control_2, port_sel);
        WRITE_TOP_MISC_CONTROL_2r(unit, top_misc_control_2);

        READ_TOP_L1_RCVD_CLK_CONTROLr(unit, top_l1_rcvd_clk_control);
        TOP_L1_RCVD_CLK_CONTROLr_L1_RCVD_CLK_RSTNf_SET(
            top_l1_rcvd_clk_control, 1);
        WRITE_TOP_L1_RCVD_CLK_CONTROLr(unit, top_l1_rcvd_clk_control);
    }

    return SYS_OK;
}

/*
 * Function:
 *      bcm5607x_time_synce_clock_set
 * Purpose:
 *      Set syncE divisor setting
 * Parameters:
 *      unit      - (IN) Unit number.
 *      clk_src   - (IN) clock source type (Primary, Secondary)
 *      divisor   - (IN) divisor setting.
 * Returns:
 *      SYS_OK/SYS_ERR_xxx
 * Notes:
 */
int
bcm5607x_time_synce_clock_set(int unit,
                             bcm_time_synce_clock_src_type_t clk_src,
                             bcm_time_synce_divisor_setting_t *divisor)
{
    int rv = SYS_ERR_FALSE;

    switch(divisor->input_src) {
    case bcmTimeSynceInputSourceTypePort:
        rv = bcm5607x_time_synce_clock_set_by_port(unit, clk_src, divisor);
        break;
    default:
        /* Firelight doesn't support PLL clock */
        rv = SYS_ERR_UNAVAIL;
        break;
    }

    return rv;
}

/*
 * Function:
 *      _bcm5607x_time_synce_clock_source_squelch_get
 * Purpose:
 *      Get syncE clock source control option
 * Parameters:
 *      unit    - (IN)  Unit number.
 *      clk_src - (IN) clock source
 *      squelch - (OUT) squelch value
 * Returns:
 *      SYS_OK/SYS_ERR_xxx
 * Notes:
 */
static int
_bcm5607x_time_synce_clock_source_squelch_get(uint8 unit,
                            bcm_time_synce_clock_src_type_t clk_src,
                            int *squelch)
{
    int rv = SYS_OK;
    TOP_L1_RCVD_CLK_CONTROLr_t top_l1_rcvd_clk_control;

    if (!squelch) return rv;

    switch(clk_src) {
    case bcmTimeSynceClockSourcePrimary:
        READ_TOP_L1_RCVD_CLK_CONTROLr(unit, top_l1_rcvd_clk_control);
        *squelch = TOP_L1_RCVD_CLK_CONTROLr_L1_RCVD_SW_OVWR_ENf_GET
                   (top_l1_rcvd_clk_control);
        break;

    case bcmTimeSynceClockSourceSecondary:
        READ_TOP_L1_RCVD_CLK_CONTROLr(unit, top_l1_rcvd_clk_control);
        *squelch = TOP_L1_RCVD_CLK_CONTROLr_L1_RCVD_SW_OVWR_ENf_GET
                   (top_l1_rcvd_clk_control);
        break;
    default:
        return SYS_ERR_PARAMETER;
    }

    return rv;
}

/*
 * Function:
 *      _bcm5607x_time_synce_clock_source_squelch_set
 * Purpose:
 *      Set syncE clock source squelch option
 * Parameters:
 *      unit      - (IN) Unit number.
 *      clk_src   - (IN) clock source type (Primary, Secondary)
 *      squelch   - (IN) synce clock source squelch setting 1 to enable , 0 to disable.
 * Returns:
 *      SYS_OK/SYS_ERR_XXX
 * Notes:
 */
STATIC int
_bcm5607x_time_synce_clock_source_squelch_set(int unit,
                             bcm_time_synce_clock_src_type_t clk_src,
                             int squelch)

{
    int rv = SYS_OK;
    TOP_L1_RCVD_CLK_CONTROLr_t top_l1_rcvd_clk_control;

    switch(clk_src) {
    case bcmTimeSynceClockSourcePrimary:
        READ_TOP_L1_RCVD_CLK_CONTROLr(unit, top_l1_rcvd_clk_control);
        TOP_L1_RCVD_CLK_CONTROLr_L1_RCVD_SW_OVWR_VALIDf_SET(
            top_l1_rcvd_clk_control, ((squelch) ? 0 : 1));
        TOP_L1_RCVD_CLK_CONTROLr_L1_RCVD_SW_OVWR_ENf_SET(
            top_l1_rcvd_clk_control, ((squelch) ? 1 : 0));
        WRITE_TOP_L1_RCVD_CLK_CONTROLr(unit, top_l1_rcvd_clk_control);
        break;

    case bcmTimeSynceClockSourceSecondary:
        READ_TOP_L1_RCVD_CLK_CONTROLr(unit, top_l1_rcvd_clk_control);
        TOP_L1_RCVD_CLK_CONTROLr_L1_RCVD_SW_OVWR_BKUP_VALIDf_SET(
            top_l1_rcvd_clk_control, ((squelch) ? 0 : 1));
        TOP_L1_RCVD_CLK_CONTROLr_L1_RCVD_SW_OVWR_ENf_SET(
            top_l1_rcvd_clk_control, ((squelch) ? 1 : 0));
        WRITE_TOP_L1_RCVD_CLK_CONTROLr(unit, top_l1_rcvd_clk_control);
        break;
    default:
        return SYS_ERR_PARAMETER;
    }
    return rv;
}

/*
 * Function:
 *      _bcm5607x_port_mode_get
 * Description:
 *      Get port mode in a TSC core by logical port number.
 * Parameters:
 *      unit          - Device number
 *      port          - Logical port
 *      int*          - Port Mode
 *
 * Each TSC3 can be configured into following 5 mode:
 *                     Lane number    return mode
 *                  0    1    2    3
 *   ------------  ---  ---  ---  --- -----------------------
 *      quad port  10G  10G  10G  10G SOC_FL_PORT_MODE_SINGLE
 *   tri_012 port  10G  10G  20G   x  SOC_FL_PORT_MODE_TRI_012
 *   tri_023 port  20G   x   10G  10G SOC_FL_PORT_MODE_TRI_023
 *      dual port  20G   x   20G   x  SOC_FL_PORT_MODE_DUAL
 *    single port  40G   x    x    x  SOC_FL_PORT_MODE_QUAD
 */
static int
_bcm5607x_port_mode_get(int unit, int logical_port, int *mode)
{
    int lane;
    int port, first_phyport, phy_port;
    int num_lanes[PORTS_PER_TSC];

    /* Each core is 4 lane core to get the first divide by 4 then multiply by 4
     * physical port is 2 => ((2-2)/4)*4+2 = 2
     * physical port is 7 => ((7-2)/4)*4+2 = 6
     */
    first_phyport = (((SOC_PORT_L2P_MAPPING(logical_port)-2)/4)*4)+2;

    for (lane = 0; lane < PORTS_PER_TSC; lane++) {
        phy_port = first_phyport + lane;
        port = SOC_PORT_P2L_MAPPING(phy_port);
        if (port == -1) {
            num_lanes[lane] = -1;
        } else {
            num_lanes[lane] = SOC_PORT_LANE_NUMBER(port);
        }
    }

    if (num_lanes[0] == 4) {
        *mode = SOC_FL_PORT_MODE_QUAD;
    } else if (num_lanes[0] == 2 && num_lanes[2] == 2) {
        *mode = SOC_FL_PORT_MODE_DUAL;
    } else if (num_lanes[0] == 2 && num_lanes[2] == 1 && num_lanes[3] == 1) {
        *mode = SOC_FL_PORT_MODE_TRI_023;
    } else if (num_lanes[0] == 1 && num_lanes[1] == 1 && num_lanes[2] == 2) {
        *mode = SOC_FL_PORT_MODE_TRI_012;
    } else {
        *mode = SOC_FL_PORT_MODE_SINGLE;
    }

    return SYS_OK;
}

typedef enum bcmi_time_port_mode_e {
    bcmi_time_port_mode_single = 0,
    bcmi_time_port_mode_dual = 1,
    bcmi_time_port_mode_tri = 2,
    bcmi_time_port_mode_quad = 3,
    bcmi_time_port_mode_invalid = 4
} bcmi_time_port_mode_t;

/* Get port mode by giving logical port. */
static void
bcm5607x_time_port_mode_get(
    int unit,
    phymod_dispatch_type_t dispatch_type,
    int lport,
    bcmi_time_port_mode_t *port_mode)
{
    if (port_mode == NULL) {
        return;
    }

    *port_mode = bcmi_time_port_mode_invalid;
    if (dispatch_type == phymodDispatchTypeTscf16_gen3) {
        int mode;

        _bcm5607x_port_mode_get(unit, lport, &mode);
        if (mode == SOC_FL_PORT_MODE_QUAD) {
            /* 4 lane for single mode. */
            *port_mode = bcmi_time_port_mode_single;
        } else if (mode == SOC_FL_PORT_MODE_DUAL) {
            *port_mode = bcmi_time_port_mode_dual;
        } else if (mode == SOC_FL_PORT_MODE_SINGLE) {
            /* single lane for quad mode. */
            *port_mode = bcmi_time_port_mode_quad;
        } else if (mode == SOC_FL_PORT_MODE_TRI_023 ||
                   mode == SOC_FL_PORT_MODE_TRI_012) {
            *port_mode = bcmi_time_port_mode_tri;
        }
    }
}

#if 1

static phymod_dispatch_type_t
dummy_bcm_time_synce_tsc_phymod_dispatch_type_get(int unit, int lport)
{
    int phy_port = SOC_PORT_L2P_MAPPING(lport);

    if (phy_port >= 2 && phy_port <= 49) {
        /* Q mode is not supported */
        return phymodDispatchTypeInvalid;
    }
    else if (phy_port >= 50 && phy_port <= 61) {
        /* PM4x10Q with eth mode, TSCE0, TSCE1, TSCE2 */
        return phymodDispatchTypeTsce16;
    }
    else if (phy_port >= 62 && phy_port <= 77) {
        /* Falcon : TSCF0, TSCF1, TSCF2, TSCF3 */
        return phymodDispatchTypeTscf16_gen3;
    }
    return phymodDispatchTypeInvalid;
}
#endif

/*
 * Function:
 *      _bcm5607x_time_synce_clock_source_frequency_get
 * Purpose:
 *      Get syncE clock source squelch option
 * Parameters:
 *      unit      - (IN) Unit number.
 *      clk_src   - (IN) clock source type (Primary, Secondary)
 *      frequency - (OUT) synce clock source frequency.
 * Returns:
 *      SYS_ERR_xxx
 * Notes:
 */
static int
_bcm5607x_time_synce_clock_source_frequency_get(uint8 unit,
    bcm_time_synce_clock_source_config_t *clk_src_config, int *frequency)
{
    int rv = SYS_OK;
    bcm_time_synce_divisor_setting_t div_out;
    /* Initialize bcm_time_synce_divisor_setting_t struct */
    bcm_time_synce_divisor_setting_t_init(&div_out);

    rv = bcm5607x_time_synce_clock_get(unit, clk_src_config->clk_src, &div_out);
    if (rv == SYS_OK) {
        if (div_out.input_src == bcmTimeSynceInputSourceTypePort) {
            clk_src_config->port = div_out.index;
        } else {
            clk_src_config->pll_index = div_out.index;
        }
    }

    if (div_out.input_src == bcmTimeSynceInputSourceTypePort) {
        if ((div_out.stage0_mode == bcmTimeSynceModeSDMFracDiv) &&
            (div_out.stage1_div == bcmTimeSynceStage1Div1)) {
            int port_speed = 0; /* speed specified in mbps*/
            uint32 sdm_divisor = 0;
            phymod_dispatch_type_t dispatch_type =
                phymodDispatchTypeInvalid;
            bcmi_time_port_mode_t port_mode =
                bcmi_time_port_mode_invalid;


#if 1
            port_speed = SOC_PORT_SPEED_STATUS(clk_src_config->port);
#else
            BCM_IF_ERROR_RETURN(bcm_esw_port_speed_get(unit,
                                clk_src_config->port, &port_speed));
#endif

             sdm_divisor = (div_out.stage0_sdm_whole << 8) |
                            div_out.stage0_sdm_frac;


#if 1
            dispatch_type = dummy_bcm_time_synce_tsc_phymod_dispatch_type_get(unit, clk_src_config->port);
#else
            dispatch_type = _bcm_time_synce_tsc_phymod_dispatch_type_get(unit, clk_src_config->port);
#endif

            bcm5607x_time_port_mode_get
                (unit, dispatch_type, clk_src_config->port, &port_mode);

            switch(dispatch_type) {
                case phymodDispatchTypeTsce16:
                    /* Eagle port with Ethernet mode. */
                    /* Regular port */
                    if (sdm_divisor == 0x14a0) {
                        *frequency = bcmTimeSyncE25MHz;
                    }
                    break;
                case phymodDispatchTypeTscf16_gen3:
                    if (port_mode == bcmi_time_port_mode_single) {
                        switch (port_speed) {
                            case 106000:
                                /* 106G HG - VCO 27.34375, 0x1B58 */
                                if (sdm_divisor == 0x1B58) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 100000:
                                /* 100G - VCO 25.78125, 0x19C8 */
                                if (sdm_divisor == 0x19C8) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 42000:
                                /* 42G HG - VCO 21.875, 0xAF0 */
                                if (sdm_divisor == 0xAF0) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 40000:
                                /* 40G - VCO 20.625, 0xA50 */
                                if (sdm_divisor == 0xA50) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            default:
                                return SYS_ERR_PARAMETER;
                            break;
                        }
                    } else if (port_mode == bcmi_time_port_mode_dual) {
                        switch (port_speed) {
                            case 53000:
                                /* 53G HG - VCO 27.34375, 0x1B58 */
                                if (sdm_divisor == 0x1B58) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 50000:
                                /* 50G - VCO 25.78125, 0x19C8 */
                                if (sdm_divisor == 0x19C8) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 42000:
                                /* 42G HG - VCO 21.875, 15E0 */
                                if (sdm_divisor == 0x15E0) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 40000:
                                /* 40G - VCO 20.625, 0x14A0 */
                                if (sdm_divisor == 0x14A0) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 21000:
                                /* 21G - VCO 21.875, 0xAF0 */
                                if (sdm_divisor == 0xAF0) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 20000:
                                /* 20G - VCO 20.625, 0xA50 */
                                if (sdm_divisor == 0xA50) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            default:
                                return SYS_ERR_PARAMETER;
                            break;
                        }
                    } else if (port_mode == bcmi_time_port_mode_quad) {
                        switch (port_speed) {
                            case 27000:
                                /* 27G HG - VCO 27.34375, 0x1B58 */
                                if (sdm_divisor == 0x1B58) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 25000:
                                /* 25G - VCO 25.78125, 0x19C8 */
                                if (sdm_divisor == 0x19C8) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 11000:
                                /* 11G HG - VCO 27.34375, 0xAF0 */
                                if (sdm_divisor == 0xAF0) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 10000:
                                /* 10G - VCO 25.78125, 0xA50 */
                                if (sdm_divisor == 0xA50) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 1000:
                                /* 1G - VCO 20.625, 0x14A0 */
                                /* 1G - VCO 25.78125, 0x19C8 */
                                if (sdm_divisor == 0x14A0 ||
                                    sdm_divisor == 0x19C8) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            default:
                                return SYS_ERR_PARAMETER;
                            break;
                        }
                    } else if (port_mode == bcmi_time_port_mode_tri) {
                        switch (port_speed) {
                            case 50000:
                            case 25000:
                                /* 25G/50G - VCO 27.34375, 0x19C8 */
                                if (sdm_divisor == 0x19C8) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 40000:
                                /* 40G - VCO 20.625, 0x14A0 */
                                if (sdm_divisor == 0x14A0) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            case 20000:
                            case 10000:
                                /* 10G/20G - VCO 20.625, 0xA50 */
                                if (sdm_divisor == 0xA50) {
                                    *frequency = bcmTimeSyncE25MHz;
                                } else {
                                    return SYS_ERR_PARAMETER;
                                }
                                break;
                            default:
                                return SYS_ERR_PARAMETER;
                            break;
                        }
                    }
                break;
                    default: break;
                } /* switch(dispatch_type) */
            }
    } else {
        return SYS_ERR_PARAMETER;
    }

    return rv;
}

/*
 * Function:
 *      _bcm5607x_time_synce_clock_source_frequency_set
 * Purpose:
 *      Set syncE clock source squelch option
 * Parameters:
 *      unit      - (IN) Unit number.
 *      clk_src   - (IN) clock source type (Primary, Secondary)
 *      frequency - (IN) synce clock source frequency.
 * Returns:
 *      SYS_ERR_xxx
 * Notes:
 */
static int
_bcm5607x_time_synce_clock_source_frequency_set(int unit,
    bcm_time_synce_clock_source_config_t *clk_src_config, int frequency)
{
    int rv = SYS_OK;
    bcm_time_synce_divisor_setting_t div_in;
    bcm_time_synce_divisor_setting_t_init(&div_in);
    uint8 vco_25g;

    if (clk_src_config->input_src == bcmTimeSynceInputSourceTypePort) {
        div_in.index = clk_src_config->port;
    } else if(clk_src_config->input_src == bcmTimeSynceInputSourceTypePLL) {
        div_in.index = clk_src_config->pll_index;
    } else {
        return SYS_ERR_PARAMETER;
    }

    div_in.input_src = clk_src_config->input_src;

    if (clk_src_config->input_src == bcmTimeSynceInputSourceTypePort) {
        int port_speed = 0; /* speed specified in mbps */
        phymod_dispatch_type_t dispatch_type = phymodDispatchTypeInvalid;
        bcmi_time_port_mode_t port_mode = bcmi_time_port_mode_invalid;

        /* no qmode checking
        int gmii_mode = 0;

        (void)bcmi_esw_time_port_is_gmii_mode(unit, clk_src_config->port,
                                              &gmii_mode);
        */

#if 1
        port_speed = SOC_PORT_SPEED_STATUS(clk_src_config->port);
#else
        BCM_IF_ERROR_RETURN(bcm_esw_port_speed_get(unit, clk_src_config->port, &port_speed));
#endif


#if 1
        dispatch_type = dummy_bcm_time_synce_tsc_phymod_dispatch_type_get(unit, clk_src_config->port);
#else
        dispatch_type = _bcm_time_synce_tsc_phymod_dispatch_type_get(unit, clk_src_config->port);
#endif
        bcm5607x_time_port_mode_get
            (unit, dispatch_type, clk_src_config->port, &port_mode);
        switch (frequency) {
            case bcmTimeSyncE25MHz: /* 25MHz using SDM mode */
                div_in.stage0_mode = bcmTimeSynceModeSDMFracDiv;
                div_in.stage1_div = bcmTimeSynceStage1Div1;
                switch(dispatch_type) {
                    case phymodDispatchTypeTsce16:
                        /* Eagle port with Ethernet mode. */
                        /* Regular port, 0x14A0 */
                        div_in.stage0_sdm_whole = 0x14;
                        div_in.stage0_sdm_frac  = 0xa0;
                        break;
                    case phymodDispatchTypeTscf16_gen3:
                        if (port_mode == bcmi_time_port_mode_single) {
                            switch (port_speed) {
                                case 106000:
                                    /* 106G HG - VCO 27.34375, 0x1B58 */
                                    div_in.stage0_sdm_whole = 0x1B;
                                    div_in.stage0_sdm_frac  = 0x58;
                                    break;
                                case 100000:
                                    /* 100G - VCO 25.78125, 0x19C8 */
                                    div_in.stage0_sdm_whole = 0x19;
                                    div_in.stage0_sdm_frac  = 0xC8;
                                    break;
                                case 42000:
                                    /* 42G HG - VCO 21.875, 0xAF0 */
                                    div_in.stage0_sdm_whole = 0xA;
                                    div_in.stage0_sdm_frac  = 0xF0;
                                    break;
                                case 40000:
                                    /* 40G - VCO 20.625, 0xA50 */
                                    div_in.stage0_sdm_whole = 0xA;
                                    div_in.stage0_sdm_frac  = 0x50;
                                    break;
                                default:
                                    return SYS_ERR_PARAMETER;
                                break;
                            }
                        } else if (port_mode == bcmi_time_port_mode_dual) {
                            switch (port_speed) {
                                case 53000:
                                    /* 53G HG - VCO 27.34375, 0x1B58 */
                                    div_in.stage0_sdm_whole = 0x1B;
                                    div_in.stage0_sdm_frac  = 0x58;
                                    break;
                                case 50000:
                                    /* 50G - VCO 25.78125, 0x19C8 */
                                    div_in.stage0_sdm_whole = 0x19;
                                    div_in.stage0_sdm_frac  = 0xC8;
                                    break;
                                case 42000:
                                    /* 42G HG - VCO 21.875, 15E0 */
                                    div_in.stage0_sdm_whole = 0x15;
                                    div_in.stage0_sdm_frac  = 0xE0;
                                    break;
                                case 40000:
                                    /* 40G - VCO 20.625, 0x14A0 */
                                    div_in.stage0_sdm_whole = 0x14;
                                    div_in.stage0_sdm_frac  = 0xA0;
                                    break;
                                case 21000:
                                    /* 21G - VCO 21.875, 0xAF0 */
                                    div_in.stage0_sdm_whole = 0xA;
                                    div_in.stage0_sdm_frac  = 0xF0;
                                    break;
                                case 20000:
                                    /* 20G - VCO 20.625, 0xA50 */
                                    div_in.stage0_sdm_whole = 0xA;
                                    div_in.stage0_sdm_frac  = 0x50;
                                    break;
                                default:
                                    return SYS_ERR_PARAMETER;
                                break;
                            }
                        } else if (port_mode == bcmi_time_port_mode_quad) {
                            rv = sal_config_uint8_get
                                 (SAL_CONFIG_SERDES_1000X_AT_25G_VCO, &vco_25g);

                            switch (port_speed) {
                                case 27000:
                                    /* 27G HG - VCO 27.34375, 0x1B58 */
                                    div_in.stage0_sdm_whole = 0x1B;
                                    div_in.stage0_sdm_frac  = 0x58;
                                    break;
                                case 25000:
                                    /* 25G - VCO 25.78125, 0x19C8 */
                                    div_in.stage0_sdm_whole = 0x19;
                                    div_in.stage0_sdm_frac  = 0xC8;
                                    break;
                                case 11000:
                                    /* 11G HG - VCO 27.34375, 0xAF0 */
                                    div_in.stage0_sdm_whole = 0xA;
                                    div_in.stage0_sdm_frac  = 0xF0;
                                    break;
                                case 10000:
                                    /* 10G - VCO 25.78125, 0xA50 */
                                    div_in.stage0_sdm_whole = 0xA;
                                    div_in.stage0_sdm_frac  = 0x50;
                                    break;
                                case 1000:
                                    if (vco_25g) {
                                        /* 1G - VCO 25.78125, 0x19C8 */
                                        div_in.stage0_sdm_whole = 0x19;
                                        div_in.stage0_sdm_frac  = 0xC8;
                                    } else {
                                        /* 1G - VCO 20.625, 0x14A0 */
                                        div_in.stage0_sdm_whole = 0x14;
                                        div_in.stage0_sdm_frac  = 0xA0;
                                    }
                                    break;
                                default:
                                    return SYS_ERR_PARAMETER;
                                break;
                            }
                        } else if (port_mode == bcmi_time_port_mode_tri) {
                            switch (port_speed) {
                                case 50000:
                                case 25000:
                                    /* 25G/50G - VCO 27.34375, 0x19C8 */
                                    div_in.stage0_sdm_whole = 0x19;
                                    div_in.stage0_sdm_frac  = 0xC8;
                                    break;
                                case 40000:
                                    /* 40G - VCO 20.625, 0x14A0 */
                                    div_in.stage0_sdm_whole = 0x14;
                                    div_in.stage0_sdm_frac  = 0xA0;
                                    break;
                                case 20000:
                                case 10000:
                                    /* 10G/20G - VCO 20.625, 0xA50 */
                                    div_in.stage0_sdm_whole = 0xA;
                                    div_in.stage0_sdm_frac  = 0x50;
                                    break;
                                default:
                                    return SYS_ERR_PARAMETER;
                                break;
                            }
                        }
                    break;
                    default:
                        sal_printf("unknown port dispatch type  ... %d\n",
                                (int)dispatch_type);
                        return SYS_ERR_FALSE;
                }
                break;
            default:
                /* not supported */
                return SYS_ERR_FALSE;
                break;
        } /* switch(frequency) */
    }

    rv = bcm5607x_time_synce_clock_set(unit, clk_src_config->clk_src, &div_in);
    return rv;
}

/*
 * Function:
 *      bcm5607x_time_synce_clock_source_control_get
 * Purpose:
 *      Get syncE clock source control option
 * Parameters:
 *      unit    - (IN)  Unit number.
 *      control - (IN) clock source config
 *      value - (OUT) control value
 * Returns:
 *      SYS_OK/SYS_ERR_xxx
 * Notes: New wrapper API to support synce operation
 */
sys_error_t
bcm5607x_time_synce_clock_source_control_get(uint8 unit,
                             bcm_time_synce_clock_source_config_t *clk_src_config,
                             bcm_time_synce_clock_source_control_t control,
                             int *value)
{
    sys_error_t rv = SYS_ERR_UNAVAIL;
    bcm_time_synce_clock_source_config_t local_clk_src_conf;
    local_clk_src_conf = *clk_src_config;

    switch (control) {
    case bcmTimeSynceClockSourceControlSquelch:
        rv = _bcm5607x_time_synce_clock_source_squelch_get(unit, clk_src_config->clk_src, value);
        break;
    case bcmTimeSynceClockSourceControlFrequency:
        rv = _bcm5607x_time_synce_clock_source_frequency_get(unit, &local_clk_src_conf, value);
        break;
    default:
        return SYS_ERR_PARAMETER;
    }

    return rv;
}

/*
 * Function:
 *      bcm5607x_time_synce_clock_source_control_set
 * Purpose:
 *      Set syncE clock source squelch option
 * Parameters:
 *      unit      - (IN) Unit number.
 *      control   - (IN) clock source type (Primary, Secondary)
 *      squelch   - (IN) synce clock source squelch setting 1 to enable , 0 to disable.
 * Returns:
 *      SYS_OK/SYS_ERR_xxx
 * Notes:
 */
sys_error_t
bcm5607x_time_synce_clock_source_control_set(uint8 unit,
                             bcm_time_synce_clock_source_config_t *clk_src_config,
                             bcm_time_synce_clock_source_control_t control,
                             int value)
{
    sys_error_t rv = SYS_ERR_UNAVAIL;

    switch (control) {
    case bcmTimeSynceClockSourceControlSquelch:
        rv = _bcm5607x_time_synce_clock_source_squelch_set(unit, clk_src_config->clk_src, value);
        break;
    case bcmTimeSynceClockSourceControlFrequency:
        rv = _bcm5607x_time_synce_clock_source_frequency_set(unit, clk_src_config, value);
        break;
    default:
        return SYS_ERR_PARAMETER;
   }

    return rv;
}

#endif /* CFG_SWITCH_SYNCE_INCLUDED */


