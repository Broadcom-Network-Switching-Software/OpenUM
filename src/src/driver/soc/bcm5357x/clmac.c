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

//#define BSL_LS_SOC_CLMAC_DEBUG    
//#define UM_READBACK_DEBUG

#if defined(LOG_VERBOSE) && defined(BSL_LS_SOC_CLMAC_DEBUG)
/* Outout the debug message with LOG_VERBOSE() */
static char *mac_cl_encap_mode[] = SOC_ENCAP_MODE_NAMES_INITIALIZER;
//static char *mac_cl_port_if_names[] = SOC_PORT_IF_NAMES_INITIALIZER;
#else
#undef LOG_VERBOSE
#define LOG_VERBOSE(ls_, stuff_)        
#endif


#define UM_PHY_NOTIFY  1
/*
 * Forward Declarations
 */
mac_driver_t soc_mac_cl;
    

#define CLMAC_RUNT_THRESHOLD_IEEE  0x40   /* Runt threshold value for XE ports */
#define CLMAC_RUNT_THRESHOLD_HG1   0x48   /* Runt threshold value for HG1 ports */
#define CLMAC_RUNT_THRESHOLD_HG2   0x4c   /* Runt threshold value for HG2 ports */

#define JUMBO_MAXSZ         0x3fe8 /* Max legal value (per regsfile) */

#define SOC_CLMAC_SPEED_1000   0x2
#define SOC_CLMAC_SPEED_100000 0x4

/*
 * CLMAC timestamp delay definitions
 */
#define CLMAC_TX_LINE_RATE_NS   0x3

/* Transmit CRC Modes */
#define CLMAC_CRC_APPEND        0x0
#define CLMAC_CRC_KEEP          0x1
#define CLMAC_CRC_REPLACE       0x2
#define CLMAC_CRC_PER_PKT_MODE  0x3

#define FD_XE_IPG   96
#define FD_HG_IPG   64

#define _SHR_E_SUCCESS(rv)              ((rv) >= 0)
#define _SHR_E_FAILURE(rv)              ((rv) < 0)
//#define SOC_SUCCESS(rv)         _SHR_E_SUCCESS(rv)
#define SOC_E_MEMORY            SYS_ERR_OUT_OF_RESOURCE


#if defined(_BCM5357X_)
extern int soc_gh2_64q_port_check(int unit, uint8 lport, int *is_64q_port);
extern int soc_gh2_mmu_bucket_qlayer_index(int unit, int p, int q, int *idx);
extern int soc_gh2_mmu_bucket_qgroup_index(int unit, int p, int g, int *idx);
#else
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

/* Forwards */
static int 
soc_port_credit_reset(int unit, uint8 lport)
{
    int phy_port;
    int bindex;
    PGW_CL_TXFIFO_CTRLr_t   pgw_cl_txfifo_ctrl;
    CLPORT_ENABLE_REGr_t    clport_enable_reg;
    EGR_PORT_CREDIT_RESETm_t    egr_port_credit_reset;
    
    /* Should only be called by XLMAC/CLMAC driver */
    //if (!IS_XL_PORT(lport) && !IS_CL_PORT(unit, port)) {
    if (!IS_CL_PORT(lport)) {
        return SYS_OK;
    }
    
    phy_port = SOC_PORT_L2P_MAPPING(lport);
    bindex = bcm5357x_clport_pport_to_index_in_block[phy_port];
    
    SOC_IF_ERROR_RETURN(
        READ_PGW_CL_TXFIFO_CTRLr(unit, lport, pgw_cl_txfifo_ctrl) );
    SOC_IF_ERROR_RETURN(
        READ_CLPORT_ENABLE_REGr(unit, lport, clport_enable_reg) );
    
    if(bindex == 0)
        CLPORT_ENABLE_REGr_PORT0f_SET(clport_enable_reg,0);
    else if(bindex == 1)
        CLPORT_ENABLE_REGr_PORT1f_SET(clport_enable_reg,0);
    else if(bindex == 2)
        CLPORT_ENABLE_REGr_PORT2f_SET(clport_enable_reg,0);
    else if(bindex == 3)
        CLPORT_ENABLE_REGr_PORT3f_SET(clport_enable_reg,0);
    
    SOC_IF_ERROR_RETURN(
        WRITE_CLPORT_ENABLE_REGr(unit, lport, clport_enable_reg) );
    
    EGR_PORT_CREDIT_RESETm_CLR(egr_port_credit_reset);
    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 1);
    /* To clear the port credit(per physical port) */
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_PORT_CREDIT_RESETm(unit, SOC_PORT_L2P_MAPPING(lport), egr_port_credit_reset) );
        
    PGW_CL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(pgw_cl_txfifo_ctrl, 1);
    PGW_CL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(pgw_cl_txfifo_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_PGW_CL_TXFIFO_CTRLr(unit, lport, pgw_cl_txfifo_ctrl) );
    
    sal_usleep(1000);

    EGR_PORT_CREDIT_RESETm_VALUEf_SET(egr_port_credit_reset, 0);
    /* To clear the port credit(per physical port) */
    SOC_IF_ERROR_RETURN(
        WRITE_EGR_PORT_CREDIT_RESETm(unit, SOC_PORT_L2P_MAPPING(lport), egr_port_credit_reset) );
    
    
    PGW_CL_TXFIFO_CTRLr_MAC_CLR_COUNTf_SET(pgw_cl_txfifo_ctrl, 0);
    PGW_CL_TXFIFO_CTRLr_CORE_CLR_COUNTf_SET(pgw_cl_txfifo_ctrl, 0);
    SOC_IF_ERROR_RETURN(
        WRITE_PGW_CL_TXFIFO_CTRLr(unit, lport, pgw_cl_txfifo_ctrl) );
        
    if(bindex == 0)
        CLPORT_ENABLE_REGr_PORT0f_SET(clport_enable_reg,1);
    else if(bindex == 1)
        CLPORT_ENABLE_REGr_PORT1f_SET(clport_enable_reg,1);
    else if(bindex == 2)
        CLPORT_ENABLE_REGr_PORT2f_SET(clport_enable_reg,1);
    else if(bindex == 3)
        CLPORT_ENABLE_REGr_PORT3f_SET(clport_enable_reg,1);
    SOC_IF_ERROR_RETURN(
        WRITE_CLPORT_ENABLE_REGr(unit, lport, clport_enable_reg) );

    return SYS_OK;
}

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
    COSLCCOUNTr_t       coslccount;
#if defined(_BCM5357X_)
    COSLCCOUNT_QGROUPr_t    coslccount_qgroup;
#endif
    
    *count = 0;
    
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
soc_port_speed_update(int unit, uint8 lport, int speed) 
{
    /* Do nothing for GH2 */
    if (speed > SOC_PORT_SPEED_MAX(lport)) {
        return SYS_ERR_PARAMETER;
    }
    SOC_PORT_SPEED_INIT(lport) = speed;
    return SYS_OK;
}

static int
soc_port_egress_buffer_sft_reset(int unit, uint8 lport, int reset)
{
    /* Do nothing for GH2 */
    return SYS_OK;
}

static int
soc_port_ingress_buffer_reset(int unit, uint8 lport, int reset)
{
    /* Do nothing for GH2 */
    return SYS_OK;
}

#if 0
static int
soc_port_fifo_reset(int unit, uint8 lport)
{
    /* Do nothing for GH2 */
    return SYS_OK;
}
#endif

static int
soc_mmu_backpressure_clear(int unit, uint8 lport) 
{
    /* Do nothing for GH2 */
    return SYS_OK;
}


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

static int
soc_mmu_flush_enable(int unit, uint8 lport, int enable)
{
    int port_off;
    MMUFLUSHCONTROLr_t mmuflushcontrol;
    
    /* First put the port in flush state - the packets from the XQ of the 
     * port are purged after dequeue.
     */
#define USE_MMU_PORT    1
#ifdef USE_MMU_PORT
    lport = SOC_PORT_P2M_MAPPING( SOC_PORT_L2P_MAPPING(lport) );
#endif
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


static int
_mac_cl_drain_cells(int unit, uint8 lport, int notify_phy)
{
    int         rv;
    int         pause_tx = 0, pause_rx = 0, pfc_rx = 0, llfc_rx = 0;
    CLMAC_CTRLr_t   clmac_ctrl;
    CLMAC_TX_CTRLr_t    clmac_tx_ctrl;
    CLMAC_TXFIFO_CELL_CNTr_t    clmac_txfifo_cell_cnt;
    uint32      drain_timeout, fval;
    
    
#if CONFIG_EMULATION
    drain_timeout = 250000000;
#else
    drain_timeout = 250000;
#endif
        
    /* Drain cells in mmu/port before cells entering TX FIFO */
    SOC_IF_ERROR_RETURN
        (soc_mmu_flush_enable(unit, lport, TRUE) );
    
    /* Disable pause/pfc/llfc function */
    {
        SOC_IF_ERROR_RETURN
            (soc_mac_cl.md_pause_get(unit, lport, &pause_tx, &pause_rx));
        SOC_IF_ERROR_RETURN
            (soc_mac_cl.md_pause_set(unit, lport, pause_tx, 0));
    
        SOC_IF_ERROR_RETURN
            (soc_mac_cl.md_control_get(unit, lport, SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                       &pfc_rx));
        SOC_IF_ERROR_RETURN
            (soc_mac_cl.md_control_set(unit, lport, SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                       0));
    
        SOC_IF_ERROR_RETURN
            (soc_mac_cl.md_control_get(unit, lport, SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                       &llfc_rx));
        SOC_IF_ERROR_RETURN
            (soc_mac_cl.md_control_set(unit, lport, SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                       0));
    }                               
    
    
    {
        /* Assert SOFT_RESET before DISCARD just in case there is no credit left */
        SOC_IF_ERROR_RETURN(
            READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 1);
        /* Drain data in TX FIFO without egressing */
        SOC_IF_ERROR_RETURN(
            WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));
    }
    
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
    CLMAC_TX_CTRLr_DISCARDf_SET(clmac_tx_ctrl, 1);
    CLMAC_TX_CTRLr_EP_DISCARDf_SET(clmac_tx_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
    
    /* Reset EP credit before de-assert SOFT_RESET */
    SOC_IF_ERROR_RETURN(
        soc_port_credit_reset(unit, lport));

    {
        /* De-assert SOFT_RESET to let the drain start */
        SOC_IF_ERROR_RETURN(
            READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 0);
        /* Drain data in TX FIFO without egressing */
        SOC_IF_ERROR_RETURN(
            WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));
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
            rv =  READ_CLMAC_TXFIFO_CELL_CNTr(unit, lport, clmac_txfifo_cell_cnt);
            if (SOC_E_NONE != rv) {
                break;
            }
            
            fval = CLMAC_TXFIFO_CELL_CNTr_CELL_CNTf_GET(clmac_txfifo_cell_cnt);
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
        READ_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
    CLMAC_TX_CTRLr_DISCARDf_SET(clmac_tx_ctrl, 0);
    CLMAC_TX_CTRLr_EP_DISCARDf_SET(clmac_tx_ctrl, 0);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
    
    
    /* Restore config below only if modified above, i.e. h/w flush not used */
    /* Restore original pause/pfc/llfc configuration */
    {
        SOC_IF_ERROR_RETURN
            (soc_mac_cl.md_pause_set(unit, lport, pause_tx, pause_rx));
        SOC_IF_ERROR_RETURN
            (soc_mac_cl.md_control_set(unit, lport,
                                       SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                       pfc_rx));
        SOC_IF_ERROR_RETURN
            (soc_mac_cl.md_control_set(unit, lport,
                                       SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                       llfc_rx));
    }
    /* Stop draining cells in mmu/port */
    SOC_IF_ERROR_RETURN(soc_mmu_flush_enable(unit, lport, FALSE));

    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_init
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
mac_cl_init(int unit, uint8 lport)
{
    CLMAC_CTRLr_t   clmac_ctrl;
    CLMAC_MODEr_t   clmac_mode;
    CLMAC_RX_CTRLr_t    clmac_rx_ctrl;
    CLMAC_TX_CTRLr_t    clmac_tx_ctrl;
    CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
    CLMAC_RX_MAX_SIZEr_t    clmac_rx_max_size;
    CLMAC_RX_LSS_CTRLr_t   clmac_rx_lss_ctrl;
    CLPORT_MAC_RSV_MASKr_t clport_mac_rsv_mask;
    int ipg, runt;
    int encap = SOC_ENCAP_IEEE;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d\n", __func__, unit, lport));
       

    /* Disable Tx/Rx, assume that MAC is stable (or out of reset) */
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
    /* Reset EP credit before de-assert SOFT_RESET */
    if( CLMAC_CTRLr_SOFT_RESETf_GET(clmac_ctrl) ){
        SOC_IF_ERROR_RETURN(
            soc_port_credit_reset(unit, lport));
    }
        
    CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 0);
    CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, 0);
    CLMAC_CTRLr_TX_ENf_SET(clmac_ctrl, 0);
    /* no such field
    if (soc_reg_field_valid(unit, CLMAC_CTRLr, XLGMII_ALIGN_ENBf)) {
        soc_reg64_field32_set(unit, CLMAC_CTRLr, &mac_ctrl, XLGMII_ALIGN_ENBf,
                              1);
    }
    */
    CLMAC_CTRLr_XGMII_IPG_CHECK_DISABLEf_SET(clmac_ctrl, (IS_HG_PORT(lport) ? 1 : 0) );
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));
                
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
    ipg = IS_HG_PORT(lport)?  FD_HG_IPG:
                              FD_XE_IPG;
                              
    CLMAC_TX_CTRLr_AVERAGE_IPGf_SET(clmac_tx_ctrl, ((ipg / 8) & 0x1f) );
    CLMAC_TX_CTRLr_CRC_MODEf_SET(clmac_tx_ctrl, CLMAC_CRC_PER_PKT_MODE );
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
        
    if (IS_ST_PORT(lport)) {
        soc_mac_cl.md_pause_set(unit, lport, FALSE, FALSE);
    } else {
        soc_mac_cl.md_pause_set(unit, lport, TRUE, TRUE);
    }
        
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
    CLMAC_PFC_CTRLr_PFC_REFRESH_ENf_SET(clmac_pfc_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));

#if 0   
    if (soc_property_port_get(unit, port, spn_PHY_WAN_MODE, FALSE)) {
        /* Max speed for WAN mode is 9.294Gbps.
         * This setting gives 10Gbps * (13/14) or 9.286 Gbps */
        SOC_IF_ERROR_RETURN
            (soc_mac_cl.md_control_set(unit, port,
                                       SOC_MAC_CONTROL_FRAME_SPACING_STRETCH,
                                       13));
    }
#endif

    /* 1. Initialize mask for purging packet data received from the MAC 
       2. Diable length check */
    CLPORT_MAC_RSV_MASKr_CLR(clport_mac_rsv_mask);
    CLPORT_MAC_RSV_MASKr_MASKf_SET(clport_mac_rsv_mask, 0x58);
    WRITE_CLPORT_MAC_RSV_MASKr(unit, lport, clport_mac_rsv_mask);
    
    /* Set jumbo max size (16360 byte payload) */
    CLMAC_RX_MAX_SIZEr_CLR(clmac_rx_max_size);
    CLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(clmac_rx_max_size, JUMBO_MAXSZ);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_RX_MAX_SIZEr(unit, lport, clmac_rx_max_size));
    
    /* Setup header mode, check for property for higig2 mode */
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_MODEr(unit, lport, clmac_mode) );
#if 0
    if (IS_HG_PORT(unit, port)) {
        mode = soc_property_port_get(unit, port, spn_HIGIG2_HDR_MODE,
               soc_feature(unit, soc_feature_no_higig_plus) ? 1 : 0) ? 2 : 1;
        encap = (mode == 2) ? SOC_ENCAP_HIGIG2 : SOC_ENCAP_HIGIG;
        soc_reg64_field32_set(unit, CLMAC_MODEr, &rval, HDR_MODEf, mode);
    }
#endif
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_MODEr(unit, lport, clmac_mode) );
        
      
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));
    CLMAC_RX_CTRLr_STRIP_CRCf_SET(clmac_rx_ctrl, 0);
    CLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(clmac_rx_ctrl, SOC_PORT_SPEED_MAX(lport) >= 10000 &&
                          IS_CL_PORT(lport) &&
                          !IS_HG_PORT(lport) ? 1 : 0);
    /* assigning RUNT_THRESHOLD per header mode setting */
    runt = (encap == SOC_ENCAP_HIGIG2) ? CLMAC_RUNT_THRESHOLD_HG2 :
           ((encap == SOC_ENCAP_HIGIG) ? CLMAC_RUNT_THRESHOLD_HG1 :
                                         CLMAC_RUNT_THRESHOLD_IEEE);
    CLMAC_RX_CTRLr_RUNT_THRESHOLDf_SET(clmac_rx_ctrl, runt);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));
      
            
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_RX_LSS_CTRLr(unit, lport, clmac_rx_lss_ctrl));
    CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LOCAL_FAULTf_SET(clmac_rx_lss_ctrl, 1);
    CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_REMOTE_FAULTf_SET(clmac_rx_lss_ctrl, 1);
    CLMAC_RX_LSS_CTRLr_DROP_TX_DATA_ON_LINK_INTERRUPTf_SET(clmac_rx_lss_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_RX_LSS_CTRLr(unit, lport, clmac_rx_lss_ctrl));

    /* Disable loopback and bring CLMAC out of reset */
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
    CLMAC_CTRLr_LOCAL_LPBKf_SET(clmac_ctrl, 0);
    CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, 1);
    CLMAC_CTRLr_TX_ENf_SET(clmac_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));  
                  
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_egress_queue_drain
 * Purpose:
 *      Drain the egress queues with out bringing down the port
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_cl_egress_queue_drain(int unit, uint8 lport)
{
    int rx_enable = 0;
    int is_active = 0;
    CLMAC_CTRLr_t   clmac_ctrl, clmac_ctrl_orig;
    EPC_LINK_BMAP_HI_64r_t epc_link_bmap_hi_64;
    EPC_LINK_BMAP_LO_64r_t epc_link_bmap_lo_64;


    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d\n", __func__, unit, lport));

    SOC_IF_ERROR_RETURN(
        READ_CLMAC_CTRLr(unit, lport, clmac_ctrl) );
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_CTRLr(unit, lport, clmac_ctrl_orig) );
        
    rx_enable = CLMAC_CTRLr_RX_ENf_GET(clmac_ctrl);
    /* Don't disable TX since it stops egress and hangs if CPU sends */
    CLMAC_CTRLr_TX_ENf_SET(clmac_ctrl, 1);
    CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, 0);
    /* Disable RX */
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));

    /* Remove port from EPC_LINK */
    if (lport >= 64) {
		READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
		if( PBMP_MEMBER(*((pbmp_t *) &epc_link_bmap_hi_64), (lport-64)) ){
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
    
    /* Drain cells */
    SOC_IF_ERROR_RETURN(
        _mac_cl_drain_cells(unit, lport, 0));

    /* Put port into SOFT_RESET */
    CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 1);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));

    /* Reset EP credit before de-assert SOFT_RESET */
    SOC_IF_ERROR_RETURN(
        soc_port_credit_reset(unit, lport));
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
    CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, (rx_enable ? 1 : 0) );

    /* Enable both TX and RX, deassert SOFT_RESET */
    CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 0);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));

    /* Restore CLMAC_CTRL to original value */
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl_orig));        

    /* Add port to EPC_LINK */
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
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_enable_set
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
mac_cl_enable_set(int unit, uint8 lport, int enable)
{
    CLMAC_CTRLr_t   clmac_ctrl, clmac_ctrl_orig;
    EPC_LINK_BMAP_HI_64r_t epc_link_bmap_hi_64;
    EPC_LINK_BMAP_LO_64r_t epc_link_bmap_lo_64;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d enable=%d\n", __func__, unit, lport, enable));

    SOC_IF_ERROR_RETURN(
        READ_CLMAC_CTRLr(unit, lport, clmac_ctrl) );
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_CTRLr(unit, lport, clmac_ctrl_orig) );
        
    /* Don't disable TX since it stops egress and hangs if CPU sends */
    CLMAC_CTRLr_TX_ENf_SET(clmac_ctrl, 1);
    CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, enable ? 1 : 0);
    
    if (!sal_memcmp(&clmac_ctrl, &clmac_ctrl_orig, sizeof(clmac_ctrl_orig))) {
        if (enable) {
            return SYS_OK;
        } else {
            if (CLMAC_CTRLr_SOFT_RESETf_GET(clmac_ctrl)) {
                return SYS_OK;
            }
        }
    }
    
    if (enable) {
        /* Reset EP credit before de-assert SOFT_RESET */
        SOC_IF_ERROR_RETURN(
            soc_port_credit_reset(unit, lport));
            
        /* Deassert SOFT_RESET */
        CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 0);
        
        /* Deassert EGR_XLPORT_BUFFER_SFT_RESET */
        SOC_IF_ERROR_RETURN(
            soc_port_egress_buffer_sft_reset(unit, lport, 0));
            
        /* Release Ingress buffers from reset */
        SOC_IF_ERROR_RETURN(
            soc_port_ingress_buffer_reset(unit, lport, 0));
        SOC_IF_ERROR_RETURN(
            WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));
            
        /* SDK-113376 */
        /* Special handling for new mac version for TH+. Internally MAC loopback looks for rising */
        /* edge on MAC loopback configuration to enter loopback state */
        if(CLMAC_CTRLr_LOCAL_LPBKf_GET(clmac_ctrl)){    /* Do only if loopback bit is intented to be set */
            CLMAC_CTRLr_LOCAL_LPBKf_SET(clmac_ctrl, 0);
            WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl);
            sal_usleep(10);                             /* Wait 10usec as suggested by MAC designer */
            CLMAC_CTRLr_LOCAL_LPBKf_SET(clmac_ctrl, 1);
            WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl);
        }
            
        if (lport >= 64) {
		    READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
			PBMP_PORT_ADD(*((pbmp_t *) &epc_link_bmap_hi_64), (lport-64));
    		WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
	    } else {
            READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
            PBMP_PORT_ADD(*((pbmp_t *) &epc_link_bmap_lo_64), lport);
            WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
        }
        
#ifdef UM_PHY_NOTIFY
        SOC_IF_ERROR_RETURN(
            soc_phyctrl_notify(unit, lport, phyEventResume,
                                               PHY_STOP_MAC_DIS));
#endif
    } else {
        /* Disable MAC RX */
        SOC_IF_ERROR_RETURN(
            WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        
        if (lport >= 64) {
    		READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
			PBMP_PORT_REMOVE(*((pbmp_t *) &epc_link_bmap_hi_64), (lport-64));
    		WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64);
        } else {
            READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
            PBMP_PORT_REMOVE(*((pbmp_t *) &epc_link_bmap_lo_64), lport);
            WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64);
        }
        
        /* Delay to ensure EOP is received at Ingress */
        sal_usleep(1000);
        
        /* Reset Ingress buffers */
        SOC_IF_ERROR_RETURN(soc_port_ingress_buffer_reset(unit, lport, 1));
        SOC_IF_ERROR_RETURN(_mac_cl_drain_cells(unit, lport, 1));
        /* Reset egress_buffer */
        SOC_IF_ERROR_RETURN(soc_port_egress_buffer_sft_reset(unit, lport, 1));
        /* Put port into SOFT_RESET */
        CLMAC_CTRLr_SOFT_RESETf_SET(clmac_ctrl, 1);
        SOC_IF_ERROR_RETURN(
            WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));
#ifdef UM_PHY_NOTIFY
        SOC_IF_ERROR_RETURN(
            soc_phyctrl_notify(unit, lport, phyEventStop,
                                               PHY_STOP_MAC_DIS));
#endif
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_enable_get
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
mac_cl_enable_get(int unit, uint8 lport, int *enable)
{
    CLMAC_CTRLr_t   clmac_ctrl;

    SOC_IF_ERROR_RETURN(
        READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));

    *enable = CLMAC_CTRLr_RX_ENf_GET(clmac_ctrl);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d *enable=%d\n", __func__, unit, lport, *enable));

    return SYS_OK;
}

#if 0
/* This is for XLMAC and this routine is expected to be called
 * by speed set */
/*
 * Function:
 *      _mac_cl_timestamp_delay_set
 * Purpose:
 *      Set Timestamp delay for one-step to account for lane and pipeline delay.
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 *      speed - New port speed
 * Returns:
 *      SOC_E_XXX
 */
static int
_mac_cl_timestamp_delay_set(int unit, uint8 lport, int speed)
{
    return SYS_OK;
}
#endif

/*
 * Function:
 *      mac_cl_duplex_set
 * Purpose:
 *      Set CLMAC in the specified duplex mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      duplex - Boolean: true --> full duplex, false --> half duplex.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
static int
mac_cl_duplex_set(int unit, uint8 lport, int duplex)
{
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d duplex=%d\n", __func__, unit, lport, duplex));
                                
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_duplex_get
 * Purpose:
 *      Get CLMAC duplex mode.
 * Parameters:
 *      unit - XGS unit #.
 *      duplex - (OUT) Boolean: true --> full duplex, false --> half duplex.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_cl_duplex_get(int unit, uint8 lport, int *duplex)
{
    *duplex = TRUE; /* Always full duplex */

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d *duplex=%d\n", __func__, unit, lport, *duplex));
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_pause_set
 * Purpose:
 *      Configure CLMAC to transmit/receive pause frames.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_tx - Boolean: transmit pause or -1 (don't change)
 *      pause_rx - Boolean: receive pause or -1 (don't change)
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_cl_pause_set(int unit, uint8 lport, int pause_tx, int pause_rx)
{
    CLMAC_PAUSE_CTRLr_t     clmac_pause_ctrl;

    SOC_IF_ERROR_RETURN(READ_CLMAC_PAUSE_CTRLr(unit, lport, clmac_pause_ctrl));
    if (pause_tx) {
        CLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(clmac_pause_ctrl, 1);
    } else {
		CLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_SET(clmac_pause_ctrl, 0);
    }

    if (pause_rx) {
        CLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(clmac_pause_ctrl, 1);
    } else {
        CLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_SET(clmac_pause_ctrl, 0);
    }
    SOC_IF_ERROR_RETURN(WRITE_CLMAC_PAUSE_CTRLr(unit, lport, clmac_pause_ctrl));
    
#if UM_READBACK_DEBUG
{
    int p_tx=0, p_rx=0;
    
    /* Read back after setting clmac_pause_ctrl for checking */
    SOC_IF_ERROR_RETURN(READ_CLMAC_PAUSE_CTRLr(unit, lport, clmac_pause_ctrl));
    
    p_rx = CLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_GET(clmac_pause_ctrl);
    p_tx = CLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_GET(clmac_pause_ctrl);

    sal_printf("read back after setting clmac_pause_ctrl: lport %d RX=%s TX=%s\n", lport,
                 p_rx ? "on" : "off",
                 p_tx ? "on" : "off");
}
#endif
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_pause_get
 * Purpose:
 *      Return the pause ability of CLMAC
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
mac_cl_pause_get(int unit, uint8 lport, int *pause_tx, int *pause_rx)
{
    CLMAC_PAUSE_CTRLr_t     clmac_pause_ctrl;
	
    SOC_IF_ERROR_RETURN(READ_CLMAC_PAUSE_CTRLr(unit, lport, clmac_pause_ctrl));
    
    *pause_rx = CLMAC_PAUSE_CTRLr_RX_PAUSE_ENf_GET(clmac_pause_ctrl);
    *pause_tx =  CLMAC_PAUSE_CTRLr_TX_PAUSE_ENf_GET(clmac_pause_ctrl);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("clmac_pause_ctrl: lport %d RX=%s TX=%s\n", lport,
                 *pause_rx ? "on" : "off",
                 *pause_tx ? "on" : "off"));
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_speed_set
 * Purpose:
 *      Set CLMAC in the specified speed.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      speed - 100000, 120000.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_cl_speed_set(int unit, uint8 lport, int speed)
{
    int enable;
    uint32 fault;
    CLPORT_FAULT_LINK_STATUSr_t     clport_fault_link_status;
    CLMAC_RX_CTRLr_t    clmac_rx_ctrl;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d speed=%dMb\n", __func__, unit, lport, speed));

    SOC_IF_ERROR_RETURN(mac_cl_enable_get(unit, lport, &enable));
        
    if (enable) {
        /* Turn off TX/RX enable */
        SOC_IF_ERROR_RETURN(mac_cl_enable_set(unit, lport, 0));
    }
    
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));
    CLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(clmac_rx_ctrl, SOC_PORT_SPEED_MAX(lport) >= 10000 &&
                          IS_CL_PORT(lport) &&
                          !IS_HG_PORT(lport) ? 1 : 0);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));
        
    /*
     * Set REMOTE_FAULT/LOCAL_FAULT for CL ports,
     * else HW Linkscan interrupt would be suppressed.
     */
    {
        fault = speed <= 1000 ? 0 : 1;

        SOC_IF_ERROR_RETURN(
            READ_CLPORT_FAULT_LINK_STATUSr(unit,
                            lport, clport_fault_link_status));
        CLPORT_FAULT_LINK_STATUSr_REMOTE_FAULTf_SET(clport_fault_link_status, fault);
        CLPORT_FAULT_LINK_STATUSr_LOCAL_FAULTf_SET(clport_fault_link_status, fault);
        SOC_IF_ERROR_RETURN(
            WRITE_CLPORT_FAULT_LINK_STATUSr(unit,
                            lport, clport_fault_link_status));
    }

    /* Update port speed related setting in components other than MAC/SerDes*/
    SOC_IF_ERROR_RETURN(soc_port_speed_update(unit, lport, speed));

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
        SOC_IF_ERROR_RETURN(mac_cl_enable_set(unit, lport, 1));
    }
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_speed_get
 * Purpose:
 *      Get CLMAC speed
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      speed - (OUT) speed in Mb
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_cl_speed_get(int unit, uint8 lport, int *speed)
{
    CLMAC_MODEr_t   clmac_mode;
    
    SOC_IF_ERROR_RETURN(READ_CLMAC_MODEr(unit, lport, clmac_mode));
    switch (CLMAC_MODEr_SPEED_MODEf_GET(clmac_mode)) {
    case SOC_CLMAC_SPEED_1000:
        *speed = 1000;
        break;
    case SOC_CLMAC_SPEED_100000:
    default:
        /* Obtain fine grained port speed, since
        * SOC_CLMAC_SPEED_1000000 implies >= 10Gbps
        */         
        // Currently no granular_speed can be abtained in soc_granular_speed_get() for GH2
        *speed =  SOC_PORT_SPEED_INIT(lport);
        
        break;
    }
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:mac_cl_speed_get: unit %d lport %d speed=%dMb\n", __func__, unit, lport, *speed));
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_loopback_set
 * Purpose:
 *      Set a CLMAC into/out-of loopback mode
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
mac_cl_loopback_set(int unit, uint8 lport, int lb)
{
    CLMAC_CTRLr_t   clmac_ctrl;
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d loopback=%d\n", __func__, unit, lport, lb));
        
    /* need to enable clock compensation for applicable serdes device */
#ifdef UM_PHY_NOTIFY
    (void)soc_phyctrl_notify(unit, lport, phyEventMacLoopback, lb? 1: 0);
#endif
        
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_CTRLr(unit, lport, clmac_ctrl) );
    CLMAC_CTRLr_LOCAL_LPBKf_SET(clmac_ctrl, (lb ? 1 : 0));
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl) );
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_loopback_get
 * Purpose:
 *      Get current CLMAC loopback mode setting.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      loopback - (OUT) Boolean: true = loopback, false = normal
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_cl_loopback_get(int unit, uint8 lport, int *lb)
{
    CLMAC_CTRLr_t   clmac_ctrl;

    SOC_IF_ERROR_RETURN(
        READ_CLMAC_CTRLr(unit, lport, clmac_ctrl) );

    *lb = CLMAC_CTRLr_LOCAL_LPBKf_GET(clmac_ctrl);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d loopback=%d\n", __func__, unit, lport, *lb));
                                
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_pause_addr_set
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
mac_cl_pause_addr_set(int unit, uint8 lport, sal_mac_addr_t mac)
{
    uint32 values[2];
    CLMAC_TX_MAC_SAr_t      clmac_tx_mac_sa;
    CLMAC_RX_MAC_SAr_t      clmac_rx_mac_sa;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d MAC=%02x:%02x:%02x:%02x:%02x:%02x\n", __func__, unit, lport, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));
    
    values[0] = (mac[0] << 8) | mac[1];
    values[1] = (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];

    CLMAC_TX_MAC_SAr_SA_LOf_SET(clmac_tx_mac_sa, values[1]);
    CLMAC_TX_MAC_SAr_SA_HIf_SET(clmac_tx_mac_sa, values[0]);
    
    CLMAC_RX_MAC_SAr_SA_LOf_SET(clmac_rx_mac_sa, values[1]);
    CLMAC_RX_MAC_SAr_SA_HIf_SET(clmac_rx_mac_sa, values[0]);
    
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_TX_MAC_SAr(unit, lport, clmac_tx_mac_sa));
        
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_RX_MAC_SAr(unit, lport, clmac_rx_mac_sa));
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_pause_addr_get
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
mac_cl_pause_addr_get(int unit, uint8 lport, sal_mac_addr_t mac)
{
    uint32 values[2];
    CLMAC_RX_MAC_SAr_t      clmac_rx_mac_sa;


    SOC_IF_ERROR_RETURN(
        READ_CLMAC_RX_MAC_SAr(unit, lport, clmac_rx_mac_sa));
        
    values[1] = CLMAC_RX_MAC_SAr_SA_LOf_GET(clmac_rx_mac_sa);
    values[0] = CLMAC_RX_MAC_SAr_SA_HIf_GET(clmac_rx_mac_sa);
    
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
 *      mac_cl_interface_set
 * Purpose:
 *      Set a CLMAC interface type
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
mac_cl_interface_set(int unit, uint8 lport, soc_port_if_t pif)
{
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d pif=%d\n", __func__, unit, lport, pif));

    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_interface_get
 * Purpose:
 *      Retrieve CLMAC interface type
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pif - (OUT) one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_NONE
 */
static int
mac_cl_interface_get(int unit, uint8 lport, soc_port_if_t *pif)
{
    *pif = SOC_PORT_IF_CGMII;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d *pif=%d\n", __func__, unit, lport, *pif));

    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_frame_max_set
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
mac_cl_frame_max_set(int unit, uint8 lport, int size)
{
    CLMAC_RX_MAX_SIZEr_t    clmac_rx_max_size;
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d size=%d\n", __func__, unit, lport, size));
    
    if (IS_CE_PORT(lport) || IS_XE_PORT(lport) || IS_GE_PORT(lport)) {
        /* For VLAN tagged packets */
        size += 4;
    }
    
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_RX_MAX_SIZEr(unit, lport, clmac_rx_max_size));
    CLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_SET(clmac_rx_max_size, size);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_RX_MAX_SIZEr(unit, lport, clmac_rx_max_size));
                                  
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_frame_max_get
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
mac_cl_frame_max_get(int unit, uint8 lport, int *size)
{
    CLMAC_RX_MAX_SIZEr_t    clmac_rx_max_size;
    
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_RX_MAX_SIZEr(unit, lport, clmac_rx_max_size));
        
    *size = CLMAC_RX_MAX_SIZEr_RX_MAX_SIZEf_GET(clmac_rx_max_size);
    if (IS_CE_PORT(lport) || IS_XE_PORT(lport) || IS_GE_PORT(lport)) {
        /* For VLAN tagged packets */
        *size -= 4;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d *size=%d\n", __func__, unit, lport, *size));
                            
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_ifg_set
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
mac_cl_ifg_set(int unit, uint8 lport, int speed,
                soc_port_duplex_t duplex, int ifg)
{
    int         cur_speed;
    int         cur_duplex;
    int         real_ifg;
    soc_port_ability_t ability;
    uint32      pa_flag;
    CLMAC_TX_CTRLr_t    clmac_tx_ctrl, clmac_tx_ctrl_orig;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d speed=%dMb duplex=%s ifg=%d\n", __func__, unit, lport, speed, duplex ? "True" : "False", ifg));
    
    pa_flag = SOC_PA_SPEED(speed); 
    sal_memset(&ability, 0, sizeof(ability));
    soc_mac_cl.md_ability_local_get(unit, lport, &ability);
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
        mac_cl_duplex_get(unit, lport, &cur_duplex));
    SOC_IF_ERROR_RETURN(
        mac_cl_speed_get(unit, lport, &cur_speed));

    /* CLMAC_MODE supports only 4 speeds with 4 being max as LINK_10G_PLUS */
    /* Unlike the corresponding XLMAC function call, mac_cl_speed_get()
     * returns a fine grained speed value when CLMAC_MODE.SPEEDf=LINK_10G_PLUS
     * Hence the check below uses cur_speed >= 10000, unlike the
     * cur_speed == 10000 check used in xlmac */
    if ((speed >= 10000) && (cur_speed >= 10000)) {
        cur_speed = speed;
    }

    if (cur_speed == speed &&
        cur_duplex == (duplex == SOC_PORT_DUPLEX_FULL ? TRUE : FALSE)) {
        SOC_IF_ERROR_RETURN(
            READ_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
        SOC_IF_ERROR_RETURN(
            READ_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl_orig));
            
        CLMAC_TX_CTRLr_AVERAGE_IPGf_SET(clmac_tx_ctrl, (real_ifg / 8) );
        
        if ( !PBMP_EQ(*((pbmp_t *) &clmac_tx_ctrl), *((pbmp_t *) &clmac_tx_ctrl_orig)) ) {        
            SOC_IF_ERROR_RETURN(
                WRITE_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
        }
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_ifg_get
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
mac_cl_ifg_get(int unit, uint8 lport, int speed,
                soc_port_duplex_t duplex, int *ifg)
{
    soc_port_ability_t ability;
    uint32      pa_flag;
    CLMAC_TX_CTRLr_t    clmac_tx_ctrl;

    if (!duplex) {
        return SYS_ERR;
    }

    pa_flag = SOC_PA_SPEED(speed); 
    sal_memset(&ability, 0, sizeof(ability));
    soc_mac_cl.md_ability_local_get(unit, lport, &ability);
    if (!(pa_flag & ability.speed_full_duplex)) {
        return SYS_ERR;
    }

    SOC_IF_ERROR_RETURN(
            READ_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
            
    *ifg = CLMAC_TX_CTRLr_AVERAGE_IPGf_GET(clmac_tx_ctrl);
    *ifg = (*ifg) * (8);
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d speed=%dMb duplex=%s *ifg=%d\n", __func__, unit, lport, speed, duplex ? "True" : "False", *ifg));
        
    return SYS_OK;
}

/*
 * Function:
 *      _mac_cl_port_mode_update
 * Purpose:
 *      Set the CLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      to_hg_port - (TRUE/FALSE)
 * Returns:
 *      SOC_E_XXX
 */
static int
_mac_cl_port_mode_update(int unit, uint8 lport, int to_hg_port)
{
    /*  Should be called since 
    *   if (soc_feature(unit, soc_feature_hg2_light_in_portmacro)) is always true for GH2 and HR3
    *   Currently not implemanted since mac_cl_encap_set should not be called at runtime.
    */
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d to_hg_port=%d\n", __func__, unit, lport, to_hg_port));
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_encap_set
 * Purpose:
 *      Set the CLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (IN) encap bits (defined above)
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_cl_encap_set(int unit, uint8 lport, int mode)
{
    int enable, encap;
    CLMAC_MODEr_t   clmac_mode;
    CLMAC_RX_CTRLr_t    clmac_rx_ctrl;
    int runt;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d encapsulation=%s\n", __func__, unit, lport, mac_cl_encap_mode[mode]));

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

    SOC_IF_ERROR_RETURN(mac_cl_enable_get(unit, lport, &enable));
    if (enable) {
        /* Turn off TX/RX enable */
        SOC_IF_ERROR_RETURN(mac_cl_enable_set(unit, lport, 0));
    }
    
    /* if (soc_feature(unit, soc_feature_hg2_light_in_portmacro)) */
    /* mode update for all encap mode change! */
    SOC_IF_ERROR_RETURN(_mac_cl_port_mode_update(unit, lport, mode));
    
    /* Update the encapsulation mode */
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_MODEr(unit, lport, clmac_mode) );
    CLMAC_MODEr_HDR_MODEf_SET(clmac_mode, encap);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_MODEr(unit, lport, clmac_mode) );
        
        
    SOC_IF_ERROR_RETURN(
        READ_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));
    runt = (encap == SOC_ENCAP_HIGIG2) ? CLMAC_RUNT_THRESHOLD_HG2 :
           ((encap == SOC_ENCAP_HIGIG) ? CLMAC_RUNT_THRESHOLD_HG1 :
                                         CLMAC_RUNT_THRESHOLD_IEEE);
    CLMAC_RX_CTRLr_RUNT_THRESHOLDf_SET(clmac_rx_ctrl, runt);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));
        
    CLMAC_RX_CTRLr_STRICT_PREAMBLEf_SET(clmac_rx_ctrl, mode == SOC_ENCAP_IEEE ? 1 : 0);
    SOC_IF_ERROR_RETURN(
        WRITE_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));
        
    if (enable) {
        /* Re-enable transmitter and receiver */
        SOC_IF_ERROR_RETURN(
            mac_cl_enable_set(unit, lport, 1));
    }

    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_encap_get
 * Purpose:
 *      Get the CLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (INT) encap bits (defined above)
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_cl_encap_get(int unit, uint8 lport, int *mode)
{
    CLMAC_MODEr_t   clmac_mode;

    if (!mode) {
        return SYS_ERR;
    }

    SOC_IF_ERROR_RETURN(READ_CLMAC_MODEr(unit, lport, clmac_mode));
    switch (CLMAC_MODEr_HDR_MODEf_GET(clmac_mode)) {
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


    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d encapsulation=%s\n", __func__, unit, lport, mac_cl_encap_mode[*mode] ));
    
    return SYS_OK;
}

/*
 * Function:
 *      mac_cl_expected_rx_latency_get
 * Purpose:
 *      Get the CLMAC port expected Rx latency
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      latency - (OUT) Latency in NS
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_cl_expected_rx_latency_get(int unit, uint8 lport, int *latency)
{
    int speed = 0;
    SOC_IF_ERROR_RETURN(mac_cl_speed_get(unit, lport, &speed));

    switch (speed) {
    case 1000:  /* GigE */
        *latency = 0;
        break;

    case 10000:  /* 10G */
        *latency = 0;
        break;

    default:
        *latency = 0;
        break;
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_cl_control_set
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
mac_cl_control_set(int unit, uint8 lport, soc_mac_control_t type,
                  int value)
{
    int rv;
    uint32 fval0;
#if UM_READBACK_DEBUG
    uint32 fval1;
#endif
    CLMAC_CTRLr_t clmac_ctrl;
    
    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:unit %d lport %d type=%d value=%d\n", __func__, unit, lport,
                            type, value));

    rv = SOC_E_NONE;
    switch (type) {
    case SOC_MAC_CONTROL_RX_SET:
        SOC_IF_ERROR_RETURN(READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        CLMAC_CTRLr_RX_ENf_SET(clmac_ctrl, (value ? 1 : 0));
        SOC_IF_ERROR_RETURN(WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        
#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        value = CLMAC_CTRLr_RX_ENf_GET(clmac_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_RX_SET: lport %d value=%d\n", __func__, lport, value));
#endif
        break;
        
    case SOC_MAC_CONTROL_TX_SET:
        SOC_IF_ERROR_RETURN(READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        CLMAC_CTRLr_TX_ENf_SET(clmac_ctrl, (value ? 1 : 0));
        SOC_IF_ERROR_RETURN(WRITE_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        
#if UM_READBACK_DEBUG
        /* read back for debug */
        SOC_IF_ERROR_RETURN(READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        value = CLMAC_CTRLr_TX_ENf_GET(clmac_ctrl);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_TX_SET: lport %d value=%d\n", __func__, lport, value));
#endif
        break;
        
    case SOC_MAC_CONTROL_FRAME_SPACING_STRETCH:
        {
            CLMAC_TX_CTRLr_t    clmac_tx_ctrl;
            
            if (value < 0 || value > 255) {
                return SYS_ERR;
            } else {
                SOC_IF_ERROR_RETURN(READ_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
                
                if (value >= 8) {
                    CLMAC_TX_CTRLr_THROT_DENOMf_SET(clmac_tx_ctrl, value);
                    CLMAC_TX_CTRLr_THROT_NUMf_SET(clmac_tx_ctrl, 1);
                } else {
                    CLMAC_TX_CTRLr_THROT_DENOMf_SET(clmac_tx_ctrl, 0);
                    CLMAC_TX_CTRLr_THROT_NUMf_SET(clmac_tx_ctrl, 0);
                }
                SOC_IF_ERROR_RETURN(WRITE_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
                
#if UM_READBACK_DEBUG
                /* read back for debug */
                SOC_IF_ERROR_RETURN(READ_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
                value = CLMAC_TX_CTRLr_THROT_DENOMf_GET(clmac_tx_ctrl);
                LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_FRAME_SPACING_STRETCH: THROT_DENOM lport %d value=%d\n", __func__, lport, value));
                value = CLMAC_TX_CTRLr_THROT_NUMf_GET(clmac_tx_ctrl);
                LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_FRAME_SPACING_STRETCH: THROT_NUM lport %d value=%d\n", __func__, lport, value));
#endif
            }
            break;
        }
    
    case SOC_MAC_PASS_CONTROL_FRAME:
        /* This is always True */
        break;

    case SOC_MAC_CONTROL_PFC_TYPE:
        {
            CLMAC_PFC_TYPEr_t   clmac_pfc_type;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_TYPEr(unit, lport, clmac_pfc_type));
            CLMAC_PFC_TYPEr_PFC_ETH_TYPEf_SET(clmac_pfc_type, value);
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_PFC_TYPEr(unit, lport, clmac_pfc_type));
            
#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_TYPEr(unit, lport, clmac_pfc_type));
            value = CLMAC_PFC_TYPEr_PFC_ETH_TYPEf_GET(clmac_pfc_type);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_TYPE: lport %d value=%d\n", __func__, lport, value));
#endif
        }
        break;
    
    case SOC_MAC_CONTROL_PFC_OPCODE:
        {
            CLMAC_PFC_OPCODEr_t     clmac_pfc_opcode;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_OPCODEr(unit, lport, clmac_pfc_opcode));
            CLMAC_PFC_OPCODEr_PFC_OPCODEf_SET(clmac_pfc_opcode, value);
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_PFC_OPCODEr(unit, lport, clmac_pfc_opcode));
            
#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_OPCODEr(unit, lport, clmac_pfc_opcode));
            value = CLMAC_PFC_OPCODEr_PFC_OPCODEf_GET(clmac_pfc_opcode);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_OPCODE: lport %d value=%d\n", __func__, lport, value));
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
            CLMAC_PFC_DAr_t     clmac_pfc_da;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_DAr(unit, lport, clmac_pfc_da));
            fval0 = CLMAC_PFC_DAr_PFC_MACDA_LOf_GET(clmac_pfc_da);
            fval0 &= 0x00ffffff;
            fval0 |= (value & 0xff) << 24;
            CLMAC_PFC_DAr_PFC_MACDA_LOf_SET(clmac_pfc_da, fval0);
            CLMAC_PFC_DAr_PFC_MACDA_HIf_SET(clmac_pfc_da, (value >> 8));
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_PFC_DAr(unit, lport, clmac_pfc_da));
            
#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_DAr(unit, lport, clmac_pfc_da));
            fval0 = CLMAC_PFC_DAr_PFC_MACDA_LOf_GET(clmac_pfc_da);
            fval1 = CLMAC_PFC_DAr_PFC_MACDA_HIf_GET(clmac_pfc_da);
            value = (fval0 >> 24) | (fval1 << 8);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_MAC_DA_OUI: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI:
        {
            CLMAC_PFC_DAr_t     clmac_pfc_da;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_DAr(unit, lport, clmac_pfc_da));
            fval0 = CLMAC_PFC_DAr_PFC_MACDA_LOf_GET(clmac_pfc_da) & 0xff000000;
            fval0 |= value;
            CLMAC_PFC_DAr_PFC_MACDA_LOf_SET(clmac_pfc_da, fval0);
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_PFC_DAr(unit, lport, clmac_pfc_da));
            
#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_DAr(unit, lport, clmac_pfc_da));
            value = CLMAC_PFC_DAr_PFC_MACDA_LOf_GET(clmac_pfc_da) & 0x00ffffff;
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI: lport %d value=0x%08x\n", __func__, lport, value));
#endif      
        }
        break;

    case SOC_MAC_CONTROL_PFC_RX_PASS:
        {
            CLMAC_RX_CTRLr_t    clmac_rx_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));
            CLMAC_RX_CTRLr_RX_PASS_PFCf_SET(clmac_rx_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));
            value = CLMAC_RX_CTRLr_RX_PASS_PFCf_GET(clmac_rx_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_RX_PASS: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;
        
    case SOC_MAC_CONTROL_PFC_RX_ENABLE:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            CLMAC_PFC_CTRLr_RX_PFC_ENf_SET(clmac_pfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */          
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            value = CLMAC_PFC_CTRLr_RX_PFC_ENf_GET(clmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_RX_ENABLE: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_TX_ENABLE:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            CLMAC_PFC_CTRLr_TX_PFC_ENf_SET(clmac_pfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            value = CLMAC_PFC_CTRLr_TX_PFC_ENf_GET(clmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_TX_ENABLE: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_FORCE_XON:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            CLMAC_PFC_CTRLr_FORCE_PFC_XONf_SET(clmac_pfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            value = CLMAC_PFC_CTRLr_FORCE_PFC_XONf_GET(clmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_FORCE_XON: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_STATS_ENABLE:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            CLMAC_PFC_CTRLr_PFC_STATS_ENf_SET(clmac_pfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            value = CLMAC_PFC_CTRLr_PFC_STATS_ENf_GET(clmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_STATS_ENABLE: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_REFRESH_TIME:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            CLMAC_PFC_CTRLr_PFC_REFRESH_TIMERf_SET(clmac_pfc_ctrl, value);
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            value = CLMAC_PFC_CTRLr_PFC_REFRESH_TIMERf_GET(clmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_REFRESH_TIME: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_PFC_XOFF_TIME:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            CLMAC_PFC_CTRLr_PFC_XOFF_TIMERf_SET(clmac_pfc_ctrl, value);
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            value = CLMAC_PFC_CTRLr_PFC_XOFF_TIMERf_GET(clmac_pfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_XOFF_TIME: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_LLFC_RX_ENABLE:
        {
            CLMAC_LLFC_CTRLr_t   clmac_llfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_LLFC_CTRLr(unit, lport, clmac_llfc_ctrl));
            CLMAC_LLFC_CTRLr_RX_LLFC_ENf_SET(clmac_llfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_LLFC_CTRLr(unit, lport, clmac_llfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_LLFC_CTRLr(unit, lport, clmac_llfc_ctrl));
            value = CLMAC_LLFC_CTRLr_RX_LLFC_ENf_GET(clmac_llfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_PFC_XOFF_TIME: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_LLFC_TX_ENABLE:
        {
            CLMAC_LLFC_CTRLr_t   clmac_llfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_LLFC_CTRLr(unit, lport, clmac_llfc_ctrl));
            CLMAC_LLFC_CTRLr_TX_LLFC_ENf_SET(clmac_llfc_ctrl, (value ? 1 : 0));
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_LLFC_CTRLr(unit, lport, clmac_llfc_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_LLFC_CTRLr(unit, lport, clmac_llfc_ctrl));            
            value = CLMAC_LLFC_CTRLr_TX_LLFC_ENf_GET(clmac_llfc_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_LLFC_TX_ENABLE: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_EEE_ENABLE:
        {
            CLMAC_EEE_CTRLr_t   clmac_eee_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_EEE_CTRLr(unit, lport, clmac_eee_ctrl));
            CLMAC_EEE_CTRLr_EEE_ENf_SET(clmac_eee_ctrl, value);
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_EEE_CTRLr(unit, lport, clmac_eee_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_EEE_CTRLr(unit, lport, clmac_eee_ctrl));
            value = CLMAC_EEE_CTRLr_EEE_ENf_GET(clmac_eee_ctrl);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_LLFC_TX_ENABLE: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_EEE_TX_IDLE_TIME:
        {
            CLMAC_EEE_TIMERSr_t   clmac_eee_timers;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_EEE_TIMERSr(unit, lport, clmac_eee_timers));
            CLMAC_EEE_TIMERSr_EEE_DELAY_ENTRY_TIMERf_SET(clmac_eee_timers, value);
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_EEE_TIMERSr(unit, lport, clmac_eee_timers));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_EEE_TIMERSr(unit, lport, clmac_eee_timers));
            value = CLMAC_EEE_TIMERSr_EEE_DELAY_ENTRY_TIMERf_GET(clmac_eee_timers);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_EEE_TX_IDLE_TIME: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_EEE_TX_WAKE_TIME:
        {
            CLMAC_EEE_TIMERSr_t   clmac_eee_timers;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_EEE_TIMERSr(unit, lport, clmac_eee_timers));
            CLMAC_EEE_TIMERSr_EEE_WAKE_TIMERf_SET(clmac_eee_timers, value);
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_EEE_TIMERSr(unit, lport, clmac_eee_timers));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_EEE_TIMERSr(unit, lport, clmac_eee_timers));
            value = CLMAC_EEE_TIMERSr_EEE_WAKE_TIMERf_GET(clmac_eee_timers);
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_EEE_TX_WAKE_TIME: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE:
        {
            CLMAC_RX_LSS_CTRLr_t   clmac_rx_lss_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_LSS_CTRLr(unit, lport, clmac_rx_lss_ctrl));
            CLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_SET(clmac_rx_lss_ctrl, (value ? 0 : 1));
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_RX_LSS_CTRLr(unit, lport, clmac_rx_lss_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_LSS_CTRLr(unit, lport, clmac_rx_lss_ctrl));
            /* If set, MAC will continue to transmit data irrespective of LOCAL_FAULT_STATUS. */
            value = CLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_GET(clmac_rx_lss_ctrl) ? 0 : 1;
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE:
        {
            CLMAC_RX_LSS_CTRLr_t   clmac_rx_lss_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_LSS_CTRLr(unit, lport, clmac_rx_lss_ctrl));
            CLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_SET(clmac_rx_lss_ctrl, (value ? 0 : 1));
            SOC_IF_ERROR_RETURN(WRITE_CLMAC_RX_LSS_CTRLr(unit, lport, clmac_rx_lss_ctrl));

#if UM_READBACK_DEBUG
            /* read back for debug */                    
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_LSS_CTRLr(unit, lport, clmac_rx_lss_ctrl));
            /* If set, MAC will continue to transmit data irrespective of REMOTE_FAULT_STATUS. */
            value = CLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_GET(clmac_rx_lss_ctrl) ? 0 : 1;
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE: lport %d value=0x%08x\n", __func__, lport, value));
#endif
        }
        break;
    
    case SOC_MAC_CONTROL_EGRESS_DRAIN:
        SOC_IF_ERROR_RETURN(mac_cl_egress_queue_drain(unit, lport));
        break;
        
    case SOC_MAC_CONTROL_FAILOVER_RX_SET:
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:do nothing...: SOC_MAC_CONTROL_FAILOVER_RX_SET lport %d type=%d\n", __func__, lport, type));
        break;

    default:
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:no such type: type=%d\n", __func__, type));
        return SOC_E_UNAVAIL;
    
    }
    
    return rv;
}

/*
 * Function:
 *      mac_cl_control_get
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
mac_cl_control_get(int unit, uint8 lport, soc_mac_control_t type,
                  int *value)
{
    int rv;
    uint32 fval0, fval1;
    CLMAC_CTRLr_t clmac_ctrl;

    if (value == NULL) {
        return SOC_E_PARAM;
    }

    rv = SOC_E_NONE;
    switch (type) {
    case SOC_MAC_CONTROL_RX_SET:
        SOC_IF_ERROR_RETURN(READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        *value = CLMAC_CTRLr_RX_ENf_GET(clmac_ctrl);
        
        break;
    case SOC_MAC_CONTROL_TX_SET:
        SOC_IF_ERROR_RETURN(READ_CLMAC_CTRLr(unit, lport, clmac_ctrl));
        *value = CLMAC_CTRLr_TX_ENf_GET(clmac_ctrl);
        
        break;
    case SOC_MAC_CONTROL_FRAME_SPACING_STRETCH:
        {
            CLMAC_TX_CTRLr_t    clmac_tx_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_TX_CTRLr(unit, lport, clmac_tx_ctrl));
            *value = CLMAC_TX_CTRLr_THROT_DENOMf_GET(clmac_tx_ctrl);
            
            break;
        }
    case SOC_MAC_CONTROL_TIMESTAMP_TRANSMIT:
        {
            CLMAC_TX_TIMESTAMP_FIFO_STATUSr_t   clmac_tx_timestamp_fifo_status;
            CLMAC_TX_TIMESTAMP_FIFO_DATAr_t     clmac_tx_timestamp_fifo_data;
         
            SOC_IF_ERROR_RETURN
                (READ_CLMAC_TX_TIMESTAMP_FIFO_STATUSr(unit, lport, clmac_tx_timestamp_fifo_status));
            if( CLMAC_TX_TIMESTAMP_FIFO_STATUSr_ENTRY_COUNTf_GET(clmac_tx_timestamp_fifo_status)  == 0){
                return SYS_ERR;
            }
            
            SOC_IF_ERROR_RETURN
                (READ_CLMAC_TX_TIMESTAMP_FIFO_DATAr(unit, lport, clmac_tx_timestamp_fifo_data));
            *value = CLMAC_TX_TIMESTAMP_FIFO_DATAr_TIME_STAMPf_GET(clmac_tx_timestamp_fifo_data);    
        }
        break;

    case SOC_MAC_PASS_CONTROL_FRAME:
        *value = TRUE;
        break;

    case SOC_MAC_CONTROL_PFC_TYPE:
        {
            CLMAC_PFC_TYPEr_t   clmac_pfc_type;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_TYPEr(unit, lport, clmac_pfc_type));
            *value = CLMAC_PFC_TYPEr_PFC_ETH_TYPEf_GET(clmac_pfc_type);
        }
        break;
    
    case SOC_MAC_CONTROL_PFC_OPCODE:
        {
            CLMAC_PFC_OPCODEr_t     clmac_pfc_opcode;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_OPCODEr(unit, lport, clmac_pfc_opcode));
            *value = CLMAC_PFC_OPCODEr_PFC_OPCODEf_GET(clmac_pfc_opcode);
        }
        break;
        
    case SOC_MAC_CONTROL_PFC_CLASSES:
        *value = 8;
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_OUI:
        {
            CLMAC_PFC_DAr_t     clmac_pfc_da;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_DAr(unit, lport, clmac_pfc_da));
            fval0 = CLMAC_PFC_DAr_PFC_MACDA_LOf_GET(clmac_pfc_da);
            fval1 = CLMAC_PFC_DAr_PFC_MACDA_HIf_GET(clmac_pfc_da);
            *value = (fval0 >> 24) | (fval1 << 8);
        }
        break;


    case SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI:
        {
            CLMAC_PFC_DAr_t     clmac_pfc_da;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_DAr(unit, lport, clmac_pfc_da));
            *value = CLMAC_PFC_DAr_PFC_MACDA_LOf_GET(clmac_pfc_da) & 0x00ffffff;
        }
        break;

    case SOC_MAC_CONTROL_PFC_RX_PASS:
        {
            CLMAC_RX_CTRLr_t    clmac_rx_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_CTRLr(unit, lport, clmac_rx_ctrl));
            *value = CLMAC_RX_CTRLr_RX_PASS_PFCf_GET(clmac_rx_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_RX_ENABLE:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            *value = CLMAC_PFC_CTRLr_RX_PFC_ENf_GET(clmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_TX_ENABLE:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            *value = CLMAC_PFC_CTRLr_TX_PFC_ENf_GET(clmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_FORCE_XON:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            *value = CLMAC_PFC_CTRLr_FORCE_PFC_XONf_GET(clmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_STATS_ENABLE:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            *value = CLMAC_PFC_CTRLr_PFC_STATS_ENf_GET(clmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_REFRESH_TIME:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            *value = CLMAC_PFC_CTRLr_PFC_REFRESH_TIMERf_GET(clmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_PFC_XOFF_TIME:
        {
            CLMAC_PFC_CTRLr_t   clmac_pfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_PFC_CTRLr(unit, lport, clmac_pfc_ctrl));
            *value = CLMAC_PFC_CTRLr_PFC_XOFF_TIMERf_GET(clmac_pfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_LLFC_RX_ENABLE:
        {
            CLMAC_LLFC_CTRLr_t   clmac_llfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_LLFC_CTRLr(unit, lport, clmac_llfc_ctrl));
            *value = CLMAC_LLFC_CTRLr_RX_LLFC_ENf_GET(clmac_llfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_LLFC_TX_ENABLE:
        {
            CLMAC_LLFC_CTRLr_t   clmac_llfc_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_LLFC_CTRLr(unit, lport, clmac_llfc_ctrl));
            *value = CLMAC_LLFC_CTRLr_TX_LLFC_ENf_GET(clmac_llfc_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_EEE_ENABLE:
        {
            CLMAC_EEE_CTRLr_t   clmac_eee_ctrl;
            
            SOC_IF_ERROR_RETURN(READ_CLMAC_EEE_CTRLr(unit, lport, clmac_eee_ctrl));
            *value = CLMAC_EEE_CTRLr_EEE_ENf_GET(clmac_eee_ctrl);
        }
        break;

    case SOC_MAC_CONTROL_EEE_TX_IDLE_TIME:
        {
            CLMAC_EEE_TIMERSr_t   clmac_eee_timers;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_EEE_TIMERSr(unit, lport, clmac_eee_timers));
            *value = CLMAC_EEE_TIMERSr_EEE_DELAY_ENTRY_TIMERf_GET(clmac_eee_timers);
        }
        break;

    case SOC_MAC_CONTROL_EEE_TX_WAKE_TIME:
        {
            CLMAC_EEE_TIMERSr_t   clmac_eee_timers;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_EEE_TIMERSr(unit, lport, clmac_eee_timers));
            *value = CLMAC_EEE_TIMERSr_EEE_WAKE_TIMERf_GET(clmac_eee_timers);
        }
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE:
        {
            CLMAC_RX_LSS_CTRLr_t   clmac_rx_lss_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_LSS_CTRLr(unit, lport, clmac_rx_lss_ctrl));
            /* If set, MAC will continue to transmit data irrespective of LOCAL_FAULT_STATUS. */
            *value = CLMAC_RX_LSS_CTRLr_LOCAL_FAULT_DISABLEf_GET(clmac_rx_lss_ctrl) ? 0 : 1;
        }
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_STATUS:
        {
            CLMAC_RX_LSS_STATUSr_t   clmac_rx_lss_status;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_LSS_STATUSr(unit, lport, clmac_rx_lss_status));
            *value = CLMAC_RX_LSS_STATUSr_LOCAL_FAULT_STATUSf_GET(clmac_rx_lss_status);
        }
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE:
        {
            CLMAC_RX_LSS_CTRLr_t   clmac_rx_lss_ctrl;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_LSS_CTRLr(unit, lport, clmac_rx_lss_ctrl));
            /* If set, MAC will continue to transmit data irrespective of REMOTE_FAULT_STATUS. */
            *value = CLMAC_RX_LSS_CTRLr_REMOTE_FAULT_DISABLEf_GET(clmac_rx_lss_ctrl) ? 0 : 1;
        }
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_STATUS:
        {
            CLMAC_RX_LSS_STATUSr_t   clmac_rx_lss_status;
        
            SOC_IF_ERROR_RETURN(READ_CLMAC_RX_LSS_STATUSr(unit, lport, clmac_rx_lss_status));
            *value = CLMAC_RX_LSS_STATUSr_REMOTE_FAULT_STATUSf_GET(clmac_rx_lss_status);
        }
        break;
    case SOC_MAC_CONTROL_EXPECTED_RX_LATENCY:
        {
            SOC_IF_ERROR_RETURN(mac_cl_expected_rx_latency_get(unit, lport, value));
        }
        break;
    default:
        return SOC_E_UNAVAIL;
    
    }
    
    return rv;
}

/*
 * Function:
 *      mac_cl_ability_local_get
 * Purpose:
 *      Return the abilities of CLMAC
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (OUT) Supported operating modes as a mask of abilities.
 * Returns:
 *      SOC_E_XXX
 */
static int
mac_cl_ability_local_get(int unit, uint8 lport,
                          soc_port_ability_t *ability)
{
    soc_info_t *si = &SOC_INFO(unit);
    int port_speed_max, use_hg_port_speeds = FALSE;
    
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

    /*
     * portmap_* config variables allow users to specifiy a HG speed as the max
     * speed for a port that initializes as an ethernet port. This allows the user
     * to optionally set the port speed to a HG speed after an encap change is
     * performed. Under such situations allow HG speeds to be part of the MAC
     * ability mask for the ethernet port.
     */
     /* no support for soc_feature_flexport_based_speed_set
    if (soc_feature(unit, soc_feature_flexport_based_speed_set)) {
        use_hg_port_speeds = TRUE;
    }
    */
    
    /* Adjust port_speed_max according to the port config */
    port_speed_max = SOC_PORT_SPEED_MAX(lport);
    
    /* Use current number of lanes per port to determine the supported speeds */
    if (use_hg_port_speeds || IS_HG_PORT(lport)) {
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:Should not use_hg_port_speeds || IS_HG_PORT\n", __func__));
        return SYS_ERR;
    } else {
        switch (port_speed_max) {
        case 106000:
            ability->speed_full_duplex |= SOC_PA_SPEED_106GB;
            /* fall through */
        case 100000:
            if (si->port_num_lanes[lport] == 4)  {
                ability->speed_full_duplex |= SOC_PA_SPEED_100GB;
            }
            /* fall through */
        case 53000:
        case 50000:
            if (si->port_num_lanes[lport] == 2)  {
                ability->speed_full_duplex |= SOC_PA_SPEED_50GB;
            }
            /* fall through */
        case 42000:
        case 40000:
            if (si->port_num_lanes[lport] != 1)  {
                ability->speed_full_duplex |= SOC_PA_SPEED_40GB;
            }
            /* fall through */
        case 25000:
            if (si->port_num_lanes[lport] == 1)  {
                ability->speed_full_duplex |= SOC_PA_SPEED_25GB;
            }
            /* fall through */
        case 21000:
        case 20000:
            if (si->port_num_lanes[lport] == 2)  {
                ability->speed_full_duplex |= SOC_PA_SPEED_20GB;
            }
            /* fall through */
        case 11000:
        case 10000:
            if (si->port_num_lanes[lport] == 1)  {
                ability->speed_full_duplex |= SOC_PA_SPEED_10GB;
            }
            /* fall through */
        case 1000:
            if (si->port_num_lanes[lport] == 1)  {
                ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
            }
        default:
            break;
        }
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:mac_cl_ability_local_get: unit %d lport=%d "
            "speed_half=0x%x speed_full=0x%x encap=0x%x pause=0x%x "
            "interface=0x%x medium=0x%x loopback=0x%x flags=0x%x\n", __func__,
            unit, lport,
            ability->speed_half_duplex, ability->speed_full_duplex,
            ability->encap, ability->pause, ability->interface,
            ability->medium, ability->loopback, ability->flags
            ));
    
    return SYS_OK;
}

/* Exported CLMAC driver structure */
mac_driver_t soc_mac_cl = {
    "CLMAC Driver",               /* drv_name */
    .md_init = mac_cl_init,                  /* md_init  */
    .md_enable_set = mac_cl_enable_set,            /* md_enable_set */
    .md_enable_get = mac_cl_enable_get,            /* md_enable_get */
    .md_duplex_set = mac_cl_duplex_set,            /* md_duplex_set */
    .md_duplex_get = mac_cl_duplex_get,            /* md_duplex_get */
    .md_speed_set = mac_cl_speed_set,             /* md_speed_set */
    .md_speed_get = mac_cl_speed_get,             /* md_speed_get */
    .md_pause_set = mac_cl_pause_set,             /* md_pause_set */
    .md_pause_get = mac_cl_pause_get,             /* md_pause_get */
    .md_pause_addr_set = mac_cl_pause_addr_set,        /* md_pause_addr_set */
    .md_pause_addr_get = mac_cl_pause_addr_get,        /* md_pause_addr_get */
    .md_lb_set = mac_cl_loopback_set,          /* md_lb_set */
    .md_lb_get = mac_cl_loopback_get,          /* md_lb_get */
    .md_interface_set = mac_cl_interface_set,         /* md_interface_set */
    .md_interface_get = mac_cl_interface_get,         /* md_interface_get */
    .md_ability_get = NULL,                         /* md_ability_get - Deprecated */
    .md_frame_max_set = mac_cl_frame_max_set,         /* md_frame_max_set */
    .md_frame_max_get = mac_cl_frame_max_get,         /* md_frame_max_get */
    .md_ifg_set = mac_cl_ifg_set,               /* md_ifg_set */
    .md_ifg_get = mac_cl_ifg_get,               /* md_ifg_get */
    .md_encap_set = mac_cl_encap_set,             /* md_encap_set */
    .md_encap_get = mac_cl_encap_get,             /* md_encap_get */
    .md_control_set = mac_cl_control_set,           /* md_control_set */
    .md_control_get = mac_cl_control_get,           /* md_control_get */
    .md_ability_local_get = mac_cl_ability_local_get      /* md_ability_local_get */
 };


