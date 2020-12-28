/*
 * $Id: soc.h,v 1.35 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _SOC_H_
#define _SOC_H_

/* Chip type */
typedef enum {
    SOC_TYPE_SWITCH_ROBO,
    SOC_TYPE_SWITCH_XGS
} soc_chip_type_t;
#undef SOC_IF_ERROR_RETURN
/* Convenient macros */
#define SOC_IF_ERROR_RETURN(op) \
    do { sys_error_t __rv__; if ((__rv__ = (op)) != SYS_OK) return(__rv__); } while(0)

/* RX handler */
#define SOC_RX_FLAG_TIMESTAMP       (1 << 0)
#define SOC_RX_FLAG_TRAFFIC_CLASS   (1 << 1)
#define SOC_RX_FLAG_TRUNCATED       (1 << 4)
#define SOC_RX_FLAG_ERROR_CRC       (1 << 5)
#define SOC_RX_FLAG_ERROR_OTHER     (1 << 6)
typedef struct soc_rx_packet_s {
    /* Filled by upper caller */
    uint8   *buffer;
    uint8   *alloc_ptr;                      /* Pointer for reusing buffer (internal). */
    uint16  buflen;

    /* Filled by soc RX engine */
    uint16  flags;
    uint16  pktlen;
    uint8   unit;
    uint8   lport;
    uint16  traffic_class;
    uint32  timestamp;
    uint32  timestamp_upper;   /* Upper 32-bit of 64-bit timestamp */
    uint32  reserved1;
    uint32  reserved2;

    /* For chaining */
    struct soc_rx_packet_s *next;
} soc_rx_packet_t;
typedef void (*SOC_RX_HANDLER)(soc_rx_packet_t *) REENTRANT;

/* TX callback */
#define SOC_TX_FLAG_TIMESTAMP_REQUEST               (1 << 0)
#define SOC_TX_FLAG_USE_UNTAG_PORT_BITMAP           (1 << 1)
/* TimeSync Packet Flags. */
#define SOC_TX_FLAG_TIMESYNC_ONE_STEP                  (1 << 2) /* One step timestamp. */
#define SOC_TX_FLAG_TIMESYNC_ONE_STEP_INGRESS_SIGN     (1 << 3) /* Ingress timestamp sign bit. */
#define SOC_TX_FLAG_TIMESYNC_ONE_STEP_HDR_START_OFFSET (1 << 4) /* PTP header offset in packet buffer. */
#define SOC_TX_FLAG_TIMESYNC_ONE_STEP_REGEN_UDP_CHKSUM (1 << 5) /* Regenerate UDP header checksum of PTP packet. */
#define SOC_TX_FLAG_TIMESYNC_TWO_STEP                  (1 << 6) /* Two-step timestamp. */

#define SOC_TX_TAG_MODE_FOLLOW_SWITCH_RULES         (0)
#define SOC_TX_TAG_MODE_UNTAG_ALL                   (1)
#define SOC_TX_TAG_MODE_TAG_ALL                     (2)
struct soc_tx_packet_s;
typedef void (*SOC_TX_CALLBACK)(struct soc_tx_packet_s *pkt) REENTRANT;
typedef struct soc_tx_packet_s {

    /* Filled by caller */
    uint8   *buffer;
    uint16  pktlen;
    uint16  flags;
    uint16  traffic_class;
    pbmp_t  port_bitmap;     /* Follow switch ARL if empty */
    pbmp_t  untag_bitmap;    /* Valid only if FOLLOW_VLAN_UNTAG_RULES not set */
    uint8   tag_mode;        /* Valid only if port_bitmap is empty  */
    uint8   timestamp_offset;/* Offset to place the timestamp in the packet. */

    /* Reserved */
    uint32  reserved1;
    uint32  reserved2;
    uint32  reserved3;
    uint32  reserved4;

    /* Filled by switch driver */
    uint8 unit;
    sys_error_t status;

    /* Used by caller */
    SOC_TX_CALLBACK callback;   /* Called after packet sent; Must set */
    void *cookie;

    /* Used internally in SOC layer */
    uint32 internal0;

    /* For chaining */
    struct soc_tx_packet_s *next;
} soc_tx_packet_t;

/* physical bitmap check *:
 *  - this macro is designed for the CPU port is different between
 *      XGS and ROBO chips.
 *  p.s. CPU port is always at bit 0 in XGS
 */
#define SOC_PBMP_PORT_SET(_type, _pbmp, _pport)     \
        ((_pbmp) |= (0x1 << (_type == SOC_TYPE_SWITCH_XGS) ? \
                (_pport + 1) : (_pport)));
#define SOC_PBMP_PORT_CLEAR(_type, _pbmp, _pport)     \
        ((_pbmp) &= ~(0x1 << (_type == SOC_TYPE_SWITCH_XGS) ? \
                (_pport + 1) : (_pport)));
#define SOC_PBMP_PORT_CHECK(_type, _pbmp, _pport)   \
        ((_pbmp) && (0x1 << (_type == SOC_TYPE_SWITCH_XGS) ? \
                (_pport + 1) : (_pport)))

#ifdef CFG_SWITCH_VLAN_INCLUDED
typedef enum vlan_type_s {
    VT_NONE,
    VT_PORT_BASED,
    VT_DOT1Q,

    VT_COUNT
} vlan_type_t;

#endif  /* CFG_SWITCH_VLAN_INCLUDED */
/* Those SOC VLAN types are defined based on ROBO's chip spec.
 *  Any new chip feature on support new VLAN type can be expended.
 */
typedef enum soc_vlan_type_s {
    SOC_VT_PORT_BASED,
    SOC_VT_DOT1Q_BASED,
    SOC_VT_MAC_BASED,
    SOC_VT_PROTOCOL_BASED,

    SOC_VT_COUNT
} soc_vlan_type_t;

#define SOC_MIN_VLAN_ID     0
#define SOC_MAX_VLAN_ID     4095
#define SOC_MIN_1QVLAN_ID   1
#define SOC_MAX_1QVLAN_ID   4094

#define SOC_VLAN_ID_IS_VALID(_vid)  \
        ((_vid) >= SOC_MIN_VLAN_ID && (_vid) <= SOC_MAX_VLAN_ID)
#define SOC_1QVLAN_ID_IS_VALID(_vid)  \
        ((_vid) >= SOC_MIN_1QVLAN_ID && (_vid) <= SOC_MAX_1QVLAN_ID)

#ifdef CFG_SWITCH_DOS_INCLUDED

/* if new chip add and new DOS type is supported, extend below definitions
 * to the higher bit(or bits) and the SOC_DOS_ALL_COUNT need be increased for
 * including those new DOS type(or types).
 */
#define SOC_DOS_ALL_COUNT                    13

#define SOC_DOS_ICMPV6_LONG_PING_DROP       ((uint16)(1U << 12))
#define SOC_DOS_ICMPV4_LONG_PING_DROP       ((uint16)(1U << 11))
#define SOC_DOS_ICMPV6_FRAGMENT_DROP        ((uint16)(1U << 10))
#define SOC_DOS_ICMPV4_FRAGMENT_DROP        ((uint16)(1U << 9))
#define SOC_DOS_TCP_FRAG_ERR_DROP           ((uint16)(1U << 8))
#define SOC_DOS_TCP_SHORT_HDR_DROP          ((uint16)(1U << 7))
#define SOC_DOS_TCP_SYN_ERR_DROP            ((uint16)(1U << 6))
#define SOC_DOS_TCP_SYNFIN_SCAN_DROP        ((uint16)(1U << 5))
#define SOC_DOS_TCP_XMASS_SCAN_DROP         ((uint16)(1U << 4))
#define SOC_DOS_TCP_NULL_SCAN_DROP          ((uint16)(1U << 3))
#define SOC_DOS_UDP_BLAT_DROP               ((uint16)(1U << 2))
#define SOC_DOS_TCP_BLAT_DROP               ((uint16)(1U << 1))
#define SOC_DOS_IP_LAND_DROP                ((uint16)(1U << 0))

#define SOC_DOS_DISABLED                    0

/* check valid bit in 16 bits width only, return 0 means all valid */
#define SOC_DOS_VALID_CHECK(dos_list)       \
        (~((uint16)((0x1 << SOC_DOS_ALL_COUNT) - 1)) & ((uint16)(dos_list)))

#endif  /* CFG_SWITCH_DOS_INCLUDED */

/*
 *  L2 ARL related definitions
 */
#define SOC_ARL_INVALID_BIN_ID      (-1)

#ifdef CFG_SWITCH_SYNCE_INCLUDED
/* SyncE Clock Source Type. */
typedef enum bcm_time_synce_clock_src_type_e {
    bcmTimeSynceClockSourcePrimary = 0, /* Primary Clock Source */
    bcmTimeSynceClockSourceSecondary = 1 /* Secondary Clock Source */
} bcm_time_synce_clock_src_type_t;

/* SyncE Input Source Type. */
typedef enum bcm_time_synce_input_src_type_e {
    bcmTimeSynceInputSourceTypePort = 0, /* Input Source Port */
    bcmTimeSynceInputSourceTypePLL = 1  /* Input Source PLL */
} bcm_time_synce_input_src_type_t;

typedef struct bcm_time_synce_clock_source_config_s {
    bcm_time_synce_clock_src_type_t clk_src; /* SyncE Clock Source Primary or
                                           Secondary */
    bcm_time_synce_input_src_type_t input_src; /* Input source type Port or PLL */
    uint32 pll_index;                   /* PLL index */
    uint8 port;                    /* Logical port number */
} bcm_time_synce_clock_source_config_t;

/* bcm_time_synce_clock_source_control */
typedef enum bcm_time_synce_clock_source_control_e {
    bcmTimeSynceClockSourceControlSquelch = 0, /* Squelch of SyncE source */
    bcmTimeSynceClockSourceControlFrequency = 1 /* Frequency of SyncE source */
} bcm_time_synce_clock_source_control_t;

#endif  /* CFG_SWITCH_SYNCE_INCLUDED */

/*
 * SOC switch class
 */
typedef struct soc_switch_s {

    /* Chip type */
    soc_chip_type_t (*chip_type)(void) REENTRANT;

    /* Chip revision */
    sys_error_t (*chip_revision)(uint8 unit, uint8 *rev) REENTRANT;

    /* Number of ports */
    uint8 (*port_count)(uint8 unit) REENTRANT;

    /* Robo register read/write (for SOC_TYPE_SWITCH_ROBO) */
    sys_error_t (*robo_switch_reg_get)(uint8 unit,
                                       uint8 page,
                                       uint8 offset,
                                       uint8 *buf,
                                       uint8 len) REENTRANT;
    sys_error_t (*robo_switch_reg_set)(uint8 unit,
                                       uint8 page,
                                       uint8 offset,
                                       uint8 *buf,
                                       uint8 len) REENTRANT;

#if CFG_RXTX_SUPPORT_ENABLED

    /*
     * Set (the one and only) RX handler.
     * If intr is TRUE, the handler will be called in interrupt context
     */
    sys_error_t (*rx_set_handler)(uint8 unit, SOC_RX_HANDLER fn, BOOL intr) REENTRANT;


    /*
     * Fill (or refill) one packet buffer to RX engine.
     * You can fill more than one buffers until it returns SYS_ERR_FULL
     */
    sys_error_t (*rx_fill_buffer)(uint8 unit, soc_rx_packet_t *pkt) REENTRANT;

    /*
     * Packet TX
     */
    sys_error_t (*tx)(uint8 unit, soc_tx_packet_t *pkt) REENTRANT;

#endif /* CFG_RXTX_SUPPORT_ENABLED */

    /* Link status of a port */
    sys_error_t (*link_status)(uint8 unit, uint8 lport, BOOL *link) REENTRANT;

    /* XGS chip revision */
    sys_error_t (*xgs_chip_revision)(uint8 unit, uint16 *dev, uint16 *rev) REENTRANT;

    /* XGS register/memory read/write */
#ifdef CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED
    /* New sbus format(separated block id) */
    sys_error_t (*xgs_switch_reg_get)(uint8 unit,
                                      uint8 block,
                                      uint32 addr,
                                      uint32 *val) REENTRANT;
    sys_error_t (*xgs_switch_reg_set)(uint8 unit,
                                      uint8 block,
                                      uint32 addr,
                                      uint32 val) REENTRANT;
    sys_error_t (*xgs_switch_mem_get)(uint8 unit,
                                      uint8 block,
                                      uint32 addr,
                                      uint32 *buf,
                                      int len) REENTRANT;
    sys_error_t (*xgs_switch_mem_set)(uint8 unit,
                                      uint8 block,
                                      uint32 addr,
                                      uint32 *buf,
                                      int len) REENTRANT;
#else
    sys_error_t (*xgs_switch_reg_get)(uint8 unit,
                                      uint32 addr,
                                      uint32 *val) REENTRANT;
    sys_error_t (*xgs_switch_reg_set)(uint8 unit,
                                      uint32 addr,
                                      uint32 val) REENTRANT;
    sys_error_t (*xgs_switch_mem_get)(uint8 unit,
                                      uint32 addr,
                                      uint32 *buf,
                                      int len) REENTRANT;
    sys_error_t (*xgs_switch_mem_set)(uint8 unit,
                                      uint32 addr,
                                      uint32 *buf,
                                      int len) REENTRANT;
#endif /* CFG_SWITCH_XGS_NEW_SBUS_FORMAT_INCLUDED */

#ifdef CFG_SWITCH_VLAN_INCLUDED
    sys_error_t (*pvlan_egress_set)(uint8 unit,
                                       uint8 lport,
                                       pbmp_t lpbmp)   REENTRANT;

    sys_error_t (*pvlan_egress_get)(uint8 unit,
                                       uint8 lport,
                                       pbmp_t *lpbmp)   REENTRANT;
    sys_error_t (*qvlan_port_set)(uint8 unit,
                                       uint16 vlan_id,
                                       pbmp_t lpbmp,
                                       pbmp_t tag_lpbmp)   REENTRANT;

    sys_error_t (*qvlan_port_get)(uint8 unit,
                                       uint16 vlan_id,
                                       pbmp_t *lpbmp,
                                       pbmp_t *tag_lpbmp)   REENTRANT;

    sys_error_t (*vlan_create)(uint8 unit,
                                       vlan_type_t type,
                                       uint16 vlan_id)   REENTRANT;

    sys_error_t (*vlan_destroy)(uint8 unit,
                                       uint16 vlan_id)   REENTRANT;
    sys_error_t (*vlan_type_set)(uint8 unit,
                                       vlan_type_t type)   REENTRANT;
    sys_error_t (*vlan_reset)(uint8 unit)   REENTRANT;
#endif  /* CFG_SWITCH_VLAN_INCLUDED */
    sys_error_t (*phy_reg_get)(uint8 unit, uint8 lport, uint16 reg_addr, uint16 *p_value);
    sys_error_t (*phy_reg_set)(uint8 unit, uint8 lport, uint16 reg_addr, uint16 value);
} soc_switch_t;

#endif /* _SOC_H_ */
