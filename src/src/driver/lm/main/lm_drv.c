/*! \file lm_drv.c
 *
 * Link Manager driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include <system.h>
#include <lm_drv_internal.h>
#include <utils/shr/shr_debug.h>

/******************************************************************************
* Local definitions
 */

/* Linkscan driver */
static lm_drv_t *lm_drv[CONFIG_MAX_UNITS];

#define LM_CALL(_ld, _lf, _la) \
        ((_ld) == 0 ? SYS_ERR_INIT: \
              ((_ld)->_lf == 0 ? SYS_OK: (_ld)->_lf _la))

#define LINKSCAN_HW_CONFIG(_u, _hw) \
        LM_CALL(lm_drv[_u], hw_config, ((_u), (_hw)))

#define LINKSCAN_HW_LINK_GET(_u, _hw_link) \
        LM_CALL(lm_drv[_u], hw_link_get, ((_u), (_hw_link)))

#define LINKSCAN_HW_INTR_CB_SET(_u, _intr_func) \
        LM_CALL(lm_drv[_u], hw_intr_cb_set, ((_u), (_intr_func)))

#define LINKSCAN_HW_SCAN_STOP(_u)    \
        LM_CALL(lm_drv[_u], hw_scan_stop, (_u))

#define LINKSCAN_HW_SCAN_START(_u)    \
        LM_CALL(lm_drv[_u], hw_scan_start, (_u))

/******************************************************************************
* Private functions
 */

/******************************************************************************
* Public functions
 */
int
lm_drv_attach(int unit, lm_drv_t *drv)
{
    SHR_FUNC_ENTER(unit);

    lm_drv[unit] = drv;

    SHR_FUNC_EXIT();
}

int
lm_hw_init(int unit)
{
    SHR_FUNC_ENTER(unit);

    if (lm_drv[unit] && lm_drv[unit]->hw_init) {
        SHR_IF_ERR_EXIT
            (lm_drv[unit]->hw_init(unit));
    }

exit:
    SHR_FUNC_EXIT();
}

int
lm_hw_cleanup(int unit)
{
    SHR_FUNC_ENTER(unit);

    if (lm_drv[unit] && lm_drv[unit]->hw_cleanup) {
        SHR_IF_ERR_EXIT
            (lm_drv[unit]->hw_cleanup(unit));
    }

exit:
    SHR_FUNC_EXIT();
}

int
lm_hw_config(int unit, pbmp_t hw)
{
    int pport, lport;
    pbmp_t ppbm;

    SHR_FUNC_ENTER(unit);

    PBMP_CLEAR(ppbm);

    PBMP_ITER(hw, lport) {
        pport = SOC_PORT_L2P_MAPPING(lport);
        if (pport < 0) {
            continue;
        }
        PBMP_PORT_ADD(ppbm, pport);
    }

    SHR_IF_ERR_EXIT
        (LINKSCAN_HW_SCAN_STOP(unit));

    SHR_IF_ERR_EXIT
        (LINKSCAN_HW_CONFIG(unit, ppbm));

    if (PBMP_NOT_NULL(ppbm)) {
        SHR_IF_ERR_EXIT
            (LINKSCAN_HW_SCAN_START(unit));
    }

exit:
    SHR_FUNC_EXIT();
}

int
lm_hw_link_get(int unit, pbmp_t *hw)
{
    int lport, pport;
    pbmp_t ppbm;

    SHR_FUNC_ENTER(unit);

    PBMP_CLEAR(*hw);
    PBMP_CLEAR(ppbm);
    SHR_IF_ERR_EXIT
        (LINKSCAN_HW_LINK_GET(unit, &ppbm));

    PBMP_ITER(ppbm, pport) {
        lport = SOC_PORT_P2L_MAPPING(pport);
        if (lport != INVALID_LPORT) {
            PBMP_PORT_ADD(*hw, lport);
        }
    }

exit:
    SHR_FUNC_EXIT();
}

int
lm_hw_intr_cb_set(int unit, sys_intr_f cb)
{
    SHR_FUNC_ENTER(unit);

    SHR_IF_ERR_EXIT
        (LINKSCAN_HW_INTR_CB_SET(unit, cb));

exit:
    SHR_FUNC_EXIT();
}

