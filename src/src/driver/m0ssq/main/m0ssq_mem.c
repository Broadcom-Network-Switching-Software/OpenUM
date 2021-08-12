/*! \file m0ssq_shmem.c
 *
 * M0SSQ shared memory driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>

#include <utils/shr/shr_debug.h>

#include <m0ssq_internal.h>
#include <m0ssq_mem.h>
#include <m0ssq_fifo_internal.h>
#include <m0ssq_mbox_internal.h>

/*******************************************************************************
 * Local definitions
 */
/*! Linkscan shared memory size. */
#define CMICX_LS_SHMEM_SIZE                    64

/*! Linkscan shared memory offset. */
#define CMICX_LS_SHMEM_OFFSET_DEFAULT          512

/*! LED shared memory size. */
#define CMICX_LED_SHMEM_SIZE                   (2048 + 64)

/*! LED shared memory offset. */
#define CMICX_LED_SHMEM_OFFSET_DEFAULT         (2048 + 512)

/*******************************************************************************
 * Public Functions
 */
void
m0ssq_mem_free(m0ssq_mem_t *pmem)
{
    if (pmem == NULL) {
        return;
    }
    sal_free(pmem);
}

int
m0ssq_mem_get(int unit, int uc, char *name, m0ssq_mem_t **pmem)
{
    m0ssq_dev_t *dev;
    m0ssq_mem_t *tcm;
    m0ssq_mem_t *mem;
    SHR_FUNC_ENTER(unit);

    SHR_NULL_CHECK(pmem, SYS_ERR_PARAMETER);
    SHR_NULL_CHECK(name, SYS_ERR_PARAMETER);
    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    SHR_IF_ERR_EXIT
        (m0ssq_dev_get(unit, &dev));

    if (uc >= dev->num_of_uc) {
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    tcm = &(dev->m0_tcm[uc]);

    mem = sal_alloc(sizeof(m0ssq_mem_t), "SocIprocM0ssqShmem");
    if (mem == NULL) {
        SHR_ERR_EXIT(SYS_ERR_OUT_OF_RESOURCE);
    }

    tcm = &(dev->m0_tcm[uc]);
    sal_memcpy(mem, tcm, sizeof(m0ssq_mem_t));

    /* Static allocation of linkscan shmem. */
    if (sal_strncmp(name, "linkscan", 8) == 0) {

        mem->size = CMICX_LS_SHMEM_SIZE;
        mem->base = tcm->base + tcm->size - CMICX_LS_SHMEM_OFFSET_DEFAULT;
        mem->unit = unit;

    /* Static allocation of led shmem. */
    } else if (sal_strncmp(name, "led", 3) == 0) {

        mem->size = CMICX_LED_SHMEM_SIZE;
        mem->base = tcm->base + tcm->size - CMICX_LED_SHMEM_OFFSET_DEFAULT;
        mem->unit = unit;

    } else {
        sal_free(mem);
        *pmem = NULL;
        return SYS_ERR_UNAVAIL;
    }

    *pmem = mem;
    SHR_LOG_DEBUG("%s\n", name);
    SHR_LOG_DEBUG("p = %x\n", mem);
    SHR_LOG_DEBUG("size = %x\n", mem->size);
    SHR_LOG_DEBUG("base = %x\n", mem->base);
exit:
    if (SHR_FUNC_ERR()) {
        if (mem) {
            sal_free(mem);
            *pmem = NULL;
        }
    }
    SHR_FUNC_EXIT();
}

int
m0ssq_mem_read32(m0ssq_mem_t *mem, uint32 offset, uint32 *data)
{
    uint32 addr;
    int unit;
    int rv;

    /* Parameter check. */
    if (mem == NULL) {
        return SYS_ERR_PARAMETER;
    }

    unit = mem->unit;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR;
    }

    if (((offset + 4) > mem->size) ||
        (offset % 4))
    {
        return SYS_ERR_PARAMETER;
    }

    /* Read data from memory block. */
    addr = mem->base + offset;

    rv = mem->read32(mem->unit, addr, data);
    if (SHR_FAILURE(rv)) {
        return SYS_ERR;
    };

    return SYS_OK;
}

int
m0ssq_mem_write32(m0ssq_mem_t *mem, uint32 offset, uint32 data)
{
    uint32 addr;
    int unit;
    int rv;

    /* Parameter check. */
    if (mem == NULL) {
        return SYS_ERR_PARAMETER;
    }

    unit = mem->unit;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR;
    }

    if (((offset + 4) > mem->size) ||
        (offset % 4))
    {
        return SYS_ERR_PARAMETER;
    }

    /* Write data into memory block. */
    addr = mem->base + offset;

    rv = mem->write32(mem->unit, addr, data);
    if (SHR_FAILURE(rv)) {
        return rv;
    }

    return SYS_OK;
}

int
m0ssq_mem_clear(m0ssq_mem_t *mem)
{
    uint32 offset;
    int rv;

    /* Parameter check. */
    if (mem == NULL) {
        return SYS_ERR_PARAMETER;
    }

    if (!UNIT_VALID(mem->unit)) {
        return SYS_ERR;
    }

    for (offset = 0; offset < mem->size; offset += 4) {
         rv = m0ssq_mem_write32(mem, offset, 0);
         if (SHR_FAILURE(rv)) {
             return rv;
         }
    }

    return SYS_OK;
}

int
m0ssq_mem_write(m0ssq_mem_t *mem, uint32 offset,
                const uint8 *data, int len)
{

    uint32 addr;
    int unit;

    /* Parameter check. */
    if (mem == NULL) {
        return SYS_ERR_PARAMETER;
    }

    unit = mem->unit;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR;
    }

    if (((offset + len) > mem->size) ||
        (offset % 4))
    {
        return SYS_ERR_PARAMETER;
    }

    /* Write data into memory block. */
    addr = mem->base + offset;

    /* Handle unaligned bytes from the beginning */
    if (addr % 4) {
        uint32 aligned = addr & ~3;
        uint32 val;
        int i;

        /* Read original 32-bit value from aligned address */
        mem->read32(unit, aligned, &val);

        /* Replace the value with partial bytes from the data */
        for (i = 1; i < 4 && len > 0; i++) {
            if (i == (int)(addr % 4)) {
                int shift = 8 * i;
                /* M0 is little endian */
                val &= ~(0xffUL << shift);
                val |= (uint32)(*data) << shift;
                data++;
                addr++;
                len--;
            }
        }

        /* Write the 32-bit value to the original aligned address */
        mem->write32(unit, aligned, val);
    }

    /* Handle aligned data */
    while (len > 4) {
        /* Convert 32-bit value to bytes (in little endian for M0)*/
        uint32 val = (((uint32)data[0]) |
                      ((uint32)data[1] <<  8) |
                      ((uint32)data[2] << 16) |
                      ((uint32)data[3] << 24));

        /* Write the 32-bit value to memory */
        mem->write32(unit, addr, val);

        data += 4;
        addr += 4;
        len -= 4;
    }

    /* Handle remaining data (less than 4 bytes) */
    if (len > 0) {
        uint32 val;
        int i;

        /* Read original 32-bit value from aligned address */
        mem->read32(unit, addr, &val);

        /* Replace the value with partial bytes from the data */
        for (i = 0; i < 4 && len > 0; i++) {
            int shift = 8 * i;
            /* M0 is little endian */
            val &= ~(0xffUL << shift);
            val |= (uint32)(*data) << shift;
            data ++;
            len --;
        }

        /* Write the 32-bit value to the original aligned address */
        mem->write32(unit, addr, val);
    }

    return SYS_OK;
}

int
m0ssq_mem_read(m0ssq_mem_t *mem, uint32 offset,
               uint8 *data, int len)
{
    uint32 addr;
    int unit;

    /* Parameter check. */
    if (mem == NULL) {
        return SYS_ERR_PARAMETER;
    }

    unit = mem->unit;

    if (!UNIT_VALID(unit)) {
        return SYS_ERR;
    }

    if (((offset + len) > mem->size) ||
        (offset % 4))
    {
        return SYS_ERR_PARAMETER;
    }

    /* Write data into memory block. */
    addr = mem->base + offset;

    /* Handle unaligned bytes from the beginning */
    if (addr % 4) {
        uint32 val;
        int i;

        /* Read 32-bit value from aligned address */
        mem->read32(unit, (addr & ~3), &val);

        /* Read needed bytes from the 32-bit value */
        for (i = 1; i < 4 && len > 0; i++) {
            /* Shift needed byte to the lowest. Note that M0 is little endian */
            val >>= 8;
            if (i == (int)(addr % 4)) {
                *data = (uint8)val;
                data ++;
                addr ++;
                len --;
            }
        }
    }

    /* Handle remaining data */
    while (len > 0) {
        uint32 val;
        int i;

        /* Read 32-bit value from memory */
        mem->read32(unit, addr, &val);

        /* Convert 32-bit value to bytes */
        for (i = 0; i < 4 && len > 0; i++) {
            *data = (uint8)val;
            data ++;
            addr ++;
            len --;
            /* M0 is little endian */
            val >>= 8;
        }
    }

    return SYS_OK;
}
