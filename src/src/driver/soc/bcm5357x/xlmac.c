/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
 
#include "system.h"
#include "utils/system.h"
#undef SOC_IF_ERROR_RETURN 
#include <soc/error.h>
#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>


/* Change the port name to the lport# */
#define SOC_PORT_NAME(unit, port)       (lport)

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>

//#define BSL_LS_SOC_XLMAC_DEBUG    
//#define UM_READBACK_DEBUG

#if defined(LOG_VERBOSE) && defined(BSL_LS_SOC_XLMAC_DEBUG)
/* Outout the debug message with LOG_VERBOSE() */
static char *mac_cl_encap_mode[] = SOC_ENCAP_MODE_NAMES_INITIALIZER;
//static char *mac_cl_port_if_names[] = SOC_PORT_IF_NAMES_INITIALIZER;
#else
#undef LOG_VERBOSE
#define LOG_VERBOSE(ls_, stuff_)        
#endif


#define UM_PHY_NOTIFY  1
#if defined(_BCM95343X_)
#define bcm5357x_xlport_pport_to_index_in_block     bcm5343x_xlport_pport_to_index_in_block
#endif

//#define CFG_TIMESTAMP_MAC_DELAY 1
#ifdef CFG_TIMESTAMP_MAC_DELAY
/* define default spn_TIMESTAMP_ADJUST value if not "0" */ 
#define SPN_TIMESTAMP_ADJUST    0

/* 250Mhz TS PLL implies 4ns resolution */
#define SOC_TIMESYNC_PLL_CLOCK_NS(unit)     (1/250 * 1000)
#define XLMAC_TIMESTAMP_ADJUST__TS_OSTS_ADJUST__MASK         ((1<<9)-1)
#endif

/*
 * Forward Declarations
 */
mac_driver_t soc_mac_xl;
    

#define XLMAC_RUNT_THRESHOLD_IEEE  0x40   /* Runt threshold value for IEEE ports */
#define XLMAC_RUNT_THRESHOLD_HG1   0x48   /* Runt threshold value for HG1 ports */
#define XLMAC_RUNT_THRESHOLD_HG2   0x4c   /* Runt threshold value for HG2 ports */

/*
 * XLMAC Register field definitions.
 */

#define JUMBO_MAXSZ              0x3fe8 /* Max legal value (per regsfile) */


#define SOC_XLMAC_SPEED_10     0x0
#define SOC_XLMAC_SPEED_100    0x1
#define SOC_XLMAC_SPEED_1000   0x2
#define SOC_XLMAC_SPEED_2500   0x3
#define SOC_XLMAC_SPEED_10000  0x4

/* Transmit CRC Modes */
#define XLMAC_CRC_APPEND        0x0
#define XLMAC_CRC_KEEP          0x1
#define XLMAC_CRC_REPLACE       0x2
#define XLMAC_CRC_PER_PKT_MODE  0x3

#define FD_XE_IPG   96
#define FD_HG_IPG   64
#define FD_HG2_IPG  96

#define _SHR_E_SUCCESS(rv)              ((rv) >= 0)
#define _SHR_E_FAILURE(rv)              ((rv) < 0)
//#define SOC_SUCCESS(rv)         _SHR_E_SUCCESS(rv)
#define SOC_E_MEMORY            SYS_ERR_OUT_OF_RESOURCE

struct {
    int speed;
    uint32 clock_rate;
}_mac_xl_clock_rate[] = {
    { 40000, 312 },
    { 20000, 156 },
    { 10000, 78  },
    { 5000,  78  },
    { 2500,  312 },
    { 1000,  125 },
    { 0,     25  },
};

#ifdef CFG_TIMESTAMP_MAC_DELAY
static int
_mac_xl_timestamp_delay_set(int unit, uint8 lport, int speed);
#endif

static int
mac_xl_speed_get(int unit, uint8 lport, int *speed);
static int
mac_xl_encap_get(int unit, uint8 lport, int *mode);

#if defined(_BCM5357X_)
/*
 * Function:
 *      soc_gh2_64q_port_check
 * Purpose:
 *      Check if the port(logical) is capable of configured up to 64 COSQ.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      port - logical port.
 *      is_64q (OUT) - with 64 COSQ or not
 * Returns:
 *      BCM_E_XXX
 */
int soc_gh2_64q_port_check(int unit, uint8 lport, int *is_64q_port)
{
    int phy_port;
    int mmu_port;

    *is_64q_port = 0;
    phy_port = SOC_PORT_L2P_MAPPING(lport);
    mmu_port = SOC_PORT_P2M_MAPPING(phy_port);
    if ((mmu_port >= SOC_GH2_64Q_MMU_PORT_IDX_MIN) &&
        (mmu_port <= SOC_GH2_64Q_MMU_PORT_IDX_MAX)) {
        *is_64q_port = 1;
    } else {
        *is_64q_port = 0;
    }
    return SOC_E_NONE;
}

/*
 * For calculate GH2 MMU_MAX/MIN_BUCKET_QLAYER table index.
 *    - mmu port 0~57 : 8 entries
 *    - mmu port 58~65 : 64 entries
 *  (IN) p : logical port
 *  (IN) q : cosq
 *  (OUT) idx : the entry index of MMU_MAX/MIN_BUCKET_QLAYER table
 *
 *  port/cos to MEM idx mapping:    if(port <=57) idx = {port, cos[2:0]}    else          idx = {(port-58), cos[5:0]}+ 58*8
 *  Hardware name: MMU_MAX_BUCKET_QLAYER
 *  Table min index:  0.  Max index:  975
 */
int
soc_gh2_mmu_bucket_qlayer_index(int unit, int p, int q, int *idx)
{
    int is_64q;
    int phy_port;
    int mmu_port;

    is_64q = 0;
    phy_port = SOC_PORT_L2P_MAPPING(p);
    mmu_port = SOC_PORT_P2M_MAPPING(phy_port);

    SOC_IF_ERROR_RETURN(soc_gh2_64q_port_check(unit, p, &is_64q));


    if (!is_64q) {
        *idx = (mmu_port * SOC_GH2_LEGACY_QUEUE_NUM) + q;
    }  else {
        *idx = (SOC_GH2_64Q_MMU_PORT_IDX_MIN * SOC_GH2_LEGACY_QUEUE_NUM) + \
               ((mmu_port-SOC_GH2_64Q_MMU_PORT_IDX_MIN) * \
                 SOC_GH2_QLAYER_COSQ_PER_PORT_NUM) + \
               q;
    }
    return SOC_E_NONE;
}

/*
 * For calculate GH2 MMU_MAX/MIN_BUCKET_QGROUP table index.
 *    - mmu port 0~57 : not available
 *    - mmu port 58~65 : 8 entries
 *  (IN) p : logical port
 *  (IN) g : queue group id
 *  (OUT) idx : the entry index of MMU_MAX/MIN_BUCKET_QGROUP table
 */
int
soc_gh2_mmu_bucket_qgroup_index(int unit, int p, int g, int *idx)
{
    int is_64q = 0;
    int phy_port;
    int mmu_port;

    SOC_IF_ERROR_RETURN(soc_gh2_64q_port_check(unit, p, &is_64q));
    if (!is_64q) {
        /* MMU port 0~57 do not have QGROUP tables */
        return SOC_E_PARAM;
    }

    phy_port = SOC_PORT_L2P_MAPPING(p);
    mmu_port = SOC_PORT_P2M_MAPPING(phy_port);
    *idx = ((mmu_port - SOC_GH2_64Q_MMU_PORT_IDX_MIN) * \
             SOC_GH2_QGROUP_PER_PORT_NUM) + g;

    return SOC_E_NONE;
}
#endif//if defined(_BCM5357X_)

void
_mac_xl_speed_to_clock_rate(int unit, uint8 lport, int speed,
                            uint32 *clock_rate)
{
    int idx;

    for (idx = 0;
         idx < sizeof(_mac_xl_clock_rate) / sizeof(_mac_xl_clock_rate[0]);
         idx++) {
        if (speed >=_mac_xl_clock_rate[idx].speed) {
            *clock_rate = _mac_xl_clock_rate[idx].clock_rate;
            return;
        }
    }
    *clock_rate = 0;
}

/* Forwards */
static int 
soc_port_credit_reset(int unit, uint8 lport)
{
    int phy_port;
    int bindex;
    PGW_XL_TXFIFO_CTRLr_t   pgw_xl_txfifo_ctrl;
    XLPORT_ENABLE_REGr_t    xlport_enable_reg;
    EGR_PORT_CREDIT_RESETm_t    egr_port_credit_reset;
    
    /* Should only be called by XLMAC/CLMAC driver */
    //if (!IS_XL_PORT(lport) && !IS_CL_PORT(unit, port)) {
    if (!IS_XL_PORT(lport)) {
        return SYS_OK;
    }
    
    phy_port = SOC_PORT_L2P_MAPPING(lport);
    bindex = bcm5357x_xlport_pport_to_index_in_block[phy_port];
    
    SOC_IF_ERROR_RETURN(
        READ_PGW_XL_TXFIFO_CTRLr(unit, lport, pgw_xl_txfifo_ctrl) );
    SOC_IF_ERROR_RETURN(
        READ_XLPORT_ENABLE_REGr(unit, lport, xlport_enable_reg) );
    
    if(bindex == 0)
        XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable_reg,0);
    else if(bindex == 1)
        XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable_reg,0);
    else if(bindex == 2)
        XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable_reg,0);
    else if(bindex == 3)
        XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable_reg,0);
    
    SOC_IF_ERROR_RETURN(
        WRITE_XLPORT_ENABLE_REGr(unit, lport, xlport_enable_reg) );
    
    EGR_PORT_CREDIT_RESETm_CLR(egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    /* To clear the port credit(per physical port) */
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_PORT_CREDIT_RESETm(unit, SOC_PORT_L2P_MAPPING(lport), egr_port_credit_reset) );
        
    PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 1);
    PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_PGW_XL_TXFIFO_CTRLr(unit, lport, pgw_xl_txfifo_ctrl) );
    
    sal_usleep(1000);

    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    /* To clear the port credit(per physical port) */
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_PORT_CREDIT_RESETm(unit, SOC_PORT_L2P_MAPPING(lport), egr_port_credit_reset) );
    
    
    PGW_XL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 0);
    PGW_XL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(pgw_xl_txfifo_ctrl, 0);
    SOC_IF_ERROR_RETURN(
        WRITE_PGW_XL_TXFIFO_CTRLr(unit, lport, pgw_xl_txfifo_ctrl) );
        
    if(bindex == 0)
        XLPORT_ENABLE_REGr_PORT0f_SET(xlport_enable_reg,1);
    else if(bindex == 1)
        XLPORT_ENABLE_REGr_PORT1f_SET(xlport_enable_reg,1);
    else if(bindex == 2)
        XLPORT_ENABLE_REGr_PORT2f_SET(xlport_enable_reg,1);
    else if(bindex == 3)
        XLPORT_ENABLE_REGr_PORT3f_SET(xlport_enable_reg,1);
    SOC_IF_ERROR_RETURN(
        WRITE_XLPORT_ENABLE_REGr(unit, lport, xlport_enable_reg) );

    return SYS_OK;
}

#if defined(_BCM95343X_)
static int
_soc_egress_metering_thaw(int unit, uint8 lport, void *setting)
{
    int       rv;
    int i;
    uint32 *buffer = setting;
    int count;
    int err_flag = 0;
    MAXBUCKETCONFIGr_t      maxbucketconfig;
    EGRMETERINGCONFIGr_t    egrmeteringconfig;

    if (setting == NULL) {
        return SOC_E_NONE;
    }

    rv = SOC_E_NONE;
    /* restore the original configuration : MAXBUCKETCONFIGr */
    count = 0;
    for (i = 0; i < MAXBUCKETCONFIG_NUM; i++) {
        MAXBUCKETCONFIGr_SET(maxbucketconfig, buffer[count]);
        rv = WRITE_MAXBUCKETCONFIGr(unit, lport, i, maxbucketconfig);
        if (SOC_FAILURE(rv)) {
            err_flag = 1;
            break;
        }
        count ++;
    }
    if (!err_flag) {
        /* restore the original configuration : EGRMETERINGCONFIGr */
        EGRMETERINGCONFIGr_SET(egrmeteringconfig, buffer[count]);
        rv = WRITE_EGRMETERINGCONFIGr(unit, lport, egrmeteringconfig);
    }
    sal_free(setting);
    
    return rv;
}
#endif//if defined(_BCM95343X_)

#if defined(_BCM5357X_)
static int
_soc_egress_metering_thaw(int unit, uint8 lport, void *setting)
{
    int       rv;
    int i;
    uint32 *buffer = setting;
    int err_flag = 0;
    int count;
    int qlayer_entry_count = 0;
    int qgroup_entry_count = 0;
    int memidx;
    int is_64q_port;
    MMU_MAX_BUCKET_QLAYERm_t    mmu_max_bucket_qlayer;
    MMU_MAX_BUCKET_QGROUPm_t    mmu_max_bucket_qgroup;
    EGRMETERINGCONFIGr_t    egrmeteringconfig;

    rv = soc_gh2_64q_port_check(unit, lport, &is_64q_port);
    if (SOC_FAILURE(rv)) {
        sal_free(setting);
        return rv;
    }
    
    if (is_64q_port) {
        qlayer_entry_count = SOC_GH2_QLAYER_COSQ_PER_PORT_NUM;
        qgroup_entry_count = SOC_GH2_QGROUP_PER_PORT_NUM;
    } else {
        qlayer_entry_count = SOC_GH2_LEGACY_QUEUE_NUM;
        qgroup_entry_count = 0;
    }
    
    /* buffer required (X*2+Y*2+1) for store original setting.
     *   MMU port 0~57: X=8, Y=0
     *   MMU port 58-65 : X=64, Y=8
     * MMU_MAX_BUCKET_QLAYERm : 
     *   X entries for a port, two fields (REFRESHf, THD_SELf)stored
     * MMU_MAX_BUCKET_QGROUPm : Y entries for a port
     *   Y entries for a port, two fields (REFRESHf, THD_SELf)stored
     * EGRMETERINGCONFIG : 1 register value for a port
     */
     
    /* restore the original configuration : MMU_MAX_BUCKET_QLAYERm */
    count = 0;
    rv = soc_gh2_mmu_bucket_qlayer_index(unit, lport, 0, &memidx);
    if (SOC_FAILURE(rv)) {
        err_flag = 1;
    } else {
        for (i = memidx; i < qlayer_entry_count; i++) {
            rv = READ_MMU_MAX_BUCKET_QLAYERm(unit, i, mmu_max_bucket_qlayer);
            if (SOC_FAILURE(rv)) {
                err_flag = 1;
                break;
            }
            MMU_MAX_BUCKET_QLAYERm_REFRESHf_SET(mmu_max_bucket_qlayer, buffer[count]);
            count ++;
            MMU_MAX_BUCKET_QLAYERm_THD_SELf_SET(mmu_max_bucket_qlayer, buffer[count]);
            count ++;
            rv = WRITE_MMU_MAX_BUCKET_QLAYERm(unit, i, mmu_max_bucket_qlayer);
            if (SOC_FAILURE(rv)) {
                err_flag = 1;
                break;
            }
        }
    }

    /* restore the original configuration : MMU_MAX_BUCKET_QGROUPm */
    if (!err_flag) {
        if (qgroup_entry_count) {
            rv = soc_gh2_mmu_bucket_qgroup_index(unit, lport, 0, 
                                                 &memidx);
            if (SOC_FAILURE(rv)) {
                err_flag = 1;
            } else {
                for (i = memidx; i < qgroup_entry_count; i++) {
                    rv = READ_MMU_MAX_BUCKET_QGROUPm(unit, i, mmu_max_bucket_qgroup);
                    if (SOC_FAILURE(rv)) {
                        err_flag = 1;
                        break;
                    }

                    MMU_MAX_BUCKET_QGROUPm_REFRESHf_SET(mmu_max_bucket_qgroup, buffer[count]);
                    count ++;
                    MMU_MAX_BUCKET_QGROUPm_THD_SELf_SET(mmu_max_bucket_qgroup, buffer[count]);
                    count ++;
                    rv = WRITE_MMU_MAX_BUCKET_QGROUPm(unit, i, mmu_max_bucket_qgroup);
                    if (SOC_FAILURE(rv)) {
                        err_flag = 1;
                        break;
                    }
                }
            }
        }
    }

    if (!err_flag) {
        /* restore the original configuration : EGRMETERINGCONFIGr */
        EGRMETERINGCONFIGr_SET(egrmeteringconfig, buffer[count]);
        rv = WRITE_EGRMETERINGCONFIGr(unit, lport, egrmeteringconfig);
    }
    sal_free(setting);    

    return rv;
}
#endif//if defined(_BCM5357X_)

#if defined(_BCM95343X_)
/*
 * Function:   soc_reg_egress_cell_count_get
 * Purpose:    Retrieves the number of egress cells for a <port, cos> pair.
 * Parameters:
 *       unit  - (IN) SOC unit number.
 *       port  - (IN) Port number.
 *       cos   - (IN) COS queue.
 *       data  - (OUT) Cell count.
 * Returns:
 *       SOC_E_XXX
 */
static int
soc_reg_egress_cell_count_get(int unit, uint8 lport, int cos, uint32 *data)
{
    COSLCCOUNTr_t coslccount;
    
    SOC_IF_ERROR_RETURN(READ_COSLCCOUNTr(unit, lport, cos, coslccount) );
    *data = COSLCCOUNTr_LCCOUNTf_GET(coslccount);
            
    return SYS_OK;
}
#endif//if defined(_BCM95343X_)

/*
 * Function:
 *      soc_egress_cell_count
 * Purpose:
 *      Return the approximate number of cells of packets pending
 *      in the MMU destined for a specified egress.
 */
static int
soc_egress_cell_count(int unit, uint8 lport, uint32 *count)
{
    int                 cos;
#if defined(_BCM95343X_)
    uint32              val;
#endif
#if defined(_BCM5357X_)
    COSLCCOUNTr_t       coslccount;
    COSLCCOUNT_QGROUPr_t    coslccount_qgroup;
#endif
    
    *count = 0;
    
#if defined(_BCM95343X_)
    for (cos = 0; cos < COSLCCOUNT_NUM; cos++) {
        SOC_IF_ERROR_RETURN(soc_reg_egress_cell_count_get(unit, lport, cos, &val));
        *count += val;
    }    
#endif

#if defined(_BCM5357X_)
    for (cos = 0; cos < COSLCCOUNT_NUM; cos++) {
        SOC_IF_ERROR_RETURN(READ_COSLCCOUNTr(unit, lport, cos, coslccount));
        *count += COSLCCOUNTr_LCCOUNTf_GET(coslccount);
    }
    for (cos = 0; cos < COSLCCOUNT_QGROUP_NUM; cos++) {
        SOC_IF_ERROR_RETURN(READ_COSLCCOUNT_QGROUPr(unit, lport, cos, coslccount_qgroup));
        *count += COSLCCOUNT_QGROUPr_LCCOUNTf_GET(coslccount_qgroup);
    }
#endif
    return SYS_OK;
}

static int
soc_port_eee_timers_init(int unit, soc_port_t port, int speed)
{
    /* Do nothing for GH2 */
    return SYS_OK;
}

static int
soc_port_eee_timers_setup(int unit, soc_port_t port, int speed)
{
    /* Do nothing for GH2 */
    return SYS_OK;
}

static int
soc_port_thdo_rx_enable_set(int unit, soc_port_t port, int enable) 
{
    /* Do nothing for GH2 */
    return SYS_OK;
}

static int
soc_port_speed_update(int unit, uint8 lport, int speed) 
{
    /* Do nothing for GH2 */
    return SYS_OK;
}

static int
soc_port_egress_buffer_sft_reset(int unit, uint8 lport, int reset)
{
    /* Do nothing for GH2 */
    return SYS_OK;
}

#if 0
static int
soc_port_ingress_buffer_reset(int unit, uint8 lport, int reset)
{
    /* Do nothing for GH2 */
    return SYS_OK;
}
#endif

static int
soc_port_fifo_reset(int unit, uint8 lport)
{
    /* Do nothing for GH2 */
    return SYS_OK;
}


static int
soc_mmu_backpressure_clear(int unit, uint8 lport) 
{
    /* Do nothing for GH2 */
    return SYS_OK;
}

#if defined(_BCM95343X_)
static int
_soc_egress_metering_freeze(int unit, uint8 lport, void **setting){
    int      rv;
    int i;
    uint32 *buffer;
    int count;
    MAXBUCKETCONFIGr_t      maxbucketconfig;
    EGRMETERINGCONFIGr_t    egrmeteringconfig;

    rv = SOC_E_NONE;
    
    /* buffer required = (X+1), for store original register setting.
     * MAXBUCKETCONFIGr : X elements for a port
     * EGRMETERINGCONFIG : 1 register value for a port
     */
    count = MAXBUCKETCONFIG_NUM + 1;
    buffer = sal_malloc(count * sizeof(uint32));
    if (buffer == NULL) {
        rv = SOC_E_MEMORY;
        return rv;
    }
    sal_memset(buffer, 0, (count * sizeof(uint32)));
    
    /* save the original configuration : MAXBUCKETCONFIGr */
    count = 0;
    for (i = 0; i < MAXBUCKETCONFIG_NUM; i++) {
        rv = READ_MAXBUCKETCONFIGr(unit, lport, i, maxbucketconfig);
        if (SOC_FAILURE(rv)) {
            sal_free(buffer);
            break;
        }
        buffer[count] = MAXBUCKETCONFIGr_GET(maxbucketconfig);
        count ++;
        /* Disable egress metering for this port */
        MAXBUCKETCONFIGr_MAX_REFRESHf_SET(maxbucketconfig, 0);
        MAXBUCKETCONFIGr_MAX_THD_SELf_SET(maxbucketconfig, 0);
        
        rv = WRITE_MAXBUCKETCONFIGr(unit, lport, i, maxbucketconfig);
        if (SOC_FAILURE(rv)) {
            sal_free(buffer);
            break;
        }
    }

    /* save the original configuration : EGRMETERINGCONFIGr */
    rv = READ_EGRMETERINGCONFIGr(unit, lport, egrmeteringconfig);
    if (SOC_SUCCESS(rv)) {
        buffer[count] = EGRMETERINGCONFIGr_GET(egrmeteringconfig);
        count ++;
        
        EGRMETERINGCONFIGr_CLR(egrmeteringconfig);
        rv = WRITE_EGRMETERINGCONFIGr(unit, lport, egrmeteringconfig);
    }
    if (SOC_FAILURE(rv)) {
        sal_free(buffer);
        return rv;
    }
    *setting = buffer;
    
    return rv;
}
#endif//if defined(_BCM95343X_)

#if defined(_BCM5357X_)
static int
_soc_egress_metering_freeze(int unit, uint8 lport, void **setting){
    int      rv;
    int i;
    uint32 value;
    uint32 *buffer;
    int count;
    int qlayer_entry_count = 0;
    int qgroup_entry_count = 0;
    int memidx;
    int is_64q_port;
    MMU_MAX_BUCKET_QLAYERm_t    mmu_max_bucket_qlayer;
    MMU_MAX_BUCKET_QGROUPm_t    mmu_max_bucket_qgroup;
    EGRMETERINGCONFIGr_t    egrmeteringconfig;
    
    rv = soc_gh2_64q_port_check(unit, lport, &is_64q_port);
    if (SOC_FAILURE(rv)) {
        return rv;
    }

    if (is_64q_port) {
        qlayer_entry_count = SOC_GH2_QLAYER_COSQ_PER_PORT_NUM;
        qgroup_entry_count = SOC_GH2_QGROUP_PER_PORT_NUM;
    } else {
        qlayer_entry_count = SOC_GH2_LEGACY_QUEUE_NUM;
        qgroup_entry_count = 0;
    }
    
    /* buffer required (X*2+Y*2+1) for store original setting.
     *   MMU port 0~57: X=8, Y=0
     *   MMU port 58-65 : X=64, Y=8
     * MMU_MAX_BUCKET_QLAYERm : 
     *   X entries for a port, 2 fields (REFRESHf, THD_SELf)stored
     * MMU_MAX_BUCKET_QGROUPm : Y entries for a port
     *   Y entries for a port, 2 fields (REFRESHf, THD_SELf)stored
     * EGRMETERINGCONFIG : 1 register value for a port
     */
    count = (qlayer_entry_count * 2) +  (qgroup_entry_count * 2) + 1;
    buffer = sal_malloc(count * sizeof(uint32));
    if (buffer == NULL) {
        rv = SOC_E_MEMORY;
        return rv;
    }
    sal_memset(buffer, 0, (count * sizeof(uint32)));

    /* save the original configuration : MMU_MAX_BUCKET_QLAYERm */
    count = 0;
    rv = soc_gh2_mmu_bucket_qlayer_index(unit, lport, 0, &memidx);
    if (SOC_FAILURE(rv)) {
        sal_free(buffer);
        return rv;
    }
    for (i = memidx; i < qlayer_entry_count; i++) {
        rv = READ_MMU_MAX_BUCKET_QLAYERm(unit, i, mmu_max_bucket_qlayer);
        if (SOC_FAILURE(rv)) {
            sal_free(buffer);
            break;
        }
        
        value = MMU_MAX_BUCKET_QLAYERm_REFRESHf_GET(mmu_max_bucket_qlayer);
        buffer[count] = value;
        count ++;
        
        value = MMU_MAX_BUCKET_QLAYERm_THD_SELf_GET(mmu_max_bucket_qlayer);
        buffer[count] = value;
        count ++;

        /* Disable egress metering for this port */
        MMU_MAX_BUCKET_QLAYERm_REFRESHf_SET(mmu_max_bucket_qlayer, 0);
        MMU_MAX_BUCKET_QLAYERm_THD_SELf_SET(mmu_max_bucket_qlayer, 0);
        
        rv = WRITE_MMU_MAX_BUCKET_QLAYERm(unit, i, mmu_max_bucket_qlayer);
        if (SOC_FAILURE(rv)) {
            sal_free(buffer);
            break;
        }
    }
    
    /* save the original configuration : MMU_MAX_BUCKET_QGROUPm */
    if (qgroup_entry_count) {
        rv = soc_gh2_mmu_bucket_qgroup_index(unit, lport, 0, &memidx);
        if (SOC_FAILURE(rv)) {
            sal_free(buffer);
        }
        for (i = memidx; i < qgroup_entry_count; i++) {
            rv = READ_MMU_MAX_BUCKET_QGROUPm(unit, i, mmu_max_bucket_qgroup);
            if (SOC_FAILURE(rv)) {
                sal_free(buffer);
                break;
            }
            value = MMU_MAX_BUCKET_QGROUPm_REFRESHf_GET(mmu_max_bucket_qgroup);
            buffer[count] = value;
            count ++;
            value = MMU_MAX_BUCKET_QGROUPm_THD_SELf_GET(mmu_max_bucket_qgroup);
            buffer[count] = value;
            count ++;

            /* Disable egress metering for this port */
            MMU_MAX_BUCKET_QGROUPm_REFRESHf_SET(mmu_max_bucket_qgroup, 0);
            MMU_MAX_BUCKET_QGROUPm_THD_SELf_SET(mmu_max_bucket_qgroup, 0);
            rv = WRITE_MMU_MAX_BUCKET_QGROUPm(unit, i, mmu_max_bucket_qgroup);
            if (SOC_FAILURE(rv)) {
                sal_free(buffer);
                break;
            }
        }
    }

    /* save the original configuration : EGRMETERINGCONFIGr */
    rv = READ_EGRMETERINGCONFIGr(unit, lport, egrmeteringconfig);
    if (SOC_SUCCESS(rv)) {
        buffer[count] = EGRMETERINGCONFIGr_GET(egrmeteringconfig);
        count ++;
        
        EGRMETERINGCONFIGr_CLR(egrmeteringconfig);
        rv = WRITE_EGRMETERINGCONFIGr(unit, lport, egrmeteringconfig);
    }
    if (SOC_FAILURE(rv)) {
        sal_free(buffer);
        return rv;
    }
    *setting = buffer;
    
    return rv;
}
#endif//if defined(_BCM5357X_)

static int
soc_egress_drain_cells(int unit, uint8 lport, uint32 drain_timeout)
{
    void *setting = NULL;
    uint32 cur_cells, new_cells;
    int rv, rv1;
    
    SOC_IF_ERROR_RETURN(soc_mmu_backpressure_clear(unit, lport)); 
    SOC_IF_ERROR_RETURN(_soc_egress_metering_freeze(unit, lport, &setting)); 

    cur_cells = 0xffffffff;

    /* Probably not required to continuously check COSLCCOUNT if the fast
     * MMU flush feature is available - done just as an insurance */
    rv = SOC_E_NONE;
    for (;;) {
        if ((rv = soc_egress_cell_count(unit, lport, &new_cells)) < 0) {
            break;
        }

        if (new_cells == 0) {
            rv = SOC_E_NONE;
            break;
        }

        if (new_cells < cur_cells) {                    /* Progress made */
            cur_cells = new_cells;
        }
        
        sal_usleep(1000);
        drain_timeout -= 1000;
        if(drain_timeout <= 0){
            if ((rv = soc_egress_cell_count(unit, lport, &new_cells)) < 0) {
                break;
            }
            
            sal_printf("%s..: unit %d lport %d drain_timeout\n", __func__, unit, lport);           
            rv = SYS_ERR;
            break;
        }
            
    }
    
    /* Restore egress metering configuration. */
    rv1 = _soc_egress_metering_thaw(unit, lport, setting);
    if (SOC_SUCCESS(rv)) {
        rv  = rv1;
    }
  
    return rv;
}

// called in CLMAC
#if defined(_BCM95343X_)
static int
soc_mmu_flush_enable(int unit, uint8 lport, int enable)
{
    int port_off;
    MMUFLUSHCONTROLr_t mmuflushcontrol;
    
    /* First put the port in flush state - the packets from the XQ of the 
     * port are purged after dequeue.
     */
    port_off = lport;
    
    SOC_IF_ERROR_RETURN(
        READ_MMUFLUSHCONTROLr(unit, mmuflushcontrol) );
    PBMP_PORT_REMOVE(*((pbmp_t *) &mmuflushcontrol), port_off);
    if(enable)
        PBMP_PORT_ADD(*((pbmp_t *) &mmuflushcontrol), port_off);
    SOC_IF_ERROR_RETURN(
        WRITE_MMUFLUSHCONTROLr(unit, mmuflushcontrol) );

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit=%d lport %d MMUFLUSHCONTROl=0x%x\n", __func__, unit, lport, mmuflushcontrol.mmuflushcontrol[0]));      
        
    return SYS_OK;
}
#endif//if defined(_BCM95343X_)

#if 0
static int
soc_mmu_flush_enable(int unit, uint8 lport, int enable)
{
    int port_off;
    MMUFLUSHCONTROLr_t mmuflushcontrol;
    
    /* First put the port in flush state - the packets from the XQ of the 
     * port are purged after dequeue.
     */
    if (lport < 32) {
        port_off = lport;
        
        SOC_IF_ERROR_RETURN(
            READ_MMUFLUSHCONTROLr(unit, 0, mmuflushcontrol) );
        PBMP_PORT_REMOVE(*((pbmp_t *) &mmuflushcontrol), port_off);
        if(enable)
            PBMP_PORT_ADD(*((pbmp_t *) &mmuflushcontrol), port_off);
        SOC_IF_ERROR_RETURN(
            WRITE_MMUFLUSHCONTROLr(unit, 0, mmuflushcontrol) );
    } else if ((lport >= 32) && (lport < 64)) {
        port_off = lport - 32;
        
        SOC_IF_ERROR_RETURN(
            READ_MMUFLUSHCONTROLr(unit, 1, mmuflushcontrol) );
        PBMP_PORT_REMOVE(*((pbmp_t *) &mmuflushcontrol), port_off);
        if(enable)
            PBMP_PORT_ADD(*((pbmp_t *) &mmuflushcontrol), port_off);
        SOC_IF_ERROR_RETURN(
            WRITE_MMUFLUSHCONTROLr(unit, 1, mmuflushcontrol) );
    } else {
        port_off = lport - 64;
        
        SOC_IF_ERROR_RETURN(
            READ_MMUFLUSHCONTROLr(unit, 2, mmuflushcontrol) );
        PBMP_PORT_REMOVE(*((pbmp_t *) &mmuflushcontrol), port_off);
        if(enable)
            PBMP_PORT_ADD(*((pbmp_t *) &mmuflushcontrol), port_off);
        SOC_IF_ERROR_RETURN(
            WRITE_MMUFLUSHCONTROLr(unit, 2, mmuflushcontrol) );
    }
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit=%d lport %d MMUFLUSHCONTROl=0x%x\n", __func__, unit, lport, mmuflushcontrol.mmuflushcontrol[0]));      
        
    return SYS_OK;
}
#endif//if 0

static int
_mac_xl_drain_cells(int unit, uint8 lport, int notify_phy, int queue_enable)
{
    int         rv;
    int         pause_tx = 0, pause_rx = 0, pfc_rx = 0, llfc_rx = 0;
    XLMAC_CTRLr_t   xlmac_ctrl;
    XLMAC_TX_CTRLr_t    xlmac_tx_ctrl;
    XLMAC_TXFIFO_CELL_CNTr_t    xlmac_txfifo_cell_cnt;
    uint32      drain_timeout, fval;
#if defined(_BCM95343X_) 
    MMU_FC_RX_ENr_t     mmu_fc_rx_en, mmu_fc_rx_en_orig;
#endif
    
    
#if CONFIG_EMULATION
    drain_timeout = 250000000;
#else
    drain_timeout = 250000;
#endif
        
    /* Drain cells in mmu/port before cells entering TX FIFO */
#if defined(_BCM95343X_) 
    SOC_IF_ERROR_RETURN
        (soc_mmu_flush_enable(unit, lport, TRUE) );
        
    SOC_IF_ERROR_RETURN(
        READ_MMU_FC_RX_ENr(unit, lport, mmu_fc_rx_en_orig));
    MMU_FC_RX_ENr_CLR(mmu_fc_rx_en);
    SOC_IF_ERROR_RETURN(
        WRITE_MMU_FC_RX_ENr(unit, lport, mmu_fc_rx_en));
#endif
    /* Disable pause/pfc/llfc function */
    {
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_pause_get(unit, lport, &pause_tx, &pause_rx));
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_pause_set(unit, lport, pause_tx, 0));
    
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_control_get(unit, lport, SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                       &pfc_rx));
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_control_set(unit, lport, SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                       0));
    
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_control_get(unit, lport, SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                       &llfc_rx));
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_control_set(unit, lport, SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                       0));
    }                               
    
    
    {
        /* Assert SOFT_RESET before DISCARD just in case there is no credit left */
        SOC_IF_ERROR_RETURN(
            READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 1);
        /* Drain data in TX FIFO without egressing */
        SOC_IF_ERROR_RETURN(
            WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    }
    
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
    XLMAC_TX_CTRLr_DISCARDf_SET(xlmac_tx_ctrl, 1);
    XLMAC_TX_CTRLr_EP_DISCARDf_SET(xlmac_tx_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
    
    /* Reset EP credit before de-assert SOFT_RESET */
    SOC_IF_ERROR_RETURN(
        soc_port_credit_reset(unit, lport));

    {
        /* De-assert SOFT_RESET to let the drain start */
        SOC_IF_ERROR_RETURN(
            READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
        /* Drain data in TX FIFO without egressing */
        SOC_IF_ERROR_RETURN(
            WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    }
    
#ifdef UM_PHY_NOTIFY
    if(notify_phy){
        /* Notify PHY driver */
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_notify(unit, lport, phyEventStop, PHY_STOP_DRAIN));
    }
#endif

    /* Wait until mmu cell count is 0 */
    rv = soc_egress_drain_cells(unit, lport, drain_timeout);
    if (SOC_E_NONE == rv) {
        /* Wait until TX fifo cell count is 0 */
        for (;;) {
            rv =  READ_XLMAC_TXFIFO_CELL_CNTr(unit, lport, xlmac_txfifo_cell_cnt);
            if (SOC_E_NONE != rv) {
                break;
            }
            
            fval = XLMAC_TXFIFO_CELL_CNTr_CELL_CNTf_GET(xlmac_txfifo_cell_cnt);
            if (fval == 0) {
                break;
            }
            
            sal_usleep(1000);
            drain_timeout -= 1000;
            if(drain_timeout <= 0){
                sal_printf("%s..: unit %d lport %d drain_timeout\n", __func__, unit, lport);           
                rv = SYS_ERR;
                break;
            }
            
        }
        
    }

#ifdef UM_PHY_NOTIFY
    if(notify_phy){
        /* Notify PHY driver */
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_notify(unit, lport, phyEventResume, PHY_STOP_DRAIN));
    }
#endif

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
    XLMAC_TX_CTRLr_DISCARDf_SET(xlmac_tx_ctrl, 0);
    XLMAC_TX_CTRLr_EP_DISCARDf_SET(xlmac_tx_ctrl, 0);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
    
#if defined(_BCM95343X_) 
    /* Stop draining cells in mmu/port */
    SOC_IF_ERROR_RETURN(soc_mmu_flush_enable(unit, lport, FALSE));
    
    SOC_IF_ERROR_RETURN(
        WRITE_MMU_FC_RX_ENr(unit, lport, mmu_fc_rx_en_orig));
#endif
        
    /* Restore config below only if modified above, i.e. h/w flush not used */
    /* Restore original pause/pfc/llfc configuration */
    {
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_pause_set(unit, lport, pause_tx, pause_rx));
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_control_set(unit, lport,
                                       SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                       pfc_rx));
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_control_set(unit, lport,
                                       SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                       llfc_rx));
    }

    return SYS_OK;
}


/*
 * Function:
 *      _mac_xl_timestamp_byte_adjust_set
 * Purpose:
 *      Set timestamp byte adjust values.
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 * Returns:
 *      SOC_E_XXX
 */
static int
_mac_xl_timestamp_byte_adjust_set(int unit, uint8 lport)
{
    XLMAC_TIMESTAMP_BYTE_ADJUSTr_t  xlmac_timestamp_byte_adjust;

    if (IS_GE_PORT(lport)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit=%d lport %d IS_GE_PORT\n", __func__, unit, lport));
        
        /* for viper and eagle ports on SB2 B0 */
        SOC_IF_ERROR_RETURN(
            READ_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, xlmac_timestamp_byte_adjust));
        XLMAC_TIMESTAMP_BYTE_ADJUSTr_TX_TIMER_BYTE_ADJUSTf_SET(xlmac_timestamp_byte_adjust, 8);
        SOC_IF_ERROR_RETURN(
            WRITE_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, xlmac_timestamp_byte_adjust));

        SOC_IF_ERROR_RETURN(
            READ_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, xlmac_timestamp_byte_adjust));
        XLMAC_TIMESTAMP_BYTE_ADJUSTr_TX_TIMER_BYTE_ADJUST_ENf_SET(xlmac_timestamp_byte_adjust, 1);
        SOC_IF_ERROR_RETURN(
            WRITE_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, xlmac_timestamp_byte_adjust));

        SOC_IF_ERROR_RETURN(
            READ_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, xlmac_timestamp_byte_adjust));
        XLMAC_TIMESTAMP_BYTE_ADJUSTr_RX_TIMER_BYTE_ADJUSTf_SET(xlmac_timestamp_byte_adjust, 8);
        SOC_IF_ERROR_RETURN(
            WRITE_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, xlmac_timestamp_byte_adjust));

        SOC_IF_ERROR_RETURN(
            READ_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, xlmac_timestamp_byte_adjust));
        XLMAC_TIMESTAMP_BYTE_ADJUSTr_RX_TIMER_BYTE_ADJUST_ENf_SET(xlmac_timestamp_byte_adjust, 1);
        SOC_IF_ERROR_RETURN(
            WRITE_XLMAC_TIMESTAMP_BYTE_ADJUSTr(unit, lport, xlmac_timestamp_byte_adjust));

    }
    
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_init
 * Purpose:
 *      Initialize Clmac into a known good state.
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
static int
mac_xl_init(int unit, uint8 lport)
{
    XLMAC_CTRLr_t   xlmac_ctrl;
    XLMAC_RX_CTRLr_t    xlmac_rx_ctrl;
    XLMAC_TX_CTRLr_t    xlmac_tx_ctrl;
    XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
    XLMAC_RX_MAX_SIZEr_t    xlmac_rx_max_size;
    XLMAC_MODEr_t   xlmac_mode;
    XLMAC_RX_LSS_CTRLr_t   xlmac_rx_lss_ctrl;
    XLPORT_MAC_RSV_MASKr_t xlport_mac_rsv_mask;
    int ipg;
    int mode;
    int runt;
    int encap = 0;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d\n", __func__, unit, lport));

    /* Disable Tx/Rx, assume that MAC is stable (or out of reset) */
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    /* Reset EP credit before de-assert SOFT_RESET */
    if( XLMAC_CTRLr_SOFT_RESETf_GET(xlmac_ctrl) ){
        SOC_IF_ERROR_RETURN(
            soc_port_credit_reset(unit, lport));
    }
        
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(xlmac_ctrl, (IS_HG_PORT(lport) ? 1 : 0) );
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
                
    if (IS_ST_PORT(lport)) {
        soc_mac_xl.md_pause_set(unit, lport, FALSE, FALSE);
    } else {
        soc_mac_xl.md_pause_set(unit, lport, TRUE, TRUE);
    }
    
    
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
    XLMAC_PFC_CTRLr_PFC_REFRESH_ENf_SET(xlmac_pfc_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if 0   //phy_wan_mode is never defined
    if (soc_property_port_get(unit, port, spn_PHY_WAN_MODE, FALSE)) {
        /* Max speed for WAN mode is 9.294Gbps.
         * This setting gives 10Gbps * (13/14) or 9.286 Gbps */
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_control_set(unit, port,
                                       SOC_MAC_CONTROL_FRAME_SPACING_STRETCH,
                                       13));
    }
#endif
    
    /* Set jumbo max size (16360 byte payload) */
    XLMAC_RX_MAX_SIZEr_CLR(xlmac_rx_max_size);
    XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(xlmac_rx_max_size, JUMBO_MAXSZ);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_RX_MAX_SIZEr(unit, lport, xlmac_rx_max_size));
    
    XLMAC_MODEr_CLR(xlmac_mode);
#if 0
    if (!IS_XE_PORT(unit, lport) && !IS_GE_PORT(unit, lport)) {
        mode = soc_property_port_get(unit, port, spn_HIGIG2_HDR_MODE,
               soc_feature(unit, soc_feature_no_higig_plus) ? 1 : 0) ? 2 : 1;
        soc_reg64_field32_set(unit, XLMAC_MODEr, &rval, HDR_MODEf, mode);
        encap = mode;
    }
#endif
    switch (SOC_PORT_SPEED_MAX(lport)) {
    case 10:
        mode = SOC_XLMAC_SPEED_10;
        break;
    case 100:
        mode = SOC_XLMAC_SPEED_100;
        break;
    case 1000:
        mode = SOC_XLMAC_SPEED_1000;
        break;
    case 2500:
        mode = SOC_XLMAC_SPEED_2500;
        break;
    default:
        mode = SOC_XLMAC_SPEED_10000;
        break;
    }
    XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, mode);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_MODEr(unit, lport, xlmac_mode) );


    /* 1. Initialize mask for purging packet data received from the MAC 
       2. Diable length check */
    XLPORT_MAC_RSV_MASKr_CLR(xlport_mac_rsv_mask);
    XLPORT_MAC_RSV_MASKr_MASKf_SET(xlport_mac_rsv_mask, 0x58);
    WRITE_XLPORT_MAC_RSV_MASKr(unit, lport, xlport_mac_rsv_mask);
        
    /* init IPG and RUNT_THRESHOLD after port encap mode been established. */
    if (encap == 1) {
        ipg = FD_HG_IPG;
        runt = XLMAC_RUNT_THRESHOLD_HG1;
    } else if (encap == 2) {
        ipg = FD_HG2_IPG;
        runt = XLMAC_RUNT_THRESHOLD_HG2;
    } else {
        ipg = FD_XE_IPG;
        runt = XLMAC_RUNT_THRESHOLD_IEEE;
    }

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
    XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, ((ipg / 8) & 0x1f) );
    XLMAC_TX_CTRLr_CRC_MODEf_SET(xlmac_tx_ctrl, XLMAC_CRC_PER_PKT_MODE );
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
    
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));
    XLMAC_RX_CTRLr_STRIP_CRCf_SET(xlmac_rx_ctrl, 0);
    XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, SOC_PORT_SPEED_MAX(lport) >= 10000 &&
                          IS_XE_PORT(lport) ? 1 : 0);
             
    /* assigning RUNT_THRESHOLD (per encap mode) */
    XLMAC_RX_CTRLr_RUNT_THRESHOLDf_SET(xlmac_rx_ctrl, runt);    
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));
    
    SOC_IF_ERROR_RETURN(soc_port_eee_timers_init(unit,
                lport, SOC_PORT_SPEED_MAX(lport)) );
                            
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(xlmac_rx_lss_ctrl, 1);
    XLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(xlmac_rx_lss_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));

    /* Disable loopback and bring XLMAC out of reset */
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    XLMAC_CTRLr_LOCAL_LPBKf_SET(xlmac_ctrl, 0);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));  
    
    SOC_IF_ERROR_RETURN(
        _mac_xl_timestamp_byte_adjust_set(unit, lport));              
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_egress_queue_drain
 * Purpose:
 *      Drain the egress queues with out bringing down the port
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_egress_queue_drain(int unit, uint8 lport)
{
    int rx_enable = 0;
    int is_active = 0;
    XLMAC_CTRLr_t   xlmac_ctrl, xlmac_ctrl_orig;
#if defined(_BCM95343X_)
    EPC_LINK_BMAP_64r_t epc_link_bmap_64;
#elif defined(_BCM5357X_)
    EPC_LINK_BMAP_HI_64r_t epc_link_bmap_hi_64;
    EPC_LINK_BMAP_LO_64r_t epc_link_bmap_lo_64;
#endif
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d\n", __func__, unit, lport));

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl) );
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl_orig) );
        
    rx_enable = XLMAC_CTRLr_RX_ENf_GET(xlmac_ctrl);
    /* Don't disable TX since it stops egress and hangs if CPU sends */
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, 0);
        /* Disable RX */
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

        /* Remove port from EPC_LINK */
#if defined(_BCM95343X_)
        READ_EPC_LINK_BMAP_64r(unit, epc_link_bmap_64);
        if( PBMP_MEMBER(*((pbmp_t *) &epc_link_bmap_64), lport) ){
            is_active = 1;
            PBMP_PORT_REMOVE(*((pbmp_t *) &epc_link_bmap_64), lport);
            WRITE_EPC_LINK_BMAP_64r(unit, epc_link_bmap_64);
        }
#elif defined(_BCM5357X_)
    if (lport >= 64) {
		READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
		if( PBMP_MEMBER(*((pbmp_t *) &epc_link_bmap_hi_64), lport) ){
		    is_active = 1;
    		PBMP_PORT_REMOVE(*((pbmp_t *) &epc_link_bmap_hi_64), (lport-64));
    		WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
	    }
    } else {
        READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
        if( PBMP_MEMBER(*((pbmp_t *) &epc_link_bmap_lo_64), lport) ){
            is_active = 1;
            PBMP_PORT_REMOVE(*((pbmp_t *) &epc_link_bmap_lo_64), lport);
            WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
        }
    }
#endif
        /* Drain cells */
    SOC_IF_ERROR_RETURN(
        _mac_xl_drain_cells(unit, lport, 0, TRUE));

        /* Reset port FIFO */
    SOC_IF_ERROR_RETURN(soc_port_fifo_reset(unit, lport));
    
        /* Put port into SOFT_RESET */
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

        /* Reset EP credit before de-assert SOFT_RESET */
    SOC_IF_ERROR_RETURN(
        soc_port_credit_reset(unit, lport));
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, (rx_enable ? 1 : 0) );

      /* Enable both TX and RX, deassert SOFT_RESET */
    XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

        /* Restore xlmac_ctrl to original value */
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl_orig));        

        /* Add port to EPC_LINK */
#if defined(_BCM95343X_)
    if(is_active) {     
            READ_EPC_LINK_BMAP_64r(unit, epc_link_bmap_64);
            PBMP_PORT_ADD(*((pbmp_t *) &epc_link_bmap_64), lport);
            WRITE_EPC_LINK_BMAP_64r(unit, epc_link_bmap_64);
    }
#elif defined(_BCM5357X_)
    if(is_active) {
        if (lport >= 64) {
		    READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
			PBMP_PORT_ADD(*((pbmp_t *) &epc_link_bmap_hi_64), (lport-64));
    		WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
	    } else {
            READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
            PBMP_PORT_ADD(*((pbmp_t *) &epc_link_bmap_lo_64), lport);
            WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
        }
    }
#endif
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_enable_set
 * Purpose:
 *      Enable or disable MAC
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 *      enable - TRUE to enable, FALSE to disable
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_enable_set(int unit, uint8 lport, int enable)
{
    XLMAC_CTRLr_t   xlmac_ctrl, xlmac_ctrl_orig;
#if defined(_BCM95343X_)
    EPC_LINK_BMAP_64r_t epc_link_bmap_64;
#elif defined(_BCM5357X_)
    EPC_LINK_BMAP_HI_64r_t epc_link_bmap_hi_64;
    EPC_LINK_BMAP_LO_64r_t epc_link_bmap_lo_64;
#endif

#ifdef CFG_TIMESTAMP_MAC_DELAY
    int speed = 1000;
#endif
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d enable=%d\n", __func__, unit, lport, enable));

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl) );
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl_orig) );
        
    /* Don't disable TX since it stops egress and hangs if CPU sends */
    XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, 1);
    XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, enable ? 1 : 0);
    
    if (!sal_memcmp(&xlmac_ctrl, &xlmac_ctrl_orig, sizeof(xlmac_ctrl_orig))) {
        if (enable) {
            return SYS_OK;
        } else {
            if (XLMAC_CTRLr_SOFT_RESETf_GET(xlmac_ctrl)) {
                return SYS_OK;
            }
        }
    }
    
    if (enable) {
        /* Reset EP credit before de-assert SOFT_RESET */
        SOC_IF_ERROR_RETURN(
            soc_port_credit_reset(unit, lport));
            
        /* Deassert SOFT_RESET */
        XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 0);
        
        /* Deassert EGR_XLPORT_BUFFER_SFT_RESET */
        SOC_IF_ERROR_RETURN(
            soc_port_egress_buffer_sft_reset(unit, lport, 0));
        
        /* Enable both TX and RX */    
        SOC_IF_ERROR_RETURN(
            WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
            
#if defined(_BCM95343X_)
            READ_EPC_LINK_BMAP_64r(unit, epc_link_bmap_64);
            PBMP_PORT_ADD(*((pbmp_t *) &epc_link_bmap_64), lport);
            WRITE_EPC_LINK_BMAP_64r(unit, epc_link_bmap_64);
#elif defined(_BCM5357X_)
        if (lport >= 64) {
		    READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
			PBMP_PORT_ADD(*((pbmp_t *) &epc_link_bmap_hi_64), (lport-64));
    		WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
	    } else {
            READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
            PBMP_PORT_ADD(*((pbmp_t *) &epc_link_bmap_lo_64), lport);
            WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
        }
#endif
 
        /* Enable output threshold RX */
        SOC_IF_ERROR_RETURN
            (soc_port_thdo_rx_enable_set(unit, lport, 1));        

        /* set timestamp adjust delay */
#ifdef CFG_TIMESTAMP_MAC_DELAY
        mac_xl_speed_get(unit, lport, &speed);
        _mac_xl_timestamp_delay_set(unit, lport, speed);
#endif
          
    } else {
        /* Disable MAC RX */
        SOC_IF_ERROR_RETURN(
            WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

#if defined(_BCM95343X_)
            READ_EPC_LINK_BMAP_64r(unit, epc_link_bmap_64);
            PBMP_PORT_REMOVE(*((pbmp_t *) &epc_link_bmap_64), lport);
            WRITE_EPC_LINK_BMAP_64r(unit, epc_link_bmap_64);
#elif defined(_BCM5357X_)
        if (lport >= 64) {
    		READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
			PBMP_PORT_REMOVE(*((pbmp_t *) &epc_link_bmap_hi_64), (lport-64));
    		WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
        } else {
            READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
            PBMP_PORT_REMOVE(*((pbmp_t *) &epc_link_bmap_lo_64), lport);
            WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
        }
#endif
   
        /* Delay to ensure EOP is received at Ingress */
        sal_usleep(1000);
        
        /* Drain cells */
        SOC_IF_ERROR_RETURN(_mac_xl_drain_cells(unit, lport, 1, FALSE));
        
        /* Reset egress_buffer */
        SOC_IF_ERROR_RETURN(soc_port_egress_buffer_sft_reset(unit, lport, 1));
        
        /* Reset port FIFO */
        SOC_IF_ERROR_RETURN(soc_port_fifo_reset(unit, lport));
        
        /* Put port into SOFT_RESET */
        XLMAC_CTRLr_SOFT_RESETf_SET(xlmac_ctrl, 1);
        SOC_IF_ERROR_RETURN(
            WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
                                                           
        /* Disable output threshold RX */
        SOC_IF_ERROR_RETURN
            (soc_port_thdo_rx_enable_set(unit, lport, 0));        
    }
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_enable_get
 * Purpose:
 *      Get MAC enable state
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 *      enable - (OUT) TRUE if enabled, FALSE if disabled
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_enable_get(int unit, uint8 lport, int *enable)
{
    XLMAC_CTRLr_t   xlmac_ctrl;

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));

    *enable = XLMAC_CTRLr_RX_ENf_GET(xlmac_ctrl);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d *enable=%d\n", __func__, unit, lport, *enable));

    return SYS_OK;
}

#ifdef CFG_TIMESTAMP_MAC_DELAY
/*
 * Function:
 *      _mac_xl_timestamp_delay_set
 * Purpose:
 *      Set Timestamp delay for one-step to account for lane and pipeline delay.
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 *      speed - Speed
 *      phy_mode - single/dual/quad phy mode
 * Returns:
 *      SOC_E_XXX
 */
static int
_mac_xl_timestamp_delay_set(int unit, uint8 lport, int speed)
{
    uint32 clk_rate, tx_clk_ns;
    int osts_delay;
    int divisor;
    XLMAC_TIMESTAMP_ADJUSTr_t   xlmac_timestamp_adjust;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d speed=%dMb\n", __func__, unit, lport, speed));

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_TIMESTAMP_ADJUSTr(unit, lport, xlmac_timestamp_adjust));

    _mac_xl_speed_to_clock_rate(unit, lport, speed, &clk_rate);
    /* Tx clock rate for single/dual/quad phy mode */
    if ((speed >= 5000) && (speed <= 40000)) {
        divisor = speed > 20000 ? 1 : speed > 10000 ? 2 : 4;
        /* tx clock rate in ns */
        tx_clk_ns = ((1000 / clk_rate) / divisor);
    } else {
        /* Same tx clk rate for < 10G  for all phy modes*/
        tx_clk_ns = 1000 / clk_rate;
    }

    
    
    /*
     * MAC pipeline delay for XGMII/XGMII mode is:
     *          = (5.5 * TX line clock period) + (Timestamp clock period)
     */
    /* signed value of pipeline delay in ns */
#if 0
    osts_delay = soc_property_port_get(unit, port, spn_TIMESTAMP_ADJUST(speed),
                                       SOC_TIMESYNC_PLL_CLOCK_NS(unit) -
                                       ((11 * tx_clk_ns ) / 2));
    
    if (SOC_E_NONE != soc_reg_signed_field_mask(unit, XLMAC_TIMESTAMP_ADJUSTr,
                                                TS_OSTS_ADJUSTf, osts_delay, &field)) {
        LOG_WARN(BSL_LS_SOC_PORT, (BSL_META_U(unit,
             "%s property out of bounds (is %d)\n"),
             spn_TIMESTAMP_ADJUST(speed), osts_delay));
        field = 0;
    }
#else
    osts_delay = SPN_TIMESTAMP_ADJUST ? SPN_TIMESTAMP_ADJUST: (SOC_TIMESYNC_PLL_CLOCK_NS(unit) -
                                       ((11 * tx_clk_ns ) / 2));
    osts_delay &= XLMAC_TIMESTAMP_ADJUST__TS_OSTS_ADJUST__MASK;
#endif
    
    XLMAC_TIMESTAMP_ADJUSTr_TS_OSTS_ADJUSTf_SET(xlmac_timestamp_adjust, osts_delay);
    
#if 0 
    /*
     * Lane delay for xlmac lanes
     *   Lane_0(0-3)  : 1 * TX line clock period
     *   Lane_1(4-7)  : 2 * TX line clock period
     *   Lane_2(8-11) : 3 * TX line clock period
     *   Lane_3(12-15): 4 * TX line clock period
     */
    /* unsigned value of lane delay in ns */
    delay = 1 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl,
                          TS_ADJUST_DEMUX_DELAY_0f, delay );
    delay = 2 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl,
                          TS_ADJUST_DEMUX_DELAY_1f, delay );
    delay = 3 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl,
                          TS_ADJUST_DEMUX_DELAY_2f, delay );
    delay = 4 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl,
                          TS_ADJUST_DEMUX_DELAY_3f, delay );
#endif
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_TIMESTAMP_ADJUSTr(unit, lport, xlmac_timestamp_adjust));

    /* Set Timestamp byte adjust values */
    SOC_IF_ERROR_RETURN(
        _mac_xl_timestamp_byte_adjust_set(unit, lport));

    return SOC_E_NONE;
}
#endif//ifdef CFG_TIMESTAMP_MAC_DELAY

/*
 * Function:
 *      mac_xl_duplex_set
 * Purpose:
 *      Set XLMAC in the specified duplex mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      duplex - Boolean: true --> full duplex, false --> half duplex.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
static int
mac_xl_duplex_set(int unit, uint8 lport, int duplex)
{
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d duplex=%d\n", __func__, unit, lport, duplex));
                                
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_duplex_get
 * Purpose:
 *      Get XLMAC duplex mode.
 * Parameters:
 *      unit - XGS unit #.
 *      duplex - (OUT) Boolean: true --> full duplex, false --> half duplex.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_duplex_get(int unit, uint8 lport, int *duplex)
{
    *duplex = TRUE; /* Always full duplex */

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d *duplex=%d\n", __func__, unit, lport, *duplex));
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_pause_set
 * Purpose:
 *      Configure XLMAC to transmit/receive pause frames.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_tx - Boolean: transmit pause or -1 (don't change)
 *      pause_rx - Boolean: receive pause or -1 (don't change)
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_pause_set(int unit, uint8 lport, int pause_tx, int pause_rx)
{
    XLMAC_PAUSE_CTRLr_t     xlmac_pause_ctrl;

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));
    if (pause_tx) {
        XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
    } else {
		XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(xlmac_pause_ctrl, 0);
    }

    if (pause_rx) {
        XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(xlmac_pause_ctrl, 1);
    } else {
        XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(xlmac_pause_ctrl, 0);
    }
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));
    
#if UM_READBACK_DEBUG
{
    int p_tx=0, p_rx=0;
    
    /* Read back after setting xlmac_pause_ctrl for checking */
    SOC_IF_ERROR_RETURN(READ_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));
    
    p_rx = XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_GET(xlmac_pause_ctrl);
    p_tx = XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_GET(xlmac_pause_ctrl);

    sal_printf("read back after setting xlmac_pause_ctrl: lport %d RX=%s TX=%s\n", lport,
                 p_rx ? "on" : "off",
                 p_tx ? "on" : "off");
}
#endif
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_pause_get
 * Purpose:
 *      Return the pause ability of XLMAC
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_tx - Boolean: transmit pause
 *      pause_rx - Boolean: receive pause
 *      pause_mac - MAC address used for pause transmission.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_pause_get(int unit, uint8 lport, int *pause_tx, int *pause_rx)
{
    XLMAC_PAUSE_CTRLr_t     xlmac_pause_ctrl;
	
    SOC_IF_ERROR_RETURN(READ_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));
    
    *pause_rx = XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_GET(xlmac_pause_ctrl);
    *pause_tx =  XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_GET(xlmac_pause_ctrl);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("xlmac_pause_ctrl: lport %d RX=%s TX=%s\n", lport,
                 *pause_rx ? "on" : "off",
                 *pause_tx ? "on" : "off"));
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_speed_set
 * Purpose:
 *      Set XLMAC in the specified speed.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      speed - 100000, 120000.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_speed_set(int unit, uint8 lport, int speed)
{
    uint32 mode;
    int enable;

    XLMAC_MODEr_t xlmac_mode;
    XLMAC_RX_CTRLr_t xlmac_rx_ctrl;
    XLMAC_RX_LSS_CTRLr_t xlmac_rx_lss_ctrl;
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d speed=%dMb\n", __func__, unit, lport, speed));
    
    switch (speed) {
    case 10:
        mode = SOC_XLMAC_SPEED_10;
        break;
    case 100:
        mode = SOC_XLMAC_SPEED_100;
        break;
    case 1000:
        mode = SOC_XLMAC_SPEED_1000;
        break;
    case 2500:
        mode = SOC_XLMAC_SPEED_2500;
        break;
    case 5000:
        mode = SOC_XLMAC_SPEED_10000;
        break;
    case 0:
        return SYS_OK;              /* Support NULL PHY */
    default:
        if (speed < 10000) {
            return SYS_ERR_PARAMETER;
        }
        mode = SOC_XLMAC_SPEED_10000;
        break;
    }


    SOC_IF_ERROR_RETURN(
        mac_xl_enable_get(unit, lport, &enable));        
    if (enable) {
        /* Turn off TX/RX enable */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, lport, 0));
    }

    /* Update the speed */
    READ_XLMAC_MODEr(unit, lport, xlmac_mode);
    XLMAC_MODEr_SPEED_MODEf_SET(xlmac_mode, mode);
    WRITE_XLMAC_MODEr(unit, lport, xlmac_mode);
    
    READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl);
    /* Enable LOCAL_FAULT_DISABLEf[Bit 0] and REMOTE_FAULT_DISABLEf[Bit 1]
     * if speed < 5000 
     */
    if (speed < 5000) {
        XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 1);
        XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 1);
    } else {
        XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 0);
        XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, 0);
    }
    WRITE_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl);
    
    
    /* Update port speed related setting in components other than MAC/SerDes*/
    SOC_IF_ERROR_RETURN(soc_port_speed_update(unit, lport, speed));
    
    /* Update port strict preamble */
    READ_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl);
    /* Enable STRICT_PREAMBLEf[Bit 3] if speed >= 10000 */
    if (speed >= 10000) {
        /* && IS_XE_PORT(unit, port) */
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 1);
    } else {
        XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, 0);
    }
    WRITE_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl);


    /*
     * Notify internal PHY driver of speed change in case it is being
     * used as pass-through to an external PHY.
     */
#ifdef UM_PHY_NOTIFY
    SOC_IF_ERROR_RETURN(
        soc_phyctrl_notify(unit, lport, phyEventSpeed, speed));
#endif
    
    if (enable) {
        /* Re-enable transmitter and receiver */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, lport, 1));
    }

#ifdef CFG_TIMESTAMP_MAC_DELAY
    /* Set Timestamp Mac Delays */
    _mac_xl_timestamp_delay_set(unit, port, speed);
#endif

    SOC_IF_ERROR_RETURN(
        soc_port_eee_timers_setup(unit, lport, speed));
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_speed_get
 * Purpose:
 *      Get XLMAC speed
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      speed - (OUT) speed in Mb
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_speed_get(int unit, uint8 lport, int *speed)
{
    XLMAC_MODEr_t   xlmac_mode;
    
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_MODEr(unit, lport, xlmac_mode) );
    switch (XLMAC_MODEr_SPEED_MODEf_GET(xlmac_mode)) {
    case SOC_XLMAC_SPEED_10:
        *speed = 10;
        break;
    case SOC_XLMAC_SPEED_100:
        *speed = 100;
        break;
    case SOC_XLMAC_SPEED_1000:
        *speed = 1000;
        break;
    case SOC_XLMAC_SPEED_2500:
        *speed = 2500;
        break;
    case SOC_XLMAC_SPEED_10000:
    default:
        *speed = 10000;
        break;
    }
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:mac_xl_speed_get: unit %d lport %d speed=%dMb\n", __func__, unit, lport, *speed));
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_loopback_set
 * Purpose:
 *      Set a XLMAC into/out-of loopback mode
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS unit # on unit.
 *      loopback - Boolean: true -> loopback mode, false -> normal operation
 * Note:
 *      On Clmac, when setting loopback, we enable the TX/RX function also.
 *      Note that to test the PHY, we use the remote loopback facility.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_loopback_set(int unit, uint8 lport, int lb)
{
    XLMAC_CTRLr_t   xlmac_ctrl;
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d loopback=%d\n", __func__, unit, lport, lb));
        
    /* need to enable clock compensation for applicable serdes device */
#ifdef UM_PHY_NOTIFY
    (void)soc_phyctrl_notify(unit, lport, phyEventMacLoopback, lb? 1: 0);
#endif
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, lport,
                                   SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE,
                                   lb ? 0 : 1));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, lport,
                                   SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE,
                                   lb ? 0 : 1));
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl) );
    XLMAC_CTRLr_LOCAL_LPBKf_SET(xlmac_ctrl, (lb ? 1 : 0));
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl) );
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_loopback_get
 * Purpose:
 *      Get current XLMAC loopback mode setting.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      loopback - (OUT) Boolean: true = loopback, false = normal
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_loopback_get(int unit, uint8 lport, int *lb)
{
    XLMAC_CTRLr_t   xlmac_ctrl;

    SOC_IF_ERROR_RETURN(
        READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl) );

    *lb = XLMAC_CTRLr_LOCAL_LPBKf_GET(xlmac_ctrl);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d loopback=%d\n", __func__, unit, lport, *lb));
                                
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_pause_addr_set
 * Purpose:
 *      Configure PAUSE frame source address.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_mac - (OUT) MAC address used for pause transmission.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_pause_addr_set(int unit, uint8 lport, sal_mac_addr_t mac)
{
    uint32 values[2];
    XLMAC_TX_MAC_SAr_t      xlmac_tx_mac_sa;
    XLMAC_RX_MAC_SAr_t      xlmac_rx_mac_sa;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d MAC=%02x:%02x:%02x:%02x:%02x:%02x\n", __func__, unit, lport, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));
    
    values[0] = (mac[0] << 8) | mac[1];
    values[1] = (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];

    XLMAC_TX_MAC_SAr_SA_LOf_SET(xlmac_tx_mac_sa, values[1]);
    XLMAC_TX_MAC_SAr_SA_HIf_SET(xlmac_tx_mac_sa, values[0]);
    
    XLMAC_RX_MAC_SAr_SA_LOf_SET(xlmac_rx_mac_sa, values[1]);
    XLMAC_RX_MAC_SAr_SA_HIf_SET(xlmac_rx_mac_sa, values[0]);
    
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_TX_MAC_SAr(unit, lport, xlmac_tx_mac_sa));
        
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_RX_MAC_SAr(unit, lport, xlmac_rx_mac_sa));
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_pause_addr_get
 * Purpose:
 *      Retrieve PAUSE frame source address.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_mac - (OUT) MAC address used for pause transmission.
 * Returns:
 *      SOC_E_XXX
 * NOTE: We always write the same thing to TX & RX SA
 *       so, we just return the contects on RX_MAC_SA.
 */
static int
mac_xl_pause_addr_get(int unit, uint8 lport, sal_mac_addr_t mac)
{
    uint32 values[2];
    XLMAC_RX_MAC_SAr_t      xlmac_rx_mac_sa;


    SOC_IF_ERROR_RETURN(
        READ_XLMAC_RX_MAC_SAr(unit, lport, xlmac_rx_mac_sa));
        
    values[1] = XLMAC_RX_MAC_SAr_SA_LOf_GET(xlmac_rx_mac_sa);
    values[0] = XLMAC_RX_MAC_SAr_SA_HIf_GET(xlmac_rx_mac_sa);
    
    mac[0] = (values[0] & 0x0000ff00) >> 8;
    mac[1] = values[0] & 0x000000ff;
    mac[2] = (values[1] & 0xff000000) >> 24;
    mac[3] = (values[1] & 0x00ff0000) >> 16;
    mac[4] = (values[1] & 0x0000ff00) >> 8;
    mac[5] = values[1] & 0x000000ff;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d MAC=%02x:%02x:%02x:%02x:%02x:%02x\n", __func__, unit, lport, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_interface_set
 * Purpose:
 *      Set a XLMAC interface type
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pif - one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_UNAVAIL - requested mode not supported.
 * Notes:
 * refer to include\soc\portmode.h and include\shared\port.h for the enumeration value
 */
static int
mac_xl_interface_set(int unit, uint8 lport, soc_port_if_t pif)
{
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d pif=%d\n", __func__, unit, lport, pif));

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_interface_get
 * Purpose:
 *      Retrieve XLMAC interface type
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pif - (OUT) one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_NONE
 */
static int
mac_xl_interface_get(int unit, uint8 lport, soc_port_if_t *pif)
{
    *pif = SOC_PORT_IF_MII;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d *pif=%d\n", __func__, unit, lport, *pif));
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_frame_max_set
 * Description:
 *      Set the maximum receive frame size for the port
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      size - Maximum frame size in bytes
 * Return Value:
 *      BCM_E_XXX
 */
static int
mac_xl_frame_max_set(int unit, uint8 lport, int size)
{
    XLMAC_RX_MAX_SIZEr_t    xlmac_rx_max_size;
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d size=%d\n", __func__, unit, lport, size));
    
    if (IS_CE_PORT(lport) || IS_XE_PORT(lport)) {
        /* For VLAN tagged packets */
        size += 4;
    }
    
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_RX_MAX_SIZEr(unit, lport, xlmac_rx_max_size));
    XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(xlmac_rx_max_size, size);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_RX_MAX_SIZEr(unit, lport, xlmac_rx_max_size));
                                  
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_frame_max_get
 * Description:
 *      Set the maximum receive frame size for the port
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      size - Maximum frame size in bytes
 * Return Value:
 *      BCM_E_XXX
 */
static int
mac_xl_frame_max_get(int unit, uint8 lport, int *size)
{
    XLMAC_RX_MAX_SIZEr_t    xlmac_rx_max_size;
    
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_RX_MAX_SIZEr(unit, lport, xlmac_rx_max_size));
        
    *size = XLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_GET(xlmac_rx_max_size);
    if (IS_CE_PORT(lport) || IS_XE_PORT(lport)) {
        /* For VLAN tagged packets */
        *size -= 4;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d *size=%d\n", __func__, unit, lport, *size));
                            
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_ifg_set
 * Description:
 *      Sets the new ifg (Inter-frame gap) value
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      ifg - number of bits to use for average inter-frame gap
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      The function makes sure the IFG value makes sense and updates the
 *      IPG register in case the speed/duplex match the current settings
 */
static int
mac_xl_ifg_set(int unit, uint8 lport, int speed,
                soc_port_duplex_t duplex, int ifg)
{
    int         cur_speed;
    int         cur_duplex;
    int         real_ifg;
    soc_port_ability_t ability;
    uint32      pa_flag;
    XLMAC_TX_CTRLr_t    xlmac_tx_ctrl, xlmac_tx_ctrl_orig;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d speed=%dMb duplex=%s ifg=%d\n", __func__, unit, lport, speed, duplex ? "True" : "False", ifg));
    
    pa_flag = SOC_PA_SPEED(speed); 
    soc_mac_xl.md_ability_local_get(unit, lport, &ability);
    if (!(pa_flag & ability.speed_full_duplex)) {
        return SYS_ERR;
    }

    /* Silently adjust the specified ifp bits to valid value */
    /* valid value: 8 to 31 bytes (i.e. multiple of 8 bits) */
    real_ifg = ifg < 64 ? 64 : (ifg > 504 ? 504 : (ifg + 7) & (0x3f << 3));
    /*  
    if (IS_CE_PORT(unit, port) || IS_XE_PORT(unit, port)) {
        si->fd_xe = real_ifg;
    } else {
        si->fd_hg = real_ifg;
    }
    */

    SOC_IF_ERROR_RETURN(
        mac_xl_duplex_get(unit, lport, &cur_duplex));
    SOC_IF_ERROR_RETURN(
        mac_xl_speed_get(unit, lport, &cur_speed));

    /* XLMAC_MODE supports only 4 speeds with 4 being max as LINK_10G_PLUS */
    /* Unlike the corresponding XLMAC function call, mac_xl_speed_get()
     * returns a fine grained speed value when XLMAC_MODE.SPEEDf=LINK_10G_PLUS
     * Hence the check below uses cur_speed >= 10000, unlike the
     * cur_speed == 10000 check used in xlmac */
    if ((speed >= 10000) && (cur_speed >= 10000)) {
        cur_speed = speed;
    }

    if (cur_speed == speed &&
        cur_duplex == (duplex == SOC_PORT_DUPLEX_FULL ? TRUE : FALSE)) {
        SOC_IF_ERROR_RETURN(
            READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
        SOC_IF_ERROR_RETURN(
            READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl_orig));
            
        XLMAC_TX_CTRLr_AVERAGE_IPGf_SET(xlmac_tx_ctrl, (real_ifg / 8) );
        
        if ( !PBMP_EQ(*((pbmp_t *) &xlmac_tx_ctrl), *((pbmp_t *) &xlmac_tx_ctrl_orig)) ) {        
            SOC_IF_ERROR_RETURN(
                WRITE_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
        }
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_ifg_get
 * Description:
 *      Sets the new ifg (Inter-frame gap) value
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      size - Maximum frame size in bytes
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      The function makes sure the IFG value makes sense and updates the
 *      IPG register in case the speed/duplex match the current settings
 */
static int
mac_xl_ifg_get(int unit, uint8 lport, int speed,
                soc_port_duplex_t duplex, int *ifg)
{
    soc_port_ability_t ability;
    uint32      pa_flag;
    XLMAC_TX_CTRLr_t    xlmac_tx_ctrl;

    if (!duplex) {
        return SYS_ERR;
    }

    pa_flag = SOC_PA_SPEED(speed); 
    soc_mac_xl.md_ability_local_get(unit, lport, &ability);
    if (!(pa_flag & ability.speed_full_duplex)) {
        return SYS_ERR;
    }

    SOC_IF_ERROR_RETURN(
            READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
            
    *ifg = XLMAC_TX_CTRLr_AVERAGE_IPGf_GET(xlmac_tx_ctrl);
    *ifg = (*ifg) * (8);
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d speed=%dMb duplex=%s *ifg=%d\n", __func__, unit, lport, speed, duplex ? "True" : "False", *ifg));
        
    return SYS_OK;
}

/*
 * Function:
 *      _mac_xl_port_mode_update
 * Purpose:
 *      Set the XLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      to_hg_port - (TRUE/FALSE)
 * Returns:
 *      SOC_E_XXX
 */
static int
_mac_xl_port_mode_update(int unit, uint8 lport, int hg_mode)
{
    /*  Should be called since 
    *   if (soc_feature(unit, soc_feature_hg2_light_in_portmacro)) is always true for GH2 and HR3
    *   Currently not implemanted since mac_xl_encap_set should not be called at runtime.
    */
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d to_hg_port=%d\n", __func__, unit, lport, hg_mode));
    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_encap_set
 * Purpose:
 *      Set the XLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (IN) encap bits (defined above)
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_encap_set(int unit, uint8 lport, int mode)
{
    int enable, encap;
    //int to_hg_port = -1;
    XLMAC_MODEr_t   xlmac_mode;
    XLMAC_RX_CTRLr_t    xlmac_rx_ctrl;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d encapsulation=%s\n", __func__, unit, lport, mac_xl_encap_mode[mode]));

    switch (mode) {
    case SOC_ENCAP_IEEE:
    case SOC_ENCAP_HIGIG2_LITE:
        encap = 0;
        break;
    case SOC_ENCAP_HIGIG:
        encap = 1;
        break;
    case SOC_ENCAP_HIGIG2:
        encap = 2;
        break;
    default:
        return SOC_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(mac_xl_enable_get(unit, lport, &enable));
    if (enable) {
        /* Turn off TX/RX enable */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, lport, 0));
    }
    
    /* if (soc_feature(unit, soc_feature_hg2_light_in_portmacro)) */
    /* mode update for all encap mode change! */
    SOC_IF_ERROR_RETURN(_mac_xl_port_mode_update(unit, lport, mode));
    
    /* Update the encapsulation mode */
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_MODEr(unit, lport, xlmac_mode) );
    XLMAC_MODEr_HDR_MODEf_SET(xlmac_mode, encap);
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_MODEr(unit, lport, xlmac_mode) );
    
    SOC_IF_ERROR_RETURN(
        READ_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));
    XLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(xlmac_rx_ctrl, (mode == SOC_ENCAP_IEEE ? 1 : 0) );
    SOC_IF_ERROR_RETURN(
        WRITE_XLMAC_RX_CTRLr(unit, lport, xlmac_rx_ctrl));        
        
    if (enable) {
        /* Re-enable transmitter and receiver */
        SOC_IF_ERROR_RETURN(
            mac_xl_enable_set(unit, lport, 1));
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_xl_encap_get
 * Purpose:
 *      Get the XLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (INT) encap bits (defined above)
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_encap_get(int unit, uint8 lport, int *mode)
{
    XLMAC_MODEr_t   xlmac_mode;

    if (!mode) {
        return SYS_ERR;
    }

    SOC_IF_ERROR_RETURN(READ_XLMAC_MODEr(unit, lport, xlmac_mode));
    switch (XLMAC_MODEr_HDR_MODEf_GET(xlmac_mode)) {
    case 0:
        *mode = SOC_ENCAP_IEEE;
        break;
    case 1:
        *mode = SOC_ENCAP_HIGIG;
        break;
    case 2:
        *mode = SOC_ENCAP_HIGIG2;
        break;
    default:
        *mode = SOC_ENCAP_COUNT;
    }


    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d encapsulation=%s\n", __func__, unit, lport, mac_xl_encap_mode[*mode] ));
    
    return SYS_OK;
}
/*
 * Function:
 *      mac_xl_expected_rx_latency_get
 * Purpose:
 *      Get the XLMAC port expected Rx latency
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      latency - (OUT) Latency in NS
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_expected_rx_latency_get(int unit, uint8 lport, int *latency)
{
    int speed = 0;
                            
    SOC_IF_ERROR_RETURN(mac_xl_speed_get(unit, lport, &speed));

    switch (speed) {
    case 1000:  /* GigE */
        *latency = 510; /* From SDK-69340 */
        break;

    case 10000:  /* 10G */
        *latency = 230; /* From SDK-69340 */
        break;

    default:
        *latency = 0;
        break;
    }
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d *latency=%d\n", __func__, unit, lport,
                            *latency));
    

    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_control_set
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
static int
mac_xl_control_set(int unit, uint8 lport, soc_mac_control_t type,
                  int value)
{
    int rv;
    uint32 fval0;
#if UM_READBACK_DEBUG
    uint32 fval1;
#endif
    XLMAC_CTRLr_t xlmac_ctrl;
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d type=%d value=%d\n", __func__, unit, lport,
                            type, value));

    rv = SOC_E_NONE;
    switch (type) {
    case SOC_MAC_CONTROL_RX_SET:
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        XLMAC_CTRLr_RX_ENf_SET(xlmac_ctrl, (value ? 1 : 0));
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        
#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        value = XLMAC_CTRLr_RX_ENf_GET(xlmac_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_RX_SET: value=%d\n", __func__, value));
#endif
        break;
        
    case SOC_MAC_CONTROL_TX_SET:
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        XLMAC_CTRLr_TX_ENf_SET(xlmac_ctrl, (value ? 1 : 0));
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        
#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        value = XLMAC_CTRLr_TX_ENf_GET(xlmac_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_TX_SET: value=%d\n", __func__, value));
#endif
        break;
        
    case SOC_MAC_CONTROL_FRAME_SPACING_STRETCH:
        {
            XLMAC_TX_CTRLr_t    xlmac_tx_ctrl;
            
            if (value < 0 || value > 255) {
                return SYS_ERR;
            } else {
                SOC_IF_ERROR_RETURN(READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
                
                if (value >= 8) {
                    XLMAC_TX_CTRLr_THROT_DENOMf_SET(xlmac_tx_ctrl, value);
                    XLMAC_TX_CTRLr_THROT_NUMf_SET(xlmac_tx_ctrl, 1);
                } else {
                    XLMAC_TX_CTRLr_THROT_DENOMf_SET(xlmac_tx_ctrl, 0);
                    XLMAC_TX_CTRLr_THROT_NUMf_SET(xlmac_tx_ctrl, 0);
                }
                SOC_IF_ERROR_RETURN(WRITE_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
                
#if UM_READBACK_DEBUG
                /* read back for debug */
                SOC_IF_ERROR_RETURN(READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
                value = XLMAC_TX_CTRLr_THROT_DENOMf_GET(xlmac_tx_ctrl);
                LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_FRAME_SPACING_STRETCH: THROT_DENOM value=%d\n", __func__, value));
                value = XLMAC_TX_CTRLr_THROT_NUMf_GET(xlmac_tx_ctrl);
                LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_FRAME_SPACING_STRETCH: THROT_NUM value=%d\n", __func__, value));
#endif
            }
            break;
        }
    
    case SOC_MAC_PASS_CONTROL_FRAME:
        {
            XLMAC_PAUSE_CTRLr_t     xlmac_pause_ctrl;
            
            SOC_IF_ERROR_RETURN(
                READ_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));    
            XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(xlmac_pause_ctrl, (value ? 0 : 1) );
            XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(xlmac_pause_ctrl, (value ? 0 : 1) );    
            SOC_IF_ERROR_RETURN(
                WRITE_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));
        
#if UM_READBACK_DEBUG
                /* read back for debug */
            SOC_IF_ERROR_RETURN(
                READ_XLMAC_PAUSE_CTRLr(unit, lport, xlmac_pause_ctrl));
            value = XLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_GET(xlmac_pause_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_PASS_CONTROL_FRAME: RX_PAUSE_EN value=%d\n", __func__, value));
            value = XLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_GET(xlmac_pause_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_PASS_CONTROL_FRAME: TX_PAUSE_EN value=%d\n", __func__, value));
#endif
        }
        break;
        
    case SOC_MAC_CONTROL_PFC_TYPE:
        {
            XLMAC_PFC_TYPEr_t   xlmac_pfc_type;
            
            SOC_IF_ERROR_RETURN(
                READ_XLMAC_PFC_TYPEr(unit, lport, xlmac_pfc_type));
            XLMAC_PFC_TYPEr_PFC_ETH_TYPEf_SET(xlmac_pfc_type, value);
            SOC_IF_ERROR_RETURN(
                WRITE_XLMAC_PFC_TYPEr(unit, lport, xlmac_pfc_type));
            
#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_TYPEr(unit, lport, xlmac_pfc_type));
            value = XLMAC_PFC_TYPEr_PFC_ETH_TYPEf_GET(xlmac_pfc_type);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_TYPE: value=%d\n", __func__, value));
#endif
        }
        break;
    
    case SOC_MAC_CONTROL_PFC_OPCODE:
        {
            XLMAC_PFC_OPCODEr_t     xlmac_pfc_opcode;
            
            SOC_IF_ERROR_RETURN(
                READ_XLMAC_PFC_OPCODEr(unit, lport, xlmac_pfc_opcode));
            XLMAC_PFC_OPCODEr_PFC_OPCODEf_SET(xlmac_pfc_opcode, value);
            SOC_IF_ERROR_RETURN(
                WRITE_XLMAC_PFC_OPCODEr(unit, lport, xlmac_pfc_opcode));
            
#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_OPCODEr(unit, lport, xlmac_pfc_opcode));
            value = XLMAC_PFC_OPCODEr_PFC_OPCODEf_GET(xlmac_pfc_opcode);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_OPCODE: value=%d\n", __func__, value));
#endif
        }
        break;
        
    case SOC_MAC_CONTROL_PFC_CLASSES:
        if (value != 8) {
            return SYS_ERR;
        }
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_OUI:
        {
            XLMAC_PFC_DAr_t     xlmac_pfc_da;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
            fval0 = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da);
            fval0 &= 0x00ffffff;
            fval0 |= (value & 0xff) << 24;
            XLMAC_PFC_DAr_PFC_MACDA_LOf_SET(xlmac_pfc_da, fval0);
            XLMAC_PFC_DAr_PFC_MACDA_HIf_SET(xlmac_pfc_da, (value >> 8));
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
            
#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
            fval0 = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da);
            fval1 = XLMAC_PFC_DAr_PFC_MACDA_HIf_GET(xlmac_pfc_da);
            value = (fval0 >> 24) | (fval1 << 8);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_MAC_DA_OUI: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI:
        {
            XLMAC_PFC_DAr_t     xlmac_pfc_da;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
            fval0 = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da) & 0xff000000;
            fval0 |= value;
            XLMAC_PFC_DAr_PFC_MACDA_LOf_SET(xlmac_pfc_da, fval0);
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
            
#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
            value = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da) & 0x00ffffff;
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI: value=0x%08x\n", __func__, value));
#endif      
        }
        break;

    case SOC_MAC_CONTROL_PFC_RX_PASS:
        /* this is always true */
        break;
        
    case SOC_MAC_CONTROL_PFC_RX_ENABLE:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            XLMAC_PFC_CTRLr_RX_PFC_ENf_SET(xlmac_pfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            value = XLMAC_PFC_CTRLr_RX_PFC_ENf_GET(xlmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_RX_ENABLE: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_TX_ENABLE:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            XLMAC_PFC_CTRLr_TX_PFC_ENf_SET(xlmac_pfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            value = XLMAC_PFC_CTRLr_TX_PFC_ENf_GET(xlmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_TX_ENABLE: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_FORCE_XON:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            XLMAC_PFC_CTRLr_FORCE_PFC_XONf_SET(xlmac_pfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            value = XLMAC_PFC_CTRLr_FORCE_PFC_XONf_GET(xlmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_FORCE_XON: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_STATS_ENABLE:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            XLMAC_PFC_CTRLr_PFC_STATS_ENf_SET(xlmac_pfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            value = XLMAC_PFC_CTRLr_PFC_STATS_ENf_GET(xlmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_STATS_ENABLE: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_REFRESH_TIME:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            XLMAC_PFC_CTRLr_PFC_REFRESH_TIMERf_SET(xlmac_pfc_ctrl, value);
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            value = XLMAC_PFC_CTRLr_PFC_REFRESH_TIMERf_GET(xlmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_REFRESH_TIME: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_XOFF_TIME:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            XLMAC_PFC_CTRLr_PFC_XOFF_TIMERf_SET(xlmac_pfc_ctrl, value);
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            value = XLMAC_PFC_CTRLr_PFC_XOFF_TIMERf_GET(xlmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_XOFF_TIME: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_LLFC_RX_ENABLE:
        {
            XLMAC_LLFC_CTRLr_t   xlmac_llfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
            XLMAC_LLFC_CTRLr_RX_LLFC_ENf_SET(xlmac_llfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
            value = XLMAC_LLFC_CTRLr_RX_LLFC_ENf_GET(xlmac_llfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_XOFF_TIME: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_LLFC_TX_ENABLE:
        {
            XLMAC_LLFC_CTRLr_t   xlmac_llfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
            XLMAC_LLFC_CTRLr_TX_LLFC_ENf_SET(xlmac_llfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));            
            value = XLMAC_LLFC_CTRLr_TX_LLFC_ENf_GET(xlmac_llfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_LLFC_TX_ENABLE: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_EEE_ENABLE:
        {
            XLMAC_EEE_CTRLr_t   xlmac_eee_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_CTRLr(unit, lport, xlmac_eee_ctrl));
            XLMAC_EEE_CTRLr_EEE_ENf_SET(xlmac_eee_ctrl, value);
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_EEE_CTRLr(unit, lport, xlmac_eee_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_CTRLr(unit, lport, xlmac_eee_ctrl));
            value = XLMAC_EEE_CTRLr_EEE_ENf_GET(xlmac_eee_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_LLFC_TX_ENABLE: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_EEE_TX_IDLE_TIME:
        {
            XLMAC_EEE_TIMERSr_t   xlmac_eee_timers;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
            XLMAC_EEE_TIMERSr_EEE_DELAY_ENTRY_TIMERf_SET(xlmac_eee_timers, value);
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
            value = XLMAC_EEE_TIMERSr_EEE_DELAY_ENTRY_TIMERf_GET(xlmac_eee_timers);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_EEE_TX_IDLE_TIME: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_EEE_TX_WAKE_TIME:
        {
            XLMAC_EEE_TIMERSr_t   xlmac_eee_timers;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
            XLMAC_EEE_TIMERSr_EEE_WAKE_TIMERf_SET(xlmac_eee_timers, value);
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
            value = XLMAC_EEE_TIMERSr_EEE_WAKE_TIMERf_GET(xlmac_eee_timers);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_EEE_TX_WAKE_TIME: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE:
        {
            XLMAC_RX_LSS_CTRLr_t   xlmac_rx_lss_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
            XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, (value ? 0 : 1));
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
            /* If set, MAC will continue to transmit data irrespective of LOCAL_FAULT_STATUS. */
            value = XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_GET(xlmac_rx_lss_ctrl) ? 0 : 1;
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE: value=0x%08x\n", __func__, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE:
        {
            XLMAC_RX_LSS_CTRLr_t   xlmac_rx_lss_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
            XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_SET(xlmac_rx_lss_ctrl, (value ? 0 : 1));
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
            /* If set, MAC will continue to transmit data irrespective of REMOTE_FAULT_STATUS. */
            value = XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_GET(xlmac_rx_lss_ctrl) ? 0 : 1;
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE: value=0x%08x\n", __func__, value));
#endif
        }
        break;
    
    case SOC_MAC_CONTROL_EGRESS_DRAIN:
        SOC_IF_ERROR_RETURN(mac_xl_egress_queue_drain(unit, lport));
        break;
        
    case SOC_MAC_CONTROL_FAILOVER_RX_SET:
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:do nothing...: SOC_MAC_CONTROL_FAILOVER_RX_SET type=%d\n", __func__, type));
        break;
    
    case SOC_MAC_CONTROL_RX_VLAN_TAG_OUTER_TPID:
        {
            XLMAC_RX_VLAN_TAGr_t    xlmac_rx_vlan_tag;
            
            SOC_IF_ERROR_RETURN(
                READ_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
            XLMAC_RX_VLAN_TAGr_OUTER_VLAN_TAGf_SET(xlmac_rx_vlan_tag, value);
            XLMAC_RX_VLAN_TAGr_OUTER_VLAN_TAG_ENABLEf_SET(xlmac_rx_vlan_tag, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(
                WRITE_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
        }
        break;
    case SOC_MAC_CONTROL_RX_VLAN_TAG_INNER_TPID:
        {
            XLMAC_RX_VLAN_TAGr_t    xlmac_rx_vlan_tag;
            
            SOC_IF_ERROR_RETURN(
                READ_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
            XLMAC_RX_VLAN_TAGr_INNER_VLAN_TAGf_SET(xlmac_rx_vlan_tag, value);
            XLMAC_RX_VLAN_TAGr_INNER_VLAN_TAG_ENABLEf_SET(xlmac_rx_vlan_tag, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(
                WRITE_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
        }
        break;
        
    default:
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:no such type: type=%d\n", __func__, type));
        return SOC_E_UNAVAIL;
    
    }
    
    return rv;
}

/*
 * Function:
 *      mac_xl_control_get
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
static int
mac_xl_control_get(int unit, uint8 lport, soc_mac_control_t type,
                  int *value)
{
    int rv;
    uint32 fval0, fval1;
    XLMAC_CTRLr_t xlmac_ctrl;

    if (value == NULL) {
        return SOC_E_PARAM;
    }

    rv = SOC_E_NONE;
    switch (type) {
    case SOC_MAC_CONTROL_RX_SET:
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        *value = XLMAC_CTRLr_RX_ENf_GET(xlmac_ctrl);
        
        break;
    case SOC_MAC_CONTROL_TX_SET:
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, lport, xlmac_ctrl));
        *value = XLMAC_CTRLr_TX_ENf_GET(xlmac_ctrl);
        
        break;
    case SOC_MAC_CONTROL_FRAME_SPACING_STRETCH:
        {
            XLMAC_TX_CTRLr_t    xlmac_tx_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_TX_CTRLr(unit, lport, xlmac_tx_ctrl));
            *value = XLMAC_TX_CTRLr_THROT_DENOMf_GET(xlmac_tx_ctrl);
            
            break;
        }
    case SOC_MAC_CONTROL_TIMESTAMP_TRANSMIT:
        {
            XLMAC_TX_TIMESTAMP_FIFO_STATUSr_t   xlmac_tx_timestamp_fifo_status;
            XLMAC_TX_TIMESTAMP_FIFO_DATAr_t     xlmac_tx_timestamp_fifo_data;
         
            SOC_IF_ERROR_RETURN
                (READ_XLMAC_TX_TIMESTAMP_FIFO_STATUSr(unit, lport, xlmac_tx_timestamp_fifo_status));
            if( XLMAC_TX_TIMESTAMP_FIFO_STATUSr_ENTRY_COUNTf_GET(xlmac_tx_timestamp_fifo_status)  == 0){
                return SYS_ERR;
            }
            
            SOC_IF_ERROR_RETURN
                (READ_XLMAC_TX_TIMESTAMP_FIFO_DATAr(unit, lport, xlmac_tx_timestamp_fifo_data));
            *value = XLMAC_TX_TIMESTAMP_FIFO_DATAr_TIME_STAMPf_GET(xlmac_tx_timestamp_fifo_data);    
        }
        break;

    case SOC_MAC_PASS_CONTROL_FRAME:
        *value = TRUE;
        break;

    case SOC_MAC_CONTROL_PFC_TYPE:
        {
            XLMAC_PFC_TYPEr_t   xlmac_pfc_type;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_TYPEr(unit, lport, xlmac_pfc_type));
            *value = XLMAC_PFC_TYPEr_PFC_ETH_TYPEf_GET(xlmac_pfc_type);
        }
        break;
    
    case SOC_MAC_CONTROL_PFC_OPCODE:
        {
            XLMAC_PFC_OPCODEr_t     xlmac_pfc_opcode;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_OPCODEr(unit, lport, xlmac_pfc_opcode));
            *value = XLMAC_PFC_OPCODEr_PFC_OPCODEf_GET(xlmac_pfc_opcode);
        }
        break;
        
    case SOC_MAC_CONTROL_PFC_CLASSES:
        *value = 8;
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_OUI:
        {
            XLMAC_PFC_DAr_t     xlmac_pfc_da;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
            fval0 = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da);
            fval1 = XLMAC_PFC_DAr_PFC_MACDA_HIf_GET(xlmac_pfc_da);
            *value = (fval0 >> 24) | (fval1 << 8);
        }
        break;


    case SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI:
        {
            XLMAC_PFC_DAr_t     xlmac_pfc_da;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, lport, xlmac_pfc_da));
            *value = XLMAC_PFC_DAr_PFC_MACDA_LOf_GET(xlmac_pfc_da) & 0x00ffffff;
        }
        break;

    case SOC_MAC_CONTROL_PFC_RX_PASS:
        *value = TRUE;
        break;

    case SOC_MAC_CONTROL_PFC_RX_ENABLE:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            *value = XLMAC_PFC_CTRLr_RX_PFC_ENf_GET(xlmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_TX_ENABLE:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            *value = XLMAC_PFC_CTRLr_TX_PFC_ENf_GET(xlmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_FORCE_XON:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            *value = XLMAC_PFC_CTRLr_FORCE_PFC_XONf_GET(xlmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_STATS_ENABLE:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            *value = XLMAC_PFC_CTRLr_PFC_STATS_ENf_GET(xlmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_REFRESH_TIME:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            *value = XLMAC_PFC_CTRLr_PFC_REFRESH_TIMERf_GET(xlmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_XOFF_TIME:
        {
            XLMAC_PFC_CTRLr_t   xlmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, lport, xlmac_pfc_ctrl));
            *value = XLMAC_PFC_CTRLr_PFC_XOFF_TIMERf_GET(xlmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_LLFC_RX_ENABLE:
        {
            XLMAC_LLFC_CTRLr_t   xlmac_llfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
            *value = XLMAC_LLFC_CTRLr_RX_LLFC_ENf_GET(xlmac_llfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_LLFC_TX_ENABLE:
        {
            XLMAC_LLFC_CTRLr_t   xlmac_llfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_LLFC_CTRLr(unit, lport, xlmac_llfc_ctrl));
            *value = XLMAC_LLFC_CTRLr_TX_LLFC_ENf_GET(xlmac_llfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_EEE_ENABLE:
        {
            XLMAC_EEE_CTRLr_t   xlmac_eee_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_CTRLr(unit, lport, xlmac_eee_ctrl));
            *value = XLMAC_EEE_CTRLr_EEE_ENf_GET(xlmac_eee_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_EEE_TX_IDLE_TIME:
        {
            XLMAC_EEE_TIMERSr_t   xlmac_eee_timers;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
            *value = XLMAC_EEE_TIMERSr_EEE_DELAY_ENTRY_TIMERf_GET(xlmac_eee_timers);
        }
        break;

    case SOC_MAC_CONTROL_EEE_TX_WAKE_TIME:
        {
            XLMAC_EEE_TIMERSr_t   xlmac_eee_timers;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_TIMERSr(unit, lport, xlmac_eee_timers));
            *value = XLMAC_EEE_TIMERSr_EEE_WAKE_TIMERf_GET(xlmac_eee_timers);
        }
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE:
        {
            XLMAC_RX_LSS_CTRLr_t   xlmac_rx_lss_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
            /* If set, MAC will continue to transmit data irrespective of LOCAL_FAULT_STATUS. */
            *value = XLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_GET(xlmac_rx_lss_ctrl) ? 0 : 1;
        }
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_STATUS:
        {
            XLMAC_RX_LSS_STATUSr_t   xlmac_rx_lss_status;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_STATUSr(unit, lport, xlmac_rx_lss_status));
            *value = XLMAC_RX_LSS_STATUSr_LOCAL_FAULT_STATUSf_GET(xlmac_rx_lss_status);
        }
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE:
        {
            XLMAC_RX_LSS_CTRLr_t   xlmac_rx_lss_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_CTRLr(unit, lport, xlmac_rx_lss_ctrl));
            /* If set, MAC will continue to transmit data irrespective of REMOTE_FAULT_STATUS. */
            *value = XLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_GET(xlmac_rx_lss_ctrl) ? 0 : 1;
        }
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_STATUS:
        {
            XLMAC_RX_LSS_STATUSr_t   xlmac_rx_lss_status;
        
            SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_STATUSr(unit, lport, xlmac_rx_lss_status));
            *value = XLMAC_RX_LSS_STATUSr_REMOTE_FAULT_STATUSf_GET(xlmac_rx_lss_status);
        }
        break;
    
    case SOC_MAC_CONTROL_RX_VLAN_TAG_OUTER_TPID:
        {
            XLMAC_RX_VLAN_TAGr_t    xlmac_rx_vlan_tag;
            
            SOC_IF_ERROR_RETURN(
                READ_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
            *value = XLMAC_RX_VLAN_TAGr_OUTER_VLAN_TAGf_GET(xlmac_rx_vlan_tag);
            
        }
        break;
        
    case SOC_MAC_CONTROL_RX_VLAN_TAG_INNER_TPID:
        {
            XLMAC_RX_VLAN_TAGr_t    xlmac_rx_vlan_tag;
            
            SOC_IF_ERROR_RETURN(
                READ_XLMAC_RX_VLAN_TAGr(unit, lport, xlmac_rx_vlan_tag));
            *value = XLMAC_RX_VLAN_TAGr_INNER_VLAN_TAGf_GET(xlmac_rx_vlan_tag);
            
        }
        break;
    case SOC_MAC_CONTROL_EXPECTED_RX_LATENCY:
        {
            SOC_IF_ERROR_RETURN(mac_xl_expected_rx_latency_get(unit, lport, value));
        }
        break;
    default:
        return SOC_E_UNAVAIL;
    
    }
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d type=%d *value=%d\n", __func__, unit, lport,
                            type, *value));
    return rv;
}

/*
 * Function:
 *      mac_xl_ability_local_get
 * Purpose:
 *      Return the abilities of XLMAC
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (OUT) Supported operating modes as a mask of abilities.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_xl_ability_local_get(int unit, uint8 lport,
                          soc_port_ability_t *ability)
{
    //int blk;
    int bindex, port_speed_max, i;
    int phy_port, active_port;
    uint32 active_mask;    
    //LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:\n", __func__));

    if (NULL == ability) {
        return SYS_ERR;
    }

    ability->speed_half_duplex  = SOC_PA_ABILITY_NONE;
    ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->interface = SOC_PA_INTF_MII | SOC_PA_INTF_XGMII;
    ability->medium    = SOC_PA_ABILITY_NONE;
    ability->loopback  = SOC_PA_LB_MAC;
    ability->flags     = SOC_PA_ABILITY_NONE;
    ability->encap = SOC_PA_ENCAP_IEEE | SOC_PA_ENCAP_HIGIG |
        SOC_PA_ENCAP_HIGIG2;

        
    /* Adjust port_speed_max according to the port config */
    port_speed_max = SOC_PORT_SPEED_MAX(lport);
    {
        phy_port = SOC_PORT_L2P_MAPPING(lport);
        bindex = -1;
        if (IS_XL_PORT(lport)){
            bindex = bcm5357x_xlport_pport_to_index_in_block[phy_port];
        }
        
        
        if (port_speed_max > 10000) {
            active_mask = 0;
            for (i = bindex + 1; i <= 3; i++) {
                active_port = SOC_PORT_P2L_MAPPING(phy_port - bindex + i);
                /* (active_port != -1 means it's not disabled.) */
                if (active_port != -1  /* &&
                    !SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit, all),
                                     active_port) */) {
                    active_mask |= 1 << i;
                }
            }
            if (bindex == 0) { /* Lanes 0 */
                if (active_mask & 0x2) { /* lane 1 is in use */
                    port_speed_max = 10000;
                } else if (port_speed_max > 20000 && active_mask & 0xc) {
                    /* Lane 1 isn't in use, lane 2 or 3 (or both) is (are) in use */
                    port_speed_max = 20000;
                }
            } else { /* (Must be) lanes 2 */
                if (active_mask & 0x8) { /* lane 3 is in use */
                    port_speed_max = 10000;
                }
            }
        }
    }
    
    /* Use current number of lanes per port to determine the supported speeds */
    if (IS_HL_PORT(lport)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:error : IS_HL_PORT.\n", __func__));
    } else if (IS_HG_PORT(lport)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:error : IS_HG_PORT.\n", __func__));
    }
    else {
        if (port_speed_max >= 40000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_40GB;
        }
        if (port_speed_max >= 20000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_20GB;
        }
        if (port_speed_max >= 10000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_10GB;
        }
        if (port_speed_max >= 5000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_5000MB;
            /* For 5G speed, MAC will actually be set to 10G */
        }
        /* Temp fix running regression, saber2 check 
         * In Saber2, the xlmac is used for MXQ ports as well. 
         */
        //if (soc_feature(unit, soc_feature_unified_port) || (SOC_IS_SABER2(unit))) 
        {
            if (port_speed_max >= 2500) {
                ability->speed_full_duplex |= SOC_PA_SPEED_2500MB;
            }
            if (port_speed_max >= 1000) {
                ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
            }
            if (port_speed_max >= 100) {
                ability->speed_full_duplex |= SOC_PA_SPEED_100MB;
            }
            if (port_speed_max >= 10) {
                ability->speed_full_duplex |= SOC_PA_SPEED_10MB;
            }
        }
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:mac_xl_ability_local_get: unit %d lport=%d "
            "speed_half=0x%x speed_full=0x%x encap=0x%x pause=0x%x "
            "interface=0x%x medium=0x%x loopback=0x%x flags=0x%x\n", __func__,
            unit, lport,
            ability->speed_half_duplex, ability->speed_full_duplex,
            ability->encap, ability->pause, ability->interface,
            ability->medium, ability->loopback, ability->flags
            ));
    
    return SYS_OK;
}

/* Exported XLMAC driver structure */
mac_driver_t soc_mac_xl = {
    "XLMAC Driver",               /* drv_name */
    .md_init = mac_xl_init,                  /* md_init  */
    .md_enable_set = mac_xl_enable_set,            /* md_enable_set */
    .md_enable_get = mac_xl_enable_get,            /* md_enable_get */
    .md_duplex_set = mac_xl_duplex_set,            /* md_duplex_set */
    .md_duplex_get = mac_xl_duplex_get,            /* md_duplex_get */
    .md_speed_set = mac_xl_speed_set,             /* md_speed_set */
    .md_speed_get = mac_xl_speed_get,             /* md_speed_get */
    .md_pause_set = mac_xl_pause_set,             /* md_pause_set */
    .md_pause_get = mac_xl_pause_get,             /* md_pause_get */
    .md_pause_addr_set = mac_xl_pause_addr_set,        /* md_pause_addr_set */
    .md_pause_addr_get = mac_xl_pause_addr_get,        /* md_pause_addr_get */
    .md_lb_set = mac_xl_loopback_set,          /* md_lb_set */
    .md_lb_get = mac_xl_loopback_get,          /* md_lb_get */
    .md_interface_set = mac_xl_interface_set,         /* md_interface_set */
    .md_interface_get = mac_xl_interface_get,         /* md_interface_get */
    .md_ability_get = NULL,                         /* md_ability_get - Deprecated */
    .md_frame_max_set = mac_xl_frame_max_set,         /* md_frame_max_set */
    .md_frame_max_get = mac_xl_frame_max_get,         /* md_frame_max_get */
    .md_ifg_set = mac_xl_ifg_set,               /* md_ifg_set */
    .md_ifg_get = mac_xl_ifg_get,               /* md_ifg_get */
    .md_encap_set = mac_xl_encap_set,             /* md_encap_set */
    .md_encap_get = mac_xl_encap_get,             /* md_encap_get */
    .md_control_set = mac_xl_control_set,           /* md_control_set */
    .md_control_get = mac_xl_control_get,           /* md_control_get */
    .md_ability_local_get = mac_xl_ability_local_get      /* md_ability_local_get */
 };

