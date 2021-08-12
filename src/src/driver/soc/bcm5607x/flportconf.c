/*
 * $Id: flportconf.c,v 1.29 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include "system.h"

bcm5607x_sw_info_t fl_sw_info;


/* OPTION_NULL, NULL port config */
static const int p2l_mapping_op_null[] = {
    0, -1,
    /* TSC4Q 0 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSC4Q 1 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSC4Q 2 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSCE 0~2 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSF 0~3*/
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1
};
static const int port_speed_max_op_null[] = {
    0, -1,
    /* TSC4Q 0 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSC4Q 1 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSC4Q 2 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSCE 0~2 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSF 0~3*/
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
};

/* OPTION_5, 10P 10G + 2P 100G */
static
const int p2l_mapping_op5[] = {
    0, -1,
    /* TSC4Q 0 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSC4Q 1 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSC4Q 2 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSCE 0~2 */
    2, 3, 4, 5,
    6, 7, 8, 9,
    10, 11, -1, -1,
    /* TSF 0~3*/
    -1, -1, -1, -1,
    12, -1, -1, -1,
    13, -1, -1, -1,
    -1, -1, -1, -1
};

static
const int port_speed_max_op5[] = {
    0, -1,
    /* TSC4Q 0 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSC4Q 1 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSC4Q 2 */
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    -1, -1, -1, -1,
    /* TSCE 0~2 */
    100, 100, 100, 100,
    100, 100, 100, 100,
    100, 100, -1, -1,
    /* TSF 0~3*/
    -1, -1, -1, -1,
    1000, -1, -1, -1,
    1000, -1, -1, -1,
    -1, -1, -1, -1,
};

/* Following the Firelight PRD,  define the serdes core configuration by options */
const core_option_t core_options[] = {
    /* OPTION_1 */
    { OPTION_1, _FL_SYSTEM_FREQ_700, { BCM56070_DEVICE_ID, BCM56072_DEVICE_ID, 0, 0 },
      {FLEX_HG42_GEN2, FLEX_HG42_GEN2, FLEX_HG42_GEN2,
       FLEX_2HG53_GEN3, FLEX_CAUI_GEN3, FLEX_CAUI_GEN3, FLEX_2HG53_GEN3}
    },
    /* OPTION_2 */
    {OPTION_2, _FL_SYSTEM_FREQ_700, { BCM56070_DEVICE_ID, BCM56072_DEVICE_ID, 0, 0 },
         {FLEX_XLAUI_QXG, FLEX_XLAUI_QXG, FLEX_XLAUI_QXG,
          FLEX_OFF, FLEX_CAUI_GEN3, FLEX_CAUI_GEN3, FLEX_2HG53_GEN3}
    },
    /* OPTION_3 */
    {OPTION_3, _FL_SYSTEM_FREQ_700, { BCM56070_DEVICE_ID, BCM56072_DEVICE_ID, 0, 0 },
         {FLEX_OFF, FLEX_OFF, FLEX_OFF,
          FLEX_CAUI_GEN3, FLEX_CAUI_GEN3, FLEX_CAUI_GEN3, FLEX_CAUI_GEN3}
    },
    /* OPTION_4 */
    {OPTION_4, _FL_SYSTEM_FREQ_700, { BCM56070_DEVICE_ID, BCM56072_DEVICE_ID, 0, 0 },
        {FLEX_OFF, FLEX_OFF, FLEX_OFF,
         FLEX_CAUI_GEN3, FLEX_CAUI_GEN3, FLEX_CAUI_GEN3, FLEX_CAUI_GEN3}
    },
    /* OPTION_5 */
    {OPTION_5, _FL_SYSTEM_FREQ_700, { BCM56070_DEVICE_ID, BCM56071_DEVICE_ID, BCM56072_DEVICE_ID, 0 },
        {FLEX_XLAUI_QXG, FLEX_XLAUI_QXG, FLEX_XLAUI_QSG,
         FLEX_OFF, FLEX_CAUI_GEN3, FLEX_CAUI_GEN3, FLEX_OFF}
    },
    /* OPTION_6 */
    {OPTION_6, _FL_SYSTEM_FREQ_219, { BCM56070_DEVICE_ID, BCM56071_DEVICE_ID, BCM56072_DEVICE_ID, 0 },
        {FLEX_MGL_GEN2, FLEX_MGL_GEN2, FLEX_MGL_GEN2,
         FLEX_MGL_GEN3, FLEX_HG42_UL_GEN3, FLEX_HG42_UL_GEN3, FLEX_MGL_GEN3}
    },
    /* NULL */
    {OPTION_NULL, 0, { 0, 0, 0, 0 },
        {-1, -1, -1, -1, -1, -1, -1}
    },
};

int auto_tdm_p2l_mapping[BCM5607X_PORT_MAX + 1];
int auto_tdm_port_speed_max[BCM5607X_PORT_MAX + 1];
char auto_tdm_config_op[16] = "";

firelight_sku_info_t fl_auto_tdm_port_config = {
  .config_op = auto_tdm_config_op,
  .freq = _FL_SYSTEM_FREQ_700,
  .p2l_mapping = auto_tdm_p2l_mapping,
  .speed_max = auto_tdm_port_speed_max
};

static const firelight_sku_info_t fl_null_option_port_config =
{   OPTION_NULL, _FL_SYSTEM_FREQ_700,
    p2l_mapping_op_null, port_speed_max_op_null
};

/* 56070/56072 Default Port Config: OPTION_5, 10P 10G + 2P 100G */
static const firelight_sku_info_t fl_56070_default_port_config =
{   OPTION_5, _FL_SYSTEM_FREQ_700,
    p2l_mapping_op5, port_speed_max_op5
};

const firelight_sku_info_t *sku_port_config = NULL;

/* 7 TSC (TSC0~2, TSCF0~3) */
static const int tsc_phy_port[] = {50, 54, 58, 62, 66, 70, 74};

/* 3 QTC */
static const int qtc_phy_port[] = {2, 18, 34};

static void
soc_port_block_info_get(uint8 unit, uint8 pport, int *block_type, int *block_idx, int *port_idx, int *pmq_port_idx)
{
    *block_type = PORT_BLOCK_TYPE_XLPORT;
    *pmq_port_idx = -1;
    if ((pport >= PHY_CLPORT3_BASE) && (pport <= BCM5607X_PORT_MAX)) {
        *block_idx = CLPORT3_BLOCK_ID;
        *port_idx = (pport - PHY_CLPORT3_BASE) & 0x3;
        *block_type = PORT_BLOCK_TYPE_CLPORT;
    } else if (pport >= PHY_CLPORT2_BASE) {
        *block_idx = CLPORT2_BLOCK_ID;
        *port_idx = (pport - PHY_CLPORT2_BASE) & 0x3;
        *block_type = PORT_BLOCK_TYPE_CLPORT;
    } else if (pport >= PHY_CLPORT1_BASE) {
        *block_idx = CLPORT1_BLOCK_ID;
        *port_idx = (pport - PHY_CLPORT1_BASE) & 0x3;
        *block_type = PORT_BLOCK_TYPE_CLPORT;
    } else if (pport >= PHY_CLPORT0_BASE) {
        *block_idx = CLPORT0_BLOCK_ID;
        *port_idx = (pport - PHY_CLPORT0_BASE) & 0x3;
        *block_type = PORT_BLOCK_TYPE_CLPORT;
    } else if (pport >= PHY_XLPORT2_BASE) {
        *block_idx = XLPORT2_BLOCK_ID;
        *port_idx = (pport - PHY_XLPORT2_BASE) & 0x3;
    } else if (pport >= PHY_XLPORT1_BASE) {
        *block_idx = XLPORT1_BLOCK_ID;
        *port_idx = (pport - PHY_XLPORT1_BASE) & 0x3;
    } else if (pport >= PHY_XLPORT0_BASE) {
        *block_idx = XLPORT0_BLOCK_ID;
        *port_idx = (pport - PHY_XLPORT0_BASE) & 0x3;
    } else if (pport >= PHY_GPORT5_BASE) {
        *block_idx = GPORT5_BLOCK_ID;
        *port_idx = (pport - PHY_GPORT5_BASE) & 0x7;
        *block_type = PORT_BLOCK_TYPE_GXPORT;
        *pmq_port_idx = (pport - PHY_GPORT5_BASE) & 0xf;
    } else if (pport >= PHY_GPORT4_BASE) {
        *block_idx = GPORT4_BLOCK_ID;
        *port_idx = (pport - PHY_GPORT4_BASE) & 0x7;
        *block_type = PORT_BLOCK_TYPE_GXPORT;
        *pmq_port_idx = (pport - PHY_GPORT4_BASE) & 0xf;
    } else if (pport >= PHY_GPORT3_BASE) {
        *block_idx = GPORT3_BLOCK_ID;
        *port_idx = (pport - PHY_GPORT3_BASE) & 0x7;
        *block_type = PORT_BLOCK_TYPE_GXPORT;
        *pmq_port_idx = (pport - PHY_GPORT3_BASE) & 0xf;
    } else if (pport >= PHY_GPORT2_BASE) {
        *block_idx = GPORT2_BLOCK_ID;
        *port_idx = (pport - PHY_GPORT2_BASE) & 0x7;
        *block_type = PORT_BLOCK_TYPE_GXPORT;
        *pmq_port_idx = (pport - PHY_GPORT2_BASE) & 0x7;
    } else if (pport >= PHY_GPORT1_BASE) {
        *block_idx = GPORT1_BLOCK_ID;
        *port_idx = (pport - PHY_GPORT1_BASE) & 0x7;
        *block_type = PORT_BLOCK_TYPE_GXPORT;
        *pmq_port_idx = (pport - PHY_GPORT1_BASE) & 0x7;
    } else if (pport >= PHY_GPORT0_BASE) {
        *block_idx = GPORT0_BLOCK_ID;
        *port_idx = (pport - PHY_GPORT0_BASE) & 0x7;
        *block_type = PORT_BLOCK_TYPE_GXPORT;
        *pmq_port_idx = (pport - PHY_GPORT0_BASE) & 0x7;
    }
}

/*
 * Check if the configured port speed matched.
 *
 * return value
 *    1 : check passed
 *    0 : check failed
 */
static int
flex_lane_speed_check(int speed, int flex_mode, int lane_count)
{
    /* Check invalid lane count */
    if ((lane_count < 1) || (lane_count > 4)) {
        return 0;
    }
    if (lane_count == 3) {
        return 0;
    }

    if (flex_mode == FLEX_QXGMII) {
        if (lane_count == 1) {
            /* 1G, 2.5G */
            if ((speed == 10) || (speed == 25)) {
                return 1;
            }
        }
    } else if (flex_mode == FLEX_QSGMII) {
        if (lane_count == 1) {
            /* 1G */
            if (speed == 10) {
                return 1;
            }
        }
    } else if ((flex_mode == FLEX_HG42_GEN2) ||
               (flex_mode == FLEX_HG42_UL_GEN3)) {
        if (lane_count == 1) {
            /* 1G, 2.5G, 5G, 10G, 11G */
            if ((speed == 10) || (speed == 25) || (speed == 50) || \
                (speed == 100) || (speed == 110)) {
                return 1;
            }
        }
        if (lane_count == 2) {
            /* 20G, 21G */
            if ((speed == 200) || (speed == 210)) {
                return 1;
            }
        }
        if (lane_count == 4) {
            /* 40G, 42G */
            if ((speed == 400) || (speed == 420)) {
                return 1;
            }
        }
    } else if ((flex_mode == FLEX_CAUI_GEN3) ||
               (flex_mode == FLEX_2HG53_GEN3)) {
        if (lane_count == 1) {
            /* 1G, 2.5G, 5G, 10G, 11G, 25G, 27G */
            if ((speed == 10) || (speed == 25) || (speed == 50) || \
                (speed == 100) || (speed == 110) || \
                (speed == 250) || (speed == 270)) {
                return 1;
            }
        }
        if (lane_count == 2) {
            /* 20G, 21G, 50G, 53G */
            if ((speed == 200) || (speed == 210) ||
                (speed == 500) || (speed == 530)) {
                return 1;
            }
        }
        if (lane_count == 4) {
            /* 40G, 42G, 100G & GEN3 */
            if ((speed == 400) || (speed == 420) ||
                ((flex_mode == FLEX_CAUI_GEN3) && (speed == 1000))) {
                return 1;
            }
        }
    } else if ((flex_mode == FLEX_MGL_GEN2) ||
               (flex_mode == FLEX_MGL_GEN3)) {
        if (lane_count == 1) {
            /* 1G, 2.5G */
            if ((speed == 10) || (speed == 25)) {
                return 1;
            }
        }
        if ((lane_count == 2) || (lane_count == 4)) {
            return 0;
        }
    } else {
        return 0;
    }
    return 0;
}

static int
soc_fl_flex_hg42_checker(int unit,
                                int tsc_idx,
                                int gen3)
{
    const int *p2l_mapping, *speed_max;
    int pport_base;
    int flex_mode = FLEX_HG42_GEN2;
    int ratio = SOC_FL_PORT_MODE_COUNT;

    p2l_mapping = sku_port_config->p2l_mapping;
    speed_max = sku_port_config->speed_max;

    if (gen3) {
        flex_mode = FLEX_HG42_UL_GEN3;
    }

    pport_base = qtc_phy_port[tsc_idx];

    if (tsc_idx < _FL_MAX_QTC_COUNT) {
        if ((p2l_mapping[pport_base] != -1) ||
            (p2l_mapping[pport_base + 1] != -1) ||
            (p2l_mapping[pport_base + 2] != -1) ||
            (p2l_mapping[pport_base + 3] != -1) ||
            (p2l_mapping[pport_base + 4] != -1) ||
            (p2l_mapping[pport_base + 5] != -1) ||
            (p2l_mapping[pport_base + 6] != -1) ||
            (p2l_mapping[pport_base + 7] != -1) ||
            (p2l_mapping[pport_base + 8] != -1) ||
            (p2l_mapping[pport_base + 9] != -1) ||
            (p2l_mapping[pport_base + 10] != -1) ||
            (p2l_mapping[pport_base + 11] != -1) ||
            (p2l_mapping[pport_base + 12] != -1) ||
            (p2l_mapping[pport_base + 13] != -1) ||
            (p2l_mapping[pport_base + 14] != -1) ||
            (p2l_mapping[pport_base + 15] != -1)) {
            /* QTC port should be disabled on the core */
            return SYS_ERR;
        }
    }

    pport_base = tsc_phy_port[tsc_idx];

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] != -1) &&
        (p2l_mapping[pport_base+2] != -1) &&
        (p2l_mapping[pport_base+3] != -1)) {

        /* If 5G is required on a port macro,
           all ports on that port macro must be configured as 5GE */
        if (gen3 && ((speed_max[pport_base] == 50) ||
                     (speed_max[pport_base + 1] == 50) ||
                     (speed_max[pport_base + 2] == 50) ||
                     (speed_max[pport_base + 3] == 50))) {

            if ((speed_max[pport_base] != 50) ||
                (speed_max[pport_base + 1] != 50) ||
                (speed_max[pport_base + 2] != 50) ||
                (speed_max[pport_base + 3] != 50)) {
                return SYS_ERR;
            }
        }

        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 3], flex_mode, 1)) {
            ratio = SOC_FL_PORT_MODE_QUAD;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] == -1) &&
        (p2l_mapping[pport_base + 2] != -1) &&
        (p2l_mapping[pport_base + 3] != -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 2) &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 3], flex_mode, 1)) {
            ratio = SOC_FL_PORT_MODE_TRI_023;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] != -1) &&
        (p2l_mapping[pport_base + 2] != -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], flex_mode, 2)) {
            ratio = SOC_FL_PORT_MODE_TRI_012;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] == -1) &&
        (p2l_mapping[pport_base + 2] != -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 2) &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], flex_mode, 2)) {
            ratio = SOC_FL_PORT_MODE_DUAL;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] == -1) &&
        (p2l_mapping[pport_base + 2] == -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 4)) {
            ratio = SOC_FL_PORT_MODE_SINGLE;
        } else if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 2)) {
            ratio = SOC_FL_PORT_MODE_DUAL;
        } else if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 1)) {
            ratio = SOC_FL_PORT_MODE_QUAD;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] != -1) &&
        (p2l_mapping[pport_base + 2] == -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], flex_mode, 1)) {
            ratio = SOC_FL_PORT_MODE_QUAD;
        }
    }

    if ((p2l_mapping[pport_base] == -1) &&
        (p2l_mapping[pport_base + 1] == -1) &&
        (p2l_mapping[pport_base + 2] == -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        /* No lanes enabled on the core, assign default value */
        ratio = SOC_FL_PORT_MODE_QUAD;
    }

    if (ratio == SOC_FL_PORT_MODE_QUAD) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 1]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 3]) = 1;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 1]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 3]) = SOC_FL_PORT_MODE_QUAD;

        return SYS_OK;
    } else if (ratio == SOC_FL_PORT_MODE_TRI_023) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 2;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 3]) = 1;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_TRI_023;
        SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_TRI_023;
        SOC_PORT_MODE(p2l_mapping[pport_base + 3]) = SOC_FL_PORT_MODE_TRI_023;

        return SYS_OK;
    } else if (ratio == SOC_FL_PORT_MODE_TRI_012) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 1]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 2;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_TRI_012;
        SOC_PORT_MODE(p2l_mapping[pport_base + 1]) = SOC_FL_PORT_MODE_TRI_012;
        SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_TRI_012;

        return SYS_OK;
    } else if (ratio == SOC_FL_PORT_MODE_SINGLE) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 4;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_SINGLE;

        return SYS_OK;
    } else if (ratio == SOC_FL_PORT_MODE_DUAL) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 2;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 2;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_DUAL;
        SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_DUAL;

        return SYS_OK;
    } else {

        return SYS_ERR;
    }
}

static int
soc_fl_flex_d50_caui_checker(int unit,
                       int tsc_idx,
                       int caui)
{
    const int *p2l_mapping, *speed_max;
    int pport_base;
    int flex_mode = FLEX_2HG53_GEN3;
    int ratio = SOC_FL_PORT_MODE_COUNT;

    p2l_mapping = sku_port_config->p2l_mapping;
    speed_max = sku_port_config->speed_max;

    if (caui) {
        flex_mode = FLEX_CAUI_GEN3;
    }

    pport_base = tsc_phy_port[tsc_idx];

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] != -1) &&
        (p2l_mapping[pport_base + 2] != -1) &&
        (p2l_mapping[pport_base + 3] != -1)) {

        /* If 5G is required on a port macro,
           all ports on that port macro must be configured as 5GE */
        if ((speed_max[pport_base] == 50) ||
            (speed_max[pport_base + 1] == 50) ||
            (speed_max[pport_base + 2] == 50) ||
            (speed_max[pport_base + 3] == 50)) {

            if ((speed_max[pport_base] != 50) ||
                (speed_max[pport_base + 1] != 50) ||
                (speed_max[pport_base + 2] != 50) ||
                (speed_max[pport_base + 3] != 50)) {
                return SYS_ERR;
            }
        }

        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 3], flex_mode, 1)) {
            ratio = SOC_FL_PORT_MODE_QUAD;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] == -1) &&
        (p2l_mapping[pport_base + 2] != -1) &&
        (p2l_mapping[pport_base + 3] != -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 2) &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 3], flex_mode, 1)) {
            ratio = SOC_FL_PORT_MODE_TRI_023;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] != -1) &&
        (p2l_mapping[pport_base + 2] != -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], flex_mode, 2)) {
            ratio = SOC_FL_PORT_MODE_TRI_012;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] == -1) &&
        (p2l_mapping[pport_base + 2] != -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 2) &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], flex_mode, 2)) {
            ratio = SOC_FL_PORT_MODE_DUAL;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] == -1) &&
        (p2l_mapping[pport_base + 2] == -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 4)) {
            ratio = SOC_FL_PORT_MODE_SINGLE;
        } else if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 2)) {
            ratio = SOC_FL_PORT_MODE_DUAL;
        } else if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 1)) {
            ratio = SOC_FL_PORT_MODE_QUAD;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] != -1) &&
        (p2l_mapping[pport_base + 2] == -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], flex_mode, 1)) {
            ratio = SOC_FL_PORT_MODE_QUAD;
        }
    }

    if ((p2l_mapping[pport_base] == -1) &&
        (p2l_mapping[pport_base + 1] == -1) &&
        (p2l_mapping[pport_base + 2] == -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        /* No lanes enabled on the core, assign default value */
        ratio = SOC_FL_PORT_MODE_QUAD;
    }

    if (ratio == SOC_FL_PORT_MODE_QUAD) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 1]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 3]) = 1;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 1]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 3]) = SOC_FL_PORT_MODE_QUAD;

        return SYS_OK;
    } else if (ratio == SOC_FL_PORT_MODE_TRI_023) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 2;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 3]) = 1;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_TRI_023;
        SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_TRI_023;
        SOC_PORT_MODE(p2l_mapping[pport_base + 3]) = SOC_FL_PORT_MODE_TRI_023;

        return SYS_OK;
    } else if (ratio == SOC_FL_PORT_MODE_TRI_012) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 1]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 2;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_TRI_012;
        SOC_PORT_MODE(p2l_mapping[pport_base + 1]) = SOC_FL_PORT_MODE_TRI_012;
        SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_TRI_012;

        return SYS_OK;
    } else if (ratio == SOC_FL_PORT_MODE_SINGLE) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 4;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_SINGLE;

        return SYS_OK;
    } else if (ratio == SOC_FL_PORT_MODE_DUAL) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 2;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 2;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_DUAL;
        SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_DUAL;

        return SYS_OK;
    } else {

        return SYS_ERR;
    }
}

static int
soc_fl_flex_mgl_checker(int unit,
                          int tsc_idx,
                          int gen3)
{
    const int *p2l_mapping, *speed_max;
    int pport_base;
    int flex_mode = FLEX_MGL_GEN2;
    int ratio = SOC_FL_PORT_MODE_COUNT;

    p2l_mapping = sku_port_config->p2l_mapping;
    speed_max = sku_port_config->speed_max;

    pport_base = qtc_phy_port[tsc_idx];
    if (tsc_idx < _FL_MAX_QTC_COUNT) {
        if ((p2l_mapping[pport_base] != -1) ||
            (p2l_mapping[pport_base + 1] != -1) ||
            (p2l_mapping[pport_base + 2] != -1) ||
            (p2l_mapping[pport_base + 3] != -1) ||
            (p2l_mapping[pport_base + 4] != -1) ||
            (p2l_mapping[pport_base + 5] != -1) ||
            (p2l_mapping[pport_base + 6] != -1) ||
            (p2l_mapping[pport_base + 7] != -1) ||
            (p2l_mapping[pport_base + 8] != -1) ||
            (p2l_mapping[pport_base + 9] != -1) ||
            (p2l_mapping[pport_base + 10] != -1) ||
            (p2l_mapping[pport_base + 11] != -1) ||
            (p2l_mapping[pport_base + 12] != -1) ||
            (p2l_mapping[pport_base + 13] != -1) ||
            (p2l_mapping[pport_base + 14] != -1) ||
            (p2l_mapping[pport_base + 15] != -1)) {
            /* QTC port should be disabled on the core */
            return SYS_ERR;
        }
    }

    if (gen3) {
        flex_mode = FLEX_MGL_GEN3;
    }

    pport_base = tsc_phy_port[tsc_idx];

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base + 1] != -1) &&
        (p2l_mapping[pport_base + 2] != -1) &&
        (p2l_mapping[pport_base + 3] != -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], flex_mode, 1) &&
            flex_lane_speed_check
                (speed_max[pport_base + 3], flex_mode, 1)) {
            ratio = SOC_FL_PORT_MODE_QUAD;
        }
    }

    if ((p2l_mapping[pport_base] == -1) &&
        (p2l_mapping[pport_base + 1] == -1) &&
        (p2l_mapping[pport_base + 2] == -1) &&
        (p2l_mapping[pport_base + 3] == -1)) {
        /* No lanes enabled on the core, assign default value */
        ratio = SOC_FL_PORT_MODE_QUAD;
    }

    if (ratio == SOC_FL_PORT_MODE_QUAD) {
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 1]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 3]) = 1;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 1]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 3]) = SOC_FL_PORT_MODE_QUAD;

        return SYS_OK;
    } else {

        return SYS_ERR;
    }
}

static int
soc_fl_flex_off_checker(int unit,
                                int tsc_idx)
{
    const int *p2l_mapping;

    int pport_base = tsc_phy_port[tsc_idx];

    p2l_mapping = sku_port_config->p2l_mapping;

    if ((p2l_mapping[pport_base] == -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] == -1) &&
        (p2l_mapping[pport_base+3] == -1)) {
        /* No lanes enabled on the core, assign default value */

        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 1]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 1;
        SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 3]) = 1;
        SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 1]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_MODE(p2l_mapping[pport_base + 3]) = SOC_FL_PORT_MODE_QUAD;

        return SYS_OK;
    }

    return SYS_ERR;
}

static sys_error_t
soc_device_id_matcher(uint16 devid, const core_option_t *core_option) {

   int i = 0;

   /* Assume BCM56070_DEVICE_ID is the last one of devid support list */
   do {
        if (core_option->devid[i] == devid) {
            return SYS_OK;
        }
        i++;
   } while(core_option->devid[i] != 0 && i < 4);

   return SYS_ERR;
}
const core_option_t *
soc_portconf_matcher(int unit) {

    int i;

    char config_op[16] = "";
    char config_op_dest[16] = "";
    for (i = 0; i < _FL_ARRAY_SIZE(core_options); i++) {
         sal_strcpy(config_op, core_options[i].config_op);
         sal_strcat(config_op, "_");
         sal_strcpy(config_op_dest, sku_port_config->config_op);
         sal_strcat(config_op_dest, "_");
         if ((sal_strncmp(config_op_dest, config_op,
               sal_strlen(config_op)) == 0) &&
              (soc_device_id_matcher(fl_sw_info.devid, &core_options[i]) == SYS_OK))
         {
             return &core_options[i];
         }

    }
    return NULL;
}

/*
 * Check portmap configured data matches the flex mode.
 */
static int
soc_portmap_checker(int unit)
{
    /*
     * Validate each core
     * Check if p2l and speed matched flex port mode of the TSC core.
     *    If matched, return the PORT_RATIO_TYPE
     *    If not matched, put warning message and return error.
     */
    int rv = SYS_OK;
    int i;
    int flex_mode;
    const core_option_t *p_core_option = NULL;

    p_core_option = soc_portconf_matcher(unit);

    if (p_core_option == NULL) {
        return SYS_ERR;
    }

    fl_auto_tdm_port_config.freq = p_core_option->freq;

    for (i = 0; i < (FL_SERDES_CORE_COUNT); i++) {
        flex_mode = p_core_option->mode[i];
        if ((fl_sw_info.devid == BCM56071_DEVICE_ID) &&
            ((i == FL_TSC_TSCF0_IDX) || (i == (FL_TSC_TSCF0_IDX + 3)))) {
            /* Falcon #0 and #3 are disabled on 56071 */
            flex_mode = FLEX_OFF;
        }
        switch (flex_mode) {
            case FLEX_XLAUI_QXG:
            case FLEX_XLAUI_QSG:
            case FLEX_HG42_GEN2:
                rv = soc_fl_flex_hg42_checker(unit, i, 0);
                break;
            case FLEX_HG42_UL_GEN3:
                rv = soc_fl_flex_hg42_checker(unit, i, 1);
                break;
            case FLEX_2HG53_GEN3:
                rv = soc_fl_flex_d50_caui_checker(unit, i, 0);
                break;
            case FLEX_CAUI_GEN3:
                rv = soc_fl_flex_d50_caui_checker(unit, i, 1);
                break;
            case FLEX_MGL_GEN2:
                rv = soc_fl_flex_mgl_checker(unit, i, 0);
                break;
            case FLEX_MGL_GEN3:
                rv = soc_fl_flex_mgl_checker(unit, i, 1);
                break;
            case FLEX_OFF:
                rv = soc_fl_flex_off_checker(unit, i);
                break;
            default:
                if (i < FL_TSC_TSCF0_IDX) {
                    sal_printf("PORTMAP config is invalid on Merlin SerDes #%d.\n", i);
                } else {
                    sal_printf("PORTMAP config is invalid on Falcon SerDes #%d.\n", i-FL_TSC_TSCF0_IDX);
                }
                /* return SYS_ERR; */
        }
        if (rv != SYS_OK) {
            if (i < FL_TSC_TSCF0_IDX) {
                sal_printf("Incorrect PORTMAP config on Merlin SerDes #%d.\n", i);
            } else {
                sal_printf("Incorrect PORTMAP config on Falcon SerDes #%d.\n", i-FL_TSC_TSCF0_IDX);
            }
            return rv;
        }
    }
    return rv;
}

static int
soc_portmap_parser(int unit)
{
    int lport;
    int pport;
    uint32 port_bw;

    int rv = SYS_ERR_NOT_FOUND;
    const char *config_str;
    char *sub_str;
    char *sub_str_end;
    int *p2l_mapping = (int *)  sku_port_config->p2l_mapping;
    int *speed_max = (int *)  sku_port_config->speed_max;

    for (pport = 1; pport <= BCM5607X_PORT_MAX; pport++) {
         p2l_mapping[pport] = -1;
         speed_max[pport] = -1;
    }

    p2l_mapping[0] = 0;
    speed_max[0] = 0;
    /* Scan spn_PORTMAP and update to p2l_mapping and speed max tables */
    for (lport = 2; lport <= BCM5607X_LPORT_MAX; lport++) {
        config_str = sal_config_xy_get(SAL_CONFIG_PORTMAP, lport, -1);
        if (config_str == NULL) {
            continue;
        }

        /*
              * portmap_<port>=<physical port number>:<bandwidth in Gb>
              */
        sub_str = (char *) config_str;

        /* Parsing physical port number */
        pport = sal_strtol(sub_str, &sub_str_end, 0);
        if (sub_str == sub_str_end) {
            sal_printf("Port %d: Missing physical port information \"%s\"\n",
                                 lport, config_str);
            rv = SYS_ERR;
            continue;
        }
        /* Skip ':' */
        sub_str = sub_str_end;
        if (*sub_str != '\0') {
            if (*sub_str != ':') {
                sal_printf("Port %d: Bad config string \"%s\"\n",
                           lport, config_str);
                rv = SYS_ERR;
                continue;
            }
            sub_str++;
        }

        /* Parsing bandwidth */
        port_bw = sal_strtol(sub_str, &sub_str_end, 0);
        if (sub_str == sub_str_end) {
            sal_printf("Port %d: Missing bandwidth information \"%s\"\n",
                        lport, config_str);
            rv = SYS_ERR;
            continue;
        }

        if (p2l_mapping[pport] != -1) {
            sal_printf("portmap fail: physical port %d has been assigned\n", pport);
            return SYS_ERR;
        }
        p2l_mapping[pport] = lport;
        speed_max[pport] = port_bw * 10;

        /* Special case for 2.5G config */
        if (!sal_strcmp(sub_str, "2.5")) {
            speed_max[pport] = 25;
        }
        rv = SYS_OK;
    }

    return rv;
}

void
soc_portmap_init(uint8 unit) {

    int pport, lport, mmu_port;
    int tmp_speed_max[BCM5607X_PORT_MAX + 1];

    /* Initialize the runtime data base of the port config */
    for (lport = 0; lport <= BCM5607X_LPORT_MAX ; lport++) {
        SOC_PORT_L2P_MAPPING(lport) = -1;
        SOC_PORT_M2P_MAPPING(lport) = -1;
    }

    for (pport = 0; pport <= BCM5607X_PORT_MAX ; pport++) {
         SOC_PORT_P2M_MAPPING(pport) = -1;
         SOC_PORT_P2L_MAPPING(pport) = sku_port_config->p2l_mapping[pport];
         tmp_speed_max[pport] = sku_port_config->speed_max[pport];
         if (tmp_speed_max[pport] != -1) {
             if (tmp_speed_max[pport] >= 1000) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 100000;
             } else if (tmp_speed_max[pport] >= 500) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 50000;
             } else if (tmp_speed_max[pport] >= 400) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 40000;
             } else if (tmp_speed_max[pport] >= 250) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 25000;
             } else if (tmp_speed_max[pport] >= 200) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 20000;
             } else if (tmp_speed_max[pport] >= 100) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 10000;
             } else if (tmp_speed_max[pport] >= 25) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 2500;
             } else if (tmp_speed_max[pport] >= 10) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 1000;
             } else {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 100;
             }
         }
    }

    /* Get SOC_PORT_COUNT from speed_max table */
    SOC_PORT_COUNT(unit) = 0;
    for (pport = 0; pport <= BCM5607X_PORT_MAX ; pport++) {
         if((tmp_speed_max[pport] > 0) && (SOC_PORT_P2L_MAPPING(pport) > 0))
         {
             SOC_PORT_COUNT(unit)++;
         }
    }


    for (pport = 0; pport <= BCM5607X_PORT_MAX ; pport++) {
        if (SOC_PORT_P2L_MAPPING(pport) != -1) {
            if (tmp_speed_max[pport] != -1) {
                SOC_PORT_L2P_MAPPING(SOC_PORT_P2L_MAPPING(pport)) = pport;
            } else if (pport == 0) {
                SOC_PORT_L2P_MAPPING(SOC_PORT_P2L_MAPPING(pport)) = 0;
            } else {
                SOC_PORT_L2P_MAPPING(SOC_PORT_P2L_MAPPING(pport)) = -1;
            }
        }
    }

    /* physical to mmu port mapping */
    SOC_PORT_P2M_MAPPING(0) = 0; /* set cpu port 0 */
    SOC_PORT_P2M_MAPPING(1) = 1; /* loopback port 1 */
    SOC_PORT_M2P_MAPPING(0) = 0;
    SOC_PORT_M2P_MAPPING(1) = 1;

    mmu_port = SOC_MAX_MMU_PORTS;
    /* Assign MMU port for all ports with speed >= 100G */
    for (pport = BCM5607X_PORT_MAX; pport > 1; pport--) {
        lport = SOC_PORT_P2L_MAPPING(pport);
        if ((lport != -1) && (SOC_PORT_SPEED_MAX(lport) >= 100000)) {
            SOC_PORT_P2M_MAPPING(pport) = mmu_port;
            SOC_PORT_M2P_MAPPING(mmu_port) = pport;
            mmu_port --;
        }
    }

    /* Assign MMU port for all ports with speed >= 50G */
    for (pport = BCM5607X_PORT_MAX; pport > 1; pport--) {
        if (SOC_PORT_P2M_MAPPING(pport) != -1) {
            continue;
        }

        lport = SOC_PORT_P2L_MAPPING(pport);
        if ((lport != -1) && (SOC_PORT_SPEED_MAX(lport) >= 50000)) {
            SOC_PORT_P2M_MAPPING(pport) = mmu_port;
            SOC_PORT_M2P_MAPPING(mmu_port) = pport;
            mmu_port --;
        }
    }

    /* Assign MMU port for all ports with speed >= 40G */
    for (pport = BCM5607X_PORT_MAX; pport > 1; pport--) {
        if (SOC_PORT_P2M_MAPPING(pport) != -1) {
            continue;
        }

        lport = SOC_PORT_P2L_MAPPING(pport);
        if ((lport != -1) && (SOC_PORT_SPEED_MAX(lport) >= 40000)) {
            SOC_PORT_P2M_MAPPING(pport) = mmu_port;
            SOC_PORT_M2P_MAPPING(mmu_port) = pport;
            mmu_port --;
        }
    }

    /* Assign MMU port for all ports with speed >= 25G */
    for (pport = BCM5607X_PORT_MAX; pport > 1; pport--) {
        if (SOC_PORT_P2M_MAPPING(pport) != -1) {
            continue;
        }

        lport = SOC_PORT_P2L_MAPPING(pport);
        if ((lport != -1) && (SOC_PORT_SPEED_MAX(lport) >= 25000)) {
            SOC_PORT_P2M_MAPPING(pport) = mmu_port;
            SOC_PORT_M2P_MAPPING(mmu_port) = pport;
            mmu_port --;
        }
    }

    /* Assign MMU port for all ports with speed >= 20G */
    for (pport = BCM5607X_PORT_MAX; pport > 1; pport--) {
        if (SOC_PORT_P2M_MAPPING(pport) != -1) {
            continue;
        }

        lport = SOC_PORT_P2L_MAPPING(pport);
        if ((lport != -1) && (SOC_PORT_SPEED_MAX(lport) >= 20000)) {
            SOC_PORT_P2M_MAPPING(pport) = mmu_port;
            SOC_PORT_M2P_MAPPING(mmu_port) = pport;
            mmu_port --;
        }
    }

    /* Assign MMU port for all ports with speed >= 10G */
    for (pport = BCM5607X_PORT_MAX; pport > 1; pport--) {
        if (SOC_PORT_P2M_MAPPING(pport) != -1) {
            continue;
        }

        lport = SOC_PORT_P2L_MAPPING(pport);
        if ((lport != -1) && (SOC_PORT_SPEED_MAX(lport) >= 10000)) {
            SOC_PORT_P2M_MAPPING(pport) = mmu_port;
            SOC_PORT_M2P_MAPPING(mmu_port) = pport;
            mmu_port --;
        }
    }

    /* Assign MMU port for all remaining ports */
    for (pport = BCM5607X_PORT_MAX; pport > 1; pport--) {
        if (SOC_PORT_P2M_MAPPING(pport) != -1) {
            continue;
        }

        lport = SOC_PORT_P2L_MAPPING(pport);
        if (lport != -1){
            SOC_PORT_P2M_MAPPING(pport) = mmu_port;
            SOC_PORT_M2P_MAPPING(mmu_port) = pport;
            mmu_port --;
        }
    }

    PBMP_CLEAR(BCM5607X_ALL_PORTS_MASK);
    SOC_LPORT_ITER(lport) {
         soc_port_block_info_get(unit, SOC_PORT_L2P_MAPPING(lport),
                                 &SOC_PORT_BLOCK_TYPE(lport),
                                 &SOC_PORT_BLOCK(lport), &SOC_PORT_BLOCK_INDEX(lport), &SOC_PMQ_BLOCK_INDEX(lport));
         PBMP_PORT_ADD(BCM5607X_ALL_PORTS_MASK, lport);
    }

    SOC_LPORT_ITER(lport) {
        SOC_PORT_SPEED_INIT(lport) = SOC_PORT_SPEED_MAX(lport);
        SOC_PORT_MODE(lport) = SOC_FL_PORT_MODE_QUAD;
        SOC_PORT_LANE_NUMBER(lport) = 1;
        if (IS_QTCE_PORT(lport)) {
            SOC_PORT_LANE_NUMBER(lport) = 4;
        }
    }

}

sys_error_t
soc_bondoption_init(uint8 unit) {

    CHIP_CONFIG_OTP_t chip_config_otp;

    DMU_PCU_OTP_CONFIG_0r_t dmu_pcu_otp_config_0;
    DMU_PCU_OTP_CONFIG_1r_t dmu_pcu_otp_config_1;
    DMU_PCU_OTP_CONFIG_2r_t dmu_pcu_otp_config_2;
    DMU_PCU_OTP_CONFIG_3r_t dmu_pcu_otp_config_3;
    DMU_PCU_OTP_CONFIG_4r_t dmu_pcu_otp_config_4;
    DMU_PCU_OTP_CONFIG_5r_t dmu_pcu_otp_config_5;
    DMU_PCU_OTP_CONFIG_6r_t dmu_pcu_otp_config_6;
    DMU_PCU_OTP_CONFIG_7r_t dmu_pcu_otp_config_7;
    DMU_PCU_OTP_CONFIG_8r_t dmu_pcu_otp_config_8;
    DMU_PCU_OTP_CONFIG_9r_t dmu_pcu_otp_config_9;
    DMU_PCU_OTP_CONFIG_10r_t dmu_pcu_otp_config_10;
    DMU_PCU_OTP_CONFIG_11r_t dmu_pcu_otp_config_11;
    DMU_PCU_OTP_CONFIG_12r_t dmu_pcu_otp_config_12;
    DMU_PCU_OTP_CONFIG_13r_t dmu_pcu_otp_config_13;
    DMU_PCU_OTP_CONFIG_14r_t dmu_pcu_otp_config_14;
    DMU_PCU_OTP_CONFIG_15r_t dmu_pcu_otp_config_15;
    DMU_PCU_OTP_CONFIG_16r_t dmu_pcu_otp_config_16;
    DMU_PCU_OTP_CONFIG_17r_t dmu_pcu_otp_config_17;
    DMU_PCU_OTP_CONFIG_18r_t dmu_pcu_otp_config_18;
    DMU_PCU_OTP_CONFIG_19r_t dmu_pcu_otp_config_19;
    DMU_PCU_OTP_CONFIG_20r_t dmu_pcu_otp_config_20;
    DMU_PCU_OTP_CONFIG_21r_t dmu_pcu_otp_config_21;
    DMU_PCU_OTP_CONFIG_22r_t dmu_pcu_otp_config_22;
    DMU_PCU_OTP_CONFIG_23r_t dmu_pcu_otp_config_23;
    DMU_PCU_OTP_CONFIG_24r_t dmu_pcu_otp_config_24;
    DMU_PCU_OTP_CONFIG_25r_t dmu_pcu_otp_config_25;
    DMU_PCU_OTP_CONFIG_26r_t dmu_pcu_otp_config_26;
    DMU_PCU_OTP_CONFIG_27r_t dmu_pcu_otp_config_27;
    DMU_PCU_OTP_CONFIG_28r_t dmu_pcu_otp_config_28;
    DMU_PCU_OTP_CONFIG_29r_t dmu_pcu_otp_config_29;
    DMU_PCU_OTP_CONFIG_30r_t dmu_pcu_otp_config_30;
    DMU_PCU_OTP_CONFIG_31r_t dmu_pcu_otp_config_31;

    READ_DMU_PCU_OTP_CONFIG_0r(unit, dmu_pcu_otp_config_0);
    READ_DMU_PCU_OTP_CONFIG_1r(unit, dmu_pcu_otp_config_1);
    READ_DMU_PCU_OTP_CONFIG_2r(unit, dmu_pcu_otp_config_2);
    READ_DMU_PCU_OTP_CONFIG_3r(unit, dmu_pcu_otp_config_3);
    READ_DMU_PCU_OTP_CONFIG_4r(unit, dmu_pcu_otp_config_4);
    READ_DMU_PCU_OTP_CONFIG_5r(unit, dmu_pcu_otp_config_5);
    READ_DMU_PCU_OTP_CONFIG_6r(unit, dmu_pcu_otp_config_6);
    READ_DMU_PCU_OTP_CONFIG_7r(unit, dmu_pcu_otp_config_7);
    READ_DMU_PCU_OTP_CONFIG_8r(unit, dmu_pcu_otp_config_8);
    READ_DMU_PCU_OTP_CONFIG_9r(unit, dmu_pcu_otp_config_9);
    READ_DMU_PCU_OTP_CONFIG_10r(unit, dmu_pcu_otp_config_10);
    READ_DMU_PCU_OTP_CONFIG_11r(unit, dmu_pcu_otp_config_11);
    READ_DMU_PCU_OTP_CONFIG_12r(unit, dmu_pcu_otp_config_12);
    READ_DMU_PCU_OTP_CONFIG_13r(unit, dmu_pcu_otp_config_13);
    READ_DMU_PCU_OTP_CONFIG_14r(unit, dmu_pcu_otp_config_14);
    READ_DMU_PCU_OTP_CONFIG_15r(unit, dmu_pcu_otp_config_15);
    READ_DMU_PCU_OTP_CONFIG_16r(unit, dmu_pcu_otp_config_16);
    READ_DMU_PCU_OTP_CONFIG_17r(unit, dmu_pcu_otp_config_17);
    READ_DMU_PCU_OTP_CONFIG_18r(unit, dmu_pcu_otp_config_18);
    READ_DMU_PCU_OTP_CONFIG_19r(unit, dmu_pcu_otp_config_19);
    READ_DMU_PCU_OTP_CONFIG_20r(unit, dmu_pcu_otp_config_20);
    READ_DMU_PCU_OTP_CONFIG_21r(unit, dmu_pcu_otp_config_21);
    READ_DMU_PCU_OTP_CONFIG_22r(unit, dmu_pcu_otp_config_22);
    READ_DMU_PCU_OTP_CONFIG_23r(unit, dmu_pcu_otp_config_23);
    READ_DMU_PCU_OTP_CONFIG_24r(unit, dmu_pcu_otp_config_24);
    READ_DMU_PCU_OTP_CONFIG_25r(unit, dmu_pcu_otp_config_25);
    READ_DMU_PCU_OTP_CONFIG_26r(unit, dmu_pcu_otp_config_26);
    READ_DMU_PCU_OTP_CONFIG_27r(unit, dmu_pcu_otp_config_27);
    READ_DMU_PCU_OTP_CONFIG_28r(unit, dmu_pcu_otp_config_28);
    READ_DMU_PCU_OTP_CONFIG_29r(unit, dmu_pcu_otp_config_29);
    READ_DMU_PCU_OTP_CONFIG_30r(unit, dmu_pcu_otp_config_30);
    READ_DMU_PCU_OTP_CONFIG_31r(unit, dmu_pcu_otp_config_31);

    CHIP_CONFIG_OTP_SET(chip_config_otp, 0,
         DMU_PCU_OTP_CONFIG_0r_GET(dmu_pcu_otp_config_0));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 1,
         DMU_PCU_OTP_CONFIG_1r_GET(dmu_pcu_otp_config_1));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 2,
         DMU_PCU_OTP_CONFIG_2r_GET(dmu_pcu_otp_config_2));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 3,
         DMU_PCU_OTP_CONFIG_3r_GET(dmu_pcu_otp_config_3));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 4,
         DMU_PCU_OTP_CONFIG_4r_GET(dmu_pcu_otp_config_4));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 5,
         DMU_PCU_OTP_CONFIG_5r_GET(dmu_pcu_otp_config_5));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 6,
         DMU_PCU_OTP_CONFIG_6r_GET(dmu_pcu_otp_config_6));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 7,
         DMU_PCU_OTP_CONFIG_7r_GET(dmu_pcu_otp_config_7));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 8,
         DMU_PCU_OTP_CONFIG_8r_GET(dmu_pcu_otp_config_8));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 9,
         DMU_PCU_OTP_CONFIG_9r_GET(dmu_pcu_otp_config_9));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 10,
         DMU_PCU_OTP_CONFIG_10r_GET(dmu_pcu_otp_config_10));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 11,
         DMU_PCU_OTP_CONFIG_11r_GET(dmu_pcu_otp_config_11));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 12,
         DMU_PCU_OTP_CONFIG_12r_GET(dmu_pcu_otp_config_12));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 13,
         DMU_PCU_OTP_CONFIG_13r_GET(dmu_pcu_otp_config_13));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 14,
         DMU_PCU_OTP_CONFIG_14r_GET(dmu_pcu_otp_config_14));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 15,
         DMU_PCU_OTP_CONFIG_15r_GET(dmu_pcu_otp_config_15));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 16,
         DMU_PCU_OTP_CONFIG_16r_GET(dmu_pcu_otp_config_16));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 17,
         DMU_PCU_OTP_CONFIG_17r_GET(dmu_pcu_otp_config_17));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 18,
         DMU_PCU_OTP_CONFIG_18r_GET(dmu_pcu_otp_config_18));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 19,
         DMU_PCU_OTP_CONFIG_19r_GET(dmu_pcu_otp_config_19));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 20,
         DMU_PCU_OTP_CONFIG_20r_GET(dmu_pcu_otp_config_20));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 21,
         DMU_PCU_OTP_CONFIG_21r_GET(dmu_pcu_otp_config_21));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 22,
         DMU_PCU_OTP_CONFIG_22r_GET(dmu_pcu_otp_config_22));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 23,
         DMU_PCU_OTP_CONFIG_23r_GET(dmu_pcu_otp_config_23));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 24,
         DMU_PCU_OTP_CONFIG_24r_GET(dmu_pcu_otp_config_24));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 25,
         DMU_PCU_OTP_CONFIG_25r_GET(dmu_pcu_otp_config_25));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 26,
         DMU_PCU_OTP_CONFIG_26r_GET(dmu_pcu_otp_config_26));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 27,
         DMU_PCU_OTP_CONFIG_27r_GET(dmu_pcu_otp_config_27));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 28,
         DMU_PCU_OTP_CONFIG_28r_GET(dmu_pcu_otp_config_28));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 29,
         DMU_PCU_OTP_CONFIG_29r_GET(dmu_pcu_otp_config_29));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 30,
         DMU_PCU_OTP_CONFIG_30r_GET(dmu_pcu_otp_config_30));
    CHIP_CONFIG_OTP_SET(chip_config_otp, 31,
         DMU_PCU_OTP_CONFIG_31r_GET(dmu_pcu_otp_config_31));

    switch (CHIP_CONFIG_OTP_L2_ENTRY_SIZEf_GET(chip_config_otp)) {
            case 0: /* 2K */
                 fl_sw_info.l2_entry_size = 2048;
            break;
            case 1: /* 4K */
                 fl_sw_info.l2_entry_size = 4096;
            break;
            case 2: /* 8K */
                 fl_sw_info.l2_entry_size = 8096;
            break;
            default:
            case 3: /* 16K */
                 fl_sw_info.l2_entry_size = 16384;
            break;
    }

    switch (CHIP_CONFIG_OTP_L2MC_SIZEf_GET(chip_config_otp)) {
            case 0: /* 256 entries */
                fl_sw_info.l2_mc_size = 256/4;
            break;
            case 1: /* 1024 entries */
            default:
                fl_sw_info.l2_mc_size = 1024/4;
            break;
    }

    switch (CHIP_CONFIG_OTP_CBP_BUFFER_SIZEf_GET(chip_config_otp)) {
            case 3:
                fl_sw_info.cbp_buffer_size = 2 * 1024 * 1024;
            break;
            case 2:
                fl_sw_info.cbp_buffer_size = 1 * 1024 * 1024;
            break;
            case 0:
                fl_sw_info.cbp_buffer_size = 1 * 1024 * 1024;
            break;
            default:
            case 1:
                fl_sw_info.cbp_buffer_size = 2 * 1024 * 1024;
            break;
    }

    fl_sw_info.disable_serdes_core = (CHIP_CONFIG_OTP_PM4X10Q_DISABLEf_GET(chip_config_otp) |
                (CHIP_CONFIG_OTP_PM4X25_DISABLEf_GET(chip_config_otp) << 3));

#if 0
    sal_printf("fl_sw_info.l2_entry_size =%d\n", fl_sw_info.l2_entry_size);
    sal_printf("fl_sw_info.l2_mc_size =%d\n", fl_sw_info.l2_mc_size);
    sal_printf("fl_sw_info.cbp_buffer_size =%d\n", fl_sw_info.cbp_buffer_size);
    sal_printf("fl_sw_info.disable_serdes_core =%d\n", fl_sw_info.disable_serdes_core);
    sal_printf("fl_sw_info.macsec =%d\n", CHIP_CONFIG_OTP_MACSEC_ENABLEf_GET(chip_config_otp));
#endif
    return SYS_OK;
}

sys_error_t
soc_port_config_init(uint8 unit) {

    int i;
    int lport;
    uint8 bypass_checker = 0;
    const char *config_id = NULL;
    pbmp_t lpbmp, lpbmp_mask;
    int rv = SYS_OK;
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    soc_info_t *si = &SOC_INFO(0);

    /* Override of sku id and option id */
    if (sal_config_uint16_get(SAL_CONFIG_SKU_DEVID, &fl_sw_info.devid) == SYS_OK) {
        sal_printf("Vendor Config : Overwrite SKU device ID with value 0x%x.\n", fl_sw_info.devid);
    }

    config_id = sal_config_get(SAL_CONFIG_SKU_OPTION);
    if (config_id != NULL) {
        sal_printf("Vendor Config : sku_option = %s.\n", config_id);
    }
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

    for (i=0; i < _FL_ARRAY_SIZE(core_options); i++) {
         if (soc_device_id_matcher(fl_sw_info.devid, &core_options[i]) == SYS_OK) {
             break;
         }
    }

    if (i == _FL_ARRAY_SIZE(core_options)) {
        sal_printf("device id %x is not supported.\n", fl_sw_info.devid);
        sal_printf("Each ports will be turn off.\n");
        sku_port_config = &fl_null_option_port_config;
        bypass_checker = 1;
        goto port_config_1;
    }

     /* If there is no valid static port config, try to create one from portmap setting */
     if (sku_port_config == NULL) {
         sku_port_config = &fl_auto_tdm_port_config;
         if (config_id != NULL) {
             sal_strcpy(sku_port_config->config_op, config_id);
         } else {
             sal_printf("Vendor Config : Use default SKU option 5.\n");
             sal_strcpy(sku_port_config->config_op, OPTION_5);
         }
         rv = soc_portmap_parser(unit);
         if (rv != SYS_OK) {
             if (rv != SYS_ERR_NOT_FOUND) {
                 sal_printf("Vendor Config : soc_portmap_parser failed.\n");
                 sal_printf("Vendor Config : Inapproprate \"portmap\" setting.\n");
                 sal_printf("Vendor Config : Please check vendor config and chip part number.\n");
             }
             sal_printf("Vendor Config : Use default port config (10P 10G + 2P 100G).\n");
             sku_port_config = &fl_56070_default_port_config;
         }
     }

port_config_1:
     /* Read bond option from chip */
     soc_bondoption_init(unit);

     /* Initialize internal strucuture of port mapping */
     soc_portmap_init(unit);

     /* Run port map checker to check the portmap and max_speed comply with Firelight PRD */
     if ((soc_portmap_checker(unit) != SYS_OK) && (bypass_checker == 0)) {
         sal_printf("Vendor Config : soc_portmap_checker failed.\n");
         sal_printf("Vendor Config : Inapproprate \"portmap\" setting.\n");
         sal_printf("Vendor Config : Please check vendor config and chip part number. \n");
         sal_printf("Vendor Config : Use default port setting (OPTION_5, 10P 10G + 2P 100G).\n");
         sku_port_config = &fl_56070_default_port_config;

         soc_portmap_init(unit);
         soc_portmap_checker(unit);
     }

    /* Initialize the macrolayer for register/memory access */
    bcm5607x_init_port_block_map(fl_sw_info.devid, fl_sw_info.revid);

    /* To disable ports according to "valid_logical_ports" and on-broad strap pin */
    PBMP_ASSIGN(lpbmp, BCM5607X_ALL_PORTS_MASK);
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    if (sal_config_pbmp_get(SAL_CONFIG_VALID_PORTS, &lpbmp_mask) == SYS_OK) {
            PBMP_AND(lpbmp, lpbmp_mask);
            sal_printf("Vendor Config : Set valid logical pbmp mask with value 0x%s.\n", SOC_PBMP_FORMAT(lpbmp));
    }
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

    SOC_PORT_COUNT(unit) = 0;
    SOC_LPORT_ITER(lport) {
        if (PBMP_MEMBER(lpbmp, lport)) {
            SOC_PORT_COUNT(unit)++;
        } else {
            SOC_PORT_L2P_MAPPING(lport) = -1;
            PBMP_PORT_REMOVE(BCM5607X_ALL_PORTS_MASK, lport);
        }
    }

    INIT_PORT(ge);
    INIT_PORT(xe);
    INIT_PORT(ce);
    INIT_PORT(cl);
    INIT_PORT(xl);
    INIT_PORT(all);
    INIT_PORT(port);

    SOC_LPORT_ITER(lport) {
         /* Initialize port type accoding block type , speed and feature */
         if (SOC_PORT_BLOCK_TYPE(lport) == PORT_BLOCK_TYPE_CLPORT) {
             ADD_PORT(cl, lport);
             if (SOC_PORT_SPEED_INIT(lport) == 100000) {
                 ADD_PORT(ce, lport);
                 sal_sprintf(si->port_name[lport], "ce%d", si->ce.num);
                 si->port_offset[lport] = si->ce.num;
             } else {
                 ADD_PORT(xe, lport);
                 sal_sprintf(si->port_name[lport], "xe%d", si->xe.num);
                 si->port_offset[lport] = si->xe.num;
             }
         } else if (SOC_PORT_BLOCK_TYPE(lport) == PORT_BLOCK_TYPE_XLPORT) {
                 ADD_PORT(xl, lport);
                 if (SOC_PORT_SPEED_INIT(lport) < 10000) {
                     sal_sprintf(si->port_name[lport], "ge%d", si->ge.num);
                     si->port_offset[lport] = si->ge.num;
                     ADD_PORT(ge, lport);
                 } else {
                     sal_sprintf(si->port_name[lport], "xe%d", si->xe.num);
                     si->port_offset[lport] = si->xe.num;
                     ADD_PORT(xe, lport);
                 }
           } else {
                 ADD_PORT(ge, lport);
                 sal_sprintf(si->port_name[lport], "ge%d", si->ge.num);
                 si->port_offset[lport] = si->ge.num;
           }
           ADD_PORT(port, lport);
           ADD_PORT(all, lport);
    }

    return SYS_OK;
}
