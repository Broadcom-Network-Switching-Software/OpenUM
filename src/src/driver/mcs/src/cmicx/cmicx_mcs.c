/*! \file cmicx_mcs.c
 *
 * CMICx-specific MCS base driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include <system.h>
#include <utils/shr/shr_debug.h>
#include <mcs_internal.h>

#ifdef CFG_MCS_INCLUDED

/*******************************************************************************
 * Local definitions
 */

/* Number of Cortex-R5. */
#define CMICX_NUM_OF_CORTEXR5              2

/* Memory space for each R5*/
#define CMICX_MCS_UC_SPACE_SIZE           (384 * 1024)

/* Disable MCS share SRAM access. */
/* MCS shared SRAM size. */
#define CMICX_MCS_SRAM_SIZE               (0)

/* R5 ATCM size. */
#define CMICX_MCS_ATCM_SIZE               (128 * 1024)

/* R5 BTCM size. */
#define CMICX_MCS_BTCM_SIZE               (256 * 1024)

/* Boot UC. */
#define CMICX_MCS_BOOT_UC                 0

/*******************************************************************************
 * CMICX specific MCS Driver
 */
static int
cmicx_mcs_mem_read32(mcs_mem_t *mem,
                     uint32 offset,
                     uint32 *data, uint32 len)
{
    uint32 addr;
    uint32 i;

    /* Parameter check. */
    if (mem == NULL) {
        return SYS_ERR_PARAMETER;
    }

    if (((offset + len) > mem->size) ||
        (offset % 4) ||
        (len % 4))
    {
        return SYS_ERR_PARAMETER;
    }

    /* Read data from memory block. */
    addr = mem->base + offset;

    for (i = 0; i < len;  i += 4) {
        data[i/4] = SYS_REG_READ32(addr);
        addr += 4;
    }

    return SYS_OK;
}

static int
cmicx_mcs_mem_write32(mcs_mem_t *mem,
                      uint32 offset,
                      const uint32 *data, uint32 len)
{
    uint32 addr;
    uint32 i;

    /* Parameter check. */
    if (mem == NULL) {
        return SYS_ERR_PARAMETER;
    }

    if (((offset + len) > mem->size) ||
        (offset % 4) ||
        (len % 4))
    {
        return SYS_ERR_PARAMETER;
    }

    /* Write data into memory block. */
    addr = mem->base + offset;

    for (i = 0; i < len;  i += 4) {
         SYS_REG_WRITE32(addr, data[i/4]);
         addr += 4;
    }

    return SYS_OK;
}
static int
cmicx_mcs_uc_start_get(int unit, uint32 uc, bool *start)
{
    MHOST_CR5_RST_CTRLr_t r5_ctrl;
    bool halt;
    bool rst_n;

    /* Check R5. */
    READ_MHOST_CR5_RST_CTRLr(unit, uc, r5_ctrl);
    halt = MHOST_CR5_RST_CTRLr_CPU_HALT_Nf_GET(r5_ctrl);
    rst_n = (MHOST_CR5_RST_CTRLr_PRESET_DBG_Nf_GET(r5_ctrl) &&
             MHOST_CR5_RST_CTRLr_DBG_RESET_Nf_GET(r5_ctrl) &&
             MHOST_CR5_RST_CTRLr_RESET_Nf_GET(r5_ctrl) &&
             MHOST_CR5_RST_CTRLr_SYS_PORESET_Nf_GET(r5_ctrl));

    *start = (halt == 0) && (rst_n == 1);
    return SYS_OK;
}

static int
cmicx_mcs_uc_start_set(int unit, uint32 uc, bool start)
{
    MHOST_CR5_RST_CTRLr_t r5_ctrl;

    if (uc == CMICX_MCS_BOOT_UC) {
        return SYS_ERR;
    }

    if (start) {
        sal_usleep(10000);
        READ_MHOST_CR5_RST_CTRLr(unit, uc, r5_ctrl);
        MHOST_CR5_RST_CTRLr_CPU_HALT_Nf_SET(r5_ctrl, 1);
        WRITE_MHOST_CR5_RST_CTRLr(unit, uc, r5_ctrl);

    } else {

        MHOST_CR5_RST_CTRLr_CLR(r5_ctrl);
        WRITE_MHOST_CR5_RST_CTRLr(unit, uc, r5_ctrl);
        sal_usleep(10000);

        MHOST_CR5_RST_CTRLr_PRESET_DBG_Nf_SET(r5_ctrl, 1);
        MHOST_CR5_RST_CTRLr_DBG_RESET_Nf_SET(r5_ctrl, 1);
        MHOST_CR5_RST_CTRLr_RESET_Nf_SET(r5_ctrl, 1);
        MHOST_CR5_RST_CTRLr_SYS_PORESET_Nf_SET(r5_ctrl, 1);
        WRITE_MHOST_CR5_RST_CTRLr(unit, uc, r5_ctrl);
    }

    return SYS_OK;
}

static int
cmicx_mcs_init(int unit) {

    uint32 uc;
    mcs_dev_t *dev;

    SHR_FUNC_ENTER(unit);

    SHR_IF_ERR_EXIT
        (mcs_dev_get(unit, &dev));

    for (uc = 0; uc < dev->num_of_uc; uc++) {
        /* Reset each R5. */
        cmicx_mcs_uc_start_set(unit, uc, false);
    }

exit:
    SHR_FUNC_EXIT();
}

static int
cmicx_mcs_cleanup(int unit)
{
    return SYS_OK;
}

/* CMICx-MCS driver. */
static const mcs_drv_t cmicx_mcs_base_drv = {

    /*! Initialize MCS driver. */
    .init         = cmicx_mcs_init,

    /*! Cleanup MCS driver. */
    .cleanup      = cmicx_mcs_cleanup,

    /*! Reset uC (Cortex-R5). */
    .uc_start_set = cmicx_mcs_uc_start_set,

    /*! Get uC (Cortex-R5) is started or not. */
    .uc_start_get = cmicx_mcs_uc_start_get,
};

/*******************************************************************************
 * Public Functions.
 */
const mcs_drv_t *
cmicx_mcs_base_drv_get(int unit)
{
    return &cmicx_mcs_base_drv;
}


/*!
 * \brief Initial device data for CMICx-MCS driver.
 *
 * \param [in]  unit Unit number.
 *
 * \retval SYS_OK Successed.
 * \retval SYS_ERR Failed.
 */
int
cmicx_mcs_dev_init(int unit) {

    mcs_dev_t *dev;
    mcs_mem_t *tcm;
    uint32 uc;

    SHR_FUNC_ENTER(unit);

    SHR_IF_ERR_EXIT
        (mcs_dev_get(unit, &dev));

    dev->num_of_uc = CMICX_NUM_OF_CORTEXR5;

    dev->sram.base = 0;
    dev->sram.size = CMICX_MCS_SRAM_SIZE;
    dev->sram.write32 = cmicx_mcs_mem_write32;
    dev->sram.read32 = cmicx_mcs_mem_read32;
    dev->sram.unit = unit;

    /* Initialize CR5 TCM memory objects. */
    for (uc = 0; uc < dev->num_of_uc; uc++) {
        tcm = &dev->tcm[uc];
        if (uc == CMICX_MCS_BOOT_UC) {
            sal_memset(tcm, 0, sizeof(mcs_mem_t));
            continue;
        }
        sal_memcpy(tcm, &dev->sram, sizeof(mcs_mem_t));
        tcm->base = dev->base_addr + (CMICX_MCS_UC_SPACE_SIZE * uc);
        tcm->size = CMICX_MCS_ATCM_SIZE + CMICX_MCS_BTCM_SIZE;
    }

exit:
    SHR_FUNC_EXIT();
}

#endif /* CFG_MCS_INCLUDED */
