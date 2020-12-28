/*! \file pcm_phyctrl_hounds.c
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#define __int8_t_defined
#define __uint32_t_defined
#include <system.h>
#undef _SOC_PHYCTRL_H_
#include <soc/phyctrl.h>
#undef SOC_IF_ERROR_RETURN
#include <soc/error.h>
#include <pcm/pcm_int.h>

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>
#include <phymod/phymod_acc.h>
#include <soc/property.h>


extern uint32
soc_property_port_get(int unit, uint8 port,
                      const char *name, uint32 defl);


#define SOC_PORT_NAME(unit, port) "??"
#define bcm_errmsg(x)             _SHR_ERRMSG(x)
#define SOC_IS_SABER2(unit)       0


extern int
soc_link_mask2_get(int unit, pbmp_t *mask);
extern int
soc_link_mask2_set(int unit, pbmp_t mask);
extern int
_bcm_esw_link_force(int unit, uint32 flags, int port, int force, int link);

/*
 * Function:
 *      pcm_phyctrl_port_ability_local_get
 * Purpose:
 *      Retrieve the local port abilities.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      ability_mask - (OUT) Mask of _SHR_PORT_ABIL_ values indicating the
 *              ability of the MAC/PHY.
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 */
int
pcm_phyctrl_port_ability_local_get(int unit, int lport,
                                   soc_port_ability_t *ability_mask)
{
    int rv = _SHR_E_NONE;

    soc_port_ability_t mac_ability, phy_ability;
    soc_pa_encap_t encap_ability;

    sal_memset(ability_mask, 0, sizeof(soc_port_ability_t));
    sal_memset(&phy_ability, 0, sizeof(soc_port_ability_t));
    sal_memset(&mac_ability, 0, sizeof(soc_port_ability_t));
    sal_memset(&encap_ability, 0, sizeof(soc_pa_encap_t));

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_ability_local_get(unit, lport, &phy_ability));

    SOC_IF_ERROR_RETURN
        (MAC_ABILITY_LOCAL_GET(PORT(unit, lport).p_mac, unit, lport,
                               &mac_ability));

    /* Combine MAC and PHY abilities */
    ability_mask->speed_half_duplex = mac_ability.speed_half_duplex &
                                      phy_ability.speed_half_duplex;
    ability_mask->speed_full_duplex = mac_ability.speed_full_duplex &
                                      phy_ability.speed_full_duplex;
    ability_mask->pause = mac_ability.pause & phy_ability.pause;

    if (phy_ability.interface == 0) {
        ability_mask->interface = mac_ability.interface;
    } else {
        ability_mask->interface = phy_ability.interface;
    }

    ability_mask->medium = phy_ability.medium;

    /* mac_ability.eee without phy_ability.eee makes no sense */
    ability_mask->eee = phy_ability.eee;
    ability_mask->loopback = mac_ability.loopback |
                             phy_ability.loopback |
                             _SHR_PA_LB_NONE;
    ability_mask->flags = mac_ability.flags | phy_ability.flags;

    /* Get port encapsulation ability */
    encap_ability = mac_ability.encap;

    ability_mask->encap = encap_ability;

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_ability_local_get: u=%d p=%d rv=%d\n"),
              unit, lport, rv));
    LOG_VERBOSE(BSL_LS_BCM_PORT,
                (BSL_META_U(unit,
                            "Speed(HD=0x%08x, FD=0x%08x) Pause=0x%08x\n"
                            "Interface=0x%08x Medium=0x%08x EEE=0x%08x "
                            "Loopback=0x%08x Flags=0x%08x\n"),
                 ability_mask->speed_half_duplex,
                 ability_mask->speed_full_duplex,
                 ability_mask->pause, ability_mask->interface,
                 ability_mask->medium, ability_mask->eee,
                 ability_mask->loopback, ability_mask->flags));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_ability_remote_get
 * Purpose:
 *      Retrieve the local port advertisement for autonegotiation.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      ability_mask - (OUT) Remote advertisement.
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 */
int
pcm_phyctrl_port_ability_remote_get(int unit, int lport,
                                    soc_port_ability_t *ability_mask)
{
    int rv = _SHR_E_NONE;
    int an, an_done;

    sal_memset(ability_mask, 0, sizeof(soc_port_ability_t));

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_auto_negotiate_get(unit,  lport, &an, &an_done));

    if (an && an_done) {
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_ability_remote_get(unit, lport, ability_mask));
    }


    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_ability_remote_get: u=%d p=%d rv=%d\n"),
              unit, lport, rv));
    LOG_VERBOSE(BSL_LS_BCM_PORT,
                (BSL_META_U(unit,
                            "Speed(HD=0x%08x, FD=0x%08x) Pause=0x%08x\n"
                            "Interface=0x%08x Medium=0x%08x Loopback=0x%08x "
                            "Flags=0x%08x\n"),
                 ability_mask->speed_half_duplex,
                 ability_mask->speed_full_duplex,
                 ability_mask->pause, ability_mask->interface,
                 ability_mask->medium, ability_mask->loopback,
                 ability_mask->flags));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_ability_advert_set
 * Purpose:
 *      Set the local port advertisement for autonegotiation.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      ability_mask - Local advertisement.
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 * Notes:
 *      This call MAY NOT restart autonegotiation (depending on the phy).
 *      To do that, follow this call with bcm_port_autoneg_set(TRUE).
 */
int
pcm_phyctrl_port_ability_advert_set(int unit, int lport,
                                    soc_port_ability_t *ability_mask)
{
    int rv;
    soc_port_ability_t port_ability, temp_ability;

    SOC_IF_ERROR_RETURN
        (pcm_phyctrl_port_ability_local_get(unit, lport, &port_ability));

    /* temporary store speed_half_duplex ability */
    temp_ability.speed_half_duplex = ability_mask->speed_half_duplex;
    /* Make sure to advertise only abilities supported by the port */
    soc_port_ability_mask(&port_ability, ability_mask);
    /* restore speed_half_duplex ability even some mac don't support it */
    port_ability.speed_half_duplex = temp_ability.speed_half_duplex;

    rv = soc_phyctrl_ability_advert_set(unit, lport, &port_ability);

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_ability_advert_set: u=%d p=%d rv=%d\n"),
              unit, lport, rv));
    LOG_VERBOSE(BSL_LS_BCM_PORT,
                (BSL_META_U(unit,
                            "Speed(HD=0x%08x, FD=0x%08x) Pause=0x%08x\n"
                            "Interface=0x%08x Medium=0x%08x Loopback=0x%08x "
                            "Flags=0x%08x\n"),
                 port_ability.speed_half_duplex,
                 port_ability.speed_full_duplex,
                 port_ability.pause, port_ability.interface,
                 port_ability.medium, port_ability.loopback,
                 port_ability.flags));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_link_status_get
 * Purpose:
 *      Retrieve current Link up/down status from the PHY.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      up - (OUT) Boolean value, FALSE for link down and TRUE for link up
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 */
int
pcm_phyctrl_port_link_status_get(int unit, int lport, int *link)
{
    int rv;

    rv = soc_phyctrl_link_get(unit, lport, link);

    if (SOC_SUCCESS(rv)) {
        if (PHY_FLAGS_TST(unit, lport, PHY_FLAGS_MEDIUM_CHANGE)) {
            soc_port_medium_t  medium;
            soc_phyctrl_medium_get(unit, lport, &medium);
            soc_phy_medium_status_notify(unit, lport, medium);
        }
    }

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_link_status_get: u=%d p=%d up=%d rv=%d\n"),
              unit, lport, *link, rv));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_speed_get
 * Purpose:
 *      Getting the speed of the port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      speed - (OUT) Value in megabits/sec (10, 100, etc)
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 * Notes:
 *      If port is in MAC loopback, the speed of the loopback is returned.
 */
int
pcm_phyctrl_port_speed_get(int unit, int lport, int *speed)
{
    int rv = _SHR_E_NONE;
    int mac_lb;

    rv = MAC_LOOPBACK_GET(PORT(unit, lport).p_mac, unit, lport, &mac_lb);

    if (SOC_SUCCESS(rv)) {
        if (mac_lb) {
            rv = MAC_SPEED_GET(PORT(unit, lport).p_mac, unit, lport, speed);
        } else {
            rv = soc_phyctrl_speed_get(unit, lport, speed);
            if (_SHR_E_UNAVAIL == rv) {
                /* PHY driver doesn't support speed_get. Get the speed from
                              * MAC.
                              */
                rv = MAC_SPEED_GET(PORT(unit, lport).p_mac, unit, lport, speed);
            }
        }
    }

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_speed_get: u=%d p=%d speed=%d rv=%d\n"),
              unit, lport, SOC_SUCCESS(rv) ? *speed : 0, rv));

    return rv;
}

/*
 * Function:
 *      bcm_port_speed_max
 * Purpose:
 *      Getting the maximum speed of the port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      speed - (OUT) Value in megabits/sec (10, 100, etc)
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 */
int
pcm_phyctrl_port_speed_max(int unit, int lport, int *speed)
{
    int rv;
    soc_port_ability_t ability;

    if (NULL == speed) {
        return (SOC_E_PARAM);
    }

    rv = pcm_phyctrl_port_ability_local_get(unit, lport, &ability);

    if (SOC_SUCCESS(rv)) {
        *speed = _SHR_PA_SPEED_MAX(ability.speed_full_duplex |
                                   ability.speed_half_duplex);
    } else {
        *speed = 0;
    }

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_speed_max: u=%d p=%d speed=%d rv=%d\n"),
              unit, lport, *speed, rv));

    return rv;
}


/*
 * Function:
 *      _bcm_port_speed_set
 * Purpose:
 *      Main part of bcm_port_speed_set.
 */

static int
_pcm_phyctrl_port_speed_set(int unit, int lport, int speed)
{
    int rv;
    int mac_lb;
    soc_port_ability_t mac_ability, phy_ability, requested_ability;

    sal_memset(&mac_ability, 0, sizeof(soc_port_ability_t));
    sal_memset(&phy_ability, 0, sizeof(soc_port_ability_t));
    sal_memset(&requested_ability, 0, sizeof(soc_port_ability_t));

    /*
     * If port is in MAC loopback mode, do not try setting the PHY
     * speed.  This allows MAC loopback at 10/100 even if the PHY is
     * 1000 only.  Loopback diagnostic tests should enable loopback
     * before setting the speed, and vice versa when cleaning up.
     */

    SOC_IF_ERROR_RETURN
        (MAC_LOOPBACK_GET(PORT(unit, lport).p_mac,unit, lport, &mac_lb));

    if (speed == 0) {
        /* if speed is 0, set the port speed to max */
        SOC_IF_ERROR_RETURN(pcm_phyctrl_port_speed_max(unit, lport, &speed));
    }

    /* Make sure MAC can handle the requested speed. */
    SOC_IF_ERROR_RETURN
         (MAC_ABILITY_LOCAL_GET(PORT(unit, lport).p_mac, unit, lport,
                                &mac_ability));

    requested_ability.speed_full_duplex = SOC_PA_SPEED(speed);
    requested_ability.speed_half_duplex = SOC_PA_SPEED(speed);

    LOG_INFO(BSL_LS_BCM_PHY,
             (BSL_META_U(unit,
                         "_bcm_port_speed_set: u=%u p=%d "
                         "MAC FD speed %08X MAC HD speed %08X "
                         "Requested FD Speed %08X Requested HD Speed %08X\n"),
              unit,
              lport,
              mac_ability.speed_full_duplex,
              mac_ability.speed_half_duplex,
              requested_ability.speed_full_duplex,
              requested_ability.speed_half_duplex));

    if (((mac_ability.speed_full_duplex &
          requested_ability.speed_full_duplex) == 0) &&
        ((mac_ability.speed_half_duplex &
          requested_ability.speed_half_duplex) == 0)) {
        LOG_VERBOSE(BSL_LS_BCM_PORT,
                    (BSL_META_U(unit,
                                "u=%d p=%d MAC doesn't support %d Mbps"
                                "speed.\n"),
                     unit, lport, speed));
        return SOC_E_CONFIG;
    }

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_ability_local_get(unit, lport, &phy_ability));

    if (((phy_ability.speed_full_duplex &
          requested_ability.speed_full_duplex) == 0) &&
        ((phy_ability.speed_half_duplex &
          requested_ability.speed_half_duplex) == 0)) {
          LOG_VERBOSE(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "u=%d p=%d PHY doesn't support %d Mbps.\n"),
                      unit, lport, speed));
          return SOC_E_CONFIG;
    } else {
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_auto_negotiate_set(unit, lport, FALSE));
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_speed_set(unit, lport, speed));

    }

    rv = MAC_SPEED_SET(PORT(unit, lport).p_mac, unit, lport, speed);
    if (SOC_FAILURE(rv)) {
        LOG_VERBOSE(BSL_LS_BCM_PORT,
                    (BSL_META_U(unit,
                                "MAC_SPEED_SET failed: %s\n"),
                     _SHR_ERRMSG(rv)));
    }

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_speed_set
 * Purpose:
 *      Setting the speed for a given port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      speed - Value in megabits/sec (10, 100, etc)
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 * Notes:
 *      Turns off autonegotiation.  Caller must make sure other forced
 *      parameters (such as duplex) are set.
 */
int
pcm_phyctrl_port_speed_set(int unit, int lport, int speed)
{
    int rv;

    
    #if 0
    if (soc_property_get(unit, spn_SAME_SPEED_INTF_DO_NOT_OVERWRITE,0)) {
        BCM_IF_ERROR_RETURN(bcm_esw_port_speed_get(unit, port, &curr_speed));
        BCM_IF_ERROR_RETURN(bcm_esw_port_interface_get(unit, port, &curr_intf));
        if((speed == curr_speed) && (PORT(unit, port).intf == curr_intf)) {
            return BCM_E_NONE;
        }
    }
    #endif

    if (IS_XL_PORT(lport) && (speed == 12700)) {
        speed = 13000;
    }

    rv = _pcm_phyctrl_port_speed_set(unit, lport, speed);

    
    #if 0
    if (SOC_SUCCESS(rv) && !SAL_BOOT_SIMULATION) {
        SOC_PBMP_CLEAR(pbm);
        SOC_PBMP_PORT_ADD(pbm, port);
        (void)bcm_esw_link_change(unit, pbm);
    }
    #endif

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_speed_set: u=%d p=%d speed=%d rv=%d\n"),
              unit, lport, speed, rv));

   return rv;
}


/*
 * Function:
 *      pcm_phyctrl_port_interface_get
 * Purpose:
 *      Getting the interface type of a port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      intf - (OUT) BCM_PORT_IF_*
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 * Notes:
 */
int
pcm_phyctrl_port_interface_get(int unit, int port, soc_port_if_t *intf)
{
    int rv;

    rv = soc_phyctrl_interface_get(unit, port, intf);

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_common_port_interface_set
 * Purpose:
 *      Setting the interface type for a given port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      if - BCM_PORT_IF_*
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 * Notes:
 */
int
pcm_phyctrl_port_interface_set(int unit, int lport, soc_port_if_t intf)
{
    int rv;

    rv = soc_phyctrl_interface_set(unit, lport, intf);

    if (SOC_FAILURE(rv)) {
       LOG_VERBOSE(BSL_LS_BCM_PORT,
                    (BSL_META_U(unit,
                                "PHY_INTERFACE_SETfailed:%s\n"),
                     _SHR_ERRMSG(rv)));
    }

    
    #if 0
    if (BCM_SUCCESS(rv)) {
        SOC_PBMP_CLEAR(pbm);
        SOC_PBMP_PORT_ADD(pbm, port);
        (void)bcm_esw_link_change(unit, pbm);
        PORT(unit, port).intf = intf;
    }
    #endif
    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_autoneg_get
 * Purpose:
 *      Get the autonegotiation state of the port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      autoneg - (OUT) Boolean value
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 */
int
pcm_phyctrl_port_autoneg_get(int unit, int lport, int *autoneg)
{
    int done, rv;

    rv = soc_phyctrl_auto_negotiate_get(unit, lport, autoneg, &done);

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_autoneg_get: u=%d p=%d an=%d done=%d "
                         "rv=%d\n"),
              unit, lport, *autoneg, done, rv));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_autoneg_set
 * Purpose:
 *      Set the autonegotiation state for a given port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      autoneg - Boolean value
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 */
#define SOC_PA_ALL_BELOW_SPEED(s) ((SOC_PA_SPEED(s)-1) | SOC_PA_SPEED(s))
int
pcm_phyctrl_port_autoneg_set(int unit, int lport, int autoneg)
{
    int rv;
    soc_port_ability_t request_ability, ability_mask;

    if (autoneg) {
        sal_memset(&request_ability, 0, sizeof(request_ability));
        pcm_phyctrl_port_ability_local_get(unit, lport, &ability_mask);
        request_ability.speed_full_duplex = ability_mask.speed_full_duplex & SOC_PA_ALL_BELOW_SPEED(SOC_PORT_SPEED_INIT(lport));
        request_ability.pause = SOC_PA_PAUSE;
        pcm_phyctrl_port_ability_advert_set(unit, lport, &request_ability);
        pcm_phyctrl_port_ability_local_get(unit, lport, &ability_mask);
        rv = soc_phyctrl_auto_negotiate_set(unit, lport, autoneg);
    } else {
        rv = soc_phyctrl_auto_negotiate_set(unit, lport, autoneg);
    }
    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_autoneg_set: u=%d p=%d an=%d rv=%d\n"),
              unit, lport, autoneg, rv));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_pause_set
 * Purpose:
 *      Set the pause state for a given port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      pause_tx - Boolean value, or -1 (don't change)
 *      pause_rx - Boolean value, or -1 (don't change)
 * Returns:
 *      _SHR_E_NONE
 *      _SHR_E_XXX
 * Notes:
 *      Symmetric pause requires the two "pause" values to be the same.
 */
int
pcm_phyctrl_port_pause_set(int unit, int lport, int pause_tx, int pause_rx)
{
    return MAC_PAUSE_SET(PORT(unit, lport).p_mac, unit, lport,
                         pause_tx, pause_rx);
}

/*
 * Function:
 *      pcm_phyctrl_port_pause_get
 * Purpose:
 *      Get the source address for transmitted PAUSE frames.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      pause_tx - (OUT) Boolean value
 *      pause_rx - (OUT) Boolean value
 * Returns:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 */

int
pcm_phyctrl_port_pause_get(int unit, int lport, int *pause_tx, int *pause_rx)
{
    return MAC_PAUSE_GET(PORT(unit, lport).p_mac, unit, lport,
                         pause_tx, pause_rx);
}

/*
 * Function:
 *      pcm_phyctrl_port_duplex_get
 * Purpose:
 *      Get the port duplex settings
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      duplex - (OUT) Duplex setting, one of SOC_PORT_DUPLEX_xxx
 * Returns:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 */
int
pcm_phyctrl_port_duplex_get(int unit, int lport, int *duplex)
{
    int rv;
    int phy_duplex;

    rv = soc_phyctrl_duplex_get(unit, lport, &phy_duplex);

    if (SOC_SUCCESS(rv)) {
        *duplex = phy_duplex ? SOC_PORT_DUPLEX_FULL : SOC_PORT_DUPLEX_HALF;
    } else {
        *duplex = SOC_PORT_DUPLEX_FULL;
    }

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_duplex_get: u=%d p=%d dup=%d rv=%d\n"),
              unit, lport, *duplex, rv));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_duplex_set
 * Purpose:
 *      Set the port duplex settings.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      duplex - Duplex setting, one of SOC_PORT_DUPLEX_xxx
 * Returns:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 * Notes:
 *      Turns off autonegotiation.  Caller must make sure other forced
 *      parameters (such as speed) are set.
 */
int
pcm_phyctrl_port_duplex_set(int unit, int lport, int duplex)
{
    int rv;
    soc_port_ability_t ability_mask;
    int speed = 0, full_duplex_speed, half_duplex_speed;
    uint32 pa_speed = 0;
    int an = 0 ;
    int an_done=0;

    sal_memset(&ability_mask, 0, sizeof(soc_port_ability_t));


    rv = pcm_phyctrl_port_ability_local_get(unit, lport, &ability_mask);
    if (rv < 0) {
        LOG_VERBOSE(BSL_LS_BCM_PORT,
                    (BSL_META("Error: Could not get port %d ability: %s\n"),
                     lport, _SHR_ERRMSG(rv)));
        return rv;
    } else {
        rv = pcm_phyctrl_port_speed_get(unit, lport, &speed);
        if (rv < 0) {
            LOG_VERBOSE(BSL_LS_BCM_PORT,
                        (BSL_META("Error: Could not get port %d speed: %s\n"),
                         lport, _SHR_ERRMSG(rv)));
            return rv;
        } else {
            pa_speed = SOC_PA_SPEED(speed);
            full_duplex_speed = ability_mask.speed_full_duplex & pa_speed;
            half_duplex_speed = ability_mask.speed_half_duplex & pa_speed;
            if (speed != 0) {
                if (duplex) {
                    if (!full_duplex_speed) {
                        LOG_ERROR(BSL_LS_BCM_PORT,
                                  (BSL_META("Error: port %d does not support %d mbps full duplex\n"),
                                   lport, speed));
                        return SOC_E_PARAM;
                    }
                } else {
                    if (!half_duplex_speed) {
                        LOG_ERROR(BSL_LS_BCM_PORT,
                                  (BSL_META("Error: port %d does not support %d mbps half duplex\n"),
                                   lport, speed));
                        return SOC_E_PARAM;
                    }
                }
            }
        }
    }

    rv = soc_phyctrl_auto_negotiate_get(unit, lport, &an, &an_done);
    if (SOC_FAILURE(rv)) {
        LOG_VERBOSE(BSL_LS_BCM_PORT,
                    (BSL_META_U(unit,
                                "PHY_AUTONEG_GET failed:%s\n"),
                     _SHR_ERRMSG(rv)));

    }
    if(an != FALSE)
    {
        rv = soc_phyctrl_auto_negotiate_set(unit, lport, FALSE);
        if (SOC_FAILURE(rv)) {
            LOG_VERBOSE(BSL_LS_BCM_PORT,
                        (BSL_META_U(unit,
                                    "PHY_AUTONEG_SET failed:%s\n"),
                         _SHR_ERRMSG(rv)));
        }
    }

    if (SOC_SUCCESS(rv)) {
        rv = soc_phyctrl_duplex_set(unit, lport, duplex);
        if (SOC_FAILURE(rv)) {
            LOG_VERBOSE(BSL_LS_BCM_PORT,
                        (BSL_META_U(unit,
                                    "PHY_DUPLEX_SET failed:%s\n"),
                         _SHR_ERRMSG(rv)));
        }
    }

    if (SOC_SUCCESS(rv)) {
        rv = MAC_DUPLEX_SET(PORT(unit, lport).p_mac, unit, lport, duplex);
        if (SOC_FAILURE(rv)) {
            LOG_VERBOSE(BSL_LS_BCM_PORT,
                        (BSL_META_U(unit,
                                    "MAC_DUPLEX_SET failed:%s\n"),
                         _SHR_ERRMSG(rv)));
        }
    }

    
    #if 0
    if (BCM_SUCCESS(rv) && !SAL_BOOT_SIMULATION) {
        SOC_PBMP_CLEAR(pbm);
        SOC_PBMP_PORT_ADD(pbm, port);
        (void)bcm_esw_link_change(unit, pbm);

    }
    #endif
    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_duplex_set: u=%d p=%d dup=%d rv=%d\n"),
              unit, lport, duplex, rv));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_enable_set
 * Purpose:
 *      Physically enable/disable the MAC/PHY on this port.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      enable - TRUE, port is enabled, FALSE port is disabled.
 * Returns:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 * Notes:
 *      If linkscan is running, it also controls the MAC enable state.
 */
int
pcm_phyctrl_port_enable_set(int unit, int lport, int enable)
{
    int rv = SOC_E_NONE;
    int loopback = PORT_LOOPBACK_NONE;
    pbmp_t mask;
#if 0
    int link;
    bcm_esw_port_loopback_get(unit, port, &loopback);
#endif

    if (enable) {
        rv = soc_phyctrl_enable_set(unit, lport, TRUE);

#if 0
        if (SOC_SUCCESS(rv)) {
            /* Get link status after PHY state has been set */
            rv = pcm_phyctrl_port_link_status_get(unit, lport, &link);
            if (SOC_FAILURE(rv)) {
                if (rv == SOC_E_INIT) {
                    link = FALSE;
                    rv = SOC_E_NONE;
                } else {
                    return rv;
                }
            }

            if (link || (loopback != PORT_LOOPBACK_NONE)) {
                rv = MAC_ENABLE_SET(PORT(unit, lport).p_mac, unit, lport, TRUE);
                if (loopback == PORT_LOOPBACK_MAC) {
                    /* Add port to EPC_LINK */
                    soc_link_mask2_get(unit, &mask);
                    PBMP_PORT_ADD(mask, lport);
                    rv = soc_link_mask2_set(unit, mask);
                    if (SOC_FAILURE(rv)) {
                        return (rv);
                    }
                }
            }
        }
#endif
    } else {
        /* Remove from EPC_LINK to prevent packet queued to this port from TX */
        soc_link_mask2_get(unit, &mask);
        PBMP_PORT_REMOVE(mask, lport);
        rv = soc_link_mask2_set(unit, mask);
        if (SOC_FAILURE(rv)) {
            return (rv);
        }
        LOG_VERBOSE(BSL_LS_BCM_PORT,
                    (BSL_META_U(unit,
                                "Disable and isolate port %d..\n"),
                     lport));
        /*
         * SDK-71770/TH-4402 WAR for TH. Mismatch in expectation between
         * SERDES and MAC on what should be presented on the data bus when
         * port is administratively disabled. SW WAR is to disable MAC.RX
         * before disabling PHY.
         */

        /* Disable MAC RX to prevent traffic going into this port */
        MAC_CONTROL_SET(PORT(unit, lport).p_mac, unit, lport,
                        SOC_MAC_CONTROL_RX_SET, FALSE);

        sal_usleep(100); /* Wait for 1 jumbo packet transmission time */

        MAC_CONTROL_SET(PORT(unit, lport).p_mac, unit, lport,
                        SOC_MAC_CONTROL_DISABLE_PHY, TRUE);

        rv = soc_phyctrl_enable_set(unit, lport, FALSE);


        MAC_CONTROL_SET(PORT(unit, lport).p_mac, unit, lport,
                        SOC_MAC_CONTROL_DISABLE_PHY, FALSE);

        /*
         *  When the port is configured to MAC loopback,
         *  can't disabling MAC.
         */
        if (loopback != PORT_LOOPBACK_MAC) {
            if (SOC_SUCCESS(rv)) {
                rv = MAC_ENABLE_SET(PORT(unit, lport).p_mac, unit, lport,
                                    FALSE);
            }
        }
    }

    if (loopback != PORT_LOOPBACK_NONE) {
        if (loopback == PORT_LOOPBACK_MAC) {
            rv = _bcm_esw_link_force(unit, 0 /*flags*/, lport, TRUE, TRUE);
        } else {
            rv = _bcm_esw_link_force(unit, 0 /*flags*/, lport, TRUE, enable);
        }
    }
    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_enable_set: u=%d p=%d enable=%d rv=%d\n"),
              unit, lport, enable, rv));

    return rv;
}


/*
 * Function:
 *      pcm_phyctrl_port_enable_get
 * Purpose:
 *      Gets the enable state as defined by bcm_port_enable_set()
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      enable - (OUT) TRUE, port is enabled, FALSE port is disabled.
 * Returns:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 * Notes:
 *      The PHY enable holds the port enable state set by the user.
 *      The MAC enable transitions up and down automatically via linkscan
 *      even if user port enable is always up.
 */
int
pcm_phyctrl_port_enable_get(int unit, int lport, int *enable)
{
    int rv;

    rv = soc_phyctrl_enable_get(unit, lport, enable);

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_enable_get: u=%d p=%d rv=%d enable=%d\n"),
              unit, lport, rv, *enable));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_loopback_set
 * Purpose:
 *      Setting the speed for a given port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      loopback - one of:
 *              PORT_LOOPBACK_NONE
 *              PORT_LOOPBACK_MAC
 *              PORT_LOOPBACK_PHY
 * Returns:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 */
int
pcm_phyctrl_port_loopback_set(int unit, int lport, int loopback)
{
    int rv, link = TRUE;

    soc_phyctrl_enable_get(unit, lport, &link);

#if 0
    if (TRUE == link) {
        if (SOC_PBMP_MEMBER(sop->lc_pbm_override_ports, port)) {
            if(!SOC_PBMP_MEMBER(sop->lc_pbm_override_link, port)) {
                link = FALSE;
            }
        }
    }
#endif
    rv = SOC_E_NONE;
    /*
     * Always force link before changing hardware to avoid
     * race with the linkscan thread.
     * On KT2, the operated port should be forced link down
     * before mode change to prevent traffic being forwarded
     * to the port.
     */
#if 0
    if (!(loopback == BCM_PORT_LOOPBACK_NONE) || SOC_IS_KATANA2(unit)) {
        rv = _bcm_esw_link_force(unit, 0 /*flags*/, port, TRUE, FALSE);

        if(rv == BCM_E_NONE) {
            /* Force the update function to be called so the state is
               updated */
            if (_soc_linkscan_phy_flags_test(unit, port, PHY_FLAGS_EXTERNAL_PHY)) {
                rv = bcm_esw_port_update(unit, port, TRUE);
            }
        }
    }
#endif


    if (SOC_SUCCESS(rv)) {
        rv = MAC_LOOPBACK_SET(PORT(unit, lport).p_mac, unit, lport,
                              (loopback == PORT_LOOPBACK_MAC));
    }
    if (SOC_SUCCESS(rv)) {
        if (loopback == PORT_LOOPBACK_PHY_REMOTE) {
                rv = soc_phyctrl_control_set(unit, lport,
                                             SOC_PHY_CONTROL_LOOPBACK_REMOTE,
                                             1);
        } else {
            if (loopback == PORT_LOOPBACK_NONE) {
                rv = soc_phyctrl_control_set(unit, lport,
                                             SOC_PHY_CONTROL_LOOPBACK_REMOTE,
                                             0);
            }

            if (rv == SOC_E_NONE || rv == SOC_E_UNAVAIL) {
                rv = soc_phyctrl_loopback_set(unit, lport,
                                              (loopback == PORT_LOOPBACK_PHY),
                                              TRUE);
            }
        }
    }

    if ((loopback == PORT_LOOPBACK_NONE) || !SOC_SUCCESS(rv)) {
        _bcm_esw_link_force(unit, 0 /*flags*/, lport, FALSE, 0);
         if ((FALSE == link) && (loopback == PORT_LOOPBACK_NONE)) {
             /* Disable MAC RX to prevent traffic going into this port */
             MAC_CONTROL_SET(PORT(unit, lport).p_mac, unit, lport,
                             SOC_MAC_CONTROL_RX_SET, FALSE);
             sal_usleep(100); /* Wait for 1 jumbo packet transmission time */

             rv = MAC_ENABLE_SET(PORT(unit, lport).p_mac, unit, lport, FALSE);
         }
    } else {
        /*
         * Enable only MAC instead of calling bcm_port_enable_set so
         * that this API doesn't silently enable the port if the
         * port is disabled by application.
         */
        rv = MAC_ENABLE_SET(PORT(unit, lport).p_mac, unit, lport, TRUE);

        if (SOC_SUCCESS(rv)) {
            /*
             * Make sure that the link status is updated only after the
             * MAC is enabled so that link_mask2 is set before the
             * calling thread synchronizes with linkscan thread in
             * _bcm_link_force call.
             * If the link is forced before MAC is enabled, there could
             * be a race condition in _soc_link_update where linkscan
             * may use an old view of link_mask2 and override the
             * EPC_LINK_BMAP after the mac_enable_set updates
             * link_mask2 and EPC_LINK_BMAP.
             */
            if (loopback == PORT_LOOPBACK_MAC) {
                rv = _bcm_esw_link_force(unit, 0 /*flags*/, lport, TRUE, TRUE);
            } else {
                rv = _bcm_esw_link_force(unit, 0 /*flags*/, lport, TRUE, link);
            }
        }
    }

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_loopback_set: u=%d p=%d lb=%d rv=%d\n"),
              unit, lport, loopback, rv));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_loopback_get
 * Purpose:
 *      Recover the current loopback operation for the specified port.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      loopback - (OUT) one of:
 *              PORT_LOOPBACK_NONE
 *              PORT_LOOPBACK_MAC
 *              PORT_LOOPBACK_PHY
 * Returns:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 */
int
pcm_phyctrl_port_loopback_get(int unit, int lport, int *loopback)
{
    int         rv = SOC_E_NONE;
    int         phy_lb = 0;
    uint32      phy_rlb = 0;
    int         mac_lb = 0;

    /*
     * Most ext-phys don't support remote loopback and return SOC_E_UNAVAIL.
     * When the return value is SOC_E_UNAVAIL, continue to get phy or mac
     * loopback mode
     */
    rv = soc_phyctrl_control_get(unit, lport,
                                 SOC_PHY_CONTROL_LOOPBACK_REMOTE,
                                 &phy_rlb);
    if (rv >= 0 || rv == SOC_E_UNAVAIL) {
        rv = soc_phyctrl_loopback_get(unit, lport, &phy_lb);
    }

    if (rv >= 0) {
        rv = MAC_LOOPBACK_GET(PORT(unit, lport).p_mac, unit, lport, &mac_lb);
    }

    if (rv >= 0) {
        if (mac_lb) {
            *loopback = PORT_LOOPBACK_MAC;
        } else if (phy_lb) {
            *loopback = PORT_LOOPBACK_PHY;
        } else if (phy_rlb) {
            *loopback = PORT_LOOPBACK_PHY_REMOTE;
        } else {
            *loopback = PORT_LOOPBACK_NONE;
        }
    } else {
        *loopback = PORT_LOOPBACK_NONE;
    }

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_loopback_get: u=%d p=%d lb=%d rv=%d\n"),
              unit, lport, *loopback, rv));

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_control_get
 * Description:
 *      Get the status of specified port feature.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      type - Enum  value of the feature
 *      value - (OUT) Current value of the port feature
 * Return Value:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 */
int
pcm_phyctrl_port_control_get(int unit, int port, int type, int *value)
{
    int rv = SOC_E_UNAVAIL;
    /* TBI */
    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_control_set
 * Description:
 *      Enable/Disable specified port feature.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      type - Enum value of the feature
 *      value - value to be set
 * Return Value:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 */
int
pcm_phyctrl_port_control_set(int unit, int port, int type, int value)
{
    int rv = SOC_E_UNAVAIL;
    /* TBI */
    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_update
 * Purpose:
 *      Get port characteristics from PHY and program MAC to match.
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      link -  True if link is active, false if link is inactive.
 * Returns:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 */

int
pcm_phyctrl_port_update(int unit, int lport, int link)
{
    int rv;
    int duplex, speed, an, an_done;
    soc_port_if_t pif;
    int cur_mac_speed;
    int enable;

    if (!link) {
        /* PHY is down.  Disable the MAC. */
        rv = MAC_ENABLE_SET(PORT(unit, lport).p_mac, unit, lport, FALSE);
        if (SOC_FAILURE(rv)) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "u=%d p=%d MAC_ENABLE_SET FALSE rv=%d\n"),
                      unit, lport, rv));
            return rv;
        }

        /* PHY link down event */
        rv = soc_phyctrl_linkdn_evt(unit, lport);
        if (SOC_FAILURE(rv) && (SOC_E_UNAVAIL != rv)) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "u=%d p=%d soc_phyctrl_linkdn_evt rv=%d\n"),
                      unit, lport, rv));
            return rv;
        }

        return SOC_E_NONE;
    }

    /*
     * PHY link up event may not be support by all PHY driver.
     * Just ignore it if not supported
     */
    rv = soc_phyctrl_linkup_evt(unit, lport);

    if (SOC_FAILURE(rv) && (SOC_E_UNAVAIL != rv)) {
        LOG_WARN(BSL_LS_BCM_PORT,
                 (BSL_META_U(unit,
                             "u=%d p=%d soc_phyctrl_linkup_evt rv=%d\n"),
                  unit, lport, rv));
        return rv;
    }

    
    #if 0
        int up = 0;

        SOC_IF_ERROR_RETURN
            (soc_phyctrl_link_get(unit, port, &up));

        /* Wait internal PHY link when external PHY is link up */
        if (up) {
            (void)soc_phyctrl_notify(unit, port, phyEventLinkWait, 0);
        }
    #endif

    /*
         * Set MAC speed first, since for GTH ports, this will switch
         * between the 1000Mb/s or 10/100Mb/s MACs.
         */

    if (IS_GX_PORT(lport)) {
        rv = soc_phyctrl_speed_get(unit, lport, &speed);
        if (SOC_FAILURE(rv) && (SOC_E_UNAVAIL != rv)) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "u=%d p=%d phyctrl_speed_get rv=%d\n"),
                      unit, lport, rv));
            return rv;
        }
        LOG_INFO(BSL_LS_BCM_PORT,
                 (BSL_META_U(unit,
                             "u=%d p=%d phyctrl_speed_get speed=%d\n"),
                  unit, lport, speed));

        if (rv == SOC_E_UNAVAIL) {
            /*
             * If PHY driver doesn't support speed_get, don't change
             * MAC speed. E.g, Null PHY driver
             */
            rv = SOC_E_NONE;
        } else {
            rv = MAC_SPEED_GET(PORT(unit, lport).p_mac, unit, lport,
                               &cur_mac_speed);
            if (SOC_FAILURE(rv)) {
                LOG_WARN(BSL_LS_BCM_PORT,
                         (BSL_META_U(unit,
                                     "u=%d p=%d MAC_SPEED_GET rv=%d\n"),
                          unit, lport, rv));
                return rv;
            }

            rv = MAC_ENABLE_GET(PORT(unit, lport).p_mac, unit, lport, &enable);
            if (SOC_FAILURE(rv)) {
                LOG_WARN(BSL_LS_BCM_PORT,
                            (BSL_META_U(unit,
                                      "u=%d p=%d MAC_ENABLE_GET rv=%d\n"),
                             unit, lport, rv));
                return rv;
            }

            /*
             * If current MAC speed is the same as PHY speed, no need set it
             * again.
             */
            if (cur_mac_speed != speed || enable != TRUE) {
                rv = MAC_SPEED_SET(PORT(unit, lport).p_mac, unit, lport, speed);
            }
        }
        if (SOC_FAILURE(rv)) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "u=%d p=%d MAC_SPEED_SET speed=%d rv=%d\n"),
                      unit, lport, speed, rv));
            return rv;
        }

        rv = soc_phyctrl_duplex_get(unit, lport, &duplex);
        if (SOC_FAILURE(rv)) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "u=%d p=%d phyctrl_duplex_get rv=%d\n"),
                                 unit, lport, rv));
            return rv;
        }

        rv = MAC_DUPLEX_SET(PORT(unit, lport).p_mac, unit, lport, duplex);
        if (SOC_FAILURE(rv)) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "u=%d p=%d MAC_DUPLEX_SET %s sp=%d rv=%d\n"),
                                 unit, lport,
                      duplex ? "FULL" : "HALF", speed, rv));
            return rv;
        }
    } else {
        duplex = 1;
    }

    rv = soc_phyctrl_interface_get(unit, lport, &pif);
    if (SOC_FAILURE(rv)) {
        LOG_WARN(BSL_LS_BCM_PORT,
                 (BSL_META_U(unit,
                             "u=%d p=%d phyctrl_interface_get rv=%d\n"),
                  unit, lport, rv));
        return rv;
    }

    rv = MAC_INTERFACE_SET(PORT(unit, lport).p_mac, unit, lport, pif);
    if (SOC_FAILURE(rv)) {
        LOG_WARN(BSL_LS_BCM_PORT,
                 (BSL_META_U(unit,
                             "u=%d p=%d MAC_INTERFACE_GET rv=%d\n"),
                  unit, lport,rv));
        return rv;
    }

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_auto_negotiate_get(unit, lport, &an, &an_done));

    /*
     * If autonegotiating, check the negotiated PAUSE values, and program
     * MACs accordingly. Link can also be achieved thru parallel detect.
     * In this case, it should be treated as in the forced mode.
     */

    if (an && an_done) {
        soc_port_ability_t remote_advert, local_advert;
        int tx_pause, rx_pause;

        sal_memset(&local_advert,  0, sizeof(soc_port_ability_t));
        sal_memset(&remote_advert, 0, sizeof(soc_port_ability_t));

        rv = soc_phyctrl_ability_advert_get(unit, lport, &local_advert);
        if (SOC_FAILURE(rv)) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "u=%d p=%d soc_phyctrl_adv_local_get "
                                 "rv=%d\n"),
                      unit, lport, rv));
            return rv;
        }
        rv = soc_phyctrl_ability_remote_get(unit, lport, &remote_advert);
        if (SOC_FAILURE(rv)) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                  "u=%d p=%d soc_phyctrl_adv_remote_get "
                                  "rv=%d\n"),
                      unit, lport, rv));
            return rv;
        }

        /*
         * IEEE 802.3 Flow Control Resolution.
         * Please see $SDK/doc/pause-resolution.txt for more information.
         */

        if (duplex) {
            tx_pause = ((remote_advert.pause & SOC_PA_PAUSE_RX) &&
                        (local_advert.pause & SOC_PA_PAUSE_RX)) ||
                       ((remote_advert.pause & SOC_PA_PAUSE_RX) &&
                        !(remote_advert.pause & SOC_PA_PAUSE_TX) &&
                        (local_advert.pause & SOC_PA_PAUSE_TX));

            rx_pause = ((remote_advert.pause & SOC_PA_PAUSE_RX) &&
                        (local_advert.pause & SOC_PA_PAUSE_RX)) ||
                       ((local_advert.pause & SOC_PA_PAUSE_RX) &&
                        (remote_advert.pause & SOC_PA_PAUSE_TX) &&
                        !(local_advert.pause & SOC_PA_PAUSE_TX));
        } else {
            rx_pause = tx_pause = 0;
        }

        rv = MAC_PAUSE_SET(PORT(unit, lport).p_mac,
                           unit, lport, tx_pause, rx_pause);
        if (SOC_FAILURE(rv)) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "u=%d p=%d MAC_PAUSE_SET rv=%d\n"),
                      unit, lport, rv));
            return rv;
        }
    }

    /* Enable the MAC. */
    rv = MAC_ENABLE_SET(PORT(unit, lport).p_mac, unit, lport, TRUE);
    if (SOC_FAILURE(rv)) {
        LOG_WARN(BSL_LS_BCM_PORT,
                 (BSL_META_U(unit,
                             "u=%d p=%d MAC_ENABLE_SET TRUE rv=%d\n"),
                  unit, lport, rv));
        return rv;
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      pcm_phyctrl_port_cable_diag
 * Description:
 *      Run Cable Diagnostics on port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      status - (OUT) cable diag status structure
 * Return Value:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 * Notes:
 *      Cable diagnostics are only supported by some phy types
 *      (currently 5248 10/100 phy and 546x 10/100/1000 phys)
 */
int
pcm_phyctrl_phy_cable_diag(int unit, int lport, pcm_port_cable_diag_t *status)
{
    return soc_phyctrl_cable_diag(unit, lport, status);
}

/*
 * Function:
 *      pcm_phyctrl_port_cable_diag_support
 * Description:
 *      Cable Diagnostics Suppot on port
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      support - (OUT) cable diag support
 * Return Value:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 * Notes:
 *      Cable diagnostics are only supported by some phy types
 *      (currently 5248 10/100 phy and 546x 10/100/1000 phys)
 */
int
pcm_phyctrl_phy_cable_diag_support(int unit, int lport, int *support)
{
    phy_ctrl_t *ext_pc;

    *support = 0;
    ext_pc = EXT_PHY_SW_STATE(unit, lport);

    if (ext_pc != NULL &&
        ext_pc->pd != NULL &&
        ext_pc->pd->pd_cable_diag != NULL) {
        *support = 1;
    }

     return SOC_E_NONE;
}

/*
 * Function:
 *      pcm_phyctrl_port_get_driver_name
 * Description:
 *      Get driver name
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      internal - 1: internal PHY
 * Return Value:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 * Notes:
 */
const char *
pcm_phyctrl_phy_get_driver_name(int unit, int lport, uint32 internal)
{
    phy_driver_t *pd;

    pd = NULL;
    if (!internal) {
        if (EXT_PHY_SW_STATE(unit, lport) == NULL) {
            return NULL;
        }
        pd = EXT_PHY_SW_STATE(unit, lport)->pd;
        return PHY_NAME(unit, lport);
    } else {
        if (INT_PHY_SW_STATE(unit, lport) == NULL) {
            return NULL;
        }
        if (EXT_PHY_SW_STATE(unit, lport) == NULL) {
            return PHY_NAME(unit, lport);
        }
        pd = INT_PHY_SW_STATE(unit, lport)->pd;
    }

    if (pd == NULL) {
        return NULL;
    }

    return pd->drv_name;
}

/*
 * Function:
 *      pcm_phyctrl_port_addr_get
 *      Get driver name
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      iaddr - Internal PHY address
 *      addr -  External PHY address
 * Return Value:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 * Notes:
 */
int
pcm_phyctrl_phy_addr_get(int unit, int lport, uint32 *iaddr, uint32 *addr)
{
    *iaddr = soc_phy_addr_int_of_port(unit, lport);
    *addr = soc_phy_addr_of_port(unit, lport);

    return SYS_OK;
}

/* Please refer to mdk/phy/include/phy/phy_reg.h for access method */
#define LSHIFT32(_val, _cnt) ((uint32_t)(_val) << (_cnt))

/* Register layout */
#define PHY_REG_ACCESS_METHOD_SHIFT 28
#define PHY_REG_ACCESS_FLAGS_SHIFT  24
#define PHY_REG_ACC_XAUI_IBLK_CL22  LSHIFT32(0x8, PHY_REG_ACCESS_FLAGS_SHIFT)
/* Flags for PHY_REG_ACC_BRCM_XE method */
#define PHY_REG_ACC_BRCM_XE_SHADOW  LSHIFT32(0x8, PHY_REG_ACCESS_FLAGS_SHIFT)
/* Access methods */
#define PHY_REG_ACC_RAW             LSHIFT32(0, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_SHADOW     LSHIFT32(1, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_1000X      LSHIFT32(2, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_XGS_IBLK        LSHIFT32(3, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_XAUI_IBLK       LSHIFT32(4, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_AER_IBLK        LSHIFT32(5, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_XE         LSHIFT32(6, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_TSC_IBLK        LSHIFT32(7, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_BRCM_RDB        LSHIFT32(8, PHY_REG_ACCESS_METHOD_SHIFT)
#define PHY_REG_ACC_MASK            LSHIFT32(0xF, PHY_REG_ACCESS_METHOD_SHIFT)

#define PHY_REG_ACCESS_METHOD(_r)   \
    ((_r) & LSHIFT32(0xf, PHY_REG_ACCESS_METHOD_SHIFT))
#define PHY_REG_IBLK_DEVAD(_r)  \
    (((_r) >> PHY_REG_ACCESS_FLAGS_SHIFT) & 0x7)

#define PHY_BUS_READ(_pc,_r,_v)   pcm_phyctrl_phy_bus_read(_pc,_r,_v)
#define PHY_BUS_WRITE(_pc,_r,_v)  pcm_phyctrl_phy_bus_write(_pc,_r,_v)

/* Transform register address from software API to datasheet format */
#define PHY_XGS_IBLK_TO_C45(_a) \
    (((_a) & 0xf) | (((_a) >> 8) & 0x7ff0) | (((_a) << 11) & 0x8000));

/* Transform register address from datasheet to software API format */
#define PHY_XGS_C45_TO_IBLK(_a) \
    ((((_a) & 0x7ff0) << 8) | (((_a) & 0x8000) >> 11) | ((_a) & 0xf))

/*
 * Always set bit 15 in block address register 0x1f to make the
 * register contents more similar to the clause 45 address.
 */
#ifndef PHY_XGS_IBLK_DBG_BIT15
#define PHY_XGS_IBLK_DBG_BIT15  1
#endif

int
pcm_phyctrl_phy_bus_read(phy_ctrl_t *pc, uint32 addr, uint32 *v)
{
    uint16 val;
    int rv;

    rv = pc->read(pc->unit, pc->phy_id, addr, &val);
    *v = val;

    return rv;
}

int
pcm_phyctrl_phy_bus_write(phy_ctrl_t *pc, uint32 addr, uint32 v)
{
    return pc->write(pc->unit, pc->phy_id, addr, v);
}

static int
pcm_phyctrl_phy_brcm_shadow_write(phy_ctrl_t *pc, uint32 addr, uint32 data)
{
    int ioerr = 0;
    uint32 reg_addr = addr & 0x1f;
    uint32 reg_bank = (addr >> 8) & 0xffff;
    uint32 reg_dev = (addr >> 24) & 0xf;

    switch(reg_addr) {
    case 0x0e:
        ioerr += PHY_BUS_WRITE(pc, 0x0d, reg_dev);
        ioerr += PHY_BUS_WRITE(pc, 0x0e, reg_bank);
        ioerr += PHY_BUS_WRITE(pc, 0x0d, reg_dev | 0x4000);
        break;
    case 0x15:
        ioerr += PHY_BUS_WRITE(pc, 0x17, reg_bank);
        break;
    case 0x18:
        if (reg_bank == 0x0007) {
            data |= 0x8000;
        }
        data = (data & ~(0x0007)) | reg_bank;
        break;
    case 0x1c:
        data = 0x8000 | (reg_bank << 10) | (data & 0x03ff);
        break;
    case 0x1d:
        /* Read-only register */
    default:
        break;
    }
    ioerr += PHY_BUS_WRITE(pc, reg_addr, data);

    return ioerr;
}

int
pcm_phyctrl_phy_brcm_shadow_read(phy_ctrl_t *pc, uint32 addr, uint32 *data)
{
    int ioerr = 0;
    uint32 reg_addr = addr & 0x1f;
    uint32 reg_bank = (addr >> 8) & 0xffff;
    uint32 reg_dev = (addr >> 24) & 0xf;

    switch(reg_addr) {
    case 0x0e:
        ioerr += PHY_BUS_WRITE(pc, 0x0d, reg_dev);
        ioerr += PHY_BUS_WRITE(pc, 0x0e, reg_bank);
        ioerr += PHY_BUS_WRITE(pc, 0x0d, reg_dev | 0x4000);
        break;
    case 0x15:
        ioerr += PHY_BUS_WRITE(pc, 0x17, reg_bank);
        break;
    case 0x18:
        ioerr += PHY_BUS_WRITE(pc, reg_addr, (reg_bank << 12) | 0x7);
        break;
    case 0x1c:
        ioerr += PHY_BUS_WRITE(pc, reg_addr, (reg_bank << 10));
        break;
    case 0x1d:
        ioerr += PHY_BUS_WRITE(pc, reg_addr, reg_bank << 15);
        break;
    default:
        break;
    }
    ioerr += PHY_BUS_READ(pc, reg_addr, data);

    return ioerr;
}

int
pcm_phyctrl_phy_brcm_1000x_read(phy_ctrl_t *pc, uint32 addr, uint32 *data)
{
    int ioerr = 0;
    uint32 reg_addr = addr & 0x1f;
    uint32 blk_sel;;

    /* Map fiber registers */
    ioerr += PHY_BUS_WRITE(pc, 0x1c, 0x7c00);
    ioerr += PHY_BUS_READ(pc, 0x1c, &blk_sel);
    ioerr += PHY_BUS_WRITE(pc, 0x1c, blk_sel | 0x8001);

    /* Read requested fiber register */
    ioerr += PHY_BUS_READ(pc, reg_addr, data);

    /* Map copper registers */
    ioerr += PHY_BUS_WRITE(pc, 0x1c, (blk_sel & 0x7ffe) | 0x8000);

    return ioerr;
}

int
pcm_phyctrl_phy_brcm_1000x_write(phy_ctrl_t *pc, uint32 addr, uint32 data)
{
    int ioerr = 0;
    uint32 reg_addr = addr & 0x1f;
    uint32 blk_sel;;

    /* Map fiber registers */
    ioerr += PHY_BUS_WRITE(pc, 0x1c, 0x7c00);
    ioerr += PHY_BUS_READ(pc, 0x1c, &blk_sel);
    ioerr += PHY_BUS_WRITE(pc, 0x1c, blk_sel | 0x8001);

    /* Write requested fiber register */
    ioerr += PHY_BUS_WRITE(pc, reg_addr, data);

    /* Map copper registers */
    ioerr += PHY_BUS_WRITE(pc, 0x1c, (blk_sel & 0x7ffe) | 0x8000);

    return ioerr;
}

int
pcm_phyctrl_phy_brcm_rdb_read(phy_ctrl_t *pc, uint32 addr, uint32 *data)
{
    int ioerr = 0;
    uint32 regaddr = addr & 0xffff;

    ioerr += PHY_BUS_WRITE(pc, 0x17, 0x0f7e);
    ioerr += PHY_BUS_WRITE(pc, 0x15, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, 0x1e, regaddr);
    ioerr += PHY_BUS_READ(pc, 0x1f, data);
    ioerr += PHY_BUS_WRITE(pc, 0x1e, 0x0087);
    ioerr += PHY_BUS_WRITE(pc, 0x1f, 0x8000);

    return ioerr;
}

int
pcm_phyctrl_phy_brcm_rdb_write(phy_ctrl_t *pc, uint32 addr, uint32 data)
{
    int ioerr = 0;
    uint32 regaddr = addr & 0xffff;

    ioerr += PHY_BUS_WRITE(pc, 0x17, 0x0f7e);
    ioerr += PHY_BUS_WRITE(pc, 0x15, 0x0000);
    ioerr += PHY_BUS_WRITE(pc, 0x1e, regaddr);
    ioerr += PHY_BUS_WRITE(pc, 0x1f, data);
    ioerr += PHY_BUS_WRITE(pc, 0x1e, 0x0087);
    ioerr += PHY_BUS_WRITE(pc, 0x1f, 0x8000);

    return ioerr;
}

int
pcm_phyctrl_phy_xgs_iblk_read(phy_ctrl_t *pc, uint32 addr, uint32 *data)
{
    int ioerr = 0;
    uint32_t devad = PHY_REG_IBLK_DEVAD(addr);
    uint32_t blkaddr;
    int clause45 = 1;

    if (clause45) {

        addr = PHY_XGS_IBLK_TO_C45(addr);
        /* DEVAD 0 is not supported, so use DEVAD 1 instead */
        if (devad == 0) {
            devad = 1;
        }
        ioerr += PHY_BUS_READ(pc, addr | (devad << 16), data);
        return ioerr;
    }

    /* Select device if non-zero */
    if (devad) {
        ioerr += PHY_BUS_WRITE(pc, 0x1f, 0xffde);
        ioerr += PHY_BUS_WRITE(pc, 0x1e, devad << 11);
    }

    /* Select block */
    blkaddr = (addr >> 8) & 0xffff;
#if PHY_XGS_IBLK_DBG_BIT15
    blkaddr |= 0x8000;
#endif
    ioerr += PHY_BUS_WRITE(pc, 0x1f, blkaddr);

    /* Read register value */
    ioerr += PHY_BUS_READ(pc, addr & 0x1f, data);

    /* Restore device type zero */
    if (devad) {
        ioerr += PHY_BUS_WRITE(pc, 0x1f, 0xffde);
        ioerr += PHY_BUS_WRITE(pc, 0x1e, 0);
    }

    return ioerr;
}

int
pcm_phyctrl_phy_xgs_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t devad = PHY_REG_IBLK_DEVAD(addr);
    uint32_t blkaddr;
    int clause45 = 1;
    if (clause45) {
        addr = PHY_XGS_IBLK_TO_C45(addr);
        /* DEVAD 0 is not supported, so use DEVAD 1 instead */
        if (devad == 0) {
            devad = 1;
        }
        ioerr += PHY_BUS_WRITE(pc, addr | (devad << 16), data);
        return ioerr;
    }

    /* Select device if non-zero */
    if (devad) {
        ioerr += PHY_BUS_WRITE(pc, 0x1f, 0xffde);
        ioerr += PHY_BUS_WRITE(pc, 0x1e, devad << 11);
    }

    /* Select block */
    blkaddr = (addr >> 8) & 0xffff;
#if PHY_XGS_IBLK_DBG_BIT15
    blkaddr |= 0x8000;
#endif
    ioerr += PHY_BUS_WRITE(pc, 0x1f, blkaddr);

    /* Write register value */
    ioerr += PHY_BUS_WRITE(pc, addr & 0x1f, data);

    /* Restore device type zero */
    if (devad) {
        ioerr += PHY_BUS_WRITE(pc, 0x1f, 0xffde);
        ioerr += PHY_BUS_WRITE(pc, 0x1e, 0);
    }

    return ioerr;
}

int
pcm_phyctrl_phy_xaui_iblk_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data)
{
    int ioerr = 0;
    uint32_t misc_ctrl;

    if ((addr & 0x10) == 0) {
        /* Map Block 0 */
        ioerr += PHY_BUS_WRITE(pc, 0x1f, 0);
        /* Read IEEE mapping */
        ioerr += PHY_BUS_READ(pc, 0x1e, &misc_ctrl);
        misc_ctrl &= ~0x0003;
        if ((addr & PHY_REG_ACC_XAUI_IBLK_CL22) == 0) {
            misc_ctrl |= 0x0001;
        }
        /* Update IEEE mapping */
        ioerr += PHY_BUS_WRITE(pc, 0x1e, misc_ctrl);
    }
    /* Read mapped register */
    ioerr += pcm_phyctrl_phy_xgs_iblk_read(pc, addr, data);

    return ioerr;
}

static int
pcm_phyctrl_phy_xaui_iblk_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t misc_ctrl;

    if ((addr & 0x10) == 0) {
        /* Map Block 0 */
        ioerr += PHY_BUS_WRITE(pc, 0x1f, 0);
        /* Read IEEE mapping */
        ioerr += PHY_BUS_READ(pc, 0x1e, &misc_ctrl);
        misc_ctrl &= ~0x0003;
        if ((addr & PHY_REG_ACC_XAUI_IBLK_CL22) == 0) {
            misc_ctrl |= 0x0001;
        }
        /* Update IEEE mapping */
        ioerr += PHY_BUS_WRITE(pc, 0x1e, misc_ctrl);
    }
    /* Write mapped register */
    ioerr += pcm_phyctrl_phy_xgs_iblk_write(pc, addr, data);

    return ioerr;
}

/* Shadow registers reside at offset 0xfff0 in clause 45 device 7 */
#define _SHDW(_regad) ((_regad) | 0x7fff0)

static int
pcm_phyctrl_phy_brcm_xe_write(phy_ctrl_t *pc, uint32_t addr, uint32_t data)
{
    int ioerr = 0;
    uint32_t reg_addr, reg_bank;

    if (addr & PHY_REG_ACC_BRCM_XE_SHADOW) {
        reg_addr = addr & 0x1f;
        reg_bank = (addr >> 8) & 0xffff;

        switch(reg_addr) {
        case 0x15:
            ioerr += PHY_BUS_WRITE(pc, _SHDW(0x17), reg_bank);
            break;
        case 0x18:
            if (reg_bank == 0x0007) {
                data |= 0x8000;
            }
            data = (data & ~(0x0007)) | reg_bank;
            break;
        case 0x1c:
            data = 0x8000 | (reg_bank << 10) | (data & 0x03ff);
            break;
        default:
            break;
        }
        ioerr += PHY_BUS_WRITE(pc, _SHDW(reg_addr), data);
    } else {
        ioerr += PHY_BUS_WRITE(pc, addr & 0x1fffff, data);
    }

    return ioerr;
}

/* Shadow registers reside at offset 0xfff0 in clause 45 device 7 */
#define _SHDW(_regad) ((_regad) | 0x7fff0)

static int
pcm_phyctrl_phy_brcm_xe_read(phy_ctrl_t *pc, uint32_t addr, uint32_t *data)
{
    int ioerr = 0;
    uint32_t reg_addr, reg_bank;

    if (addr & PHY_REG_ACC_BRCM_XE_SHADOW) {
        reg_addr = addr & 0x1f;
        reg_bank = (addr >> 8) & 0xffff;

        switch(reg_addr) {
        case 0x15:
            ioerr += PHY_BUS_WRITE(pc, _SHDW(0x17), reg_bank);
            break;
        case 0x18:
            ioerr += PHY_BUS_WRITE(pc, _SHDW(reg_addr), (reg_bank << 12) | 0x7);
            break;
        case 0x1c:
            ioerr += PHY_BUS_WRITE(pc, _SHDW(reg_addr), (reg_bank << 10));
            break;
        default:
            break;
        }
        ioerr += PHY_BUS_READ(pc, _SHDW(reg_addr), data);
    } else {
        ioerr += PHY_BUS_READ(pc, addr & 0x1fffff, data);
    }

    return ioerr;
}

/*
 * Function:
 *      pcm_phyctrl_phy_reg_read
 * PHY register read function
 *
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      flags -
 *      phy_reg_addr - address of phy register
 * Return Value:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 * Notes:
 */
int
pcm_phyctrl_phy_reg_get(int unit, int lport, uint32 index, uint32 phy_reg_addr,
                        uint32 *data)
{
    phy_ctrl_t *pc;
    const phymod_access_t *pa;
    int r = SOC_E_NONE;

    if (index == 0 && EXT_PHY_SW_STATE(unit, lport) != NULL) {
        pc = EXT_PHY_SW_STATE(unit, lport);
    } else {
        pc = INT_PHY_SW_STATE(unit, lport);
    }

    if (pc->phymod_ctrl.phy[0] != NULL) {
        pa = &(pc->phymod_ctrl.phy[0]->pm_phy.access);
    } else {
        pa = NULL;
    }

    switch (PHY_REG_ACCESS_METHOD(phy_reg_addr)) {
    case PHY_REG_ACC_BRCM_SHADOW:
        r = pcm_phyctrl_phy_brcm_shadow_read(pc, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_BRCM_1000X:
        r = pcm_phyctrl_phy_brcm_1000x_read(pc, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_XGS_IBLK:
        r = pcm_phyctrl_phy_xgs_iblk_read(pc, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_XAUI_IBLK:
        r = pcm_phyctrl_phy_xaui_iblk_read(pc, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_AER_IBLK:
        if (pa == NULL) {
            return SOC_E_FAIL;
        }
        /* TO-CHECK ME */
        r = phymod_bus_read(pa, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_BRCM_XE:
        r = pcm_phyctrl_phy_brcm_xe_read(pc, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_TSC_IBLK:
        if (pa == NULL) {
            return SOC_E_FAIL;
        }
        /* TO-CHECK ME */
        r = phymod_bus_read(pa, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_BRCM_RDB:
        r = pcm_phyctrl_phy_brcm_rdb_read(pc, phy_reg_addr, data);
        break;
    default:
        r = PHY_BUS_READ(pc, phy_reg_addr, data);
        break;
    }

    return r;
}

/*
 * Function:
 *      pcm_phyctrl_phy_reg_write
 * PHY register write function
 *
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      flags -
 * Return Value:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 * Notes:
 */
int
pcm_phyctrl_phy_reg_set(int unit, int lport, uint32 flags, uint32 phy_reg_addr,
                        uint32 data)
{
    phy_ctrl_t *pc;
    const phymod_access_t *pa;
    int r = SOC_E_NONE;

    if (flags == 0) {
        pc = EXT_PHY_SW_STATE(unit, lport);
    } else {
        pc = INT_PHY_SW_STATE(unit, lport);
    }

    if (pc == NULL) return SOC_E_FAIL;

    if (pc->phymod_ctrl.phy[0] != NULL) {
        pa = &(pc->phymod_ctrl.phy[0]->pm_phy.access);
    } else {
        pa = NULL;
    }

    switch (PHY_REG_ACCESS_METHOD(phy_reg_addr)) {
    case PHY_REG_ACC_BRCM_SHADOW:
        r = pcm_phyctrl_phy_brcm_shadow_write(pc, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_BRCM_1000X:
        r = pcm_phyctrl_phy_brcm_1000x_write(pc, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_XGS_IBLK:
        r = pcm_phyctrl_phy_xgs_iblk_write(pc, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_XAUI_IBLK:
        r = pcm_phyctrl_phy_xaui_iblk_write(pc, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_AER_IBLK:
        if (pa == NULL) {
            return SOC_E_FAIL;
        }
        r = phymod_bus_write(pa, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_BRCM_XE:
        r = pcm_phyctrl_phy_brcm_xe_write(pc, phy_reg_addr, data);
        break;
    case PHY_REG_ACC_TSC_IBLK:
        if (pa == NULL) {
            return SOC_E_FAIL;
        }
        
        /* r = phymod_tsc_iblk_write(pa, phy_reg_addr, data); */
        r = SOC_E_NONE;
        break;
    case PHY_REG_ACC_BRCM_RDB:
        r = pcm_phyctrl_phy_brcm_rdb_write(pc, phy_reg_addr, data);
        break;
    default:
        r = PHY_BUS_WRITE(pc, phy_reg_addr, data);
        break;
    }

    return r;
}

/*
 * Function:
 *      pcm_phyctrl_port_eee_enable_set
 *      Enable/Disable EEE mode
 * Parameters:
 *      unit - Unit #.
 *      lport - Logical port #.
 *      enable - 1: enable 0:disable
 *      mode : 0: Disable EEE feature
 *                 1: Native EEE
 *                 2: Auto Green EEE
 * Return Value:
 *    _SHR_E_NONE
 *     _SHR_E_XXX
 * Notes:
 */
int
pcm_phyctrl_port_eee_enable_set(int unit, int lport, int enable, int* mode)
{
    int rv = SOC_E_UNAVAIL, rv_mac, rv_phy;
    int mac_val;
    uint32 phy_val;

    *mode = 0;

    rv_mac = MAC_CONTROL_GET(PORT(unit, lport).p_mac, unit, lport,
                             SOC_MAC_CONTROL_EEE_ENABLE, &mac_val);
    rv_phy = soc_phyctrl_control_get(unit, lport,
                                     _SHR_PORT_PHY_CONTROL_EEE, &phy_val);
    if (rv_mac != SOC_E_UNAVAIL && rv_phy != SOC_E_UNAVAIL) {
        /* If MAC/Switch is EEE aware (Native EEE mode is supported)
         * and PHY also supports Native EEE mode
         */

        /* a. Disable AutoGrEEEn mode by PHY if applicable */
        rv = soc_phyctrl_control_get(unit, lport,
                                     _SHR_PORT_PHY_CONTROL_EEE_AUTO, &phy_val);
        if ((rv != SOC_E_UNAVAIL) && (phy_val != 0)) {
            rv = soc_phyctrl_control_set(unit, lport,
                                         _SHR_PORT_PHY_CONTROL_EEE_AUTO, 0);
        }

        /* b. Enable/Disable Native EEE in PHY */
        rv = soc_phyctrl_control_set(unit, lport, _SHR_PORT_PHY_CONTROL_EEE,
                                     enable ? 1 : 0);
        if (SOC_SUCCESS(rv)) {
            /*
             * If (enable ==1), EEE will be enabled in MAC after 1 sec.
             * during linkscan update
             */
            if (enable == 0) {
                /* Disable EEE in MAC immediately*/
                rv = MAC_CONTROL_SET(PORT(unit, lport).p_mac, unit, lport,
                                     SOC_MAC_CONTROL_EEE_ENABLE, 0);
            } else {
                *mode = 1;
            }

            /*
             * Notify Int-PHY to bypass LPI for native EEE mode.
             *
             * Note :
             *  1. Not all internal SerDes support the setting to
             *      enable/disable bypass LPI signal.
             *  2. Int-PHY to bypass LPI will sync with Ext-PHY's EEE
             *      enabling status for Native EEE mode.
             */
            (void)soc_phyctrl_notify(unit, lport, phyEventLpiBypass,
                                     enable ? 1: 0);
        }
    } else {
        /*
         * If native EEE mode is not supported,
         * set PHY in AutoGrEEEn mode.
         */

        /* a. Disable Native EEE mode in PHY if applicable */
        rv = soc_phyctrl_control_get(unit, lport, _SHR_PORT_PHY_CONTROL_EEE,
                                     &phy_val);
        if ((rv != SOC_E_UNAVAIL) && (phy_val != 0)) {
            rv = soc_phyctrl_control_set(unit, lport,
                                         _SHR_PORT_PHY_CONTROL_EEE, 0);
        }

        /*
         * b. Enable/Disable AutoGrEEEn in PHY.
         *
         * If PHY does not support AutoGrEEEn mode,
         * rv will be assigned SOC_E_UNAVAIL.
         */
        rv = soc_phyctrl_control_set(unit, lport,
                                     _SHR_PORT_PHY_CONTROL_EEE_AUTO,
                                     enable ? 1 : 0);
        if (rv != SOC_E_NONE) {
            *mode = 0;
        } else {
            *mode = 2;
        }
    }

    return rv;
}

/*
 * Function:
 *      _bcm_port_software_init
 * Purpose:
 *      Initialization of software for port subsystem.
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_NONE - success (or already initialized)
 *      BCM_E_INTERNAL- failed to write PTABLE entries
 *      BCM_E_MEMORY - failed to allocate required memory.
 */
static int
pcm_phyctrl_port_software_init(int unit)
{
    return soc_phy_common_init(unit);
}

/*
 * Function:
 *      pcm_phyctrl_port_mode_setup
 * Purpose:
 *      Set initial operating mode for a port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      enable - Whether to enable or disable
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Must be called with PORT_LOCK held.
 */
static int
pcm_phyctrl_port_mode_setup(int unit, int port, int enable)
{
    soc_port_if_t pif;
    soc_port_ability_t local_pa, advert_pa;

    sal_memset(&local_pa, 0, sizeof(soc_port_ability_t));
    sal_memset(&advert_pa, 0, sizeof(soc_port_ability_t));

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "_bcm_port_mode_setup: u=%d p=%d\n"), unit, port));

    SOC_IF_ERROR_RETURN
        (pcm_phyctrl_port_ability_local_get(unit, port, &local_pa));

    /* If MII supported, enable it, otherwise use TBI */
    if (local_pa.interface &
        (SOC_PA_INTF_MII | SOC_PA_INTF_GMII |
         SOC_PA_INTF_SGMII | SOC_PA_INTF_XGMII)) {
        if (IS_GX_PORT(port)) {
            pif = SOC_PORT_IF_GMII;
        } else if (IS_XE_PORT(port)) {
            if (local_pa.interface & SOC_PA_INTF_XGMII) {
                pif = SOC_PORT_IF_XGMII;
            } else { /*  external GbE phy in xe port mode */
                pif = SOC_PORT_IF_SGMII;
            }
        } else {
            pif = SOC_PORT_IF_MII;
        }
    } else if (local_pa.interface & SOC_PA_INTF_CGMII) {
        pif = SOC_PORT_IF_CGMII;
    } else {
        pif = SOC_PORT_IF_TBI;
    }

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_interface_set(unit, port, pif));

    SOC_IF_ERROR_RETURN
        (MAC_INTERFACE_SET(PORT(unit, port).p_mac, unit, port, pif));

    SOC_IF_ERROR_RETURN
        (MAC_ENABLE_SET(PORT(unit, port).p_mac, unit, port, enable));

    return SOC_E_NONE;
}

int
pcm_phyctrl_port_mode_to_ability(soc_port_mode_t mode,
                                 soc_port_ability_t *ability)
{
    uint32 port_abil;

    if (!ability) {
        return SOC_E_PARAM;
    }

    /* Half duplex speeds */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_10MB_HD) ? SOC_PA_SPEED_10MB : 0;
    port_abil |= (mode & SOC_PM_100MB_HD) ? SOC_PA_SPEED_100MB : 0;
    port_abil |= (mode & SOC_PM_1000MB_HD) ? SOC_PA_SPEED_1000MB : 0;
    port_abil |= (mode & SOC_PM_2500MB_HD) ? SOC_PA_SPEED_2500MB : 0;
    port_abil |= (mode & SOC_PM_3000MB_HD) ? SOC_PA_SPEED_3000MB : 0;
    port_abil |= (mode & SOC_PM_10GB_HD) ? SOC_PA_SPEED_10GB : 0;
    port_abil |= (mode & SOC_PM_12GB_HD) ? SOC_PA_SPEED_12GB : 0;
    port_abil |= (mode & SOC_PM_13GB_HD) ? SOC_PA_SPEED_13GB : 0;
    port_abil |= (mode & SOC_PM_16GB_HD) ? SOC_PA_SPEED_16GB : 0;
    ability->speed_half_duplex = port_abil;

    /* Full duplex speeds */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_10MB_FD) ? SOC_PA_SPEED_10MB : 0;
    port_abil |= (mode & SOC_PM_100MB_FD) ? SOC_PA_SPEED_100MB : 0;
    port_abil |= (mode & SOC_PM_1000MB_FD) ? SOC_PA_SPEED_1000MB : 0;
    port_abil |= (mode & SOC_PM_2500MB_FD) ? SOC_PA_SPEED_2500MB : 0;
    port_abil |= (mode & SOC_PM_3000MB_FD) ? SOC_PA_SPEED_3000MB : 0;
    port_abil |= (mode & SOC_PM_10GB_FD) ? SOC_PA_SPEED_10GB : 0;
    port_abil |= (mode & SOC_PM_12GB_FD) ? SOC_PA_SPEED_12GB : 0;
    port_abil |= (mode & SOC_PM_13GB_FD) ? SOC_PA_SPEED_13GB : 0;
    port_abil |= (mode & SOC_PM_16GB_FD) ? SOC_PA_SPEED_16GB : 0;
    ability->speed_full_duplex = port_abil;

    /* Pause Modes */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_PAUSE_TX)? SOC_PA_PAUSE_TX : 0;
    port_abil |= (mode & SOC_PM_PAUSE_RX) ? SOC_PA_PAUSE_RX : 0;
    port_abil |= (mode & SOC_PM_PAUSE_ASYMM) ? SOC_PA_PAUSE_ASYMM : 0;
    ability->pause = port_abil;

    /* Interface Types */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_TBI) ? SOC_PA_INTF_TBI : 0;
    port_abil |= (mode & SOC_PM_MII) ? SOC_PA_INTF_MII : 0;
    port_abil |= (mode & SOC_PM_GMII) ? SOC_PA_INTF_GMII : 0;
    port_abil |= (mode & SOC_PM_SGMII) ? SOC_PA_INTF_SGMII : 0;
    port_abil |= (mode & SOC_PM_XGMII) ? SOC_PA_INTF_XGMII : 0;
    ability->interface = port_abil;

    /* Loopback Mode */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_LB_MAC) ? SOC_PA_LB_MAC : 0;
    port_abil |= (mode & SOC_PM_LB_PHY) ? SOC_PA_LB_PHY : 0;
    port_abil |= (mode & SOC_PM_LB_NONE) ? SOC_PA_LB_NONE : 0;
    ability->loopback = port_abil;

    /* Remaining Flags */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_AN) ? SOC_PA_AUTONEG : 0;
    port_abil |= (mode & SOC_PM_COMBO) ? SOC_PA_COMBO : 0;
    ability->flags = port_abil;

    return SOC_E_NONE;
}

/*
 * Function:
 *      _bcm_port_mac_init
 * Purpose:
 *      Set up the mac of the indicated port
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      port - Port to setup
 *      okay - Output parameter indicates port can be enabled.
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_INTERNAL - internal error.
 * Notes:
 *      If error is returned, the port should not be enabled.
 */

static int
pcm_phyctrl_port_mac_init(int unit, bcm_port_t port, int *okay)
{
    int rv;
    mac_driver_t *macd;

    *okay = FALSE;

    LOG_VERBOSE(BSL_LS_BCM_PORT,
                (BSL_META_U(unit,
                            "Init port %d MAC...\n"), port));

    if ((rv = soc_mac_probe(unit, port, &macd)) < 0) {
        LOG_WARN(BSL_LS_BCM_PORT,
                 (BSL_META_U(unit,
                             "Port %s: Failed to probe MAC: %s\n"),
                  SOC_PORT_NAME(unit, port), soc_errmsg(rv)));
        return rv;
    }

    PORT(unit, port).p_mac = macd;

    if ((rv = MAC_INIT(PORT(unit, port).p_mac, unit, port)) < 0) {
         LOG_WARN(BSL_LS_BCM_PORT,
                  (BSL_META_U(unit,
                              "Port %s: Failed to initialize MAC: %s\n"),
                   SOC_PORT_NAME(unit, port), soc_errmsg(rv)));
          return rv;
    }

    /* Probe function should leave port disabled */
    SOC_IF_ERROR_RETURN
        (MAC_ENABLE_SET(PORT(unit, port).p_mac, unit, port, 0));

    *okay = TRUE;

    return SOC_E_NONE;
}

/*
 * Function:
 *      bcm_port_probe
 * Purpose:
 *      Probe the PHY and set up the PHY and MAC for the specified ports.
 *      This is purely a discovery routine and does no configuration.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      pbmp - Bitmap of ports to probe.
 *      okay_pbmp (OUT) - Ports which were successfully probed.
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_INTERNAL - internal error.
 * Notes:
 *      If error is returned, the port should not be enabled.
 *      Assumes port_init done.
 *      Note that if a PHY is not present, the port will still probe
 *      successfully.  The default driver will be installed.
 */
int
pcm_phyctrl_port_probe(int unit, pbmp_t pbmp, pbmp_t *okay_pbmp)
{
    int rv = SOC_E_NONE;
    int port;
    int okay;

    PBMP_CLEAR(*okay_pbmp);

    rv = soc_phyctrl_pbm_probe_init(unit, pbmp, okay_pbmp);

    PBMP_ITER(*okay_pbmp, port) {
        /* Probe function should leave port disabled */
        rv = soc_phyctrl_enable_set(unit, port, 0);
        if (rv < 0) {
            break;
        }
    }

    PBMP_ITER(pbmp, port) {
        rv = pcm_phyctrl_port_mac_init(unit, port, &okay);
        if (!okay) {
            PBMP_PORT_REMOVE(*okay_pbmp, port);
        }
        if (rv < 0) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "MAC init failed on port %s\n"),
                      SOC_PORT_NAME(unit, port)));
            break;
        }
    }

    return rv;
}

/*
 * Function:
 *      _bcm_port_settings_init
 * Purpose:
 *      Initialize port settings if they are to be different from the
 *      default ones
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      port - port number
 * Returns:
 *      BCM_E_NONE - success (or already initialized)
 *      BCM_E_INTERNAL- failed to write PTABLE entries
 * Notes:
 *      This function initializes port settings based on the folowing config
 *      variables:
 *           port_init_speed
 *           port_init_duplex
 *           port_init_adv
 *           port_init_autoneg
 *      If a variable is not set, then no additional initialization of the
 *      corresponding parameter is done (and the defaults will normally be
 *      advertize everything you can do and use autonegotiation).
 *
 *      A typical use would be to set:
 *          port_init_adv=0
 *          port_init_autoneg=1
 *      to force link down in the beginning.
 *
 *      Another setup that makes sense is something like:
 *          port_init_speed=10
 *          port_init_duplex=0
 *          port_init_autoneg=0
 *      in order to force link into a certain mode. (It is very important to
 *      disable autonegotiation in this case).
 *
 *      PLEASE NOTE:
 *          The standard rc.soc forces autoneg=on on all the ethernet ports
 *          (FE and GE). Thus, to use the second example one has to edit rc.soc
 *          as well.
 *
 *     This function has been declared as global, but not exported. This will
 *     make port initialization easier when using VxWorks shell.
 */
int
pcm_phyctrl_port_settings_init(int unit, int port)
{
    int an, duplex, speed, adv;
    soc_port_ability_t local_adv;

    LOG_INFO(BSL_LS_BCM_PORT,
             (BSL_META_U(unit,
                         "bcm_port_settings_init: u=%d p=%d\n"),
              unit, port));

    an = soc_property_port_get(unit, port, spn_PORT_INIT_AUTONEG, -1);
    if (an != -1) {
        pcm_phyctrl_port_autoneg_set(unit, port, an);
    } else {
        pcm_phyctrl_port_autoneg_get(unit, port, &an);
    }

    speed = soc_property_port_get(unit, port, spn_PORT_INIT_SPEED, -1);
    if (speed != -1 ) {
        if (an) {
            LOG_VERBOSE(BSL_LS_BCM_PORT,
                        (BSL_META_U(unit,
                                    "bcm_port_speed_set failed due to AN enable"
                                    "\n")));
        } else {
            pcm_phyctrl_port_speed_set(unit, port, speed);
        }
    } else {
        pcm_phyctrl_port_speed_get(unit, port, &speed);
    }

    duplex = soc_property_port_get(unit, port, spn_PORT_INIT_DUPLEX, -1);
    if (duplex != -1) {
        if (an) {
            LOG_VERBOSE(BSL_LS_BCM_PORT,
                        (BSL_META_U(unit,
                                    "pcm_phyctrl_port_duplex_set failed due to "
                                    "AN enable\n")));
        } else {
            pcm_phyctrl_port_duplex_set(unit, port, duplex);
        }
    }

    adv = soc_property_port_get(unit, port, spn_PORT_INIT_ADV, -1);

    if (adv != -1) {
        pcm_phyctrl_port_mode_to_ability(adv, &local_adv);
        pcm_phyctrl_port_ability_advert_set(unit, port, &local_adv);
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      bcm_port_init
 * Purpose:
 *      Initialize the PORT interface layer for the specified SOC device.
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_NONE - success (or already initialized)
 *      BCM_E_INTERNAL- failed to write PTABLE entries
 *      BCM_E_MEMORY - failed to allocate required memory.
 * Notes:
 *      By default ports come up enabled. They can be made to come up disabled
 *      at startup by a compile-time application policy flag in your Make.local
 *      PTABLE initialized.
 */
int
pcm_phyctrl_port_probe_init(int unit, pbmp_t lpbmp, pbmp_t *okay_lpbmp)
{
    int rv, port_enable;
    int p;

    LOG_VERBOSE(BSL_LS_BCM_PORT,
                (BSL_META_U(unit,
                            "bcm_port_init: unit %d\n"), unit));

    rv = pcm_phyctrl_port_software_init(unit);
    if (rv != SOC_E_NONE) {
        LOG_ERROR(BSL_LS_BCM_PORT,
                  (BSL_META_U(unit,
                              "Error unit %d:  Failed software port init: "
                              "%s\n"),
                   unit, bcm_errmsg(rv)));
        return rv;
    }

    
    #if 0
    if (SAL_BOOT_SIMULATION &&
        soc_property_get(unit, "skip_port_hw_init", 0)) {
        return SOC_E_NONE;
    }
    #endif
    /* Probe for ports */
    PBMP_CLEAR(*okay_lpbmp);
    rv = pcm_phyctrl_port_probe(unit, lpbmp, okay_lpbmp);
    if (rv != SOC_E_NONE) {
        LOG_ERROR(BSL_LS_BCM_PORT,
                  (BSL_META_U(unit,
                              "Error unit %d:  Failed port probe: %s\n"),
                   unit, bcm_errmsg(rv)));
        return rv;
    }

    
    #if 0
    LOG_VERBOSE(BSL_LS_BCM_PORT,
                (BSL_META_U(unit,
                            "Probed ports okay: %s of %s\n"),
                 SOC_PBMP_FMT(okay_ports, pfmtok),
                 SOC_PBMP_FMT(PBMP_PORT_ALL(unit), pfmtall)));
    #endif
    /*
     * A compile-time application policy may prefer to disable ports
     * when switch boots up
     */
    port_enable = FALSE;

    if(SOC_IS_SABER2(unit)) {
        PBMP_ITER(*okay_lpbmp, p) {
            rv = pcm_phyctrl_port_enable_set(unit, p, port_enable);
            if (rv < 0) {
                LOG_WARN(BSL_LS_BCM_PORT,
                         (BSL_META_U(unit,
                                     "Warning: Unit %d Port %d: "
                                     "Failed to %s port: %s\n"),
                          unit, p,(port_enable) ? "enable" : "disable",
                          bcm_errmsg(rv)));
            }
        }
    }

    /* Probe and initialize MAC and PHY drivers for ports that were OK */
    PBMP_ITER(*okay_lpbmp, p) {
        LOG_VERBOSE(BSL_LS_BCM_PORT,
                    (BSL_META_U(unit,
                                "bcm_port_init: unit %d port %d\n"),
                                unit, p));

        rv = pcm_phyctrl_port_mode_setup(unit, p, TRUE);
        if (rv < 0) {
             LOG_WARN(BSL_LS_BCM_PORT,
                         (BSL_META_U(unit,
                                     "Warning: Port %d: "
                                     "Failed to set initial mode: %s\n"),
                          p, bcm_errmsg(rv)));
        }

        rv = pcm_phyctrl_port_settings_init(unit, p);
        if (rv < 0) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "Warning: Port %d: "
                                 "Failed to configure initial settings: %s\n"),
                      p, bcm_errmsg(rv)));
        }

        /*
         * A compile-time application policy may prefer to disable ports
         * when switch boots up
         */

        port_enable = FALSE;

        if(!SOC_IS_SABER2(unit)) {
            if ((rv = pcm_phyctrl_port_enable_set(unit, p, port_enable)) < 0) {
            LOG_WARN(BSL_LS_BCM_PORT,
                     (BSL_META_U(unit,
                                 "Warning: Port %d: "
                                 "Failed to %s port: %s\n"),
                      p, (port_enable) ? "enable" : "disable",
                      bcm_errmsg(rv)));
            }
        }
    }

    return 0;
}

/*
 * Function:
 *      pcm_phyctrl_port_pause_addr_get
 * Purpose:
 *      Get the source address for transmitted PAUSE frames.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      port - StrataSwitch port #.
 *      mac - (OUT) MAC address sent with pause frames.
  * Returns:
  *      SOC_E_NONE
  *      SOC_E_XXX
  * Notes:
  *      Symmetric pause requires the two "pause" values to be the same.
  */
int
pcm_phyctrl_port_pause_addr_get(int unit, int port, sal_mac_addr_t mac)
{
    int rv;

    rv = MAC_PAUSE_ADDR_GET(PORT(unit, port).p_mac, unit, port, mac);

    return rv;
}

/*
 * Function:
 *      pcm_phyctrl_port_pause_addr_set
 * Purpose:
 *      Set the source address for transmitted PAUSE frames.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      port - StrataSwitch port #.
 *      mac - station address used for pause frames.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_XXX
 * Notes:
 *      Symmetric pause requires the two "pause" values to be the same.
 */
int
pcm_phyctrl_port_pause_addr_set(int unit, int port, sal_mac_addr_t mac)
{
    int rv;

    rv = MAC_PAUSE_ADDR_SET(PORT(unit, port).p_mac, unit, port, mac);

    return rv;
}

int
pcm_phyctrl_phy_notify(int unit, int port, int event, uint32 value)
{
    return soc_phyctrl_notify(unit, port, (soc_phy_event_t)event, value);
}
