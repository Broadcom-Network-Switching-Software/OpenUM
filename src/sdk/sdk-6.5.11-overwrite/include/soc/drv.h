/*
 * 
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * This file contains structure and routine declarations for the
 * Switch-on-a-Chip Driver.
 *
 * This file also includes the more common include files so the
 * individual driver files don't have to include as much.
 */

#ifndef _SOC_DRV_H
#define _SOC_DRV_H

#include <sal/core/time.h>
#include <sal/core/boot.h>

#include <assert.h>

#include <shared/avl.h>
#include <shared/bitop.h>
#include <shared/fifo.h>
#include <shared/switch.h>
#include <shared/gport.h>

#include <soc/util.h>
#include <soc/error.h>
#include <soc/cm.h>
#include <soc/feature.h>
#include <soc/property.h>
#include <soc/dport.h>

#include <shared/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/sync.h>
#include <sal/core/thread.h>

#include <soc/chip.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/error.h>
#include <soc/dma.h>
#include <soc/enet.h>
#include <soc/counter.h>
#include <soc/mmuerr.h>
#include <soc/drv_if.h>
#include <soc/types.h>
#include <soc/macipadr.h>
#include <soc/ll.h>
#include <soc/axp.h>
#include <soc/intr.h>
#include <soc/mem.h>

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || defined(BCM_SAND_SUPPORT)
#ifndef SWIG_C3_BUILD
#include <soc/mcm/memregs.h>
#endif
#endif

#ifdef BCM_ROBO_SUPPORT
#ifndef SWIG_C3_BUILD
#include <soc/robo/mcm/memregs.h>
#endif
#endif

#ifdef BCM_ISM_SUPPORT
#include <soc/ism.h>
#include <soc/ism_hash.h>
#endif
#ifdef BCM_CMICM_SUPPORT
#include <soc/shared/mos_intr_common.h>
#include <soc/shared/mos_msg_common.h>
#ifdef BCM_RCPU_SUPPORT
#include <soc/rcpu.h>
#endif
#endif

#if defined(BCM_CMICM_SUPPORT) || defined(BCM_IPROC_SUPPORT) || defined(BCM_UC_MHOST_SUPPORT)
#include <soc/uc_msg.h>
#endif

#ifdef BCM_SBUSDMA_SUPPORT
#include <soc/sbusdma.h>
#endif

#ifdef BCM_CCMDMA_SUPPORT
#include <soc/ccmdma.h>
#endif

#ifdef BCM_ROBO_SUPPORT
#include <soc/robo/robo_drv.h>
#endif
#ifdef BCM_EA_SUPPORT
#include <soc/ea/allenum.h>
#ifdef BCM_TK371X_SUPPORT
#include <soc/ctrl_if.h>
#endif
#endif


#ifdef  BCM_CALADAN3_SUPPORT
#include <soc/sbx/caladan3/port.h>
#endif

/****************************************************************
 * UNIT DRIVER ACCESS MACROS
 *
 *         MACRO                             EVALUATES TO
 *  ________________________________________________________________
 *      SOC_DRIVER(unit)                Chip driver structure
 *      SOC_INFO(unit)                  SOC Info structure
 *      SOC_MEM_INFO(unit,mem)          Memory info structure
 *      SOC_REG_INFO(unit,reg)          Register info structure
 *      SOC_BLOCK_INFO(unit,blk)        Block info structure
 *      SOC_PORT_INFO(unit,port)        Port info structure
 *      SOC_BLOCK2SCH(unit,blk)         Integer schan num for block
 *      SOC_BLOCK2OFFSET(unit,blk)      Block to idx for cmic cmds
 *      SOC_HAS_CTR_TYPE(unit, ctype)   Does the device have a given
 *                                      counter map defined?
 *      SOC_CTR_DMA_MAP(unit, ctype)    Return pointer to the counter
 *                                      map of the indicated type.
 *      SOC_CTR_TO_REG(unit, ctype, ind) Return the register index for
 *                                       a given counter index.
 *      SOC_CTR_MAP_SIZE(unit, ctype)   How many entries in a given
 *                                      counter map.
 */

#define SOC_CONTROL(unit)               (soc_control[unit])
#define SOC_DRIVER(unit)                (SOC_CONTROL(unit)->chip_driver)
#define SOC_FUNCTIONS(unit)             (SOC_CONTROL(unit)->soc_functions)
#define SOC_PORTCTRL_FUNCTIONS(unit)    (SOC_CONTROL(unit)->soc_portctrl_functions)
#define SOC_INFO(unit)                  (SOC_CONTROL(unit)->info)
#define SOC_STAT(unit)                  (&(SOC_CONTROL(unit)->stat))
#define SOC_REG_MASK_SUBSET(unit)       (SOC_CONTROL(unit)->reg_subset_mask)

#define DRV_SERVICES(unit)              (SOC_DRIVER(unit)->services)
#define SOC_MEM_INFO(unit, mem)         (*SOC_DRIVER(unit)->mem_info[mem])
#define SOC_MEM_AGGR(unit, index)       (SOC_DRIVER(unit)->mem_aggr[index])
#define SOC_MEM_UNIQUE_ACC(unit, mem)   (SOC_DRIVER(unit)->mem_unique_acc[mem])
#define SOC_MEM_PTR(unit, mem)          (SOC_DRIVER(unit)->mem_info[mem])
#define SOC_REG_INFO(unit, reg)         (*SOC_DRIVER(unit)->reg_info[reg])
#define SOC_REG_STAGE(unit, reg)        ((SOC_REG_INFO(unit, reg).offset >> 26) & 0x3F)
#define SOC_REG_UNIQUE_ACC(unit, reg)   (SOC_DRIVER(unit)->reg_unique_acc[reg])
#define SOC_REG_ABOVE_64_INFO(unit, reg) (*SOC_DRIVER(unit)->reg_above_64_info[reg])
#define SOC_REG_ARRAY_INFO(unit, reg)   (*SOC_DRIVER(unit)->reg_array_info[reg])
#define SOC_REG_ARRAY_INFOP(unit, reg)  (SOC_DRIVER(unit)->reg_array_info[reg])
#define SOC_MEM_ARRAY_INFO(unit, mem)   (*SOC_DRIVER(unit)->mem_array_info[mem])
#define SOC_MEM_ARRAY_INFOP(unit, mem)  (SOC_DRIVER(unit)->mem_array_info[mem])
#define SOC_REG_PTR(unit, reg)          (SOC_DRIVER(unit)->reg_info[reg])
#define SOC_BLOCK_INFO(unit, blk)       (SOC_DRIVER(unit)->block_info[blk])
#define SOC_PORT_IDX_INFO(unit, port, idx) \
    (SOC_DRIVER(unit)->port_info[SOC_DRIVER(unit)->port_num_blktype > 1 ? \
                                 (port) * SOC_DRIVER(unit)->port_num_blktype + \
                                 (idx) : (port)])
#define SOC_PORT_INFO(unit, port)       SOC_PORT_IDX_INFO(unit, port, 0)

#define SOC_CHIP_STRING(unit)           (SOC_DRIVER(unit)->chip_string)
#define SOC_ORIGIN(unit)                (SOC_DRIVER(unit)->origin)
#define SOC_PCI_VENDOR(unit)            (SOC_DRIVER(unit)->pci_vendor)
#define SOC_PCI_DEVICE(unit)            (SOC_DRIVER(unit)->pci_device)
#define SOC_PCI_REVISION(unit)          (SOC_DRIVER(unit)->pci_revision)
#define SOC_MEM_UNIQUE_ACC_XPE_PIPE(unit, mem, xpe, pipe) \
    (SOC_DRIVER(unit)->mem_unique_acc[mem])[(xpe * NUM_XPE(unit)) + pipe]

#define SOC_MEM_UNIQUE_ACC_PIPE(unit, mem, pipe) \
    (SOC_DRIVER(unit)->mem_unique_acc[mem])[pipe]
#define SOC_MEM_UNIQUE_ACC_XPE(unit, mem, xpe) \
    (SOC_DRIVER(unit)->mem_unique_acc[mem])[xpe]
#define SOC_MEM_UNIQUE_ACC_SC(unit, mem, sc) \
    (SOC_DRIVER(unit)->mem_unique_acc[mem])[sc]

#define SOC_REG_UNIQUE_ACC_PIPE(unit, reg, pipe) \
    (SOC_DRIVER(unit)->reg_unique_acc[reg])[pipe]
#define SOC_REG_UNIQUE_ACC_XPE(unit, reg, xpe) \
    (SOC_DRIVER(unit)->reg_unique_acc[reg])[xpe]

#define SOC_REG_ADDR_STAGE(addr)        ((addr >> 26) & 0x3F)
#define    SOC_MEM_ADDR_OFFSET(_addr)      ((_addr) & 0x000fffff)
#define    SOC_MEM_ADDR_STAGE(_addr)       (((_addr) >> 24) & 0x3f)
#ifdef BCM_EXTND_SBUS_SUPPORT
#define    SOC_MEM_ADDR_NUM_EXTENDED(_addr) (((_addr) >> 18) & 0xff)
#define    SOC_MEM_ADDR_OFFSET_EXTENDED_IPIPE(_addr) ((_addr) & 0x0003ffff)
#define    SOC_MEM_ADDR_OFFSET_EXTENDED(_addr) ((_addr) & 0x03ffffff)
#define    SOC_MEM_ADDR_STAGE_EXTENDED(_addr) (((_addr) >> 26) & 0x3f)
#endif

#define SOC_BLOCK2SCH(unit, blk)        (SOC_BLOCK_INFO(unit,blk).schan)
#define SOC_BLOCK2OFFSET(unit, blk)     (SOC_BLOCK_INFO(unit,blk).cmic)

#define SOC_PERSIST(unit)               (soc_persist[unit])

#define SOC_IRQ_MASK(unit)              (SOC_CONTROL(unit)->irq_mask)
#define SOC_IRQ1_MASK(unit)             (SOC_CONTROL(unit)->irq1_mask)
#define SOC_IRQ2_MASK(unit)             (SOC_CONTROL(unit)->irq2_mask)

#define SOC_SWITCH_EVENT_NOMINAL_STORM(unit)    (SOC_CONTROL(unit)->switch_event_nominal_storm)

#define SOC_CMCS_NUM(unit)              (SOC_CONTROL(unit)->cmc_num)
#define SOC_PCI_CMCS_NUM(unit)          (SOC_CONTROL(unit)->pci_cmc_num)
#define SOC_PCI_CMC(unit)               (SOC_CONTROL(unit)->pci_cmc)
#ifdef BCM_CMICM_SUPPORT
#define SOC_CMCx_NUM_SBUSDMA            (3)
#define SOC_ARM_CMC(unit, arm)          (SOC_CONTROL(unit)->arm_cmc[(arm)])
#define SOC_CMCx_IRQ0_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq0_mask[cmc])
#define SOC_CMCx_IRQ1_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq1_mask[cmc])
#define SOC_CMCx_IRQ2_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq2_mask[cmc])
#define SOC_CMCx_IRQ3_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq3_mask[cmc])
#define SOC_CMCx_IRQ4_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq4_mask[cmc])
#define SOC_CMCx_IRQ5_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq5_mask[cmc])
#define SOC_CMCx_IRQ6_MASK(unit,cmc)    (SOC_CONTROL(unit)->cmc_irq6_mask[cmc])
#define CPU_ARM_QUEUE_BITMAP(unit,cmc)  (SOC_CONTROL(unit)->cpu_arm_queues[cmc])
#define CPU_ARM_RSVD_QUEUE_BITMAP(unit,cmc)  (SOC_CONTROL(unit)->cpu_arm_reserved_queues[cmc])
#define NUM_CPU_ARM_COSQ(unit, cmc)     (SOC_CONTROL(unit)->num_cpu_arm_cosq[cmc])
#define SOC_VCHAN_NUM(unit)             (SOC_CONTROL(unit)->soc_max_channels)
#define SOC_DCHAN_NUM(unit)             (SOC_CONTROL(unit)->soc_dma_channels)

#ifdef BCM_RCPU_SUPPORT
#define SOC_RCPU_IRQ0_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq0_mask)
#define SOC_RCPU_IRQ1_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq1_mask)
#define SOC_RCPU_IRQ2_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq2_mask)
#define SOC_RCPU_IRQ3_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq3_mask)
#define SOC_RCPU_IRQ4_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq4_mask)
#define SOC_RCPU_IRQ5_MASK(unit)        (SOC_CONTROL(unit)->rpe_irq5_mask)
#define SOC_RCPU_CMC0_IRQ0_MASK(unit)   (SOC_CONTROL(unit)->rpe_pcie_irq0_mask)
#define SOC_RCPU_CMC1_IRQ0_MASK(unit)   (SOC_CONTROL(unit)->rpe_uc0_irq0_mask)
#define SOC_RCPU_CMC2_IRQ0_MASK(unit)   (SOC_CONTROL(unit)->rpe_uc1_irq0_mask)
#endif /* BCM_RCPU_SUPPORT */

#ifdef BCM_SBUSDMA_SUPPORT
#define SOC_SBUSDMA_INFO(unit)          (SOC_CONTROL(unit)->sbd_dm_inf)
#endif
#else
#define SOC_CMCS_NUM_MAX                (1)
#endif /* BCM_CMICM_SUPPORT */
#ifdef BCM_ISM_SUPPORT
#define SOC_ISM_INFO(unit)              (SOC_CONTROL(unit)->ism)
#define SOC_ISM_INFO_MAX_BANKS(unit)    (SOC_ISM_INFO(unit)->max_banks)
#define SOC_ISM_HASH_INFO(unit)         (SOC_CONTROL(unit)->ism_hash)
#endif /* BCM_ISM_SUPPORT */
#ifdef BCM_DDR3_SUPPORT
#define SOC_DDR3_NUM_COLUMNS(unit)           (SOC_CONTROL(unit)->ddr3_num_columns)
#define SOC_DDR3_NUM_ROWS(unit)              (SOC_CONTROL(unit)->ddr3_num_rows)
#define SOC_DDR3_NUM_BANKS(unit)             (SOC_CONTROL(unit)->ddr3_num_banks)
#define SOC_DDR3_NUM_MEMORIES(unit)          (SOC_CONTROL(unit)->ddr3_num_memories)
#define SOC_DDR3_CLOCK_MHZ(unit)             (SOC_CONTROL(unit)->ddr3_clock_mhz)
#define SOC_DDR3_MEM_GRADE(unit)             (SOC_CONTROL(unit)->ddr3_mem_grade)
#define SOC_DDR3_OFFSET_WR_DQ_CI02_WL0(unit) (SOC_CONTROL(unit)->ddr3_offset_wr_dq_ci02_wl0)
#define SOC_DDR3_OFFSET_WR_DQ_CI00_WL1(unit) (SOC_CONTROL(unit)->ddr3_offset_wr_dq_ci00_wl1)
#endif /* BCM_DDR3_SUPPORT */
#define SOC_IS_RCPU_UNIT(unit)          (SOC_CONTROL(unit)->remote_cpu)
#define SOC_IS_RCPU_SCHAN(unit)         (SOC_CONTROL(unit)->soc_flags & SOC_F_RCPU_SCHAN)
#define SOC_IS_RCPU_ONLY(unit)          (SOC_CONTROL(unit)->soc_flags & SOC_F_RCPU_ONLY)
#define SOC_DPORT_MAP(unit)             (SOC_CONTROL(unit)->dport_map)
#define SOC_DPORT_RMAP(unit)            (SOC_CONTROL(unit)->dport_rmap)
#define SOC_DPORT_MAP_FLAGS(unit)       (SOC_CONTROL(unit)->dport_map_flags)

/*Linkscan*/
#ifdef BCM_LINKSCAN_LOCK_PER_UNIT
#define SOC_LINKSCAN_LOCK(unit, s)                                                  \
        if (soc_feature(unit, soc_feature_linkscan_lock_per_unit)) {                \
            sal_mutex_take(SOC_CONTROL(unit)->linkscan_mutex, sal_mutex_FOREVER);   \
            s = 0;                                                                  \
        } else {                                                                    \
            s = sal_splhi();                                                        \
        }

#define SOC_LINKSCAN_UNLOCK(unit, s)                                                \
        if (soc_feature(unit, soc_feature_linkscan_lock_per_unit)) {                \
            sal_mutex_give(SOC_CONTROL(unit)->linkscan_mutex);                      \
            s = 0;                                                                  \
        } else {                                                                    \
            sal_spl(s);                                                             \
        }
#else
#define SOC_LINKSCAN_LOCK(unit, s)                                                  \
            s = sal_splhi();
#define SOC_LINKSCAN_UNLOCK(unit, s)                                                \
            sal_spl(s);
#endif /*BCM_LINKSCAN_LOCK_PER_UNIT*/


#ifdef BCM_TIMESYNC_SUPPORT
#define SOC_TIMESYNC_PLL_CLOCK_NS(unit) \
        (SOC_CONTROL(unit)->timesync_pll_clock_ns)
#endif /* BCM_TIMESYNC_SUPPORT */
/* rval must be 64-bit value */
#define SOC_REG_RST_VAL_GET(unit, reg, rval) \
    if(!SOC_REG_IS_ABOVE_64(unit, reg)) {\
        COMPILER_64_SET(rval, SOC_REG_INFO(unit, reg).rst_val_hi, SOC_REG_INFO(unit, reg).rst_val_lo); \
    } else { \
        COMPILER_64_SET(rval, 0, 0); \
    }

#define SOC_REG_RST_MSK_GET(unit, reg, rmsk) \
    if(!SOC_REG_IS_ABOVE_64(unit, reg)) {\
        COMPILER_64_SET(rmsk, SOC_REG_INFO(unit, reg).rst_mask_hi, SOC_REG_INFO(unit, reg).rst_mask_lo); \
    } else { \
        COMPILER_64_SET(rmsk, 0, 0); \
    }

/* rval for above 64-bit value */
/* Coverity fix 21831 and 21832 : SOC_REG_ABOVE_64_CLEAR is removed in else part   as rval is not above 64 bit & SOC_REG_ABOVE_64_CLEAR results in out of bounds   access while doing memset 
*/
#define SOC_REG_ABOVE_64_RST_VAL_GET(unit, reg, rval) \
    if(SOC_REG_IS_ABOVE_64(unit, reg)) {\
        SOC_REG_ABOVE_64_CLEAR(rval); \
        SHR_BITCOPY_RANGE(rval, 0, SOC_REG_ABOVE_64_INFO(unit, reg).reset, 0, SOC_REG_ABOVE_64_INFO(unit, reg).size*32); \
    } else { \
        /* SOC_REG_ABOVE_64_CLEAR(rval); Coverity fix */ \
        rval[0] = SOC_REG_INFO(unit, reg).rst_val_lo; \
        rval[1] = SOC_REG_INFO(unit, reg).rst_val_hi; \
    }


#define SOC_REG_ABOVE_64_RST_MSK_GET(unit, reg, rmsk) \
    if(SOC_REG_IS_ABOVE_64(unit, reg)) {\
        SOC_REG_ABOVE_64_CLEAR(rmsk); \
        SHR_BITCOPY_RANGE(rmsk, 0, SOC_REG_ABOVE_64_INFO(unit, reg).mask, 0, SOC_REG_ABOVE_64_INFO(unit, reg).size*32); \
    } else { \
        SOC_REG_ABOVE_64_CLEAR(rmsk); \
        rmsk[0] = SOC_REG_INFO(unit, reg).rst_mask_lo; \
        rmsk[1] = SOC_REG_INFO(unit, reg).rst_mask_hi; \
    }
/*
 * Counter map macros
 * Assumes "ctype" is of SOC_CTR_TYPE_xxx
 */
#define SOC_HAS_CTR_TYPE(unit, ctype) \
    (SOC_DRIVER(unit)->counter_maps[ctype].cmap_base != NULL)
/* Reference to array of counters */
#define SOC_CTR_DMA_MAP(unit, ctype)  \
    (SOC_DRIVER(unit)->counter_maps[ctype])
/* Map a counter index to a register index */
#define SOC_CTR_TO_REG(unit, ctype, ind)  \
    (SOC_DRIVER(unit)->counter_maps[ctype].cmap_base[ind].reg)
#define SOC_CTR_TO_REG_IDX(unit, ctype, ind)  \
    (SOC_DRIVER(unit)->counter_maps[ctype].cmap_base[ind].index)
/* Right now, this is really per chip, not per counter type. */
#define SOC_CTR_MAP_SIZE(unit, ctype)   \
    (SOC_DRIVER(unit)->counter_maps[ctype].cmap_size)

#define SOC_REG_CTR_IDX(unit, reg) \
    (SOC_REG_INFO(unit, reg).ctr_idx)

#define SOC_REG_NUMELPORTLIST_IDX(unit, reg) \
    (SOC_REG_INFO(unit, reg).numelportlist_idx)

#ifdef BCM_ROBO_SUPPORT
#define SOC_MEM_IS_VALID(unit, mem) \
    ((mem >= 0 && mem < (SOC_IS_ROBO(unit) ? NUM_SOC_ROBO_MEM : NUM_SOC_MEM)) && \
     (SOC_CONTROL(unit) != NULL) && \
     (SOC_DRIVER(unit) != NULL) && \
     (SOC_MEM_PTR(unit, mem) != NULL) && \
     (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_VALID))
#else /* !BCM_ROBO_SUPPORT */
#define SOC_MEM_IS_VALID(unit, mem) \
    ((mem >= 0 && mem < NUM_SOC_MEM) && \
     (SOC_CONTROL(unit) != NULL) && \
     (SOC_DRIVER(unit) != NULL) && \
     (SOC_MEM_PTR(unit, mem) != NULL) && \
     (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_VALID))
#endif /* BCM_ROBO_SUPPORT */

#define SOC_MEM_IS_ENABLED(unit, mem) \
    (SOC_MEM_IS_VALID(unit, mem) && \
     !(SOC_MEM_INFO(unit, mem).flags & \
       SOC_CONTROL(unit)->disabled_mem_flags))

#define SOC_MEM_IS_ARRAY(unit, mem)     \
    (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_IS_ARRAY)
#define SOC_MEM_IS_ARRAY_SAFE(unit, mem)     \
    ( SOC_MEM_IS_ARRAY(unit, mem) && SOC_MEM_ARRAY_INFOP(unit, mem) )
#define SOC_MEM_ELEM_SKIP(unit, mem)     \
    ((SOC_IS_ARAD(unit) && (mem == NBI_TBINS_MEMm || mem == NBI_RBINS_MEMm))?     \
    (SOC_MEM_INFO(unit, mem).index_max - SOC_MEM_INFO(unit, mem).index_min + 1) : (SOC_MEM_ARRAY_INFO(unit, mem).element_skip))
#define SOC_MEM_NUMELS(unit, mem)     \
    (SOC_MEM_ARRAY_INFO(unit, mem).numels)
#define SOC_MEM_ELEM_SKIP_SAFE(unit, mem)     \
    ( SOC_MEM_ARRAY_INFOP(unit, mem) ? SOC_MEM_ARRAY_INFOP(unit, mem)->element_skip : 0 )
#define SOC_MEM_NUMELS_SAFE(unit, mem)     \
    ( SOC_MEM_ARRAY_INFOP(unit, mem) ? SOC_MEM_ARRAY_INFOP(unit, mem)->numels : 1 )


#ifdef BCM_ROBO_SUPPORT
#define SOC_REG_IS_VALID(unit, reg) \
    ((reg >= 0 && reg < (SOC_IS_ROBO(unit) ? NUM_SOC_ROBO_REG : NUM_SOC_REG)) && \
     (SOC_REG_PTR(unit, reg) != NULL))
#else /* !BCM_ROBO_SUPPORT */
#define SOC_REG_IS_VALID(unit, reg)                            \
        ((reg >= 0 && reg < NUM_SOC_REG)  &&                   \
         (SOC_REG_PTR(unit, reg) != NULL) &&                   \
         (SOC_REG_INFO(unit, reg).regtype != soc_invalidreg))
#endif /* BCM_ROBO_SUPPORT */

#define SOC_MAX_COUNTER_NUM(unit)  \
        ((IS_FE_PORT(unit,0)) ?  SOC_CTR_MAP_SIZE(unit,SOC_CTR_TYPE_FE) : \
        SOC_CTR_MAP_SIZE(unit, SOC_CTR_TYPE_GE))


#define SOC_REG_IS_ENABLED(unit, reg) \
    (SOC_REG_IS_VALID(unit, reg) && \
     !(SOC_REG_INFO(unit, reg).flags & \
       SOC_CONTROL(unit)->disabled_reg_flags))

#define SOC_MEM_BYTES(unit, mem)        (SOC_MEM_INFO(unit, mem).bytes)
#define SOC_MEM_WORDS(unit, mem)        (BYTES2WORDS(SOC_MEM_BYTES(unit, mem)))

#define SOC_MEM_BASE(unit, mem)     (SOC_MEM_INFO(unit, mem).base)
#define SOC_MEM_SIZE(unit, mem) \
    (SOC_MEM_INFO(unit, mem).index_max - \
     SOC_MEM_INFO(unit, mem).index_min + 1)
#define SOC_MEM_TABLE_BYTES(unit, mem) \
    (4 * SOC_MEM_WORDS(unit, mem) * \
     (SOC_MEM_INFO(unit, mem).index_max - \
      SOC_MEM_INFO(unit, mem).index_min + 1))

#define SOC_REG_TYPE(unit, reg)      \
    (SOC_REG_INFO(unit, reg).regtype)

#define SOC_REG_IS_WRITE_ONLY(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_WO)

#define SOC_REG_IS_ABOVE_64(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_ABOVE_64_BITS)

#define SOC_REG_IS_64(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_64_BITS)

#define SOC_REG_IS_ABOVE_32(unit, reg) \
    (SOC_REG_INFO(unit, reg).flags & (SOC_REG_FLAG_64_BITS | SOC_REG_FLAG_ABOVE_64_BITS))

#define SOC_REG_IS_32(unit, reg) \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_32_BITS)

#define SOC_REG_IS_16(unit, reg) \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_16_BITS)

#define SOC_REG_IS_8(unit, reg) \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_8_BITS)

/* Register requires special processing */
#define SOC_REG_IS_SPECIAL(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_SPECIAL)

#define SOC_REG_ARRAY(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_ARRAY)
#define SOC_REG_ARRAY2(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_ARRAY2)
#define SOC_REG_ARRAY4(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_ARRAY4)
#define SOC_REG_IS_ARRAY(unit, reg)     \
    (SOC_REG_INFO(unit, reg).flags & SOC_REG_FLAG_REG_ARRAY)
#define SOC_REG_BASE(unit, reg)     \
    (SOC_REG_INFO(unit, reg).offset)
#define SOC_REG_NUMELS(unit, reg)     \
    (SOC_REG_INFO(unit, reg).numels)
#define SOC_REG_GRAN(unit, reg)     \
    (soc_regtype_gran[SOC_REG_INFO(unit, reg).regtype])
#define SOC_REG_PAGE(unit, reg) \
    (SOC_REG_INFO(unit, reg).page)
#define SOC_REG_ELEM_SKIP(unit, reg)     \
    (SOC_REG_ARRAY_INFO(unit, reg).element_skip)

#define SOC_CHIP_TYPE(unit)     (SOC_INFO(unit).driver_type)
#define SOC_CHIP_GROUP(unit)    (SOC_INFO(unit).driver_group)

#define SOC_IS_DIRECT_PORT(unit, port) \
    (port>=SOC_INFO(unit).physical_port_offset && \
           SOC_PBMP_MEMBER(SOC_INFO(unit).physical_pbm, port-SOC_INFO(unit).physical_port_offset+1))

/*
 * SOC_IS_* Macros.  If support for the chip is defined out of the
 * build, they are defined as zero to let the optimizer remove
 * code.
 */

/* Not supported in this version of SDK */
#define SOC_IS_DRACO1(unit)     (0)
#define SOC_IS_DRACO15(unit)    (0)
#define SOC_IS_DRACO(unit)      (0)
#define SOC_IS_HERCULES1(unit)  (0)
#define SOC_IS_LYNX(unit)       (0)
#define SOC_IS_TUCANA(unit)     (0)
#define SOC_IS_XGS12_SWITCH(unit) (0)

#ifdef  BCM_HERCULES_SUPPORT
#ifdef  BCM_HERCULES15_SUPPORT
#define SOC_IS_HERCULES15(unit) ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HERCULES15))
#else
#define SOC_IS_HERCULES15(unit) (0)
#endif
#define SOC_IS_HERCULES(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HERCULES))
#else
#define SOC_IS_HERCULES15(unit) (0)
#define SOC_IS_HERCULES(unit)   (0)
#endif

#ifdef  BCM_FIREBOLT_SUPPORT
#define SOC_IS_FIREBOLT(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FIREBOLT))
#define SOC_IS_FB(unit)         ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FB))
#define SOC_IS_FB_FX_HX(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FB_FX_HX))
#define SOC_IS_HB_GW(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HB_GW))
#define SOC_IS_HBX(unit)        ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HBX))
#define SOC_IS_FBX(unit)        ((!SOC_INFO(unit).spi_device) && \
    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_FBX) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_APACHE) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HURRICANE3) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TOMAHAWKPLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND2)))
#else
#define SOC_IS_FIREBOLT(unit)   (0)
#define SOC_IS_FB(unit)         (0)
#define SOC_IS_FB_FX_HX(unit)   (0)
#define SOC_IS_HB_GW(unit)      (0)
#define SOC_IS_HBX(unit)        (0)
#define SOC_IS_FBX(unit)        (0)
#endif

#ifdef  BCM_HURRICANE2_SUPPORT
#define SOC_IS_HURRICANE2(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2))
#else
#define SOC_IS_HURRICANE2(unit)    (0)
#endif

#ifdef  BCM_GREYHOUND_SUPPORT
#define SOC_IS_GREYHOUND(unit)  ((!SOC_INFO(unit).spi_device) && \
        (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND))
#else
#define SOC_IS_GREYHOUND(unit)    (0)
#endif

#ifdef  BCM_HURRICANE3_SUPPORT
#define SOC_IS_HURRICANE3(unit)  ((!SOC_INFO(unit).spi_device) && \
        (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HURRICANE3))
#else
#define SOC_IS_HURRICANE3(unit)    (0)
#endif

#ifdef  BCM_GREYHOUND2_SUPPORT
#define SOC_IS_GREYHOUND2(unit)  ((!SOC_INFO(unit).spi_device) && \
        (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND2))
#else
#define SOC_IS_GREYHOUND2(unit)    (0)
#endif

#ifdef  BCM_HELIX_SUPPORT
#define SOC_IS_HELIX1(unit)     ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FELIX))
#define SOC_IS_HELIX15(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HELIX15))
#define SOC_IS_HELIX(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HELIX) || \
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HELIX15)))
#else
#define SOC_IS_HELIX(unit)      (0)
#define SOC_IS_HELIX1(unit)     (0)
#define SOC_IS_HELIX15(unit)    (0)
#endif
#ifdef  BCM_FELIX_SUPPORT
#define SOC_IS_FELIX1(unit)     ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FELIX))
#define SOC_IS_FELIX15(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FELIX15))
#define SOC_IS_FELIX(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FHX) && \
                ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FELIX) || \
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FELIX15)))
#else
#define SOC_IS_FELIX(unit)      (0)
#define SOC_IS_FELIX1(unit)     (0)
#define SOC_IS_FELIX15(unit)    (0)
#endif
#ifdef  BCM_RAPTOR_SUPPORT
#define SOC_IS_RAPTOR(unit)     ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_RAPTOR))
#define SOC_IS_RAVEN(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_RAVEN))
#define SOC_IS_HAWKEYE(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HAWKEYE))
#else
#define SOC_IS_RAPTOR(unit)     (0)
#define SOC_IS_RAVEN(unit)      (0)
#define SOC_IS_HAWKEYE(unit)    (0)
#endif
#if defined(BCM_HELIX_SUPPORT) || defined(BCM_FELIX_SUPPORT)
#define SOC_IS_FX_HX(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FX_HX))
#else
#define SOC_IS_FX_HX(unit)      (0)
#endif

#ifdef  BCM_GOLDWING_SUPPORT
#define SOC_IS_GOLDWING(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_GOLDWING))
#else
#define SOC_IS_GOLDWING(unit)   (0)
#endif

#ifdef  BCM_HUMV_SUPPORT
#define SOC_IS_HUMV(unit)       ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HUMV))
#else
#define SOC_IS_HUMV(unit)       (0)
#endif

#ifdef  BCM_BRADLEY_SUPPORT
#define SOC_IS_BRADLEY(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_BRADLEY))
#else
#define SOC_IS_BRADLEY(unit)    (0)
#endif

#ifdef  BCM_FIREBOLT2_SUPPORT
#define SOC_IS_FIREBOLT2(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_FIREBOLT2))
#else
#define SOC_IS_FIREBOLT2(unit)  (0)
#endif

#ifdef  BCM_TRIUMPH_SUPPORT
#define SOC_IS_TRIUMPH(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIUMPH))
#define SOC_IS_ENDURO(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_ENDURO))
#define SOC_IS_HURRICANE(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE))
#define SOC_IS_HURRICANEX(unit)   ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE) || \
                 (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2) || \
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HURRICANE3)))
#define SOC_IS_TR_VL(unit)      ((!SOC_INFO(unit).spi_device) && \
    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TR_VL) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_APACHE) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HURRICANE3) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TOMAHAWKPLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND2)))
#else
#define SOC_IS_TRIUMPH(unit)    (0)
#define SOC_IS_ENDURO(unit)     (0)
#define SOC_IS_HURRICANE(unit)  (0)
#define SOC_IS_HURRICANEX(unit) (0)
#define SOC_IS_TR_VL(unit)      (0)
#endif

#ifdef  BCM_VALKYRIE_SUPPORT
#define SOC_IS_VALKYRIE(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_VALKYRIE))
#else
#define SOC_IS_VALKYRIE(unit)   (0)
#endif

#ifdef  BCM_SCORPION_SUPPORT
#define SOC_IS_SCORPION(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_SCORPION))
#define SOC_IS_SC_CQ(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_SC_CQ))
#else
#define SOC_IS_SCORPION(unit)   (0)
#define SOC_IS_SC_CQ(unit)      (0)
#endif

#ifdef  BCM_CONQUEROR_SUPPORT
#define SOC_IS_CONQUEROR(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_CONQUEROR))
#else
#define SOC_IS_CONQUEROR(unit)  (0)
#endif

#ifdef  BCM_TRIUMPH2_SUPPORT
#define SOC_IS_TRIUMPH2(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIUMPH2))
#else
#define SOC_IS_TRIUMPH2(unit)   (0)
#endif

#ifdef  BCM_APOLLO_SUPPORT
#define SOC_IS_APOLLO(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_APOLLO))
#else
#define SOC_IS_APOLLO(unit)     (0)
#endif

#ifdef  BCM_VALKYRIE2_SUPPORT
#define SOC_IS_VALKYRIE2(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_VALKYRIE2))
#else
#define SOC_IS_VALKYRIE2(unit)  (0)
#endif

#ifdef  BCM_SIRIUS_SUPPORT
#define SOC_IS_SIRIUS(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_SIRIUS))
#else
#define SOC_IS_SIRIUS(unit)     (0)
#endif

#ifdef  BCM_CALADAN3_SUPPORT
#define SOC_IS_CALADAN3(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_CALADAN3))
#else
#define SOC_IS_CALADAN3(unit)   (0)
#endif

#ifdef  BCM_TRIDENT_SUPPORT
#define SOC_IS_TRIDENT(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIDENT))
#define SOC_IS_TITAN(unit)      ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TITAN))
#define SOC_IS_TD_TT(unit)      ((!SOC_INFO(unit).spi_device) && \
    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TD_TT) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_APACHE) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TOMAHAWKPLUS)))
#else
#define SOC_IS_TRIDENT(unit)    (0)
#define SOC_IS_TITAN(unit)      (0)
#define SOC_IS_TD_TT(unit)      (0)
#endif

#ifdef  BCM_SHADOW_SUPPORT
#define SOC_IS_SHADOW(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_SHADOW))
#else
#define SOC_IS_SHADOW(unit)     (0)
#endif

#ifdef  BCM_TRIUMPH3_SUPPORT
#define SOC_IS_TRIUMPH3(unit)  ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIUMPH3) || \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4)))
#else
#define SOC_IS_TRIUMPH3(unit)   (0)
#endif

#ifdef  BCM_HELIX4_SUPPORT
#define SOC_IS_HELIX4(unit) (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4)
#else
#define SOC_IS_HELIX4(unit)     (0)
#endif

#ifdef  BCM_KATANA_SUPPORT
#define SOC_IS_KATANA(unit)    ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA))
#define SOC_IS_KATANAX(unit)   ((!SOC_INFO(unit).spi_device) && \
                    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA) || \
                     (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2)))
#else
#define SOC_IS_KATANA(unit)     (0)
#define SOC_IS_KATANAX(unit)    (0)
#endif

#ifdef  BCM_KATANA2_SUPPORT
#define SOC_IS_KATANA2(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2))
#else
#define SOC_IS_KATANA2(unit)    (0)
#endif

#ifdef  BCM_SABER2_SUPPORT
#define SOC_IS_SABER2(unit)  ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_SABER2) || \
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_METROLITE)))
#else
#define SOC_IS_SABER2(unit)    (0)
#endif

#ifdef  BCM_METROLITE_SUPPORT
#define SOC_IS_METROLITE(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_METROLITE))
#define SOC_PORT_PP_BITMAP(unit, port)   (SOC_INFO(unit).pp_port_bitmap[port])
#else
#define SOC_IS_METROLITE(unit)    (0)
#endif

#ifdef  BCM_TRIDENT2_SUPPORT
#define SOC_IS_TRIDENT2(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIDENT2))
#define SOC_IS_TITAN2(unit)     ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TITAN2))
#define SOC_IS_TD2_TT2(unit)    ((!SOC_INFO(unit).spi_device) && \
    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TD2_TT2) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_APACHE) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TOMAHAWKPLUS)))
#define SOC_IS_TRIDENT2X(unit)  ((!SOC_INFO(unit).spi_device) && \
            ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIDENT2X) || \
             (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_APACHE)))
#define SOC_IS_TITAN2X(unit)    ((!SOC_INFO(unit).spi_device) && \
        ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TITAN2) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS)))
#else
#define SOC_IS_TRIDENT2(unit)   (0)
#define SOC_IS_TITAN2(unit)     (0)
#define SOC_IS_TD2_TT2(unit)    (0)
#define SOC_IS_TRIDENT2X(unit)  (0)
#define SOC_IS_TITAN2X(unit)    (0)
#endif

#ifdef BCM_TRIDENT2PLUS_SUPPORT
#define SOC_IS_TRIDENT2PLUS(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIDENT2PLUS))
#define SOC_IS_TITAN2PLUS(unit)     ((!SOC_INFO(unit).spi_device) && \
        (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS))
#define SOC_IS_TD2P_TT2P(unit)      ((!SOC_INFO(unit).spi_device) && \
        ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TRIDENT2PLUS) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS)))
#else
#define SOC_IS_TRIDENT2PLUS(unit)   (0)
#define SOC_IS_TITAN2PLUS(unit)     (0)
#define SOC_IS_TD2P_TT2P(unit)      (0)
#endif
#ifdef  BCM_TOMAHAWK_SUPPORT
#define SOC_IS_TOMAHAWK(unit)   ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TOMAHAWK) || \
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TOMAHAWKPLUS)))
#define SOC_IS_TOMAHAWKX(unit)   ((!SOC_INFO(unit).spi_device) && \
                ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TOMAHAWK) || \
                 (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TOMAHAWKPLUS)))
#else
#define SOC_IS_TOMAHAWK(unit)   (0)
#define SOC_IS_TOMAHAWKX(unit)  (0)
#endif

#ifdef  BCM_TOMAHAWKPLUS_SUPPORT
#define SOC_IS_TOMAHAWKPLUS(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TOMAHAWKPLUS))
#else
#define SOC_IS_TOMAHAWKPLUS(unit)   (0)
#endif

#ifdef BCM_APACHE_SUPPORT
#define SOC_IS_APACHE(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_APACHE))
#else
#define SOC_IS_APACHE(unit)    (0)
#endif

#ifdef BCM_TRX_SUPPORT
#define SOC_IS_TRX(unit)    ((!SOC_INFO(unit).spi_device) && \
    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_TRX) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2)||\
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_APACHE) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HURRICANE3) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TOMAHAWKPLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND2)))
#else
#define SOC_IS_TRX(unit)        (0)
#endif

#ifdef  BCM_XGS_SUPPORT
#define SOC_IS_XGS(unit)    ((!SOC_INFO(unit).spi_device) && \
    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2) ||\
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_APACHE) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HURRICANE3) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND2)))
#else
#define SOC_IS_XGS(unit)        (0)
#endif
#ifdef  BCM_XGS_FABRIC_SUPPORT
#define SOC_IS_XGS_FABRIC(unit) ((!SOC_INFO(unit).spi_device) && \
        ((SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS_FABRIC) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS)))
#define SOC_IS_XGS_FABRIC_TITAN(unit) ((!SOC_INFO(unit).spi_device) && \
        ((SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS_FABRIC) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITANHAWK)))
#else
#define SOC_IS_XGS_FABRIC(unit) (0)
#define SOC_IS_XGS_FABRIC_TITAN(unit) (0)
#endif
#ifdef  BCM_XGS_SWITCH_SUPPORT
#define SOC_IS_XGS_SWITCH(unit) ((!SOC_INFO(unit).spi_device) && \
    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS_SWITCH) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2)||\
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_APACHE) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HURRICANE3) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND2)))
#else
#define SOC_IS_XGS_SWITCH(unit) (0)
#endif
#ifdef  BCM_XGS12_FABRIC_SUPPORT
#define SOC_IS_XGS12_FABRIC(unit)   ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS12_FABRIC))
#else
#define SOC_IS_XGS12_FABRIC(unit) (0)
#endif
#ifdef  BCM_XGS3_SWITCH_SUPPORT
#define SOC_IS_XGS3_SWITCH(unit)    ((!SOC_INFO(unit).spi_device) && \
    ((SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS3_SWITCH) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_KATANA2) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HELIX4) || \
     (SOC_INFO(unit).chip & SOC_INFO_CHIP_HURRICANE2)|| \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_APACHE) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_HURRICANE3) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TOMAHAWKPLUS) || \
     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_GREYHOUND2)))
#else
#define SOC_IS_XGS3_SWITCH(unit) (0)
#endif
#ifdef  BCM_XGS3_FABRIC_SUPPORT
#define SOC_IS_XGS3_FABRIC(unit)  ((!SOC_INFO(unit).spi_device) && \
        ((SOC_INFO(unit).chip & SOC_INFO_CHIP_XGS3_FABRIC) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TITAN2PLUS)))

#else
#define SOC_IS_XGS3_FABRIC(unit) (0)
#endif
#ifdef  BCM_ROBO_SUPPORT
#define SOC_IS_ROBO(unit)        ((SOC_INFO(unit).spi_device) && \
                SOC_IS_ROBO_ARCH(unit))

#define SOC_IS_ROBO5325(unit) (0)

#else
#define SOC_IS_ROBO(unit)        (0)
#define SOC_IS_ROBO_GE_SWITCH(unit) (0)
#endif

#ifdef  BCM_EA_SUPPORT
#ifdef  BCM_TK371X_SUPPORT
#define SOC_IS_TK371X(unit)  ((!SOC_INFO(unit).spi_device) && \
                (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_TK371X))
#else
#define SOC_IS_TK371X(unit) (0)
#endif /* BCM_TK371X_SUPPORT */
#define SOC_IS_EA(unit) (SOC_IS_TK371X(unit))
#else
#define SOC_IS_EA(unit)      (0)
#define SOC_IS_TK371X(unit)  (0)
#endif  /* BCM_EA_SUPPORT */

#ifdef BCM_ARAD_SUPPORT
#define SOC_IS_ARDON(unit)        (SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_ARDON)
#define SOC_IS_ARADPLUS(unit)     ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_ARADPLUS) || (SOC_IS_ARDON(unit)) || (SOC_IS_JERICHO(unit)))
#define SOC_IS_ARAD(unit)         ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_ARAD) || (SOC_IS_ARADPLUS(unit))) /* SOC_IS_ARADPLUS already
                                                                                                                includes all chips above it. */
#ifdef VENDOR_CUSTOMER52
int soc_is_cisco(int unit);
#define SOC_IS_DPP_CISCO_TYPE(unit) ((SOC_IS_ARAD(unit) && \
                                   ((SOC_INFO(unit).driver_type == SOC_CHIP_BCM88652_A0) || \
                                    (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88652_B0) || \
                                    (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88652_B1))) || \
                                    soc_is_cisco(unit))

#define SOC_IS_ARAD_A0(unit)           (SOC_IS_ARAD(unit)     && ((SOC_INFO(unit).driver_type == SOC_CHIP_BCM88650_A0) || (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88652_A0)))
#define SOC_IS_ARAD_B0(unit)           (SOC_IS_ARAD(unit)     && ((SOC_INFO(unit).driver_type == SOC_CHIP_BCM88650_B0) || (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88652_B0)))
#define SOC_IS_ARAD_B1(unit)           (SOC_IS_ARAD(unit)     && ((SOC_INFO(unit).driver_type == SOC_CHIP_BCM88650_B1) || (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88652_B1)))
#define SOC_IS_ARADPLUS_A0(unit)       (SOC_IS_ARADPLUS(unit) &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88660_A0))
#define SOC_IS_ARDON_A0(unit)          (SOC_IS_ARDON(unit)    &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88202_A0))
#else   /*VENDOR_CUSTOMER52*/
#define SOC_IS_ARAD_A0(unit)           (SOC_IS_ARAD(unit)     &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88650_A0))
#define SOC_IS_ARAD_B0(unit)           (SOC_IS_ARAD(unit)     &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88650_B0))
#define SOC_IS_ARAD_B1(unit)           (SOC_IS_ARAD(unit)     &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88650_B1))
#define SOC_IS_ARADPLUS_A0(unit)       (SOC_IS_ARADPLUS(unit) &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88660_A0))
#define SOC_IS_ARDON_A0(unit)          (SOC_IS_ARDON(unit)    &&  (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88202_A0))
#endif  /*VENDOR_CUSTOMER52*/
#define SOC_IS_ARAD_B0_AND_ABOVE(unit)  (SOC_IS_ARAD(unit)     && (!SOC_IS_ARAD_A0(unit)))
#define SOC_IS_ARAD_B1_AND_BELOW(unit)  (SOC_IS_ARAD(unit)     && (!SOC_IS_ARADPLUS(unit)))
#define SOC_IS_ARADPLUS_AND_BELOW(unit) (SOC_IS_ARAD(unit)     && (!SOC_IS_JERICHO(unit)))
#define SOC_IS_ACP(unit)        (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_ACP)
#else
#define SOC_IS_ARAD(unit)               (0)
#define SOC_IS_ARADPLUS(unit)           (0)
#define SOC_IS_ARDON(unit)              (0)
#define SOC_IS_ARAD_A0(unit)            (0)
#define SOC_IS_ARAD_B0(unit)            (0)
#define SOC_IS_ARAD_B1(unit)            (0)
#define SOC_IS_ARADPLUS_A0(unit)        (0)
#define SOC_IS_ARAD_B0_AND_ABOVE(unit)  (0)
#define SOC_IS_ARAD_B1_AND_BELOW(unit)  (0)
#define SOC_IS_ARADPLUS_AND_BELOW(unit) (0)
#define SOC_IS_ACP(unit)                (0)
#endif

#ifdef BCM_JERICHO_SUPPORT
#define SOC_IS_QMX(unit)          (SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_QMX)
#define SOC_IS_JERICHO(unit)      ((SOC_INFO(unit).chip_type  == SOC_INFO_CHIP_TYPE_JERICHO) || (SOC_IS_QMX(unit)) || (SOC_IS_QMX_B0(unit))  || (SOC_IS_QAX(unit))  || (SOC_IS_JERICHO_A0(unit))  || (SOC_IS_JERICHO_B0(unit)) || (SOC_IS_JERICHO_PLUS(unit)) || (SOC_IS_FLAIR(unit)))
#define SOC_IS_QMX_A0(unit)       (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88375_A0)
#define SOC_IS_QMX_B0(unit)       (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88375_B0)
#define SOC_IS_JERICHO_A0(unit)   (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88675_A0)
#define SOC_IS_JERICHO_B0(unit)       (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88675_B0)
#define SOC_IS_JERICHO_AND_BELOW(unit)  (SOC_IS_ARAD(unit) && !SOC_IS_QAX(unit) && !SOC_IS_JERICHO_PLUS(unit))
#define SOC_IS_JERICHO_B0_AND_ABOVE(unit) (SOC_IS_JERICHO_B0(unit) || SOC_IS_QMX_B0(unit) || SOC_IS_JERICHO_PLUS(unit) || SOC_IS_QAX(unit))

#else
#define SOC_IS_QMX(unit)          (0)
#define SOC_IS_JERICHO(unit)      (0)
#define SOC_IS_QMX_A0(unit)       (0)
#define SOC_IS_QMX_B0(unit)       (0)
#define SOC_IS_JERICHO_A0(unit)   (0)
#define SOC_IS_JERICHO_B0(unit)       (0)
#define SOC_IS_JERICHO_AND_BELOW(unit)  (SOC_IS_ARAD(unit) && !SOC_IS_QAX(unit) && !SOC_IS_JERICHO_PLUS(unit))
#define SOC_IS_JERICHO_B0_AND_ABOVE(unit) (0)
#endif

#ifdef BCM_QAX_SUPPORT
#define SOC_IS_QAX(unit)          ((SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_QAX) || (SOC_IS_KALIA(unit)) ||  SOC_IS_QUX(unit))
#define SOC_IS_QAX_A0(unit)       (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88470_A0)
#define SOC_IS_KALIA(unit)        (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_KALIA)
#define SOC_IS_QAX_WITH_FABRIC_ENABLED(unit) (SOC_IS_KALIA(unit) || (SOC_IS_QAX(unit) && SOC_DPP_IS_MESH(unit)))
#else
#define SOC_IS_QAX(unit)          (0)
#define SOC_IS_QAX_A0(unit)       (0)
#define SOC_IS_KALIA(unit)          (0)
#define SOC_IS_QAX_WITH_FABRIC_ENABLED(unit)    (0)
#endif


#ifdef BCM_QUX_SUPPORT
#define SOC_IS_QUX(unit)          ((SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_QUX))
#define SOC_IS_QUX_A0(unit)       (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88270_A0)
#else
#define SOC_IS_QUX(unit)          (0)
#define SOC_IS_QUX_A0(unit)       (0)
#endif

#ifdef BCM_FLAIR_SUPPORT
#define SOC_IS_FLAIR(unit)          ((SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_FLAIR))
#define SOC_IS_FLAIR_A0(unit)       (SOC_INFO(unit).driver_type == SOC_CHIP_BCM8206_A0)
#else
#define SOC_IS_FLAIR(unit)          (0)
#define SOC_IS_FLAIR_A0(unit)       (0)
#endif

#ifdef BCM_JERICHO_PLUS_SUPPORT
#define SOC_IS_JERICHO_PLUS(unit)          ((SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_JERICHO_PLUS) || (SOC_IS_QAX(unit)))
#define SOC_IS_JERICHO_PLUS_A0(unit)       (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88680_A0)
#define SOC_IS_JERICHO_PLUS_AND_BELOW(unit)  (SOC_IS_ARAD(unit) && !SOC_IS_QAX(unit))
#define SOC_IS_JERICHO_PLUS_ONLY(unit)       (SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_JERICHO_PLUS)
#else
#define SOC_IS_JERICHO_PLUS(unit)          (0)
#define SOC_IS_JERICHO_PLUS_A0(unit)       (0)
#define SOC_IS_JERICHO_PLUS_AND_BELOW(unit) (SOC_IS_ARAD(unit) && !SOC_IS_QAX(unit))
#define SOC_IS_JERICHO_PLUS_ONLY(unit)  (0)
#endif

#ifdef BCM_DNX_SUPPORT
#define SOC_IS_DNX(unit)                (SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_JERICHO_2)
#define SOC_IS_JERICHO_2_A0(unit)       (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88690_A0)
#else
#define SOC_IS_DNX(unit)                (0)
#define SOC_IS_JERICHO_2_A0(unit)       (0)
#endif

#if defined(DNX_TEST_BOARD) && (defined(BCM_83207_A0) || defined(BCM_83208_A0))
#define DNX_TEST_CHIPS_SUPPORT
#define SOC_IS_SAMAR(unit)     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_SAMAR)
#define SOC_IS_SINAI(unit)     (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_SINAI)
#define SOC_IS_DNX_TEST_DEVICE(unit) (SOC_IS_SAMAR(unit) || SOC_IS_SINAI(unit))
#else
#define SOC_IS_SAMAR(unit)     0
#define SOC_IS_SINAI(unit)     0
#define SOC_IS_DNX_TEST_DEVICE(unit) 0
#endif


#ifdef BCM_DFE_SUPPORT
#define SOC_IS_FE1600(unit) (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FE1600)
#define SOC_IS_FE1600_REDUCED(unit) (CMDEV(unit).dev.dev_id == BCM88755_DEVICE_ID)
#define SOC_IS_BCM88754_A0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88754_A0)
#define SOC_IS_BCM88753(unit) (CMDEV(unit).dev.dev_id == BCM88753_DEVICE_ID)
#define SOC_IS_BCM88753_B0(unit) (CMDEV(unit).dev.dev_id == BCM88753_DEVICE_ID && CMDEV(unit).dev.rev_id == BCM88750_B0_REV_ID)
#define SOC_IS_BCM88773(unit) (CMDEV(unit).dev.dev_id == BCM88773_DEVICE_ID || CMDEV(unit).dev.dev_id == BCM88953_DEVICE_ID)
#define SOC_IS_BCM88773_A1(unit) ((CMDEV(unit).dev.dev_id == BCM88773_DEVICE_ID || CMDEV(unit).dev.dev_id == BCM88953_DEVICE_ID) && CMDEV(unit).dev.rev_id == BCM88773_A1_REV_ID)
#define SOC_IS_BCM88774(unit) (CMDEV(unit).dev.dev_id == BCM88774_DEVICE_ID || CMDEV(unit).dev.dev_id == BCM88954_DEVICE_ID)
#define SOC_IS_BCM88774_A1(unit) ((CMDEV(unit).dev.dev_id == BCM88774_DEVICE_ID || CMDEV(unit).dev.dev_id == BCM88954_DEVICE_ID) && CMDEV(unit).dev.rev_id == BCM88774_A1_REV_ID)
#define SOC_IS_BCM88775(unit) (CMDEV(unit).dev.dev_id == BCM88775_DEVICE_ID || CMDEV(unit).dev.dev_id == BCM88955_DEVICE_ID)
#define SOC_IS_BCM88775_A1(unit) ((CMDEV(unit).dev.dev_id == BCM88775_DEVICE_ID || CMDEV(unit).dev.dev_id == BCM88955_DEVICE_ID) && CMDEV(unit).dev.rev_id == BCM88775_A1_REV_ID)
#define SOC_IS_BCM88776(unit) (CMDEV(unit).dev.dev_id == BCM88776_DEVICE_ID || CMDEV(unit).dev.dev_id == BCM88956_DEVICE_ID)
#define SOC_IS_BCM88776_A1(unit) ((CMDEV(unit).dev.dev_id == BCM88776_DEVICE_ID || CMDEV(unit).dev.dev_id == BCM88956_DEVICE_ID) && CMDEV(unit).dev.rev_id == BCM88776_A1_REV_ID)
#define SOC_IS_FE3200(unit) (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FE3200)
#define SOC_IS_FE3200_A0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88950_A0)
#ifdef VENDOR_CUSTOMER52
#define SOC_IS_FE1600_A0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88750_A0 || SOC_INFO(unit).driver_type == SOC_CHIP_BCM88752_A0)
#define SOC_IS_FE1600_B0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88750_B0 || SOC_INFO(unit).driver_type == SOC_CHIP_BCM88754_A0 || SOC_INFO(unit).driver_type == SOC_CHIP_BCM88752_B0)
#define SOC_IS_DFE_TYPE(dev_type) ((dev_type) == BCM88750_DEVICE_ID || (dev_type) == BCM88753_DEVICE_ID || (dev_type) == BCM88752_DEVICE_ID || (dev_type) == BCM88754_DEVICE_ID || (dev_type) == BCM88755_DEVICE_ID || (dev_type) == BCM88770_DEVICE_ID || (dev_type) == BCM88773_DEVICE_ID || (dev_type) == BCM88774_DEVICE_ID || (dev_type) == BCM88775_DEVICE_ID || (dev_type) == BCM88776_DEVICE_ID || (dev_type) == BCM88950_DEVICE_ID || (dev_type) == BCM88772_DEVICE_ID || (dev_type) == BCM88952_DEVICE_ID || (dev_type) == BCM88953_DEVICE_ID || (dev_type) == BCM88954_DEVICE_ID || (dev_type) == BCM88955_DEVICE_ID || (dev_type) == BCM88956_DEVICE_ID)
#define SOC_IS_DFE_CUSTOMER52(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88750_A0 || SOC_INFO(unit).driver_type == SOC_CHIP_BCM88750_B0 || SOC_INFO(unit).driver_type == SOC_CHIP_BCM88752_A0 || SOC_INFO(unit).driver_type == SOC_CHIP_BCM88752_B0 || CMDEV(unit).dev.dev_id == BCM88772_DEVICE_ID || CMDEV(unit).dev.dev_id == BCM88952_DEVICE_ID )
#else
#define SOC_IS_FE1600_A0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88750_A0|| SOC_INFO(unit).driver_type == SOC_CHIP_BCM88752_A0)
#define SOC_IS_FE1600_B0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88750_B0 || SOC_INFO(unit).driver_type == SOC_CHIP_BCM88754_A0|| SOC_INFO(unit).driver_type == SOC_CHIP_BCM88752_B0)
#define SOC_IS_DFE_TYPE(dev_type) ( (dev_type) == BCM88772_DEVICE_ID || (dev_type) == BCM88752_DEVICE_ID || (dev_type) == BCM88952_DEVICE_ID || (dev_type) == BCM88750_DEVICE_ID || (dev_type) == BCM88753_DEVICE_ID || (dev_type) == BCM88754_DEVICE_ID  || (dev_type) == BCM88755_DEVICE_ID || (dev_type) == BCM88770_DEVICE_ID || (dev_type) == BCM88773_DEVICE_ID || (dev_type) == BCM88774_DEVICE_ID || (dev_type) == BCM88775_DEVICE_ID || (dev_type) == BCM88776_DEVICE_ID || (dev_type) == BCM88950_DEVICE_ID || (dev_type) == BCM88953_DEVICE_ID || (dev_type) == BCM88954_DEVICE_ID || (dev_type) == BCM88955_DEVICE_ID || (dev_type) == BCM88956_DEVICE_ID)
#endif

#define SOC_IS_FE1600_B0_AND_ABOVE(unit) (SOC_IS_FE1600(unit) && !SOC_IS_FE1600_A0(unit))
#else /*!BCM_DFE_SUPPORT*/
#define SOC_IS_FE1600_A0(unit)          (0)
#define SOC_IS_FE1600_B0(unit)          (0)
#define SOC_IS_FE1600_B0_AND_ABOVE(unit)(0)
#define SOC_IS_FE1600(unit)             (0)
#define SOC_IS_FE3200(unit)             (0)
#define SOC_IS_FE3200_A0(unit)          (0)
#define SOC_IS_DFE_TYPE(dev_type)       (0)
#define SOC_IS_FE1600_REDUCED(unit)     (0)
#define SOC_IS_BCM88754_A0(unit) 		(0)
#ifdef VENDOR_CUSTOMER52
#define SOC_IS_DFE_CUSTOMER52(unit) (0)
#endif
#endif /*BCM_DFE_SUPPORT*/

#ifdef BCM_DNXF_SUPPORT
#define SOC_IS_DNXF(unit) (SOC_INFO(unit).chip_type   == SOC_INFO_CHIP_TYPE_RAMON)
#define SOC_IS_DNXF_TYPE(dev_type) ((dev_type) == BCM88790_DEVICE_ID)
#define SOC_IS_RAMON(unit) (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_RAMON)
#define SOC_IS_RAMON_A0(unit) (SOC_INFO(unit).driver_type == SOC_CHIP_BCM88790_A0)
#else
#define SOC_IS_DNXF(unit)           (0)
#define SOC_IS_DNXF_TYPE(dev_type)  (0)
#define SOC_IS_RAMON(unit)          (0)
#define SOC_IS_RAMON_A0(unit)       (0)
#endif /*BCM_DNXF_SUPPORT*/

#define SOC_IS_DPP(unit) (SOC_IS_ARAD(unit) || SOC_IS_ACP(unit) || SOC_IS_ARDON(unit) || SOC_IS_QAX(unit)  || SOC_IS_JERICHO_PLUS(unit) || SOC_IS_DNX_TEST_DEVICE(unit))
#define SOC_IS_DFE(unit) (SOC_IS_FE1600(unit) || SOC_IS_FE3200(unit))
#define SOC_IS_SAND(unit) (SOC_IS_DPP(unit) || SOC_IS_DFE(unit) || SOC_IS_DNX(unit) || SOC_IS_DNXF(unit))

#define SOC_IS_DPP_DRC_COMBO28(unit) (SOC_IS_ARDON(unit) || SOC_IS_JERICHO(unit) || SOC_IS_DNX_TEST_DEVICE(unit))

#ifdef BCM_ARAD_SUPPORT
#define SOC_BLOCK_BY_DEVICE(unit, core) ((SOC_DPP_DEFS_GET(unit, nof_cores) > 1) ? core : SOC_BLOCK_ALL)
#else /* BCM_ARAD_SUPPORT */
#define SOC_BLOCK_BY_DEVICE(unit, core) 0
#endif
/*
 * Note: ESW and ROBO devices need similar SOC_IS definitions
 *
 * SOC_IS_* macros for SBX devices
 */
typedef enum {
    SOC_INFO_CHIP_TYPE_HERCULES15 = 1, /* only use non-zero values to be able to skip further compares */
    SOC_INFO_CHIP_TYPE_FIREBOLT,
    SOC_INFO_CHIP_TYPE_FELIX,
    SOC_INFO_CHIP_TYPE_HELIX,       /* 5630x */
    SOC_INFO_CHIP_TYPE_GOLDWING,
    SOC_INFO_CHIP_TYPE_HUMV,
    SOC_INFO_CHIP_TYPE_BRADLEY,
    SOC_INFO_CHIP_TYPE_RAPTOR,
    SOC_INFO_CHIP_TYPE_FELIX15,     /* 5611x */
    SOC_INFO_CHIP_TYPE_HELIX15,     /* 5631x */
    SOC_INFO_CHIP_TYPE_FIREBOLT2,
    SOC_INFO_CHIP_TYPE_TRIUMPH,
    SOC_INFO_CHIP_TYPE_SHADOW,
    SOC_INFO_CHIP_TYPE_TRIUMPH3,
    SOC_INFO_CHIP_TYPE_HELIX4,
    SOC_INFO_CHIP_TYPE_SABER2,
    SOC_INFO_CHIP_TYPE_METROLITE,
    SOC_INFO_CHIP_TYPE_RAVEN,
    SOC_INFO_CHIP_TYPE_GREYHOUND,
    SOC_INFO_CHIP_TYPE_QE2000,
    SOC_INFO_CHIP_TYPE_BME3200,
    SOC_INFO_CHIP_TYPE_FE2000,
    SOC_INFO_CHIP_TYPE_FE2000XT,
    SOC_INFO_CHIP_TYPE_BM9600,
    SOC_INFO_CHIP_TYPE_CALADAN3,
    SOC_INFO_CHIP_TYPE_ARAD,
    SOC_INFO_CHIP_TYPE_ARADPLUS,
    SOC_INFO_CHIP_TYPE_JERICHO,
    SOC_INFO_CHIP_TYPE_QMX,
    SOC_INFO_CHIP_TYPE_QAX,
    SOC_INFO_CHIP_TYPE_KALIA,
    SOC_INFO_CHIP_TYPE_QUX,
    SOC_INFO_CHIP_TYPE_FLAIR,
    SOC_INFO_CHIP_TYPE_JERICHO_PLUS,
    SOC_INFO_CHIP_TYPE_JERICHO_2,
    SOC_INFO_CHIP_TYPE_ARDON,
    /* JERICHO-2-P3 */
    SOC_INFO_CHIP_TYPE_JERICHO_P3,
    SOC_INFO_CHIP_TYPE_FE1600,
    SOC_INFO_CHIP_TYPE_FE3200,
    SOC_INFO_CHIP_TYPE_TK371X,
    SOC_INFO_CHIP_TYPE_TKDUMMY,
    SOC_INFO_CHIP_TYPE_SIRIUS,
    SOC_INFO_CHIP_TYPE_SCORPION,
    SOC_INFO_CHIP_TYPE_ACP,
    SOC_INFO_CHIP_TYPE_APACHE,
    SOC_INFO_CHIP_TYPE_HURRICANE3,
    SOC_INFO_CHIP_TYPE_GREYHOUND2,
    /* TRIDENT2PLUS variants */
    SOC_INFO_CHIP_TYPE_TITAN2PLUS,
    /* TITANHAWK variants */
    SOC_INFO_CHIP_TYPE_TITANHAWK,
    SOC_INFO_CHIP_TYPE_TOMAHAWKPLUS
} soc_chip_e;

#ifdef BCM_SBX_SUPPORT
#define SBX_CHIPS(unit) \
        ((SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FE2000) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_FE2000XT) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_QE2000) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_BME3200) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_BM9600) || \
         (SOC_INFO(unit).chip_type == SOC_INFO_CHIP_TYPE_CALADAN3))
#define SOC_IS_ESW(unit) (!SOC_IS_ROBO(unit) && !SBX_CHIPS(unit) && \
                          !SOC_IS_SIRIUS(unit) && !SOC_IS_SAND(unit))
#else
#define SOC_IS_ESW(unit) (!SOC_IS_ROBO(unit) && !SOC_IS_SIRIUS(unit) &&\
                          !SOC_IS_CALADAN3(unit) && !SOC_IS_SAND(unit))
#endif

/* Some devices need a register updated before starting a DMA TX operation */
#if     defined(BCM_DRACO1_SUPPORT) || \
        defined(BCM_LYNX_SUPPORT) || \
        defined(BCM_TUCANA_SUPPORT)
#define SOC_TX_PKT_PROPERTIES
#endif


#define SOC_UNIT_NUM_VALID(unit)        (((unit) >= 0) &&                  \
                                         ((unit) < SOC_MAX_NUM_DEVICES))

#define SOC_UNIT_VALID(unit)            (SOC_UNIT_NUM_VALID(unit) && \
                                         (SOC_CONTROL(unit) != NULL))

/* Indicates port lies in module 1.  Currently applies only to Tucana */
#ifdef  BCM_TUCANA_SUPPORT
#define SOC_PORT_MOD1(unit, port)       ((((port) & 0x20) != 0) && \
                                        (soc_feature(unit, soc_feature_mod1)))

/*
 * This macro returns the memory table (soc_mem_t) associated with
 * the given unit/port for Tucana.  It returns a memory valid for
 * other chips as well.
 */
#define SOC_PORT_MEM_TAB(unit, port)                            \
    (SOC_PORT_MOD1(unit, port) ? PORT_TAB1m : PORT_TABm)


/*
 * Macro:
 *      SOC_PORT_MOD_OFFSET
 * Purpose:
 *      Maps a port to its offset in its module on Tucana.
 *      (Lower 5 bits: port % 32)
 *      Example:   7 -> 7,   32 -> 0,  35 -> 3.
 */
#define SOC_PORT_MOD_OFFSET(unit, port) ((soc_feature(unit, soc_feature_mod1) ? \
                                         (port & 0x1f) : port))

/*
 * Read/write macros for accessing proper port table w/ proper index
 */
#define READ_PORT_TAB_MOD_CHK(unit, block, port, val_p)          \
    (SOC_PORT_MOD1(unit, port) ?                                 \
        READ_PORT_TAB1m(unit, block, (port) & 0x1f, val_p) :     \
        READ_PORT_TABm(unit, block, port, val_p))

#define WRITE_PORT_TAB_MOD_CHK(unit, block, port, val_p)         \
    (SOC_PORT_MOD1(unit, port) ?                                 \
        WRITE_PORT_TAB1m(unit, block, (port) & 0x1f, val_p) :    \
        WRITE_PORT_TABm(unit, block, port, val_p))

/* Forces port to lie in module 1.  Currently applies only to Tucana */
#define SOC_PORT_IN_MOD1(unit, port)    ((port) | 0x20)

#else /* BCM_TUCANA_SUPPORT */
#define SOC_PORT_MOD1(unit, port)       (0)
#define SOC_PORT_MEM_TAB(unit, port)    (PORT_TABm)
#define SOC_PORT_MOD_OFFSET(unit, port) (port)
#define READ_PORT_TAB_MOD_CHK(unit, block, port, val_p)          \
            (READ_PORT_TABm(unit, block, port, val_p))

#define WRITE_PORT_TAB_MOD_CHK(unit, block, port, val_p)         \
            (WRITE_PORT_TABm(unit, block, port, val_p))
#define SOC_PORT_IN_MOD1(unit, port)    (port)

#endif /* BCM_TUCANA_SUPPORT */

#if defined(BCM_TOMAHAWK_SUPPORT) && (226 > SOC_MAX_NUM_PORTS)
#define SOC_MAX_NUM_MMU_PORTS  226
#else
#define SOC_MAX_NUM_MMU_PORTS SOC_MAX_NUM_PORTS
#endif

#if defined(BCM_TOMAHAWK_SUPPORT)
#define _SOC_UPDATE_PORT_TYPE_INFO(ptype, port_num) \
    si->ptype.num = 0; \
    si->ptype.min = si->ptype.max = -1; \
    PBMP_ITER(si->ptype.bitmap, port_num) { \
        si->ptype.port[si->ptype.num++] = port_num; \
        if (si->ptype.min < 0) { \
            si->ptype.min = port_num; \
        } \
        if (it_port > si->ptype.max) { \
            si->ptype.max = port_num; \
        } \
    }
#endif

#ifdef BCM_ROBO_SUPPORT
#if defined(BCM_EA_SUPPORT) && defined(BCM_TK371X_SUPPORT)
#define SOC_PORT_VALID_RANGE(unit,port)      ((port) >= 0 && \
                                         (port) < (SOC_IS_ROBO(unit) ? \
                                             SOC_ROBO_MAX_NUM_PORTS : \
                            SOC_IS_EA(unit)?SOC_EA_MAX_NUM_PORTS:SOC_MAX_NUM_PORTS) )

#else
#define SOC_PORT_VALID_RANGE(unit,port) \
                            ((port) >= 0 && \
                            (port) < (SOC_IS_ROBO(unit) ? \
                                SOC_ROBO_MAX_NUM_PORTS : \
                                ((SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS) ? \
                                    SOC_MAX_NUM_PP_PORTS : SOC_MAX_NUM_PORTS)))
#endif
#else /* !BCM_ROBO_SUPPORT */
#define SOC_PORT_VALID_RANGE(unit,port) \
             ((port) >= 0 && \
             ((port) < ((SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS) ? \
                 SOC_MAX_NUM_PP_PORTS : SOC_MAX_NUM_PORTS)))
#endif /* BCM_ROBO_SUPPORT */

/* Holes in the port list shall be detected by port type */

#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
#define SOC_PP_PORT_VALID(unit, port) \
    ((soc_feature(unit, soc_feature_linkphy_coe) && \
        SOC_PBMP_MEMBER(SOC_INFO(unit).linkphy_pp_port_pbm, port)) || \
        (soc_feature(unit, soc_feature_subtag_coe) && \
        SOC_PBMP_MEMBER(SOC_INFO(unit).subtag_pp_port_pbm, port)))

#define SOC_PORT_VALID(unit,port)   \
    (SOC_PORT_VALID_RANGE(unit, port) && \
        ((SOC_PORT_TYPE(unit, port) != 0) || \
        SOC_PP_PORT_VALID(unit, port)))
#else
#define SOC_PP_PORT_VALID(unit, port)    (0)
#define SOC_PORT_VALID(unit,port)   ((SOC_PORT_VALID_RANGE(unit, port)) && \
                                     (SOC_PORT_TYPE(unit, port) != 0))

#endif
#define SOC_PORT_CLASS_VALID(unit, port_class) ((port_class) >= 0 && \
                                 (port_class) < SOC_INFO(unit).port_class_max)


#define SOC_PORT_NAME(unit,port)        (SOC_INFO(unit).port_name[port])
#define SOC_PORT_NAME_ALTER(unit,port)  (SOC_INFO(unit).port_name_alter[port])
#define SOC_PORT_NAME_ALTER_VALID(unit,port)  (SOC_INFO(unit).port_name_alter_valid[port])
#define SOC_PORT_OFFSET(unit,port)      (SOC_INFO(unit).port_offset[port])
#define SOC_PORT_TYPE(unit,port)        (SOC_INFO(unit).port_type[port])
#define SOC_PORT_SPEED_MAX_SET(unit,port,speed)      \
    if (SOC_PORT_VALID_RANGE(unit,port)) {           \
        SOC_INFO(unit).port_speed_max[port] = speed; \
    }
#define SOC_PORT_GROUP_SET(unit,port,group)      \
    if (SOC_PORT_VALID_RANGE(unit,port)) {       \
        SOC_INFO(unit).port_group[port] = group; \
    }
#define SOC_PORT_SERDES_SET(unit,port,serdes)      \
    if (SOC_PORT_VALID_RANGE(unit,port)) {       \
        SOC_INFO(unit).port_serdes[port] = serdes; \
    }
#define SOC_PORT_NUM_LANES_SET(unit,port,num)      \
    if (SOC_PORT_VALID_RANGE(unit,port)) {         \
        SOC_INFO(unit).port_num_lanes[port] = num; \
    }
#define SOC_PORT_P2L_MAPPING_SET(unit,phy_port,port)      \
    if (SOC_PORT_VALID_RANGE(unit,phy_port)) {            \
        SOC_INFO(unit).port_p2l_mapping[phy_port] = port; \
    }
#define SOC_PORT_L2P_MAPPING_SET(unit,port,phy_port)      \
    if (SOC_PORT_VALID_RANGE(unit,port)) {                \
        SOC_INFO(unit).port_l2p_mapping[port] = phy_port; \
    }
#define SOC_PORT_P2M_MAPPING_SET(unit,phy_port,mmu_port)      \
    if (SOC_PORT_VALID_RANGE(unit,phy_port)) {                \
        SOC_INFO(unit).port_p2m_mapping[phy_port] = mmu_port; \
    }
#define SOC_PORT_M2P_MAPPING_SET(unit,mmu_port,phy_port)      \
    if ((mmu_port) >= 0 && (mmu_port) < SOC_MAX_NUM_MMU_PORTS) { \
        SOC_INFO(unit).port_m2p_mapping[mmu_port] = phy_port; \
    }
/* NOTE: The param 'port' is physical port in the following */
#define SOC_PORT_IDX_BLOCK(unit, port, idx) \
    (SOC_PORT_IDX_INFO(unit, port, idx).blk)
#define SOC_PORT_IDX_BINDEX(unit, port, idx) \
    (SOC_PORT_IDX_INFO(unit, port, idx).bindex)
#define SOC_PORT_BLOCK(unit, port)      SOC_PORT_IDX_BLOCK(unit, port, 0)
#define SOC_PORT_BINDEX(unit, port)     SOC_PORT_IDX_BINDEX(unit, port, 0)
#define SOC_PIPE_ANY                    -1

/* Number of Queues for a given port */
#define SOC_PORT_NUM_COSQ(unit, port)   (SOC_INFO(unit).port_num_cosq[port])

/*
 * In blocks with an instance per core, used to select all instance for writes, and any instance for reads.
 * In register access used as the port parameter.
 * In memory access used either as the block parameter or as <block>_BLOCK(unit, SOC_CORE_ALL).
 */
#define SOC_CORE_DEFAULT                 0
#define SOC_CORE_INVALID                 -1
#define SOC_CORE_ALL                    _SHR_CORE_ALL
#define MAX_NUM_OF_CORES                2
#define MAX_PORTS_IN_CORE             256

/* used as the block parameter in memory access */
#define SOC_BLOCK_ANY                   -1      /* for reading */
#define SOC_BLOCK_ALL                   -1      /* for writing */
#define SOC_BLOCK_NAME(unit,blk)        ((int)blk == SOC_BLOCK_ALL ? \
                                        "*" : \
                                        SOC_INFO(unit).block_name[blk])

#define SOC_BLOCK_PORT(unit, blk)       (SOC_INFO(unit).block_port[blk])
#define SOC_BLOCK_BITMAP(unit, blk)     (SOC_INFO(unit).block_bitmap[blk])
#define SOC_BLOCK_TYPE(unit, blk)       (SOC_BLOCK_INFO(unit, blk).type)
#define SOC_BLOCK_NUMBER(unit, blk)     (SOC_BLOCK_INFO(unit, blk).number)
/* macros for broadcast blocks */
#define SOC_BLOCK_IS_BROADCAST(unit, blk)            ((blk >= 0) && (SOC_INFO(unit).broadcast_blocks[blk]))
#define SOC_BLOCK_BROADCAST_SIZE(unit, blk)          (SOC_INFO(unit).broadcast_blocks_size[blk])
#define SOC_BLOCK_BROADCAST_MEMBER(unit, blk, index) (SOC_INFO(unit).broadcast_blocks[blk][index])

#define SOC_BLOCK_IS(blk, type) (blk[0] == type)
/* Used for regs in a single block only */
#define SOC_REG_BLOCK_IS(unit, reg, type) \
    SOC_BLOCK_IS(SOC_REG_INFO(unit, reg).block, type)

extern int SOC_BLOCK_IN_LIST(int *blk, int type);
/* Used for regs in multiple blocks */
#define SOC_REG_BLOCK_IN_LIST(unit, reg, type) \
    SOC_BLOCK_IN_LIST(SOC_REG_INFO(unit, reg).block, type)

/*
 * Macros to get the device block type, number (instance) and index
 * within block for a given port
 */
#define SOC_PORT_BLOCK_TYPE(unit, port)    \
    (SOC_BLOCK_TYPE(unit, SOC_PORT_BLOCK(unit, port)))

#define SOC_PORT_BLOCK_NUMBER(unit, port)    \
    (SOC_BLOCK_NUMBER(unit, SOC_PORT_BLOCK(unit, port)))

#define SOC_PORT_BLOCK_INDEX(unit, port)   (SOC_PORT_BINDEX(unit, port))

#define NUM_PIPE(unit)                  (SOC_INFO(unit).num_pipe)
#define NUM_XPE(unit)                   (SOC_INFO(unit).num_xpe)
#define NUM_SLICE(unit)                 (SOC_INFO(unit).num_slice)
#define NUM_LAYER(unit)                 (SOC_INFO(unit).num_layer)

#define NUM_COS(unit)                   (SOC_DRIVER(unit)->num_cos)
#define NUM_CPU_COSQ(unit)              (SOC_INFO(unit).num_cpu_cosq)
#define NUM_CPU_COSQ_DEF                (16)
#define NUM_CPU_COSQ_MAX                (64)

                                        
#define NUM_TIME_INTERFACE(unit)        (SOC_INFO(unit).num_time_interface)

#define SFLOW_RANGE_MAX(unit)           (SOC_INFO(unit).sflow_range_max)
#define NAT_ID_MAX(unit)                (SOC_INFO(unit).nat_id_max)

#define NUM_MODID(unit)                 (SOC_INFO(unit).modid_count)
#define SOC_MODPORT_MAX(unit)           (SOC_INFO(unit).modport_max)
#define SOC_MODPORT_MAX_FIRST(unit)     (SOC_INFO(unit).modport_max_first)
#define PORT_DUALMODID_VALID(_u_, _p_) \
    if ((NUM_MODID(_u_) > 1) && ((_p_ > SOC_INFO(_u_).modport_max) || \
                                 (_p_ < -1))) \
        {return SOC_E_PORT;}

#define SOC_BASE_MODID(unit)  SOC_CONTROL(unit)->base_modid
#define SOC_BASE_MODID_SET(unit, value) \
            SOC_CONTROL(unit)->base_modid = value

#define SOC_VLAN_XLATE_GLP_WILDCARD(unit) (SOC_INFO(unit).vlan_xlate_glp_wildcard)
#define SOC_MODID_MAX(unit)             (SOC_INFO(unit).modid_max)
#define SOC_TRUNK_BIT_POS(unit)         (SOC_INFO(unit).trunk_bit_pos)
#define SOC_PORT_ADDR_MAX(unit)         (SOC_INFO(unit).port_addr_max)
#define SOC_HG_OFFSET(unit)             (SOC_INFO(unit).hg_offset)
#define SOC_PORT_ADDRESSABLE(unit,port) (port >= 0 && port <= \
                                            SOC_INFO(unit).port_addr_max)
#define SOC_MODID_ADDRESSABLE(unit,mod) (mod >= 0 && mod <= \
                                            SOC_INFO(unit).modid_max)

#define SOC_PORT(unit,ptype,pno)        (SOC_INFO(unit).ptype.port[pno])
#define SOC_PORT_NUM(unit,ptype)        (SOC_INFO(unit).ptype.num)
#define SOC_PORT_MIN(unit,ptype)        (SOC_INFO(unit).ptype.min)
#define SOC_PORT_MAX(unit,ptype)        (SOC_INFO(unit).ptype.max)
#define SOC_PORT_BITMAP(unit,ptype)     (SOC_INFO(unit).ptype.bitmap)
#define SOC_PORT_DISABLED_BITMAP(unit,ptype) \
    (SOC_INFO(unit).ptype.disabled_bitmap)

#define NUM_OLP_PORT(unit)              (SOC_INFO(unit).olp_port[0] > 0 ? 1 : 0)
#define NUM_ERP_PORT(unit)              (SOC_INFO(unit).erp_port[0] > 0 ? 1 : 0)
#define NUM_OAMP_PORT(unit)             (SOC_INFO(unit).oamp_port[0] > 0 ? 1 : 0)
#define NUM_RCY_PORT(unit)              SOC_PORT_NUM(unit,rcy)
#define NUM_FE_PORT(unit)               SOC_PORT_NUM(unit,fe)
#define NUM_GE_PORT(unit)               SOC_PORT_NUM(unit,ge)
#define NUM_LLID_PORT(unit)             SOC_PORT_NUM(unit,llid)
#define NUM_PON_PORT(unit)              SOC_PORT_NUM(unit,pon)
#define NUM_XE_PORT(unit)               SOC_PORT_NUM(unit,xe)
#define NUM_HG_PORT(unit)               SOC_PORT_NUM(unit,hg)
#define NUM_HG_SUBPORT_PORT(unit)       SOC_PORT_NUM(unit,hg_subport)
#define NUM_HL_PORT(unit)               SOC_PORT_NUM(unit,hl)
#define NUM_IL_PORT(unit)               SOC_PORT_NUM(unit,il)
#define NUM_SCH_PORT(unit)              SOC_PORT_NUM(unit,sch)
#define NUM_ST_PORT(unit)               SOC_PORT_NUM(unit,st)
#define NUM_GX_PORT(unit)               SOC_PORT_NUM(unit,gx)
#define NUM_XL_PORT(unit)               SOC_PORT_NUM(unit,xl)
#define NUM_XLB0_PORT(unit)             SOC_PORT_NUM(unit,xlb0)
#define NUM_MXQ_PORT(unit)              SOC_PORT_NUM(unit,mxq)
#define NUM_XG_PORT(unit)               SOC_PORT_NUM(unit,xg)
#define NUM_XQ_PORT(unit)               SOC_PORT_NUM(unit,xq)
#define NUM_HYPLITE_PORT(unit)          SOC_PORT_NUM(unit,hyplite)
#define NUM_SPI_PORT(unit)              SOC_PORT_NUM(unit,spi)
#define NUM_SPI_SUBPORT_PORT(unit)      SOC_PORT_NUM(unit,spi_subport)
#define NUM_SCI_PORT(unit)              SOC_PORT_NUM(unit,sci)
#define NUM_SFI_PORT(unit)              SOC_PORT_NUM(unit,sfi)
#define NUM_REQ_PORT(unit)              SOC_PORT_NUM(unit,req)
#define NUM_FAB_PORT(unit)              SOC_PORT_NUM(unit,fab)
#define NUM_E_PORT(unit)                SOC_PORT_NUM(unit,ether)
#define NUM_LP_PORT(unit)               SOC_PORT_NUM(unit,lp)
#define NUM_SUBTAG_PORT(unit)           SOC_PORT_NUM(unit,subtag)
#define NUM_PORT(unit)                  SOC_PORT_NUM(unit,port)
#define NUM_ALL_PORT(unit)              SOC_PORT_NUM(unit,all)

#define MAX_PORT(unit)                  (SOC_INFO(unit).port_num)
#define LB_PORT(unit)                   (SOC_INFO(unit).lb_port)
#define RDB_PORT(unit)                  (SOC_INFO(unit).rdb_port)
#define AXP_PORT(unit, type)            (SOC_INFO(unit).axp_port[type])
#define IPIC_PORT(unit)                 (SOC_INFO(unit).ipic_port)
#define IPIC_BLOCK(unit)                (SOC_INFO(unit).ipic_block)
#define CMIC_PORT(unit)                 (SOC_INFO(unit).cmic_port)
#define CMIC_BLOCK(unit)                (SOC_INFO(unit).cmic_block)
#define RCPU_PORT(unit)                 (SOC_INFO(unit).rcpu_port)
#define ARL_BLOCK(unit)                 (SOC_INFO(unit).arl_block)
#define MMU_BLOCK(unit)                 (SOC_INFO(unit).mmu_block)
#define MMU_GLB_BLOCK(unit)             (SOC_INFO(unit).mmu_glb_block)
#define MMU_XPE_BLOCK(unit)             (SOC_INFO(unit).mmu_xpe_block)
#define MMU_SC_BLOCK(unit)              (SOC_INFO(unit).mmu_sc_block)
#define MCU_BLOCK(unit)                 (SOC_INFO(unit).mcu_block)
#define IPIPE_BLOCK(unit)               (SOC_INFO(unit).ipipe_block)
#define IPIPE_HI_BLOCK(unit)            (SOC_INFO(unit).ipipe_hi_block)
#define EPIPE_BLOCK(unit)               (SOC_INFO(unit).epipe_block)
#define EPIPE_HI_BLOCK(unit)            (SOC_INFO(unit).epipe_hi_block)
#define IGR_BLOCK(unit)                 (SOC_INFO(unit).igr_block)
#define EGR_BLOCK(unit)                 (SOC_INFO(unit).egr_block)
#define BSE_BLOCK(unit)                 (SOC_INFO(unit).bse_block)
#define CSE_BLOCK(unit)                 (SOC_INFO(unit).cse_block)
#define HSE_BLOCK(unit)                 (SOC_INFO(unit).hse_block)
#define BSAFE_BLOCK(unit)               (SOC_INFO(unit).bsafe_block)
#define OTPC_BLOCK(unit)                (SOC_INFO(unit).otpc_block)
#define ESM_BLOCK(unit)                 (SOC_INFO(unit).esm_block)
#define PGW_CL_BLOCK(unit, instance)    ((((instance) >= 0) && ((instance) < 8)) ? SOC_INFO(unit).pgw_cl_block[(instance)] : (-1))
#define PG4_BLOCK(unit, instance)       ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).pg4_block[(instance)] : (-1))
#define PG5_BLOCK(unit, instance)       ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).pg5_block[(instance)] : (-1))
#define EXTERN_BLOCK(unit)              (SOC_INFO(unit).exter_block)
#define TOP_BLOCK(unit)                 (SOC_INFO(unit).top_block)
#define SER_BLOCK(unit)                 (SOC_INFO(unit).ser_block)
#define AVS_BLOCK(unit)                 (SOC_INFO(unit).avs_block)
#define AXP_BLOCK(unit)                 (SOC_INFO(unit).axp_block)
#define ISM_BLOCK(unit)                 (SOC_INFO(unit).ism_block)
#define ETU_BLOCK(unit)                 (SOC_INFO(unit).etu_block)
#define ETU_WRAP_BLOCK(unit)            (SOC_INFO(unit).etu_wrap_block)
#define IBOD_BLOCK(unit)                (SOC_INFO(unit).ibod_block)
#define LLS_BLOCK(unit)                 (SOC_INFO(unit).lls_block)
#define CES_BLOCK(unit)                 (SOC_INFO(unit).ces_block)
#define IPROC_BLOCK(unit)               (SOC_INFO(unit).iproc_block)
#define SOC_MAX_NUM_PMQ_BLKS            6
#define PMQ_BLOCK(unit, instance)       ((((instance) >= 0) && ((instance) < SOC_INFO(unit).nof_pmqs)) ? SOC_INFO(unit).pmq_block[(instance)] : (-1))
#define PGW_GE_BLOCK(unit, instance)    ((((instance) >= 0) && ((instance) < 3)) ? SOC_INFO(unit).pgw_ge_block[(instance)] : (-1))


#define MS_ISEC0_BLOCK(unit)            (SOC_INFO(unit).ms_isec_block[0])
#define MS_ISEC1_BLOCK(unit)            (SOC_INFO(unit).ms_isec_block[1])
#define MS_ISEC_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).ms_isec_block[(instance)] : (-1))
#define MS_ESEC0_BLOCK(unit)            (SOC_INFO(unit).ms_esec_block[0])
#define MS_ESEC1_BLOCK(unit)            (SOC_INFO(unit).ms_esec_block[1])
#define MS_ESEC_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).ms_esec_block[(instance)] : (-1))
#define IL0_BLOCK(unit)                 (SOC_INFO(unit).il_block[0])
#define IL1_BLOCK(unit)                 (SOC_INFO(unit).il_block[1])
#define IL_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).il_block[(instance)] : (-1))
#define CW_BLOCK(unit)                  (SOC_INFO(unit).cw)

#define MS_ISEC0_BLOCK(unit)            (SOC_INFO(unit).ms_isec_block[0])
#define MS_ISEC1_BLOCK(unit)            (SOC_INFO(unit).ms_isec_block[1])
#define MS_ISEC_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).ms_isec_block[(instance)] : (-1))
#define MS_ESEC0_BLOCK(unit)            (SOC_INFO(unit).ms_esec_block[0])
#define MS_ESEC1_BLOCK(unit)            (SOC_INFO(unit).ms_esec_block[1])
#define MS_ESEC_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).ms_esec_block[(instance)] : (-1))
#define IL0_BLOCK(unit)                 (SOC_INFO(unit).il_block[0])
#define IL1_BLOCK(unit)                 (SOC_INFO(unit).il_block[1])
#define IL_BLOCK(unit, instance)   ((((instance) >= 0) && ((instance) < 2)) ? SOC_INFO(unit).il_block[(instance)] : (-1))
#define CW_BLOCK(unit)                  (SOC_INFO(unit).cw)

#define BP_BLOCK(unit)                  (SOC_INFO(unit).bp_block)
#define CS_BLOCK(unit)                  (SOC_INFO(unit).cs_block)
#define EB_BLOCK(unit)                  (SOC_INFO(unit).eb_block)
#define EP_BLOCK(unit)                  (SOC_INFO(unit).ep_block)
#define ES_BLOCK(unit)                  (SOC_INFO(unit).es_block)
#define FD_BLOCK(unit)                  (SOC_INFO(unit).fd_block)
#define FF_BLOCK(unit)                  (SOC_INFO(unit).ff_block)
#define FR_BLOCK(unit)                  (SOC_INFO(unit).fr_block)
#define TX_BLOCK(unit)                  (SOC_INFO(unit).tx_block)
#define QMA_BLOCK(unit)                 (SOC_INFO(unit).qma_block)
#define QMB_BLOCK(unit)                 (SOC_INFO(unit).qmb_block)
#define QMC_BLOCK(unit)                 (SOC_INFO(unit).qmc_block)
#define QSA_BLOCK(unit)                 (SOC_INFO(unit).qsa_block)
#define QSB_BLOCK(unit)                 (SOC_INFO(unit).qsb_block)
#define RB_BLOCK(unit)                  (SOC_INFO(unit).rb_block)
#define SC_TOP_BLOCK(unit)              (SOC_INFO(unit).sc_top_block)
#define SF_TOP_BLOCK(unit)              (SOC_INFO(unit).sf_top_block)
#define TS_BLOCK(unit)                  (SOC_INFO(unit).ts_block)

#ifdef  BCM_CALADAN3_SUPPORT
#define SOC_MAX_NUM_CI_BLKS             (16)
#else
#define SOC_MAX_NUM_CI_BLKS             (15)
#endif

#define CI0_BLOCK(unit)                 (SOC_INFO(unit).ci_block[0])
#define CI1_BLOCK(unit)                 (SOC_INFO(unit).ci_block[1])
#define CI2_BLOCK(unit)                 (SOC_INFO(unit).ci_block[2])
#define CI3_BLOCK(unit)                 (SOC_INFO(unit).ci_block[3])
#define CI4_BLOCK(unit)                 (SOC_INFO(unit).ci_block[4])
#define CI5_BLOCK(unit)                 (SOC_INFO(unit).ci_block[5])
#define CI6_BLOCK(unit)                 (SOC_INFO(unit).ci_block[6])
#define CI7_BLOCK(unit)                 (SOC_INFO(unit).ci_block[7])
#define CI8_BLOCK(unit)                 (SOC_INFO(unit).ci_block[8])
#define CI9_BLOCK(unit)                 (SOC_INFO(unit).ci_block[9])
#define CI9_BLOCK(unit)                 (SOC_INFO(unit).ci_block[9])
#define CI10_BLOCK(unit)                (SOC_INFO(unit).ci_block[10])
#define CI11_BLOCK(unit)                (SOC_INFO(unit).ci_block[11])
#define CI12_BLOCK(unit)                (SOC_INFO(unit).ci_block[12])
#define CI13_BLOCK(unit)                (SOC_INFO(unit).ci_block[13])
#define CI14_BLOCK(unit)                (SOC_INFO(unit).ci_block[14])
#ifdef  BCM_CALADAN3_SUPPORT
#define CI15_BLOCK(unit)                (SOC_INFO(unit).ci_block[15])
#endif
#define CI_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_CI_BLKS)) ? \
                                               SOC_INFO(unit).ci_block[(instance)] : (-1))
#define SOC_MAX_NUM_XPORT_BLKS          (8)
#define XPORT0_BLOCK(unit)              (SOC_INFO(unit).xport_block[0])
#define XPORT1_BLOCK(unit)              (SOC_INFO(unit).xport_block[1])
#define XPORT2_BLOCK(unit)              (SOC_INFO(unit).xport_block[2])
#define XPORT3_BLOCK(unit)              (SOC_INFO(unit).xport_block[3])
#define XPORT4_BLOCK(unit)              (SOC_INFO(unit).xport_block[4])
#define XPORT5_BLOCK(unit)              (SOC_INFO(unit).xport_block[5])
#define XPORT6_BLOCK(unit)              (SOC_INFO(unit).xport_block[6])
#define XPORT7_BLOCK(unit)              (SOC_INFO(unit).xport_block[7])
#define XPORT_BLOCK(unit, instance)     ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_XPORT_BLKS)) ? SOC_INFO(unit).xport_block[(instance)] : (-1))

#define SOC_MAX_NUM_CLPORT_BLKS         (2)
#define CLPORT0_BLOCK(unit)              (SOC_INFO(unit).clport_block[0])
#define CLPORT1_BLOCK(unit)              (SOC_INFO(unit).clport_block[1])
#define CLPORT_BLOCK(unit, instance)     ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_XPORT_BLKS)) ? \
                                                   SOC_INFO(unit).clport_block[(instance)] : (-1))

#define SOC_MAX_NUM_CLPORT0_BLKS         (1)
#define SOC_MAX_NUM_XTPORT_BLKS          (3)
#define SOC_MAX_NUM_ILPORT_BLKS          (2)

#define CM_BLOCK(unit)                  (SOC_INFO(unit).cm_block)

#define SOC_MAX_NUM_COP_BLKS            (2)
#define CO0_BLOCK(unit)                 (SOC_INFO(unit).co_block[0])
#define CO1_BLOCK(unit)                 (SOC_INFO(unit).co_block[1])
#define CO_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_COP_BLKS)) ? \
                                                   SOC_INFO(unit).co_block[(instance)] : (-1))

#define CX_BLOCK(unit)                  (SOC_INFO(unit).cx_block)
#define PD_BLOCK(unit)                  (SOC_INFO(unit).pd_block)

#define SOC_MAX_NUM_PR_BLKS             (2)
#define PR0_BLOCK(unit)                 (SOC_INFO(unit).pr_block[0])
#define PR1_BLOCK(unit)                 (SOC_INFO(unit).pr_block[1])
#define PR_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_PR_BLKS)) ? \
                                                   SOC_INFO(unit).pr_block[(instance)] : (-1))

#define SOC_MAX_NUM_PT_BLKS             (2)
#define PT0_BLOCK(unit)                 (SOC_INFO(unit).pt_block[0])
#define PT1_BLOCK(unit)                 (SOC_INFO(unit).pt_block[1])
#define PT_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_PT_BLKS)) ? \
                                                   SOC_INFO(unit).pt_block[(instance)] : (-1))

#define QM_BLOCK(unit)                  (SOC_INFO(unit).qm_block)

#define SOC_MAX_NUM_TMU_BLKS            (2)
#define TMA_BLOCK(unit)                 (SOC_INFO(unit).tma_block)
#define TMB_BLOCK(unit)                 (SOC_INFO(unit).tmb_block)

#define CM_BLOCK(unit)                  (SOC_INFO(unit).cm_block)
#define PD_BLOCK(unit)                  (SOC_INFO(unit).pd_block)
#define OC_BLOCK(unit)                  (SOC_INFO(unit).oc_block)
#define RC_BLOCK(unit)                  (SOC_INFO(unit).rc_block)
#define LRA_BLOCK(unit)                 (SOC_INFO(unit).lra_block)
#define LRB_BLOCK(unit)                 (SOC_INFO(unit).lrb_block)
#define PP_BLOCK(unit)                  (SOC_INFO(unit).pp_block)
#define PB_BLOCK(unit)                  (SOC_INFO(unit).pb_block)

#define SOC_MAX_NUM_TM_QE_BLKS             (16)
#define TM_QE0_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[0])
#define TM_QE1_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[1])
#define TM_QE2_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[2])
#define TM_QE3_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[3])
#define TM_QE4_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[4])
#define TM_QE5_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[5])
#define TM_QE6_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[6])
#define TM_QE7_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[7])
#define TM_QE8_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[8])
#define TM_QE9_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[9])
#define TM_QE9_BLOCK(unit)                 (SOC_INFO(unit).tm_qe_block[9])
#define TM_QE10_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[10])
#define TM_QE11_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[11])
#define TM_QE12_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[12])
#define TM_QE13_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[13])
#define TM_QE14_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[14])
#define TM_QE15_BLOCK(unit)                (SOC_INFO(unit).tm_qe_block[15])
#define TM_QE_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_TM_QE_BLKS)) ? \
                                               SOC_INFO(unit).tm_qe_block[(instance)] : (-1))
#define SOC_MAX_NUM_TP_BLKS             (2)
#define TP0_BLOCK(unit)                 (SOC_INFO(unit).tp_block[0])
#define TP1_BLOCK(unit)                 (SOC_INFO(unit).tp_block[1])
#define TP_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_TP_BLKS)) ? \
                                                   SOC_INFO(unit).tp_block[(instance)] : (-1))
#define SOC_MAX_NUM_TXLP_BLKS             (2)
#define SOC_MAX_NUM_RXLP_BLKS             (2)
#define TXLP0_BLOCK(unit)                 (SOC_INFO(unit).txlp_block[0])
#define TXLP1_BLOCK(unit)                 (SOC_INFO(unit).txlp_block[1])
#define RXLP0_BLOCK(unit)                 (SOC_INFO(unit).rxlp_block[0])
#define RXLP1_BLOCK(unit)                 (SOC_INFO(unit).rxlp_block[1])
#define TXLP_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_TXLP_BLKS)) ? \
                                               SOC_INFO(unit).txlp_block[(instance)] : (-1))
#define RXLP_BLOCK(unit, instance)        ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_RXLP_BLKS)) ? \
                                                   SOC_INFO(unit).rxlp_block[(instance)] : (-1))
#define SOC_MAX_NUM_IECELL_BLKS           (2)
#define IECELL0_BLOCK(unit)               (SOC_INFO(unit).iecell_block[0])
#define IECELL1_BLOCK(unit)               (SOC_INFO(unit).iecell_block[1])
#define IECELL_BLOCK(unit, instance)      ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_IECELL_BLKS)) ? \
                                               SOC_INFO(unit).iecell_block[(instance)] : (-1))

/* DPP blocks */
#define SOC_MAX_NUM_DPP_PIPES 2
#define SOC_MAX_NUM_CGM_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_EGQ_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_EPNI_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_IHB_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_IHP_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_IPS_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_IQM_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_SCH_BLKS SOC_MAX_NUM_DPP_PIPES
#define SOC_MAX_NUM_CFC_BLKS 2
#define CFC_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_CFC_BLKS)) ? SOC_INFO(unit).cfc_block[instance] : (-1))
#define SOC_MAX_NUM_OCB_BLKS 2
#define OCB_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_OCB_BLKS)) ? SOC_INFO(unit).ocb_block[instance] : (-1))
#define CRPS_BLOCK(unit) (SOC_INFO(unit).crps_block)
#define ECI_BLOCK(unit) (SOC_INFO(unit).eci_block)
#define EDB_BLOCK(unit) (SOC_INFO(unit).edb_block)
#define ILKN_PMH_BLOCK(unit) (SOC_INFO(unit).ilkn_pmh_block)
#define IPST_BLOCK(unit) (SOC_INFO(unit).ipst_block)
#define ILKN_PML_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_ILKN_PML_BLKS)) ? SOC_INFO(unit).ilkn_pml_block[instance] : (-1))
#define IQMT_BLOCK(unit) (SOC_INFO(unit).iqmt_block)
#define KAPS_BLOCK(unit) (SOC_INFO(unit).kaps_block)

#define ILB_BLOCK(unit) (SOC_INFO(unit).ilb_block)
#define IEP_BLOCK(unit) (SOC_INFO(unit).iep_block)
#define IMP_BLOCK(unit) (SOC_INFO(unit).imp_block)
#define SPB_BLOCK(unit) (SOC_INFO(unit).spb_block)
#define ITE_BLOCK(unit) (SOC_INFO(unit).ite_block)
#define DDP_BLOCK(unit) (SOC_INFO(unit).ddp_block)
#define TXQ_BLOCK(unit) (SOC_INFO(unit).txq_block)
#define TAR_BLOCK(unit) (SOC_INFO(unit).tar_block)
#define PTS_BLOCK(unit) (SOC_INFO(unit).pts_block)
#define SQM_BLOCK(unit) (SOC_INFO(unit).sqm_block)
#define IPSEC_BLOCK(unit) (SOC_INFO(unit).ipsec_block)
#define SOC_MAX_NUM_IPSEC_SPU_WRAPPER_TOP_BLKS 2
#define IPSEC_SPU_WRAPPER_TOP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_IPSEC_SPU_WRAPPER_TOP_BLKS)) ? SOC_INFO(unit).ipsec_spu_wrapper_top_block[instance] : (-1))
#define SOC_MAX_NUM_MXQ_BLKS 7
#define MXQ_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_MXQ_BLKS)) ? SOC_INFO(unit).mxq_block[instance] : (-1))
#define SOC_MAX_NUM_KAPS_BBS_BLKS 3
#define PLL_BLOCK(unit) (SOC_INFO(unit).pll_block)
#define DQM_BLOCK(unit) (SOC_INFO(unit).dqm_block)
#define ECGM_BLOCK(unit) (SOC_INFO(unit).ecgm_block)
#define GCK_BLOCK(unit) (SOC_INFO(unit).gck_block)
#define IDB_BLOCK(unit) (SOC_INFO(unit).idb_block)
#define PEM_BLOCK(unit) (SOC_INFO(unit).pem_block)

#define PPDB_A_BLOCK(unit) (SOC_INFO(unit).ppdb_a_block)
#define PPDB_B_BLOCK(unit) (SOC_INFO(unit).ppdb_b_block)
#define SOC_MAX_NUM_NBIL_BLKS 2
#define NBIL_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_NBIL_BLKS)) ? SOC_INFO(unit).nbil_block[instance] : (-1))
#define NBIH_BLOCK(unit) (SOC_INFO(unit).nbih_block)
#define NIF_BLOCK(unit) (SOC_INFO(unit).nif_block)
#define FCR_BLOCK(unit) (SOC_INFO(unit).fcr_block)
#define FCT_BLOCK(unit) (SOC_INFO(unit).fct_block)
#define FDR_BLOCK(unit) (SOC_INFO(unit).fdr_block)
#define FDA_BLOCK(unit) (SOC_INFO(unit).fda_block)
#define FDT_BLOCK(unit) (SOC_INFO(unit).fdt_block)
#define MESH_TOPOLOGY_BLOCK(unit) (SOC_INFO(unit).mesh_topology_block)
#define IDR_BLOCK(unit) (SOC_INFO(unit).idr_block)
#define IPT_BLOCK(unit) (SOC_INFO(unit).ipt_block)
#define IRE_BLOCK(unit) (SOC_INFO(unit).ire_block)
#define IRR_BLOCK(unit) (SOC_INFO(unit).irr_block)
#ifdef BCM_88790_A0
#define SOC_MAX_NUM_FMAC_BLKS 48
#else
#define SOC_MAX_NUM_FMAC_BLKS 36
#endif
#define FMAC_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_FMAC_BLKS)) ? SOC_INFO(unit).fmac_block[instance] : (-1))
#define SOC_MAX_NUM_XLP_BLKS 12
#define XLP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_XLP_BLKS)) ? SOC_INFO(unit).xlp_block[instance] : (-1))
#define SOC_MAX_NUM_CLP_BLKS 12
#define CLP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_CLP_BLKS)) ? SOC_INFO(unit).clp_block[instance] : (-1))
#define NBI_BLOCK(unit) (SOC_INFO(unit).nbi_block)
#define OAMP_BLOCK(unit) (SOC_INFO(unit).oamp_block)
#define OLP_BLOCK(unit) (SOC_INFO(unit).olp_block)
#define SOC_MAX_NUM_PQP_BLKS 2
#define PQP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_PQP_BLKS)) ? SOC_INFO(unit).pqp_block[instance] : (-1))
#define SOC_MAX_NUM_SIF_BLKS 2
#define SIF_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_SIF_BLKS)) ? SOC_INFO(unit).sif_block[instance] : (-1))
#define SOC_MAX_NUM_NIF_ILU_BLKS 8
#define NIF_ILU_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_NIF_ILU_BLKS)) ? SOC_INFO(unit).nif_ilu_block[instance] : (-1))
#define SOC_MAX_NUM_NIF_CDU_BLKS 8
#define NIF_CDU_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_NIF_CDU_BLKS)) ? SOC_INFO(unit).nif_cdu_block[instance] : (-1))
#define SOC_MAX_NUM_NIF_NMG_BLKS 2
#define NIF_NMG_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_NIF_NMG_BLKS)) ? SOC_INFO(unit).nif_nmg_block[instance] : (-1))
#define SOC_MAX_NUM_MCP_BLKS 2
#define MCP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_MCP_BLKS)) ? SOC_INFO(unit).mcp_block[instance] : (-1))
#define SOC_MAX_NUM_ITPP_BLKS 4
#define ITPP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_ITPP_BLKS)) ? SOC_INFO(unit).itpp_block[instance] : (-1))
#define SOC_MAX_NUM_PDM_BLKS 2
#define PDM_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_PDM_BLKS)) ? SOC_INFO(unit).pdm_block[instance] : (-1))
#define SOC_MAX_NUM_BDM_BLKS 2
#define BDM_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_BDM_BLKS)) ? SOC_INFO(unit).bdm_block[instance] : (-1))
#define MTM_BLOCK(unit) (SOC_INFO(unit).mtm_block)
#define HBM_TDU_BLOCK(unit) (SOC_INFO(unit).hbm_tdu_block)
#define SOC_MAX_NUM_HBM_BLKS 16
#define HBM_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_HBM_BLKS)) ? SOC_INFO(unit).hbm_block[instance] : (-1))
#define SOC_MAX_NUM_RQP_BLKS 2
#define RQP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_RQP_BLKS)) ? SOC_INFO(unit).rqp_block[instance] : (-1))
#define SOC_MAX_NUM_EQM_BLKS 2
#define EQM_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_EQM_BLKS)) ? SOC_INFO(unit).eqm_block[instance] : (-1))
#define SOC_MAX_NUM_EPS_SPR_BLKS 2
#define EPS_SPR_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_EPS_SPR_BLKS)) ? SOC_INFO(unit).eps_spr_block[instance] : (-1))
#define SOC_MAX_NUM_FQP_BLKS 2
#define FQP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_FQP_BLKS)) ? SOC_INFO(unit).fqp_block[instance] : (-1))
#define SOC_MAX_NUM_DHA_BLKS 16
#define DHA_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DHA_BLKS)) ? SOC_INFO(unit).dha_block[instance] : (-1))
#define SOC_MAX_NUM_DHB_BLKS 8
#define DHB_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DHB_BLKS)) ? SOC_INFO(unit).dhb_block[instance] : (-1))
#define MDB_BLOCK(unit) (SOC_INFO(unit).mdb_block)
#define SOC_MAX_NUM_ERPP_BLKS 2
#define ERPP_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_ERPP_BLKS)) ? SOC_INFO(unit).erpp_block[instance] : (-1))
#define SOC_MAX_NUM_ETPPA_BLKS 2
#define ETPPA_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_ETPPA_BLKS)) ? SOC_INFO(unit).etppa_block[instance] : (-1))
#define SOC_MAX_NUM_ETPPB_BLKS 2
#define ETPPB_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_ETPPB_BLKS)) ? SOC_INFO(unit).etppb_block[instance] : (-1))
#define EVENTOR_BLOCK(unit) (SOC_INFO(unit).eventor_block)
#define MACT_BLOCK(unit) (SOC_INFO(unit).mact_block)
#define SOC_MAX_NUM_IPPA_BLKS 2
#define IPPA_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_IPPA_BLKS)) ? SOC_INFO(unit).ippa_block[instance] : (-1))
#define SOC_MAX_NUM_IPPB_BLKS 2
#define IPPB_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_IPPB_BLKS)) ? SOC_INFO(unit).ippb_block[instance] : (-1))
#define SOC_MAX_NUM_IPPC_BLKS 2
#define IPPC_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_IPPC_BLKS)) ? SOC_INFO(unit).ippc_block[instance] : (-1))
#define SOC_MAX_NUM_IPPD_BLKS 2
#define IPPD_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_IPPD_BLKS)) ? SOC_INFO(unit).ippd_block[instance] : (-1))
#ifdef BCM_88790_A0
#define SOC_MAX_NUM_FSRD_BLKS 24
#else
#define SOC_MAX_NUM_FSRD_BLKS 12
#endif
#define SOC_MAX_NUM_ILKN_PML_BLKS 2
#define FSRD_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_FSRD_BLKS)) ? SOC_INFO(unit).fsrd_block[instance] : (-1))
#define RTP_BLOCK(unit) (SOC_INFO(unit).rtp_block)
#define DRCA_BLOCK(unit) (SOC_INFO(unit).drca_block)
#define DRCB_BLOCK(unit) (SOC_INFO(unit).drcb_block)
#define DRCC_BLOCK(unit) (SOC_INFO(unit).drcc_block)
#define DRCD_BLOCK(unit) (SOC_INFO(unit).drcd_block)
#define DRCE_BLOCK(unit) (SOC_INFO(unit).drce_block)
#define DRCF_BLOCK(unit) (SOC_INFO(unit).drcf_block)
#define DRCG_BLOCK(unit) (SOC_INFO(unit).drcg_block)
#define DRCH_BLOCK(unit) (SOC_INFO(unit).drch_block)
#define DRCBROADCAST_BLOCK(unit) (SOC_INFO(unit).drcbroadcast_block)
#define OCCG_BLOCK(unit) (SOC_INFO(unit).occg_block)
#define SOC_MAX_NUM_MRPS_BLKS 2
#define SOC_MAX_NUM_MTRPS_EM_BLKS 2
#ifdef BCM_88790_A0
#define SOC_MAX_NUM_DCH_BLKS 8
#else
#define SOC_MAX_NUM_DCH_BLKS 4
#endif
#define DCH_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DCH_BLKS)) ? SOC_INFO(unit).dch_block[instance] : (-1))
#define SOC_MAX_NUM_DCL_BLKS 4
#define DCL_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DCL_BLKS)) ? SOC_INFO(unit).dcl_block[instance] : (-1))
#define SOC_MAX_NUM_DCMA_BLKS 2
#define DCMA_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DCMA_BLKS)) ? SOC_INFO(unit).dcma_block[instance] : (-1))
#define SOC_MAX_NUM_DCMB_BLKS 2
#define DCMB_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DCMB_BLKS)) ? SOC_INFO(unit).dcmb_block[instance] : (-1))
#define DCMC_BLOCK(unit) (SOC_INFO(unit).dcmc_block)
#define SOC_MAX_NUM_CCS_BLKS 4
#define CCS_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_CCS_BLKS)) ? SOC_INFO(unit).ccs_block[instance] : (-1))
#define SOC_MAX_NUM_DCM_BLKS 4
#define DCM_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_DCM_BLKS)) ? SOC_INFO(unit).dcm_block[instance] : (-1))

#define SOC_MAX_NUM_GPORT_BLKS 12
#define GPORT_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_GPORT_BLKS)) ? SOC_INFO(unit).gport_block[instance] : (-1))

#define KAPS_BBS_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_KAPS_BBS_BLKS)) ? \
   SOC_INFO(unit).kaps_bbs_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_kaps_bbs_block : -1))

#define MRPS_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_MRPS_BLKS)) ? \
   SOC_INFO(unit).mrps_blocks[instance] : (-1))

#define MTRPS_EM_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_MTRPS_EM_BLKS)) ? \
   SOC_INFO(unit).mtrps_em_blocks[instance] : (-1))

#define CGM_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_CGM_BLKS)) ? \
   SOC_INFO(unit).cgm_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_cgm_block : -1))

#define IPS_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_IPS_BLKS)) ? \
   SOC_INFO(unit).ips_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_ips_block : -1))

#define IHP_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_IHP_BLKS)) ? \
   SOC_INFO(unit).ihp_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_ihp_block : -1))

#define IQM_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_IQM_BLKS)) ? \
   SOC_INFO(unit).iqm_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_iqm_block : -1))

#define SCH_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_SCH_BLKS)) ? \
   SOC_INFO(unit).sch_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_sch_block : -1))

#define EPNI_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_EPNI_BLKS)) ? \
   SOC_INFO(unit).epni_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_epni_block : -1))

#define IHB_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_IHB_BLKS)) ? \
   SOC_INFO(unit).ihb_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_ihb_block : -1))

#define EGQ_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_EGQ_BLKS)) ? \
   SOC_INFO(unit).egq_blocks[instance] :    \
   (instance == SOC_CORE_ALL ? SOC_INFO(unit).brdc_egq_block : -1))

#define SOC_MAX_NUM_DCML_BLKS 8
#define DCML_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_DCML_BLKS)) ? \
   SOC_INFO(unit).dcml_block[instance] : (-1))
#define SOC_MAX_NUM_QRH_BLKS 8
#define QRH_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_QRH_BLKS)) ? \
   SOC_INFO(unit).qrh_block[instance] : (-1))
#define SOC_MAX_NUM_CCH_BLKS 8
#define CCH_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_CCH_BLKS)) ? \
   SOC_INFO(unit).cch_block[instance] : (-1))
#define SOC_MAX_NUM_LCM_BLKS 8
#define LCM_BLOCK(unit, instance) \
( ((instance >= 0) && (instance < SOC_MAX_NUM_LCM_BLKS)) ? \
   SOC_INFO(unit).lcm_block[instance] : (-1))

#define MCT_BLOCK(unit)         (SOC_INFO(unit).mct_block)
#define BRDC_CCH_BLOCK(unit)    (SOC_INFO(unit).brdc_cch_block)
#define BRDC_DCML_BLOCK(unit)   (SOC_INFO(unit).brdc_dcml_block)
#define BRDC_LCM_BLOCK(unit)    (SOC_INFO(unit).brdc_lcm_block)
#define BRDC_QRH_BLOCK(unit)    (SOC_INFO(unit).brdc_qrh_block)
#define BRDC_FMACH_BLOCK(unit)  (SOC_INFO(unit).brdc_fmach_block)
#define BRDC_FMACL_BLOCK(unit)  (SOC_INFO(unit).brdc_fmacl_block)
#define BRDC_FSRD_BLOCK(unit)   (SOC_INFO(unit).brdc_fsrd_block)
#define BRDC_FMAC_BLOCK(unit)   (SOC_INFO(unit).brdc_fmac_block)
#define BRDC_FMAC_AC_BLOCK(unit)  (SOC_INFO(unit).brdc_fmac_ac_block)
#define BRDC_FMAC_BD_BLOCK(unit)  (SOC_INFO(unit).brdc_fmac_bd_block)
#define BRDC_DCH_BLOCK(unit)  (SOC_INFO(unit).brdc_dch_block)
#define BRDC_DCL_BLOCK(unit)  (SOC_INFO(unit).brdc_dcl_block)
#define BRDC_DCM_BLOCK(unit)  (SOC_INFO(unit).brdc_dcm_block)
#define BRDC_CCS_BLOCK(unit)  (SOC_INFO(unit).brdc_ccs_block)
#define BRDC_CGM_BLOCK(unit)    (SOC_INFO(unit).brdc_cgm_block)
#define BRDC_EGQ_BLOCK(unit)    (SOC_INFO(unit).brdc_egq_block)
#define BRDC_EPNI_BLOCK(unit)   (SOC_INFO(unit).brdc_epni_block)
#define BRDC_IHB_BLOCK(unit)    (SOC_INFO(unit).brdc_ihb_block)
#define BRDC_IHP_BLOCK(unit)    (SOC_INFO(unit).brdc_ihp_block)
#define BRDC_IPS_BLOCK(unit)    (SOC_INFO(unit).brdc_ips_block)
#define BRDC_IQM_BLOCK(unit)    (SOC_INFO(unit).brdc_iqm_block)
#define BRDC_SCH_BLOCK(unit)    (SOC_INFO(unit).brdc_sch_block)
#define BRDC_KAPS_BBS_BLOCK(unit)  (SOC_INFO(unit).brdc_kaps_bbs_block)
#define PRM_BLOCK(unit) (SOC_INFO(unit).prm_block)
#define SOC_MAX_NUM_BLH_BLKS 3
#define BLH_BLOCK(unit, instance) ((((instance) >= 0) && ((instance) < SOC_MAX_NUM_BLH_BLKS)) ? SOC_INFO(unit).blh_block[instance] : (-1))
#define AM_TOP_BLOCK(unit) (SOC_INFO(unit).am_top_block)

/*
 * E is ethernet port (FE|GE|XE)
 * PORT is all net ports (E|HG|IL)
 * ALL is all ports (PORT|CMIC)
 */
#define PBMP_FE_ALL(unit)               SOC_PORT_BITMAP(unit,fe)
#define PBMP_GE_ALL(unit)               SOC_PORT_BITMAP(unit,ge)
#define PBMP_LLID_ALL(unit)             SOC_PORT_BITMAP(unit,llid)
#define PBMP_PON_ALL(unit)              SOC_PORT_BITMAP(unit,pon)
#define PBMP_CE_ALL(unit)               SOC_PORT_BITMAP(unit,ce)
#define PBMP_XE_ALL(unit)               SOC_PORT_BITMAP(unit,xe)
#define PBMP_HG_ALL(unit)               SOC_PORT_BITMAP(unit,hg)
#define PBMP_IL_ALL(unit)               SOC_PORT_BITMAP(unit,il)
#define PBMP_SCH_ALL(unit)              SOC_PORT_BITMAP(unit,sch)
#define PBMP_HG_SUBPORT_ALL(unit)       SOC_PORT_BITMAP(unit,hg_subport)
#define PBMP_HL_ALL(unit)               SOC_PORT_BITMAP(unit,hl)
#define PBMP_ST_ALL(unit)               SOC_PORT_BITMAP(unit,st)
#define PBMP_GX_ALL(unit)               SOC_PORT_BITMAP(unit,gx)
#define PBMP_XL_ALL(unit)               SOC_PORT_BITMAP(unit,xl)
#define PBMP_XLB0_ALL(unit)             SOC_PORT_BITMAP(unit,xlb0)
#define PBMP_XT_ALL(unit)               SOC_PORT_BITMAP(unit,xt)
#define PBMP_XW_ALL(unit)               SOC_PORT_BITMAP(unit,xw)
#define PBMP_CL_ALL(unit)               SOC_PORT_BITMAP(unit,cl)
#define PBMP_CLG2_ALL(unit)             SOC_PORT_BITMAP(unit,clg2)
#define PBMP_CXX_ALL(unit)              SOC_PORT_BITMAP(unit,cxx)
#define PBMP_C_ALL(unit)                SOC_PORT_BITMAP(unit,c)
#define PBMP_LB_ALL(unit)               SOC_PORT_BITMAP(unit,lbport)
#define PBMP_RDB_ALL(unit)              SOC_PORT_BITMAP(unit,rdbport)
#define PBMP_AXP_ALL(unit)              SOC_PORT_BITMAP(unit,axp)
#define PBMP_MXQ_ALL(unit)              SOC_PORT_BITMAP(unit,mxq)
#define PBMP_XG_ALL(unit)               SOC_PORT_BITMAP(unit,xg)
#define PBMP_XQ_ALL(unit)               SOC_PORT_BITMAP(unit,xq)
#define PBMP_HYPLITE_ALL(unit)          SOC_PORT_BITMAP(unit,hyplite)
#define PBMP_SPI_ALL(unit)              SOC_PORT_BITMAP(unit,spi)
#define PBMP_SPI_SUBPORT_ALL(unit)      SOC_PORT_BITMAP(unit,spi_subport)
#define PBMP_SCI_ALL(unit)              SOC_PORT_BITMAP(unit,sci)
#define PBMP_SFI_ALL(unit)              SOC_PORT_BITMAP(unit,sfi)
#define PBMP_REQ_ALL(unit)              SOC_PORT_BITMAP(unit,req)
#define PBMP_FAB_ALL(unit)              SOC_PORT_BITMAP(unit,fab)
#define PBMP_E_ALL(unit)                SOC_PORT_BITMAP(unit,ether)
#define PBMP_LP_ALL(unit)               SOC_PORT_BITMAP(unit,lp)
#define PBMP_SUBTAG_ALL(unit)           SOC_PORT_BITMAP(unit,subtag)
#define PBMP_TDM_ALL(unit)              SOC_PORT_BITMAP(unit,tdm)
#define PBMP_RCY_ALL(unit)              SOC_PORT_BITMAP(unit,rcy)
#define PBMP_QSGMII_ALL(unit)           SOC_PORT_BITMAP(unit,qsgmii)
#define PBMP_PORT_ALL(unit)             SOC_PORT_BITMAP(unit,port)
#define PBMP_INTP_ALL(unit)             SOC_PORT_BITMAP(unit, intp)
#define PBMP_LBG_ALL(unit)              SOC_PORT_BITMAP(unit, lbgport)
#define PBMP_CMIC(unit)                 SOC_INFO(unit).cmic_bitmap
#define PBMP_LB(unit)                   SOC_INFO(unit).lb_pbm
#define PBMP_MMU(unit)                  SOC_INFO(unit).mmu_pbm
#define PBMP_EQ(unit)                   SOC_INFO(unit).eq_pbm
#define PBMP_MANAGEMENT(unit)           SOC_INFO(unit).management_pbm
#define PBMP_OVERSUB(unit)              SOC_INFO(unit).oversub_pbm
#define PBMP_XPIPE(unit)                SOC_INFO(unit).xpipe_pbm
#define PBMP_YPIPE(unit)                SOC_INFO(unit).ypipe_pbm
#define PBMP_TDM(unit)                  SOC_INFO(unit).tdm_pbm
#define PBMP_RCY(unit)                  SOC_INFO(unit).rcy_pbm
#define PBMP_SPI(unit)                  SOC_INFO(unit).spi_bitmap
#define PBMP_PP_ALL(unit)               SOC_PORT_BITMAP(unit,pp)
#define PBMP_ALL(unit)                  SOC_PORT_BITMAP(unit,all)
#define PBMP_EXT_MEM(unit)              SOC_INFO(unit).pbm_ext_mem
#define PBMP_PIPE(unit,pipe)            SOC_INFO(unit).pipe_pbm[pipe]

/* use PORT_MIN/_MAX to be more efficient than PBMP_ITER */
#define _SOC_PBMP_ITER(_u,_pt,_p)       \
        for ((_p) = SOC_PORT_MIN(_u,_pt); \
             (_p) >= 0 && (_p) <= SOC_PORT_MAX(_u,_pt); \
             (_p)++) \
                if (_SHR_PBMP_MEMBER(SOC_PORT_BITMAP(_u,_pt), (_p)))
#define PBMP_FE_ITER(_u, _p)            _SOC_PBMP_ITER(_u,fe,_p)
#define PBMP_GE_ITER(_u, _p)            _SOC_PBMP_ITER(_u,ge,_p)
#define PBMP_LLID_ITER(_u,_p)           _SOC_PBMP_ITER(_u,llid,_p)
#define PBMP_PON_ITER(_u,_p)            _SOC_PBMP_ITER(_u,pon,_p)
#define PBMP_XE_ITER(_u, _p)            _SOC_PBMP_ITER(_u,xe,_p)
#define PBMP_CE_ITER(_u, _p)            _SOC_PBMP_ITER(_u,ce,_p)
#define PBMP_HG_ITER(_u, _p)            _SOC_PBMP_ITER(_u,hg,_p)
#define PBMP_IL_ITER(_u, _p)            _SOC_PBMP_ITER(_u,il,_p)
#define PBMP_SCH_ITER(_u, _p)           _SOC_PBMP_ITER(_u,sch,_p)
#define PBMP_HG_SUBPORT_ITER(_u, _p)    _SOC_PBMP_ITER(_u,hg_subport,_p)
#define PBMP_HL_ITER(_u, _p)            _SOC_PBMP_ITER(_u,hl,_p)
#define PBMP_ST_ITER(_u, _p)            _SOC_PBMP_ITER(_u,st,_p)
#define PBMP_GX_ITER(_u, _p)            _SOC_PBMP_ITER(_u,gx,_p)
#define PBMP_XL_ITER(_u, _p)            _SOC_PBMP_ITER(_u,xl,_p)
#define PBMP_XLB0_ITER(_u, _p)          _SOC_PBMP_ITER(_u,xlb0,_p)
#define PBMP_XT_ITER(_u, _p)            _SOC_PBMP_ITER(_u,xt,_p)
#define PBMP_CL_ITER(_u, _p)            _SOC_PBMP_ITER(_u,cl,_p)
#define PBMP_CLG2_ITER(_u, _p)          _SOC_PBMP_ITER(_u,clg2,_p)
#define PBMP_C_ITER(_u, _p)             _SOC_PBMP_ITER(_u,c,_p)
#define PBMP_LB_ITER(_u, _p)            _SOC_PBMP_ITER(_u,lbport,_p)
#define PBMP_RDB_ITER(_u, _p)           _SOC_PBMP_ITER(_u,rdbport,_p)
#define PBMP_AXP_ITER(_u, _p)           _SOC_PBMP_ITER(_u,axp,_p)
#define PBMP_MXQ_ITER(_u, _p)           _SOC_PBMP_ITER(_u,mxq,_p)
#define PBMP_XG_ITER(_u, _p)            _SOC_PBMP_ITER(_u,xg,_p)
#define PBMP_XQ_ITER(_u, _p)            _SOC_PBMP_ITER(_u,xq,_p)
#define PBMP_HYPLITE_ITER(_u, _p)       _SOC_PBMP_ITER(_u,hyplite,_p)
#define PBMP_SPI_ITER(_u, _p)           _SOC_PBMP_ITER(_u,spi,_p)
#define PBMP_SPI_SUBPORT_ITER(_u, _p)   _SOC_PBMP_ITER(_u,spi_subport,_p)
#define PBMP_SCI_ITER(_u, _p)           _SOC_PBMP_ITER(_u,sci,_p)
#define PBMP_SFI_ITER(_u, _p)           _SOC_PBMP_ITER(_u,sfi,_p)
#define PBMP_REQ_ITER(_u, _p)           _SOC_PBMP_ITER(_u,req,_p)
#define PBMP_E_ITER(_u, _p)             _SOC_PBMP_ITER(_u,ether,_p)
#define PBMP_LP_ITER(_u, _p)            _SOC_PBMP_ITER(_u,lp,_p)
#define PBMP_SUBTAG_ITER(_u, _p)        _SOC_PBMP_ITER(_u,subtag,_p)
#define PBMP_TDM_ITER(_u, _p)           _SOC_PBMP_ITER(_u,tdm,_p)
#define PBMP_PORT_ITER(_u, _p)          _SOC_PBMP_ITER(_u,port,_p)
#define PBMP_PP_ALL_ITER(_u, _p)        _SOC_PBMP_ITER(_u,pp,_p)
#define PBMP_ALL_ITER(_u, _p)           _SOC_PBMP_ITER(_u,all,_p)
#define PMBP_INTP_ITER(_u, _p)          _SOC_PBMP_ITER(_u,intp,_p)

#define IS_OLP_PORT(unit, port)         \
        (port == OLP_PORT(unit))
#define IS_ERP_PORT(unit, port)         \
        (port == ERP_PORT(unit))
#define IS_RCPU_PORT(unit, port)         \
        (port == RCPU_PORT(unit))
#define IS_S_PORT(unit, port)           \
        (SOC_PBMP_MEMBER(SOC_INFO(unit).s_pbm, port))
#define IS_GMII_PORT(unit, port)        \
        (SOC_PBMP_MEMBER(SOC_INFO(unit).gmii_pbm, port))
#define IS_FE_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_FE_ALL(unit), port))
#define IS_GE_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_GE_ALL(unit), port))
#define IS_LLID_PORT(unit,port)         \
        (SOC_PBMP_MEMBER(PBMP_LLID_ALL(unit), port))
#define IS_PON_PORT(unit,port)          \
        (SOC_PBMP_MEMBER(PBMP_PON_ALL(unit), port))
#define IS_XE_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XE_ALL(unit), port))
#define IS_CE_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_CE_ALL(unit), port))
#define IS_HG_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_HG_ALL(unit), port))
#define IS_IL_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_IL_ALL(unit), port))
#define IS_SCH_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_SCH_ALL(unit), port))
#define IS_HG_SUBPORT_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_HG_SUBPORT_ALL(unit), port))
#define IS_HL_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_HL_ALL(unit), port))
#define IS_ST_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_ST_ALL(unit), port))
#define IS_GX_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_GX_ALL(unit), port))
#define IS_XL_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XL_ALL(unit), port))
#define IS_XLB0_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XLB0_ALL(unit), port))
#define IS_CL_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_CL_ALL(unit), port))
#define IS_CLG2_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_CLG2_ALL(unit), port))
#define IS_CXX_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_CXX_ALL(unit), port))
#define IS_C_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_C_ALL(unit), port))
#define IS_AXP_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_AXP_ALL(unit), port))
#define IS_XT_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XT_ALL(unit), port))
#define IS_XW_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XW_ALL(unit), port))
#define IS_MXQ_PORT(unit,port)           \
            (SOC_PBMP_MEMBER(PBMP_MXQ_ALL(unit), port))
#define IS_XG_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XG_ALL(unit), port))
#define IS_XQ_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_XQ_ALL(unit), port))
#define IS_HYPLITE_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_HYPLITE_ALL(unit), port))
#define IS_SPI_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_SPI_ALL(unit), port))
#define IS_SPI_SUBPORT_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_SPI_SUBPORT_ALL(unit), port))
#define IS_SCI_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_SCI_ALL(unit), port))
#define IS_SFI_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_SFI_ALL(unit), port))
#define IS_REQ_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_REQ_ALL(unit), port))
#define IS_E_PORT(unit,port)            \
        (SOC_PBMP_MEMBER(PBMP_E_ALL(unit), port))
#define IS_LP_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_LP_ALL(unit), port))
#define IS_SUBTAG_PORT(unit, port)      \
        (SOC_PBMP_MEMBER(PBMP_SUBTAG_ALL(unit), port))
#define IS_RCY_PORT(unit,port)            \
        (SOC_PBMP_MEMBER(PBMP_RCY_ALL(unit), port))
#define IS_TDM_PORT(unit,port)            \
        (SOC_PBMP_MEMBER(PBMP_TDM_ALL(unit), port))
#define IS_PORT(unit,port)              \
        (SOC_BLOCK_IN_LIST(&(SOC_PORT_TYPE(unit, port)), SOC_BLK_NET))
#define IS_LB_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_LB(unit), port))
#define IS_RDB_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_RDB_ALL(unit), port))
#define IS_MMU_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_MMU(unit), port))
#define IS_EXT_MEM_PORT(unit,port)           \
            (SOC_PBMP_MEMBER(PBMP_EXT_MEM(unit), port))
#define IS_CPU_PORT(unit,port)          \
        (SOC_BLOCK_IN_LIST(&(SOC_PORT_TYPE(unit, port)), SOC_BLK_CPU))
#define IS_SPI_BLK(unit,port)      \
    (SOC_PORT_TYPE(unit, port) == SOC_BLK_SPI)
#define IS_EXP_PORT(unit,port)      \
    (SOC_PORT_TYPE(unit, port) == SOC_BLK_EXP)
#define IS_ALL_PORT(unit,port)          \
        (SOC_PORT_TYPE(unit, port) != 0)
#define IS_INTP_PORT(unit, port)        \
        (SOC_PBMP_MEMBER(PBMP_INTP_ALL(unit), port))
#define IS_QSGMII_PORT(unit,port)           \
        (SOC_PBMP_MEMBER(PBMP_QSGMII_ALL(unit), port))
/* Stack related macros */

#define SOC_PBMP_STACK_CURRENT(unit)    \
    (SOC_PERSIST(unit)->stack_ports_current)
#define SOC_PBMP_STACK_INACTIVE(unit)   \
    (SOC_PERSIST(unit)->stack_ports_inactive)
#define SOC_PBMP_STACK_PREVIOUS(unit)   \
    (SOC_PERSIST(unit)->stack_ports_previous)

#define SOC_PBMP_STACK_ACTIVE_GET(unit, active) do { \
        SOC_PBMP_ASSIGN(active, SOC_PBMP_STACK_CURRENT(unit)); \
        SOC_PBMP_REMOVE(active, SOC_PBMP_STACK_INACTIVE(unit)); \
    } while (0)

#define SOC_IS_STACK_PORT(unit, port) \
    SOC_PBMP_MEMBER(SOC_PBMP_STACK_CURRENT(unit), port)

#define SOC_IS_INACTIVE_STACK_PORT(unit, port) \
    SOC_PBMP_MEMBER(SOC_PBMP_STACK_INACTIVE(unit), port)

/* SL Mode set/get macros */
#define SOC_SL_MODE(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_SL_MODE) != 0)

#define SOC_SL_MODE_SET(unit, sl_mode) \
    if (sl_mode) SOC_CONTROL(unit)->soc_flags |= SOC_F_SL_MODE; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_SL_MODE

/* KNET Mode set/get macros */
#define SOC_KNET_MODE(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_KNET_MODE) != 0)

#define SOC_KNET_MODE_SET(unit, knet_mode) \
    if (knet_mode) SOC_CONTROL(unit)->soc_flags |= SOC_F_KNET_MODE; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_KNET_MODE

/* URPF on/off set/get macros */
#define SOC_URPF_STATUS_GET(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_URPF_ENABLED) != 0)

#define SOC_URPF_STATUS_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_URPF_ENABLED; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_URPF_ENABLED

#define SOC_L3_DEFIP_INDEX_REMAP_GET(unit) \
    SOC_CONTROL(unit)->l3_defip_index_remap

#define SOC_L3_DEFIP_MAX_TCAMS_GET(unit) \
    SOC_CONTROL(unit)->l3_defip_max_tcams

#define SOC_L3_DEFIP_TCAM_DEPTH_GET(unit) \
    SOC_CONTROL(unit)->l3_defip_tcam_size

#define SOC_L3_DEFIP_LOG_TO_PHY_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_phy_index

#define SOC_L3_DEFIP_PHY_TO_LOG_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_log_index

#define SOC_L3_DEFIP_URPF_LOG_TO_PHY_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_urpf_phy_index

#define SOC_L3_DEFIP_URPF_PHY_TO_LOG_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_urpf_log_index

#define SOC_L3_DEFIP_LOG_TO_PHY_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_phy_index[index]

#define SOC_L3_DEFIP_PHY_TO_LOG_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_log_index[index]

#define SOC_L3_DEFIP_URPF_LOG_TO_PHY_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_urpf_phy_index[index]

#define SOC_L3_DEFIP_URPF_PHY_TO_LOG_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_urpf_log_index[index]

#define SOC_L3_DEFIP_PAIR_LOG_TO_PHY_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_phy_index

#define SOC_L3_DEFIP_PAIR_PHY_TO_LOG_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_log_index

#define SOC_L3_DEFIP_PAIR_URPF_LOG_TO_PHY_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_urpf_phy_index

#define SOC_L3_DEFIP_PAIR_URPF_PHY_TO_LOG_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_urpf_log_index

#define SOC_L3_DEFIP_PAIR_LOG_TO_PHY_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_phy_index[index]

#define SOC_L3_DEFIP_PAIR_PHY_TO_LOG_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_log_index[index]

#define SOC_L3_DEFIP_PAIR_URPF_LOG_TO_PHY_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_urpf_phy_index[index]

#define SOC_L3_DEFIP_PAIR_URPF_PHY_TO_LOG_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_urpf_log_index[index]

#define SOC_L3_DEFIP_ALPM_URPF_LOG_TO_PHY_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_alpm_urpf_phy_index

#define SOC_L3_DEFIP_ALPM_URPF_PHY_TO_LOG_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_alpm_urpf_log_index
#define SOC_L3_DEFIP_ALPM_URPF_LOG_TO_PHY_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_alpm_urpf_phy_index[index]

#define SOC_L3_DEFIP_ALPM_URPF_PHY_TO_LOG_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_tcam_alpm_urpf_log_index[index]

#define SOC_L3_DEFIP_PAIR_ALPM_URPF_LOG_TO_PHY_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_alpm_urpf_phy_index

#define SOC_L3_DEFIP_PAIR_ALPM_URPF_PHY_TO_LOG_ARRAY(unit) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_alpm_urpf_log_index

#define SOC_L3_DEFIP_PAIR_ALPM_URPF_LOG_TO_PHY_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_alpm_urpf_phy_index[index]

#define SOC_L3_DEFIP_PAIR_ALPM_URPF_PHY_TO_LOG_INDEX(unit, index) \
    SOC_CONTROL(unit)->defip_index_table->defip_pair_tcam_alpm_urpf_log_index[index]

#define SOC_L3_DEFIP_INDEX_INIT(unit) \
    SOC_CONTROL(unit)->defip_index_table

#define SOC_L3_DEFIP_MAX_128B_ENTRIES(unit) \
    SOC_CONTROL(unit)->l3_defip_max_128b_entries

#define SOC_L3_DEFIP_SCACHE_HANDLE(unit) \
    SOC_CONTROL(unit)->l3_defip_scache_ptr

#define SOC_FP_TCAM_SCACHE_HANDLE(unit) \
    SOC_CONTROL(unit)->fp_tcam_scache_ptr

/* URPF on/off set/get macros */
#define SOC_L2X_GROUP_ENABLE_GET(_unit_) (SOC_CONTROL(_unit_)->l2x_group_enable)
#define SOC_L2X_GROUP_ENABLE_SET(_unit_, _status_) \
           (SOC_CONTROL(_unit_)->l2x_group_enable) = (_status_) ?  TRUE : FALSE


/* Device should use memory dma for memory clear ops */
#define SOC_MEM_CLEAR_USE_DMA(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_MEM_CLEAR_USE_DMA) != 0)

#define SOC_MEM_CLEAR_USE_DMA_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_MEM_CLEAR_USE_DMA; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_MEM_CLEAR_USE_DMA

/* Device should use hw pipe clear for memory clear ops */
#define SOC_MEM_CLEAR_HW_ACC(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_MEM_CLEAR_HW_ACC) != 0)

#define SOC_MEM_CLEAR_HW_ACC_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_MEM_CLEAR_HW_ACC; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_MEM_CLEAR_HW_ACC

/* Device should use hw pipe clear for memory clear ops */
#define SOC_MEM_CLEAR_HW_ACC(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_MEM_CLEAR_HW_ACC) != 0)

#define SOC_MEM_CLEAR_HW_ACC_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_MEM_CLEAR_HW_ACC; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_MEM_CLEAR_HW_ACC

/* Device should not use mem cache in mem test mode */
#define SOC_MEM_TEST_SKIP_CACHE(unit) \
    (SOC_CONTROL(unit)->skip_cache_use)

#define SOC_MEM_TEST_SKIP_CACHE_SET(unit, enable) \
    (SOC_CONTROL(unit)->skip_cache_use = enable)

#define SOC_MEM_FORCE_READ_THROUGH(unit) \
    (SOC_CONTROL(unit)->force_read_through)

#define SOC_MEM_FORCE_READ_THROUGH_SET(unit, enable) \
    (SOC_CONTROL(unit)->force_read_through = enable)

#define SOC_MEM_CACHE_COHERENCY_CHECK(unit) \
    (SOC_CONTROL(unit)->cache_coherency_chk)

#define SOC_MEM_CACHE_COHERENCY_CHECK_SET(unit, enable) \
    (SOC_CONTROL(unit)->cache_coherency_chk = enable)

#define SOC_SER_SUPPORT(unit) \
    (SOC_CONTROL(unit)->parity_enable)

#define SOC_SER_CORRECTION_SUPPORT(unit) \
    (SOC_CONTROL(unit)->parity_correction)

#define SOC_SER_COUNTER_CORRECTION(unit) \
    (SOC_CONTROL(unit)->parity_counter_clear)

#define SOC_REG_RETURN_SER_ERROR(unit) \
    (SOC_CONTROL(unit)->return_reg_error)

#define SOC_REG_RETURN_SER_ERROR_SET(unit, enable) \
    (SOC_CONTROL(unit)->return_reg_error = enable)

/* Device should use gport for source/destination notation */
#define SOC_USE_GPORT(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_GPORT) != 0)

#define SOC_USE_GPORT_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_GPORT; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_GPORT

/* Maximum vrf id for the device set/get macro */
#define SOC_VRF_MAX(unit)  SOC_CONTROL(unit)->max_vrf_id
#define SOC_VRF_MAX_SET(unit, value) \
            SOC_CONTROL(unit)->max_vrf_id = value

/* Maximum address class for the device set/get macro */
#define SOC_ADDR_CLASS_MAX(unit)  SOC_CONTROL(unit)->max_address_class
#define SOC_ADDR_CLASS_MAX_SET(unit, value) \
            SOC_CONTROL(unit)->max_address_class = value

/* Maximum overlaid address class for the device set/get macro */
#define SOC_OVERLAID_ADDR_CLASS_MAX(unit) \
            SOC_CONTROL(unit)->max_overlaid_address_class
#define SOC_OVERLAID_ADDR_CLASS_MAX_SET(unit, value) \
            SOC_CONTROL(unit)->max_overlaid_address_class = value

/* Maximum extended address class for the device set/get macro */
#define SOC_EXT_ADDR_CLASS_MAX(unit) \
            SOC_CONTROL(unit)->max_extended_address_class
#define SOC_EXT_ADDR_CLASS_MAX_SET(unit, value) \
            SOC_CONTROL(unit)->max_extended_address_class = value

/* Maximum interface class for the device set/get macro */
#define SOC_INTF_CLASS_MAX(unit)  SOC_CONTROL(unit)->max_interface_class
#define SOC_INTF_CLASS_MAX_SET(unit, value) \
            SOC_CONTROL(unit)->max_interface_class = value


/* Device memory clear chunk size set/get macros */
#define SOC_MEM_CLEAR_CHUNK_SIZE_GET(unit)  SOC_CONTROL(unit)->mem_clear_chunk_size
#define SOC_MEM_CLEAR_CHUNK_SIZE_SET(unit, value) \
            SOC_CONTROL(unit)->mem_clear_chunk_size = value

/* IPMC replication sharing set/get macros */
#define SOC_IPMCREPLSHR_GET(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_IPMCREPLSHR) != 0)

#define SOC_IPMCREPLSHR_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_IPMCREPLSHR; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_IPMCREPLSHR

/* Device should use gport for source/destination notation */
#define SOC_REMOTE_ENCAP(unit) \
    ((SOC_CONTROL(unit)->soc_flags & SOC_F_REMOTE_ENCAP) != 0)

#define SOC_REMOTE_ENCAP_SET(unit, status) \
    if (status) SOC_CONTROL(unit)->soc_flags |= SOC_F_REMOTE_ENCAP; \
    else SOC_CONTROL(unit)->soc_flags &= ~SOC_F_REMOTE_ENCAP

/* Dual hash global maximum recursion level */
#define SOC_DUAL_HASH_MOVE_MAX(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth
/* L2X dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_L2X(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_l2x
/* MPLS dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_MPLS(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_mpls
/* VLAN dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_VLAN(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_vlan
/* EGRESS VLAN dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_EGRESS_VLAN(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_egress_vlan
#if defined(INCLUDE_L3)
/* L3X dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_L3X(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_l3x
#endif
#if defined(BCM_TRIUMPH3_SUPPORT)
/* L3X dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_WLAN_PORT(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_wlan_port
#define SOC_DUAL_HASH_MOVE_MAX_WLAN_CLIENT(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_wlan_client
/* FT_SESSION*m dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MAX_FT_SESSION_IPV4(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_ft_session_ipv4
#define SOC_DUAL_HASH_MAX_FT_SESSION_IPV6(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_ft_session_ipv6
#endif
/* ING_VP_VLAN_MEMBERSHIP dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_ING_VP_VLAN_MEMBER(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_ing_vp_vlan_member
/* EGR_VP_VLAN_MEMBERSHIP dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_EGR_VP_VLAN_MEMBER(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_egr_vp_vlan_member
/* ING_DNAT_ADDRESS_TYPE dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_ING_DNAT_ADDRESS_TYPE(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_ing_dnat_address_type
/* L2_ENDPOINT_ID dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_L2_ENDPOINT_ID(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_l2_endpoint_id
/* ENDPOINT_QUEUE_MAP dual hash tables specific recursion level */
#define SOC_DUAL_HASH_MOVE_MAX_ENDPOINT_QUEUE_MAP(unit) \
    SOC_CONTROL(unit)->dual_hash_recurse_depth_endpoint_queue_map
/* Multi hash global maximum recursion level */
#define SOC_MULTI_HASH_MOVE_MAX(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth
/* L2 multi hash tables specific recursion level */
#define SOC_MULTI_HASH_MOVE_MAX_L2(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth_l2
/* L3 multi hash tables specific recursion level */
#define SOC_MULTI_HASH_MOVE_MAX_L3(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth_l3
/* MPLS multi hash tables specific recursion level */
#define SOC_MULTI_HASH_MOVE_MAX_MPLS(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth_mpls
/* VLAN multi hash tables specific recursion level */
#define SOC_MULTI_HASH_MOVE_MAX_VLAN(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth_vlan
/* EGRESS VLAN multi hash tables specific recursion level */
#define SOC_MULTI_HASH_MOVE_MAX_EGRESS_VLAN(unit) \
    SOC_CONTROL(unit)->multi_hash_recurse_depth_egress_vlan

/* Multi hash tables specific moving recursion algorithm */
#define SOC_MULTI_HASH_MOVE_ALGORITHM(unit) \
    SOC_CONTROL(unit)->multi_hash_move_algorithm

#define SOC_MCAST_ADD_ALL_ROUTER_PORTS(unit) \
    SOC_CONTROL(unit)->mcast_add_all_router_ports

#ifdef BCM_CB_ABORT_ON_ERR
#define SOC_CB_ABORT_ON_ERR(unit) \
    SOC_CONTROL(unit)->cb_abort_on_err
#endif

/* Switching logic bypass mode */
#define SOC_SWITCH_BYPASS_MODE_NONE             0
#define SOC_SWITCH_BYPASS_MODE_L3_ONLY          1
#define SOC_SWITCH_BYPASS_MODE_L3_AND_FP        2
#ifdef BCM_TOMAHAWK_SUPPORT
/* modes redefined for TH SKUs, values reused */
#define SOC_SWITCH_BYPASS_MODE_BALANCED         1
#define SOC_SWITCH_BYPASS_MODE_LOW              2
#endif

#define SOC_SWITCH_BYPASS_MODE(unit) \
    SOC_CONTROL(unit)->switch_bypass_mode

/* Double tag mode when it is a device-wide property */
#define SOC_DT_MODE(unit) \
    SOC_CONTROL(unit)->dt_mode

#define SOC_MAC_LOW_POWER_ENABLED(unit) \
    SOC_CONTROL(unit)->mac_low_power_enabled

#define SOC_AUTO_MAC_LOW_POWER(unit) \
    SOC_CONTROL(unit)->auto_mac_low_power

#define MAC_LOW_POWER_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->mac_low_power_mutex, sal_mutex_FOREVER)
#define MAC_LOW_POWER_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->mac_low_power_mutex)

#if defined(BCM_53125) || defined(BCM_53128) || defined(BCM_POLAR_SUPPORT) || \
    defined(BCM_STARFIGHTER3_SUPPORT)
/* Lock the internal micro control unit to prevent the PHY register access */
#define INT_MCU_LOCK(unit) \
        if (soc_feature(unit, soc_feature_int_cpu_arbiter) && \
            (SOC_CONTROL(unit)->int_cpu_enabled))  \
        DRV_DEV_PROP_SET(unit, DRV_DEV_PROP_RESOURCE_ARBITER_REQ, TRUE)
#define INT_MCU_UNLOCK(unit) \
        if (soc_feature(unit, soc_feature_int_cpu_arbiter) && \
            (SOC_CONTROL(unit)->int_cpu_enabled))  \
        DRV_DEV_PROP_SET(unit, DRV_DEV_PROP_RESOURCE_ARBITER_REQ, FALSE)
#else
#define INT_MCU_LOCK(unit)
#define INT_MCU_UNLOCK(unit)
#endif /* BCM_53125 || BCM_53128 || BCM_POLAR_SUPPORT ||
        * BCM_STARFIGHTER3_SUPPORT
        */

/*
 * LMD enabled ports. 2.5 Gbps stacking support
 */
#define SOC_LMD_PBM(unit) SOC_INFO(unit).lmd_pbm
#define SOC_LMD_ENABLED_PORT_SET(unit, port)     \
    SOC_PBMP_PORT_SET(SOC_INFO(unit).lmd_pbm, port)
#define IS_LMD_ENABLED_PORT(unit, port)          \
    SOC_PBMP_MEMBER(SOC_INFO(unit).lmd_pbm, port)

/*
 * Ports using HiGig2 encapsulation.
 * This is a cached value used for bcm_tx performance reasons.
 */
#define SOC_HG2_PBM(unit) SOC_INFO(unit).hg2_pbm
#define SOC_HG2_ENABLED_PORT_ADD(unit, port)     \
    SOC_PBMP_PORT_ADD(SOC_INFO(unit).hg2_pbm, port)
#define SOC_HG2_ENABLED_PORT_REMOVE(unit, port)     \
    SOC_PBMP_PORT_REMOVE(SOC_INFO(unit).hg2_pbm, port)
#define IS_HG2_ENABLED_PORT(unit, port)          \
    SOC_PBMP_MEMBER(SOC_INFO(unit).hg2_pbm, port)

extern int SOC_BLOCK_IS_TYPE(int unit, int blk_idx, int *list);
/* iterate over enabled blocks of type */
#define SOC_BLOCKS_ITER(unit, var, list) \
        for ((var) = 0; SOC_BLOCK_INFO(unit, var).type >= 0; (var)++) \
                if (SOC_INFO(unit).block_valid[var] && \
                    SOC_BLOCK_IS_TYPE(unit, var, list))

#define SOC_BLOCK_IS_CMP(unit, blk, val)\
    (SOC_BLOCK_TYPE(unit, blk) == val)
extern int SOC_BLOCK_IS_COMPOSITE(int unit, int blk_idx, int type);
/* iterate over enabled block of type(specific or composite) */
#define SOC_BLOCK_ITER(unit, var, val) \
        for ((var) = 0; SOC_BLOCK_INFO(unit, var).type >= 0; (var)++) \
                if (SOC_INFO(unit).block_valid[var] && \
                    (SOC_BLOCK_IS_CMP(unit, var, val) || \
                     SOC_BLOCK_IS_COMPOSITE(unit, var, val)))

#define SOC_BLOCK_ITER_ALL(unit, var, val) \
        for ((var) = 0; SOC_BLOCK_INFO(unit, var).type >= 0; (var)++) \
                if (SOC_BLOCK_IS_CMP(unit, var, val) || \
                     SOC_BLOCK_IS_COMPOSITE(unit, var, val))

#define SOC_MEM_BLOCK_MIN(unit, mem)    SOC_MEM_INFO(unit, mem).minblock
#define SOC_MEM_BLOCK_MAX(unit, mem)    (soc_mem_is_unified(unit, mem) ? \
                        SOC_MEM_INFO(unit, mem).minblock : \
                        SOC_MEM_INFO(unit, mem).maxblock)
#define SOC_MEM_BLOCK_ANY(unit, mem)    (SOC_INFO(unit).mem_block_any[mem])

#ifdef BCM_ROBO_SUPPORT
#define SOC_MEM_BLOCK_VALID(unit, mem, blk)     \
                        ((blk) >= 0 && \
                         (blk) < (SOC_IS_ROBO(unit) ? SOC_ROBO_MAX_NUM_BLKS \
                         : SOC_MAX_NUM_BLKS) && \
                         (SOC_IS_ROBO(unit) || (((blk)>=32)?(SOC_MEM_INFO(unit, mem).blocks_hi & (1<<((blk)&0x1F))):(SOC_MEM_INFO(unit, mem).blocks & (1<<(blk)))))&& \
                         (SOC_INFO(unit).block_valid[blk]))
#else /* !BCM_ROBO_SUPPORT */
#define SOC_MEM_BLOCK_VALID(unit, mem, blk)     \
                        ((blk) >= 0 && \
                         (blk) < SOC_MAX_NUM_BLKS && \
                         (SOC_MAX_NUM_BLKS > 64 || (((blk)>=32)?(SOC_MEM_INFO(unit, mem).blocks_hi & (1<<((blk)&0x1F))):(SOC_MEM_INFO(unit, mem).blocks & (1<<(blk)))))&& \
                         (SOC_INFO(unit).block_valid[blk]))

#endif /* BCM_ROBO_SUPPORT */

#ifdef BCM_ROBO_SUPPORT
#define SOC_MEM_BLOCK_ITER(unit, mem, var) \
        for ((var) = SOC_MEM_BLOCK_MIN(unit, mem); \
                (var) <= SOC_MEM_BLOCK_MAX(unit, mem); \
                (var)++) \
            if ((SOC_IS_ROBO(unit) || (((var)>=32)?(SOC_MEM_INFO(unit, mem).blocks_hi & (1 << ((var)&0x1F))):(SOC_MEM_INFO(unit, mem).blocks & (1 << (var))))) && \
                           SOC_INFO(unit).block_valid[var])
#else /* !BCM_ROBO_SUPPORT */
#define SOC_MEM_BLOCK_ITER(unit, mem, var) \
          for ((var) = SOC_MEM_BLOCK_MIN(unit, mem); \
                  (var) <= SOC_MEM_BLOCK_MAX(unit, mem); \
                  (var)++) \
            if ((SOC_MAX_NUM_BLKS > 64 || (((var)>=32)?(SOC_MEM_INFO(unit, mem).blocks_hi & (1 << ((var)&0x1F))):(SOC_MEM_INFO(unit, mem).blocks & (1 << (var))))) && \
                         SOC_INFO(unit).block_valid[var])
#endif /* BCM_ROBO_SUPPORT */


/* Default dcb/dma values */
#define SOC_DEFAULT_DMA_SRCMOD_GET(_u)      SOC_PERSIST(_u)->dcb_srcmod
#define SOC_DEFAULT_DMA_SRCPORT_GET(_u)     SOC_PERSIST(_u)->dcb_srcport
#define SOC_DEFAULT_DMA_PFM_GET(_u)         SOC_PERSIST(_u)->dcb_pfm
#define SOC_DEFAULT_DMA_SRCMOD_SET(_u, _v)  SOC_PERSIST(_u)->dcb_srcmod = (_v)
#define SOC_DEFAULT_DMA_SRCPORT_SET(_u, _v) SOC_PERSIST(_u)->dcb_srcport = (_v)
#define SOC_DEFAULT_DMA_PFM_SET(_u, _v)     SOC_PERSIST(_u)->dcb_pfm = (_v)

/* Features cache */
#define SOC_FEATURE_GET(unit, feat)     \
        SHR_BITGET(SOC_CONTROL(unit)->features, feat)
#define SOC_FEATURE_SET(unit, feat)     \
        SHR_BITSET(SOC_CONTROL(unit)->features, feat)
#define SOC_FEATURE_CLEAR(unit, feat)     \
        SHR_BITCLR(SOC_CONTROL(unit)->features, feat)

/*
 * Various mutex controls
 */
#define SCHAN_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->schanMutex, sal_mutex_FOREVER)
#define SCHAN_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->schanMutex)

#ifdef BCM_CMICM_SUPPORT
#define FSCHAN_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->fschanMutex, sal_mutex_FOREVER)
#define FSCHAN_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->fschanMutex)
#endif

#define MIIM_LOCK(unit) do { } while(0)       
#define MIIM_UNLOCK(unit) do { } while(0)       
        

#define SBUS_DMA_LOCK(unit, cmc, ch) \
        sal_mutex_take(SOC_CONTROL(unit)->sbusDmaMutexs[(cmc)][(ch)], sal_mutex_FOREVER)
#define SBUS_DMA_UNLOCK(unit, cmc, ch) \
        sal_mutex_give(SOC_CONTROL(unit)->sbusDmaMutexs[(cmc)][(ch)])

#define TABLE_DMA_LOCK(unit) \
        SBUS_DMA_LOCK(unit, SOC_PCI_CMC(unit), soc->tdma_ch)
#define TABLE_DMA_UNLOCK(unit) \
        SBUS_DMA_UNLOCK(unit, SOC_PCI_CMC(unit), soc->tdma_ch)

#define TSLAM_DMA_LOCK(unit) \
        SBUS_DMA_LOCK(unit, SOC_PCI_CMC(unit), soc->tslam_ch)

#define TSLAM_DMA_UNLOCK(unit) \
        SBUS_DMA_UNLOCK(unit, SOC_PCI_CMC(unit), soc->tslam_ch)

#define SBUSDMA_DMA_INTR_WAIT(unit, cmc, ch, to ) \
        sal_sem_take(SOC_CONTROL(unit)->sbusDmaIntrs[(cmc)][(ch)], to)

#define TSLAM_DMA_INTR_WAIT(unit, to ) \
        SBUSDMA_DMA_INTR_WAIT(unit, SOC_PCI_CMC(unit), soc->tslam_ch, to)

#define TABLE_DMA_INTR_WAIT(unit, to ) \
        SBUSDMA_DMA_INTR_WAIT(unit, SOC_PCI_CMC(unit), soc->tdma_ch, to)

#ifdef BCM_CMICM_SUPPORT
#define CCM_DMA_LOCK(unit, cmc) \
        sal_mutex_take(SOC_CONTROL(unit)->ccmDmaMutex[cmc], sal_mutex_FOREVER)
#define CCM_DMA_UNLOCK(unit, cmc) \
        sal_mutex_give(SOC_CONTROL(unit)->ccmDmaMutex[cmc])
#endif

#ifdef BCM_SBUSDMA_SUPPORT
#define SOC_SBUSDMA_DM_LOCK(unit) \
        sal_mutex_take(SOC_SBUSDMA_DM_MUTEX(unit), sal_mutex_FOREVER)
#define SOC_SBUSDMA_DM_UNLOCK(unit) \
        sal_mutex_give(SOC_SBUSDMA_DM_MUTEX(unit))
#endif

#define SOC_CONTROL_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->socControlMutex, sal_mutex_FOREVER)
#define SOC_CONTROL_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->socControlMutex)

#define SOC_MEM_SER_INFO_LOCK(unit) \
            sal_mutex_take(SOC_CONTROL(unit)->mem_ser_info_lock, sal_mutex_FOREVER)
#define SOC_MEM_SER_INFO_UNLOCK(unit) \
            sal_mutex_give(SOC_CONTROL(unit)->mem_ser_info_lock)

#define COUNTER_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->counterMutex, sal_mutex_FOREVER)
#define COUNTER_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->counterMutex)
#define SOC_COUNTER_IF_ERROR_RETURN(op)         \
    do {                                        \
        int __rv__;                             \
        if (((__rv__ = (op)) < 0)) {            \
            COUNTER_UNLOCK(unit);               \
            _SHR_ERROR_TRACE(__rv__);           \
            return(__rv__);                     \
        }                                       \
    } while(0)


#define OVERRIDE_LOCK(unit) \
       sal_mutex_take(SOC_CONTROL(unit)->overrideMutex, sal_mutex_FOREVER)
#define OVERRIDE_UNLOCK(unit) \
       sal_mutex_give(SOC_CONTROL(unit)->overrideMutex)

#define SPI_LOCK \
    sal_mutex_take(spiMutex, sal_mutex_FOREVER)
#define SPI_UNLOCK \
    sal_mutex_give(spiMutex)

#define SOC_EGRESS_METERING_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->egressMeteringMutex, \
                       sal_mutex_FOREVER)
#define SOC_EGRESS_METERING_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->egressMeteringMutex)

#define SOC_LLS_SCHEDULER_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->llsMutex, \
                       sal_mutex_FOREVER)
#define SOC_LLS_SCHEDULER_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->llsMutex)

#define ARLDMA_SIZE_DEFAULT     1024    /* Size in 16-byte entries */

#define IP_ARBITER_LOCK(unit) \
    sal_mutex_take(SOC_CONTROL(unit)->ipArbiterMutex, sal_mutex_FOREVER)
#define IP_ARBITER_UNLOCK(unit) \
    sal_mutex_give(SOC_CONTROL(unit)->ipArbiterMutex)

#if defined(BCM_TRIUMPH3_SUPPORT)
#define SOC_ESM_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->esm_lock, sal_mutex_FOREVER)
#define SOC_ESM_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->esm_lock)
#endif

#if defined(BCM_TOMAHAWK_SUPPORT)
#define SOC_IDB_LOCK(unit) \
        sal_mutex_take(SOC_CONTROL(unit)->idb_lock, sal_mutex_FOREVER)
#define SOC_IDB_UNLOCK(unit) \
        sal_mutex_give(SOC_CONTROL(unit)->idb_lock)
#endif

/*
 * Define the default number of milliseconds before a BIST operation
 * times out and fails.  Different values apply if the SAL boot flag
 * BOOT_F_QUICKTURN or BOOT_F_PLISIM are set.
 */

#define BIST_TIMEOUT            (1  * SECOND_MSEC)
#define BIST_TIMEOUT_QT         (1  * MINUTE_MSEC)
#define BIST_TIMEOUT_PLI        (20 * MINUTE_MSEC)

typedef struct _soc_l3_defip_index_table_s {
    int         *defip_tcam_phy_index;  /* Array holding defip phyiscal indexes
                                         * for a given logical index */
    int         *defip_tcam_log_index;    /* Array holding defip logical indexes
                                           * for a  given physical index */
    int         *defip_tcam_urpf_phy_index; /* Array holding defip phyiscal
                                             * indexes for a given logical
                                             * index when urpf is enabled */
    int         *defip_tcam_urpf_log_index; /* Array holding defip logical
                                             * indexes for a  given physical
                                             * index when urpf is enabled */
    int         *defip_pair_tcam_phy_index;  /* Array holding defip_pair
                                              * phyiscal indexes for a given
                                              * logical index */
    int         *defip_pair_tcam_log_index;  /* Array holding defip_pair logical
                                              * indexes for a  given physical
                                              * index */
    int         *defip_pair_tcam_urpf_phy_index; /* Array holding defip_pair
                                                  * phyiscal indexes for a
                                                  * given logical index when
                                                  * urpf is enabled */
    int         *defip_pair_tcam_urpf_log_index; /* Array holding defip logical
                                                  * indexes for a  given
                                                  * physical index when urpf is
                                                  * enabled */
    int         *defip_tcam_alpm_urpf_phy_index; /* Array holding defip phyiscal
                                                  * indexes for a given logical
                                                  * index when alpm_tcam/parallel
                                                  * and urpf are both enabled */
    int         *defip_tcam_alpm_urpf_log_index; /* Array holding defip logical
                                                  * indexes for a given physical
                                                  * index when alpm_tcam/parallel
                                                  * and urpf are both enabled */
    int         *defip_pair_tcam_alpm_urpf_phy_index; /* Array holding defip phyiscal
                                                       * indexes for a given logical
                                                       * index when alpm_tcam/parallel
                                                       * and urpf are both enabled */
    int         *defip_pair_tcam_alpm_urpf_log_index; /* Array holding defip logical
                                                       * indexes for a given physical
                                                       * index when alpm_tcam/parallel
                                                       * and urpf are both enabled */
} _soc_l3_defip_index_table_t;

/*
 * Typedef: soc_stat_t
 * Purpose: Driver statistics counts (interrupts, errors, etc).
 */
typedef struct soc_stat_s {
    uint32      intr;           /* Total interrupt count */
    uint32      intr_sce;       /* S-Channel error interrupt count */
    uint32      intr_sc;        /* S-Channel interrupt count */
    uint32      intr_ls;        /* Link status interrupt count */
    uint32      intr_gbp;       /* GBP Full interrupt count */
    uint32      intr_pci_fe;    /* PCI Fatal Error interrupt count */
    uint32      intr_pci_pe;    /* PCI Parity Error interrupt count */
    uint32      intr_arl_d;     /* ARL message dropped interrupt count */
    uint32      intr_arl_m;     /* ARL message ready interrupt count */
    uint32      intr_arl_x;     /* ARL DMA xfer done interrupt count */
    uint32      intr_arl_0;     /* ARL DMA count=0 interrupt count */
    uint32      intr_i2c;       /* I2C controller interrupt count */
    uint32      intr_mii;       /* MII interrupt count */
    uint32      intr_stats;     /* Stats DMA interrupt count */
    uint32      intr_desc;      /* DMA desc done interrupt count */
    uint32      intr_chain;     /* DMA chain done interrupt count */
    uint32      intr_reload;    /* DMA reload done interrupt count */
    uint32      intr_mmu;       /* MMU status interrupt count */
    uint32      intr_tdma;      /* Table DMA complete interrupt count */
    uint32      intr_tslam;     /* Table SLAM DMA complete interrupt count */
    uint32      intr_ccmdma;    /* CCM DMA complete interrupt count */
    uint32      intr_sw;        /* Cross CPU S/W interrupts */
    uint32      intr_mem_cmd[3]; /* Memory command complete interrupt count */
    uint32      intr_chip_func[5]; /* Chip functional interrupt count */
    uint32      intr_fifo_dma[4];  /* Fifo-dma interrupt count */
    uint32      intr_block;     /* Block interrupt count */

    uint32      schan_op;       /* Number of S-Channel operations */
    uint32      mem_cmd_op;     /* Number of memory command operations */

    uint32      err_sdram;      /* SDRAM parity error count */
    uint32      err_cfap;       /* CFAP oversubscribed count */
    uint32      err_fcell;      /* Unexpected First cell count */
    uint32      err_sr;         /* MMU Soft Reset count */
    uint32      err_cellcrc;    /* CBP cell CRC count */
    uint32      err_cbphp;      /* CBP header parity count */
    uint32      err_npcell;     /* MMU cells not in packet count */
    uint32      err_mp;         /* Memory parity error count */
    uint32      err_pdlock;     /* PLL/DLL Lock loss count */
    uint32      err_cpcrc;      /* Cell pointer CRC error count */
    uint32      err_cdcrc;      /* Cell data CRC error count */
    uint32      err_fdcrc;      /* Frame data CRC error count */
    uint32      err_cpbcrc;     /* Cell pointer block CRC error count */
    uint32      err_multi;      /* Multiple error count */
    uint32      err_invalid;    /* Invalid schan error count */
    uint32      err_sc_tmo;     /* S-Channel operation timeout count */
    uint32      err_mii_tmo;    /* MII operation timeout count */
    uint32      err_mc_tmo;     /* Memory command operation timeout count */

    uint32      arl_msg_ins;    /* Count of ARL insert messages processed */
    uint32      arl_msg_del;    /* Count of ARL delete messages processed */
    uint32      arl_msg_bad;    /* Count of bad messages processed */
    uint32      arl_msg_tot;    /* Count of all ARL messages */

    uint32      dma_rpkt;       /* Packets received by CPU */
    uint32      dma_rbyt;       /* Bytes received by CPU */
    uint32      dma_tpkt;       /* Packets transmitted by CPU */
    uint32      dma_tbyt;       /* Bytes transmitted by CPU */

    uint32      dv_alloc;       /* Number of DV alloc's */
    uint32      dv_free;        /* Number of DV free's */
    uint32      dv_alloc_q;     /* Free list satisfied DV allocs */

    uint32      mem_cache_count;
    uint32      mem_cache_size;
    uint32      mem_cache_vmap_size;
    uint32      tcam_corrupt_map_size;
    uint32      ser_err_int;    /* Number of ser interrupt events */
    uint32      ser_err_fifo;   /* Number of ser fifo events */
    uint32      ser_err_tcam;   /* Number of ser tcam events */
    uint32      ser_err_nak;    /* Number of ser NAK events */
    uint32      ser_err_stat;   /* Number of ser stat events */
    uint32      ser_err_ecc;    /* Number of ser ecc events */
    uint32      ser_err_corr;   /* Number of ser error corrections */
    uint32      ser_err_clear;  /* Number of ser error reg/mem cleared */
    uint32      ser_err_restor; /* Number of ser error cache restored */
    uint32      ser_err_spe;    /* Number of special ser errors */
    uint32      ser_err_reg;    /* Number of reg based ser errors */
    uint32      ser_err_mem;    /* Number of mem based ser errors */
    uint32      ser_err_sw;     /* Number of ser s/w errors */
} soc_stat_t;

/*
 * Typedef: soc_memstate_t
 * Purpose: Maintain per-memory information
 * Notes: To avoid deadlock, do not use MEM_LOCK while holding SCHAN_LOCK.
 *      MEM_LOCK must be held while manipulating memory caches in any way.
 */

#define SOC_MEM_STATE(unit, mem)        SOC_CONTROL(unit)->memState[mem]

#define MEM_LOCK(unit, mem) \
        sal_mutex_take(SOC_MEM_STATE(unit, mem).lock, sal_mutex_FOREVER)
#define MEM_UNLOCK(unit, mem) \
        sal_mutex_give(SOC_MEM_STATE(unit, mem).lock)

#define CACHE_VMAP_OP(vmap, index, BOOL_OP) \
        ((vmap)[(index) / 8] BOOL_OP (1 << ((index) % 8)))

#define CACHE_VMAP_SET(vmap, index)     CACHE_VMAP_OP(vmap, index, |=)
#define CACHE_VMAP_CLR(vmap, index)     CACHE_VMAP_OP(vmap, index, &= ~)
#define CACHE_VMAP_TST(vmap, index)     CACHE_VMAP_OP(vmap, index, &)

#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HELIX4_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) || \
    defined(BCM_GREYHOUND_SUPPORT)
#define TCAM_CORRUPT_MAP_OP(corrupt, index, BOOL_OP) \
        ((corrupt)[(index) / 8] BOOL_OP (1 << ((index) % 8)))

#define TCAM_CORRUPT_MAP_SET(corrupt, index)     TCAM_CORRUPT_MAP_OP(corrupt, index, |=)
#define TCAM_CORRUPT_MAP_CLR(corrupt, index)     TCAM_CORRUPT_MAP_OP(corrupt, index, &= ~)
#define TCAM_CORRUPT_MAP_TST(corrupt, index)     TCAM_CORRUPT_MAP_OP(corrupt, index, &)
#endif

typedef struct soc_memstate_s {
    sal_mutex_t lock;                   /* Table update lock */
                                        /*   (sorted tables only) */
    uint32     *cache[SOC_MAX_NUM_BLKS];/* Write-through cache when non-NULL */
    uint8      *vmap[SOC_MAX_NUM_BLKS]; /* Cache entry valid bitmap */
#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HELIX4_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) || \
    defined(BCM_GREYHOUND_SUPPORT)
    uint8      *corrupt[SOC_MAX_NUM_BLKS]; /* entry corrupted bitmap */
                                           /* tcam tables only */
#endif
#ifdef BCM_TRIDENT2_SUPPORT
    SHR_BITDCL  *OverlayTcamMap[SOC_MAX_NUM_BLKS]; /* FP Overlay tcam bitmap*/
#endif
} soc_memstate_t;

/*
 * Typedef: soc_driver_t
 * Purpose: Chip driver.  All info about a particular device type.
 * Notes: These structures are automatically generated by mcm.
 *        Used in soc/mcm/bcm*.c files.
 */
typedef struct soc_driver_s {
    soc_chip_types          type;                /* the chip type id */
    char                    *chip_string;        /* chip string for var defs */
    char                    *origin;             /* regsfile origin */
    uint16                  pci_vendor;          /* nominal PCI vendor */
    uint16                  pci_device;          /* nominal PCI device */
    uint8                   pci_revision;        /* nominal PCI revision */
    int                     num_cos;             /* classes of service */
    soc_reg_info_t          **reg_info;          /* register array */
    soc_reg_t               **reg_unique_acc;    /* unique access type register array */
    soc_reg_above_64_info_t **reg_above_64_info; /* large register array */
    soc_reg_array_info_t    **reg_array_info;    /* register array array */
    soc_mem_info_t          **mem_info;          /* memory array */
    soc_mem_t               **mem_unique_acc;    /* unique access type memory array */
    soc_mem_t               **mem_aggr;          /* memory aggregate */
    soc_mem_array_info_t    **mem_array_info;    /* memory array array */
    soc_block_info_t        *block_info;         /* block array */
    soc_port_info_t         *port_info;          /* port array */
    soc_cmap_t              *counter_maps;       /* counter map */
    soc_feature_fun_t       feature;             /* boolean feature function */
    soc_init_chip_fun_t     init;                /* chip init function */
    drv_if_t                *services;
    int                     port_num_blktype;    /* block types per port */
    uint32                  cmicd_base;          /* Base address for CMICd Registers */
} soc_driver_t;


#define GE_PORT    0x0001
#define XE_PORT    0x0002
#define HG_PORT    0x0004
#define HGL_PORT   0x0008
#define STK_PORT   0x0010
#define CES_PORT   0x0020
#define OLP_PORT   0x0040
#define LPHY_PORT  0x0080
#define ETH_PORT   0x0100
#define INTLB_PORT 0x0200

/* soc_port_details_t is applicable to katana2 device only */
typedef struct soc_port_details_s {
    /* Range of port numbers */
    uint8 start_port_no;
    uint8 end_port_no;
    uint8 port_incr;

    /* Port Properties */
    uint8 port_type;/*Combination of hash defines(GE,FE,XE,HG,HGL,STK,CE,LPH*/

    /* Port Speed */
    uint32 port_speed;
} soc_port_details_t;


#define SOC_LANE_CONFIG_100G_DEFAULT SOC_LANE_CONFIG_100G_4_4_2
#define SOC_LANE_CONFIG_100G_AN_CORE_DEFAULT (1)

/*
 * This define tells whether a driver is active (not null).
 * The null driver is allocated, but is all zeros.  We assume that
 * the block list (block_info) is non-null for non-null drivers.
 */
#define SOC_DRIVER_ACTIVE(i) (soc_base_driver_table[i]->block_info)
#define SOC_ROBO_DRIVER_ACTIVE(i) (soc_robo_base_driver_table[i]->block_info)

/* Find an active chip to get reg info from base driver table. */
#define SOC_FIRST_ACTIVE_DRIVER(chip) \
    do { \
        chip = 0; \
        while (!SOC_DRIVER_ACTIVE(chip) && chip < SOC_NUM_SUPPORTED_CHIPS) { \
            chip++; \
        } \
        assert(chip < SOC_NUM_SUPPORTED_CHIPS); \
    } while (0)

#ifdef BCM_ROBO_SUPPORT
#define SOC_ROBO_DRIVER_ACTIVE(i) (soc_robo_base_driver_table[i]->block_info)

/* Find an active chip to get reg info from base driver table. */
#define SOC_ROBO_FIRST_ACTIVE_DRIVER(chip) \
    do { \
    chip = 0; \
    while (!SOC_ROBO_DRIVER_ACTIVE(chip) && chip < SOC_ROBO_NUM_SUPPORTED_CHIPS) { \
        chip++; \
    } \
    assert(chip < SOC_ROBO_NUM_SUPPORTED_CHIPS); \
    } while (0)
#endif /* BCM_ROBO_SUPPORT */

extern soc_driver_t *soc_chip_driver_find(uint16 dev_id, uint8 rev_id);
extern void soc_chip_dump(int unit, soc_driver_t *d);
extern void soc_robo_chip_dump(int unit, soc_driver_t *d);

/*
 * Typedef: soc_fm_t
 * Purpose: For each filter mask entry, cache the index of the first rule
 *          using that mask, and the number of rules using that mask.
 *          (All rules that use a given mask are necessarily contiguous.)
 *          These values speed up searches in the FFP, and on 5690 they
 *          are required by the hardware.
 */

#define SOC_FFP_MASK_MAX        24      /* Max # masks of any chip */

typedef struct soc_fm_s {
    uint16      start;
    uint16      count;
} soc_fm_t;

/*
 * Typedef: soc_ipg_t
 * Purpose: IFG register setting for all speed/duplex combinations
 */

typedef struct soc_ipg_s {
    uint32      hd_10;
    uint32      hd_100;
    uint32      hd_1000;
    uint32      hd_2500;
    uint32      fd_10;
    uint32      fd_100;
    uint32      fd_1000;
    uint32      fd_2500;
    uint32      fd_10000;
    uint32      fd_xe;
    uint32      fd_hg;
    uint32      fd_hg2;
    uint32      fe_hd_10;
    uint32      fe_hd_100;
    uint32      fe_fd_10;
    uint32      fe_fd_100;
    uint32      gth_hd_10;
    uint32      gth_hd_100;
    uint32      gth_fd_10;
    uint32      gth_fd_100;
    uint32      ge_hd_1000;
    uint32      ge_fd_1000;
    uint32      ge_hd_2500;
    uint32      ge_fd_2500;
    uint32      bm_fd_10000;
} soc_ipg_t;


#define SOC_INFO_CHIP_ENDURO         0x00000001    /* 56334 */
#define SOC_INFO_CHIP_APOLLO         0x00000002    /* 56524 */
#define SOC_INFO_CHIP_TRIDENT2       0x00000004    /* 5685x */
#define SOC_INFO_CHIP_TITAN2         0x00000008    /* 5675x */
#define SOC_INFO_CHIP_KATANA2        0x00000010    /* 5645x */
#define SOC_INFO_CHIP_HURRICANE2     0x00000020    /* 5615x */
#define SOC_INFO_CHIP_HELIX4         0x00000040    /* 5634x */
#define SOC_INFO_CHIP_HERCULES15     0x00000080    /* 5675 */
#define SOC_INFO_CHIP_TOMAHAWK       0x00000100    /* 5696x */
#define SOC_INFO_CHIP_EASYRIDER      0x00000100    /* 5660x */
#define SOC_INFO_CHIP_FIREBOLT       0x00000200    /* 56504 */
#define SOC_INFO_CHIP_VALKYRIE2      0x00000400    /* 56685 */
#define SOC_INFO_CHIP_TRIDENT2PLUS   0x00000800    /* 5686x */
#define SOC_INFO_CHIP_FHX            0x00001000    /* 5610x, 5630x */
#define SOC_INFO_CHIP_SHADOW         0x00002000    /* 88732 */
#define SOC_INFO_CHIP_GOLDWING       0x00004000    /* 56580 */
#define SOC_INFO_CHIP_HUMV           0x00008000    /* 56700 */
#define SOC_INFO_CHIP_BRADLEY        0x00010000    /* 56800 */
#define SOC_INFO_CHIP_RAPTOR         0x00020000    /* 56218 */
#define SOC_INFO_CHIP_KATANA         0x00040000    /* 56440 */
#define SOC_INFO_CHIP_TRIUMPH3       0x00080000    /* 56640 */
#define SOC_INFO_CHIP_FIREBOLT2      0x00100000    /* 56514 */
#define SOC_INFO_CHIP_SCORPION       0x00200000    /* 56820 */
#define SOC_INFO_CHIP_TRIUMPH        0x00400000    /* 56624 */
#define SOC_INFO_CHIP_RAVEN          0x00800000    /* 56224 */
#define SOC_INFO_CHIP_HAWKEYE        0x01000000    /* 53314 */
#define SOC_INFO_CHIP_VALKYRIE       0x02000000    /* 56680 */
#define SOC_INFO_CHIP_CONQUEROR      0x04000000    /* 56725 */
#define SOC_INFO_CHIP_TRIUMPH2       0x08000000    /* 56634 */
#define SOC_INFO_CHIP_TRIDENT        0x10000000    /* 5684x */
#define SOC_INFO_CHIP_HURRICANE      0x20000000    /* 56142 */
#define SOC_INFO_CHIP_TITAN          0x40000000    /* 5674x */
#define SOC_INFO_CHIP_SIRIUS         0x80000000    /* 88230 */
#define SOC_INFO_CHIP_HERCULES       (SOC_INFO_CHIP_HERCULES15)
#define SOC_INFO_CHIP_FB             (SOC_INFO_CHIP_FIREBOLT | SOC_INFO_CHIP_FIREBOLT2)
#define SOC_INFO_CHIP_FX_HX          (SOC_INFO_CHIP_FHX | \
                                      SOC_INFO_CHIP_RAPTOR | \
                                      SOC_INFO_CHIP_RAVEN | \
                                      SOC_INFO_CHIP_HAWKEYE)
#define SOC_INFO_CHIP_FB_FX_HX       (SOC_INFO_CHIP_FB | SOC_INFO_CHIP_FX_HX)
#define SOC_INFO_CHIP_HB_GW          (SOC_INFO_CHIP_HUMV | \
                                      SOC_INFO_CHIP_BRADLEY | \
                                      SOC_INFO_CHIP_GOLDWING)
#define SOC_INFO_CHIP_TD2_TT2        (SOC_INFO_CHIP_TRIDENT2 | \
                                      SOC_INFO_CHIP_TITAN2 | \
                                      SOC_INFO_CHIP_TOMAHAWK | \
                                      SOC_INFO_CHIP_TRIDENT2PLUS)
#define SOC_INFO_CHIP_TRIDENT2X      (SOC_INFO_CHIP_TRIDENT2 | \
                                      SOC_INFO_CHIP_TRIDENT2PLUS)
#define SOC_INFO_CHIP_TD_TT          (SOC_INFO_CHIP_TRIDENT | \
                                      SOC_INFO_CHIP_TITAN | \
                                      SOC_INFO_CHIP_TD2_TT2)
#define SOC_INFO_CHIP_TR_VL          (SOC_INFO_CHIP_TRIUMPH | \
                                      SOC_INFO_CHIP_VALKYRIE | \
                                      SOC_INFO_CHIP_TRIUMPH2 | \
                                      SOC_INFO_CHIP_APOLLO | \
                                      SOC_INFO_CHIP_VALKYRIE2 | \
                                      SOC_INFO_CHIP_ENDURO | \
                                      SOC_INFO_CHIP_TD_TT | \
                                      SOC_INFO_CHIP_HURRICANE | \
                                      SOC_INFO_CHIP_KATANA | \
                                      SOC_INFO_CHIP_TRIUMPH3 | \
                                      SOC_INFO_CHIP_HELIX4)
#define SOC_INFO_CHIP_SC_CQ          (SOC_INFO_CHIP_SCORPION | \
                                      SOC_INFO_CHIP_CONQUEROR | \
                                      SOC_INFO_CHIP_SHADOW)
#define SOC_INFO_CHIP_HBX            (SOC_INFO_CHIP_HB_GW | SOC_INFO_CHIP_SC_CQ)
#define SOC_INFO_CHIP_TRX            (SOC_INFO_CHIP_TR_VL | \
                                      SOC_INFO_CHIP_SC_CQ | \
                                      SOC_INFO_CHIP_KATANA2 | \
                                      SOC_INFO_CHIP_HURRICANE2)
#define SOC_INFO_CHIP_FBX            (SOC_INFO_CHIP_FB_FX_HX | \
                                      SOC_INFO_CHIP_HB_GW | \
                                      SOC_INFO_CHIP_TRX)
#define SOC_INFO_CHIP_XGS3_SWITCH    (SOC_INFO_CHIP_FBX)
#define SOC_INFO_CHIP_XGS3_FABRIC    (SOC_INFO_CHIP_HUMV | \
                                      SOC_INFO_CHIP_CONQUEROR | \
                                      SOC_INFO_CHIP_TITAN | \
                                      SOC_INFO_CHIP_TITAN2)
#define SOC_INFO_CHIP_XGS_SWITCH     (SOC_INFO_CHIP_XGS3_SWITCH)
#define SOC_INFO_CHIP_XGS12_FABRIC   (SOC_INFO_CHIP_HERCULES)
#define SOC_INFO_CHIP_XGS_FABRIC     (SOC_INFO_CHIP_XGS12_FABRIC | \
                                      SOC_INFO_CHIP_XGS3_FABRIC)
#define SOC_INFO_CHIP_XGS            (SOC_INFO_CHIP_XGS_FABRIC | \
                                      SOC_INFO_CHIP_XGS_SWITCH)

#include <soc/drv_chip_depend.h>

typedef int (*soc_rcpu_schan_op_cb)(
    int unit,
    schan_msg_t *msg,
    int dwc_write,
    int dwc_read);

extern int soc_rcpu_schan_op_register(int unit, soc_rcpu_schan_op_cb f);
extern int soc_rcpu_schan_op_unregister(int unit);


typedef void (*soc_event_cb_t)(
    int                 unit,
    soc_switch_event_t  event,
    uint32              arg1,
    uint32              arg2,
    uint32              arg3,
    void                *userdata);

typedef struct list_event_cb_s {
   soc_event_cb_t           cb;
   void                     *userdata;
   struct list_event_cb_s   *next;
}soc_event_cb_list_t;

extern int soc_event_register(int unit, soc_event_cb_t cb, void *userdata);
extern int soc_event_unregister(int unit, soc_event_cb_t cb, void *userdata);
extern int soc_event_generate(int unit,  soc_switch_event_t event,
                              uint32 arg1, uint32 arg2, uint32 arg3);
extern void soc_event_assert(const char *expr, const char *file, int line);



#ifdef BCM_WARM_BOOT_SUPPORT
extern int soc_scache_esw_nh_mem_write(int unit, uint8 *buf,
                                       int offset, int nbytes);
extern int soc_scache_esw_nh_mem_read(int unit, uint8 *buf,
                                      int offset, int nbytes);

#ifdef BROADCOM_DEBUG
extern int soc_scache_file_close(void);
extern int soc_scache_file_open(int unit, int warm_boot);
#endif /* BROADCOM_DEBUG */

extern int soc_switch_control_scache_init(int unit);
extern int soc_switch_control_scache_sync(int unit);

#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_CMICM_SUPPORT
/*Structures for UC message receive LL*/
/*LL Node*/
typedef struct ll_element_s {
    struct ll_element_s *p_next;
    struct ll_element_s *p_prev;
}ll_element_t;

/* Doubly linked list ctrl structure*/
typedef struct ll_ctrl_s {
    ll_element_t    *p_head;    /* pointer to the first element */
    ll_element_t    *p_tail;    /* pointer to the last element */
    uint32          ll_count;   /* the number of elements  the list */
}ll_ctrl_t;

typedef struct mos_msg_ll_node_s {
    ll_element_t  msg_q;        /*LL linkage*/
    mos_msg_data_t  msg_data;   /*Msg Data*/
}mos_msg_ll_node_t;
#endif

/* SOC DMA mode definitions */
/* Continuous mode uses reload descriptors to chain DVs */
typedef enum soc_dma_mode_e {
    SOC_DMA_MODE_CHAINED = 0,       /* Legacy CMIC devices */
    SOC_DMA_MODE_CONTINUOUS = 1     /* CMICDV2 */
} soc_dma_mode_t;



/*
 * Typedef: sop_memstate_t
 * Purpose: Maintain per-memory persistent information
 */

#define SOP_MEM_STATE(unit, mem)        SOC_PERSIST(unit)->memState[mem]

typedef struct sop_memstate_s {
    uint32      count[SOC_MAX_NUM_BLKS];/* Number of valid table entries */
                                        /*   (sorted tables only) */
    int         index_max;              /* May be updated per unit */
} sop_memstate_t;

typedef struct sop_memcfg_er_s {
    uint32      ext_table_cfg;
    uint32      host_hash_table_cfg;
    uint32      l3v4_search_offset;
    uint32      l3v6_search_offset;
    uint32      mvl_hash_table_cfg;
    uint32      mystation_search_offset;
    int         tcam_select;
} sop_memcfg_er_t;

/*
 * Typedef: soc_persist_t
 * Purpose: SOC Persistent Structure.
 *      All info about a device instance that must be saved across a reload.
 * Notes:
 *      Modifications to this structure will prevent reload.  A system running
 *      a given version would not be able to be upgraded to a modified version.
 */

typedef struct soc_persist_s {
    /* Miscellaneous chip state */

    int         version;        /* Persistent state version */

    int         debugMode;      /* True if MMU is in debug mode */

    /* DCB static configuration */

    uint32      dcb_srcmod;             /* Source module and port for dcbs */
    uint32      dcb_srcport;
    uint32      dcb_pfm;                /* Port Forwarding Mode */

    /* IPG Configuration per port */

    soc_ipg_t   ipg[SOC_MAX_NUM_PORTS];

    /* On-chip memory management */

    sop_memstate_t
                memState[NUM_SOC_MEM];
#ifdef SOC_MEM_L3_DEFIP_WAR
    uint32      l3_defip_map;   /* Unused L3_DEFIP blocks */
#endif

    sop_memcfg_er_t
                er_memcfg;

    /* Link status */

    pbmp_t      link_fwd;       /* Forwarding ports current value */

    /* Linkscan status (private; may only accessed by link.c) */

    pbmp_t      lc_pbm_link;            /* Ports currently up */
    pbmp_t      lc_pbm_link_change;     /* Ports needed to recognize down */
    pbmp_t      lc_pbm_override_ports;  /* Force up/Down ports */
    pbmp_t      lc_pbm_override_link;   /* Force up/Down status */
    pbmp_t      lc_pbm_linkdown_tx;     /* Enable tx without link */
    pbmp_t      lc_pbm_remote_fault;    /* Ports receiving remote fault */
    pbmp_t      lc_pbm_failover;        /* Ports set for LAG failover */
    pbmp_t      lc_pbm_failed;          /* Failed LAG failover ports */
    pbmp_t      lc_pbm_failed_clear;    /* Failed ports ready to reset */
    pbmp_t      lc_pbm_fc;              /* FC ports */

    /*
     * Stacking related:
     *     stack_ports_current:   Ports currently engaged in stacking
     *     stack_ports_inactive:  Ports stacked, but explicitly blocked
     *     stack_ports_simplex:   Ports stacked using simplex mode
     *     stack_ports_previous:  Last stack port; to detect changes
     */
    soc_pbmp_t stack_ports_current;
    soc_pbmp_t stack_ports_inactive;
    soc_pbmp_t stack_ports_previous;
} soc_persist_t;

#define SOC_AVERAGE_IPG_IEEE (96)
#define SOC_AVERAGE_IPG_HG   (64)

#define SOC_FLAGS_CLR(s, f) {       \
    int _s = sal_splhi();       \
    (s)->soc_flags &= ~(f);     \
    sal_spl(_s);            \
}

#define SOC_NDEV_IDX2DEV(_dev_idx)    soc_ndev_idx2dev_map[_dev_idx]

/*
 * soc_control: Per-device non-persistent global driver state
 * soc_persist: Per-device persistent global driver state
 * soc_ndev: Number of devices found during probe
 */

extern soc_control_t    *soc_control[SOC_MAX_NUM_DEVICES];
extern soc_persist_t    *soc_persist[SOC_MAX_NUM_DEVICES];
extern int              soc_ndev_attached;
extern int              soc_ndev;
extern int              soc_ndev_idx2dev_map[SOC_MAX_NUM_DEVICES];
extern int      soc_eth_ndev;
extern int      soc_all_ndev;
extern int      soc_eth_unit;
extern int      soc_mii_unit;
extern char     soc_reg_noinfo[];

extern soc_block_name_t soc_block_port_names[];
extern soc_block_name_t soc_block_names[];
extern soc_block_name_t soc_sbx_block_port_names[];
extern soc_block_name_t soc_sbx_block_names[];
#if defined(BCM_SAND_SUPPORT)
extern soc_block_name_t soc_dpp_block_port_names[];
extern soc_block_name_t soc_dpp_block_names[];
#endif /* defined(BCM_SAND_SUPPORT) */
extern sal_mutex_t spiMutex;

extern uint32    soc_state[SOC_MAX_NUM_DEVICES];

#if defined(BCM_WARM_BOOT_SUPPORT)
/*
 * BCM Warm Boot Support
 *
 * Purpose:  Indicates whether the device is currently in reload
 *           state (performing Warm boot) or not.
 *
 *           If finer granularity is needed in the future, additional
 *           flags can be defined to control specific hardware accesses.
 */

#define SOC_ST_F_RELOADING         0x1    /* Warm boot in progress, device is reloading */
#define SOC_WARM_BOOT(unit)       (soc_state[unit] == SOC_ST_F_RELOADING)
#define SOC_WARM_BOOT_START(unit) (soc_state[unit] = SOC_ST_F_RELOADING)
#define SOC_WARM_BOOT_DONE(unit)  (soc_state[unit] = soc_state[unit] & ~SOC_ST_F_RELOADING)
#define SOC_IS_DONE_INIT(unit) 		((SOC_CONTROL(unit)->soc_flags & SOC_F_ALL_MODULES_INITED) && \
                                     (SOC_CONTROL(unit)->soc_flags & SOC_F_ATTACHED) && \
                                     (SOC_CONTROL(unit)->soc_flags & SOC_F_INITED))
/*(((SOC_CONTROL(unit)->soc_flags & SOC_F_RESET) == SOC_F_RESET) ? TRUE : FALSE)*/

extern int soc_wb_mim_state[SOC_MAX_NUM_DEVICES];
#define SOC_WARM_BOOT_MIM(unit) (soc_wb_mim_state[unit] = 1)
#define SOC_WARM_BOOT_IS_MIM(unit) (soc_wb_mim_state[unit] == 1)
extern int soc_shutdown(int unit);
extern int soc_system_scrub(int unit);

/* Allow schan write during warmboot */
#define SOC_ALLOW_WB_WRITE(_unit, _operation, _rv)\
      do {\
        int _rv_tmp;\
        _rv = soc_schan_override_enable(_unit);\
        if (SOC_SUCCESS(_rv)) {\
            _rv = _operation;\
            _rv_tmp = soc_schan_override_disable(_unit);\
            if (SOC_FAILURE(_rv_tmp)) {\
                _rv = _rv_tmp;\
            }\
        }\
      } while(0)

#define SOC_AUTOSYNC_IS_ENABLE(_unit) (SOC_CONTROL(_unit)->autosync)
#else
#define SOC_WARM_BOOT(unit)       (0)
#define soc_shutdown(unit)        (SOC_E_UNAVAIL)
#define soc_system_scrub(unit)    (SOC_E_UNAVAIL)
#define SOC_WARM_BOOT_START(unit) \
             do { \
             } while(0)

#define SOC_WARM_BOOT_DONE(unit)  \
             do { \
             } while(0)
#define SOC_IS_DONE_INIT(unit) 		((SOC_CONTROL(unit)->soc_flags & SOC_F_ALL_MODULES_INITED) && \
                                     (SOC_CONTROL(unit)->soc_flags & SOC_F_ATTACHED) && \
                                     (SOC_CONTROL(unit)->soc_flags & SOC_F_INITED))
#define SOC_WARM_BOOT_MIM(unit)   (0)
#define SOC_WARM_BOOT_IS_MIM(unit)   (0)

#define SOC_ALLOW_WB_WRITE(_unit, _operation, _rv)\
    do {\
        _rv = _operation;\
    } while(0)
#define SOC_AUTOSYNC_IS_ENABLE(_unit) (0)
#endif /* BCM_WARM_BOOT_SUPPORT */

#define SOC_HW_RESET(unit)          (SOC_CONTROL(unit)->soc_flags & SOC_F_HW_RESETING)
#define SOC_HW_RESET_START(unit)    (SOC_CONTROL(unit)->soc_flags |= SOC_F_HW_RESETING)
#define SOC_HW_RESET_DONE(unit)     (SOC_CONTROL(unit)->soc_flags &= ~SOC_F_HW_RESETING)

#if defined(BCM_EASY_RELOAD_SUPPORT)
/*
 * BCM Easy Reload Support
 *
 * Purpose:  Indicates whether the device is currently in reload
 *           state (performing Easy Reload) or not.
 *
 *           If finer granularity is needed in the future, additional
 *           flags can be defined to control specific hardware accesses.
 */

#define SOC_ST_F_EASY_RELOAD       0x2    /* Easy Reload is in progress */
/* Reload mode set/get macros */
#define SOC_IS_RELOADING(unit)    (soc_state[unit] == SOC_ST_F_EASY_RELOAD)
#define SOC_RELOAD_MODE_SET(unit, reload_mode) \
    if (reload_mode) {              \
    soc_state[unit] = SOC_ST_F_EASY_RELOAD; \
    } else {                    \
        soc_state[unit] = soc_state[unit] & ~SOC_ST_F_EASY_RELOAD;           \
    }
#elif defined(BCM_EASY_RELOAD_WB_COMPAT_SUPPORT)
/*Easy reload wb compat*/
#define SOC_ST_F_EASY_RELOAD       0x2    /* Easy Reload is in progress */
#define SOC_IS_RELOADING(unit)    (soc_feature(unit, soc_feature_easy_reload_wb_compat) &&  \
                                    soc_state[unit] == SOC_ST_F_EASY_RELOAD)
#define SOC_RELOAD_MODE_SET(unit, reload_mode)                                              \
    if (soc_feature(unit, soc_feature_easy_reload_wb_compat)) {                             \
        if (reload_mode) {                                                                  \
            soc_state[unit] = SOC_ST_F_EASY_RELOAD;                                         \
        } else {                                                                            \
            soc_state[unit] = soc_state[unit] & ~SOC_ST_F_EASY_RELOAD;                                                             \
        }                                                                                   \
    }

#else
/*No Easy reload support*/
#define SOC_IS_RELOADING(unit) (0)
#define SOC_RELOAD_MODE_SET(unit, reload_mode)  \
             do { \
             } while(0)
#endif /* BCM_EASY_RELOAD_SUPPORT */
/*
 * BCM detach
 *
 * Purpose:  Indicates whether the device is currently being detached from the
 *           system state (performing detach) or not.
 */

#define SOC_ST_F_DETACH       0x4    /* Detach is in progress */
/* Reload mode set/get macros */
#define SOC_IS_DETACHING(_unit_)    (soc_state[_unit_] == SOC_ST_F_DETACH)
#define SOC_DETACH(_unit_, _detach_) \
    if (_detach_) {              \
        soc_state[_unit_] = SOC_ST_F_DETACH; \
    } else {                    \
        soc_state[_unit_] = 0;            \
    }

#define SOC_HW_ACCESS_DISABLE(_unit_)                  \
    ((SOC_WARM_BOOT(_unit_) &&                         \
      (SOC_CONTROL(unit)->schan_wb_thread_id != sal_thread_self())) ||\
     SOC_IS_RELOADING(_unit_) || SOC_IS_DETACHING(_unit_))

/*
 * Driver calls.
 */
extern int soc_attach(int unit);
extern int soc_attached(int unit);
extern int soc_reset(int unit);
extern int soc_init(int unit);
extern int soc_reset_init(int unit);
extern int soc_device_reset(int unit, int mdoe, int action);
extern int soc_detach(int unit);
extern int soc_deinit(int unit);

#if defined(BCM_ESW_SUPPORT) && defined(BCM_IPROC_SUPPORT)
extern int soc_bond_info_init(int unit);
extern int soc_bond_info_deinit(int unit);
#endif

extern int soc_bist(int unit, soc_mem_t *mems, int num_mems, int timeout_msec);
extern int soc_bist_all(int unit);

extern int soc_bpdu_addr_set(int unit, int index, sal_mac_addr_t addr);
extern int soc_bpdu_addr_get(int unit, int index, sal_mac_addr_t *addr);

extern int soc_rcpu_schan_enable_set(int unit, int enable);

extern int soc_mmu_init(int unit);
extern int soc_misc_init(int unit);
extern const char *soc_dev_name(int unit);
extern int soc_info_config(int unit, soc_control_t *soc);
extern int soc_max_supported_vrf_get(int unit, uint16 *value);
extern void soc_xgxs_lcpll_lock_check(int unit);
extern void soc_xport_type_update(int unit, soc_port_t port, int to_hg_port);
extern int  soc_port_type_verify(int unit);
extern int  soc_port_name_verify(int unit);
extern int  soc_robo_attach(int unit);
extern int  soc_robo_reset(int unit);
extern int  soc_robo_init(int unit);
extern int  soc_robo_reset_init(int unit);
extern int  soc_robo_detach(int unit);
extern int  soc_robo_mgt_init(int unit);
extern int _soc_robo_device_created(int unit);

#ifdef BCM_LEDPROC_SUPPORT
extern int soc_ledproc_config(int unit, const uint8 *program, int bytes);
#endif

extern int soc_warpcore_firmware_set(int unit, int port, uint8 *array,
                                     int datalen, int wc_instance,
                                     soc_mem_t wc_ucmem_data,
                                     soc_reg_t wc_ucmem_ctrl);

extern int
soc_phy_firmware_load(int unit, int port, uint8 *fw_data, int fw_size);

extern int
soc_sbus_mdio_reg_read(int unit, int port, int blk, int wc_instance,
                       uint32 phy_addr, uint32 phy_reg, uint32 *phy_data,
                       soc_mem_t wc_ucmem_data, soc_reg_t wc_ucmem_ctrl);
extern int
soc_sbus_mdio_reg_write(int unit, int port, int blk, int wc_instance,
                        uint32 phy_addr, uint32 phy_reg, uint32 phy_data,
                        soc_mem_t wc_ucmem_data, soc_reg_t wc_ucmem_ctrl);

extern int
soc_sbus_tsc_reg_read(int unit, int port, int blk, uint32 phy_addr, 
                      uint32 phy_reg, uint32 *phy_data);
extern int
soc_sbus_tsc_reg_write(int unit, int port, int blk, uint32 phy_addr, 
                       uint32 phy_reg, uint32 phy_data);

#define SOC_PROPERTY_NAME_MAX   128
#define SOC_PROPERTY_VALUE_MAX  64

extern char *soc_property_get_str(int unit, const char *name);
extern uint32 soc_property_get(int unit, const char *name, uint32 defl);
extern uint32 soc_property_obj_attr_get(int unit, const char *prefix,
                                        const char *obj, int index,
                                        const char *attr, int scale,
                                        char *suffix, uint32 defl);
extern pbmp_t soc_property_get_pbmp(int unit, const char *name,
                                    int defneg);
extern pbmp_t soc_property_get_pbmp_default(int unit, const char *name,
                                            pbmp_t def_pbmp);

extern pbmp_t soc_property_suffix_num_pbmp_get(int unit, int num, const char *name,
                            const char *suffix, soc_pbmp_t pbmp_def);

extern void soc_property_get_bitmap_default(int unit, const char *name,
                              uint32 *bitmap, int max_words, uint32 *def_bitmap);

extern int soc_property_get_csv(int unit, const char *name,
                                     int val_max, int *val_array);
extern char *soc_property_port_get_str(int unit, soc_port_t port,
                                       const char *name);
extern uint32 soc_property_port_get(int unit, soc_port_t port,
                                    const char *name, uint32 defl);
extern char *soc_property_phy_get_str(int unit, soc_port_t port,
                                      int phy_num, int phy_port, int lane,
                                      const char *name);
extern uint32 soc_property_phy_get(int unit, soc_port_t port,
                                   int phy_num, int phy_port, int lane,
                                   const char *name, uint32 defl);
extern uint32 soc_property_phys_port_get(int unit, soc_port_t port,
                                         const char *name, uint32 defl);
extern uint32 soc_property_port_obj_attr_get(int unit, soc_port_t port,
                                             const char *prefix,
                                             const char *obj, int index,
                                             const char *attr, int scale,
                                             char *suffix, uint32 defl);
extern int soc_property_port_get_csv(int unit, soc_port_t port,
                                     const char *name, int val_max,
                                     int *val_array);
extern uint32 soc_property_suffix_num_get(int unit, int num,
                                          const char *name,
                                          const char *suffix, uint32 defl);

extern uint32 soc_property_suffix_num_get_only_suffix(int unit, int num, const char *name,
                            const char *suffix, uint32 defl);

extern char* soc_property_suffix_num_str_get(int unit, int num, const char *name,
                            const char *suffix);
extern uint32 soc_property_port_suffix_num_get(int unit, soc_port_t port,
                                               int num, const char *name,
                                          const char *suffix, uint32 defl);
extern char *soc_property_port_suffix_num_get_str(int unit, soc_port_t port,
                                               int num, const char *name,
                                          const char *suffix);
extern uint32 soc_property_cos_get(int unit, soc_cos_t cos,
                                    const char *name, uint32 defl);
extern uint32 soc_property_uc_get(int unit, int uc,
                                   const char *name, uint32 defl);
extern uint32 soc_property_ci_get(int unit, int ci,
                                   const char *name, uint32 defl);
extern int soc_property_ci_get_csv(int unit, int ci,
                                     const char *name, int val_max,
                                     int *val_array);

extern char* soc_property_suffix_num_only_suffix_str_get(int unit, int num, const char *name,
                                                            const char *suffix);

extern char *soc_block_port_name_lookup_ext(soc_block_t, int);
extern char *soc_block_name_lookup_ext(soc_block_t, int);
extern soc_block_t soc_block_port_name_match(char *);
extern soc_block_t soc_block_name_match(char *);


#define SOC_PCI_DEV_TYPE      SAL_PCI_DEV_TYPE    /* PCI device */
#define SOC_SPI_DEV_TYPE      SAL_SPI_DEV_TYPE    /* SPI device */
#define SOC_EB_DEV_TYPE       SAL_EB_DEV_TYPE     /* EB device */
#define SOC_ICS_DEV_TYPE      SAL_ICS_DEV_TYPE    /* ICS device */
#define SOC_MII_DEV_TYPE      SAL_MII_DEV_TYPE    /* MII device */
#define SOC_RCPU_DEV_TYPE     SAL_RCPU_DEV_TYPE   /* RCPU device */
#define SOC_I2C_DEV_TYPE      SAL_I2C_DEV_TYPE    /* I2C device */
#define SOC_AXI_DEV_TYPE      SAL_AXI_DEV_TYPE    /* AXI device */
#define SOC_EMMI_DEV_TYPE     SAL_EMMI_DEV_TYPE   /* EMMI device*/
#define SOC_DEV_BUS_ALT       SAL_DEV_BUS_ALT     /* Alternate Access */
#define SOC_DEV_BUS_MSI       SAL_DEV_BUS_MSI     /* Message-signaled interrupts */

#define SOC_SWITCH_DEV_TYPE   SAL_SWITCH_DEV_TYPE /* Switch device */
#define SOC_ETHER_DEV_TYPE    SAL_ETHER_DEV_TYPE  /* Ethernet device */
#define SOC_CPU_DEV_TYPE      SAL_CPU_DEV_TYPE    /* CPU device */

/* Backward compatibility */
#define SOC_ET_DEV_TYPE       SOC_MII_DEV_TYPE

#define SOC_TIMEOUT_VAL 100 /* Times for retrying */
/* soc/intr.c exported routines */

/* Interrupt function type */
typedef void (*soc_interrupt_fn_t)(void *_unit);

#ifdef VENDOR_CUSTOMER52
/* Interrupt CB Regestration */
extern int soc_interrupt_cb_register(int unit, soc_interrupt_fn_t fn);
#endif

/*******************************************
* @function soc_sbusdma_lock_init
* purpose API to Initialize SBUSDMA
* locks and Semaphores. This will also be used
* for TSLAMDMA and TABLEDMA
*
* @param unit [in] unit
*
* @returns SOC_E_NONE
* @returns SOC_E_XXX
*
* @end
 */
extern int
soc_sbusdma_lock_init(int unit);

/*******************************************
* @function soc_sbusdma_lock_deinit
* purpose API to deinit SBUSDMA
* locks and Semaphores
*
* @param unit [in] unit
*
* @returns SOC_E_NONE
*
* @end
 */
extern int
soc_sbusdma_lock_deinit(int unit);



extern void soc_intr(void *unit);
extern uint32  soc_intr_enable(int unit, uint32 mask);
extern uint32  soc_intr_disable(int unit, uint32 mask);
extern uint32  soc_intr_block_lo_enable(int unit, uint32 mask);
extern uint32  soc_intr_block_lo_disable(int unit, uint32 mask);
extern uint32  soc_intr_block_hi_enable(int unit, uint32 mask);
extern uint32  soc_intr_block_hi_disable(int unit, uint32 mask);
#ifdef BCM_CMICM_SUPPORT
extern void soc_cmicm_intr(void *unit);
#ifdef VENDOR_CUSTOMER52
void soc_user_cmicm_intr(void *_unit);
#endif
extern uint32  soc_cmicm_intr0_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr0_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr1_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr1_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr2_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr2_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr3_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr3_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr4_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr4_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr5_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr5_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr6_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_intr6_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr0_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr0_disable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr1_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr1_disable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr2_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr2_disable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr3_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr3_disable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr4_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr4_disable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr5_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr5_disable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr6_enable(int unit, int cmc, uint32 mask);
extern uint32  soc_cmicm_cmcx_intr6_disable(int unit, int cmc, uint32 mask);
#ifdef BCM_RCPU_SUPPORT
extern void soc_cmicm_rcpu_intr(int unit, soc_rcpu_intr_packet_t *intr_pkt);
extern uint32  soc_cmicm_rcpu_intr0_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr0_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr1_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr1_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr2_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr2_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr3_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr3_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr4_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_intr4_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc0_intr0_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc0_intr0_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc1_intr0_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc1_intr0_disable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc2_intr0_enable(int unit, uint32 mask);
extern uint32  soc_cmicm_rcpu_cmc2_intr0_disable(int unit, uint32 mask);
#endif /* BCM_RCPU_SUPPORT */
extern void soc_cmicm_ihost_irq_offset_set(int unit);
extern void soc_cmicm_ihost_irq_offset_reset(int unit);
#endif /* CMICM Support */

extern void soc_endian_get(int unit, int *pio, int *packet, int *other);
extern void soc_endian_config(int unit);
extern void soc_pci_ep_config(int unit, int pcie);
extern void soc_pci_burst_enable(int unit);

extern int soc_max_vrf_set(int unit, int value);

#if defined(BROADCOM_DEBUG)                      /* Debug Support */
extern int      soc_dump(int unit, const char *pfx);
extern int  soc_robo_dump(int unit, const char *pfx);
#endif /* defined(BROADCOM_DEBUG) */

/* MAC Core initialization */
extern int soc_xgxs_reset(int unit, soc_port_t port);
extern int soc_wc_xgxs_reset(int unit, soc_port_t port, int reg_idx);
extern int soc_wc_xgxs_pll_check(int unit, soc_port_t port, int reg_idx);
extern int soc_wc_xgxs_power_down(int unit, soc_port_t port, int reg_idx);
extern int soc_tsc_xgxs_reset(int unit, soc_port_t port, int reg_idx);
extern int soc_tsc_xgxs_power_mode(int unit, soc_port_t port, int reg_idx, int mode);
extern int soc_tsc_xgxs_pll_check(int unit, soc_port_t port, int reg_idx);
extern int soc_fusioncore_reset(int unit, soc_port_t port);
#if defined(BCM_GXPORT_SUPPORT)
extern int soc_unicore_reset(int unit, soc_port_t port);
#endif /* BCM_GXPORT_SUPPORT */

/* Cosq init */
#ifdef BCM_COSQ_HIGIG_MAP_DISABLE
int soc_cosq_stack_port_map_disable(int unit);
#endif

#ifdef BCM_XGS3_SWITCH_SUPPORT
int soc_cpu_priority_mapping_init(int unit);
#endif

#if defined(BCM_FIREBOLT_SUPPORT)
#if defined(INCLUDE_L3)
extern int soc_tunnel_term_block_size_set (int unit, int size);
extern int soc_tunnel_term_block_size_get (int unit, int *size);
#endif /* BCM_FIREBOLT_SUPPORT */
#endif /* INCLUDE_L3 */

/* SER HW engine configuration and control structures */

/* SER per-table control flags */
#define _SOC_SER_FLAG_ACC_TYPE_MASK  0x0001f  /* Access type for multi-pipe */
#define _SOC_SER_FLAG_SW_COMPARE     0x00100  /* SW implemented SER check */
#define _SOC_SER_FLAG_DISCARD_READ   0x00200  /* Read DMA with HW discard */
#define _SOC_SER_FLAG_MULTI_PIPE     0x00400  /* HW multi-pipe table */
#define _SOC_SER_FLAG_XY_READ        0x00800  /* Read SER optimization */
#define _SOC_SER_FLAG_NO_DMA         0x01000  /* Use PIO iterations */
#define _SOC_SER_FLAG_REMAP_READ     0x02000  /* Read remapped indexes */
#define _SOC_SER_FLAG_OVERLAY        0x04000  /* Memories shared in HW */
#define _SOC_SER_FLAG_OVERLAY_CASE   0x08000  /* Polarity of overlay */
#define _SOC_SER_FLAG_SIZE_VERIFY    0x10000  /* Check table size */
#define _SOC_SER_FLAG_ACC_TYPE_CHK   0x20000  /* Used for unique pipe acc type mode */
#define _SOC_SER_FLAG_RANGE_DISABLE  0x40000  /* Use addr_mask as range disable bmap */
/* Create entry for this mem view but don't enable the engines range
 * Also, memscan will ignore such entry */
#define _SOC_SER_FLAG_VIEW_DISABLE   0x80000
/* Don't create entry for this mem view in ser_engine.
 * This is only for memscan engine */
#define _SOC_SER_FLAG_CONFIG_SKIP        0x100000
/*This flag is used to directly return when DMA read op occurs error
*in soc_mem_ser_read_range.*/
#define _SOC_SER_FLAG_DMA_ERR_RETURN    0x200000

/* when pipe_mode_unique is set, memscan will remap mem to mem_PIPE0,1,2,3 */
#define _SOC_SER_STATE_PIPE_MODE_UNIQUE  0x0001

#define _SOC_SER_MEM_MODE_PIPE_UNIQUE 1 /* Memory accessed in per pipe (unique) mode */
#define _SOC_SER_MEM_MODE_GLOBAL      0 /* Memory accessed in global (duplicate) mode */

/*
 * Multipipe entries for tables that should have the SER HW engine
 * configured to protect more than one pipeline must be
 * consecutive in the (generic) SER info structure for the same table.
 *
 * Overlay tables must also be consecutive by overlay type.
 * See trident[2].c for examples of the tables.
 */

typedef enum {
    _SOC_SER_PARITY_MODE_1BIT,
    _SOC_SER_PARITY_MODE_2BITS,
    _SOC_SER_PARITY_MODE_4BITS,
    _SOC_SER_PARITY_MODE_8BITS,
    _SOC_SER_PARITY_MODE_NUM
} _soc_ser_parity_mode_t;

typedef struct _soc_ser_parity_info_s {
    soc_mem_t               mem;
    _soc_ser_parity_mode_t  parity_mode; /* 0/1/2/3 => 1/2/4/8 bit parity */
    soc_reg_t               start_addr_reg;
    soc_reg_t               end_addr_reg;
    soc_reg_t               cmic_mem_addr_reg;
    soc_reg_t               parity_mode_reg;
    soc_field_t             parity_mode_field;
    int                     bit_offset; /* Ignored if -1 */
    soc_reg_t               entry_len_reg; /* Ignored if bit_offset is ignored */
    uint8                   cmic_ser_id;
    int                     ser_section_start;
    int                     ser_section_end;
    uint32                  ser_flags;
} _soc_ser_parity_info_t;

#define SOC_NUM_GENERIC_PROT_SECTIONS  4
typedef enum {
    _SOC_SER_PARITY_1BIT,
    _SOC_SER_ECC_1FLD = _SOC_SER_PARITY_1BIT,
    _SOC_SER_PARITY_2BITS,
    _SOC_SER_ECC_2FLD = _SOC_SER_PARITY_2BITS,
    _SOC_SER_PARITY_4BITS,
    _SOC_SER_ECC_4FLD = _SOC_SER_PARITY_4BITS,
    _SOC_SER_PARITY_8BITS
} _soc_ser_protection_mode_t;

typedef enum {
    _SOC_SER_TYPE_PARITY,
    _SOC_SER_TYPE_ECC
} _soc_ser_protection_type_t;

typedef enum {
    _SOC_SER_INTERLEAVE_NONE,
    _SOC_SER_INTERLEAVE_MOD2,
    _SOC_SER_INTERLEAVE_MOD4
} _soc_ser_interleave_type_t;

typedef struct __soc_ser_start_end_bits_s {
    int start_bit;
    int end_bit;
} __soc_ser_start_end_bits_t;

typedef struct _soc_generic_ser_info_s {
    soc_mem_t                  mem; /* last if INVALIDm */
    soc_mem_t                  alias_mem; /* share parity data with alias mem */
    _soc_ser_protection_type_t prot_type;
    _soc_ser_protection_mode_t prot_mode;
    _soc_ser_interleave_type_t intrlv_mode;
    __soc_ser_start_end_bits_t start_end_bits[SOC_NUM_GENERIC_PROT_SECTIONS];
    /* Following is also re-used for the new range disable bitmap to skip memory holes */
    uint32                     addr_mask;
    int                        ser_section_start;
    int                        ser_section_end;
    int                        ser_hw_index;
    uint32                     ser_flags; /* these are read-only */
    uint8                      num_instances; /* Valid only for _SOC_SER_FLAG_ACC_TYPE_CHK */
    uint8                      addr_start_bit; /* Valid only for _SOC_SER_FLAG_RANGE_DISABLE */
    uint32                     ser_dynamic_state; /* can change during runtime */
} _soc_generic_ser_info_t;

extern int
soc_ser_init(int unit, _soc_ser_parity_info_t *_soc_ser_parity_info,
              int max_mem);
extern int
soc_process_ser_parity_error(int unit,
                             _soc_ser_parity_info_t *_soc_ser_parity_info,
                             int parity_err_type);
extern int
soc_process_cmicm_ser_parity_error(int unit,
                             _soc_ser_parity_info_t *_soc_ser_parity_info,
                                   int parity_err_type);

extern int
soc_ser_mem_clear(int unit,
                  _soc_ser_parity_info_t *_soc_ser_parity_info,
                  soc_mem_t mem);
extern int
soc_cmicm_ser_mem_clear(int unit,
                        _soc_ser_parity_info_t *_soc_ser_parity_info,
                        soc_mem_t mem);
extern int
soc_generic_ser_mem_update(int unit, soc_mem_t mem, int mode);
extern int
soc_generic_ser_init(int unit, _soc_generic_ser_info_t *_ser_info);
extern int
soc_generic_ser_at_map_init(int unit, uint32 map[], int items);
extern int
soc_generic_ser_process_error(int unit, _soc_generic_ser_info_t *_ser_info,
                              int err_type);

extern int
soc_ser_mem_nack(void *unit_vp, void *addr_vp, void *d2,
             void *d3, void *d4);

extern void
soc_ser_fail(void *unit_vp, void *addr_vp, void *d2,
             void *d3, void *d4);
extern int
soc_ser_parity_error_intr(int unit);

extern int
soc_ser_parity_error_cmicm_intr(void *unit_vp, void *d1, void *d2,
             void *d3, void *d4);

typedef struct soc_ser_functions_s {
    void (*_soc_ser_stat_nack_f)(int, int*);
    void (*_soc_ser_fail_f)(int);
    void (*_soc_ser_mem_nack_f)(void*, void*, void*, void*, void*);
    void (*_soc_ser_parity_error_intr_f)(void*, void*, void*, void*, void*);
    void (*_soc_ser_parity_error_cmicm_intr_f)(void*, void*, void*, void*, void*);
    int  (*_soc_ser_populate_tcam_log_f)(int, soc_mem_t, soc_acc_type_t, int);
} soc_ser_functions_t;

typedef struct soc_oam_event_functions_s {
    void (*_soc_oam_event_intr_f)(void*, void*, void*, void*, void*);
} soc_oam_event_functions_t;

extern void
soc_ser_function_register(int unit, soc_ser_functions_t *functions);
extern void
soc_oam_event_function_register(int unit, soc_oam_event_functions_t* functions);
extern int
soc_ser_populate_tcam_log(int unit, soc_mem_t mem, int target_pipe, int index);
extern int
soc_ser_stat_error(int unit, int port);
extern int
soc_ser_reg_cache_init(int unit);
extern int
soc_ser_reg_cache_clear(int unit, soc_reg_t reg, int port);
extern int
soc_ser_reg_cache_set(int unit, soc_reg_t reg, int port, int index, uint64 data);
extern int
soc_ser_reg32_cache_set(int unit, soc_reg_t reg, int port, int index, uint32 data);
extern int
soc_ser_reg_cache_get(int unit, soc_reg_t reg, int port, int index, uint64 *data);
extern int
soc_ser_reg32_cache_get(int unit, soc_reg_t reg, int port, int index, uint32 *data);
extern void
soc_ser_reg_cache_info(int unit, int *count, int *size);
extern int
soc_ser_reg_load_scrub(int unit, int scrub_load);

typedef struct _soc_ser_correct_info_s {
    uint32 flags;
#define SOC_SER_SRC             0x1 /* Reg: 0, Mem: 1 */
#define SOC_SER_SRC_REG         0x0
#define SOC_SER_SRC_MEM         0x1
#define SOC_SER_REG_MEM_KNOWN   0x2 /* No decoding required */
#define SOC_SER_REG_MEM_UNKNOWN 0x0 /* Decoding required */
#define SOC_SER_ERR_HW          0x0 /* Error reported for hw lookup */
#define SOC_SER_ERR_CPU         0x4 /* Error reported for cpu access */
#define SOC_SER_ERR_MULTI       0x8
#define SOC_SER_LOG_WRITE_CACHE    0x10 /* ser_correction logic will fill 'cached' entry portion of ser_log */
#define SOC_SER_LOG_MEM_REPORTED    0x20 /* ser_correction logic will set log_mem.memory to spci.mem_reported */
#define SOC_SER_ALSO_UPDATE_SW_COUNTER  0x40 /* ser_correction logic must update both hw and sw counters */
    soc_reg_t reg;
    soc_mem_t mem;
    soc_mem_t mem_reported;
    soc_mem_t counter_base_mem;
    int counter_inst;
    soc_block_t blk_type; /* s/w block type */
    uint32 sblk; /* schan blk */
    int pipe_num; /* for multi pipe devices */
    int acc_type;
    uint32 stage;
    uint32 addr;
    int port;
    int index;
    uint8 double_bit;
    uint32 log_id;
    sal_usecs_t detect_time;
    int parity_type;
} _soc_ser_correct_info_t;

typedef struct _soc_ser_sram_info_s {
#define _SOC_SER_MAX_SRAMS              16
#define _SOC_SER_MAX_ENTRIES_PER_BKT    6
#define _SOC_SER_SRAM_CORRECTION_MODE_0 0 /* So called "original" XOR scheme */
#define _SOC_SER_SRAM_CORRECTION_MODE_1 1 /* So called "modified" XOR scheme */
    int ram_count;
    soc_mem_t view[_SOC_SER_MAX_SRAMS];
    int index_count[_SOC_SER_MAX_SRAMS];
    int mem_indexes[_SOC_SER_MAX_SRAMS][_SOC_SER_MAX_ENTRIES_PER_BKT];
    soc_reg_t force_reg;
    soc_field_t force_field;   /* Force XOR generation */
    soc_reg_t disable_reg;
    soc_field_t disable_field; /* Disable error generation on write */
} _soc_ser_sram_info_t;

extern int
soc_ser_correction(int unit, _soc_ser_correct_info_t *si);
extern int
soc_ser_sram_correction(int unit, int pipe, int sblk, int addr, soc_mem_t mem,
                        int copyno, int index, _soc_ser_correct_info_t *si);
extern int
soc_generic_ser_mem_scan_start(int unit);
extern int
soc_generic_ser_mem_scan_stop(int unit);
extern int
soc_generic_sram_mem_scan_start(int unit);
extern int
soc_generic_sram_mem_scan_stop(int unit);
extern _soc_generic_ser_info_t *
soc_mem_scan_ser_info_get(int unit);
extern int
soc_mem_alpm_aux_table_correction(int unit, int pipe, int index, soc_mem_t mem);
extern int
soc_ser_counter_info_set(int unit, soc_mem_t rst_mem,
                              uint32 hw_idx, uint32 *entry_data);
extern int
soc_ser_counter_info_get(int unit, soc_mem_t rst_mem,
                               int hw_idx, uint32 *entry_data);

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
/* FCOE Header Types */
#define SOC_FCOE_HDR_TYPE_STD     0x1 /* FC standard header     */
#define SOC_FCOE_HDR_TYPE_VFT     0x2 /* Virtual Fabric Tag     */
#define SOC_FCOE_HDR_TYPE_ENCAP   0x3 /* FC encapsulation hdr   */
#define SOC_FCOE_HDR_TYPE_IFR     0x4 /* Inter fabric routing   */

/* FCOE header type configuration */
typedef struct _soc_fcoe_header_info_s {
    int fc_hdr_type;
    int r_ctl_min;
    int r_ctl_max;
} _soc_fcoe_header_info_t;
#endif /* (BCM_TRIDENT_SUPPORT) || (BCM_TRIUMPH3_SUPPORT) */

typedef _shr_switch_temperature_monitor_t soc_switch_temperature_monitor_t;

extern int soc_do_init(int unit, int reset);
#ifdef BCM_ROBO_SUPPORT
extern int soc_robo_chip_reset(int unit);
#endif /* BCM_ROBO_SUPPORT */
extern int soc_robo_do_init(int unit, int reset);

extern int soc_esw_div64(uint64 x, uint32 y, uint32 *result);
extern int soc_esw_hw_qnum_get(int unit, int port, int cos, int *qnum);

extern int soc_ea_reset_init(int unit);
extern int soc_ea_pre_attach(int unit);
extern int soc_ea_detach(int unit);
extern int soc_ea_do_init(int unit_count);
extern int soc_is_valid_block_instance(int unit, soc_block_types_t block_types, int block_instance, int *is_valid);

extern int soc_port_pipe_get(int unit, int port, int *pipe);
/* port_types currently supported: -1(all), ETH_PORT, STK_PORT, INTLB_PORT */
extern int soc_pipe_port_get(int unit, uint32 port_type, int pipe,
                             soc_pbmp_t *pbmp);
#if defined(BCM_SAND_SUPPORT)
extern int soc_get_reg_first_block_id(int unit,soc_reg_t reg,uint32 *block);
#endif /* defined(BCM_SAND_SUPPORT) */

#ifdef BCM_NORTHSTARPLUS_SUPPORT
/* report the attached MACSEC devic ID (-1 means invalid) */
#define SOC_ROBO_NSP_MACSEC_ID_GET(unit, port)  \
        ((SOC_IS_NORTHSTARPLUS(unit) == 0) ? -1 : (((port) == 4) ? 1 : \
        (((port) == 5) ? 0 : -1)))

#define NSP_CORE_APB_ADDR               (0x3F000) /* offset from 0x18000000 */
#define NSP_MACSEC_CONFIG               (0x40C)
#define NSP_MACSEC0_BYPASS_EN_OFFSET    (0) /* bit 0 */
#define NSP_MACSEC1_BYPASS_EN_OFFSET    (1) /* bit 1 */
#define NSP_MACSEC_BYPASS_EN_LEN        (1) /* 1 bit */
#define NSP_MACSEC_BYPASS_EN_MASK       (0x1)
#define NSP_MACSEC1_BYPASS_EN_MASK      NSP_MACSEC0_BYPASS_EN_MASK

#define NSP_MACSEC_BYPASS_EN_SET(val, id, en)   \
        if ((en))   (val) |= (NSP_MACSEC_BYPASS_EN_MASK <<  \
                    (NSP_MACSEC0_BYPASS_EN_OFFSET + (id * NSP_MACSEC_BYPASS_EN_LEN)));\
        else    (val) &= ~(NSP_MACSEC_BYPASS_EN_MASK <<     \
                    (NSP_MACSEC0_BYPASS_EN_OFFSET + (id * NSP_MACSEC_BYPASS_EN_LEN)))
#define NSP_MACSEC_BYPASS_EN_GET(val, id)   \
        (((val) & (NSP_MACSEC_BYPASS_EN_MASK << \
        (NSP_MACSEC0_BYPASS_EN_OFFSET + (id * NSP_MACSEC_BYPASS_EN_LEN)))) ? \
        TRUE : FALSE)
#endif     /* BCM_NORTHSTARPLUS_SUPPORT */

#ifdef INCLUDE_MACSEC
#ifdef BCM_ROBO_SUPPORT
/* macsec bypass set/get are supported for NOPHY_MACSEC only. */
extern int soc_robo_macsec_bypass_set(int unit, soc_port_t port, uint32 value);
extern int soc_robo_macsec_bypass_get(int unit, soc_port_t port, uint32 *value);
#endif  /* BCM_ROBO_SUPPORT */
#endif     /* INCLUDE_MACSEC */

/* Generic parameters to configure a PLL */
typedef struct soc_pll_param_s {
    unsigned int ref_freq;
    unsigned int ndiv_int;
    unsigned int ndiv_frac;
    unsigned int pdiv;
    unsigned int mdiv;
    unsigned int ka;
    unsigned int ki;
    unsigned int kp;
    unsigned int vco_div2;
} soc_pll_param_t;

typedef enum {
    SOC_RCVR_CLK_SERDES_PLL,
    SOC_RCVR_CLK_MASTER_PLL,
    SOC_RCVR_CLK_XGPLL_0,
    SOC_RCVR_CLK_XGPLL_1,
    SOC_RCVR_CLK_XGPLL_2,
    SOC_RCVR_CLK_XGPLL_3,
    SOC_RCVR_CLK_EXT_PHY_0,
    SOC_RCVR_CLK_EXT_PHY_1
} soc_recov_clk_src_t;

extern int soc_switch_sync_mux_from_port(int unit, int port, int *mux);
extern int soc_switch_sync_mux_from_pbm(int unit,  pbmp_t pbm, int *mux);
extern int soc_switch_sync_mux_from_other(int unit, soc_recov_clk_src_t src, int *mux);

#ifdef CRASH_RECOVERY_SUPPORT 
#define SOC_CR_SUPPORTED(unit) (SOC_UNIT_NUM_VALID(unit) && SOC_IS_JERICHO(unit) && (SOC_CONTROL(unit))->crash_recovery)
#define SOC_CR_ENABALED(unit) (SOC_CR_SUPPORTED(unit) && SOC_IS_DONE_INIT(unit))
#define SOC_CR_ENABLE(unit)  (SOC_CONTROL(unit)->crash_recovery = TRUE)
#define SOC_CR_DISABLE(unit) (SOC_CONTROL(unit)->crash_recovery = FALSE)
#else
#define SOC_CR_SUPPORTED(unit) FALSE
#define SOC_CR_ENABALED(unit) FALSE
#endif

#endif  /* !_SOC_DRV_H */
