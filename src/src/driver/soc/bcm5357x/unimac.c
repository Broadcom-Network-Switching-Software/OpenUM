/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#define _SAL_TYPES_H
#define CONST const

#include "system.h"
#include "utils/system.h"
#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>
#include <pcm/pcm_int.h>


/*
 * Forward Declarations
 */
mac_driver_t soc_mac_uni;

#define SOC_UNIMAC_SPEED_10     0x0
#define SOC_UNIMAC_SPEED_100    0x1
#define SOC_UNIMAC_SPEED_1000   0x2
#define SOC_UNIMAC_SPEED_2500   0x3

static void
soc_egress_drain_cells(uint8 unit, uint8 lport, uint32 drain_timeout)
{
    int cos;
    int timeout;
    uint32 count;
    int mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(lport)); 
    
    EGRMETERINGCONFIGr_t orig_egrmeteringconfig, egrmeteringconfig;
    COSLCCOUNTr_t coslccount;

    READ_EGRMETERINGCONFIGr(unit, mport,  orig_egrmeteringconfig);
    EGRMETERINGCONFIGr_CLR(egrmeteringconfig);
    WRITE_EGRMETERINGCONFIGr(unit, mport, egrmeteringconfig);


    timeout = 100000;
    do {
        count = 0;
        for (cos = 0; cos < COS_QUEUE_NUM; cos++) {           
            READ_COSLCCOUNTr(unit, mport, cos, coslccount);
            count += COSLCCOUNTr_LCCOUNTf_GET(coslccount);
        }
        sal_usleep(10);
        timeout--;
    } while (count != 0 && timeout > 0 );

    WRITE_EGRMETERINGCONFIGr(unit, mport, orig_egrmeteringconfig);

}

static int
_mac_uni_sw_reset(uint8 unit, uint8 lport, BOOL reset_assert)
{
    int reset_sleep_usec;

    COMMAND_CONFIGr_t command_config;

#if CONFIG_EMULATION
    reset_sleep_usec = 50000;
#else
    reset_sleep_usec = 2;
#endif

    READ_COMMAND_CONFIGr(unit, lport, command_config);
    if (reset_assert) {
        /* SIDE EFFECT: TX and RX are disabled when SW_RESET is set. */
        /* Assert SW_RESET */
        COMMAND_CONFIGr_SW_RESETf_SET(command_config, 1);
    } else {
        /* Deassert SW_RESET */
        COMMAND_CONFIGr_SW_RESETf_SET(command_config, 0);
    }
    WRITE_COMMAND_CONFIGr(unit, lport, command_config);

    sal_usleep(reset_sleep_usec);

    return SYS_OK;
}

/*
 * Function:
 *      mac_uni_ifg_to_ipg
 * Description:
 *      Converts the inter-frame gap specified in bit-times into a value
 *      suitable to be programmed into the IPG register
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      ifg    - Inter-frame gap in bit-times
 *      ipg    - (OUT) the value to be written into IPG
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      The function makes sure the calculated IPG value will not cause
 *      hardware to fail. If the requested ifg value cannot be supported in
 *      hardware, the function will choose a value that approximates the
 *      requested value as best as possible.
 *
 *      Specifically:
 *         -- Current chips only support ifg which is divisible by 8. If
 *            the specified ifg is not divisible by 8, it will be rounded
 *            to the next multiplier of 8 (65 will result in 72).
 *         -- ifg < 64 are not supported
 *
 *      This function supports only GE portion of GE MAC (FE portion is
 *      supported by mac_fe_ipg_* functions
 */
STATIC int
mac_uni_ifg_to_ipg(int unit, soc_port_t port, int speed, int duplex,
                  int ifg, int *ipg)
{
    int         real_ifg;

    /* 
     * Silently adjust the specified ifp bits to valid value
     *  - valid value: 8 to 31 bytes (i.e. multiple of 8 bits) 
     */
    real_ifg = ifg < 64 ? 64 : (ifg > 248 ? 248 : (ifg + 7) & (0x1f << 3));
    *ipg = real_ifg / 8;

    return SYS_OK;
}

/*
 * Function:
 *      mac_uni_ipg_to_ifg
 * Description:
 *      Converts the IPG register value into the inter-frame gap expressed in
 *      bit-times
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      ipg    - the value in the IPG register
 *      ifg    - Inter-frame gap in bit-times
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      This function supports only GE portion of GE MAC (FE portion is
 *      supported by mac_fe_ipg_* functions
 */
STATIC int
mac_uni_ipg_to_ifg(int unit, soc_port_t port, int speed, int duplex,
                  int ipg, int *ifg)
{
    /*
     * Now we need to convert the value according to various chips'
     * peculiarities (there are none as of now)
     */
    *ifg = ipg * 8;

    return SYS_OK;
}


STATIC int
mac_uni_pause_get(int unit, uint8 lport, int *pause_tx, int *pause_rx)
{
    COMMAND_CONFIGr_t              command_config;


    SOC_IF_ERROR_RETURN(READ_COMMAND_CONFIGr(unit, lport, command_config));
   
    
    *pause_rx = !COMMAND_CONFIGr_PAUSE_IGNOREf_GET(command_config);
    *pause_tx =  !COMMAND_CONFIGr_IGNORE_TX_PAUSEf_GET(command_config);


    SAL_DEBUGF(("mac_uni_pause_get: port %d RX=%s TX=%s\n", lport,
                 *pause_rx ? "on" : "off",
                 *pause_tx ? "on" : "off"));
    return SYS_OK;
}

static int
mac_uni_pause_set(int unit, uint8 lport, int pause_tx, int pause_rx)
{

    COMMAND_CONFIGr_t command_config;


    READ_COMMAND_CONFIGr(unit, lport, command_config);
    if (pause_tx) {
        COMMAND_CONFIGr_IGNORE_TX_PAUSEf_SET(command_config, 0);
    } else {
        COMMAND_CONFIGr_IGNORE_TX_PAUSEf_SET(command_config, 1);
    }

    if (pause_rx) {
        COMMAND_CONFIGr_PAUSE_IGNOREf_SET(command_config, 0);
    } else {
        COMMAND_CONFIGr_PAUSE_IGNOREf_SET(command_config, 1);
    }

    /* First put the MAC in reset */
    _mac_uni_sw_reset(unit, lport, TRUE);

    COMMAND_CONFIGr_SW_RESETf_SET(command_config, 1);
    WRITE_COMMAND_CONFIGr(unit, lport, command_config);

    /* Add 2usec delay before deasserting SW_RESET */
    sal_usleep(2);

    /* Bring the MAC out of reset */
    _mac_uni_sw_reset(unit, lport, FALSE); 

    return SYS_OK;
}

static void
_mac_uni_drain_cells(uint8 unit, uint8 lport)
{

    BOOL pause_tx = 1, pause_rx = 1;
    MMUFLUSHCONTROLr_t mmuflushcontrol;
    COMMAND_CONFIGr_t command_config;
    FLUSH_CONTROLr_t flush_control;
    uint32 flush = 0;
    uint8 mport; 
    /* First put the port in flush state - the packets from the XQ of the 
     * port are purged after dequeue.
     */
    mport = SOC_PORT_P2M_MAPPING(SOC_PORT_L2P_MAPPING(lport));
    MMUFLUSHCONTROLr_CLR(mmuflushcontrol);
    if (mport >= 32) {
        flush = 1 << (mport-32);
        MMUFLUSHCONTROLr_FLUSHf_SET(mmuflushcontrol, flush);
        WRITE_MMUFLUSHCONTROLr(unit, 1, mmuflushcontrol);
    } else {
        flush = 1 << (mport);
        MMUFLUSHCONTROLr_FLUSHf_SET(mmuflushcontrol, flush);
        WRITE_MMUFLUSHCONTROLr(unit, 0, mmuflushcontrol);
    }

    READ_COMMAND_CONFIGr(unit, lport, command_config);

    /* Bit[8]: PAUSE_IGNORE, Bit[28]: IGNORE_TX_PAUSE */
    if (COMMAND_CONFIGr_IGNORE_TX_PAUSEf_GET(command_config)) {
        pause_tx = 0;
    }

    if (COMMAND_CONFIGr_PAUSE_IGNOREf_GET(command_config)) {
        pause_rx = 0;
    }

    /* Disable pause function */
    soc_mac_uni.md_pause_set(unit, lport, 0, 0);


    FLUSH_CONTROLr_CLR(flush_control);
    FLUSH_CONTROLr_FLUSHf_SET(flush_control, 1);

    /* Drop out all packets in TX FIFO without egressing any packets */
    WRITE_FLUSH_CONTROLr(unit, lport, flush_control);

    /* Notify PHY driver */
    soc_phyctrl_notify(unit, lport, phyEventStop, PHY_STOP_DRAIN);

    /* Disable switch egress metering so that packet draining is not rate
     * limited.
     */
    soc_egress_drain_cells(unit, lport, 250000);

    /* Notify PHY driver */
    soc_phyctrl_notify(unit, lport, phyEventResume, PHY_STOP_DRAIN);

    /* Soft-reset is recommended here. 
     * SOC_IF_ERROR_RETURN
     *     (soc_mac_uni.md_control_set(unit, port, SOC_MAC_CONTROL_SW_RESET,
     *                                 TRUE));
     * SOC_IF_ERROR_RETURN
     *     (soc_mac_uni.md_control_set(unit, port, SOC_MAC_CONTROL_SW_RESET,
     *                                 FALSE));
     */
  
    /* Bring the TxFifo out of flush */
    FLUSH_CONTROLr_FLUSHf_SET(flush_control, 0);
    WRITE_FLUSH_CONTROLr(unit, lport, flush_control);

    /* Restore original pause configuration */
    mac_uni_pause_set(unit, lport, pause_tx, pause_rx);
    
    /* Bring the switch MMU out of flush */
    MMUFLUSHCONTROLr_CLR(mmuflushcontrol);
    if (mport >= 32) {
        WRITE_MMUFLUSHCONTROLr(unit, 1, mmuflushcontrol);
    } else {
        WRITE_MMUFLUSHCONTROLr(unit, 0, mmuflushcontrol);
    }


}

static int
mac_uni_enable_set(int unit, uint8 lport, int enable)
{
    COMMAND_CONFIGr_t command_config;
    EPC_LINK_BMAP_HI_64r_t epc_link_bmap_hi_64;
    EPC_LINK_BMAP_LO_64r_t epc_link_bmap_lo_64;


    READ_COMMAND_CONFIGr(unit, lport, command_config);

    /* First put the MAC in reset */
    _mac_uni_sw_reset(unit, lport, TRUE);

    /* de-assert RX_ENA and TX_ENA */
    COMMAND_CONFIGr_RX_ENAf_SET(command_config, 0);
    COMMAND_CONFIGr_TX_ENAf_SET(command_config, 0);
    COMMAND_CONFIGr_SW_RESETf_SET(command_config, 1);
    WRITE_COMMAND_CONFIGr(unit, lport, command_config);
    sal_usleep(2);

    /* Bring the MAC out of reset */
    _mac_uni_sw_reset(unit, lport, FALSE);

    if (!enable) {
        /* Remove port from EPC_LINK */
        if (lport >= 64) {
            READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
            PBMP_PORT_REMOVE(*((pbmp_t *) &epc_link_bmap_hi_64), (lport-64));
            WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
        } else {
           READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
           PBMP_PORT_REMOVE(*((pbmp_t *) &epc_link_bmap_lo_64), lport);
           WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
        }
        _mac_uni_drain_cells(unit, lport);

        /* Put in reset */
        _mac_uni_sw_reset(unit, lport, TRUE);
        
        soc_phyctrl_notify(unit, lport, phyEventStop, PHY_STOP_MAC_DIS);
    } else {
        /* if it is to enable, assert RX_ENA and TX_ENA */
        COMMAND_CONFIGr_RX_ENAf_SET(command_config, 1);
        COMMAND_CONFIGr_TX_ENAf_SET(command_config, 1);
        COMMAND_CONFIGr_SW_RESETf_SET(command_config, 0);

        WRITE_COMMAND_CONFIGr(unit, lport, command_config);

        sal_usleep(2);

        /* Add port to EPC_LINK */
        if (lport >= 64) {
            READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
            PBMP_PORT_ADD(*((pbmp_t *) &epc_link_bmap_hi_64), (lport-64));
            WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
        } else {
           READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
           PBMP_PORT_ADD(*((pbmp_t *) &epc_link_bmap_lo_64), lport);
           WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
        }


        _mac_uni_sw_reset(unit, lport, TRUE);
        soc_phyctrl_notify(unit, lport, phyEventResume, PHY_STOP_MAC_DIS);
        _mac_uni_sw_reset(unit, lport, FALSE);
    }
    return SYS_OK;
}

static int
mac_uni_init(int unit, uint8 lport)
{
    COMMAND_CONFIGr_t command_config, ocommand_config;
    GPORT_RSV_MASKr_t gport_rsv_mask;
    GPORT_STAT_UPDATE_MASKr_t gport_stat_update_mask;
    TX_IPG_LENGTHr_t tx_ipg_length;


    /* First put the MAC in reset and sleep */
    _mac_uni_sw_reset(unit, lport, TRUE);

    /* Do the initialization */
    /* 
     *   ETH_SPEEDf = 1000, PROMIS_ENf = 1, CRC_FWDf=1, PAUSE_FWDf = 1
     */
    READ_COMMAND_CONFIGr(unit, lport, ocommand_config);
    COMMAND_CONFIGr_CLR(command_config);
    COMMAND_CONFIGr_SW_RESETf_SET(command_config, 1);
    COMMAND_CONFIGr_PROMIS_ENf_SET(command_config, 1);
    COMMAND_CONFIGr_PAUSE_FWDf_SET(command_config, 1);
    COMMAND_CONFIGr_NO_LGTH_CHECKf_SET(command_config, 1);
    COMMAND_CONFIGr_ETH_SPEEDf_SET(command_config, 2);
    COMMAND_CONFIGr_CRC_FWDf_SET(command_config, 1);
    
    if (sal_memcmp(&ocommand_config, &command_config, sizeof(command_config))) {
        return SYS_OK;
    }
    WRITE_COMMAND_CONFIGr(unit, lport, command_config);
    /* Initialize mask for purging packet data received from the MAC */    
    GPORT_RSV_MASKr_CLR(gport_rsv_mask);
    GPORT_RSV_MASKr_MASKf_SET(gport_rsv_mask, 0x70);
    WRITE_GPORT_RSV_MASKr(unit, lport, gport_rsv_mask);

    GPORT_STAT_UPDATE_MASKr_CLR(gport_stat_update_mask);
    GPORT_STAT_UPDATE_MASKr_MASKf_SET(gport_stat_update_mask, 0x70);
    WRITE_GPORT_STAT_UPDATE_MASKr(unit, lport, gport_stat_update_mask);


    /* Bring the UniMAC out of reset */
    _mac_uni_sw_reset(unit, lport, FALSE);

    /* Pulse the Serdes AN if using auto_cfg mode */
#if 0
    if (auto_cfg[unit][port]) {
        soc_phyctrl_notify(unit, port, phyEventAutoneg, 0);
        soc_phyctrl_notify(unit, port, phyEventAutoneg, 1);
    }
#endif

    TX_IPG_LENGTHr_CLR(tx_ipg_length);
    TX_IPG_LENGTHr_TX_IPG_LENGTHf_SET(tx_ipg_length, 12);
    WRITE_TX_IPG_LENGTHr(unit, lport, tx_ipg_length);

    return SYS_OK;
}


static int mac_uni_speed_get(int unit, uint8 lport, int *speed) 
{
        COMMAND_CONFIGr_t    command_config;
        uint32 uni_speed;
    
        SOC_IF_ERROR_RETURN(READ_COMMAND_CONFIGr(unit, lport, command_config));
    
        uni_speed = COMMAND_CONFIGr_ETH_SPEEDf_GET(command_config);
    
        switch(uni_speed) {
        case SOC_UNIMAC_SPEED_10:
            *speed = 10;
            break;
        case SOC_UNIMAC_SPEED_100:
            *speed = 100;
            break;
        case SOC_UNIMAC_SPEED_1000:
            *speed = 1000;
            break;
        case SOC_UNIMAC_SPEED_2500:
            *speed = 2500;
            break;
        default:
            return SYS_ERR;
            break;
        }
    
        SAL_DEBUGF(("mac_uni_speed_get: unit %d port %d speed=%dMb\n", unit, lport, *speed));

    return SYS_OK;
}

                                      

/*
 * Function:
 *      mac_uni_duplex_get
 * Purpose:
 *      Get UniMAC duplex mode.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port # on unit.
 *      duplex - (OUT) Boolean: true --> full duplex, false --> half duplex.
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
mac_uni_duplex_get(int unit, uint8 lport, int *duplex)
{
    COMMAND_CONFIGr_t command_config;

    int    speed;


    SOC_IF_ERROR_RETURN(mac_uni_speed_get(unit, lport, &speed));

    if ((1000 == speed) || (2500 == speed)) {
        *duplex = TRUE;
    } else {
        SOC_IF_ERROR_RETURN
            (READ_COMMAND_CONFIGr(unit, lport, command_config));

        *duplex = COMMAND_CONFIGr_HD_ENAf_GET(command_config) ? FALSE : TRUE;
    }

    SAL_DEBUGF(("mac_uni_duplex_get: unit %d port %d duplex=%s\n",unit, lport,*duplex ? "Full" : "Half"));
    return SYS_OK;
}







/*
 * Function:
 *      mac_uni_ipg_update
 * Purpose:
 *      Set the IPG appropriate for current duplex
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - Port number on unit.
 * Notes:
 *      The current duplex is read from the hardware registers.
 */
#define UNIMAC_IFG_10M_HD  96
#define UNIMAC_IFG_100M_HD 96
#define UNIMAC_IFG_10M_FD  96
#define UNIMAC_IFG_100M_FD 96
#define UNIMAC_IFG_1G_FD   96
#define UNIMAC_IFG_2P5G_FD 96

STATIC int
mac_uni_ipg_update(int unit, int lport)
{
    int                 fd, speed, ipg, ifg;
    TX_IPG_LENGTHr_t    tx_ipg_length; 
    IPG_HD_BKP_CNTLr_t  ipg_hd_bkp_cntl;

    SOC_IF_ERROR_RETURN(mac_uni_duplex_get(unit, lport, &fd));
    SOC_IF_ERROR_RETURN(mac_uni_speed_get(unit, lport, &speed));

    if (fd) {
        switch (speed) {
        case 10:
            ifg = UNIMAC_IFG_10M_FD;
            break;
        case 100:
            ifg = UNIMAC_IFG_100M_FD;
            break;
        case 1000:
            ifg = UNIMAC_IFG_100M_FD;
            break;
        case 2500:
            ifg = UNIMAC_IFG_100M_FD;
            break;
        default:
            return SYS_ERR;
            break;
        }
            } else {
                switch (speed) {
                case 10:
                    ifg = UNIMAC_IFG_10M_HD;
                    break;
                case 100:
                    ifg = UNIMAC_IFG_100M_HD;
                    break;
                default:
                    return SYS_ERR;
                    break;
                }
            }
        
       /*
             * Convert the ifg value from bit-times into IPG register-specific value
             */
       SOC_IF_ERROR_RETURN(mac_uni_ifg_to_ipg(unit, lport, speed, fd, ifg, &ipg));
        
       /*
            * Program the appropriate register
            */
       TX_IPG_LENGTHr_CLR(tx_ipg_length);
       TX_IPG_LENGTHr_TX_IPG_LENGTHf_SET(tx_ipg_length, ipg);
       WRITE_TX_IPG_LENGTHr(unit, lport, tx_ipg_length);

       /* Do not process any packets that have less than 8 bytes IPG */
       READ_IPG_HD_BKP_CNTLr(unit, lport, ipg_hd_bkp_cntl);
       IPG_HD_BKP_CNTLr_IPG_CONFIG_RXf_SET(ipg_hd_bkp_cntl, speed >= 1000 ? 6 : 12);
       WRITE_IPG_HD_BKP_CNTLr(unit, lport, ipg_hd_bkp_cntl);
        
       return SYS_OK;
}


static int
mac_uni_duplex_set(int unit, uint8 lport, int duplex)
{
    uint32 speed;


    COMMAND_CONFIGr_t command_config;
#if 0
    if (auto_cfg[unit][port]) {
        return SYS_OK;
    }
#endif

    READ_COMMAND_CONFIGr(unit, lport, command_config);

    speed = COMMAND_CONFIGr_ETH_SPEEDf_GET(command_config);

    if (speed >= SOC_UNIMAC_SPEED_1000) {
        /* If speed is 1000 or 2500 Mbps, duplex bit is ignored by unimac 
         * and unimac runs at full duplex mode.
         */
        return SYS_OK;
    }

    /* Bit[10]: Half duplex enable */ 
    if (duplex) {
        COMMAND_CONFIGr_HD_ENAf_SET(command_config, 0);
    } else {
        COMMAND_CONFIGr_HD_ENAf_SET(command_config, 1);
    }

    /* First put the MAC in reset */
    _mac_uni_sw_reset(unit, lport, TRUE);
    

    COMMAND_CONFIGr_SW_RESETf_SET(command_config, 1);
    WRITE_COMMAND_CONFIGr(unit, lport, command_config);

    /* Set IPG to match new duplex */
    SOC_IF_ERROR_RETURN(mac_uni_ipg_update(unit, lport));
    
    /*
        * Notify internal PHY driver of duplex change in case it is being
        * used as pass-through to an external PHY.
        */
    soc_phyctrl_notify(unit, lport, phyEventDuplex, duplex);

    SAL_DEBUGF(("mac_uni_duplex_set: unit %d port %d duplex=%s\n",unit, lport, duplex ? "Full" : "Half"));

    _mac_uni_sw_reset(unit, lport, FALSE);

    return SYS_OK;
}
static int
mac_uni_speed_set(int unit, uint8 lport, int speed)
{
    uint32 speed_select; 
    uint32 cur_speed;
    COMMAND_CONFIGr_t command_config;
    
    READ_COMMAND_CONFIGr(unit, lport, command_config);
#if 0
    if (auto_cfg[unit][port]) {
        return SYS_OK;
    }
#endif

    switch (speed) {
    case 10:
        speed_select = SOC_UNIMAC_SPEED_10;
    break;
    /* support non-standard speed in Broadreach mode */
    case 20:
    case 25:
    case 33:
    case 50:
    /* fall through to case 100 */
    case 100:
        speed_select = SOC_UNIMAC_SPEED_100;
    break;
    case 1000:
        speed_select = SOC_UNIMAC_SPEED_1000;
        break;
    case 2500:
        speed_select = SOC_UNIMAC_SPEED_2500;
        break;
    case 0:
        return (SYS_OK);              /* Support NULL PHY */            
    default:
        return (SYS_ERR_PARAMETER);
    }

    cur_speed = COMMAND_CONFIGr_ETH_SPEEDf_GET(command_config);

    COMMAND_CONFIGr_ETH_SPEEDf_SET(command_config, speed_select);
    /* First reset the MAC */
    _mac_uni_sw_reset(unit, lport, TRUE);

    COMMAND_CONFIGr_SW_RESETf_SET(command_config, 1);
    WRITE_COMMAND_CONFIGr(unit, lport, command_config);

    /*
     * Notify internal PHY driver of speed change in case it is being
     * used as pass-through to an external PHY.
     */
    soc_phyctrl_notify(unit, lport, phyEventSpeed, speed);

    /* Set IPG to match new speed */
    mac_uni_ipg_update(unit, lport);

    /* Bring the MAC out of reset */
    _mac_uni_sw_reset(unit, lport, FALSE);

    /* MAC speed switch results in a tx clock glitch to the serdes in 100fx mode.
     * reset serdes txfifo clears this condition. However this reset triggers
     * a link transition. Do not apply this reset if speed is already in 100M
     */
    if ((speed == 100) && (cur_speed != SOC_UNIMAC_SPEED_100)) {
        (void)soc_phyctrl_notify(unit, lport, phyEventTxFifoReset, 100);
    }

    return SYS_OK;
}


/*
 * Function:
 *      mac_uni_enable_get
 * Purpose:
 *      Get UniMAC enable state
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - Port number on unit.
 *      enable - (OUT) TRUE if enabled, FALSE if disabled
 * Returns:
 *      SOC_E_XXX
 */

int
mac_uni_enable_get(int unit, uint8 lport, int *enable)
{
    COMMAND_CONFIGr_t    command_config;



    SOC_IF_ERROR_RETURN
        (READ_COMMAND_CONFIGr(unit, lport, command_config));

    *enable = COMMAND_CONFIGr_RX_ENAf_GET(command_config);

    SAL_DEBUGF(("mac_uni_enable_get: unit %d port %d enable=%s\n", unit, lport, *enable ? "True" : "False"));

    return SYS_OK;
}


static int
mac_uni_loopback_set(int unit, uint8 lport, int lb)
{
    COMMAND_CONFIGr_t command_config;
    int phy_lb;

    _mac_uni_sw_reset(unit, lport, TRUE);

    READ_COMMAND_CONFIGr(unit, lport, command_config);
    if (lb) {
        COMMAND_CONFIGr_LOOP_ENAf_SET(command_config, 1);
    } else {
        COMMAND_CONFIGr_LOOP_ENAf_SET(command_config, 0);
    }

    WRITE_COMMAND_CONFIGr(unit, lport, command_config);
    if (pcm_phyctrl_phy_get_driver_name(unit, lport, 0) == NULL &&
        pcm_phyctrl_phy_get_driver_name(unit, lport, 1) != NULL &&
        lb != 0) 
    {
      /* GH2's QTCE ports need phy linkup signal to active MAC loopback */
      SOC_IF_ERROR_RETURN(soc_phyctrl_loopback_get(unit, lport, &phy_lb));
      SOC_IF_ERROR_RETURN(soc_phyctrl_loopback_set(unit, lport, TRUE, TRUE));  
      sal_usleep(1000);
      SOC_IF_ERROR_RETURN(soc_phyctrl_loopback_set(unit, lport, phy_lb, TRUE));
    }

    sal_usleep(5000);

    _mac_uni_sw_reset(unit, lport, FALSE);

    return SYS_OK;
}

static int
mac_uni_loopback_get(int unit, uint8 lport, int *lb)
{

      *lb = 0;

      return SYS_OK;
   
}

static int 
mac_uni_ability_local_get(int unit, uint8 lport, soc_port_ability_t *ability) {


   SAL_DEBUGF(("mac_uni_ability_local_get %d\n", lport));
   ability->speed_half_duplex =
       SOC_PA_SPEED_10MB | SOC_PA_SPEED_100MB;
   ability->speed_full_duplex =
       SOC_PA_SPEED_10MB | SOC_PA_SPEED_100MB | SOC_PA_SPEED_1000MB;
   if (SOC_PORT_SPEED_MAX(lport) > 1000) {
       ability->speed_full_duplex |= SOC_PA_SPEED_2500MB;
   }

   ability->interface = SOC_PA_INTF_MII | SOC_PA_INTF_GMII;
   ability->pause      = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
   ability->medium      = SOC_PA_ABILITY_NONE;
   ability->loopback  = SOC_PA_LB_MAC;
   ability->flags      = SOC_PA_ABILITY_NONE;
   ability->encap      = SOC_PA_ENCAP_IEEE;


   return SYS_OK;
}
/*
 * Function:
 *      mac_uni_interface_set
 * Purpose:
 *      Set GE MAC interface type
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - Port number on unit.
 *      pif - one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_XXX
 */

 int
mac_uni_interface_set(int unit, uint8 port, soc_port_if_t pif)
{


    switch (pif) {
    case SOC_PORT_IF_MII:
    case SOC_PORT_IF_GMII:
    case SOC_PORT_IF_SGMII:
        return SYS_OK;
    default:
        return SYS_ERR;
    }
}

/*
 * Function:
 *      mac_uni_interface_get
 * Purpose:
 *      Retrieve GE MAC interface type
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port # on unit.
 *      pif - (OUT) one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
mac_uni_interface_get(int unit, uint8 lport, soc_port_if_t *pif)
{
    *pif = SOC_PORT_IF_GMII;

    SAL_DEBUGF(("mac_uni_interface_get: unit %d port %d interface=%d\n", unit, lport, *pif));

    return SYS_OK;
}

/*
 * Function:
 *      mac_uni_control_set
 * Purpose:
 *      To configure MAC control properties.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      type - MAC control property to set.
 *      int  - New setting for MAC control.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_uni_control_set(int unit, uint8 lport, soc_mac_control_t type,
                    int value)
{
    int rv = SYS_OK;

    COMMAND_CONFIGr_t command_config, command_config_orig;

    SAL_DEBUGF(("mac_uni_control_set: unit %d port %d type=%d value=%d\n", unit, lport, type, value));

    switch(type) {
        case SOC_MAC_CONTROL_RX_SET:
        {
            SOC_IF_ERROR_RETURN(READ_COMMAND_CONFIGr(unit, lport, command_config));
            sal_memcpy(&command_config_orig, &command_config, sizeof(COMMAND_CONFIGr_t));
            COMMAND_CONFIGr_RX_ENAf_SET(command_config, (value ? 1: 0));
            if (sal_memcmp(&command_config_orig, &command_config, sizeof(COMMAND_CONFIGr_t)) != 0) {
                SOC_IF_ERROR_RETURN(WRITE_COMMAND_CONFIGr(unit, lport, command_config));
            }
            break;
        }
        case SOC_MAC_CONTROL_TX_SET:
        {
            SOC_IF_ERROR_RETURN(READ_COMMAND_CONFIGr(unit, lport, command_config));
            sal_memcpy(&command_config_orig, &command_config, sizeof(COMMAND_CONFIGr_t));
            COMMAND_CONFIGr_TX_ENAf_SET(command_config, (value ? 1: 0));
            if (sal_memcmp(&command_config_orig, &command_config, sizeof(COMMAND_CONFIGr_t)) != 0) {
                SOC_IF_ERROR_RETURN(WRITE_COMMAND_CONFIGr(unit, lport, command_config));
            }
            break;
        }
        case SOC_MAC_CONTROL_SW_RESET:
        case SOC_MAC_CONTROL_DISABLE_PHY:
            return _mac_uni_sw_reset(unit, lport, value);

        case SOC_MAC_CONTROL_PFC_TYPE:
        {
            MAC_PFC_TYPEr_t mac_pfc_type;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_TYPEr(unit, lport, mac_pfc_type));
            MAC_PFC_TYPEr_PFC_ETH_TYPEf_SET(mac_pfc_type, value);
            SOC_IF_ERROR_RETURN(WRITE_MAC_PFC_TYPEr(unit, lport, mac_pfc_type));
            break;
        }
        case SOC_MAC_CONTROL_PFC_OPCODE:
        {
            MAC_PFC_OPCODEr_t mac_pfc_opcode;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_OPCODEr(unit, lport, mac_pfc_opcode));
            MAC_PFC_OPCODEr_PFC_OPCODEf_SET(mac_pfc_opcode, value);
            SOC_IF_ERROR_RETURN(WRITE_MAC_PFC_OPCODEr(unit, lport, mac_pfc_opcode));
            break;
        }
        case SOC_MAC_CONTROL_PFC_CLASSES:
            return SYS_ERR_PARAMETER;
            break;

        case SOC_MAC_CONTROL_PFC_MAC_DA_OUI:
        {
            MAC_PFC_DA_0r_t mac_pfc_da_0;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_DA_0r(unit, lport, mac_pfc_da_0));
            mac_pfc_da_0.v[0] &= 0x00ffffff;
            mac_pfc_da_0.v[0] |= (value & 0x0000ff) << 24;
            SOC_IF_ERROR_RETURN(WRITE_MAC_PFC_DA_0r(unit, lport, mac_pfc_da_0));
            break;
        }
        case SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI:
        {
            MAC_PFC_DA_0r_t mac_pfc_da_0;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_DA_0r(unit, lport, mac_pfc_da_0));
            mac_pfc_da_0.v[0] &= 0x00ffffff;
            mac_pfc_da_0.v[0] |= (value & 0x0000ff) << 24;
            SOC_IF_ERROR_RETURN(WRITE_MAC_PFC_DA_0r(unit, lport, mac_pfc_da_0));
            break;
        }
        case SOC_MAC_CONTROL_PFC_RX_PASS:
        {
            MAC_PFC_CTRLr_t mac_pfc_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));
            MAC_PFC_CTRLr_RX_PASS_PFC_FRMf_SET(mac_pfc_ctrl, value ? 1 : 0);
            SOC_IF_ERROR_RETURN(WRITE_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));
            break;
        }
        case SOC_MAC_CONTROL_PFC_RX_ENABLE:
        {
            MAC_PFC_CTRLr_t mac_pfc_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));
            MAC_PFC_CTRLr_PFC_RX_ENBLf_SET(mac_pfc_ctrl, value ? 1 : 0);
            SOC_IF_ERROR_RETURN(WRITE_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));
            break;
        }
        case SOC_MAC_CONTROL_PFC_TX_ENABLE:
        {
            MAC_PFC_CTRLr_t mac_pfc_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));
            MAC_PFC_CTRLr_PFC_TX_ENBLf_SET(mac_pfc_ctrl, value ? 1 : 0);
            SOC_IF_ERROR_RETURN(WRITE_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));
            break;
        }
        case SOC_MAC_CONTROL_PFC_FORCE_XON:
        {
            MAC_PFC_CTRLr_t mac_pfc_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));
            MAC_PFC_CTRLr_FORCE_PFC_XONf_SET(mac_pfc_ctrl, value ? 1 : 0);
            SOC_IF_ERROR_RETURN(WRITE_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));
            break;
        }
        case SOC_MAC_CONTROL_PFC_STATS_ENABLE:
        {
            MAC_PFC_CTRLr_t mac_pfc_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));
            MAC_PFC_CTRLr_PFC_STATS_ENf_SET(mac_pfc_ctrl, value ? 1 : 0);
            SOC_IF_ERROR_RETURN(WRITE_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));
            break;
        }
        case SOC_MAC_CONTROL_PFC_REFRESH_TIME:
        {
            MAC_PFC_REFRESH_CTRLr_t mac_pfc_refresh_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_REFRESH_CTRLr(unit, lport, mac_pfc_refresh_ctrl));
            MAC_PFC_REFRESH_CTRLr_PFC_REFRESH_TIMERf_SET(mac_pfc_refresh_ctrl, value);
            SOC_IF_ERROR_RETURN(WRITE_MAC_PFC_REFRESH_CTRLr(unit, lport, mac_pfc_refresh_ctrl));
            break;
        }
        case SOC_MAC_CONTROL_PFC_XOFF_TIME:
        {
            PFC_XOFF_TIMERr_t mac_xoff_timer;
            SOC_IF_ERROR_RETURN(READ_PFC_XOFF_TIMERr(unit, lport, mac_xoff_timer));
            PFC_XOFF_TIMERr_PFC_XOFF_TIMERf_SET(mac_xoff_timer, value);
            SOC_IF_ERROR_RETURN(WRITE_PFC_XOFF_TIMERr(unit, lport, mac_xoff_timer));
            break;
        }
        case SOC_MAC_CONTROL_EEE_ENABLE:
        {
            UMAC_EEE_CTRLr_t umac_eee_ctrl;
            SOC_IF_ERROR_RETURN(READ_UMAC_EEE_CTRLr(unit, lport, umac_eee_ctrl));
            UMAC_EEE_CTRLr_EEE_ENf_SET(umac_eee_ctrl, value);
            SOC_IF_ERROR_RETURN(WRITE_UMAC_EEE_CTRLr(unit, lport, umac_eee_ctrl));
            break;
        }
        case SOC_MAC_CONTROL_EEE_TX_IDLE_TIME:
        {
            MII_EEE_DELAY_ENTRY_TIMERr_t mii_eee_delay_entry_timer;
            GMII_EEE_DELAY_ENTRY_TIMERr_t gmii_eee_delay_entry_timer;
            SOC_IF_ERROR_RETURN(READ_MII_EEE_DELAY_ENTRY_TIMERr(unit, lport, mii_eee_delay_entry_timer));
            MII_EEE_DELAY_ENTRY_TIMERr_MII_EEE_LPI_TIMERf_SET(mii_eee_delay_entry_timer, value);
            SOC_IF_ERROR_RETURN(WRITE_MII_EEE_DELAY_ENTRY_TIMERr(unit, lport, mii_eee_delay_entry_timer));
            SOC_IF_ERROR_RETURN(READ_GMII_EEE_DELAY_ENTRY_TIMERr(unit, lport, gmii_eee_delay_entry_timer));
            GMII_EEE_DELAY_ENTRY_TIMERr_GMII_EEE_LPI_TIMERf_SET(gmii_eee_delay_entry_timer, value);
            SOC_IF_ERROR_RETURN(WRITE_GMII_EEE_DELAY_ENTRY_TIMERr(unit, lport, gmii_eee_delay_entry_timer));
            break;
        }
        case SOC_MAC_CONTROL_EEE_TX_WAKE_TIME:
        {
             MII_EEE_WAKE_TIMERr_t mii_eee_wake_timer;
            GMII_EEE_WAKE_TIMERr_t gmii_eee_wake_timer;
            SOC_IF_ERROR_RETURN(READ_MII_EEE_WAKE_TIMERr(unit, lport, mii_eee_wake_timer));
            MII_EEE_WAKE_TIMERr_MII_EEE_WAKE_TIMERf_SET(mii_eee_wake_timer, value);
            SOC_IF_ERROR_RETURN(WRITE_MII_EEE_WAKE_TIMERr(unit, lport, mii_eee_wake_timer));
            SOC_IF_ERROR_RETURN(READ_GMII_EEE_WAKE_TIMERr(unit, lport, gmii_eee_wake_timer));
            GMII_EEE_WAKE_TIMERr_GMII_EEE_WAKE_TIMERf_SET(gmii_eee_wake_timer, value);
            SOC_IF_ERROR_RETURN(WRITE_GMII_EEE_WAKE_TIMERr(unit, lport, gmii_eee_wake_timer));
            break;
        }
        default:
            rv = SYS_ERR_NOT_FOUND;
            break;
    }

    return rv;
}

/*
 * Function:
 *      mac_uni_control_get
 * Purpose:
 *      To get current MAC control setting.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      type - MAC control property to set.
 *      int  - New setting for MAC control.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int 
mac_uni_control_get(int unit, uint8 lport, soc_mac_control_t type,
                    int *value)
{

    COMMAND_CONFIGr_t command_config;

    int rv;
    uint32  fval0, fval1;

    rv = SYS_OK;

    switch (type) {
        case SOC_MAC_CONTROL_RX_SET:
            SOC_IF_ERROR_RETURN(READ_COMMAND_CONFIGr(unit, lport, command_config));
            *value = COMMAND_CONFIGr_RX_ENAf_GET(command_config);
            break;
        case SOC_MAC_CONTROL_TX_SET:
            SOC_IF_ERROR_RETURN(READ_COMMAND_CONFIGr(unit, lport, command_config));                
            *value = COMMAND_CONFIGr_TX_ENAf_GET(command_config);
            break;
        case SOC_MAC_CONTROL_TIMESTAMP_TRANSMIT:
        {
             TS_STATUS_CNTRLr_t ts_status_cntrl;
             TX_TS_SEQ_IDr_t tx_ts_seq_id;
             TX_TS_DATAr_t tx_ts_data;
             SOC_IF_ERROR_RETURN(READ_TS_STATUS_CNTRLr(unit, lport, ts_status_cntrl));
             if (TS_STATUS_CNTRLr_TX_TS_FIFO_EMPTYf_GET(ts_status_cntrl)) {
                 return SYS_ERR;
             }
             SOC_IF_ERROR_RETURN(READ_TX_TS_SEQ_IDr(unit, lport, tx_ts_seq_id));
             if (TX_TS_SEQ_IDr_TSTS_VALIDf_GET(tx_ts_seq_id) == 0) {
                 return SYS_ERR;
             };
             SOC_IF_ERROR_RETURN(READ_TX_TS_DATAr(unit, lport, tx_ts_data));
             *value = (int) TX_TS_DATAr_TX_TS_DATAf_GET(tx_ts_data);
             break;
        }
        case SOC_MAC_CONTROL_PFC_TYPE:
        {
            MAC_PFC_TYPEr_t mac_pfc_type;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_TYPEr(unit, lport, mac_pfc_type));
            *value = MAC_PFC_TYPEr_PFC_ETH_TYPEf_GET(mac_pfc_type);
            break;
        }
        case SOC_MAC_CONTROL_PFC_OPCODE:
        {
            MAC_PFC_OPCODEr_t mac_pfc_opcode;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_OPCODEr(unit, lport, mac_pfc_opcode));
            *value = MAC_PFC_OPCODEr_PFC_OPCODEf_GET(mac_pfc_opcode);
            rv  = SYS_OK;
            break;
        }
        case SOC_MAC_CONTROL_PFC_CLASSES:
            rv =  SYS_ERR_PARAMETER;
            break;

        case SOC_MAC_CONTROL_PFC_MAC_DA_OUI:
        {
            MAC_PFC_DA_0r_t mac_pfc_da_0;
            MAC_PFC_DA_1r_t mac_pfc_da_1;

            SOC_IF_ERROR_RETURN(READ_MAC_PFC_DA_0r(unit, lport, mac_pfc_da_0));
            fval0 = MAC_PFC_DA_0r_PFC_MACDA_0f_GET(mac_pfc_da_0);
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_DA_1r(unit, lport, mac_pfc_da_1));
            fval1 = MAC_PFC_DA_1r_PFC_MACDA_1f_GET(mac_pfc_da_1);
            *value = (fval0 >> 24) | (fval1 << 8);
            break;
        }

        case SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI:
        {
            MAC_PFC_DA_0r_t mac_pfc_da_0;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_DA_0r(unit, lport, mac_pfc_da_0));
            *value = MAC_PFC_DA_0r_PFC_MACDA_0f_GET(mac_pfc_da_0) & 0x00ffffff;
            break;
        }

        case SOC_MAC_CONTROL_PFC_RX_PASS:
        {
            MAC_PFC_CTRLr_t mac_pfc_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));     
            *value = MAC_PFC_CTRLr_RX_PASS_PFC_FRMf_GET(mac_pfc_ctrl);
            break;
        }
        case SOC_MAC_CONTROL_PFC_RX_ENABLE:
        {
            MAC_PFC_CTRLr_t mac_pfc_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));     
            *value = MAC_PFC_CTRLr_PFC_RX_ENBLf_GET(mac_pfc_ctrl);
            break;
        }
        case SOC_MAC_CONTROL_PFC_TX_ENABLE:
        {
            MAC_PFC_CTRLr_t mac_pfc_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));     
            *value = MAC_PFC_CTRLr_PFC_TX_ENBLf_GET(mac_pfc_ctrl);
            break;
        }
        case SOC_MAC_CONTROL_PFC_FORCE_XON:
        {
            MAC_PFC_CTRLr_t mac_pfc_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));     
            *value = MAC_PFC_CTRLr_FORCE_PFC_XONf_GET(mac_pfc_ctrl);
            break;
        }
        case SOC_MAC_CONTROL_PFC_STATS_ENABLE:
        {
            MAC_PFC_CTRLr_t mac_pfc_ctrl;
            SOC_IF_ERROR_RETURN(READ_MAC_PFC_CTRLr(unit, lport, mac_pfc_ctrl));     
            *value = MAC_PFC_CTRLr_PFC_STATS_ENf_GET(mac_pfc_ctrl);
            break;
        }
        case SOC_MAC_CONTROL_PFC_REFRESH_TIME:
        {
            PFC_XOFF_TIMERr_t pfc_xoff_timer;
            SOC_IF_ERROR_RETURN(READ_PFC_XOFF_TIMERr(unit, lport, pfc_xoff_timer));
            *value = PFC_XOFF_TIMERr_PFC_XOFF_TIMERf_GET(pfc_xoff_timer);
            break;
        }
        case SOC_MAC_CONTROL_PFC_XOFF_TIME:
        {
            PFC_XOFF_TIMERr_t pfc_xoff_timer;
            SOC_IF_ERROR_RETURN(READ_PFC_XOFF_TIMERr(unit, lport, pfc_xoff_timer));
            *value = PFC_XOFF_TIMERr_PFC_XOFF_TIMERf_GET(pfc_xoff_timer);
        }
        break; 
        case SOC_MAC_CONTROL_EEE_ENABLE:
        {
            UMAC_EEE_CTRLr_t umac_eee_ctrl;
            SOC_IF_ERROR_RETURN(READ_UMAC_EEE_CTRLr(unit, lport, umac_eee_ctrl));
            *value = UMAC_EEE_CTRLr_EEE_ENf_GET(umac_eee_ctrl);
            break; 
        }

        case SOC_MAC_CONTROL_EEE_TX_IDLE_TIME:
        {
            MII_EEE_DELAY_ENTRY_TIMERr_t mii_eee_delay_entry_timer;
            SOC_IF_ERROR_RETURN(READ_MII_EEE_DELAY_ENTRY_TIMERr(unit, lport, mii_eee_delay_entry_timer));
            *value = MII_EEE_DELAY_ENTRY_TIMERr_MII_EEE_LPI_TIMERf_GET(mii_eee_delay_entry_timer);
            break; 
        }

        case SOC_MAC_CONTROL_EEE_TX_WAKE_TIME:
        {
            MII_EEE_WAKE_TIMERr_t mii_eee_wake_timer;
            SOC_IF_ERROR_RETURN(READ_MII_EEE_WAKE_TIMERr(unit, lport, mii_eee_wake_timer));
            *value = MII_EEE_WAKE_TIMERr_MII_EEE_WAKE_TIMERf_GET(mii_eee_wake_timer);
            break;
        }
        case SOC_MAC_CONTROL_FAULT_LOCAL_STATUS: 
        case SOC_MAC_CONTROL_FAULT_REMOTE_STATUS: 
             *value = 0;
             break;
        default:
             rv = SYS_ERR_PARAMETER;
             break;
   }
   SAL_DEBUGF(("mac_uni_control_get: unit %d port %s type=%d value=%d rv=%d\n", unit, lport, type, *value, rv));
   return rv;
}

/*
 * Function:
 *      mac_uni_pause_addr_set
 * Purpose:
 *      Set GE MAC source address for transmitted PAUSE frame
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port # on unit.
 *      pause_mac - MAC address used for pause transmission.
 * Returns:
 *      SOC_E_XXX
 */

STATIC  int
mac_uni_pause_addr_set(int unit, uint8 lport, sal_mac_addr_t pause_mac)
{
    MAC_0r_t mac_0;
    MAC_1r_t mac_1;

    mac_0.v[0] = pause_mac[0] << 24 |
            pause_mac[1] << 16 |
            pause_mac[2] << 8 |
            pause_mac[3] << 0;
    mac_1.v[0] = pause_mac[4] << 8 |
            pause_mac[5];

    SOC_IF_ERROR_RETURN(WRITE_MAC_0r(unit, lport, mac_0));
    SOC_IF_ERROR_RETURN(WRITE_MAC_1r(unit, lport, mac_1));
    
    SAL_DEBUGF(("mac_uni_pause_addr_set: unit %d port %d "
                             "MAC=<%02x:%02x:%02x:%02x:%02x:%02x>\n",
                 unit, lport,
                 pause_mac[0], pause_mac[1], pause_mac[2],
                 pause_mac[3], pause_mac[4], pause_mac[5]));
    return SYS_OK;
}

/*
 * Function:
 *      mac_uni_pause_addr_get
 * Purpose:
 *      Return current GE MAC source address for transmitted PAUSE frames
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port # on unit.
 *      pause_mac - (OUT) MAC address used for pause transmission.
 * Returns:
 *      SOC_E_XXX
 */

STATIC  int
mac_uni_pause_addr_get(int unit, uint8 lport, sal_mac_addr_t pause_mac)
{
    MAC_0r_t mac_0;
    MAC_1r_t mac_1;

    SOC_IF_ERROR_RETURN(READ_MAC_0r(unit, lport, mac_0));
    SOC_IF_ERROR_RETURN(READ_MAC_1r(unit, lport, mac_1));

    pause_mac[0] = (uint8)(mac_0.v[0] >> 24);
    pause_mac[1] = (uint8)(mac_0.v[0] >> 16);
    pause_mac[2] = (uint8)(mac_0.v[0] >> 8);
    pause_mac[3] = (uint8)(mac_0.v[0] >> 0);
    pause_mac[4] = (uint8)(mac_1.v[0] >> 8);
    pause_mac[5] = (uint8)(mac_1.v[0] >> 0);

    SAL_DEBUGF(("mac_uni_pause_addr_get: unit %d port %d "
                             "MAC=<%02x:%02x:%02x:%02x:%02x:%02x>\n",
                 unit, lport,
                 pause_mac[0], pause_mac[1], pause_mac[2],
                 pause_mac[3], pause_mac[4], pause_mac[5]));
    return SYS_OK;
}

/*
 * Function:
 *      mac_uni_encap_set
 * Purpose:
 *      Set the port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - encapsulation mode
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_uni_encap_set(int unit, soc_port_t port, int mode)
{

    if (mode == SOC_ENCAP_IEEE) {
        return SYS_OK;
    }
    return SYS_OK;
}

/*
 * Function:
 *      mac_uni_encap_get
 * Purpose:
 *      Get the port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (INT) encapsulation mode
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_uni_encap_get(int unit, soc_port_t port, int *mode)
{
    *mode = SOC_ENCAP_IEEE;

    return SYS_OK;
}


/*
 * Function:
 *      mac_uni_ifg_set
 * Description:
 *      Sets the new ifg (Inter-frame gap) value
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      ifg    - Inter-frame gap in bit-times
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      The function makes sure the IFG value makes sense and updates the
 *      IPG register in case the speed/duplex match the current settings
 */
STATIC int
mac_uni_ifg_set(int unit, uint8 lport, int speed,
                soc_port_duplex_t  duplex, int ifg)
{
    int         cur_speed = 0;
    int         cur_duplex;
    int         ipg, real_ifg;
    TX_IPG_LENGTHr_t tx_ipg_length;

    SAL_DEBUGF(("mac_uni_ifg_set: unit %d port %d speed=%dMb duplex=%s "
                             "ifg=%d\n",
                 unit, lport,
                 speed, duplex == SOC_PORT_DUPLEX_FULL ? "Full" : "Half",
                 ifg));

    /* Get IPG, check range of IFG */
    SOC_IF_ERROR_RETURN(mac_uni_ifg_to_ipg(unit, lport, speed, duplex,
                                               ifg, &ipg));
    SOC_IF_ERROR_RETURN(mac_uni_ipg_to_ifg(unit, lport, speed, duplex,
                                          ipg, &real_ifg));

    SOC_IF_ERROR_RETURN(mac_uni_duplex_get(unit, lport, &cur_duplex));
    SOC_IF_ERROR_RETURN(mac_uni_speed_get(unit, lport, &cur_speed));

    if (cur_speed == speed &&
        cur_duplex == (duplex == SOC_PORT_DUPLEX_FULL ? TRUE : FALSE)) {
        /* First put the MAC in reset */
        SOC_IF_ERROR_RETURN
            (mac_uni_control_set(unit, lport, SOC_MAC_CONTROL_SW_RESET, TRUE));

        SOC_IF_ERROR_RETURN(READ_TX_IPG_LENGTHr(unit, lport, tx_ipg_length));
        TX_IPG_LENGTHr_TX_IPG_LENGTHf_SET(tx_ipg_length, ipg);
        SOC_IF_ERROR_RETURN(WRITE_TX_IPG_LENGTHr(unit, lport, tx_ipg_length));

        /* Bring the MAC out of reset */
        SOC_IF_ERROR_RETURN
            (mac_uni_control_set(unit, lport, SOC_MAC_CONTROL_SW_RESET, FALSE));
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_uni_ifg_get
 * Description:
 *      Gets the ifg (Inter-frame gap) value for a specific speed/duplex
 *      combination
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      ifg    - Inter-frame gap in bit-times
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      The function returns the REAL ifg value that will be (or is currently)
 *      used by the chip, which might be different from the value initially
 *      set by the ifg_set() call. The reason for that is that only certain
 *      values are allowed.
 */
STATIC int
mac_uni_ifg_get(int unit, uint8 lport, int speed,
                soc_port_duplex_t  duplex, int *ifg)
{
    TX_IPG_LENGTHr_t tx_ipg_length;
    int         ipg;

    

    SOC_IF_ERROR_RETURN(READ_TX_IPG_LENGTHr(unit, lport, tx_ipg_length));
    ipg = TX_IPG_LENGTHr_TX_IPG_LENGTHf_GET(tx_ipg_length);
    SOC_IF_ERROR_RETURN(mac_uni_ipg_to_ifg(unit, lport, speed, duplex, ipg, ifg));


    return SYS_OK;
}


/*
 * Function:
 *      mac_uni_frame_max_set
 * Description:
 *      Set the maximum receive frame size for the GE port
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      size - Maximum frame size in bytes
 * Return Value:
 *      BCM_E_XXX
 */

STATIC int
mac_uni_frame_max_set(int unit, uint8 lport, int size)
{
    COMMAND_CONFIGr_t command_config;
    FRM_LENGTHr_t frm_length;
    int rx_ena = 0;
    int speed = 0;
    SAL_DEBUGF(("mac_uni_frame_max_set: unit %d port %d size=%d\n", unit, lport, size));

    SOC_IF_ERROR_RETURN(READ_COMMAND_CONFIGr(unit, lport, command_config));

    /* If Rx is enabled then disable RX */
    if (COMMAND_CONFIGr_RX_ENAf_GET(command_config)) {
        rx_ena = 1;
        /* Disable RX */
        COMMAND_CONFIGr_RX_ENAf_SET(command_config, 0);  
        SOC_IF_ERROR_RETURN
            (WRITE_COMMAND_CONFIGr(unit, lport, command_config));

        /* Wait for maximum frame receiption time(for 16K) based on speed */
        SOC_IF_ERROR_RETURN(mac_uni_speed_get(unit, lport, &speed));
        switch (speed) {
        case 2500:
            sal_usleep(55);
            break;
        case 1000:
            sal_usleep(131);
            break;
        case 100:
            sal_usleep(1310);
            break;
        case 10:
            sal_usleep(13100);
            break;
        default:
            break;
        }
    }
    FRM_LENGTHr_CLR(frm_length);
    FRM_LENGTHr_MAXFRf_SET(frm_length, size);
    SOC_IF_ERROR_RETURN(WRITE_FRM_LENGTHr(unit, lport, frm_length));

    /* if Rx was enabled before, restore it */
    if (rx_ena) {
        /* Enable RX */
        COMMAND_CONFIGr_RX_ENAf_SET(command_config, 1);  
        SOC_IF_ERROR_RETURN
            (WRITE_COMMAND_CONFIGr(unit, lport, command_config));
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_uni_frame_max_get
 * Description:
 *      Set the maximum receive frame size for the GE port
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      size - Maximum frame size in bytes
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      Depending on chip or port type the actual maximum receive frame size
 *      might be slightly higher.
 */
STATIC int
mac_uni_frame_max_get(int unit, uint8 lport, int *size)
{
    FRM_LENGTHr_t  frame_length;
 
    SOC_IF_ERROR_RETURN(READ_FRM_LENGTHr(unit, lport, frame_length));

    *size = (int) FRM_LENGTHr_MAXFRf_GET(frame_length);

    SAL_DEBUGF(("mac_uni_frame_max_get: unit %d port %s size=%d\n", unit, lport,*size));
    return SYS_OK;
}
/*
 * Function:
 *      mac_uni_ability_get
 * Purpose:
 *      Return the UniMAC abilities
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      port - StrataSwitch Port # on unit.
 *      mode - (OUT) Mask of MAC abilities returned.
 * Returns:
 *      SOC_E_NONE
 */

int
mac_uni_ability_get(int unit, uint8 lport, soc_port_mode_t *mode)
{
   *mode = (SOC_PM_10MB | SOC_PM_100MB | SOC_PM_1000MB_FD |
                 SOC_PM_2500MB_FD |  SOC_PM_MII | SOC_PM_GMII |
                 SOC_PM_LB_MAC | SOC_PM_PAUSE);


    SAL_DEBUGF(("mac_uni_ability_get: unit %d port %d mode=0x%x\n",
                 unit, lport, *mode));
    return SYS_OK;
}



mac_driver_t soc_mac_uni = {
    .drv_name = "UniMac Driver",
    .md_init  = mac_uni_init,
    .md_enable_set = mac_uni_enable_set, 
    .md_enable_get = mac_uni_enable_get,
    .md_duplex_set = mac_uni_duplex_set,
    .md_duplex_get = mac_uni_duplex_get,
    .md_speed_set = mac_uni_speed_set,
    .md_speed_get = mac_uni_speed_get,
    .md_pause_set = mac_uni_pause_set,
    .md_pause_get = mac_uni_pause_get,
    .md_lb_set = mac_uni_loopback_set,
    .md_pause_addr_set = mac_uni_pause_addr_set,
    .md_pause_addr_get = mac_uni_pause_addr_get,
    .md_lb_get = mac_uni_loopback_get,
    .md_ability_local_get =  mac_uni_ability_local_get,
    .md_ability_get =  mac_uni_ability_get,
    .md_interface_set = mac_uni_interface_set,
    .md_interface_get = mac_uni_interface_get,
    .md_frame_max_set =    mac_uni_frame_max_set,             
    .md_frame_max_get =    mac_uni_frame_max_get,             
    .md_ifg_set =    mac_uni_ifg_set,                
    .md_ifg_get =     mac_uni_ifg_get,                
    .md_encap_set = mac_uni_encap_set,                
    .md_encap_get =    mac_uni_encap_get,                
    .md_control_set = mac_uni_control_set,            
    .md_control_get = mac_uni_control_get,            
};


