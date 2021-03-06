/*
 * $Id: fllinkscan.c,v 1.11 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>
#include <pcm/pcm_int.h>
#include <boardapi/led.h>
#include <cmicx_customer_led_common.h>

#define LINKSCAN_INTERVAL        (100000UL)   /* 100 ms */
#define PORT_LINK_UP                    (1)
#define PORT_LINK_DOWN                  (0)

#if CONFIG_EMULATION
int link_qt[BCM5607X_LPORT_MAX+1];

void
bcm5607x_link_change(uint8 port)
{
    link_qt[port] ^= 1;
}
#endif
void
bcm5607x_handle_link_up(uint8 unit, uint8 lport, int changed, uint32 *flags)
{
    int tx_pause, rx_pause;
    int duplex;
    int speed;
    int an = 0;
    if (1 == changed) {
        /* Port changes to link up from link down */
        pcm_port_update(0, lport, 1);
        pcm_port_autoneg_get(unit,lport, &an);
        pcm_port_speed_get(unit, lport, &speed);
        pcm_port_duplex_get(unit, lport, &duplex);
        pcm_port_pause_get(unit, lport, &tx_pause, &rx_pause);

        SOC_PORT_LINK_STATUS(lport) = TRUE;
        SOC_PORT_SPEED_STATUS(lport) = speed;
        SOC_PORT_DUPLEX_STATUS(lport) = duplex ? TRUE : FALSE;
        SOC_PORT_AN_STATUS(lport) = an ? TRUE : FALSE;
        SOC_PORT_TX_PAUSE_STATUS(lport) = tx_pause;
        SOC_PORT_RX_PAUSE_STATUS(lport) = rx_pause;
#if defined(CFG_SWITCH_EEE_INCLUDED)
        {
            pcm_port_ability_t remote_ability;
            /* In the case of Native EEE mode, neet to enable MAC EEE after link + 1 sec    */
            /* check if the port is auto-neg, need to use remote eee */
            /* use bit 12 to show the remote eee is enable(1) or not(0) */
            pcm_port_ability_remote_get(unit, lport, &remote_ability);
            if ((an) && (remote_ability.eee)) {
                /* Enable EEE in UMAC_EEE_CTRL register after one second
                 * if EEE is enabled in S/W database
                 */
                fl_sw_info.link_up_time[lport] = sal_get_ticks();
                fl_sw_info.need_process_for_eee_1s[lport] = TRUE;
            }
        }
#endif /* CFG_SWITCH_EEE_INCLUDED */

#if CFG_CONSOLE_ENABLED
        if (board_linkup_message) {
            uint16 uport = 0;
            board_lport_to_uport(unit, lport, &uport);
            sal_printf("\nU-port %d (L-port:%d,P-port:%d), speed = %d, duplex = %d, tx_pause = %d, rx_pause = %d, an = %d\n",
                   uport, lport, SOC_PORT_L2P_MAPPING(lport), speed, duplex, tx_pause, rx_pause, an);
        }
#endif /* CFG_CONSOLE_ENABLED */

      fl_sw_info.link[lport] = PORT_LINK_UP;

    } else {
        /* Port stays in link up state */
#if defined(CFG_SWITCH_EEE_INCLUDED)
        /* EEE one second delay for link up timer check */
        if ((fl_sw_info.need_process_for_eee_1s[lport]) &&
            (SAL_TIME_EXPIRED(fl_sw_info.link_up_time[lport], 1000))) {
#if CFG_CONSOLE_ENABLED
            SAL_DEBUGF(("EEE : enable eee for port %d\n", lport));
#endif /* CFG_CONSOLE_ENABLED */
            bcm5607x_port_eee_enable_set(unit, lport, TRUE, TRUE);
            fl_sw_info.need_process_for_eee_1s[lport] = FALSE;
        }
#endif /* CFG_SWITCH_EEE_INCLUDED */

    }

}

void
bcm5607x_handle_link_down(uint8 unit, uint8 lport, int changed)
{
    int loopback = PCM_PORT_LOOPBACK_NONE;

#ifdef CFG_LOOPBACK_TEST_ENABLED
    if (1 == changed) {
#if CFG_CONSOLE_ENABLED
        if (board_linkdown_message) {
            uint16 uport = 0;
            board_lport_to_uport(unit, lport, &uport);
            sal_printf("\nU-port %d (L-port:%d,P-port:%d) goes down!\n",
                        uport, lport, SOC_PORT_L2P_MAPPING(lport));
        }
#endif /* CFG_CONSOLE_ENABLED */
        fl_sw_info.link[lport] = PORT_LINK_DOWN;
    }
#else
    if (1 == changed) {
        pcm_port_loopback_get(unit, lport, &loopback);

        if (loopback == PCM_PORT_LOOPBACK_NONE) {
            SOC_PORT_LINK_STATUS(lport) = FALSE;

            /* Port changes to link down from link up */
            pcm_port_update(unit, lport, 0);
        }

#if CFG_CONSOLE_ENABLED
        if (board_linkdown_message) {
            uint16 uport = 0;
            board_lport_to_uport(unit, lport, &uport);
            sal_printf("\nU-port %d (L-port:%d,P-port:%d) goes down!\n",
                        uport, lport, SOC_PORT_L2P_MAPPING(lport));
        }
#endif /* CFG_CONSOLE_ENABLED */


#if defined(CFG_SWITCH_EEE_INCLUDED)
        /* Disable EEE in UMAC_EEE_CTRL register if EEE is enabled in S/W database */
#if CFG_CONSOLE_ENABLED
         SAL_DEBUGF(("EEE : disable eee for lport %d\n", lport));
#endif /* CFG_CONSOLE_ENABLED */
         bcm5607x_port_eee_enable_set(unit, lport, FALSE, TRUE);
         fl_sw_info.need_process_for_eee_1s[lport] = FALSE;

#endif /* CFG_SWITCH_EEE_INCLUDED */
         fl_sw_info.link[lport] = PORT_LINK_DOWN;

    } else {
        /* Port stays in link down state */
    }
#endif /* CFG_LOOPBACK_TEST_ENABLED */

}

/*
 *  Function : bcm5607x_linkscan_task
 *
 *  Purpose :
 *      Update the link status of switch ports.
 *
 *  Parameters :
 *
 *  Return :
 *
 *  Note :
 */
void
bcm5607x_linkscan_task(void *param)
{
    uint8 unit = 0, lport;
    uint32 flags;
    int link;

    if (board_linkscan_disable) {
        return;
    }

    SOC_LPORT_ITER(lport) {
        sys_error_t r = 0;

        /* Do polled irq at task top layer to improve polled irq response time. */
        POLLED_IRQ();

        r = pcm_phyctrl_port_link_status_get(unit, lport, &link);
        if (r < 0) {
            continue;
        }

        if (link == PORT_LINK_UP) {
            /* Link up */
            flags = 0;
            bcm5607x_handle_link_up(unit, lport,
                (fl_sw_info.link[lport] == PORT_LINK_DOWN) ? TRUE : FALSE, &flags);
        } else {
            bcm5607x_handle_link_down(unit, lport,
                (fl_sw_info.link[lport] == PORT_LINK_UP) ? TRUE : FALSE);
        }
    }

}

sys_error_t
bcm5607x_link_status(uint8 unit, uint8 port, BOOL *link)
{
    if (link == NULL || unit > 0 || port > BCM5607X_LPORT_MAX) {
    return SYS_ERR_PARAMETER;
    }

    *link = fl_sw_info.link[port];

    return SYS_OK;
}

sys_error_t
bcm5607x_loopback_enable(uint8 unit, uint8 port, int loopback_mode)
{
    int link;
    sys_error_t r = SYS_OK;

    if (loopback_mode == PORT_LOOPBACK_MAC) {
        fl_sw_info.loopback[port] = PORT_LOOPBACK_MAC;
        pcm_port_autoneg_set(unit, port, 0);
        pcm_port_speed_set(unit, port, SOC_PORT_SPEED_INIT(port));
        pcm_port_duplex_set(unit, port, 1);
        pcm_port_pause_set(unit, port, 0, 0);
        pcm_phyctrl_port_loopback_set(unit, port, fl_sw_info.loopback[port]);
        return r;
    } else if (loopback_mode == PORT_LOOPBACK_NONE) {
        if (fl_sw_info.loopback[port] != PORT_LOOPBACK_NONE) {
            pcm_phyctrl_port_loopback_set(unit,port, PORT_LOOPBACK_NONE);
            r = pcm_phyctrl_port_autoneg_set(unit, port, 1);
            bcm5607x_handle_link_down(unit, port, TRUE);
            fl_sw_info.loopback[port] = PORT_LOOPBACK_NONE;
        }
        return r;
    }

    fl_sw_info.loopback[port] = loopback_mode;

    r = pcm_phyctrl_port_link_status_get(unit, port, &link);
    if (r < 0) {
#if CFG_CONSOLE_ENABLED
        sal_printf("Failed to get link of port %d\n", (int)port);
#endif /* CFG_CONSOLE_ENABLED */
        return r;
    }

    if (link) {
        /* Force link change */
        sal_printf("force port %d link change\n", port);
        fl_sw_info.link[port] = PORT_LINK_DOWN;
    }

    if (loopback_mode == PORT_LOOPBACK_PHY) {
        /*
        *   pcm_port_autoneg_set is a must.
        *   Since the external PHY may work in 1G speed, no need to set the pcm_port_speed_set(unit, port, SOC_PORT_SPEED_MAX(port)).
        *   SOC_PORT_SPEED_MAX(port) may be, for example 2.5G
        */
        r = pcm_phyctrl_port_autoneg_set(unit, port, 0);
        r |= pcm_phyctrl_port_loopback_set(unit, port, PORT_LOOPBACK_PHY);
        if (r < 0) {
#if CFG_CONSOLE_ENABLED
            sal_printf("Failed to set phy loopback of port %d\n", (int)port);
#endif /* CFG_CONSOLE_ENABLED */
        }
    }
    return r;
}


sys_error_t
bcm5607x_linkscan_init(int timer_period) {


    int lport;

    if (timer_period < 0) {
    timer_period = LINKSCAN_INTERVAL;
    }

    SOC_LPORT_ITER(lport) {
      fl_sw_info.link[lport] = PORT_LINK_DOWN;
    }
    /* Register background process for handling link status */
    timer_add(bcm5607x_linkscan_task, NULL, timer_period);

    return SYS_OK;
}

