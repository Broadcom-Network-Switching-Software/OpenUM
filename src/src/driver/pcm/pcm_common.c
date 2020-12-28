/*! \file pcm_common.c
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifdef __C51__
#ifdef CODE_USERCLASS
#pragma userclass (code = brdimpl)
#endif /* CODE_USERCLASS */
#endif /* __C51__ */

#include "system.h"
#include "utils/ports.h"
#include "appl/persistence.h"
#include <pcm/pcm_int.h>

/*
 * Port Function Vector Driver
 */

/*
 * Variable:
 *      bcm_port_info
 * Purpose:
 *      One entry for each SOC device containing port information
 *      for that device.  Use the PORT macro to access it.
 */
pcm_port_ctrl_t pcm_port_ctrl[SOC_MAX_NUM_PORTS];

/*
 * Function:
 *      pcm_software_init
 * Purpose:
 *      Initialize the PORT interface layer for the specified SOC device.
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      SOC_E_NONE - success (or already initialized)
 *      SOC_E_INTERNAL- failed to write PTABLE entries
 *      SOC_E_MEMORY - failed to allocate required memory.
 * Notes:
 *      By default ports come up enabled. They can be made to come up disabled
 *      at startup by a compile-time application policy flag in your Make.local
 *      PTABLE initialized.
 */
int
pcm_software_init(int unit)
{
    int lport;

#if CONFIG_EMULATION
    sal_config_set("phy_null", "1");
#endif

    SOC_LPORT_ITER(lport) {
        pcm_port_ctrl[lport].f = &pcm_phyctrl;
    }

    pcm_phyctrl.software_init(unit);

    return SYS_OK;
}

/*
 * Function:
 *      pcm_port_probe_init
 * Purpose:
 *      Probe the PHYs and initialize the PHYs and MACs for the specified ports.
 *      This is purely a discovery routine and does no configuration.
 * Parameters:
 *      unit - StrataSwitch unit number.
 *      pbmp - Bitmap of ports to probe and initalize.
 *      okay_pbmp (OUT) - Ports which were successfully probed and intialized.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_INTERNAL - internal error.
 * Notes:
 *      If error is returned, the port should not be enabled.
 *      Assumes port_init done.
 *      Note that if a PHY is not present, the port will still probe
 *      successfully.  The default driver will be installed.
 */

int
pcm_port_probe_init(int unit, pbmp_t lpbmp, pbmp_t *okay_lpbmp)
{
    return  pcm_phyctrl.port_probe_init(unit, lpbmp, okay_lpbmp);
}


/*
 * Function:
 *      pcm_port_loopback_set
 * Purpose:
 *      Setting the speed for a given port
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - StrataSwitch Logical Port #.
 *      loopback - one of:
 *              PCM_PORT_LOOPBACK_NONE
 *              PCM_PORT_LOOPBACK_MAC
 *              PCM_PORT_LOOPBACK_PHY
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
int
pcm_port_loopback_set(int unit, int lport, int loopback)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_loopback_set) {
        rv = pcm_port_ctrl[lport].f->port_loopback_set(unit, lport, loopback);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_loopback_get
 * Purpose:
 *      Get the current loopback operation for the specified port.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - StrataSwitch Logical Port #.
 *      loopback - (OUT)  one of:
 *              PCM_PORT_LOOPBACK_NONE
 *              PCM_PORT_LOOPBACK_MAC
 *              PCM_PORT_LOOPBACK_PHY
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
int
pcm_port_loopback_get(int unit, int lport, int *loopback)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_loopback_get) {
        rv = pcm_port_ctrl[lport].f->port_loopback_get(unit, lport, loopback);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_control_get
 * Description:
 *      Get the status of specified port feature.
 * Parameters:
 *      unit - Device number
 *      lport - Logical Port number
 *      type - Enum  value of the feature
 *      value - (OUT) Current value of the port feature
 * Return Value:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
int
pcm_port_control_get(int unit, int lport, int type, int *value)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_control_get) {
        rv = pcm_port_ctrl[lport].f->port_control_get(unit, lport, type, value);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_control_set
 * Description:
 *      Enable/Disable specified port feature.
 * Parameters:
 *      unit - Device number
 *      lport - Logical Port number
 *      type - Enum value of the feature
 *      value - value to be set
 * Return Value:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
int
pcm_port_control_set(int unit, int lport, int type, int value)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_control_set) {
        rv = pcm_port_ctrl[lport].f->port_control_set(unit, lport, type,value);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_speed_set
 * Purpose:
 *      Setting the speed for a given port
 * Parameters:
 *      unit - Switch Unit #.
 *      lport - Logical Port number
 *      speed - Value in megabits/sec (10, 100, etc)
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 * Notes:
 *      Turns off autonegotiation.  Caller must make sure other forced
 *      parameters (such as duplex) are set.
 */
int
pcm_port_speed_set(int unit, int lport, int speed)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_speed_set) {
        rv = pcm_port_ctrl[lport].f->port_speed_set(unit, lport, speed);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_speed_get
 * Purpose:
 *      Getting the speed for a given port
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      *speed - Value in megabits/sec (10, 100, etc)
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 * Notes:
 *      Turns off autonegotiation.  Caller must make sure other forced
 *      parameters (such as duplex) are set.
 */
int
pcm_port_speed_get(int unit, int lport, int *speed)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_speed_get) {
        rv = pcm_port_ctrl[lport].f->port_speed_get(unit, lport, speed);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_link_status_get
 * Purpose:
 *      Retrieve current Link up/down status queries the PHY.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      *link - (OUT) Boolean value, FALSE for link down and TRUE for link up
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
int
pcm_port_link_status_get(int unit, int lport, int *link)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_link_status_get) {
        rv = pcm_port_ctrl[lport].f->port_link_status_get(unit, lport, link);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_autoneg_get
 * Purpose:
 *      Get the auto negotiation state of the port
 * Parameters:
 *      unit - Switch Unit number.
 *      lport - Logical Port number
 *      autoneg - (OUT)
 *                      Boolean value, FALSE for autoneg disabled
 *                      and TRUE for autoneg enabled
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
int
pcm_port_autoneg_get(int unit, int lport, int *autoneg)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_autoneg_get) {
        rv = pcm_port_ctrl[lport].f->port_autoneg_get(unit, lport, autoneg);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_autoneg_set
 * Purpose:
 *      Set the auto negotiation state for the given port
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      autoneg - Boolean value
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
int
pcm_port_autoneg_set(int unit, int lport, int autoneg)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_autoneg_set) {
        rv = pcm_port_ctrl[lport].f->port_autoneg_set(unit, lport, autoneg);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_pause_set
 * Purpose:
 *      Set the pause state for a given port
 * Parameters:
 *      unit - Switch Unit number.
 *      lport - Logical Port number
 *      pause_tx - Boolean value, or -1 (don't change)
 *      pause_rx - Boolean value, or -1 (don't change)
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 * Notes:
 *      Symmetric pause requires the two "pause" values to be the same.
 */
int
pcm_port_pause_set(int unit, int lport, int pause_tx, int pause_rx)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_pause_set) {
        rv = pcm_port_ctrl[lport].f->port_pause_set(unit, lport,
                                                    pause_tx, pause_rx);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_pause_get
 * Purpose:
 *      Get the pause state for a given port.
 * Parameters:
 *      unit - Switch Unit number.
 *      lport - Logical Port number
 *      pause_tx - (OUT) Boolean value
 *      pause_rx - (OUT) Boolean value
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
int
pcm_port_pause_get(int unit, int lport, int *pause_tx, int *pause_rx)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_pause_get) {
        rv = pcm_port_ctrl[lport].f->port_pause_get(unit, lport,
                                                    pause_tx, pause_rx);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_duplex_set
 * Purpose:
 *      Set the port duplex settings.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      duplex - Duplex setting, 0 means half-duplex
 *                   1 means full-duplex
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 * Notes:
 *      Turns off autonegotiation.  Caller must make sure other forced
 *      parameters (such as speed) are set.
 */
int
pcm_port_duplex_set(int unit, int lport, int duplex)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_duplex_set) {
        rv = pcm_port_ctrl[lport].f->port_duplex_set(unit, lport, duplex);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_duplex_get
 * Purpose:
 *      Get the port duplex settings
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      duplex - (OUT) Duplex setting, one of SOC_PORT_DUPLEX_xxx
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
int pcm_port_duplex_get(int unit, int lport, int *duplex)
{

    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_duplex_get) {
        rv = pcm_port_ctrl[lport].f->port_duplex_get(unit, lport, duplex);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_enable_set
 * Purpose:
 *      Physically enable/disable the MAC/PHY on this port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      lport - Logical Port number
 *      enable - TRUE, port is enabled, FALSE port is disabled.
 * Returns:
 *      SYS_ERR_XXX
 * Notes:
 *      If linkscan is running, it also controls the MAC enable state.
 */
int
pcm_port_enable_set(int unit, int lport, int enable)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_enable_set) {
        rv = pcm_port_ctrl[lport].f->port_enable_set(unit, lport, enable);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_enable_get
 * Purpose:
 *      Gets the enable state as defined by pcm_port_enable_set()
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      lport - Logical Port number
 *      enable - (OUT) TRUE, port is enabled, FALSE port is disabled.
 * Returns:
 *      SYS_ERR_XXX
 * Notes:
 *      The PHY enable holds the port enable state set by the user.
 *      The MAC enable transitions up and down automatically via linkscan
 *      even if user port enable is always up.
 */
int
pcm_port_enable_get(int unit, int lport, int *enable)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_enable_get) {
        rv = pcm_port_ctrl[lport].f->port_enable_get(unit, lport, enable);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_ability_advert_set
 * Purpose:
 *      Set the local port advertisement for autonegotiation.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      ability_mask - Local advertisement.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      This call MAY NOT restart autonegotiation (depending on the phy).
 *      To do that, follow this call with pcm_port_autoneg_set(TRUE).
 */
int
pcm_port_ability_advert_set(int unit, int lport, pcm_port_ability_t *adv)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_ability_advert_set) {
        rv = pcm_port_ctrl[lport].f->port_ability_advert_set(unit, lport, adv);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_ability_get
 * Purpose:
 *      Retrieve the port abilities.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      ability_mask - (OUT) Mask of BCM_PORT_ABIL_ values indicating the
 *              ability of the MAC/PHY.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
int
pcm_port_ability_get(int unit, int lport, pcm_port_ability_t *ability)
{
   int rv = SYS_ERR_NOT_IMPLEMENTED;

   if (pcm_port_ctrl[lport].f &&
       pcm_port_ctrl[lport].f->port_ability_local_get) {
       rv = pcm_port_ctrl[lport].f->port_ability_local_get(unit, lport,
                                                           ability);
   }

   return rv;
}

/*
 * Function:
 *      pcm_port_ability_remote_get
 * Purpose:
 *      Retrieve the local port advertisement for autonegotiation.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      ability_mask - (OUT) Remote advertisement.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
int
pcm_port_ability_remote_get(int unit, int lport, pcm_port_ability_t *ability)
{
   int rv = SYS_ERR_NOT_IMPLEMENTED;

   if (pcm_port_ctrl[lport].f &&
       pcm_port_ctrl[lport].f->port_ability_remote_get) {
       rv = pcm_port_ctrl[lport].f->port_ability_remote_get(unit, lport,
                                                            ability);
   }

   return rv;
}

/*
 * Function:
 *      pcm_port_cable_diag
 * Description:
 *      Run Cable Diagnostics on port
 * Parameters:
 *      unit - Device number
 *      lport - Logical Port number
 *      status - (OUT) cable diag status structure
 * Return Value:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      Cable diagnostics are only supported by some phy types
 *      (currently 5248 10/100 phy and 546x 10/100/1000 phys)
 */
int
pcm_port_cable_diag(int unit, int lport, pcm_port_cable_diag_t *status)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_cable_diag) {
        rv = pcm_port_ctrl[lport].f->phy_cable_diag(unit, lport, status);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_cable_diag_support
 * Description:
 *      Check Cable Diagnostics Support on port
 * Parameters:
 *      unit - Device number
 *      lport - Logical Port number
 *      support - (OUT) capbility of cable diag
 * Return Value:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      Cable diagnostics are only supported by some phy types
 *      (currently 5248 10/100 phy and 546x 10/100/1000 phys)
 */
int
pcm_port_cable_diag_support(int unit, int lport, int *support)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_cable_diag_support) {
        rv = pcm_port_ctrl[lport].f->phy_cable_diag_support(unit, lport,
                                                            support);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_eee_enable_set
 * Description:
 *      Enable EEE on port
 * Parameters:
 *      unit - Device number
 *      lport - Logical Port number
 *      enable - enable/disable EEE
 *      mode - (OUT) EEE mode used by system
 * Return Value:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      Cable diagnostics are only supported by some phy types
 *      (currently 5248 10/100 phy and 546x 10/100/1000 phys)
 */
int
pcm_port_eee_enable_set(int unit, int lport, int enable, int *mode)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_eee_enable_set) {
        rv = pcm_port_ctrl[lport].f->port_eee_enable_set(unit, lport, enable,
                                                         mode);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_update
 * Purpose:
 *      Get port characteristics from PHY and program MAC to match.
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      link - TRUE - process as link up.
 *             FALSE - process as link down.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
int
pcm_port_update(int unit, int lport, int link)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_update) {
        rv = pcm_port_ctrl[lport].f->port_update(unit, lport, link);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_interface_set
 * Purpose:
 *      Setting the interface type for a given port
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      if - _SHR_PORT_IF_*
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      WARNING: assumes _SHR_PORT_IF_* matches SOC_PORT_IF_*
 */
int
pcm_port_interface_set(int unit, int lport,  pcm_port_if_t intf)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_interface_set) {
        rv = pcm_port_ctrl[lport].f->port_interface_set(unit, lport, intf);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_interface_get
 * Purpose:
 *      Getting the interface type of a port
 * Parameters:
 *      unit - StrataSwitch Unit #.
 *      lport - Logical Port number
 *      intf - (OUT) _SHR_PORT_IF_*
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      WARNING: assumes BCM_PORT_IF_* matches SOC_PORT_IF_*
 */
int
pcm_port_interface_get(int unit, int lport,  pcm_port_if_t *intf)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_interface_get) {
        rv = pcm_port_ctrl[lport].f->port_interface_get(unit, lport, intf);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_reg_get
 * Description:
 *      General PHY register read
 * Parameters:
 *      unit - Device number
 *      lport - Logical Port number
 *      flags - Logical OR of one or more of the following flags:
 *              _SHR_PORT_PHY_INTERNAL
 *                      Address internal SERDES PHY for port
 *      phy_addr - PHY internal register address
 *      phy_data - (OUT) Data that was read
 * Returns:
 *      SYS_ERR_XXX
 */
int
pcm_phy_reg_get(int unit, int lport, uint32 index,
                uint32 phy_reg_addr, uint32* phy_data)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_reg_get) {
        rv = pcm_port_ctrl[lport].f->phy_reg_get(unit, lport, index,
                                                 phy_reg_addr, phy_data);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_reg_set
 * Description:
 *      General PHY register write
 * Parameters:
 *      unit - Device number
 *      lport - Logical Port number
 *      flags - Logical OR of one or more of the following flags:
 *              _SHR_PORT_PHY_INTERNAL
 *      phy_data - Data for write
 * Returns:
 *      SYS_ERR_XXX
 */
int
pcm_phy_reg_set(int unit, int lport, uint32 flags,
                uint32 phy_reg_addr, uint32 phy_data)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_reg_set) {
        rv = pcm_port_ctrl[lport].f->phy_reg_set(unit, lport, flags,
                                                 phy_reg_addr, phy_data);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_notify
 * Purpose:
 *      Notify events to internal PHY driver.
 * Parameters:
 *      unit - SOC Unit #.
 *      lport - Logical Port number
 *      event - notify event
 *      value - associate value
 * Returns:
 *      SYS_ERR_XXX
 */
int
pcm_port_notify(int unit, int lport, int event, int value)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_notify) {
        rv = pcm_port_ctrl[lport].f->port_notify(unit, lport, event, value);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_driver_name_get
 * Description:
 *      Get PHY driver name
 * Parameters:
 *      unit - Device number
 *      lport - Logical Port number
 *      flags - Logical OR of one or more of the following flags:
 *              _SHR_PORT_PHY_INTERNAL
 *                      Address internal SERDES PHY for port
 *              _SHR_PORT_PHY_NOMAP
 *                      Instead of mapping port to PHY MDIO address,
 *                      treat port parameter as actual PHY MDIO address.
 * Returns:
 *      char * driver_name or NULL
 */
const char *
pcm_phy_driver_name_get(int unit, int lport, uint32 index)
{
    const char * ret = NULL;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_driver_name_get) {
        ret = pcm_port_ctrl[lport].f->phy_driver_name_get(unit, lport, index);
    }

    return ret;
}

/*
 * Function:
 *      pcm_phy_addr_get
 * Description:
 *      Get PHY driver address
 * Parameters:
 *      unit - Device number
 *      lport - Logical Port number
 *      iaddr - Internal Serdes/PHY address
 *      addr -  External PHY address
 * Returns:
 *      SYS_OK
 */
int
pcm_phy_addr_get(int unit, int lport, uint32 *iaddr, uint32 *addr)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_addr_get) {
        rv = pcm_port_ctrl[lport].f->phy_addr_get(unit, lport, iaddr, addr);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_pause_addr_get
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
pcm_port_pause_addr_get(int unit, int lport, sal_mac_addr_t mac)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_pause_addr_get) {
        rv = pcm_port_ctrl[lport].f->port_pause_addr_get(unit, lport, mac);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_pause_addr_set
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
pcm_port_pause_addr_set(int unit, int lport, sal_mac_addr_t mac)
{
    int rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_pause_addr_set) {
        rv = pcm_port_ctrl[lport].f->port_pause_addr_set(unit, lport, mac);
    }

    return rv;
}
