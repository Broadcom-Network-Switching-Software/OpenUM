/*
 * $Id: bcm5607x.h,v 1.20 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BCM5607X_H_
#define _BCM5607X_H_

#include "auto_generated/bcm5607x_defs.h"
#include <intr.h>

#ifndef __LINUX__
#define READCSR(x)   SYS_REG_READ32(x)
#define WRITECSR(x,v) SYS_REG_WRITE32(x,v)
#endif

/* Firelight */
#define BCM56070_DEVICE_ID      0xb070
#define BCM56070_A0_REV_ID      1
#define BCM56071_DEVICE_ID      0xb071
#define BCM56071_A0_REV_ID      1
#define BCM56072_DEVICE_ID      0xb072
#define BCM56072_A0_REV_ID      1

/* Port Option string */
/* Option string */
#define OPTION_1 "1"
#define OPTION_2 "2"
#define OPTION_3 "3"
#define OPTION_4 "4"
#define OPTION_5 "5"
#define OPTION_6 "6"
#define OPTION_5_8 "5_8"
#define OPTION_NULL "?"

/* System Clock Freq (MHz) */
#define _FL_SYSTEM_FREQ_700          (700) /* 700 MHz */
#define _FL_SYSTEM_FREQ_469          (469) /* 468.75 MHz */
#define _FL_SYSTEM_FREQ_344          (344) /* 343.75 MHz */
#define _FL_SYSTEM_FREQ_219          (219) /* 218.75 MHz */

#define _FL_56070_SYSTEM_FREQ        _FL_SYSTEM_FREQ_700

#define _FL_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* Schannel opcode */
#define SC_OP_RD_MEM_CMD         0x07
#define SC_OP_RD_MEM_ACK         0x08
#define SC_OP_WR_MEM_CMD         0x09
#define SC_OP_RD_REG_CMD         0x0B
#define SC_OP_RD_REG_ACK         0x0C
#define SC_OP_WR_REG_CMD         0x0D
#define SC_OP_L2_INS_CMD         0x0F
#define SC_OP_L2_DEL_CMD         0x11
#define SC_OP_L2_LOOKUP_CMD      0x20
#define SC_OP_TBL_INS_CMD        0x24
#define SC_OP_TBL_DEL_CMD        0x26
#define SC_OP_TBL_LOOKUP_CMD     0x28


/* FP_TCAM index */
#define MDNS_TO_CPU_IDX                (0)
#define SYS_MAC_IDX                    (1 * ENTRIES_PER_SLICE)

#define RATE_IGR_IDX                   (64 /* 64 port entries */ + 64 /* use second half */)

#define QOS_BASE_IDX                   (2 * ENTRIES_PER_SLICE)
/* 802.1p higher than DSCP fp entries index */
#define DOT1P_BASE_IDX                 ((2 * ENTRIES_PER_SLICE) + 64 /* 64 port entries */ + 64 /* use second half */)

/* Loop Detect, per port from LOOP_COUNT_IDX + MIN~ */
#define LOOP_COUNT_IDX                 ( 3 * ENTRIES_PER_SLICE)
#define LOOP_REDIRECT_IDX              ( 3 * ENTRIES_PER_SLICE + 64 /* 64 port entries */ + 64 /* use second half */)
#define LOOP_REDIRECT_CPU_IDX          ( 3 ) /* 128 entries starting from 768 is not enough; need one more entry for CPU */

#define TX_CH                    0
#define RX_CH1                   1


#if defined(CFG_SOC_SEMAPHORE_INCLUDED) &&  !defined(__SIM__)
#define SCHAN_LOCK(unit) \
        do { if (!READCSR(R_CMIC_SEMAPHORE_3_SHADOW))\
                 while (!READCSR(R_CMIC_SEMAPHORE_1)); } while(0)
#define SCHAN_UNLOCK(unit) \
        do { if (!READCSR(R_CMIC_SEMAPHORE_3_SHADOW))\
               WRITECSR(R_CMIC_SEMAPHORE_1, 0); } while(0)
#define MIIM_LOCK(unit) \
        do { while (!READCSR(R_CMIC_SEMAPHORE_2)); } while(0)
#define MIIM_UNLOCK(unit) \
        do { WRITECSR(R_CMIC_SEMAPHORE_2, 0); } while(0)
/* Access serdes registers through s-channel */
#define MIIM_SCHAN_LOCK(unit) \
        do { while (!READCSR(R_CMIC_SEMAPHORE_3)); } while(0)
#define MIIM_SCHAN_UNLOCK(unit) \
        do { WRITECSR(R_CMIC_SEMAPHORE_3, 0); } while(0)
#else
#define SCHAN_LOCK(unit)
#define SCHAN_UNLOCK(unit)
#define MIIM_LOCK(unit)
#define MIIM_UNLOCK(unit)
#define MIIM_SCHAN_LOCK(unit)
#define MIIM_SCHAN_UNLOCK(unit)
#endif /* CFG_SOC_SEMAPHORE_INCLUDED */

#define LED_0_PORT_STATUS_OFFSET(p)   CMIC_LEDUP0_DATA_RAM(0xa0 + (p))
#define LED_1_PORT_STATUS_OFFSET(p)   CMIC_LEDUP1_DATA_RAM(0xa0 + (p))
#define LED1_PHYPORT_OFFSET     (57) //58 maps to led out[1]; 58-1 = 57; xl_0 phy(58-61) :   L(34-37):    LED Phy(28-31)->(1-4)

#define VLAN_DEFAULT                 1
#define SOC_MAX_NUM_MMU_PORTS SOC_MAX_NUM_PORTS
#define SOC_MAX_NUM_BLKS         30
#define SOC_MAX_NUM_DEVICES 1
typedef uint32 soc_chip_e;
typedef void soc_driver_t;
typedef uint32 sal_mutex_t;
#include <soc/bcm5607x_drv.h>

/* EEE functions */
#ifdef CFG_SWITCH_EEE_INCLUDED
extern void bcm5607x_port_eee_enable_get(uint8 unit, uint8 port, uint8 *enable);
extern sys_error_t bcm5607x_port_eee_enable_set(uint8 unit,
                      uint8 port, uint8 enable, uint8 save);
extern sys_error_t bcm5607x_port_eee_tx_wake_time_set(uint8 unit,
                      uint8 port, uint8 type, uint16 wake_time);
extern void bcm5607x_eee_init(void);
#endif /* CFG_SWITCH_EEE_INCLUDED */
/* End of EEE */

/* phy bcm542xx functions */
extern int bcm542xx_phy_cl45_reg_read(uint8 unit, uint8 port,
                      uint8 dev_addr, uint16 reg_addr, uint16 *p_value);
extern int bcm542xx_phy_cl45_reg_write(uint8 unit, uint8 port,
                      uint8 dev_addr, uint16 reg_addr, uint16 p_value);
extern int bcm542xx_phy_cl45_reg_modify(uint8 unit, uint8 port,
                      uint8 dev_addr, uint16 reg_addr, uint16 val, uint16 mask);
extern int bcm542xx_phy_direct_reg_write(uint8 unit, uint8 port,
                      uint16 reg_addr, uint16 data);
extern int bcm542xx_phy_reg_modify(uint8 unit, uint8 port, uint16 reg_bank,
                            uint8 reg_addr, uint16 data, uint16 mask);
/* End of phy bcm542xx functions */

typedef struct l2x_entry_s {
    uint16 vlan_id;
    uint8  mac_addr[6];
    /* Port or multicast index */
    uint8  port;
    uint8  is_static;
} l2x_entry_t;

/* physical port */
#define BCM5607X_PORT_MIN                        2
#define BCM5607X_PORT_MAX                        77

/* logical port */
#define BCM5607X_LPORT_MIN                        2
#define BCM5607X_LPORT_MAX                       65

/* cpu port */
#define BCM5607X_PORT_CMIC                      0

/* mmu port */
#define SOC_MAX_MMU_PORTS        BCM5607X_LPORT_MAX
typedef enum port_block_type_s {
    PORT_BLOCK_TYPE_XLPORT,
    PORT_BLOCK_TYPE_GXPORT,
    PORT_BLOCK_TYPE_CLPORT,
    PORT_BLOCK_TYPE_COUNT
} port_block_type_t;

enum soc_fl_port_mode_e {
    /* WARNING: values given match hardware register; do not modify */
    SOC_FL_PORT_MODE_QUAD = 0,
    SOC_FL_PORT_MODE_TRI_012 = 1,
    SOC_FL_PORT_MODE_TRI_023 = 2,
    SOC_FL_PORT_MODE_DUAL = 3,
    SOC_FL_PORT_MODE_SINGLE = 4,
    SOC_FL_PORT_MODE_TDM_DISABLE = 5
};

#define _FL_MAX_TSC_COUNT 7
#define _FL_MAX_QTC_COUNT 3
#define FL_SERDES_CORE_COUNT (_FL_MAX_TSC_COUNT + _FL_MAX_QTC_COUNT)
typedef enum qtc_interface_s {
    QTC_INTERFACE_QSGMII = 1,
    QTC_INTERFACE_SGMII = 2,
    QTC_INTERFACE_FIBER = 3,
} qtc_interface_t;

typedef enum tsce_interface_s {
    TSCE_INTERFACE_SGMII = 1,
    TSCE_INTERFACE_XFI = TSCE_INTERFACE_SGMII,
    TSCE_INTERFACE_FIBER = 2,
    TSCE_INTERFACE_XAUI = 3
} tsce_interface_t;

typedef enum tscf_interface_s {
    TSCF_INTERFACE_SGMII = 1,
    TSCF_INTERFACE_XFI = TSCF_INTERFACE_SGMII,
    TSCF_INTERFACE_FIBER = 2,
} tscf_interface_t;

typedef enum sgmiipx4_interface_s {
    SGMIIPX4_INTERFACE_SGMII = 1,
    SGMIIPX4_INTERFACE_FIBER = 2,
} sgmiipx4_interface_t;

/* LEDUP number and control*/
#define BCM5607X_LEDUP_NUMBER                    2

typedef enum {
   LED_MODE_LINK = 0,
   LED_MODE_TXRX,
   LED_MODE_BLINK,
   LED_MODE_FORCE_ON
} ledup_mode_t;

typedef struct {
   /*

         Physical ports list on the scan chain for each LED processor based on board design
         For example:
          LED0:   { 2, 3, 4, 5, 10, 11, 12, 13, 35, 34, 33, 32 }
      */
   uint8 *pport_seq[BCM5607X_LEDUP_NUMBER];

   /* Total port count for each LED scan chain */
   uint8 port_count[BCM5607X_LEDUP_NUMBER];
   /*
         Fixed total bits for each LED scan chain,
         If this field is zero, system will fill this field as "port_count * leds_per_port * bits_per_led"
         For some special board design, you need to send more dump bits out.
     */
   uint8 fix_bits_total[BCM5607X_LEDUP_NUMBER];
   /* Number of leds per port: maximun LED number per port = 3 */
   uint8 leds_per_port;
   /* Number of bits for each single LED*/
   uint8 bits_per_led;
   /* Number of color user could specify */
   uint8 num_of_color;
   /*
         Scan chain bits for controling color for each LED,
         The number of available color is specifiedd by num_of_color
         The number of available bits for each color is specified by bits_per_led
      */
   uint8 bits_color[4];
   /* Bits to turn off LED */
   uint8 bits_color_off;
   /* For 1~3 LEDs, each mode is two bits wide,
         in case of third LED is active, the SW link is not supoorted */
   ledup_mode_t led_mode[3];
   /* ledup blink period, uint=30ms */
   uint8 led_blink_period;
   /*
        For the extenstion of the LED-on time when LED indicate tx_rx activity
        uint=30ms , 4 bits wide
     */
   uint8 led_tx_rx_extension:4;

} ledup_ctrl_t;


typedef struct firelight_sku_info_s {
    char                 *config_op; /* option string */
    int                  freq;
    const int            *p2l_mapping;
    const int            *speed_max;
    qtc_interface_t      qtc_interface_default;
    tsce_interface_t     tsce_interface_default;
    tscf_interface_t     tscf_interface_default;
} firelight_sku_info_t;

extern const firelight_sku_info_t *sku_port_config;
/* SOC interface */
extern sys_error_t bcm5607x_chip_revision(uint8 unit, uint16 *dev, uint16 *rev);
extern sys_error_t soc_port_config_init(uint8 unit);
extern uint8 bcm5607x_port_count(uint8 unit);

/* For switching schan channel in interrupt context only. */
#define BCM5607X_SCHAN_CH_DEFAULT                       0
#define BCM5607X_SCHAN_CH_INTR                          1
extern uint32 bcm5607x_schan_ch;

extern sys_error_t bcm5607x_reg_get(uint8 unit, uint8 block_id, uint32 addr, uint32 *val);
extern sys_error_t bcm5607x_reg_set(uint8 unit, uint8 block_id, uint32 addr, uint32 val);
extern sys_error_t bcm5607x_reg64_get(uint8 unit, uint8 block_id, uint32 addr, uint32 *val, int len);
extern sys_error_t bcm5607x_reg64_set(uint8 unit, uint8 block_id, uint32 addr, uint32 *buf, int len);
extern sys_error_t bcm5607x_mem_get(uint8 unit, uint8 block_id, uint32 addr, uint32 *buf, int len);
extern sys_error_t bcm5607x_mem_set(uint8 unit, uint8 block_id, uint32 addr, uint32 *buf, int len);
extern sys_error_t bcm5607x_l2_op(uint8 unit, l2x_entry_t *entry, uint8 op, uint16 *index);
extern sys_error_t bcm5607x_rx_set_handler(uint8 unit, SOC_RX_HANDLER fn,
                                           BOOL intr);
extern sys_error_t bcm5607x_rx_fill_buffer(uint8 unit,
                                           soc_rx_packet_t *pkt);
extern sys_error_t bcm5607x_tx(uint8 unit, soc_tx_packet_t *pkt);
extern sys_error_t bcm5607x_link_status(uint8 unit, uint8 port, BOOL *link);
extern void bcm5607x_rxtx_stop(void);
extern void bcm5607x_ep_to_cpu_hdr_dump(uint8 unit, char *prefix, void *addr);

/* FP_TCAM encode/decode utility */
extern void bcm5607x_dm_to_xy(uint32 *dm_entry, uint32 *xy_entry, int data_words, int bit_length);
extern void bcm5607x_xy_to_dm(uint32 *xy_entry, uint32 *dm_entry, int data_words, int bit_length);

#ifdef CFG_SWITCH_VLAN_INCLUDED
extern sys_error_t bcm5607x_pvlan_egress_set(uint8 unit,
                    uint8 pport,
                    pbmp_t pbmp);
extern sys_error_t bcm5607x_pvlan_egress_get(uint8 unit,
                    uint8 pport,
                    pbmp_t *pbmp);
extern sys_error_t bcm5607x_qvlan_port_set(uint8 unit,
                    uint16 vlan_id,
                    pbmp_t pbmp,
                    pbmp_t tag_pbmp);
extern sys_error_t bcm5607x_qvlan_port_get(uint8 unit,
                    uint16 vlan_id,
                    pbmp_t *pbmp,
                    pbmp_t *tag_pbmp);
extern sys_error_t bcm5607x_vlan_create(uint8 unit,
                    vlan_type_t type, uint16 vlan_id);
extern sys_error_t bcm5607x_vlan_destroy(uint8 unit, uint16 vlan_id);
extern sys_error_t bcm5607x_vlan_type_set(uint8 unit, vlan_type_t type);
extern sys_error_t bcm5607x_vlan_reset(uint8 unit);
#endif /* CFG_SWITCH_VLAN_INCLUDED */

#ifdef CFG_SWITCH_LAG_INCLUDED
extern sys_error_t bcm5607x_lag_group_set(uint8 unit, uint8 lagid, pbmp_t pbmp);
extern void bcm5607x_lag_group_get(uint8 unit, uint8 lagid, pbmp_t *pbmp);
#endif /* CFG_SWITCH_LAG_INCLUDED */

extern void bcm5607x_loop_detect_enable(BOOL enable);
extern uint8 bcm5607x_loop_detect_status_get(void);

#ifdef CFG_ZEROCONF_MDNS_INCLUDED
sys_error_t bcm5607x_mdns_enable_set(uint8 unit, BOOL mdns_enable);
sys_error_t bcm5607x_mdns_enable_get(uint8 unit, BOOL *mdns_enable);
#endif /* CFG_ZEROCONF_MDNS_INCLUDED */

#ifdef CFG_LED_MICROCODE_INCLUDED
extern void bcm5607x_ledup_init(const ledup_ctrl_t *borad_ledup_ctrl);
extern void bcm5607x_load_ledup_program(uint8 unit);
extern void bcm5607x_ledup_mode_set(uint8 unit, int lport, int led_no, int mode);
extern void bcm5607x_ledup_sw_linkup(uint8 unit, int lport, int linkup);
extern void bcm5607x_ledup_color_set(uint8 unit, int lport, int color);

#endif

extern void bcm5607x_linkscan_task(void *param);
extern void bcm5607x_handle_link_down(uint8 unit, uint8 lport, int changed);
extern void bcm5607x_handle_link_up(uint8 unit, uint8 lport, int changed, uint32* flags);
extern sys_error_t bcm5607x_linkscan_init(int timer_period);

extern sys_error_t bcm5607x_phy_reg_get(uint8 unit, uint8 port, uint16 reg_addr, uint16 *p_value);
extern sys_error_t bcm5607x_phy_reg_set(uint8 unit, uint8 port, uint16 reg_addr, uint16 value);

/* Interrupt system */
extern sys_error_t bcm5607x_intr_init(int unit);

/* Initialization */
extern int bcm5607x_pcm_software_init(int unit);
extern sys_error_t bcm5607x_sw_init(void);
extern void bcm5607x_dscp_map_enable(BOOL enable);
extern void bcm5607x_qos_init(void);
extern void bcm5607x_rate_init(void);
extern void bcm5607x_loop_detect_init(void);
extern void bcm5607x_rxtx_init(void);
extern void bcm5607x_loopback_enable(uint8 unit, uint8 port, int loopback_mode);
extern sys_error_t bcm5607x_mdns_enable_set(uint8 unit, BOOL enable);
extern sys_error_t bcm5607x_mdns_enable_get(uint8 unit, BOOL *enable);

extern mac_driver_t soc_mac_xl, soc_mac_uni, soc_mac_cl;

#define FLEX_HG42_GEN2          0 /* F.HG[42] */
#define FLEX_HG42_UL_GEN3       1 /* F.HG[42]-GEN3 */
#define FLEX_2HG53_GEN3         2 /* F.2HG53-GEN3 */
#define FLEX_CAUI_GEN3          3 /* F.CAUI */
#define FLEX_XLAUI_QXG          4 /* F.XLAUI or F.QXG */
#define FLEX_XLAUI_QSG          5 /* F.XLAUI or F.QSG */
#define FLEX_MGL_GEN2           6 /* F.MGL-GEN2 */
#define FLEX_MGL_GEN3           7 /* F.MGL-GEN3 */
#define FLEX_QXGMII             8 /* F.QXG */
#define FLEX_QSGMII             9 /* F.QSG */
#define FLEX_OFF             (-1) /* core is off */

typedef struct {
    mac_driver_t *p_mac;      /* mac driver */
} pcm_port_info_t;



typedef struct
{
    uint16  devid;
    uint16  revid;
    uint8   link[BCM5607X_LPORT_MAX + 1];           /* link status */
    uint8   loopback[BCM5607X_LPORT_MAX + 1];       /* loopback mode */
    uint8   port_count;
#if defined(CFG_SWITCH_EEE_INCLUDED)
    tick_t link_up_time[BCM5607X_LPORT_MAX+1];
    uint8 need_process_for_eee_1s[BCM5607X_LPORT_MAX+1];
#endif /*  CFG_SWITCH_EEE_INCLUDED */
    int    port_block_id[BCM5607X_LPORT_MAX + 1];           /* logical port */
    int    port_block_port_id[BCM5607X_LPORT_MAX + 1];      /* logical port */
    int    port_block_type[BCM5607X_LPORT_MAX + 1];         /* logical port */
    int    port_speed_satus[BCM5607X_LPORT_MAX + 1];        /* logical port */
    BOOL   port_link_status[BCM5607X_LPORT_MAX + 1];       /* logical port */
    BOOL   port_duplex_status[BCM5607X_LPORT_MAX + 1];     /* logical port */
    BOOL   port_an_status[BCM5607X_LPORT_MAX + 1];         /* logical port */
    BOOL   port_tx_pause_status[BCM5607X_LPORT_MAX + 1];   /* logical port */
    BOOL   port_rx_pause_status[BCM5607X_LPORT_MAX + 1];   /* logical port */
    int    port_mode[BCM5607X_LPORT_MAX + 1];   /* logical port */
    int    pmq_block_port_id[BCM5607X_PORT_MAX + 1];
    int    port_p2m_mapping[BCM5607X_PORT_MAX + 1];   /* logical port */
    int    port_m2p_mapping[BCM5607X_PORT_MAX + 1];   /* logical port */
    pcm_port_info_t port_info[BCM5607X_LPORT_MAX + 1];
    int    l2_entry_size;
    int    l2_mc_size;
    int    cbp_buffer_size;
    uint32 disable_serdes_core;
    char   pbmp_format[32];
    soc_control_t soc_um_control;

    uint32 cpu_clock;
} bcm5607x_sw_info_t;

extern bcm5607x_sw_info_t fl_sw_info;

#define COS_QUEUE_NUM                   (4)
#define MAXBUCKETCONFIG_NUM             (8)
#define COSLCCOUNT_NUM                  (64)
#define COSLCCOUNT_QGROUP_NUM           (8)

/* MMU port index 58~65 in FL is with 64 COSQ */
#define SOC_FL_64Q_MMU_PORT_IDX_MIN 58
#define SOC_FL_64Q_MMU_PORT_IDX_MAX 65

/* Per port COSQ and QGROUP number */
#define SOC_FL_QGROUP_PER_PORT_NUM 8
#define SOC_FL_QLAYER_COSQ_PER_QGROUP_NUM 8
#define SOC_FL_QLAYER_COSQ_PER_PORT_NUM (SOC_FL_QGROUP_PER_PORT_NUM * \
                                          SOC_FL_QLAYER_COSQ_PER_QGROUP_NUM)

#define SOC_FL_LEGACY_QUEUE_NUM 8
#define SOC_FL_2LEVEL_QUEUE_NUM SOC_FL_QLAYER_COSQ_PER_PORT_NUM


/* Mask of all logical ports */
#define BCM5607X_ALL_PORTS_MASK        (fl_sw_info.soc_um_control.info.all.bitmap)
#define BCM5607X_ALL_PORTS             (fl_sw_info.soc_um_control.info.all.bitmap)
/*
 * Macros to get the device block type, block index and index
 * within block for a given port
 */
#define _FL_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define SOC_PORT_SPEED_INIT(port)        (fl_sw_info.soc_um_control.info.port_init_speed[(port)])
#define SOC_PORT_SPEED_MAX(port)         (fl_sw_info.soc_um_control.info.port_speed_max[(port)])
#define SOC_PORT_LANE_NUMBER(port)       (fl_sw_info.soc_um_control.info.port_num_lanes[(port)])
#define SOC_PORT_SPEED_STATUS(port)      (fl_sw_info.port_speed_satus[(port)])
#define SOC_PORT_LINK_STATUS(port)       (fl_sw_info.port_link_status[(port)])
#define SOC_PORT_DUPLEX_STATUS(port)     (fl_sw_info.port_duplex_status[(port)])
#define SOC_PORT_AN_STATUS(port)         (fl_sw_info.port_an_status[(port)])
#define SOC_PORT_TX_PAUSE_STATUS(port)   (fl_sw_info.port_tx_pause_status[(port)])
#define SOC_PORT_RX_PAUSE_STATUS(port)   (fl_sw_info.port_rx_pause_status[(port)])
#define SOC_PORT_P2L_MAPPING(port)       (fl_sw_info.soc_um_control.info.port_p2l_mapping[(port)])
#define SOC_PORT_L2P_MAPPING(port)       (fl_sw_info.soc_um_control.info.port_l2p_mapping[(port)])
#define SOC_PORT_P2M_MAPPING(port)       (fl_sw_info.soc_um_control.info.port_p2m_mapping[(port)])
#define SOC_PORT_M2P_MAPPING(port)       (fl_sw_info.soc_um_control.info.port_m2p_mapping[(port)])
#define SOC_PORT_COUNT(unit)             (fl_sw_info.port_count)
#define PORT(unit, lport)                (fl_sw_info.port_info[lport])
#define SOC_INFO(unit)                   (fl_sw_info.soc_um_control.info)
#define SOC_CONTROL(unit)                (fl_sw_info.soc_um_control)

#define SOC_PMQ_BLOCK_INDEX(port)        (fl_sw_info.pmq_block_port_id[(port)])
#define SOC_PORT_BLOCK_TYPE(port)        (fl_sw_info.port_block_type[(port)])
#define SOC_PORT_BLOCK(port)             (fl_sw_info.port_block_id[(port)])
#define SOC_PORT_BLOCK_INDEX(port)       (fl_sw_info.port_block_port_id[(port)])

#define SOC_PORT_MODE(port)              (fl_sw_info.port_mode[(port)])
#define SOC_PBMP_FORMAT(pbmp)            (_shr_pbmp_format(pbmp, fl_sw_info.pbmp_format))

#define IS_XL_PORT(port) \
        (SOC_PORT_BLOCK_TYPE(port) == PORT_BLOCK_TYPE_XLPORT)

#define IS_GX_PORT(port) \
        (SOC_PORT_BLOCK_TYPE(port) == PORT_BLOCK_TYPE_GXPORT)

#define IS_CL_PORT(port) \
        (SOC_PORT_BLOCK_TYPE(port) == PORT_BLOCK_TYPE_CLPORT)

/* Do not support HG port */
#define IS_HG_PORT(port) (0)

/* Do not support ST port */
#define IS_ST_PORT(port) (0)
#define TSCE_NUM_OF_CORES        7
#define TSCF_NUM_OF_CORES        1
#define QTCE_NUM_OF_CORES        2
#define SGMIIP4_NUM_OF_CORES     6

#define PORTS_PER_TSC                4

/* Do not support HG Lite port */
#define IS_HL_PORT(port) (0)

#define IS_CPU_PORT(port)   (port == BCM5607X_PORT_CMIC)

#define IS_XE_PORT(port)   (PBMP_MEMBER(fl_sw_info.soc_um_control.info.xe.bitmap, port))
#define IS_GE_PORT(port)   (PBMP_MEMBER(fl_sw_info.soc_um_control.info.ge.bitmap, port))
#define IS_CE_PORT(port)   (PBMP_MEMBER(fl_sw_info.soc_um_control.info.ce.bitmap, port))

#define IS_TSCF_PORT(lport)     ((SOC_PORT_L2P_MAPPING(lport) >= PHY_CLPORT0_BASE) && \
                                 (SOC_PORT_L2P_MAPPING(lport) <= (BCM5607X_PORT_MAX)))
#define TSCF_CORE_NUM_GET(lport) ((SOC_PORT_L2P_MAPPING(lport) - PHY_CLPORT0_BASE) / 4)

#define IS_TSCE_PORT(lport)     ((SOC_PORT_L2P_MAPPING(lport) >= PHY_XLPORT0_BASE) && \
                                 (SOC_PORT_L2P_MAPPING(lport) <  PHY_CLPORT0_BASE))
#define TSCE_CORE_NUM_GET(lport) ((SOC_PORT_L2P_MAPPING(lport) - PHY_XLPORT0_BASE) / 4)

#define IS_QTCE_PORT(lport)     ((SOC_PORT_L2P_MAPPING(lport) >= PHY_GPORT0_BASE) && \
                                 (SOC_PORT_L2P_MAPPING(lport) <  PHY_XLPORT0_BASE))
#define QTCE_CORE_NUM_GET(lport) ((SOC_PORT_L2P_MAPPING(lport) - PHY_GPORT0_BASE) / 16)

#define INIT_PORT(ptype) \
        si->ptype.min = si->ptype.max = -1;      \
        si->ptype.num = -1;                        \
        PBMP_CLEAR(si->ptype.bitmap)

#define ADD_PORT(ptype, port_num) \
        si->ptype.num++; \
        if (si->ptype.min > port_num || si->ptype.min < 0) { \
                si->ptype.min = port_num; \
        } \
        if (si->ptype.max < port_num) {     \
            si->ptype.max = port_num; \
        } \
        PBMP_PORT_ADD(si->ptype.bitmap, port_num);


/* Physical port iteration */
#define SOC_PPORT_ITER(_p)       \
        for ((_p) = BCM5607X_PORT_MIN; \
             (_p) <= BCM5607X_PORT_MAX; \
             (_p)++) \
                if (SOC_PORT_P2L_MAPPING(_p) != -1)

/* Logical port iteration */
#define SOC_LPORT_ITER(_p)       \
        for ((_p) = BCM5607X_LPORT_MIN; \
             (_p) <= BCM5607X_LPORT_MAX; \
             (_p)++) \
                if ((SOC_PORT_L2P_MAPPING(_p) != -1))

#define SOC_PBMP(pbmp)          ((uint32 *) &(pbmp))

typedef struct _fl_sku_option_list_s {
    uint16      dev_id;
    char        *option_string; /* option */
    int         default_option; /* 1 : the default option of the SKU */
} _fl_sku_option_list_t;


typedef struct core_option_s {
    char        *config_op; /* option string */
    int         freq;
    uint32      devid[4];
    int         mode[FL_SERDES_CORE_COUNT];
} core_option_t;

typedef struct core_prop_s {
   int pport_base;
   int len_max;
} core_prop_t;

typedef struct core_mode_s {
   uint32 mode;
   int lane_number;
} core_mode_t;

#if CFG_POLLED_INTR && defined(CFG_INTR_INCLUDED)
#define POLLED_IRQ()   sys_intr_entry()
#else
#define POLLED_IRQ()
#endif
#ifdef CFG_SWITCH_SYNCE_INCLUDED
/* SyncE Frequency */
typedef enum bcm_time_synce_frequency_e {
    bcmTimeSyncE23MHz = 1,  /* TSCF - 23.4375 MHz */
    bcmTimeSyncE25MHz = 2,  /* SDM - 25MHz */
    bcmTimeSyncE28MHz = 3,  /* General - 28.4 MHz */
    bcmTimeSyncE46MHz = 4,  /* General - 46.875 MHz */
    bcmTimeSyncE125MHz = 5, /* General - 125MHz */
    bcmTimeSyncE515MHz = 6, /* General - 515.625MHz */
    bcmTimeSyncE7MHz = 7,   /* SDM - 7.8125 MHz */
    bcmTimeSyncE12MHz = 8,  /* SDM - 12.5 MHz */
    bcmTimeSyncE31MHz = 9,  /* SDM - 31.25 MHz */
    bcmTimeSyncE37MHz = 10, /* SDM - 37.5 MHz */
    bcmTimeSyncE156MHz = 11 /* General - 156.25 MHz */
} bcm_time_synce_frequency_t;

extern sys_error_t bcm5607x_time_synce_clock_source_control_set(uint8 unit,
                   bcm_time_synce_clock_source_config_t *clk_src_config,
                   bcm_time_synce_clock_source_control_t control,
                   int value);
extern sys_error_t bcm5607x_time_synce_clock_source_control_get(uint8 unit,
                   bcm_time_synce_clock_source_config_t *clk_src_config,
                   bcm_time_synce_clock_source_control_t control,
                   int *value);
#endif /* CFG_SWITCH_SYNCE_INCLUDED */

#endif /* _BCM5607X_H_ */
