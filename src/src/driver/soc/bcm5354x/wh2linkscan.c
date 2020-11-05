/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include <system.h>


#define PORT_LINK_UP                    (1)
#define PORT_LINK_DOWN                  (0)

#if CONFIG_EMULATION
int link_qt[BCM5354X_LPORT_MAX+1];

void
bcm5354x_link_change(uint8 port)
{
    link_qt[port] ^= 1;
}
#endif
void
bcm5354x_handle_link_up(uint8 unit, uint8 lport, int changed, uint32 *flags)
{
    int tx_pause, rx_pause;
    int duplex;
    int speed;
    int an = 0;
#ifdef CFG_LED_MICROCODE_INCLUDED
    uint32 val;
#endif /* CFG_LED_MICROCODE_INCLUDED */
    
    if (1 == changed) {
        /* Port changes to link up from link down */
        if (wh2_sw_info.loopback[lport] == PORT_LOOPBACK_MAC) {
            /* Force link up in MAC loopback mode */
            speed = SOC_PORT_SPEED_MAX(lport);
            duplex = TRUE;
            an = tx_pause = rx_pause = FALSE;
        } else {
            pcm_port_update(0, lport, 1);
			pcm_port_autoneg_get(unit,lport, &an);
			pcm_port_speed_get(unit, lport, &speed);			
			pcm_port_duplex_get(unit, lport, &duplex);
			
			pcm_port_pause_get(unit, lport, &tx_pause, &rx_pause);
#if defined(CFG_SWITCH_EEE_INCLUDED)
            {
                pcm_port_ability_t remote_ability;
                
                /* In the case of Native EEE mode, neet to enable MAC EEE after link + 1 sec	*/
                /* check if the port is auto-neg, need to use remote eee */
                /* use bit 12 to show the remote eee is enable(1) or not(0) */
                pcm_port_ability_remote_get(unit, lport, &remote_ability);
                if ((an) && (remote_ability.eee)) {
                /* Enable EEE in UMAC_EEE_CTRL register after one second
                * if EEE is enabled in S/W database
                */
                  wh2_sw_info.link_up_time[lport] = sal_get_ticks();
                  wh2_sw_info.need_process_for_eee_1s[lport] = TRUE;
                } 
			}
#endif /* CFG_SWITCH_EEE_INCLUDED */

        }


        SOC_PORT_LINK_STATUS(lport) = TRUE;
        SOC_PORT_SPEED_STATUS(lport) = speed;
        SOC_PORT_DUPLEX_STATUS(lport) = duplex ? TRUE : FALSE;
        SOC_PORT_AN_STATUS(lport) = an ? TRUE : FALSE;
        SOC_PORT_TX_PAUSE_STATUS(lport) = tx_pause;
        SOC_PORT_RX_PAUSE_STATUS(lport) = rx_pause;
        
        
#if CFG_CONSOLE_ENABLED
        sal_printf("\nuport %d lport %d (P:%d), speed = %d, duplex = %d, tx_pause = %d, rx_pause = %d, an = %d\n",
                   SOC_PORT_L2U_MAPPING(lport), lport, SOC_PORT_L2P_MAPPING(lport), speed, duplex, tx_pause, rx_pause, an);
#endif /* CFG_CONSOLE_ENABLED */

        if (wh2_sw_info.loopback[lport] != PORT_LOOPBACK_NONE) {
            pcm_port_speed_set(unit, lport, speed);
            pcm_port_duplex_set(unit, lport, duplex);
            pcm_port_pause_set(unit, lport, tx_pause, rx_pause);
            pcm_port_loopback_set(unit, lport, wh2_sw_info.loopback[lport]);
        }                  
        wh2_sw_info.link[lport] = PORT_LINK_UP;

    } else {
        /* Port stays in link up state */
#if defined(CFG_SWITCH_EEE_INCLUDED)
        /* EEE one second delay for link up timer check */
        if ((wh2_sw_info.need_process_for_eee_1s[lport]) &&
            (SAL_TIME_EXPIRED(wh2_sw_info.link_up_time[lport], 1000))) {
#if CFG_CONSOLE_ENABLED
            sal_printf("EEE : enable eee for port %d\n", lport);
#endif /* CFG_CONSOLE_ENABLED */
            bcm5354x_port_eee_enable_set(unit, lport, TRUE, TRUE);
            wh2_sw_info.need_process_for_eee_1s[lport] = FALSE;
        }
#endif /* CFG_SWITCH_EEE_INCLUDED */

    }

#ifdef CFG_LED_MICROCODE_INCLUDED
        if ((changed == 1) && (wh2_sw_info.link[lport] == PORT_LINK_UP)) {
            /* Update LED status */
            val = READCSR(LED_PORT_STATUS_OFFSET(SOC_PORT_L2P_MAPPING(lport)));
            val |= 0x01;
            WRITECSR(LED_PORT_STATUS_OFFSET(SOC_PORT_L2P_MAPPING(lport)), val);
        }
#endif /* CFG_LED_MICROCODE_INCLUDED */

}

void
bcm5354x_handle_link_down(uint8 unit, uint8 lport, int changed)
{
#ifdef CFG_LED_MICROCODE_INCLUDED
    uint32 val;
#endif /* CFG_LED_MICROCODE_INCLUDED */

#ifdef CFG_LOOPBACK_TEST_ENABLED
    if (1 == changed) {
#if CFG_CONSOLE_ENABLED
        if (board_linkdown_message) {
            sal_printf("port %d goes down!\n", lport);
        }
#endif /* CFG_CONSOLE_ENABLED */
        wh2_sw_info.link[lport] = PORT_LINK_DOWN;
    }
#else

    if (1 == changed) {
        SOC_PORT_LINK_STATUS(lport) = FALSE;
#if CFG_CONSOLE_ENABLED
        if (board_linkdown_message) {
            sal_printf("port %d goes down!\n", lport);
        }
#endif /* CFG_CONSOLE_ENABLED */

        /* Port changes to link down from link up */
        pcm_port_update(unit, lport, 0);

#if defined(CFG_SWITCH_EEE_INCLUDED)
    {
        int an = 0 ;
        /* In the case of Native EEE mode, neet to enable MAC EEE after link + 1 sec	*/
        /* check if the port is auto-neg, need to use remote eee */
        /* use bit 12 to show the remote eee is enable(1) or not(0) */
        pcm_port_autoneg_get(unit,lport, &an);
        if (an) {      
            /* Disable EEE in UMAC_EEE_CTRL register if EEE is enabled in S/W database */
#if CFG_CONSOLE_ENABLED
             //sal_printf("EEE : disable eee for lport %d\n", lport);
#endif /* CFG_CONSOLE_ENABLED */
             bcm5354x_port_eee_enable_set(unit, lport, FALSE, TRUE);
             wh2_sw_info.need_process_for_eee_1s[lport] = FALSE;
        }
    }
#endif /* CFG_SWITCH_EEE_INCLUDED */
        wh2_sw_info.link[lport] = PORT_LINK_DOWN;
    } else {
        /* Port stays in link down state */
    }
#endif /* CFG_LOOPBACK_TEST_ENABLED */

#ifdef CFG_LED_MICROCODE_INCLUDED
        if ((changed == 1) && (wh2_sw_info.link[lport] == PORT_LINK_DOWN)) {
            /* Update LED status */
            val = READCSR(LED_PORT_STATUS_OFFSET(SOC_PORT_L2P_MAPPING(lport)));
            val &= (~0x01);
            WRITECSR(LED_PORT_STATUS_OFFSET(SOC_PORT_L2P_MAPPING(lport)), val);
        }
#endif /* CFG_LED_MICROCODE_INCLUDED */

}

/*
 *  Function : bcm5354x_linkscan_task
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
bcm5354x_linkscan_task(void *param)
{
    uint8 unit = 0, lport;
    uint32 flags;
    int link;

    if (board_linkscan_disable) {
        return;
    }

    SOC_LPORT_ITER(lport) {
#if CONFIG_EMULATION
        link = (int)link_qt[lport];
#else
        if (wh2_sw_info.loopback[lport] == PORT_LOOPBACK_MAC) {
            /* Force link up in mac loopback mode */
            link = PORT_LINK_UP;
        } else {
            int rv = 0;
            rv = pcm_port_link_status_get(unit, lport, &link);
            if (rv < 0) {
                continue;
            }
        }
#endif /* CONFIG_EMULATION */

        if (link == PORT_LINK_UP) {
            /* Link up */
            flags = 0;
            bcm5354x_handle_link_up(unit, lport,
                (wh2_sw_info.link[lport] == PORT_LINK_DOWN) ? TRUE : FALSE, &flags);
        } else {
            bcm5354x_handle_link_down(unit, lport,
                (wh2_sw_info.link[lport] == PORT_LINK_UP) ? TRUE : FALSE);
        }
    }
}

sys_error_t
bcm5354x_link_status(uint8 unit, uint8 lport, BOOL *link)
{
    if (link == NULL || unit > 0 || lport > BCM5354X_LPORT_MAX) {
        return SYS_ERR_PARAMETER;
    }

    *link = wh2_sw_info.link[lport];

    return SYS_OK;
}

void
bcm5354x_loopback_enable(uint8 unit, uint8 port, int loopback_mode)
{
    int rv = 0;
    int link;
	uint32 flag;

    if (loopback_mode == PORT_LOOPBACK_MAC) {
      bcm5354x_handle_link_down(unit, port, TRUE);
      wh2_sw_info.loopback[port] = PORT_LOOPBACK_MAC;
      bcm5354x_handle_link_up(unit, port, TRUE, &flag);
      return;
    } else if (loopback_mode == PORT_LOOPBACK_NONE) {
        if (wh2_sw_info.loopback[port] != PORT_LOOPBACK_NONE) {
            pcm_port_loopback_set(unit,port, PORT_LOOPBACK_NONE);
			bcm5354x_handle_link_down(unit, port, TRUE);
            wh2_sw_info.loopback[port] = PORT_LOOPBACK_NONE;
        }
        return;
    }

    wh2_sw_info.loopback[port] = loopback_mode;

    rv = pcm_port_link_status_get(unit, port, &link);
    if (rv < 0) {
#if CFG_CONSOLE_ENABLED
        sal_printf("Failed to get link of port %d\n", (int)port);
#endif /* CFG_CONSOLE_ENABLED */
        return;
    }

    if (link) {
        /* Force link change */
        sal_printf("force port %d link change\n", port);
        wh2_sw_info.link[port] = PORT_LINK_DOWN;
    }

    if (loopback_mode == PORT_LOOPBACK_PHY) {
        rv = pcm_port_autoneg_set(unit, port, 0);
        rv |= pcm_port_speed_set(unit, port, SOC_PORT_SPEED_MAX(port));
        rv |= pcm_port_loopback_set(unit, port, PORT_LOOPBACK_PHY);
        if (rv < 0) {
#if CFG_CONSOLE_ENABLED
            sal_printf("Failed to set phy loopback of port %d\n", (int)port);
#endif /* CFG_CONSOLE_ENABLED */
        }
    }

}


sys_error_t 
bcm5354x_linkscan_init(int timer_period) {


    int lport;

    if (timer_period < 0) { 
        timer_period = LINKSCAN_INTERVAL;
    }

#if CONFIG_EMULATION
    SOC_LPORT_ITER(lport) {
        link_qt[lport] = PORT_LINK_UP;
    }
    bcm5354x_linkscan_task(NULL);
    sal_printf("all ports up!\n");
#else
    SOC_LPORT_ITER(lport) {
        wh2_sw_info.link[lport] = PORT_LINK_DOWN;
    }
#endif /* CONFIG_EMULATION */
    
    /* Register background process for handling link status */
    timer_add(bcm5354x_linkscan_task, NULL, timer_period);

    return SYS_OK;
}