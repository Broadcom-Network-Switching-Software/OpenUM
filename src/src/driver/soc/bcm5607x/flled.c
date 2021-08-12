/*! \file flled.c
 *
 * Device-specific LED driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */
#include <system.h>
#include <utils/shr/shr_debug.h>
#include <led_internal.h>
#include <cmicx_led_internal.h>
#include <cmicx_led.h>

/*******************************************************************************
 * Local defines.
 */
#define SWITCH_CORE_FREQ_HZ    (1500 * 1000000)
#define M0SSQ_FREQ_HZ          (1000 * 1000000)
#define LED_REFRESH_RATE_HZ    (30)
#define LED_LAST_PORT          (0)
#define LED_CLK_HZ             (5 * 1000000)
#define LED_PORT_OFFSET        (2)

/*******************************************************************************
 * Local variables.
 */

/* Device-specific LED driver. */
static led_drv_t bcm5607x_led_drv;

/*******************************************************************************
 * Device-specific driver.
 */
static int
bcm5607x_led_init(int unit)
{
    const led_drv_t *base_drv;
    led_dev_t *dev;
    int port;

    SHR_FUNC_ENTER(unit);

    /* Get CMICx-LED base driver. */
    base_drv = cmicx_led_base_drv_get(unit);
    SHR_NULL_CHECK(base_drv, SYS_ERR_UNAVAIL);

    /* Get LED chip specific data. */
    SHR_IF_ERR_EXIT
        (led_dev_get(unit, &dev));

    /* Initialize LED refresh rate. */
    SHR_IF_ERR_EXIT
        (cmicx_led_refresh_freq_set(unit, SWITCH_CORE_FREQ_HZ,
                                          LED_REFRESH_RATE_HZ));

    /* Initialize LED output clock rate. */
    SHR_IF_ERR_EXIT
        (cmicx_led_clk_freq_set(unit, M0SSQ_FREQ_HZ, LED_CLK_HZ));

    /* Last port setting of LED. */
    SHR_IF_ERR_EXIT
        (cmicx_led_last_port_set(unit, LED_LAST_PORT));

    /* Invoke CMICx-LED base driver init function. */
    SHR_IF_ERR_EXIT
        (base_drv->init(unit));

    /* Initialize the internal Firelight led port mapping  */
    SOC_PPORT_ITER(port) {
        dev->pport_to_led_uc_port[port] = port - LED_PORT_OFFSET;
        dev->pport_to_led_uc[port] = 0;
    }

exit:
    SHR_FUNC_EXIT();
}

/*******************************************************************************
 * Public functions.
 */
int
bcm5607x_led_drv_init(int unit)
{
    const led_drv_t *base_drv;

    SHR_FUNC_ENTER(unit);

    if (!UNIT_VALID(unit)) {
        SHR_ERR_EXIT(SYS_ERR_PARAMETER);
    }

    /* Get CMICx-LED base driver. */
    base_drv = cmicx_led_base_drv_get(unit);
    SHR_NULL_CHECK(base_drv, SYS_ERR_UNAVAIL);

    /* Inherit driver methods from LED base driver. */
    sal_memcpy(&bcm5607x_led_drv, base_drv, sizeof(led_drv_t));

    /* Override with chip-specific driver methods. */
    bcm5607x_led_drv.init = bcm5607x_led_init;

    /* Install driver methods. */
    SHR_IF_ERR_EXIT
        (led_drv_init(unit, &bcm5607x_led_drv));

    SHR_IF_ERR_EXIT
        (led_init(unit));
exit:
    SHR_FUNC_EXIT();
}

int bcm5607x_led_drv_cleanup(int unit)
{
    return led_drv_init(unit, NULL);
}
