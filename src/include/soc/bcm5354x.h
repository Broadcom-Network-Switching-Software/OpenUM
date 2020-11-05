/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BCM5354X_H_
#define _BCM5354X_H_

#include "auto_generated/bcm5354x_defs.h"


#ifndef __LINUX__
#define READCSR(x)    \
  (*(volatile uint32 *)(x))
#define WRITECSR(x,v) \
  (*(volatile uint32 *)(x) = (uint32)(v))
#endif


#if CONFIG_EMULATION
#define BCM53540_DEVICE_ID      0xb179
#else
#define BCM53540_DEVICE_ID      0x8540
#endif
#define BCM53540_A0_REV_ID      1
#define BCM53547_DEVICE_ID      0x8547
#define BCM53547_A0_REV_ID      1
#define BCM53548_DEVICE_ID      0x8548
#define BCM53548_A0_REV_ID      1
#define BCM53549_DEVICE_ID      0x8549
#define BCM53549_A0_REV_ID      1


/* Option string */
#define OPTION_1 "op1"
#define OPTION_2 "op2"
#define OPTION_3 "op3"
#define OPTION_4 "op4"
#define OPTION_5 "op5"
#define OPTION_6 "op6"
#define OPTION_7 "op7"
#define OPTION_8 "op8"
#define OPTION_9 "op9"
#define OPTION_10 "op10"
#define OPTION_11 "op11"
#define OPTION_12 "op12"
#define OPTION_13 "op13"
#define OPTION_14 "op14"
#define OPTION_15 "op15"
#define OPTION_16 "op16"
#define OPTION_17 "op17"
#define OPTION_18 "op18"

#define SC_OP_RD_MEM_CMD         0x07
#define SC_OP_RD_MEM_ACK         0x08
#define SC_OP_WR_MEM_CMD         0x09
#define SC_OP_RD_REG_CMD         0x0B
#define SC_OP_RD_REG_ACK         0x0C
#define SC_OP_WR_REG_CMD         0x0D
#define SC_OP_L2_INS_CMD         0x0F
#define SC_OP_L2_DEL_CMD         0x11
#define SC_OP_TBL_INS_CMD        0x24
#define SC_OP_TBL_DEL_CMD        0x26
                              

/* FP_TCAM index */
#define MDNS_TO_CPU_IDX                (0)
#define SYS_MAC_IDX                    (1 * ENTRIES_PER_SLICE)
#define RATE_IGR_IDX                   ((1 * ENTRIES_PER_SLICE) + 3)
#define QOS_BASE_IDX                   (2 * ENTRIES_PER_SLICE)
/* 802.1p higher than DSCP fp entries index */
#define DOT1P_BASE_IDX                 ((2 * ENTRIES_PER_SLICE) + 48)

/* MDNS Redirection table index */
#define MDNS_REDIR_IDX                 (2)
/* Loop Detect, per port from LOOP_COUNT_IDX + MIN~ */
#define LOOP_COUNT_IDX                 ( 3 * ENTRIES_PER_SLICE)
#define LOOP_REDIRECT_IDX              ( 3 * ENTRIES_PER_SLICE + 35)


#define TX_CH                    0
#define RX_CH1                   1

#if defined(CFG_SOC_SEMAPHORE_INCLUDED) &&  !defined(__SIM__)
#define SCHAN_LOCK(unit) \
        do { if (!READCSR(CMIC_SEMAPHORE_3_SHADOW))\
                 while (!READCSR(CMIC_SEMAPHORE_1)); } while(0)
#define SCHAN_UNLOCK(unit) \
        do { if (!READCSR(CMIC_SEMAPHORE_3_SHADOW))\
               WRITECSR(CMIC_SEMAPHORE_1, 0); } while(0)
#define MIIM_LOCK(unit) \
        do { while (!READCSR(CMIC_SEMAPHORE_2)); } while(0)
#define MIIM_UNLOCK(unit) \
        do { WRITECSR(CMIC_SEMAPHORE_2, 0); } while(0)
/* Access serdes registers through s-channel */
#define MIIM_SCHAN_LOCK(unit) \
        do { while (!READCSR(CMIC_SEMAPHORE_3)); } while(0)
#define MIIM_SCHAN_UNLOCK(unit) \
        do { WRITECSR(CMIC_SEMAPHORE_3, 0); } while(0)
#else
#define SCHAN_LOCK(unit)
#define SCHAN_UNLOCK(unit)
#define MIIM_LOCK(unit)
#define MIIM_UNLOCK(unit)
#define MIIM_SCHAN_LOCK(unit)
#define MIIM_SCHAN_UNLOCK(unit)
#endif /* CFG_SOC_SEMAPHORE_INCLUDED */

#define LED_PORT_STATUS_OFFSET(p)   CMIC_LEDUP0_DATA_RAM(0xa0 + (p))

#define VLAN_DEFAULT                 1
#define SOC_MAX_NUM_MMU_PORTS SOC_MAX_NUM_PORTS
#define SOC_MAX_NUM_BLKS         20
#define SOC_MAX_NUM_DEVICES 1
typedef uint32 soc_chip_e;
typedef void soc_driver_t;
#include <soc/bcm5354x_drv.h>

/* EEE functions */
#ifdef CFG_SWITCH_EEE_INCLUDED
extern void bcm5354x_port_eee_enable_get(uint8 unit, uint8 port, uint8 *enable);
extern sys_error_t bcm5354x_port_eee_enable_set(uint8 unit, 
                      uint8 port, uint8 enable, uint8 save);
extern sys_error_t bcm5354x_port_eee_tx_wake_time_set(uint8 unit, 
                      uint8 port, uint8 type, uint16 wake_time);
extern void bcm5354x_eee_init(void);
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
} l2x_entry_t;


/* physical port */
#define BCM5354X_PORT_MIN                        2
#define BCM5354X_PORT_MAX                        37

/* logical port */
#define BCM5354X_LPORT_MIN                        2
#define BCM5354X_LPORT_MAX                       29

/* user port */
#define BCM5354X_UPORT_MIN                        1
#define BCM5354X_UPORT_MAX                       SOC_PORT_COUNT(unit)

/* cpu port */
#define BCM5354X_PORT_CMIC                      0

typedef enum port_block_type_s {
    PORT_BLOCK_TYPE_XLPORT,
    PORT_BLOCK_TYPE_GXPORT,
    PORT_BLOCK_TYPE_COUNT
} port_block_type_t;

enum soc_wh2_port_mode_e {
    /* WARNING: values given match hardware register; do not modify */
    SOC_WH2_PORT_MODE_QUAD = 0,
    SOC_WH2_PORT_MODE_TRI_012 = 1,
    SOC_WH2_PORT_MODE_TRI_023 = 2,
    SOC_WH2_PORT_MODE_DUAL = 3,
    SOC_WH2_PORT_MODE_SINGLE = 4
};

typedef enum sgmiipx4_interface_s {
    SGMIIPX4_INTERFACE_SGMII = 1,
    SGMIIPX4_INTERFACE_FIBER = 2,
} sgmiipx4_interface_t;

typedef struct wolfhound2_sku_info_s {
    uint16      dev_id;
    int         config_op; /* sku option */
    int         freq;
    char        *op_str;
    const int   *p2l_mapping;
    const int   *speed_max;
    const int   *dport_mapping;
    const uint32      *tdm_table;
    int         tdm_table_size;
    uint32      qgphy_core_map;
    uint8       qgphy5_lane;
    uint8       sgmii_4p0_lane;
    sgmiipx4_interface_t sgmiipx4_interface_default;
} wolfhound2_sku_info_t;

extern wolfhound2_sku_info_t *sku_port_config;

/* SOC interface */
extern sys_error_t bcm5354x_chip_revision(uint8 unit, uint16 *dev, uint16 *rev);
extern sys_error_t soc_port_config_init(uint8 unit);
extern uint8 bcm5354x_port_count(uint8 unit);
extern sys_error_t bcm5354x_reg_get(uint8 unit, uint8 block_id, uint32 addr, uint32 *val);
extern sys_error_t bcm5354x_reg_set(uint8 unit, uint8 block_id, uint32 addr, uint32 val);
extern sys_error_t bcm5354x_reg64_get(uint8 unit, uint8 block_id, uint32 addr, uint32 *val, int len);
extern sys_error_t bcm5354x_reg64_set(uint8 unit, uint8 block_id, uint32 addr, uint32 *buf, int len);
extern sys_error_t bcm5354x_mem_get(uint8 unit, uint8 block_id, uint32 addr, uint32 *buf, int len);
extern sys_error_t bcm5354x_mem_set(uint8 unit, uint8 block_id, uint32 addr, uint32 *buf, int len);
extern sys_error_t bcm5354x_l2_op(uint8 unit, l2x_entry_t *entry, uint8 op);
extern sys_error_t bcm5354x_rx_set_handler(uint8 unit, SOC_RX_HANDLER fn, 
                                           BOOL intr);
extern sys_error_t bcm5354x_rx_fill_buffer(uint8 unit, 
                                           soc_rx_packet_t *pkt);
extern sys_error_t bcm5354x_tx(uint8 unit, soc_tx_packet_t *pkt);
extern sys_error_t bcm5354x_link_status(uint8 unit, uint8 port, BOOL *link);
extern void bcm5354x_rxtx_stop(void);

/* FP_TCAM encode/decode utility */
extern void bcm5354x_dm_to_xy(uint32 *dm_entry, uint32 *xy_entry, int data_words, int bit_length);
extern void bcm5354x_xy_to_dm(uint32 *xy_entry, uint32 *dm_entry, int data_words, int bit_length);

#ifdef CFG_SWITCH_VLAN_INCLUDED
extern sys_error_t bcm5354x_pvlan_egress_set(uint8 unit, 
                    uint8 pport, 
                    pbmp_t pbmp);
extern sys_error_t bcm5354x_pvlan_egress_get(uint8 unit, 
                    uint8 pport, 
                    pbmp_t *pbmp);
extern sys_error_t bcm5354x_qvlan_port_set(uint8 unit, 
                    uint16 vlan_id, 
                    pbmp_t pbmp, 
                    pbmp_t tag_pbmp);
extern sys_error_t bcm5354x_qvlan_port_get(uint8 unit, 
                    uint16 vlan_id, 
                    pbmp_t *pbmp, 
                    pbmp_t *tag_pbmp);
extern sys_error_t bcm5354x_vlan_create(uint8 unit, 
                    vlan_type_t type, uint16 vlan_id);
extern sys_error_t bcm5354x_vlan_destroy(uint8 unit, uint16 vlan_id);
extern sys_error_t bcm5354x_vlan_type_set(uint8 unit, vlan_type_t type);
extern sys_error_t bcm5354x_vlan_reset(uint8 unit);
#endif /* CFG_SWITCH_VLAN_INCLUDED */

#ifdef CFG_SWITCH_LAG_INCLUDED
extern sys_error_t bcm5354x_lag_group_set(uint8 unit, uint8 lagid, pbmp_t pbmp);
extern void bcm5354x_lag_group_get(uint8 unit, uint8 lagid, pbmp_t *pbmp);
#endif /* CFG_SWITCH_LAG_INCLUDED */

extern void bcm5354x_loop_detect_enable(BOOL enable);
extern uint8 bcm5354x_loop_detect_status_get(void);

#ifdef CFG_ZEROCONF_MDNS_INCLUDED
sys_error_t bcm5354x_mdns_enable_set(uint8 unit, BOOL mdns_enable);
sys_error_t bcm5354x_mdns_enable_get(uint8 unit, BOOL *mdns_enable);
#endif /* CFG_ZEROCONF_MDNS_INCLUDED */

#ifdef CFG_LED_MICROCODE_INCLUDED
extern void bcm5354x_load_led_program(uint8 unit);
#endif

#define LINKSCAN_INTERVAL        (100000UL)   /* 100 ms */
extern void bcm5354x_linkscan_task(void *param);
extern void bcm5354x_handle_link_down(uint8 unit, uint8 lport, int changed);
extern void bcm5354x_handle_link_up(uint8 unit, uint8 lport, int changed, uint32* flags);
extern sys_error_t bcm5354x_linkscan_init(int timer_period);

extern sys_error_t bcm5354x_phy_reg_get(uint8 unit, uint8 port, uint16 reg_addr, uint16 *p_value);
extern sys_error_t bcm5354x_phy_reg_set(uint8 unit, uint8 port, uint16 reg_addr, uint16 value);


/* Initialization */
extern int bcm5354x_pcm_software_init(int unit);
extern sys_error_t bcm5354x_sw_init(void);
extern void bcm5354x_dscp_map_enable(BOOL enable);
extern void bcm5354x_qos_init(void);
extern void bcm5354x_rate_init(void);
extern void bcm5354x_loop_detect_init(void);
extern void bcm5354x_rxtx_init(void);
extern void bcm5354x_loopback_enable(uint8 unit, uint8 port, int loopback_mode);
extern sys_error_t bcm5354x_mdns_enable_set(uint8 unit, BOOL enable);
extern sys_error_t bcm5354x_mdns_enable_get(uint8 unit, BOOL *enable);

extern int soc_wolfhound2_gphy_get(int unit, int port, uint8 *is_gphy);
extern int soc_wolfhound2_port_reset(int unit);
extern int soc_wh2_sgmii_init_top(int unit, int sgmii_inst);
extern int soc_wh2_qgphy_init(int unit, uint32 qgphy_core_map, uint8 qgphy5_lane);
extern int soc_wh2_sgmii_init_top(int unit, int sgmii_inst);
extern void soc_mmu_init(uint8 unit);

extern mac_driver_t soc_mac_xl, soc_mac_uni;

typedef struct {
    mac_driver_t *p_mac;      /* mac driver */
} wh2_port_info_t;



typedef struct
{
    uint16  devid;
    uint16  revid;
    uint8   link[BCM5354X_LPORT_MAX + 1];           /* link status */
    uint8   loopback[BCM5354X_LPORT_MAX + 1];       /* loopback mode */
    uint8   port_count;
#if defined(CFG_SWITCH_EEE_INCLUDED)
    tick_t link_up_time[BCM5354X_LPORT_MAX+1];
    uint8 need_process_for_eee_1s[BCM5354X_LPORT_MAX+1];
#endif /*  CFG_SWITCH_EEE_INCLUDED */
    int    port_block_id[BCM5354X_LPORT_MAX + 1];           /* logical port */
    int    port_block_port_id[BCM5354X_LPORT_MAX + 1];      /* logical port */
    int    port_block_type[BCM5354X_LPORT_MAX + 1];         /* logical port */
    int    port_speed_status[BCM5354X_LPORT_MAX + 1];        /* logical port */
    BOOL   port_link_status[BCM5354X_LPORT_MAX + 1];       /* logical port */
    BOOL   port_duplex_status[BCM5354X_LPORT_MAX + 1];     /* logical port */
    BOOL   port_an_status[BCM5354X_LPORT_MAX + 1];         /* logical port */
    BOOL   port_tx_pause_status[BCM5354X_LPORT_MAX + 1];   /* logical port */
    BOOL   port_rx_pause_status[BCM5354X_LPORT_MAX + 1];   /* logical port */
    int    port_mode[BCM5354X_LPORT_MAX + 1];   /* logical port */
    int    pmq_block_port_id[BCM5354X_PORT_MAX + 1];    
    int    port_p2m_mapping[BCM5354X_PORT_MAX + 1];   /* logical port */
    int    port_m2p_mapping[BCM5354X_PORT_MAX + 1];   /* logical port */
    wh2_port_info_t port_info[BCM5354X_LPORT_MAX + 1];    
    char   pbmp_format[16];   
	soc_control_t soc_um_control;
    uint32 cpu_clock;
} bcm5354x_sw_info_t;

extern bcm5354x_sw_info_t wh2_sw_info;

#define COS_QUEUE_NUM                   (4)
#define MAXBUCKETCONFIG_NUM             (8)
#define COSLCCOUNT_NUM                  (8)

/* Mask of all logical ports */
#define BCM5354X_ALL_PORTS_MASK          (wh2_sw_info.soc_um_control.info.port.bitmap)

/*
 * Macros to get the device block type, block index and index
 * within block for a given port
 */
#define SOC_PORT_SPEED_INIT(port)        (wh2_sw_info.soc_um_control.info.port_init_speed[(port)])
#define SOC_PORT_SPEED_MAX(port)         (wh2_sw_info.soc_um_control.info.port_speed_max[(port)])
#define SOC_PORT_LANE_NUMBER(port)       (wh2_sw_info.soc_um_control.info.port_num_lanes[(port)])
#define SOC_PORT_SPEED_STATUS(port)      (wh2_sw_info.port_speed_status[(port)])
#define SOC_PORT_LINK_STATUS(port)       (wh2_sw_info.port_link_status[(port)])
#define SOC_PORT_DUPLEX_STATUS(port)     (wh2_sw_info.port_duplex_status[(port)])
#define SOC_PORT_AN_STATUS(port)         (wh2_sw_info.port_an_status[(port)])
#define SOC_PORT_TX_PAUSE_STATUS(port)   (wh2_sw_info.port_tx_pause_status[(port)])
#define SOC_PORT_RX_PAUSE_STATUS(port)   (wh2_sw_info.port_rx_pause_status[(port)])
#define SOC_PORT_BLOCK_TYPE(port)        (wh2_sw_info.port_block_type[(port)])
#define SOC_PORT_BLOCK(port)             (wh2_sw_info.port_block_id[(port)])
#define SOC_PORT_BLOCK_INDEX(port)       (wh2_sw_info.port_block_port_id[(port)])
#define SOC_PORT_P2L_MAPPING(port)       (wh2_sw_info.soc_um_control.info.port_p2l_mapping[(port)])
#define SOC_PORT_L2P_MAPPING(port)       (wh2_sw_info.soc_um_control.info.port_l2p_mapping[(port)])
#define SOC_PORT_L2P_MAPPING_VALID(port) (wh2_sw_info.soc_um_control.info.port_l2p_mapping_valid[(port)])
#define SOC_PORT_P2M_MAPPING(port)       (wh2_sw_info.soc_um_control.info.port_p2m_mapping[(port)])
#define SOC_PORT_M2P_MAPPING(port)       (wh2_sw_info.soc_um_control.info.port_m2p_mapping[(port)])
#define SOC_PORT_U2L_MAPPING(port)       (wh2_sw_info.soc_um_control.info.port_u2l_mapping[(port)])
#define SOC_PORT_L2U_MAPPING(port)       (wh2_sw_info.soc_um_control.info.port_l2u_mapping[(port)])

#define SOC_PORT_COUNT(unit)             (wh2_sw_info.port_count)
#define SOC_PBMP_FORMAT(pbmp)            (_shr_pbmp_format(pbmp, wh2_sw_info.pbmp_format))

#define PORT(unit, lport)                (wh2_sw_info.port_info[lport])
#define SOC_INFO(unit)                   (wh2_sw_info.soc_um_control.info)
#define SOC_CONTROL(unit)                (wh2_sw_info.soc_um_control)
#define SOC_PMQ_BLOCK_INDEX(port)        (wh2_sw_info.pmq_block_port_id[(port)])
#define SOC_PORT_MODE(port)              (wh2_sw_info.port_mode[(port)])

#define IS_CPU_PORT(port)                (port == BCM5354X_PORT_CMIC)
#define IS_XL_PORT(port)                 (SOC_PORT_BLOCK_TYPE(port) == PORT_BLOCK_TYPE_XLPORT)
#define IS_GX_PORT(port)                 (SOC_PORT_BLOCK_TYPE(port) == PORT_BLOCK_TYPE_GXPORT)
#define IS_XE_PORT(port)                 (PBMP_MEMBER(wh2_sw_info.soc_um_control.info.xe.bitmap, port))
#define IS_GE_PORT(port)                 (PBMP_MEMBER(wh2_sw_info.soc_um_control.info.ge.bitmap, port)) 
#define IS_CE_PORT(port)                 (PBMP_MEMBER(wh2_sw_info.soc_um_control.info.ce.bitmap, port))
#define IS_HG_PORT(port)                 (0)
#define IS_ST_PORT(port)                 (0)
#define IS_HL_PORT(port)                 (0)
#define XLPORT_SUBPORT(port)             SOC_PORT_BLOCK_INDEX(port)

/* Do not support HG port */
#define IS_HG_PORT(port) (0)

/* Do not support ST port */
#define IS_ST_PORT(port) (0)
#define SGMIIP4_NUM_OF_CORES     2

#define IS_SGMIIPX4_PORT(lport)         (bcm5354x_pgw_ge_pport_to_blockid[SOC_PORT_L2P_MAPPING(lport)]==PGW_GE2_BLOCK_ID)


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
        for ((_p) = BCM5354X_PORT_MIN; \
             (_p) <= BCM5354X_PORT_MAX; \
             (_p)++) \
                if (SOC_PORT_P2L_MAPPING(_p) != -1)

/* Physical port iteration 1st half(from pport#17 ~ pport#2) for u2l mapping */
#define SOC_PPORT_ITER_1(_p)       \
        for ((_p) = (BCM5354X_PORT_MIN+16-1); \
             (_p) >= BCM5354X_PORT_MIN; \
             (_p)--) \
                if (SOC_PORT_P2L_MAPPING(_p) != -1)
                    
/* Physical port iteration 1st half(from pport#18 ~ pport#37) for u2l mapping */
#define SOC_PPORT_ITER_2(_p)       \
        for ((_p) = (BCM5354X_PORT_MIN+16); \
             (_p) <= BCM5354X_PORT_MAX; \
             (_p)++) \
                if (SOC_PORT_P2L_MAPPING(_p) != -1)                    

/* Logical port iteration */
#define SOC_LPORT_ITER(_p)       \
        for ((_p) = BCM5354X_LPORT_MIN; \
             (_p) <= BCM5354X_LPORT_MAX; \
             (_p)++) \
                if ((SOC_PORT_L2P_MAPPING(_p) != -1))
                    
/* User port iteration */
#define SOC_UPORT_ITER(_p)       \
        for ((_p) = 1; \
             (_p) <= BCM5354X_UPORT_MAX; \
             (_p)++) \
                if ((SOC_PORT_U2L_MAPPING(_p) != -1))

#define SOC_PBMP(pbmp)          (pbmp.pbits[0])

#endif /* _BCM5354X_H_ */
