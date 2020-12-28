/*
 * $Id: flportconf.c,v 1.29 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
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

/* OPTION_5_8, 10P 10G + 2P 100G */
static
const int p2l_mapping_op5_8[] = {
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
const int port_speed_max_op5_8[] = {
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
  .speed_max = auto_tdm_port_speed_max,
  .qtc_interface_default = QTC_INTERFACE_FIBER,
  .tsce_interface_default = TSCE_INTERFACE_FIBER,
  .tscf_interface_default = TSCF_INTERFACE_FIBER
};

const core_prop_t core_prop[FL_SERDES_CORE_COUNT] = {
    { .pport_base = PHY_GPORT0_BASE, .len_max = 4 }, 
    { .pport_base = PHY_GPORT2_BASE, .len_max = 4 }, 
    { .pport_base = PHY_GPORT4_BASE, .len_max = 4 }, 
    { .pport_base = PHY_XLPORT0_BASE, .len_max = 4 }, 
    { .pport_base = PHY_XLPORT1_BASE, .len_max = 4 }, 
    { .pport_base = PHY_XLPORT2_BASE, .len_max = 4 }, 
    { .pport_base = PHY_CLPORT0_BASE, .len_max = 4 }, 
    { .pport_base = PHY_CLPORT1_BASE, .len_max = 4 }, 
    { .pport_base = PHY_CLPORT2_BASE, .len_max = 4 }, 
    { .pport_base = PHY_CLPORT3_BASE, .len_max = 4 }, 
};

static const firelight_sku_info_t fl_null_option_port_config =
{   OPTION_NULL, _FL_SYSTEM_FREQ_700,
    p2l_mapping_op_null, port_speed_max_op_null,
    QTC_INTERFACE_QSGMII,
    TSCE_INTERFACE_FIBER,
    TSCF_INTERFACE_FIBER
};

/* 56070/56072 Default Port Config: OPTION_5_8, 10P 10G + 2P 100G */
static const firelight_sku_info_t fl_56070_default_port_config =
{   OPTION_5_8, _FL_SYSTEM_FREQ_700,
    p2l_mapping_op5_8, port_speed_max_op5_8,
    QTC_INTERFACE_QSGMII,
    TSCE_INTERFACE_FIBER,
    TSCF_INTERFACE_FIBER
};

const firelight_sku_info_t *sku_port_config = NULL;
uint8 tscf_interface[TSCF_NUM_OF_CORES][4];
uint8 tsce_interface[TSCE_NUM_OF_CORES][4];
uint8 qtc_interface[QTCE_NUM_OF_CORES][4];

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

#if 0 
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
    if ((lane_count != 1) &&
        (lane_count != 2) &&
        (lane_count != 4) &&
        (lane_count != -4)) 
    {
        return SYS_ERR;
    }

    if (flex_mode == FLEX_1G) {
         if ((speed == 10)) {
             return SYS_OK;
         }
    } else if (flex_mode == FLEX_QSGMII) {
        if (lane_count == 1) {
            /* 1G, 2.5G */
            if ((speed == 10) || (speed == 25)) {
                return SYS_OK;
            }
        } 
        if (lane_count == -4) {
            if ((speed == 10)) {
                return SYS_OK;
            }
        }
    } else if (flex_mode == FLEX_2P5G) {
        if (lane_count == 1) {
            /* 1G, 2.5G */
            if ((speed == 10) || (speed == 25)) {
                return SYS_OK;
            }
        }
    } else if (flex_mode == FLEX_10G) {
        if (lane_count == 1) {
            /* 1G, 10G */
            if ((speed == 10) || (speed == 100)) {
                return SYS_OK;
            }
        }
    } else if (flex_mode == FLEX_40G) {
        if (lane_count == 1) {
            /* 1G, 2.5G, 5G, 10G*/
            if ((speed == 10) || (speed == 25) || (speed == 50) || \
                (speed == 100)) {
                return SYS_OK;
            }
        }
        if (lane_count == 2) {
            /* 20G*/
            if ((speed == 200)) {
                return SYS_OK;
            }
        }
        if (lane_count == 4) {
            /* 40G */
            if ((speed == 400)) {
                return SYS_OK;
            }
        }
    } else if (flex_mode == FLEX_50G) {
        if (lane_count == 1) {
            /* 1G, 10G, 25G */
            if ((speed == 10) || (speed == 100) || \
                (speed == 250)) {
                return SYS_OK;
            }
        }
        if (lane_count == 2) {
            /* 20G, 50G */
            if ((speed == 200) || (speed == 500)) {
                return SYS_OK;
            }
        }
        if (lane_count == 4) {
            /* 40G */
            if ((speed == 400)) {
                return SYS_OK;
            }
        }
    } else if (flex_mode == FLEX_XAUI) {
        if (lane_count == 1) {
            /* 1G, 2.5G */
            if ((speed == 10) || (speed == 25)) {
                return SYS_OK;
            }
        }
        if (lane_count == 2) {
            return SYS_ERR;
        }
        if (lane_count == 4) {
            /* 10G */
            if (speed == 100) {
                return SYS_OK;
            }
        }
    } else {
        return SYS_ERR;
    }
    return SYS_ERR;
}

static int
soc_fl_flex1g_checker(int unit,
                              int core_idx)
{
    const int *p2l_mapping, *speed_max;


    p2l_mapping = sku_port_config->p2l_mapping;
    speed_max = sku_port_config->speed_max;
    int pport_base = core_prop[core_idx].pport_base;
    int pport, lport;

    for (pport = pport_base; 
         pport < (pport_base + 4); pport++)
    {
         lport = p2l_mapping[pport]; 
         if (lport != -1) {
             SOC_PORT_MODE(lport) = SOC_FL_PORT_MODE_QUAD;
             if (IS_QTCE_PORT(lport)) {
                 SOC_PORT_LANE_NUMBER(lport) = 4;
             } else {
                 SOC_PORT_LANE_NUMBER(lport) = 1;
             }
             if (flex_lane_speed_check
                 (speed_max[pport], FLEX_1G, 1) != SYS_OK) {
                 sal_printf("portmap check fail[pport=%d]: wrong max_speed \n", pport);
                 return SYS_ERR;
             }
         }
         


    }
    return SYS_OK;
}

static int
soc_fl_flex_qsgmii_checker(int unit,
                                     int core_idx)
{
    const int *p2l_mapping, *speed_max;

    p2l_mapping = sku_port_config->p2l_mapping;
    speed_max = sku_port_config->speed_max;

    int pport_base = core_prop[core_idx].pport_base;
    int pport, lport;
    int lane_number = 1;
    int is_qtc;

    is_qtc = (pport_base == PHY_GPORT3_BASE) || (pport_base == PHY_GPORT5_BASE);
    for (pport = (pport_base + 4); pport < (pport_base + 16); pport++) {  
         lport = p2l_mapping[pport]; 
         if (lport != -1)
             SOC_PORT_LANE_NUMBER(lport) = 4; 
    }
    /* Check if it's QSGMII mode */
    if (is_qtc) {  
        for (pport = (pport_base + 4); pport < (pport_base + 16); pport++) {
             lport = p2l_mapping[pport]; 
             if (lport != -1) {
                 SOC_PORT_LANE_NUMBER(lport) = 1;
                 lane_number = -4;
                 if (flex_lane_speed_check(speed_max[pport], FLEX_QSGMII, lane_number) != SYS_OK) {
                     sal_printf("[portmap checker]: %d port max speed is wrong \n", pport);
                     return SYS_ERR;
                 };                  
             }
             if (lport == -1 && lane_number == -4) {
                 /* the portmap should totally match according to the PRD setting */
                 sal_printf("[portmap checker]: p2l_mapping of physical port %d ", pport);
                 sal_printf("should totally match the PRD setting \n");
                 return SYS_ERR;
             }
            
        }        
        for (pport = pport_base; pport < pport_base + 4; pport++) {
             lport = p2l_mapping[pport]; 
             if (lport == -1) {
                 sal_printf("[portmap checker]: p2l_mapping of physical port %d ", pport);
                 sal_printf("should totally match the PRD setting \n");
                 return SYS_ERR;
             }
             if (flex_lane_speed_check(speed_max[pport], FLEX_QSGMII, lane_number) != SYS_OK) {
                 sal_printf("[portmap checker]: %d port max speed is wrong \n", pport);
                 return SYS_ERR;
             };       
             if (lane_number == -4) {
                 SOC_PORT_MODE(lport) = SOC_FL_PORT_MODE_SINGLE;
                 if (IS_QTCE_PORT(lport)) {
                     SOC_PORT_LANE_NUMBER(lport) = 1;
                 } else {
                     SOC_PORT_LANE_NUMBER(lport) = 4;
                 }
             } else {
                 SOC_PORT_MODE(lport) = SOC_FL_PORT_MODE_QUAD;                                     
                 if (IS_QTCE_PORT(lport)) {
                     SOC_PORT_LANE_NUMBER(lport) = 4;
                 } else {
                     SOC_PORT_LANE_NUMBER(lport) = 1;
                 }
             }
        }
    } else {
        sal_printf("[portmap checker]: Only QTC can support FLEX_QSGMII\n");
        return SYS_ERR;
    }
    return SYS_OK;
}

static int
soc_fl_flex2p5g_checker(int unit,
                                 int core_idx)
{
    const int *p2l_mapping, *speed_max;

    int pport, lport;
    int pport_base = core_prop[core_idx].pport_base;
    int is_qtc;
    
    p2l_mapping = sku_port_config->p2l_mapping;
    speed_max = sku_port_config->speed_max;

    is_qtc = (pport_base == PHY_GPORT3_BASE) || (pport_base == PHY_GPORT5_BASE);

    if (is_qtc) {
        for (pport = pport_base + 4; pport < (pport_base + 16); pport++) {
             if (p2l_mapping[pport] != -1) {
                 sal_printf("[portmap checker]: p2l_mapping of physical port %d ", pport);
                 sal_printf("should totally match the PRD setting \n");
                 return SYS_ERR;
             };             
        }

    }
    for (pport = pport_base; pport < (pport_base + 4); pport++) {
         lport = p2l_mapping[pport];
         if (lport == -1) {
             sal_printf("[portmap checker]: p2l_mapping of physical port %d ", pport);
             sal_printf("should totally match the PRD setting \n");
             return SYS_ERR;
         }
         if (flex_lane_speed_check(speed_max[pport], FLEX_2P5G, 1) != SYS_OK) {
             sal_printf("[portmap checker]: %d port max speed is wrong \n", pport);
             return SYS_ERR;
         };           
         SOC_PORT_MODE(lport) = SOC_FL_PORT_MODE_QUAD;
 
         if (IS_QTCE_PORT(lport)) {
             SOC_PORT_LANE_NUMBER(lport) = 4;
         } else {
             SOC_PORT_LANE_NUMBER(lport) = 1;
         }

    }
    
    return SYS_OK;
}


static int
soc_fl_flex10g_checker(int unit,
                                int core_idx)
{
    const int *p2l_mapping, *speed_max;
    
    int pport, lport;
    int pport_base = core_prop[core_idx].pport_base;

    p2l_mapping = sku_port_config->p2l_mapping;
    speed_max = sku_port_config->speed_max;


    for (pport = pport_base; pport < (pport_base + 4); pport++) {
         lport = p2l_mapping[pport]; 
         if (lport != -1) {
             SOC_PORT_LANE_NUMBER(lport) = 1;
             SOC_PORT_MODE(lport) = SOC_FL_PORT_MODE_QUAD;
         }
         if (flex_lane_speed_check(speed_max[pport], FLEX_10G, 1) != SYS_OK) {
             sal_printf("[portmap checker]: %d port max speed is wrong \n", pport);
             return SYS_ERR;
         };             
    }

    return SYS_OK;
}

static int
soc_fl_flex40g_checker(int unit,
                                int core_idx)
{
    const int *p2l_mapping, *speed_max;

    int pport_base = core_prop[core_idx].pport_base;
    int is_falcon;
    p2l_mapping = sku_port_config->p2l_mapping;
    speed_max = sku_port_config->speed_max;

    is_falcon = (pport_base == PHY_CLPORT0_BASE);

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] != -1) &&
        (p2l_mapping[pport_base+2] != -1) &&
        (p2l_mapping[pport_base+3] != -1)) 
    {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_40G, 1) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], FLEX_40G, 1) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], FLEX_40G, 1) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 3], FLEX_40G, 1) == SYS_OK) 
        {
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
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] != -1) &&
        (p2l_mapping[pport_base+3] != -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_40G, 2) == SYS_OK &&
            flex_lane_speed_check
              (speed_max[pport_base+2], FLEX_40G, 1) == SYS_OK &&
            flex_lane_speed_check
              (speed_max[pport_base+3], FLEX_40G, 1) == SYS_OK) 
        {
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 2;
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 1;
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 3]) = 1;            
            SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_TRI_023;
            SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_TRI_023;
            SOC_PORT_MODE(p2l_mapping[pport_base + 3]) = SOC_FL_PORT_MODE_TRI_023;  
            return SYS_OK;
        } 
        
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] != -1) &&
        (p2l_mapping[pport_base+2] != -1) &&
        (p2l_mapping[pport_base+3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base+2], FLEX_40G, 2) == SYS_OK &&
            flex_lane_speed_check
              (speed_max[pport_base], FLEX_40G, 1) == SYS_OK &&
            flex_lane_speed_check
              (speed_max[pport_base+1], FLEX_40G, 1) == SYS_OK) 
        {
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 1;
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 1]) = 1;            
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 2;
            SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_TRI_012;
            SOC_PORT_MODE(p2l_mapping[pport_base + 1]) = SOC_FL_PORT_MODE_TRI_012;
            SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_TRI_012;
            return SYS_OK;
        } 
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] != -1) &&
        (p2l_mapping[pport_base+3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_40G, 2) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], FLEX_40G, 2) == SYS_OK) 
        {
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 2;
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 2;
            SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_DUAL;
            return SYS_OK;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] == -1) &&
        (p2l_mapping[pport_base+3] == -1)) 
    {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_40G, 4) == SYS_OK) {
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 4;
            SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_SINGLE;        
            return SYS_OK;
        }
        /* Check if it is XAUI port */
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_XAUI, 4) == SYS_OK && !is_falcon) {
            if (is_falcon) {
                sal_printf("[portmap] : Falcon Serdes don't support XAUI\n");
                return SYS_ERR;  
            }
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 4;
            SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_SINGLE;
            return SYS_OK;
        }
    }

    if ((p2l_mapping[pport_base] == -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] == -1) &&
        (p2l_mapping[pport_base+3] == -1)) {
        /* No lanes enabled on the core, assign default value */
        return SYS_OK;
    }

    return SYS_ERR;
}

static int
soc_fl_flex50g_checker(int unit,
                                int core_idx)
{
    const int *p2l_mapping, *speed_max;
    int pport_base = core_prop[core_idx].pport_base;
    p2l_mapping = sku_port_config->p2l_mapping;
    speed_max = sku_port_config->speed_max;

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] != -1) &&
        (p2l_mapping[pport_base+2] != -1) &&
        (p2l_mapping[pport_base+3] != -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_50G, 1) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], FLEX_50G, 1) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], FLEX_50G, 1) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 3], FLEX_50G, 1) == SYS_OK) 
        {
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
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] != -1) &&
        (p2l_mapping[pport_base+3] != -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_50G, 2) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], FLEX_50G, 1) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 3], FLEX_50G, 1) == SYS_OK) 
        {
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 2;
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 1;
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 3]) = 1;            
            SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_TRI_023;
            SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_TRI_023;
            SOC_PORT_MODE(p2l_mapping[pport_base + 3]) = SOC_FL_PORT_MODE_TRI_023;  
            
            return SYS_OK;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] != -1) &&
        (p2l_mapping[pport_base+2] != -1) &&
        (p2l_mapping[pport_base+3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_50G, 1) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], FLEX_50G, 1) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], FLEX_50G, 2) == SYS_OK) 
        {
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 1;
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 1]) = 1;            
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 2;
            SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_TRI_012;
            SOC_PORT_MODE(p2l_mapping[pport_base + 1]) = SOC_FL_PORT_MODE_TRI_012;
            SOC_PORT_MODE(p2l_mapping[pport_base + 2]) = SOC_FL_PORT_MODE_TRI_012;
            return SYS_OK;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] != -1) &&
        (p2l_mapping[pport_base+3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_50G, 2) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], FLEX_50G, 2) == SYS_OK) {
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 2;
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base + 2]) = 2;
            SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_DUAL;
            return SYS_OK;
        }
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] == -1) &&
        (p2l_mapping[pport_base+3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_50G, 4) == SYS_OK) {
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 4;
            SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_SINGLE;  
            return SYS_OK;            
        }        
    }

    if ((p2l_mapping[pport_base] == -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] == -1) &&
        (p2l_mapping[pport_base+3] == -1)) {
        /* No lanes enabled on the core, assign default value */
        return SYS_OK;
    }

    return SYS_ERR;
}

static int
soc_fl_flex_xaui_checker(int unit,
                                int core_idx)
{
    const int *p2l_mapping, *speed_max;

    int pport_base = core_prop[core_idx].pport_base;

    p2l_mapping = sku_port_config->p2l_mapping;
    speed_max = sku_port_config->speed_max;

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] != -1) &&
        (p2l_mapping[pport_base+2] != -1) &&
        (p2l_mapping[pport_base+3] != -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_XAUI, 1) == SYS_OK &&
            flex_lane_speed_check
                (speed_max[pport_base + 1], FLEX_XAUI, 1) == SYS_OK  &&
            flex_lane_speed_check
                (speed_max[pport_base + 2], FLEX_XAUI, 1) == SYS_OK  &&
            flex_lane_speed_check
                (speed_max[pport_base + 3], FLEX_XAUI, 1) == SYS_OK) 
        {
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
    }

    if ((p2l_mapping[pport_base] != -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] == -1) &&
        (p2l_mapping[pport_base+3] == -1)) {
        if (flex_lane_speed_check
                (speed_max[pport_base], FLEX_XAUI, 4) == SYS_OK) 
        {
            SOC_PORT_LANE_NUMBER(p2l_mapping[pport_base]) = 4;
            SOC_PORT_MODE(p2l_mapping[pport_base]) = SOC_FL_PORT_MODE_SINGLE;  
            return SYS_OK;            
        }
    }

    if ((p2l_mapping[pport_base] == -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] == -1) &&
        (p2l_mapping[pport_base+3] == -1)) 
    {
        /* No lanes enabled on the core, assign default value */
        return SYS_OK;  
    }

    return SYS_ERR;
}
#endif

static int
soc_fl_flex_off_checker(int unit,
                                int core_idx)
{
    const int *p2l_mapping;

    int pport_base = core_prop[core_idx].pport_base;

    p2l_mapping = sku_port_config->p2l_mapping;

    if ((p2l_mapping[pport_base] == -1) &&
        (p2l_mapping[pport_base+1] == -1) &&
        (p2l_mapping[pport_base+2] == -1) &&
        (p2l_mapping[pport_base+3] == -1)) {
        /* No lanes enabled on the core, assign default value */
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
             break;
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
    const core_option_t *p_core_option = NULL;

    p_core_option = soc_portconf_matcher(unit);


    if (p_core_option == NULL) {
        return SYS_ERR;
    }
    
    fl_auto_tdm_port_config.freq = p_core_option->freq;

    for (i = 0; i < (FL_SERDES_CORE_COUNT); i++) {
        switch (p_core_option->mode[i]) {
            case FLEX_OFF:
                rv = soc_fl_flex_off_checker(unit, i);
                break;
            default:
if (0) { 
                sal_printf("PORTMAP config is invalid "
                                    "on SERDES CORE #%d.\n", i);
}
                return SYS_ERR;
        }
        if (rv != SYS_OK) {
if (0) { 
            sal_printf("Incorrect PORTMAP config on SERDES CORE #%d.\n", i);
}
            return rv;
        }
    }
    return rv;
}

#ifndef CFG_FL_SOC_DEBUG
static int
soc_portmap_parser(int unit)
{
    int lport;
    int pport;
    uint32 port_bw;

    int rv = SYS_ERR;
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
#endif

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
    for (pport = BCM5607X_PORT_MAX; pport > 1; pport--) {
         if (SOC_PORT_P2L_MAPPING(pport) != -1) {
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
#ifdef CFG_PORTMAP_DUMP
typedef struct {
  int speed_max;
  int lane_number;
  const char *port_description;
  int port_num;
} port_type_t;
void soc_portmap_dump(int unit) {

    int pport, lport, speed_max, i, block_start_port;
    port_type_t port[8];

    port[0].speed_max = 10;
    port[0].port_description = "1G";
    port[0].port_num = 0;
    port[1].speed_max = 25;
    port[1].port_description = "2.5G";
    port[1].port_num = 0;
    port[2].speed_max = 100;
    port[2].port_description = "10G";
    port[2].port_num = 0;
    port[3].speed_max = 200;
    port[3].port_description = "20G";
    port[3].port_num = 0;
    port[4].speed_max = 250;
    port[4].port_description = "25G";
    port[4].port_num = 0;
    port[5].speed_max = 400;
    port[5].port_description = "40G";
    port[5].port_num = 0;
    port[6].speed_max = 500;
    port[6].port_description = "50G";
    port[6].port_num = 0;
    port[7].port_num = 0;
    port[7].speed_max = 100;
    port[7].port_description = "XAUI";
    
    
    
    SOC_LPORT_ITER(lport) {
        pport = SOC_PORT_L2P_MAPPING(lport);
        speed_max = sku_port_config->speed_max[pport];
        block_start_port = pport - SOC_PORT_BLOCK_INDEX(lport);
        if ((speed_max == 100) && (block_start_port == pport) && (SOC_PORT_P2L_MAPPING(pport+1) < 0)) {
            sal_printf("lport %d is XAUI\n", lport);
            port[7].port_num++;
        } else {
            for (i=0; i < 7; i++) {
                 if (port[i].speed_max == speed_max) {
                     port[i].port_num++;
                 }
            }
        }
    }
    for (i=0; i < 8; i++) {
         if (port[i].port_num) {
             sal_printf("%dP %s +", port[i].port_num, port[i].port_description);
         }
    }

    sal_printf("\nsku_option=%s\n", sku_port_config->config_op);
    
    SOC_LPORT_ITER(lport) {
        pport = SOC_PORT_L2P_MAPPING(lport);
        speed_max = sku_port_config->speed_max[pport];
        if (speed_max != 25) {
            sal_printf("portmap_%d=%d:%d\n", lport, pport, speed_max / 10);
        } else {
            sal_printf("portmap_%d=%d:2.5\n", lport, pport); 
        }
    }
    sal_printf("\n");

}
#endif /* CFG_PORTMAP_DUMP */

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
#endif
    return SYS_OK;
}

sys_error_t
soc_port_config_init(uint8 unit) {

    int i;
    int lport;
    uint8 core_num, lane_num, bypass_checker = 0;
    const char *config_id = NULL;
    pbmp_t lpbmp, lpbmp_mask;
    char name[32];
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
    soc_info_t *si = &SOC_INFO(0);

    /* Override of sku id and option id */
    if (sal_config_uint16_get(SAL_CONFIG_SKU_DEVID, &fl_sw_info.devid) == SYS_OK) {
        sal_printf("Vendor Config : Overwrite SKU device ID with value 0x%x.\n", fl_sw_info.devid);
    }

    config_id = sal_config_get(SAL_CONFIG_SKU_OPTION);
    if (config_id != NULL) {
        sal_printf("Vendor Config : Overwrite SKU option with value %s.\n", config_id);
    }
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

    for (i=0; i < _FL_ARRAY_SIZE(core_options); i++) {
         if (soc_device_id_matcher(fl_sw_info.devid, &core_options[i]) == SYS_OK) {
             break;
         }
    }

    if (i == _FL_ARRAY_SIZE(core_options)) {
        sal_printf("device id %x is not supported\n", fl_sw_info.devid);
        sal_printf("Each ports will be turn off\n");
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
             sal_printf("Vendor Config : Force to default option 5.\n");
             sal_strcpy(sku_port_config->config_op, OPTION_5);
         }
         if (soc_portmap_parser(unit) != SYS_OK) {
if (0) { 
             sal_printf("Vendor Config : soc_portmap_parser failed.\n");
             sal_printf("Vendor Config : Inapproprate \"portmap\" setting.\n");
             sal_printf("Vendor Config : Please check vendor config and chip part number. \n");
             sal_printf("Vendor Config : Force to default port setting.\n");
             sal_printf("Vendor Config : OPTION_5_8, 10P 10G + 2P 100G\n");
}
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
if (0) { 
         sal_printf("Vendor Config : soc_portmap_checker failed.\n");
         sal_printf("Vendor Config : Inapproprate \"portmap\" setting.\n");
         sal_printf("Vendor Config : Please check vendor config and chip part number. \n");
         sal_printf("Vendor Config : Force to default port setting.\n");
         sal_printf("Vendor Config : OPTION_5_8, 10P 10G + 2P 100G\n");
}
         sku_port_config = &fl_56070_default_port_config;

         soc_portmap_init(unit);
         soc_portmap_checker(unit);
     }

#ifdef CFG_PORTMAP_DUMP
     soc_portmap_dump(unit);
#endif

    for (core_num = 0; core_num < TSCF_NUM_OF_CORES; core_num++) {
         for (lane_num = 0; lane_num < 4; lane_num++) {
              tscf_interface[core_num][lane_num] = sku_port_config->tscf_interface_default; /* per system default setting set by config */
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
              sal_config_uint8_xy_get(SAL_CONFIG_TSCF_INTERFACE, core_num, lane_num, &tscf_interface[core_num][lane_num]);
#endif /*  CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

         }
    }

    for (core_num = 0; core_num < TSCE_NUM_OF_CORES; core_num++) {
         for (lane_num = 0; lane_num < 4; lane_num++) {
              tsce_interface[core_num][lane_num] = sku_port_config->tsce_interface_default;
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
              sal_config_uint8_xy_get(SAL_CONFIG_TSCE_INTERFACE, core_num, lane_num, &tsce_interface[core_num][lane_num]);
#endif /*  CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

        }
    }

    for (core_num = 0; core_num < QTCE_NUM_OF_CORES; core_num++) {
         for (lane_num = 0; lane_num < 4; lane_num++) {
              qtc_interface[core_num][lane_num] = sku_port_config->qtc_interface_default;
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
              sal_config_uint8_xy_get(SAL_CONFIG_QTC_INTERFACE, core_num, lane_num, &qtc_interface[core_num][lane_num]);
#endif /*  CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */
         }
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

    /* To determine the lane number and port mode */
#define WITHIN(x, min, max) (((x) >= (min)) && ((x) <= (max)))
        SOC_LPORT_ITER(lport) {
             if (IS_QTCE_PORT(lport)) {
                  /* Check if 40G-KR4 four lane mode*/
                 if (SOC_PORT_MODE(lport) == SOC_FL_PORT_MODE_SINGLE) /* QSGMII mode */
                 {
                      if (qtc_interface[QTCE_CORE_NUM_GET(lport)][0] != QTC_INTERFACE_QSGMII) {
                          sal_printf("Vendor Config: QTCE%d_%d is forced to QSGMII mode.\n", QTCE_CORE_NUM_GET(lport), 0);
                      }
                      qtc_interface[QTCE_CORE_NUM_GET(lport)][0] = QTC_INTERFACE_QSGMII;
                 }
             } else if (IS_TSCE_PORT(lport)) {
                 /* Check if 40G-KR4 four lane mode*/
                 if ((SOC_PORT_SPEED_INIT(lport) == 40000) && SOC_PORT_MODE(lport) == SOC_FL_PORT_MODE_SINGLE)
                 {
                      if (tsce_interface[TSCE_CORE_NUM_GET(lport)][0] != TSCE_INTERFACE_FIBER) {
                          sal_printf("Vendor Config: TSCE%d_%d is forced to 40G-KR4 FIBER mode.\n", TSCE_CORE_NUM_GET(lport), 0);
                      }
                      tsce_interface[TSCE_CORE_NUM_GET(lport)][0] = TSCE_INTERFACE_FIBER;
                 }

                 /* Check if 10G-XAUI four lane mode*/
                 if ((SOC_PORT_SPEED_INIT(lport) == 10000) && SOC_PORT_MODE(lport) == SOC_FL_PORT_MODE_SINGLE)
                 {
                      if (tsce_interface[TSCE_CORE_NUM_GET(lport)][0] != TSCE_INTERFACE_XAUI) {
                          sal_printf("Vendor Config: TSCE%d_%d is forced to XAUI mode.\n", TSCE_CORE_NUM_GET(lport), 0);
                      }
                      tsce_interface[TSCE_CORE_NUM_GET(lport)][0] = TSCE_INTERFACE_XAUI;
                 }
             } else if (IS_TSCF_PORT(lport)) {
                 /* Check if 40G-KR4 four lane mode*/
                 if ((SOC_PORT_SPEED_INIT(lport) == 40000) && SOC_PORT_MODE(lport) == SOC_FL_PORT_MODE_SINGLE)
                 {
                      if (tscf_interface[TSCF_CORE_NUM_GET(lport)][0] != TSCF_INTERFACE_FIBER) {
                          sal_printf("Vendor Config: TSCF%d_%d is forced to 40G-KR4 FIBER mode.\n", TSCF_CORE_NUM_GET(lport), 0);
                      }
                      tscf_interface[TSCE_CORE_NUM_GET(lport)][0] = TSCF_INTERFACE_FIBER;
                 }
             }
        }

#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED
        PBMP_CLEAR(lpbmp);
        if (sal_config_pbmp_get(SAL_CONFIG_SPEED_1000_PORTS, &lpbmp) == SYS_OK) {
            sal_printf("Vendor Config : Set speed (1G) logical pbmp with value %s.\n", SOC_PBMP_FORMAT(lpbmp));
            PBMP_AND(lpbmp, BCM5607X_ALL_PORTS_MASK);
            PBMP_ITER(lpbmp, lport) {

                if ((SOC_PORT_MODE(lport) == SOC_FL_PORT_MODE_SINGLE))
                {
                    sal_printf("Vendor Config : lport %d can not set speed to 1G because it is a 2/4 lane port or QSGMII port\n", lport); 
                    continue;
                }

                if ((SOC_PORT_LANE_NUMBER(lport) != 1) && !IS_QTCE_PORT(lport)) {
                    sal_printf("Vendor Config : lport %d can not set speed to 1G because it is a 2/4 lane port or QSGMII port\n", lport);
                    continue;
                }
 
                if (SOC_PORT_L2P_MAPPING(lport) >= 2 && (SOC_PORT_SPEED_MAX(lport) >= 1000))
                {
                    SOC_PORT_SPEED_INIT(lport) = 1000;
                } else {
                    sal_printf("Vendor Config: Port %d can't support 1G\n", lport);
                }
            }
        }
        PBMP_CLEAR(lpbmp);
        if (sal_config_pbmp_get(SAL_CONFIG_SPEED_2500_PORTS, &lpbmp) == SYS_OK) {
            sal_printf("Vendor Config : Set speed (2.5G) logical pbmp with value %s.\n", SOC_PBMP_FORMAT(lpbmp));
            PBMP_AND(lpbmp, BCM5607X_ALL_PORTS_MASK);
            PBMP_ITER(lpbmp, lport) {
                if ((SOC_PORT_MODE(lport) == SOC_FL_PORT_MODE_SINGLE))
                {
                    sal_printf("Vendor Config : lport %d can not set speed to 2.5G because it is a 2/4 lane port or QSGMII port\n", lport);
                    PBMP_PORT_REMOVE(lpbmp, lport);
                    continue;
                }

                if ((SOC_PORT_LANE_NUMBER(lport) != 1) && !IS_QTCE_PORT(lport)) {
                    sal_printf("Vendor Config : lport %d can not set speed to 2.5G because it is a 2/4 lane port or QSGMII port\n", lport);
                    PBMP_PORT_REMOVE(lpbmp, lport);
                    continue;
                }

                if (IS_TSCF_PORT(lport)) {
                    sal_printf("Vendor Config : Physical port %d (Falcon) doesn't support 2.5/5G.\n", SOC_PORT_L2P_MAPPING(lport));
                    SOC_PORT_SPEED_INIT(lport) = SOC_PORT_SPEED_MAX(lport);
                    PBMP_PORT_REMOVE(lpbmp, lport);
                } else {
                    if (SOC_PORT_L2P_MAPPING(lport) >= 2 && (SOC_PORT_SPEED_MAX(lport) >= 2500))
                    {
                       SOC_PORT_SPEED_INIT(lport) = 2500;
                    } else {
                       sal_printf("Vendor Config: Port %d can't support 2.5G\n", lport);
                       PBMP_PORT_REMOVE(lpbmp, lport);
                    }
                }
            }
            /* Workarrond of QTC phy driver */
            PBMP_ITER(lpbmp, lport) {
                 if (IS_QTCE_PORT(lport)) {
                     sal_sprintf(name, "port_init_speed_%d", lport);
                     sal_config_set(name, "2500");
                 }
            }
        }
        PBMP_CLEAR(lpbmp);
        if (sal_config_pbmp_get(SAL_CONFIG_SPEED_5000_PORTS, &lpbmp) == SYS_OK) {
            sal_printf("Vendor Config : Set speed (5G) logical pbmp with value %s.\n", SOC_PBMP_FORMAT(lpbmp));
            PBMP_AND(lpbmp, BCM5607X_ALL_PORTS_MASK);
            PBMP_ITER(lpbmp, lport) {
                 if (SOC_PORT_LANE_NUMBER(lport) == 2 ||
                     (SOC_PORT_LANE_NUMBER(lport) == 4))
                 {
                    sal_printf("Vendor Config : lport %d can not set speed to 5G because it is a 2/4 lane port\n", lport);
                    continue;
                 }
                if (IS_TSCF_PORT(lport)) {
                    sal_printf("Vendor Config : Falcon port %d (Falcon) doesn't support 2.5/5G.\n", lport);
                    SOC_PORT_SPEED_INIT(lport) = SOC_PORT_SPEED_MAX(lport);
                } else {
                    if (SOC_PORT_L2P_MAPPING(lport) >= 2 && (SOC_PORT_SPEED_MAX(lport) >= 5000))
                    {
                        SOC_PORT_SPEED_INIT(lport) = 5000;
                    } else {
                        sal_printf("Vendor Config: Port %d can't support 5G\n", lport);
                    }
                }
            }
        }
        PBMP_CLEAR(lpbmp);
        if (sal_config_pbmp_get(SAL_CONFIG_SPEED_10000_PORTS, &lpbmp) == SYS_OK) {
            sal_printf("Vendor Config : Set speed (10G) logical pbmp with value %s.\n", SOC_PBMP_FORMAT(lpbmp));
            PBMP_AND(lpbmp, BCM5607X_ALL_PORTS_MASK);
            PBMP_ITER(lpbmp, lport) {
                if (SOC_PORT_LANE_NUMBER(lport) == 2)
                {
                   sal_printf("Vendor Config : lport %d can not set speed to 10G because it is a 2 lane port\n", lport);
                   continue;
                }
                /* Speed checking */
                if (SOC_PORT_L2P_MAPPING(lport) >= 2 && (SOC_PORT_SPEED_MAX(lport) >= 10000))
                {
                    if (IS_TSCF_PORT(lport) && SOC_PORT_MODE(lport) == SOC_FL_PORT_MODE_SINGLE) {
                        sal_printf("Vendor Config: Port %d(TSCF) doesn't support XAUI\n", lport);
                    } else {
                        SOC_PORT_SPEED_INIT(lport) = 10000;
                    }
                } else {
                    sal_printf("Vendor Config: Port %d can't support 10G\n", lport);
                }
            }
        }
        PBMP_CLEAR(lpbmp);
        if (sal_config_pbmp_get(SAL_CONFIG_SPEED_25000_PORTS, &lpbmp) == SYS_OK) {
            sal_printf("Vendor Config : Set speed (25G) logical pbmp with value %s.\n", SOC_PBMP_FORMAT(lpbmp));
            PBMP_AND(lpbmp, BCM5607X_ALL_PORTS_MASK);
            PBMP_ITER(lpbmp, lport) {
                if ((SOC_PORT_LANE_NUMBER(lport) == 2) ||
                    (SOC_PORT_LANE_NUMBER(lport) == 4))
                {
                   sal_printf("Vendor Config : lport %d can not set speed to 25G because it is a 2/4 lane port\n", lport);
                   continue;
                }

                /* Speed checking */
                if (SOC_PORT_L2P_MAPPING(lport) >= 2 && (SOC_PORT_SPEED_MAX(lport) >= 25000) &&
                    (IS_TSCF_PORT(lport)))
                {
                    SOC_PORT_SPEED_INIT(lport) = 25000;
                } else {
                    sal_printf("Vendor Config: Port %d can't support 25G\n", lport);
                }
            }
        }
        PBMP_CLEAR(lpbmp);
        if (sal_config_pbmp_get(SAL_CONFIG_SPEED_40000_PORTS, &lpbmp) == SYS_OK) {
            sal_printf("Vendor Config : Set speed (40G) logical pbmp with value %s.\n", SOC_PBMP_FORMAT(lpbmp));
            PBMP_AND(lpbmp, BCM5607X_ALL_PORTS_MASK);
            PBMP_ITER(lpbmp, lport) {
                if ((SOC_PORT_LANE_NUMBER(lport) != 4))
                {
                   sal_printf("Vendor Config : lport %d can not set speed to 40G because it is not a 4 lane port\n", lport);
                   continue;
                }
                /* Speed checking */
                if (SOC_PORT_L2P_MAPPING(lport) >= 2 && (SOC_PORT_SPEED_MAX(lport) >= 40000))
                {
                    SOC_PORT_SPEED_INIT(lport) = 40000;
                } else {
                    sal_printf("Vendor Config: Port %d can't support 40G\n", lport);
                }
            }
        }
        PBMP_CLEAR(lpbmp);
        if (sal_config_pbmp_get(SAL_CONFIG_SPEED_50000_PORTS, &lpbmp) == SYS_OK) {
            sal_printf("Vendor Config : Set speed (50G) logical pbmp with value %s.\n", SOC_PBMP_FORMAT(lpbmp));
            PBMP_AND(lpbmp, BCM5607X_ALL_PORTS_MASK);
            PBMP_ITER(lpbmp, lport) {
                if ((SOC_PORT_LANE_NUMBER(lport) != 2))
                {
                   sal_printf("Vendor Config : lport %d can not set speed to 50G because it is not a 2 lane port\n", lport);
                   continue;
                }
                /* Speed checking */
                if (SOC_PORT_L2P_MAPPING(lport) >= 2 && (SOC_PORT_SPEED_MAX(lport) >= 50000) &&
                    (IS_TSCF_PORT(lport)))
                {
                    SOC_PORT_SPEED_INIT(lport) = 50000;
                } else {
                    sal_printf("Vendor Config: Port %d can't support 50G\n", lport);
                }
            }
        }

#endif /*  CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */

    return SYS_OK;
} 
