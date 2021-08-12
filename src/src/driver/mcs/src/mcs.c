/*! \file mcs.c
 *
 * Multiple Controller System (MCS) driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#include <utils/shr/shr_debug.h>
#include <mcs_internal.h>
#include <sal.h>

#ifdef CFG_MCS_INCLUDED

/*******************************************************************************
 * Local definition.
 */
#define MAX_UNITS                      1

#define DRV_CALL(_d, _f, _a) \
        ((_d) == NULL ? SYS_ERR_UNAVAIL: \
              ((_d)->_f == NULL ? SYS_ERR_UNAVAIL: (_d)->_f _a))

#define MCS_INIT(_u) \
        DRV_CALL(mcs_drv[_u], init, ((_u)))

#define MCS_UC_START_SET(_u, _uc, _start) \
        DRV_CALL(mcs_drv[_u], uc_start_set, \
                     ((_u), (_uc), (_start)))

#define MCS_UC_START_GET(_u, _uc, _start) \
        DRV_CALL(mcs_drv[_u], uc_start_get, \
                 ((_u), (_uc), (_start)))

/*******************************************************************************
 * Local variables.
 */
static const mcs_drv_t *mcs_drv[MAX_UNITS] = { NULL };
static mcs_dev_t *mcs_dev[MAX_UNITS] = { NULL };

/*******************************************************************************
 * Public functions
 */
int
mcs_drv_init(int unit, const mcs_drv_t *drv)
{
    SHR_FUNC_ENTER(unit);

    if ((uint32) unit >= MAX_UNITS) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    mcs_drv[unit] = drv;

exit:
    SHR_FUNC_EXIT();
}

int
mcs_dev_get(int unit, mcs_dev_t **dev)
{
    SHR_FUNC_ENTER(unit);

    if ((uint32) unit >= MAX_UNITS) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    *dev = mcs_dev[unit];
    SHR_NULL_CHECK(*dev, SYS_ERR_UNAVAIL);

exit:
    SHR_FUNC_EXIT();
}

int
mcs_uc_num_get(int unit)
{
    mcs_dev_t *dev;

    if ((uint32) unit >= MAX_UNITS) {
        return 0;
    }

    dev = mcs_dev[unit];
    if (dev) {
        return dev->num_of_uc;
    }

    return 0;
}

int
mcs_uc_fw_load(int unit, uint32 uc, uint32 offset,
               const uint8_t *buf, uint32 len)
{
    uint32 wlen;
    uint32 *ptr = NULL;
    uint32 i;
    mcs_mem_t *mem;
    mcs_dev_t *dev = NULL;

    SHR_FUNC_ENTER(unit);

    if ((uint32) unit >= MAX_UNITS) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    dev = mcs_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR_UNAVAIL);

    /* Make wlen as multiple of a 32 bits word. */
    wlen = (len + 3) & ~(0x3);

    /*
     * Allocate a temporary buffer to convert uint8_t input buffer
     * to uint32 buffer for bottom driver use.
     */
    ptr = sal_malloc(wlen);
    SHR_NULL_CHECK(ptr, SYS_ERR_OUT_OF_RESOURCE);

    /*
     * CR5 is litte endian processor.
     * Assume that input buffer byte order is little endian.
     */
    for (i = 0; i < wlen; i += 4) {
         ptr[i/4] = ((uint32) buf[i]) |
                    ((uint32) buf[i + 1] << 8) |
                    ((uint32) buf[i + 2] << 16) |
                    ((uint32) buf[i + 3] << 24);
    }

    /* Get uC program memory object. */
    mem = &dev->tcm[uc];

    /* Write firmware binary into program memory. */
    SHR_IF_ERR_EXIT
        (mem->write32(mem, offset, ptr, wlen));

    sal_usleep(10000);
exit:
    sal_free(ptr);
    SHR_FUNC_EXIT();
}

int
mcs_uc_start(int unit, uint32 uc)
{
    mcs_dev_t *dev;

    SHR_FUNC_ENTER(unit);

    if ((uint32) unit >= MAX_UNITS) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    dev = mcs_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR_UNAVAIL);

    if (uc >= dev->num_of_uc) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    /* Let uc start running. */
    SHR_IF_ERR_EXIT(MCS_UC_START_SET(unit, uc, true));

exit:
    SHR_FUNC_EXIT();
}

int
mcs_uc_reset(int unit, uint32 uc)
{
    mcs_dev_t *dev;

    SHR_FUNC_ENTER(unit);

    if ((uint32) unit >= MAX_UNITS) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    dev = mcs_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR_UNAVAIL);

    if (uc >= dev->num_of_uc) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    /* Let uc stop running. */
    SHR_IF_ERR_EXIT
        (MCS_UC_START_SET(unit, uc, false));

exit:
    SHR_FUNC_EXIT();
}

int
mcs_uc_start_get(int unit, uint32 uc, bool *start)
{
    SHR_FUNC_ENTER(unit);

    if ((uint32) unit >= MAX_UNITS) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    SHR_IF_ERR_EXIT
        (MCS_UC_START_GET(unit, uc, start));

exit:
    SHR_FUNC_EXIT();
}

int
mcs_uc_mem_read32(int unit, uint32 uc,
                  uint32 offset, uint32 *data32)
{
    mcs_dev_t *dev;
    mcs_mem_t *mem;

    SHR_FUNC_ENTER(unit);

    if ((uint32) unit >= MAX_UNITS) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    dev = mcs_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR_UNAVAIL);

    mem = &dev->tcm[uc];
    SHR_IF_ERR_EXIT(mem->read32(mem, offset, data32, 4));

exit:
    SHR_FUNC_EXIT();
}

int
mcs_uc_mem_write32(int unit, uint32 uc,
                   uint32 offset, uint32 data32)
{
    mcs_dev_t *dev;
    mcs_mem_t *mem;

    SHR_FUNC_ENTER(unit);

    if ((uint32) unit >= MAX_UNITS) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    dev = mcs_dev[unit];
    SHR_NULL_CHECK(dev, SYS_ERR_UNAVAIL);

    mem = &dev->tcm[uc];
    SHR_IF_ERR_EXIT(mem->write32(mem, offset, &data32, 4));

exit:
    SHR_FUNC_EXIT();
}

int
mcs_init(int unit)
{
    mcs_dev_t *dev = NULL;

    SHR_FUNC_ENTER(unit);

    if ((uint32) unit >= MAX_UNITS) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    if (mcs_drv[unit] == NULL) {
        SHR_EXIT();
    }

    /* Allocate device-specific data. */
    dev = sal_malloc(sizeof(mcs_dev_t));
    SHR_NULL_CHECK(dev, SYS_ERR_OUT_OF_RESOURCE);

    /* Install device-specific data. */
    mcs_dev[unit] = dev;

    /* Initialize chip specific part. */
    SHR_IF_ERR_EXIT(MCS_INIT(unit));

exit:
    if (SHR_FUNC_ERR()) {
        /* Uninstall device-specific data. */
        mcs_dev[unit] = NULL;
        sal_free(dev);
    }
    SHR_FUNC_EXIT();
}

int
mcs_cleanup(int unit)
{
    mcs_dev_t *dev = NULL;

    SHR_FUNC_ENTER(unit);

    if ((uint32) unit >= MAX_UNITS) {
        SHR_ERR_EXIT(SYS_ERR_UNAVAIL);
    }

    if (mcs_drv[unit] == NULL) {
        SHR_EXIT();
    }

    dev = mcs_dev[unit];
    SHR_NULL_CHECK(dev, SYS_OK);

    /* Cleanup MCS driver. */
    mcs_dev[unit] = NULL;

exit:
    /* Free device-specific data. */
    if (dev != NULL) {
        sal_free(dev);
    }
    SHR_FUNC_EXIT();
}

int
mcs_mem_clear(mcs_mem_t *mem)
{
    uint32 zero32 = 0;
    uint32 offset;
    int rv;

    /* Parameter check. */
    if (mem == NULL) {
        return SYS_ERR_PARAMETER;
    }

    if (mem->write32 == NULL) {
        return SYS_ERR_UNAVAIL;
    }

    for (offset = 0; offset < mem->size; offset += 4) {
         rv = mem->write32(mem, offset, &zero32, 4);
         if (SHR_FAILURE(rv)) {
             return rv;
         }
    }

    return SYS_OK;
}
#endif /* CFS_MCS_INCLUDED */
