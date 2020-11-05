/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"

#define UM_DEBUG    0

#define _WH2_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
int matched_devid_idx;


static const int p2l_mapping_default[] = {
    0, -1,  2,  3,  4,  5,  6,  7,
    8,  9, 10, 11, 12, 13, -1, -1,
   -1, -1, 14, 15, 16, 17, 18, 19,
   20, 21, 22, 23, 24, 25, -1, -1,
   -1, -1, 26, 27, 28, 29
};

static const int port_speed_max_default[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};

static const uint32 tdm_24p[] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22, 34,  7, 27, 23, 35,
     8, 28, 24, 36,  9, 29, 25, 37,
     0, 63
};

static const int port_speed_max_op1[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 25, 25, 25, 25, -1, -1,
    -1, -1, 25, 25, 25, 25
};

static const int port_speed_max_op2[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};

static const int port_speed_max_op3[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};

static const int port_speed_max_op4_5_6[] = {
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};

static const int port_speed_max_op7[] = {
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 25, 25, 25, 25, -1, -1,
    -1, -1, 25, 25, 25, 25
};

static const int port_speed_max_op8[] = {
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};

static const int port_speed_max_op9[] = {
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};

static const int port_speed_max_op10_11_12[] = {
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};

static const int port_speed_max_op13[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25, -1, -1,
    -1, -1, 25, 25, 25, 25
};

static const int port_speed_max_op14[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};

static const int port_speed_max_op15[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 25, 25, 25, 25
};

static const int port_speed_max_op16_17_18[] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10, -1, -1,
    -1, -1, 10, 10, 10, 10
};

static const int p2l_mapping_tdm_0[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};

static const uint32 tdm_0[] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22, 34,  7, 27, 23, 35,
     8, 28, 24, 36,  9, 29, 25, 37,
     0, 63
};

static const int p2l_mapping_tdm_1[] = {
     0, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  2,  3,  4,  5, -1, -1,
    -1, -1,  6,  7,  8,  9, -1, -1,
    -1, -1, 10, 11, 12, 13
};

static const uint32 tdm_1[] = {
    18, 26, 34,  0, 19, 27, 35, 63,
    20, 28, 36, 63, 21, 29, 37, 63
};

static const int p2l_mapping_tdm_2[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};

static const uint32 tdm_2[] = {
     2, 10, 18, 34,  3, 11, 19, 35,
     4, 12, 20, 36,  5, 13, 21, 37,
     6, 26, 22,  0,  7, 27, 23, 63,
     8, 28, 24, 63,  9, 29, 25, 63
};

static const int p2l_mapping_tdm_3[] = {
     0, -1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, -1, -1,
    -1, -1, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, -1, -1,
    -1, -1, 26, 27, 28, 29
};

static const uint32 tdm_3[] = {
     2, 18, 26, 34, 10,  3, 27, 35,
    19, 11, 28, 36,  4, 20, 29, 37,
    12,  5, 26, 34, 21, 13, 27, 35,
     6, 22, 28, 36,  7, 23, 29, 37,
     8, 24,  0, 63,  9, 25, 63, 63
};

static const int p2l_mapping_tdm_4[] = {
     0, -1,  2,  3,  4,  5, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,  6,  7,  8,  9, 10, 11,
    12, 13, 14, 15, 16, 17, -1, -1,
    -1, -1, 18, 19, 20, 21
};

static const uint32 tdm_4[] = {
     2, 18, 26, 34,  3, 19, 27, 35,
     4, 20, 28, 36,  5, 21, 29, 37,
     0, 22, 26, 34, 63, 23, 27, 35,
    63, 24, 28, 36, 63, 25, 29, 37
};


wolfhound2_sku_info_t wh2_sku_port_config[] = {
    /* 53540 : 24 + 4 */
    {BCM53540_DEVICE_ID, 2, 78, OPTION_2,
     p2l_mapping_default, port_speed_max_default, NULL,
     tdm_24p, 34, 0x3F, 0xF, 0, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53547_DEVICE_ID, 1, 104, OPTION_1,
     p2l_mapping_tdm_3, port_speed_max_op1, NULL,
     tdm_3, 40, 0x1F, 0, 0xF, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53547_DEVICE_ID, 2, 104, OPTION_2,
     p2l_mapping_tdm_0, port_speed_max_op2, NULL,
     tdm_0, 34, 0x3F, 0xF, 0, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53547_DEVICE_ID, 3, 104, OPTION_3,
     p2l_mapping_tdm_0, port_speed_max_op3, NULL,
     tdm_0, 34, 0x3F, 0x3, 0xC, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53547_DEVICE_ID, 4, 62, OPTION_4,
     p2l_mapping_tdm_2, port_speed_max_op4_5_6, NULL,
     tdm_2, 32, 0x3F, 0x3, 0xC, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53547_DEVICE_ID, 5, 62, OPTION_5,
     p2l_mapping_tdm_2, port_speed_max_op4_5_6, NULL,
     tdm_2, 32, 0x1F, 0, 0xF, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53547_DEVICE_ID, 6, 62, OPTION_6,
     p2l_mapping_tdm_2, port_speed_max_op4_5_6, NULL,
     tdm_2, 32, 0x3F, 0xF, 0, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53548_DEVICE_ID, 7, 104, OPTION_7,
     p2l_mapping_tdm_4, port_speed_max_op7, NULL,
     tdm_4, 32, 0x19, 0, 0xF, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53548_DEVICE_ID, 8, 104, OPTION_8,
     p2l_mapping_tdm_0, port_speed_max_op8, NULL,
     tdm_0, 34, 0x39, 0xF, 0, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53548_DEVICE_ID, 9, 104, OPTION_9,
     p2l_mapping_tdm_0, port_speed_max_op9, NULL,
     tdm_0, 34, 0x39, 0x3, 0xC, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53548_DEVICE_ID, 10, 62, OPTION_10,
     p2l_mapping_tdm_2, port_speed_max_op10_11_12, NULL,
     tdm_2, 32, 0x39, 0x3, 0xC, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53548_DEVICE_ID, 11, 62, OPTION_11,
     p2l_mapping_tdm_2, port_speed_max_op10_11_12, NULL,
     tdm_2, 32, 0x19, 0, 0xF, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53548_DEVICE_ID, 12, 62, OPTION_12,
     p2l_mapping_tdm_2, port_speed_max_op10_11_12, NULL,
     tdm_2, 32, 0x39, 0xF, 0, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53549_DEVICE_ID, 13, 104, OPTION_13,
     p2l_mapping_tdm_1, port_speed_max_op13, NULL,
     tdm_1, 16, 0x8, 0, 0xF, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53549_DEVICE_ID, 14, 104, OPTION_14,
     p2l_mapping_tdm_1, port_speed_max_op14, NULL,
     tdm_1, 16, 0x28, 0xF, 0, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53549_DEVICE_ID, 15, 104, OPTION_15,
     p2l_mapping_tdm_1, port_speed_max_op15, NULL,
     tdm_1, 16, 0x28, 0x3, 0xC, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53549_DEVICE_ID, 16, 41, OPTION_16,
     p2l_mapping_tdm_1, port_speed_max_op16_17_18, NULL,
     tdm_1, 16, 0x28, 0x3, 0xC, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53549_DEVICE_ID, 17, 41, OPTION_17,
     p2l_mapping_tdm_1, port_speed_max_op16_17_18, NULL,
     tdm_1, 16, 0x8, 0, 0xF, SGMIIPX4_INTERFACE_SGMII
    },
    {BCM53549_DEVICE_ID, 18, 41, OPTION_18,
     p2l_mapping_tdm_1, port_speed_max_op16_17_18, NULL,
     tdm_1, 16, 0x28, 0xF, 0, SGMIIPX4_INTERFACE_SGMII
    },
    {0}
};

wolfhound2_sku_info_t *sku_port_config = NULL;
uint8 sgmiipx4_interface[SGMIIP4_NUM_OF_CORES][4];

/*
* Input logical "port"
* Output "*is_gphy" to specify if this port is GPHY(especially providing the indication for the 5th qgphy_core)
*/
int
soc_wolfhound2_gphy_get(int unit, int port, uint8 *is_gphy)
{
    wolfhound2_sku_info_t  *matched_port_config = NULL;
    soc_info_t      *si;
    int             qgphy_id, phy_port;
    uint32          qgphy_core_map;
    uint8           qgphy5_lane;

    matched_port_config = &wh2_sku_port_config[matched_devid_idx];
    qgphy_core_map = matched_port_config->qgphy_core_map;
    qgphy5_lane = matched_port_config->qgphy5_lane;

    si = &SOC_INFO(unit);
    phy_port = si->port_l2p_mapping[port];

    /* get the quad gphy id */
    /* 0:2-5, 1:6-9, 2:10-13, 3:18-21, 4:22-25, 5:26-29 */
    qgphy_id = (phy_port - 2) / 4;
    if (phy_port >= 34) {
        /* SGMII_4P1 => qgphy_id = -1 */
        qgphy_id = -1;
    } else if (phy_port >= 18) {
        qgphy_id--;
    }

    if (qgphy_id != -1) {
        if (qgphy_core_map & (1 << qgphy_id)) {
            if (qgphy_id == 5) {
                if (qgphy5_lane & (1 << (phy_port - 2) % 4)) {
                    *is_gphy = 1;
                } else {
                    *is_gphy = 0;
                }

            } else {
                *is_gphy = 1;
            }
        } else {
            *is_gphy = 0;
        }
    } else {
        *is_gphy = 0;
    }
    return SYS_OK;  
}


#define WH2_QGPHY_CORE_AMAC             (5)
#define WH2_QGPHY_CORE_ALL              (0x3F)
#define WH2_QGPHY_CORE_PBMP_ALL         (0xFFFFFF)
#define WH2_QGPHY_CORE_AMAC_BIT_SHIFT   (20)

static const uint32 wh2_qgphy_core_pbmp [6] = {
    0xF, 0xF0, 0xF00, 0xF000, 0xF0000, 0xF00000
};

int
soc_wh2_qgphy_init(int unit, uint32 qgphy_core_map, uint8 qgphy5_lane)
{
    int     amac = 0, i;
    uint32  pbmp, pbmp_temp, core, core_temp, iddq_pwr, iddq_bias;
    pbmp = 0;
    TOP_QGPHY_CTRL_0r_t     top_qgphy_ctrl_0;
    TOP_QGPHY_CTRL_2r_t     top_qgphy_ctrl_2;
    TOP_SOFT_RESET_REGr_t top_soft_reset_reg;
        
    /* TOP_SOFT_RESET_REG.TOP_QGPHY_RST_L[21:16] = LOW */
    READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
    core = TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_GET(top_soft_reset_reg);
#if 0      
    /* Check if gphy5 is up and used by AMAC */
    if (core & (1 << WH2_QGPHY_CORE_AMAC)) {
        amac = 1;
    }
#endif

    /* power up the QGPHY */
    for (i = 0; i < 6; i++) {
        if (qgphy_core_map & (1 << i)) {
            pbmp_temp |= wh2_qgphy_core_pbmp[i];
        }
    }
    if (amac) {
        pbmp_temp &= ~wh2_qgphy_core_pbmp[WH2_QGPHY_CORE_AMAC];
    }
    
    /* TOP_QGPHY_CTRL_0.EXT_PWRDOWN[23:20] = LOW(except for wh2_qgphy_core_pbmp[WH2_QGPHY_CORE_AMAC]) */
    READ_TOP_QGPHY_CTRL_0r(unit, top_qgphy_ctrl_0);
    pbmp = TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_GET(top_qgphy_ctrl_0);
    pbmp &= ~pbmp_temp;
    TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_SET(top_qgphy_ctrl_0, pbmp);
    WRITE_TOP_QGPHY_CTRL_0r(unit, top_qgphy_ctrl_0);
    

    /* Release iddq isolation */
    core = qgphy_core_map;
    if ((amac) || (qgphy5_lane == 0x3)) {
        core &= ~(1 << WH2_QGPHY_CORE_AMAC);
    }
    
    /* TOP_QGPHY_CTRL_2.GPHY_IDDQ_GLOBAL_PWR[18] = LOW (except for WH2_QGPHY_CORE_AMAC) */
    /* TOP_QGPHY_CTRL_2.IDDQ_BIAS[5] = LOW (except for WH2_QGPHY_CORE_AMAC) */
    READ_TOP_QGPHY_CTRL_2r(unit, top_qgphy_ctrl_2);
    iddq_pwr = TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_GET(top_qgphy_ctrl_2);
    iddq_bias = TOP_QGPHY_CTRL_2r_IDDQ_BIASf_GET(top_qgphy_ctrl_2);
    iddq_pwr &= ~core;
    iddq_bias &= ~core;
    TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_SET(top_qgphy_ctrl_2, iddq_pwr);
    TOP_QGPHY_CTRL_2r_IDDQ_BIASf_SET(top_qgphy_ctrl_2, iddq_bias);
    WRITE_TOP_QGPHY_CTRL_2r(unit, top_qgphy_ctrl_2);
    
    /* Toggle reset */
    /* TOP_SOFT_RESET_REG.TOP_QGPHY_RST_L[21] = HIGH (except for WH2_QGPHY_CORE_AMAC) */
    READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
    TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_SET(top_soft_reset_reg, core);
    WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
    ////////////////////////////////////////////////////////////////////////////////////////////
    /* Full Quad power down */
    pbmp_temp = 0;
    core_temp = WH2_QGPHY_CORE_ALL;
    core_temp &= ~qgphy_core_map;
    if (amac) {
        /* Avoid power down qgphy5 when amac is using it */
        core_temp &= ~(1 << WH2_QGPHY_CORE_AMAC);
    } else if (qgphy5_lane == 0x3) {
        /* Power down qgphy 5 first then enable some bitmap later */
        core_temp |= (1 << WH2_QGPHY_CORE_AMAC);
    }
    if (core_temp) {
        for (i = 0; i < 6; i++) {
            if (core_temp & (1 << i)) {
                pbmp_temp |= wh2_qgphy_core_pbmp[i];
            }
        }
        /* TOP_QGPHY_CTRL_0.EXT_PWRDOWN[23:20] = pbmp */
        READ_TOP_QGPHY_CTRL_0r(unit, top_qgphy_ctrl_0);
        pbmp = TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_GET(top_qgphy_ctrl_0);
        pbmp |= pbmp_temp;
        TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_SET(top_qgphy_ctrl_0, pbmp);
        WRITE_TOP_QGPHY_CTRL_0r(unit, top_qgphy_ctrl_0);
    

        /* Toggle iddq isolation and reset */
        /* iddq_pw1 = 1,  iddq_bias = 1, reset_n = 1*/
        READ_TOP_QGPHY_CTRL_2r(unit, top_qgphy_ctrl_2);
        iddq_pwr = TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_GET(top_qgphy_ctrl_2);
        iddq_bias = TOP_QGPHY_CTRL_2r_IDDQ_BIASf_GET(top_qgphy_ctrl_2);
        iddq_pwr |= core_temp;
        iddq_bias |= core_temp;
        TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_SET(top_qgphy_ctrl_2, iddq_pwr);
        TOP_QGPHY_CTRL_2r_IDDQ_BIASf_SET(top_qgphy_ctrl_2, iddq_bias);
        WRITE_TOP_QGPHY_CTRL_2r(unit, top_qgphy_ctrl_2);
    
        
        READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
        core = TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_GET(top_soft_reset_reg);
        core |= core_temp;
        TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_SET(top_soft_reset_reg, core);
        WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
        ////////////////////////////////////////////////////////////////////////////////////////////
        /* iddq_pw1 = 0,  iddq_bias = 0, reset_n = 0*/
        READ_TOP_QGPHY_CTRL_2r(unit, top_qgphy_ctrl_2);
        iddq_pwr = TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_GET(top_qgphy_ctrl_2);
        iddq_bias = TOP_QGPHY_CTRL_2r_IDDQ_BIASf_GET(top_qgphy_ctrl_2);
        iddq_pwr &= ~core_temp;
        iddq_bias &= ~core_temp;
        TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_SET(top_qgphy_ctrl_2, iddq_pwr);
        TOP_QGPHY_CTRL_2r_IDDQ_BIASf_SET(top_qgphy_ctrl_2, iddq_bias);
        WRITE_TOP_QGPHY_CTRL_2r(unit, top_qgphy_ctrl_2);

        READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
        core = TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_GET(top_soft_reset_reg);
        core &= ~core_temp;
        TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_SET(top_soft_reset_reg, core);
        WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
        ////////////////////////////////////////////////////////////////////////////////////////////
        /* reset_n = 1, iddq_pw1 = 1,  iddq_bias = 1 */
        READ_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
        core = TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_GET(top_soft_reset_reg);
        core |= core_temp;
        TOP_SOFT_RESET_REGr_TOP_QGPHY_RST_Lf_SET(top_soft_reset_reg, core);
        WRITE_TOP_SOFT_RESET_REGr(unit, top_soft_reset_reg);
        
        
        READ_TOP_QGPHY_CTRL_2r(unit, top_qgphy_ctrl_2);
        iddq_pwr = TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_GET(top_qgphy_ctrl_2);
        iddq_bias = TOP_QGPHY_CTRL_2r_IDDQ_BIASf_GET(top_qgphy_ctrl_2);
        iddq_pwr |= core_temp;
        iddq_bias |= core_temp;
        TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_SET(top_qgphy_ctrl_2, iddq_pwr);
        TOP_QGPHY_CTRL_2r_IDDQ_BIASf_SET(top_qgphy_ctrl_2, iddq_bias);
        WRITE_TOP_QGPHY_CTRL_2r(unit, top_qgphy_ctrl_2);
        
    }

    /* Partial power up */
    if (qgphy5_lane == 0x3) {
        if (!amac) {
            /* iddq_pw1 = 0,  iddq_bias = 0*/
            READ_TOP_QGPHY_CTRL_2r(unit, top_qgphy_ctrl_2);
            iddq_pwr = TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_GET(top_qgphy_ctrl_2);
            iddq_bias = TOP_QGPHY_CTRL_2r_IDDQ_BIASf_GET(top_qgphy_ctrl_2);
            iddq_pwr &= ~(1 << WH2_QGPHY_CORE_AMAC);;
            iddq_bias &= ~(1 << WH2_QGPHY_CORE_AMAC);
            TOP_QGPHY_CTRL_2r_GPHY_IDDQ_GLOBAL_PWRf_SET(top_qgphy_ctrl_2, iddq_pwr);
            TOP_QGPHY_CTRL_2r_IDDQ_BIASf_SET(top_qgphy_ctrl_2, iddq_bias);
            WRITE_TOP_QGPHY_CTRL_2r(unit, top_qgphy_ctrl_2);
        }
        /* power up qghphy lane */
        READ_TOP_QGPHY_CTRL_0r(unit, top_qgphy_ctrl_0);
        pbmp = TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_GET(top_qgphy_ctrl_0);
        pbmp &= ~(qgphy5_lane << WH2_QGPHY_CORE_AMAC_BIT_SHIFT);
        TOP_QGPHY_CTRL_0r_EXT_PWRDOWNf_SET(top_qgphy_ctrl_0, pbmp);
        WRITE_TOP_QGPHY_CTRL_0r(unit, top_qgphy_ctrl_0);
    }

    return SYS_OK;
}

/*
* pmq_inst: 0/1 specify the PMQ0_BLOCK_ID/PMQ1_BLOCK_ID
*/
int
soc_wh2_gphy_pmq_init(int unit, int pmq_inst)
{
    GPORT_XGXS0_CTRL_REGr_t gport_xgxs0_ctrl_reg;
    int sleep_usec = CONFIG_EMULATION ? 500000 : 1100;
    uint8 lport;    
	
	if(pmq_inst == 0){
	    lport = SOC_PORT_P2L_MAPPING(PHY_GPORT0_BASE);
    }else{//pmq_inst == 1
        lport = SOC_PORT_P2L_MAPPING(PHY_GPORT2_BASE);
    }

    READ_GPORT_XGXS0_CTRL_REGr(unit, lport, gport_xgxs0_ctrl_reg);
    GPORT_XGXS0_CTRL_REGr_IDDQf_SET(gport_xgxs0_ctrl_reg, 0);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, lport, gport_xgxs0_ctrl_reg);
    
    /* Deassert power down */
    GPORT_XGXS0_CTRL_REGr_PWRDWNf_SET(gport_xgxs0_ctrl_reg, 0);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, lport, gport_xgxs0_ctrl_reg);
    sal_usleep(sleep_usec);
    
    /* Reset XGXS */
    GPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(gport_xgxs0_ctrl_reg, 0);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, lport, gport_xgxs0_ctrl_reg);
    sal_usleep(sleep_usec + 10000);
    
    /* Bring XGXS out of reset */
    GPORT_XGXS0_CTRL_REGr_RSTB_HWf_SET(gport_xgxs0_ctrl_reg, 1);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, lport, gport_xgxs0_ctrl_reg);
    sal_usleep(sleep_usec);
   
    /* Activate MDIO on SGMII */
    GPORT_XGXS0_CTRL_REGr_RSTB_REFCLKf_SET(gport_xgxs0_ctrl_reg, 1);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, lport, gport_xgxs0_ctrl_reg);
    sal_usleep(sleep_usec);
    
    /* Activate clocks */
    GPORT_XGXS0_CTRL_REGr_RSTB_PLLf_SET(gport_xgxs0_ctrl_reg, 1);
    WRITE_GPORT_XGXS0_CTRL_REGr(unit, lport, gport_xgxs0_ctrl_reg);

    return SYS_OK;
}

int
soc_wh2_sgmii_init_top(int unit, int sgmii_inst)
{
    TOP_SGMII_CTRL_REGr_t   top_sgmii_ctrl_reg;
    int sleep_usec = CONFIG_EMULATION ? 500000 : 1100;
    
    if (sgmii_inst != 0)
        sal_printf("%s..:ERROR: sgmii_inst should be 0.\n", __func__);
    
    READ_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl_reg);
  	TOP_SGMII_CTRL_REGr_IDDQf_SET(top_sgmii_ctrl_reg, 0);
  	/* Deassert power down */
    TOP_SGMII_CTRL_REGr_PWRDWNf_SET(top_sgmii_ctrl_reg, 0);
  	WRITE_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl_reg);
  	sal_usleep(sleep_usec); // at least 1ms

    /* Reset SGMII */
    TOP_SGMII_CTRL_REGr_RSTB_HWf_SET(top_sgmii_ctrl_reg, 0);
    WRITE_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl_reg);
    sal_usleep(sleep_usec + 10000);

    /* Bring SGMII out of reset */
    TOP_SGMII_CTRL_REGr_RSTB_HWf_SET(top_sgmii_ctrl_reg, 1);
    WRITE_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl_reg);
    sal_usleep(sleep_usec);

    /* Activate MDIO on SGMII */
    TOP_SGMII_CTRL_REGr_RSTB_MDIOREGSf_SET(top_sgmii_ctrl_reg, 1);
  	WRITE_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl_reg);
  	sal_usleep(sleep_usec);

    /* Activate clocks */
    TOP_SGMII_CTRL_REGr_RSTB_PLLf_SET(top_sgmii_ctrl_reg, 1);
  	WRITE_TOP_SGMII_CTRL_REGr(unit, top_sgmii_ctrl_reg);

    return SYS_OK;
}


int
soc_wh2_sgmii_init_sgmii0(int unit, int sgmii_inst)
{
    GPORT_SGMII0_CTRL_REGr_t   gport_sgmii0_ctrl_reg;
    int sleep_usec = CONFIG_EMULATION ? 500000 : 1100;
    uint8 lport;
    
	if (sgmii_inst != 1)
        sal_printf("%s..:ERROR: sgmii_inst should be 1.\n", __func__);
        
    lport = SOC_PORT_P2L_MAPPING(PHY_GPORT4_BASE);
	
    READ_GPORT_SGMII0_CTRL_REGr(unit, lport, gport_sgmii0_ctrl_reg);
  	GPORT_SGMII0_CTRL_REGr_IDDQf_SET(gport_sgmii0_ctrl_reg, 0);
  	/* Deassert power down */
    GPORT_SGMII0_CTRL_REGr_PWRDWNf_SET(gport_sgmii0_ctrl_reg, 0);
  	WRITE_GPORT_SGMII0_CTRL_REGr(unit, lport, gport_sgmii0_ctrl_reg);
  	sal_usleep(sleep_usec); // at least 1ms

    /* Reset SGMII */
    GPORT_SGMII0_CTRL_REGr_RSTB_HWf_SET(gport_sgmii0_ctrl_reg, 0);
    WRITE_GPORT_SGMII0_CTRL_REGr(unit, lport, gport_sgmii0_ctrl_reg);
    sal_usleep(sleep_usec + 10000);

    /* Bring SGMII out of reset */
    GPORT_SGMII0_CTRL_REGr_RSTB_HWf_SET(gport_sgmii0_ctrl_reg, 1);
    WRITE_GPORT_SGMII0_CTRL_REGr(unit, lport, gport_sgmii0_ctrl_reg);
    sal_usleep(sleep_usec);

    /* Activate MDIO on SGMII */
    GPORT_SGMII0_CTRL_REGr_RSTB_MDIOREGSf_SET(gport_sgmii0_ctrl_reg, 1);
  	WRITE_GPORT_SGMII0_CTRL_REGr(unit, lport, gport_sgmii0_ctrl_reg);
  	sal_usleep(sleep_usec);

    /* Activate clocks */
    GPORT_SGMII0_CTRL_REGr_RSTB_PLLf_SET(gport_sgmii0_ctrl_reg, 1);
  	WRITE_GPORT_SGMII0_CTRL_REGr(unit, lport, gport_sgmii0_ctrl_reg);

    return SYS_OK;
}


int
soc_wolfhound2_port_reset(int unit)
{

    if(wh2_sw_info.devid == 0x8549){
        soc_wh2_gphy_pmq_init(unit, 1);
    }else{
        /* For 53547 and 53548, reset the GPORT_XGXS0_CTRL_REG for both PMQ0_BLOCK_ID and PMQ1_BLOCK_ID */
        soc_wh2_gphy_pmq_init(unit, 0);
        soc_wh2_gphy_pmq_init(unit, 1);
    }
    /* reset SGMII_4P1 */
    soc_wh2_sgmii_init_sgmii0(unit, 1);
    return SYS_OK;
}

static void
soc_port_block_info_get(uint8 unit, uint8 pport, int *block_type, int *block_idx, int *port_idx, int *pmq_port_idx)
{
    *block_type = PORT_BLOCK_TYPE_GXPORT;
    *pmq_port_idx = -1;

    *block_idx = bcm5354x_gport_pport_to_blockid[pport];
    *port_idx = bcm5354x_gport_pport_to_index_in_block[pport];
    *pmq_port_idx = bcm5354x_pmq_pport_to_index_in_block[pport];
    if ((pport < BCM5354X_PORT_MIN) || (pport > BCM5354X_PORT_MAX)) {
        sal_printf("soc_port_block_info_get : invalid pport %d\n", pport);
    }
}


sys_error_t 
soc_port_config_init(uint8 unit) {
    soc_info_t *si = &SOC_INFO(unit);
    int i;
    int pport;
    int uport;
    int tmp_speed_max[BCM5354X_PORT_MAX + 1];
    uint8 core_num, lane_num;
    uint8 lport;
    uint32   config_op = 0;
    char *config_id = NULL;
    int valid_sku_option;
    
    /* Clear all soc_control_t structure */
    sal_memset(&SOC_CONTROL(unit), 0, sizeof(soc_control_t));

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    pbmp_t lpbmp;

    /* Override of sku id and option id */
    if (sal_config_uint16_get(SAL_CONFIG_SKU_DEVID, &wh2_sw_info.devid) == SYS_OK) {
        sal_printf("Vendor Config : SKU device ID with value 0x%x.\n", wh2_sw_info.devid);
    }

    sal_config_uint32_get(SAL_CONFIG_SKU_OPTION, &config_op);
    if (config_op != 0) {
        sal_printf("Vendor Config : Set SKU option with value op%d\n", config_op);
    } else {
        config_op = 1;
    }
    /* [10] should be always enough for "opxx-xx" format */
    config_id = sal_malloc(10);
    if (config_id == NULL) {
        return SYS_ERR_OUT_OF_RESOURCE;
    }
    sal_sprintf(config_id, "op%d", config_op);
    
#if (CONFIG_EMULATION==1)
    config_id = CFG_CONFIG_OPTION;
#endif

#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

    valid_sku_option = -1;
    matched_devid_idx = -1;
    /* Find sku config according to sku device id and option */
    for (i=0; wh2_sku_port_config[i].dev_id != 0; i++) {
        
        if (wh2_sw_info.devid == wh2_sku_port_config[i].dev_id) {
            if (config_id == NULL) {
                config_id = wh2_sku_port_config[i].op_str;
                sal_printf("Warning: sku_option is not specified. Take %s for %x \n", config_id, wh2_sku_port_config[i].dev_id);
            }
            
            if (matched_devid_idx == -1) {
                matched_devid_idx = i;
            }
            
            if (!sal_strcmp(wh2_sku_port_config[i].op_str, config_id)) {
                valid_sku_option = i;
#if UM_DEBUG
                sal_printf("valid_sku_option=%d\n", valid_sku_option);
#endif
                break;
            }
            
        }
    }
    
    if(matched_devid_idx == -1){
        sal_printf("Error : matched_devid_idx=%d\n", matched_devid_idx);
    }
    
    if (valid_sku_option == -1) {
        valid_sku_option = wh2_sku_port_config[matched_devid_idx].config_op;
        sal_printf("Warning: option %s is not supported in BCM53540_A0. Take %s\n", config_id, wh2_sku_port_config[matched_devid_idx].op_str);
        sal_printf("Warning: valid_sku_option=%d\n", valid_sku_option);
    }
    matched_devid_idx = valid_sku_option;
    sku_port_config = &wh2_sku_port_config[matched_devid_idx];       
    
    sal_printf("matched_devid_idx=%d dev_id=%x config_op=%d\n", matched_devid_idx, sku_port_config->dev_id, sku_port_config->config_op); 

    
#if defined(CFG_UM_BCMSIM) || defined(CONFIG_EMULATION)
    if(i==0) {
        sku_port_config = &wh2_sku_port_config[1];       
        sal_printf("\nforce to use BCM53547_DEVICE_ID wh2_sku_port_config as SDK.\n");
    }
#endif

    /* Initialize the runtime data base of the port config */
    for (lport = 0; lport <= BCM5354X_LPORT_MAX ; lport++) {
        SOC_PORT_L2P_MAPPING(lport) = -1; 
        SOC_PORT_M2P_MAPPING(lport) = -1;
    }

    for (pport = 0; pport <= BCM5354X_PORT_MAX ; pport++) {
         SOC_PORT_P2L_MAPPING(pport) = sku_port_config->p2l_mapping[pport];
         SOC_PORT_P2M_MAPPING(pport) = sku_port_config->p2l_mapping[pport];
         
         tmp_speed_max[pport] = sku_port_config->speed_max[pport];
         if (tmp_speed_max[pport] != -1) {
             if (tmp_speed_max[pport] >= 500) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 50000;
             } else if (tmp_speed_max[pport] >= 400) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 40000;
             } else if (tmp_speed_max[pport] >= 250) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 25000;
             } else if (tmp_speed_max[pport] >= 100) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 10000;
             } else if (tmp_speed_max[pport] >= 25) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 2500;
             } else if (tmp_speed_max[pport] >= 10) {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 1000;
             } else {
                 SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)) = 100;
             }
#if UM_DEBUG 
            if(SOC_PORT_P2L_MAPPING(pport) != -1){
                sal_printf("pport=%d lport %d max_speed = %d\n", pport, SOC_PORT_P2L_MAPPING(pport), SOC_PORT_SPEED_MAX(SOC_PORT_P2L_MAPPING(pport)));
            }
#endif
         }
    }

    /* Get SOC_PORT_COUNT from speed_max table */
    SOC_PORT_COUNT(unit) = 0;
    for (pport = 0; pport <= BCM5354X_PORT_MAX ; pport++) {
         if((tmp_speed_max[pport] > 0) && (SOC_PORT_P2L_MAPPING(pport) > 0)) {
             SOC_PORT_COUNT(unit)++;
         }
    }
    sal_printf("SOC_PORT_COUNT(unit) = %d\n", SOC_PORT_COUNT(unit));

    /* Initialize the runtime data base of the port config */
    for (uport = 0; uport <= BCM5354X_UPORT_MAX ; uport++) {
        SOC_PORT_U2L_MAPPING(uport) = -1; 
    }
    for (lport = 0; lport <= BCM5354X_LPORT_MAX ; lport++) {
        SOC_PORT_L2U_MAPPING(lport) = -1; 
    }

    /* init SOC_PORT_L2P_MAPPING */
    for (pport = 0; pport <= BCM5354X_PORT_MAX ; pport++) {
        if (SOC_PORT_P2L_MAPPING(pport) != -1) {
            if (tmp_speed_max[pport] != -1) {
                SOC_PORT_L2P_MAPPING(SOC_PORT_P2L_MAPPING(pport)) = pport;
                SOC_PORT_M2P_MAPPING(SOC_PORT_P2M_MAPPING(pport)) = pport;
            } else if (pport == 0) {
                SOC_PORT_L2P_MAPPING(SOC_PORT_P2L_MAPPING(pport)) = 0;
                SOC_PORT_M2P_MAPPING(SOC_PORT_P2M_MAPPING(pport)) = 0;
            } else {
                SOC_PORT_L2P_MAPPING(SOC_PORT_P2L_MAPPING(pport)) = -1;
                SOC_PORT_M2P_MAPPING(SOC_PORT_P2M_MAPPING(pport)) = -1;
            }
        }
    }

    
    PBMP_CLEAR(BCM5354X_ALL_PORTS_MASK);
    SOC_LPORT_ITER(lport) {
         soc_port_block_info_get(unit, SOC_PORT_L2P_MAPPING(lport),
                                 &SOC_PORT_BLOCK_TYPE(lport),
                                 &SOC_PORT_BLOCK(lport), &SOC_PORT_BLOCK_INDEX(lport), &SOC_PMQ_BLOCK_INDEX(lport));

         PBMP_PORT_ADD(BCM5354X_ALL_PORTS_MASK, lport);
    }
#if UM_DEBUG 
        sal_printf("BCM5354X_ALL_PORTS_MASK = 0x%08x\n", BCM5354X_ALL_PORTS_MASK);
#endif
    

    SOC_LPORT_ITER(lport) {
          SOC_PORT_SPEED_INIT(lport) = SOC_PORT_SPEED_MAX(lport);
          SOC_PORT_MODE(lport) = SOC_WH2_PORT_MODE_QUAD;
          SOC_PORT_LANE_NUMBER(lport) = 1;
    }

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    PBMP_CLEAR(lpbmp);
    if (sal_config_pbmp_get(SAL_CONFIG_SPEED_1000_PORTS, &lpbmp) == SYS_OK) {
        sal_printf("Vendor Config : Set speed (1G) logical pbmp with value %s.\n", SOC_PBMP_FORMAT(lpbmp));
        SOC_LPORT_ITER(lport) {
          if (PBMP_MEMBER(lpbmp, lport) && (SOC_PORT_SPEED_MAX(lport) >= 1000)) {
              sal_printf("Vendor Config : Set SOC_PORT_SPEED_MAX speed (1G) lport=%d\n", lport);
              SOC_PORT_SPEED_INIT(lport) = 1000;
              SOC_PORT_SPEED_MAX(lport) = 1000;
          }
        }
    }         
#endif /*  CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */


    for (core_num = 0; core_num < SGMIIP4_NUM_OF_CORES; core_num++) {
        for (lane_num = 0; lane_num < 4; lane_num++) {
            sgmiipx4_interface[core_num][lane_num] = sku_port_config->sgmiipx4_interface_default;
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
            sal_config_uint8_xy_get(SAL_CONFIG_SGMIIPX4_INTERFACE, core_num, lane_num, &sgmiipx4_interface[core_num][lane_num]);
#endif /*  CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

        }
    }

    SOC_LPORT_ITER(lport) {
        pport = SOC_PORT_L2P_MAPPING(lport);
        if (IS_SGMIIPX4_PORT(lport)) {
            /* Default 1/2.5G one lane mode*/
            SOC_PORT_LANE_NUMBER(lport) = 1; /* SGMII or Fiber*/
            SOC_PORT_MODE(lport) = SOC_WH2_PORT_MODE_QUAD;
        } 
    }
    
    /* Initialize the macrolayer for register/memory access */
    bcm5354x_init_port_block_map(wh2_sw_info.devid, wh2_sw_info.revid);
    
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    for (i =0; i <= BCM5354X_LPORT_MAX; i++){
        SOC_PORT_L2P_MAPPING_VALID(i) = SOC_PORT_L2P_MAPPING(i);
    }
    
    PBMP_CLEAR(lpbmp);
    if (sal_config_pbmp_get(SAL_CONFIG_VALID_PORTS, &lpbmp) == SYS_OK) {
        sal_printf("Vendor Config : Set valid logical pbmp with value %s.\n", SOC_PBMP_FORMAT(lpbmp));
        SOC_PORT_COUNT(unit) = 0;
        PBMP_AND(lpbmp, BCM5354X_ALL_PORTS_MASK);
        SOC_LPORT_ITER(lport) {
            if (PBMP_MEMBER(lpbmp, lport)) {                  
                SOC_PORT_COUNT(unit)++; 
            } else {
                /* Keep the SOC_PORT_L2P_MAPPING(), use SOC_PORT_L2P_MAPPING_VALID() for new VALID ports */
                SOC_PORT_L2P_MAPPING_VALID(lport) = -1;
                PBMP_PORT_REMOVE(BCM5354X_ALL_PORTS_MASK, lport);
            }
        }
        sal_printf("Vendor Config : new BCM5354X_ALL_PORTS_MASK %s\n", SOC_PBMP_FORMAT(BCM5354X_ALL_PORTS_MASK) );
        sal_printf("Vendor Config : new SOC_PORT_COUNT(unit)=%d\n", SOC_PORT_COUNT(unit) );
    } 
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */   


    for (uport = 1; uport <= SOC_PORT_COUNT(unit) ; uport++) {
        /* Note: board_uport_to_lport needs to be after SOC_PORT_COUNT(unit) has been initialized. */
        board_uport_to_lport(uport, &unit, &lport);
        pport = SOC_PORT_L2P_MAPPING(lport);
        sal_printf("uport=%d pport=%d lport %d max_speed = %d\n", uport, pport, lport, SOC_PORT_SPEED_MAX(lport));
        
        SOC_PORT_U2L_MAPPING(uport)=lport;
        SOC_PORT_L2U_MAPPING(lport)=uport;
    }
    SOC_PORT_U2L_MAPPING(0)=0;
    SOC_PORT_L2U_MAPPING(0)=0;

#if UM_DEBUG     
    SOC_UPORT_ITER(uport){
        sal_printf("uport=%d lport %d \n", uport, SOC_PORT_U2L_MAPPING(uport));
    }
#endif

    
    INIT_PORT(ge);
	INIT_PORT(xe);
	INIT_PORT(ce);
	INIT_PORT(cl);
	INIT_PORT(xl);
	INIT_PORT(port);
	INIT_PORT(all);
	
    
    SOC_UPORT_ITER(uport) {
        lport = SOC_PORT_U2L_MAPPING(uport);
        pport = SOC_PORT_L2P_MAPPING(lport);
        /* Initialize port type accoding block type , speed and feature */
        if (SOC_PORT_BLOCK_TYPE(lport) == PORT_BLOCK_TYPE_XLPORT) {
            ADD_PORT(xl, lport);
            if (SOC_PORT_SPEED_MAX(lport) < 10000) {
                ADD_PORT(ge, lport);				
                sal_sprintf(si->port_name[uport], "ge%d", si->ge.num);
                si->port_offset[lport] = si->ge.num;
            } else {
                ADD_PORT(xe, lport);
                sal_sprintf(si->port_name[uport], "xe%d", si->xe.num);
                si->port_offset[lport] = si->xe.num;
            }
        } else {
            /* si->ge.num is initialized to -1 */
            ADD_PORT(ge, lport);
            sal_sprintf(si->port_name[uport], "ge%d", si->ge.num);
            si->port_offset[lport] = si->ge.num;
        }
        ADD_PORT(port, lport);
        ADD_PORT(all, lport);

#if UM_DEBUG 
        sal_printf("uport=%d port_name=%s pport=%d lport %d max_speed = %d\n", uport, si->port_name[uport], pport, lport, SOC_PORT_SPEED_MAX(lport));
#endif
	}
	
    return SYS_OK;
} 
