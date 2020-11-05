/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _BOARDAPI_PORT_H_
#define _BOARDAPI_PORT_H_

/* Get link status of a port */
extern sys_error_t board_get_port_link_status(uint16 uport, BOOL *link) REENTRANT;

/* Get link speed of a port */
extern sys_error_t board_get_port_link_speed(uint16 uport, int16 *speed) REENTRANT;

/* Get the status of auto neg */
extern sys_error_t board_get_port_phy_autoneg(uint16 uport, BOOL *an, BOOL *an_done) REENTRANT;

/* Get the remote EEE capability */
sys_error_t board_get_port_phy_eee_ability_remote(uint16 uport, uint8 *types) REENTRANT;

/* Get the minimum PHY EEE transition wake time */
extern sys_error_t board_get_port_phy_eee_min_wake_time(uint16 uport, uint8 type, uint16 *wake_t) REENTRANT;

/* Set the EEE tx wake timer */
extern sys_error_t board_set_port_phy_eee_tx_wake_time(uint16 uport, uint8 type, uint16 wake_t) REENTRANT;
    
/*
 * Port status
 */
typedef enum port_mode_s {
    PM_LINKDOWN = 0,
    PM_10MB_HD = 10,
    PM_10MB_FD = 11,
    PM_100MB_HD = 100,
    PM_100MB_FD = 101,
    PM_1000MB = 1000,
    PM_2500MB = 2500,
    PM_5000MB = 5000,
    PM_10000MB = 10000,
    PM_25000MB = 25000,
    PM_40000MB = 40000,
    PM_50000MB = 50000,
    PM_100000MB = 100000,
    PM_AUTO = -1,
} port_mode_t;

extern sys_error_t board_port_mode_get(uint16 uport, 
                                       port_mode_t *mode) REENTRANT;
                                       
extern sys_error_t board_port_enable_get(uint16 uport, 
                                       BOOL *portenable) REENTRANT;

extern sys_error_t board_port_enable_set(uint16 uport, 
                                       BOOL portenable) REENTRANT;

extern sys_error_t board_port_an_get(uint16 uport, 
                                       BOOL *an) REENTRANT;

extern sys_error_t board_port_pause_get(uint16 uport, 
                                       BOOL *tx_pause, BOOL *rx_pause) REENTRANT;

                                       
/*
 * Cable Diagnostics
 */
typedef enum {
    PORT_CABLE_STATE_NO_CABLE,
    PORT_CABLE_STATE_OK,
    PORT_CABLE_STATE_OPEN,
    PORT_CABLE_STATE_SHORT,
    PORT_CABLE_STATE_OPENSHORT,
    PORT_CABLE_STATE_CROSSTALK
} port_cable_state_t;

typedef struct port_cable_diag_s {
    port_cable_state_t  state;   /* state */
    int32               length;  /* length in metres */
} port_cable_diag_t;

extern sys_error_t board_port_cable_diag(uint16 uport, 
                                         port_cable_diag_t *status);

extern sys_error_t board_get_cable_diag_support_by_port(uint16 uport,
                                         BOOL *support);
extern uint16 board_cable_diag_port_count(void) REENTRANT;
/*
 * MIB information
 */
typedef struct port_stat_s {
    /* Byte Counter */
    uint32  RxOctets_lo;  /* Rx byte count lo */
    uint32  RxOctets_hi;  /* Rx byte count hi */ 
    uint32  TxOctets_lo;  /* Tx byte count lo */
    uint32  TxOctets_hi;  /* Tx byte count hi */
    /* Frame counter */
    uint32  RxPkts_lo;
    uint32  RxPkts_hi;
    uint32  TxPkts_lo;
    uint32  TxPkts_hi;
    /* Rx FCS Error Frame Counter */
    uint32  CRCErrors_lo;
    uint32  CRCErrors_hi;
    /* Unicast Frame Counter */   
    uint32  RxUnicastPkts_lo;
    uint32  RxUnicastPkts_hi;
    uint32  TxUnicastPkts_lo;
    uint32  TxUnicastPkts_hi;       
    /* Multicast Frame Counter */
    uint32  RxMulticastPkts_lo;
    uint32  RxMulticastPkts_hi;
    uint32  TxMulticastPkts_lo;
    uint32  TxMulticastPkts_hi;    
    /* Broadcast Frame Counter */
    uint32  RxBroadcastPkts_lo;
    uint32  RxBroadcastPkts_hi;
    uint32  TxBroadcastPkts_lo;
    uint32  TxBroadcastPkts_hi;
    /* Pause Frame Counter */
    uint32  RxPauseFramePkts_lo;
    uint32  RxPauseFramePkts_hi;
    uint32  TxPauseFramePkts_lo;
    uint32  TxPauseFramePkts_hi;
    /* Oversized Frame Counter */
    uint32  RxOversizePkts_lo;
    uint32  RxOversizePkts_hi;
    uint32  TxOversizePkts_lo;
    uint32  TxOversizePkts_hi;

     /* IPIPE HOLD Drop Counter */
    uint32  EgressDropPkts_lo;
    uint32  EgressDropPkts_hi;

     /* EEE LPI counter */
    uint32  RxLPIPkts_hi;   
    uint32  RxLPIDuration_hi;
    uint32  TxLPIPkts_hi;
    uint32  TxLPIDuration_hi;
    uint32  RxLPIPkts_lo;   
    uint32  RxLPIDuration_lo;
    uint32  TxLPIPkts_lo;
    uint32  TxLPIDuration_lo;
    
} port_stat_t;
extern sys_error_t board_port_stat_get(uint16 uport, port_stat_t *stat);

/*
 * Reset counters
 */
extern sys_error_t board_port_stat_clear(uint16 uport);
extern sys_error_t board_port_stat_clear_all(void);

#define PORT_LOOPBACK_NONE    0
#define PORT_LOOPBACK_MAC     1
#define PORT_LOOPBACK_PHY     2
#define PORT_LOOPBACK_PHY_REMOTE 3
#define PORT_LOOPBACK_COUNT 4

extern sys_error_t board_port_loopback_enable_set(uint16 uport, int lb_mode);

#endif /* _BOARDAPI_PORT_H_ */
