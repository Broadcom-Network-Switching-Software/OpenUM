/*
 * $Id: macutil.c,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "utils/system.h"
#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a) ((void)(_a))
#define COMPILER_ATTRIBUTE(_a) __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>

/*******************************************************************************
 * Private functions
 */

/*
 * Function:
 *      soc_fl_64q_port_check
 * Purpose:
 *      Check if the port(logical) is capable of configured up to 64 COSQ.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      port - logical port.
 *      is_64q (OUT) - with 64 COSQ or not
 * Returns:
 *      BCM_E_XXX
 */
static int
soc_fl_64q_port_check(int unit, uint8 lport, int *is_64q_port)
{
    int phy_port;
    int mmu_port;

    *is_64q_port = 0;
    phy_port = SOC_PORT_L2P_MAPPING(lport);
    mmu_port = SOC_PORT_P2M_MAPPING(phy_port);
    if ((mmu_port >= SOC_FL_64Q_MMU_PORT_IDX_MIN) &&
        (mmu_port <= SOC_FL_64Q_MMU_PORT_IDX_MAX)) {
        *is_64q_port = 1;
    } else {
        *is_64q_port = 0;
    }
    return SYS_OK;
}

/*
 * For calculate FL MMU_MAX/MIN_BUCKET_QLAYER table index.
 *    - mmu port 0~57 : 8 entries
 *    - mmu port 58~65 : 64 entries
 *  (IN) p : logical port
 *  (IN) q : cosq
 *  (OUT) idx : the entry index of MMU_MAX/MIN_BUCKET_QLAYER table
 */
static int
soc_fl_mmu_bucket_qlayer_index(int unit, int p, int q, int *idx)
{
    int is_64q;
    int phy_port;
    int mmu_port;

    is_64q = 0;
    phy_port = SOC_PORT_L2P_MAPPING(p);
    mmu_port = SOC_PORT_P2M_MAPPING(phy_port);

    SOC_IF_ERROR_RETURN(soc_fl_64q_port_check(unit, p, &is_64q));

    if (!is_64q) {
        *idx = (mmu_port * SOC_FL_LEGACY_QUEUE_NUM) + q;
    }  else {
        *idx = (SOC_FL_64Q_MMU_PORT_IDX_MIN * SOC_FL_LEGACY_QUEUE_NUM) + \
               ((mmu_port-SOC_FL_64Q_MMU_PORT_IDX_MIN) * \
                 SOC_FL_QLAYER_COSQ_PER_PORT_NUM) + \
               q;
    }
    return SYS_OK;
}

/*
 * For calculate FL MMU_MAX/MIN_BUCKET_QGROUP table index.
 *    - mmu port 0~57 : not available
 *    - mmu port 58~65 : 8 entries
 *  (IN) p : logical port
 *  (IN) g : queue group id
 *  (OUT) idx : the entry index of MMU_MAX/MIN_BUCKET_QGROUP table
 */
static int
soc_fl_mmu_bucket_qgroup_index(int unit, int p, int g, int *idx)
{
    int is_64q = 0;
    int phy_port;
    int mmu_port;

    SOC_IF_ERROR_RETURN(soc_fl_64q_port_check(unit, p, &is_64q));
    if (!is_64q) {
        /* MMU port 0~57 do not have QGROUP tables */
        return SYS_ERR_PARAMETER;
    }

    phy_port = SOC_PORT_L2P_MAPPING(p);
    mmu_port = SOC_PORT_P2M_MAPPING(phy_port);
    *idx = ((mmu_port - SOC_FL_64Q_MMU_PORT_IDX_MIN) * \
             SOC_FL_QGROUP_PER_PORT_NUM) + g;

    return SYS_OK;
}

static int
_soc_egress_metering_thaw(int unit, uint8 lport, void *setting)
{
    int rv;
    int i;
    uint32 *buffer = setting;
    int count;
    int qlayer_entry_count = 0;
    int qgroup_entry_count = 0;
    int memidx;
    int is_64q_port;
    MMU_MAX_BUCKET_QLAYERm_t mmu_max_bucket_qlayer;
    MMU_MAX_BUCKET_QGROUPm_t mmu_max_bucket_qgroup;
    EGRMETERINGCONFIGr_t egrmeteringconfig;

    rv = soc_fl_64q_port_check(unit, lport, &is_64q_port);
    if (SOC_FAILURE(rv)) {
        sal_free(setting);
        return rv;
    }

    if (is_64q_port) {
        qlayer_entry_count = SOC_FL_QLAYER_COSQ_PER_PORT_NUM;
        qgroup_entry_count = SOC_FL_QGROUP_PER_PORT_NUM;
    } else {
        qlayer_entry_count = SOC_FL_LEGACY_QUEUE_NUM;
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
    rv = soc_fl_mmu_bucket_qlayer_index(unit, lport, 0, &memidx);
    if (SOC_FAILURE(rv)) {
        sal_free(setting);
        return rv;
    }
    for (i = memidx; i < qlayer_entry_count; i++) {
        rv = READ_MMU_MAX_BUCKET_QLAYERm(unit, i, mmu_max_bucket_qlayer);
        if (SOC_FAILURE(rv)) {
            sal_free(setting);
            return rv;
        }
        MMU_MAX_BUCKET_QLAYERm_REFRESHf_SET(mmu_max_bucket_qlayer,
                                            buffer[count]);
        count++;
        MMU_MAX_BUCKET_QLAYERm_THD_SELf_SET(mmu_max_bucket_qlayer,
                                            buffer[count]);
        count++;
        rv = WRITE_MMU_MAX_BUCKET_QLAYERm(unit, i, mmu_max_bucket_qlayer);
        if (SOC_FAILURE(rv)) {
            sal_free(setting);
            return rv;
        }
    }

    /* restore the original configuration : MMU_MAX_BUCKET_QGROUPm */
    if (qgroup_entry_count) {
        rv = soc_fl_mmu_bucket_qgroup_index(unit, lport, 0, &memidx);
        if (SOC_FAILURE(rv)) {
            sal_free(setting);
            return rv;
        }
        for (i = memidx; i < qgroup_entry_count; i++) {
            rv = READ_MMU_MAX_BUCKET_QGROUPm(unit, i, mmu_max_bucket_qgroup);
            if (SOC_FAILURE(rv)) {
                sal_free(setting);
                return rv;
            }

            MMU_MAX_BUCKET_QGROUPm_REFRESHf_SET(mmu_max_bucket_qgroup,
                                                buffer[count]);
            count++;
            MMU_MAX_BUCKET_QGROUPm_THD_SELf_SET(mmu_max_bucket_qgroup,
                                                buffer[count]);
            count++;
            rv = WRITE_MMU_MAX_BUCKET_QGROUPm(unit, i, mmu_max_bucket_qgroup);
            if (SOC_FAILURE(rv)) {
                sal_free(setting);
                return rv;
            }
        }
    }

    /* restore the original configuration : EGRMETERINGCONFIGr */
    EGRMETERINGCONFIGr_SET(egrmeteringconfig, buffer[count]);
    rv = WRITE_EGRMETERINGCONFIGr(unit, lport, egrmeteringconfig);

    sal_free(setting);
    return rv;
}

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
    int cos;
    COSLCCOUNTr_t coslccount;
    COSLCCOUNT_QGROUPr_t coslccount_qgroup;

    *count = 0;

    for (cos = 0; cos < COSLCCOUNT_NUM; cos++) {
        SOC_IF_ERROR_RETURN(READ_COSLCCOUNTr(unit, lport, cos, coslccount));
        *count += COSLCCOUNTr_LCCOUNTf_GET(coslccount);
    }

    for (cos = 0; cos < COSLCCOUNT_QGROUP_NUM; cos++) {
        SOC_IF_ERROR_RETURN
            (READ_COSLCCOUNT_QGROUPr(unit, lport, cos, coslccount_qgroup));
        *count += COSLCCOUNT_QGROUPr_LCCOUNTf_GET(coslccount_qgroup);
    }

    return SYS_OK;
}

static int
soc_mmu_backpressure_clear(int unit, uint8 lport)
{
    /* Do nothing for FL */
    return SYS_OK;
}

static int
_soc_egress_metering_freeze(int unit, uint8 lport, void **setting){
    int rv;
    int i;
    uint32 value;
    uint32 *buffer;
    int count;
    int qlayer_entry_count = 0;
    int qgroup_entry_count = 0;
    int memidx;
    int is_64q_port;
    MMU_MAX_BUCKET_QLAYERm_t mmu_max_bucket_qlayer;
    MMU_MAX_BUCKET_QGROUPm_t mmu_max_bucket_qgroup;
    EGRMETERINGCONFIGr_t egrmeteringconfig;

    rv = soc_fl_64q_port_check(unit, lport, &is_64q_port);
    if (SOC_FAILURE(rv)) {
        return rv;
    }

    if (is_64q_port) {
        qlayer_entry_count = SOC_FL_QLAYER_COSQ_PER_PORT_NUM;
        qgroup_entry_count = SOC_FL_QGROUP_PER_PORT_NUM;
    } else {
        qlayer_entry_count = SOC_FL_LEGACY_QUEUE_NUM;
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
        rv = SYS_ERR_OUT_OF_RESOURCE;
        return rv;
    }
    sal_memset(buffer, 0, (count * sizeof(uint32)));

    /* save the original configuration : MMU_MAX_BUCKET_QLAYERm */
    count = 0;
    rv = soc_fl_mmu_bucket_qlayer_index(unit, lport, 0, &memidx);
    if (SOC_FAILURE(rv)) {
        sal_free(buffer);
        return rv;
    }
    for (i = memidx; i < qlayer_entry_count; i++) {
        rv = READ_MMU_MAX_BUCKET_QLAYERm(unit, i, mmu_max_bucket_qlayer);
        if (SOC_FAILURE(rv)) {
            sal_free(buffer);
            return rv;
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
            return rv;
        }
    }

    /* save the original configuration : MMU_MAX_BUCKET_QGROUPm */
    if (qgroup_entry_count) {
        rv = soc_fl_mmu_bucket_qgroup_index(unit, lport, 0, &memidx);
        if (SOC_FAILURE(rv)) {
            sal_free(buffer);
        }
        for (i = memidx; i < qgroup_entry_count; i++) {
            rv = READ_MMU_MAX_BUCKET_QGROUPm(unit, i, mmu_max_bucket_qgroup);
            if (SOC_FAILURE(rv)) {
                sal_free(buffer);
                return rv;
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
                return rv;
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

/*******************************************************************************
 * Public functions
 */

int
soc_port_speed_update(int unit, uint8 lport, int speed)
{
    if (speed > SOC_PORT_SPEED_MAX(lport)) {
        return SYS_ERR_PARAMETER;
    }

    SOC_PORT_SPEED_INIT(lport) = speed;

    return SYS_OK;
}

int
soc_port_egress_buffer_sft_reset(int unit, uint8 lport, int reset)
{
    /* Do nothing for FL */
    return SYS_OK;
}

int
soc_port_ingress_buffer_reset(int unit, uint8 lport, int reset)
{
    /* Do nothing for FL */
    return SYS_OK;
}

int
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
    rv = SYS_OK;
    for (;;) {
        rv = soc_egress_cell_count(unit, lport, &new_cells);
        if (rv < 0) {
            break;
        }

        if (new_cells == 0) {
            rv = SYS_OK;
            break;
        }

        if (new_cells < cur_cells) { /* Progress made */
            cur_cells = new_cells;
        }

        sal_usleep(1000);
        drain_timeout -= 1000;
        if (drain_timeout <= 0) {
            rv = soc_egress_cell_count(unit, lport, &new_cells);
            if (rv < 0) {
                break;
            }

            sal_printf("%s..: unit %d lport %d drain_timeout\n",
                       __func__, unit, lport);
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

int
soc_mmu_flush_enable(int unit, uint8 lport, int enable)
{
    int ridx, pidx;
    uint32 flush_ctrl;
    MMUFLUSHCONTROLr_t mmuflushcontrol;

    ridx = lport / 32;
    pidx = lport - ridx * 32;

    /*
     * First put the port in flush state - the packets from the XQ of the
     * port are purged after dequeue.
     */
    SOC_IF_ERROR_RETURN
        (READ_MMUFLUSHCONTROLr(unit, ridx, mmuflushcontrol));
    flush_ctrl = MMUFLUSHCONTROLr_FLUSHf_GET(mmuflushcontrol);
    flush_ctrl &= ~(0x1 << pidx);
    flush_ctrl |= enable ? (0x1 << pidx) : 0;
    MMUFLUSHCONTROLr_FLUSHf_SET(mmuflushcontrol, flush_ctrl);
    SOC_IF_ERROR_RETURN
        (WRITE_MMUFLUSHCONTROLr(unit, ridx, mmuflushcontrol));

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit=%d lport %d MMUFLUSHCONTROL_%dr=0x%x\n",
                 __func__, unit, lport, ridx, flush_ctrl));

    return SYS_OK;
}

int
soc_port_mmu_buffer_enable(int unit, uint8 lport, int enable)
{
    int ridx, pidx;
    int phy_port, mmu_port;
    uint32 enable_ctrl;
    MMUPORTENABLEr_t mmuportenable;

    phy_port = SOC_PORT_L2P_MAPPING(lport);
    mmu_port = SOC_PORT_P2M_MAPPING(phy_port);

    ridx = mmu_port / 32;
    pidx = mmu_port - ridx * 32;

    SOC_IF_ERROR_RETURN
        (READ_MMUPORTENABLEr(unit, ridx, mmuportenable));
    enable_ctrl = MMUPORTENABLEr_MMUPORTENABLEf_GET(mmuportenable);
    if (enable) {
        enable_ctrl |= 0x1 << pidx;
    } else {
        enable_ctrl &= ~(0x1 << pidx);
    }
    MMUPORTENABLEr_MMUPORTENABLEf_SET(mmuportenable, enable_ctrl);
    SOC_IF_ERROR_RETURN
        (WRITE_MMUPORTENABLEr(unit, ridx, mmuportenable));

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                ("%s..:unit=%d lport %d MMUPORTENABLE_%dr=0x%x\n",
                 __func__, unit, lport, ridx, enable_ctrl));

    return SYS_OK;
}

int
soc_port_epc_link_set(int unit, uint8 lport, int link)
{
    EPC_LINK_BMAP_HI_64r_t epc_link_bmap_hi_64;
    EPC_LINK_BMAP_LO_64r_t epc_link_bmap_lo_64;

    if (lport >= 64) {
        SOC_IF_ERROR_RETURN
            (READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64));
        if (link) {
            PBMP_PORT_ADD(*((pbmp_t *)&epc_link_bmap_hi_64), (lport - 64));
        } else {
            PBMP_PORT_REMOVE(*((pbmp_t *)&epc_link_bmap_hi_64), (lport - 64));
        }
        SOC_IF_ERROR_RETURN
            (WRITE_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64));
    } else {
        SOC_IF_ERROR_RETURN
            (READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64));
        if (link) {
            PBMP_PORT_ADD(*((pbmp_t *)&epc_link_bmap_lo_64), (lport));
        } else {
            PBMP_PORT_REMOVE(*((pbmp_t *)&epc_link_bmap_lo_64), (lport));
        }
        SOC_IF_ERROR_RETURN
            (WRITE_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64));
    }

    return SYS_OK;
}

int
soc_port_epc_link_get(int unit, uint8 lport, int *link)
{
    EPC_LINK_BMAP_HI_64r_t epc_link_bmap_hi_64;
    EPC_LINK_BMAP_LO_64r_t epc_link_bmap_lo_64;

    *link = 0;
    if (lport >= 64) {
        SOC_IF_ERROR_RETURN
            (READ_EPC_LINK_BMAP_HI_64r(unit, epc_link_bmap_hi_64));
        if (PBMP_MEMBER(*((pbmp_t *)&epc_link_bmap_hi_64), (lport - 64))) {
            *link = 1;
        }
    } else {
        SOC_IF_ERROR_RETURN
            (READ_EPC_LINK_BMAP_LO_64r(unit, epc_link_bmap_lo_64));
        if (PBMP_MEMBER(*((pbmp_t *)&epc_link_bmap_lo_64), lport)) {
            *link = 1;
        }
    }

    return SYS_OK;
}
