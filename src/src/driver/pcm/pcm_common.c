/*! \file pcm_common.c
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
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
 *      bcm_port_ctrl
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
 *      unit - Unit number.
 * Returns:
 *      SOC_E_NONE - success (or already initialized)
 *      SOC_E_INTERNAL- failed to write PTABLE entries
 *      SOC_E_MEMORY - failed to allocate required memory.
 * Notes:
 *      By default ports come up enabled. They can be made to come up disabled
 *      at startup by a compile-time application policy flag in your Make.local
 *      PTABLE initialized.
 */
sys_error_t
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
 *      pcm_phy_reg_sbus_get
 * Description:
 *      Read internal PHY register via SBUS.
 * Parameters:
 *      unit - Unit number
 *      phy_addr - PHY address
 *      phy_reg - PHY register
 *      phy_data - (OUT) Data that was read
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_reg_sbus_get(int unit, uint32 phy_addr,
                     uint32 phy_reg, uint32 *phy_data)
{
    if (!pcm_phyctrl.phy_reg_sbus_get) {
        return SYS_ERR_NOT_IMPLEMENTED;
    }

    return pcm_phyctrl.phy_reg_sbus_get(unit, phy_addr, phy_reg, phy_data);
}

/*
 * Function:
 *      pcm_phy_reg_sbus_set
 * Description:
 *      Write internal PHY register via SBUS.
 * Parameters:
 *      unit - Unit number
 *      phy_addr - PHY address
 *      phy_reg - PHY register
 *      phy_data - Data to write
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_reg_sbus_set(int unit, uint32 phy_addr,
                     uint32 phy_reg, uint32 phy_data)
{
    if (!pcm_phyctrl.phy_reg_sbus_set) {
        return SYS_ERR_NOT_IMPLEMENTED;
    }

    return pcm_phyctrl.phy_reg_sbus_set(unit, phy_addr, phy_reg, phy_data);
}

/*
 * Function:
 *      pcm_port_probe_init
 * Purpose:
 *      Probe the PHYs and initialize the PHYs and MACs for the specified ports.
 *      This is purely a discovery routine and does no configuration.
 * Parameters:
 *      unit - Unit number.
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

sys_error_t
pcm_port_probe_init(int unit, pbmp_t lpbmp, pbmp_t *okay_lpbmp)
{
    return pcm_phyctrl.port_probe_init(unit, lpbmp, okay_lpbmp);
}


/*
 * Function:
 *      pcm_port_reinit
 * Purpose:
 *      Reinit a port.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 * Returns:
 *      SOC_OK
 *      SOC_ERR_XXX.
* Notes:
 *      If a port to be detached does not appear in detached, its
 *      state is not defined.
 */

sys_error_t
pcm_port_reinit(int unit, int lport)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_reinit) {
        rv = pcm_port_ctrl[lport].f->port_reinit(unit, lport);
    }
    return rv;
}


/*
 * Function:
 *      pcm_port_loopback_set
 * Purpose:
 *      Setting the speed for a given port
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      loopback - one of:
 *              PCM_PORT_LOOPBACK_NONE
 *              PCM_PORT_LOOPBACK_MAC
 *              PCM_PORT_LOOPBACK_PHY
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_loopback_set(int unit, int lport, int loopback)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      loopback - (OUT)  one of:
 *              PCM_PORT_LOOPBACK_NONE
 *              PCM_PORT_LOOPBACK_MAC
 *              PCM_PORT_LOOPBACK_PHY
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_loopback_get(int unit, int lport, int *loopback)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number
 *      lport - Logical port number
 *      type - Enum  value of the feature
 *      value - (OUT) Current value of the port feature
 * Return Value:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_control_get(int unit, int lport, int type, int *value)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number
 *      lport - Logical port number
 *      type - Enum value of the feature
 *      value - value to be set
 * Return Value:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_control_set(int unit, int lport, int type, int value)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      speed - Value in megabits/sec (10, 100, etc).
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 * Notes:
 *      Turns off autonegotiation.  Caller must make sure other forced
 *      parameters (such as duplex) are set.
 */
sys_error_t
pcm_port_speed_set(int unit, int lport, int speed)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      speed - Value in megabits/sec (10, 100, etc).
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 * Notes:
 *      Turns off autonegotiation.  Caller must make sure other forced
 *      parameters (such as duplex) are set.
 */
sys_error_t
pcm_port_speed_get(int unit, int lport, int *speed)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      link - (OUT) Boolean value, FALSE for link down and TRUE for link up.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_link_status_get(int unit, int lport, int *link)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      autoneg - (OUT)
 *                      Boolean value, FALSE for autoneg disabled
 *                      and TRUE for autoneg enabled
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_autoneg_get(int unit, int lport, int *autoneg)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      autoneg - Boolean value.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_autoneg_set(int unit, int lport, int autoneg)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      pause_tx - Boolean value, or -1 (don't change).
 *      pause_rx - Boolean value, or -1 (don't change).
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 * Notes:
 *      Symmetric pause requires the two "pause" values to be the same.
 */
sys_error_t
pcm_port_pause_set(int unit, int lport, int pause_tx, int pause_rx)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      pause_tx - (OUT) Boolean value.
 *      pause_rx - (OUT) Boolean value.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_pause_get(int unit, int lport, int *pause_tx, int *pause_rx)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_pause_get) {
        rv = pcm_port_ctrl[lport].f->port_pause_get(unit, lport,
                                                    pause_tx, pause_rx);
    }

    return rv;
}


/*
 * Function:
 *      pcm_port_class_set
 * Purpose:
 *      Set the ports class id for a given port.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      pclass - Classification type.
 *      pclass_id - New class ID of the port.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_class_set(int unit, int lport, port_class_t pclass, uint32 pclass_id)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_class_set) {
        rv = pcm_port_ctrl[lport].f->port_class_set(unit, lport,
                                                    pclass, pclass_id);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_class_get
 * Purpose:
 *      Get the ports class id for a given port.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      pclass - Classification type.
 *      pclass_id - (OUT) New class ID of the port.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_class_get(int unit, int lport, port_class_t pclass, uint32 *pclass_id)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_class_get) {
        rv = pcm_port_ctrl[lport].f->port_class_get(unit, lport,
                                                    pclass, pclass_id);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_frame_max_set
 * Purpose:
 *      Set the maximum receive frame size
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      size - Maximum frame size in bytes.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_frame_max_set(int unit, int lport, int size)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_frame_max_set) {
        rv = pcm_port_ctrl[lport].f->port_frame_max_set(unit, lport, size);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_frame_max_get
 * Purpose:
 *      Get the maximum receive frame size
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      size - (OUT) Maximum frame size in bytes.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_frame_max_get(int unit, int lport, int *size)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_frame_max_get) {
        rv = pcm_port_ctrl[lport].f->port_frame_max_get(unit, lport, size);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_ifg_set
 * Purpose:
 *      Set the new ifg (Inter-frame gap) value for a given port
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      size - Inter-frame gap in bit-times.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_ifg_set(int unit, int lport, int ifg)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_ifg_set) {
        rv = pcm_port_ctrl[lport].f->port_ifg_set(unit, lport, ifg);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_frame_max_get
 * Purpose:
 *      Get the new ifg (Inter-frame gap) value for a given port
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      size - (OUT) Inter-frame gap in bit-times.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 */
sys_error_t
pcm_port_ifg_get(int unit, int lport, int *ifg)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_ifg_get) {
        rv = pcm_port_ctrl[lport].f->port_ifg_get(unit, lport, ifg);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_duplex_set
 * Purpose:
 *      Set the port duplex settings.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      duplex - Duplex setting, 0 means half-duplex
 *                   1 means full-duplex
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX - Functionality not available
 * Notes:
 *      Turns off autonegotiation.  Caller must make sure other forced
 *      parameters (such as speed) are set.
 */
sys_error_t
pcm_port_duplex_set(int unit, int lport, int duplex)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      duplex - (OUT) Duplex setting, one of SOC_PORT_DUPLEX_xxx.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_duplex_get(int unit, int lport, int *duplex)
{

    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      enable - TRUE, port is enabled, FALSE port is disabled.
 * Returns:
 *      SYS_ERR_XXX
 * Notes:
 *      If linkscan is running, it also controls the MAC enable state.
 */
sys_error_t
pcm_port_enable_set(int unit, int lport, int enable)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      enable - (OUT) TRUE, port is enabled, FALSE port is disabled.
 * Returns:
 *      SYS_ERR_XXX
 * Notes:
 *      The PHY enable holds the port enable state set by the user.
 *      The MAC enable transitions up and down automatically via linkscan
 *      even if user port enable is always up.
 */
sys_error_t
pcm_port_enable_get(int unit, int lport, int *enable)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      ability_mask - Local advertisement.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      This call MAY NOT restart autonegotiation (depending on the phy).
 *      To do that, follow this call with pcm_port_autoneg_set(TRUE).
 */
sys_error_t
pcm_port_ability_advert_set(int unit, int lport, pcm_port_ability_t *adv)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      ability_mask - (OUT) Mask of BCM_PORT_ABIL_ values indicating the
 *              ability of the MAC/PHY.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_ability_get(int unit, int lport, pcm_port_ability_t *ability)
{
   sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      ability_mask - (OUT) Remote advertisement.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_ability_remote_get(int unit, int lport, pcm_port_ability_t *ability)
{
   sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number
 *      lport - Logical port number
 *      status - (OUT) cable diag status structure
 * Return Value:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      Cable diagnostics are only supported by some phy types
 *      (currently 5248 10/100 phy and 546x 10/100/1000 phys)
 */
sys_error_t
pcm_port_cable_diag(int unit, int lport, pcm_port_cable_diag_t *status)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number
 *      lport - Logical port number
 *      support - (OUT) capbility of cable diag
 * Return Value:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      Cable diagnostics are only supported by some phy types
 *      (currently 5248 10/100 phy and 546x 10/100/1000 phys)
 */
sys_error_t
pcm_port_cable_diag_support(int unit, int lport, int *support)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number
 *      lport - Logical port number
 *      enable - enable/disable EEE
 *      mode - (OUT) EEE mode used by system
 * Return Value:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      Cable diagnostics are only supported by some phy types
 *      (currently 5248 10/100 phy and 546x 10/100/1000 phys)
 */
sys_error_t
pcm_port_eee_enable_set(int unit, int lport, int enable, int *mode)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      link - TRUE - process as link up.
 *             FALSE - process as link down.
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_update(int unit, int lport, int link)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      if - _SHR_PORT_IF_*
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      WARNING: assumes _SHR_PORT_IF_* matches SOC_PORT_IF_*
 */
sys_error_t
pcm_port_interface_set(int unit, int lport, int intf)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      intf - (OUT) _SHR_PORT_IF_*
 * Returns:
 *      SYS_OK
 *      SYS_ERR_XXX
 * Notes:
 *      WARNING: assumes BCM_PORT_IF_* matches SOC_PORT_IF_*
 */
sys_error_t
pcm_port_interface_get(int unit, int lport, int *intf)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number
 *      lport - Logical port number
 *      flags - Logical OR of one or more of the following flags:
 *              _SHR_PORT_PHY_INTERNAL
 *                      Address internal SERDES PHY for port
 *      phy_addr - PHY internal register address
 *      phy_data - (OUT) Data that was read
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_reg_get(int unit, int lport, uint32 index,
                uint32 phy_reg_addr, uint32* phy_data)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number
 *      lport - Logical port number
 *      flags - Logical OR of one or more of the following flags:
 *              _SHR_PORT_PHY_INTERNAL
 *      phy_data - Data for write
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_reg_set(int unit, int lport, uint32 flags,
                uint32 phy_reg_addr, uint32 phy_data)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      event - notify event.
 *      value - associate value.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_notify(int unit, int lport, int event, int value)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number
 *      lport - Logical port number
 *      sel - Internal (1) or external (0) PHY driver.
 * Returns:
 *      char * driver_name or NULL
 */
const char *
pcm_phy_driver_name_get(int unit, int lport, uint32 sel)
{
    const char *ret = NULL;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_driver_name_get) {
        ret = pcm_port_ctrl[lport].f->phy_driver_name_get(unit, lport, sel);
    }

    return ret;
}

/*
 * Function:
 *      pcm_phy_addr_get
 * Description:
 *      Get PHY driver address
 * Parameters:
 *      unit - Unit number
 *      lport - Logical port number
 *      iaddr - Internal Serdes/PHY address
 *      addr -  External PHY address
 * Returns:
 *      SYS_OK
 */
sys_error_t
pcm_phy_addr_get(int unit, int lport, uint32 *iaddr, uint32 *addr)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      mac - (OUT) MAC address sent with pause frames.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_XXX
 * Notes:
 *      Symmetric pause requires the two "pause" values to be the same.
 */
sys_error_t
pcm_port_pause_addr_get(int unit, int lport, sal_mac_addr_t mac)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

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
 *      unit - Unit number.
 *      lport - Logical port number.
 *      mac - station address used for pause frames.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_XXX
 * Notes:
 *      Symmetric pause requires the two "pause" values to be the same.
 */
sys_error_t
pcm_port_pause_addr_set(int unit, int lport, sal_mac_addr_t mac)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_pause_addr_set) {
        rv = pcm_port_ctrl[lport].f->port_pause_addr_set(unit, lport, mac);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_synce_clock_get
 * Purpose:
 *      configure SyncE clock.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      mode0 - Stage0 mode.
 *      mode1 - Stage1 mode.
 *      sdm_value - SDM value of stage0.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_synce_clock_get(int unit, int lport, uint32 *mode0,
                        uint32 *mode1, uint32 *sdm_value)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_synce_clock_get) {
        rv = pcm_port_ctrl[lport].f->phy_synce_clock_get(unit, lport, mode0,
                                                         mode1, sdm_value);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_synce_clock_set
 * Purpose:
 *      configure SyncE clock.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      mode0 - Stage0 mode.
 *      mode1 - Stage1 mode.
 *      sdm_value - SDM value of stage0.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_synce_clock_set(int unit, int lport, uint32 mode0,
                        uint32 mode1, uint32 sdm_value)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_synce_clock_set) {
        rv = pcm_port_ctrl[lport].f->phy_synce_clock_set(unit, lport, mode0,
                                                         mode1, sdm_value);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_timesync_enable_get
 * Purpose:
 *      Get PHY timesync enable state.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      en - PHY timestamp is enabled (1) or disabled (0).
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_timesync_enable_get(int unit, int lport, int *en)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_timesync_enable_get) {
        rv = pcm_port_ctrl[lport].f->phy_timesync_enable_get(unit, lport, en);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_timesync_enable_set
 * Purpose:
 *      Set PHY timesync enable state.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      en - Enable (1) or disable (0) PHY timestamp.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_timesync_enable_set(int unit, int lport, int en)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_timesync_enable_set) {
        rv = pcm_port_ctrl[lport].f->phy_timesync_enable_set(unit, lport, en);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_timesync_ctrl_get
 * Purpose:
 *      Get PHY timesync control value for each type.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      type - Timesync control type.
 *      value - Timesync control value.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_timesync_ctrl_get(int unit, int lport,
                          pcm_phy_timesync_ctrl_t type, uint64 *value)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_timesync_ctrl_get) {
        rv = pcm_port_ctrl[lport].f->phy_timesync_ctrl_get(unit, lport, type,
                                                           value);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_timesync_ctrl_set
 * Purpose:
 *      Set PHY timesync control value for each type.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      type - Timesync control type.
 *      value - Timesync control value.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_timesync_ctrl_set(int unit, int lport,
                          pcm_phy_timesync_ctrl_t type, uint64 value)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_timesync_ctrl_set) {
        rv = pcm_port_ctrl[lport].f->phy_timesync_ctrl_set(unit, lport, type,
                                                           value);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_timesync_tx_info_get
 * Purpose:
 *      Get port timesync Tx information.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      info - Timesync tx information.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_timesync_tx_info_get(int unit, int lport,
                              pcm_port_timesync_tx_info_t *info)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_timesync_tx_info_get) {
        rv = pcm_port_ctrl[lport].f->port_timesync_tx_info_get(unit, lport,
                                                               info);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_fault_status_get
 * Purpose:
 *      Get port fault status.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      st - Fault status.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_fault_status_get(int unit, int lport, board_port_fault_st_t *st)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_fault_status_get) {
        rv = pcm_port_ctrl[lport].f->port_fault_status_get(unit, lport, st);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_fault_ctrl_get
 * Purpose:
 *      Get port fault configuration.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      ctrl - Fault configuration.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_fault_ctrl_get(int unit, int lport, board_port_fault_ctrl_t *ctrl)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_fault_ctrl_get) {
        rv = pcm_port_ctrl[lport].f->port_fault_ctrl_get(unit, lport, ctrl);
    }

    return rv;
}

/*
 * Function:
 *      pcm_port_fault_ctrl_set
 * Purpose:
 *      Set port fault configuration.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      ctrl - Fault configuration.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_port_fault_ctrl_set(int unit, int lport, board_port_fault_ctrl_t *ctrl)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->port_fault_ctrl_set) {
        rv = pcm_port_ctrl[lport].f->port_fault_ctrl_set(unit, lport, ctrl);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_fec_mode_set
 * Purpose:
 *      Set port FEC mode.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      mode - FEC mode.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_fec_mode_set(int unit, int lport, board_phy_fec_mode_t mode)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_fec_mode_set) {
        rv = pcm_port_ctrl[lport].f->phy_fec_mode_set(unit, lport, mode);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_fec_mode_get
 * Purpose:
 *      Get port FEC mode.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      mode - FEC mode.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_fec_mode_get(int unit, int lport, board_phy_fec_mode_t *mode)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_fec_mode_get) {
        rv = pcm_port_ctrl[lport].f->phy_fec_mode_get(unit, lport, mode);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_fec_status_get
 * Purpose:
 *      Get port FEC status.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      st - FEC status.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_fec_status_get(int unit, int lport, board_phy_fec_st_t *st)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_fec_status_get) {
        rv = pcm_port_ctrl[lport].f->phy_fec_status_get(unit, lport, st);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_power_set
 * Purpose:
 *      Set PHY power configuration.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      power - power PHY power state. 1 for power on, 0 for power down.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_power_set(int unit, int lport, int power)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_power_set) {
        rv = pcm_port_ctrl[lport].f->phy_power_set(unit, lport, power);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_power_get
 * Purpose:
 *      Get PHY power configuration.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      power - PHY power state. 1 for power on, 0 for power down.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_power_get(int unit, int lport, int *power)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_power_get) {
        rv = pcm_port_ctrl[lport].f->phy_power_get(unit, lport, power);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_tx_ctrl_set
 * Purpose:
 *      Set PHY Tx configuration.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      ctrl - PHY tx configuration.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_tx_ctrl_set(int unit, int lport, board_phy_tx_ctrl_t *ctrl)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_tx_ctrl_set) {
        rv = pcm_port_ctrl[lport].f->phy_tx_ctrl_set(unit, lport, ctrl);
    }

    return rv;
}

/*
 * Function:
 *      phy_tx_ctrl_get
 * Purpose:
 *      Get PHY Tx configuration.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      ctrl - PHY tx configuration.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_tx_ctrl_get(int unit, int lport, board_phy_tx_ctrl_t *ctrl)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_tx_ctrl_get) {
        rv = pcm_port_ctrl[lport].f->phy_tx_ctrl_get(unit, lport, ctrl);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_rx_status_get
 * Purpose:
 *      Get port Rx status.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      st - Rx status.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_rx_status_get(int unit, int lport, board_phy_rx_st_t *st)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_rx_status_get) {
        rv = pcm_port_ctrl[lport].f->phy_rx_status_get(unit, lport, st);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_prbs_ctrl_set
 * Purpose:
 *      Set port PRBS configuration.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      ctrl - PRBS configuration.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_prbs_ctrl_set(int unit, int lport, int flags,
                      board_phy_prbs_ctrl_t *ctrl)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_prbs_ctrl_set) {
        rv = pcm_port_ctrl[lport].f->phy_prbs_ctrl_set(unit, lport, flags,
                                                       ctrl);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_prbs_ctrl_get
 * Purpose:
 *      Get port PRBS configuration.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      ctrl - PRBS configuration.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_prbs_ctrl_get(int unit, int lport, int flags,
                      board_phy_prbs_ctrl_t *ctrl)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_prbs_ctrl_get) {
        rv = pcm_port_ctrl[lport].f->phy_prbs_ctrl_get(unit, lport, flags,
                                                       ctrl);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_prbs_enable_set
 * Purpose:
 *      Set port PRBS enable state.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      en - Enable/disable PRBS.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_prbs_enable_set(int unit, int lport, int flags, int en)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_prbs_enable_set) {
        rv = pcm_port_ctrl[lport].f->phy_prbs_enable_set(unit, lport, flags,
                                                         en);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_prbs_enable_get
 * Purpose:
 *      Get port PRBS enable state.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      en - PRBS is enabled or disabled.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_prbs_enable_get(int unit, int lport, int flags, int *en)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_prbs_enable_get) {
        rv = pcm_port_ctrl[lport].f->phy_prbs_enable_get(unit, lport, flags,
                                                         en);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_prbs_status_get
 * Purpose:
 *      Get port PRBS status.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      st - PRBS status.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_prbs_status_get(int unit, int lport, board_phy_prbs_st_t *st)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_prbs_status_get) {
        rv = pcm_port_ctrl[lport].f->phy_prbs_status_get(unit, lport, st);
    }

    return rv;
}

/*
 * Function:
 *      pcm_phy_stats_get
 * Purpose:
 *      Get port statistisc counts.
 * Parameters:
 *      unit - Unit number.
 *      lport - Logical port number.
 *      stats - PHY error statistics.
 * Returns:
 *      SYS_ERR_XXX
 */
sys_error_t
pcm_phy_stats_get(int unit, int lport, board_phy_stats_t *stats)
{
    sys_error_t rv = SYS_ERR_NOT_IMPLEMENTED;

    if (pcm_port_ctrl[lport].f &&
        pcm_port_ctrl[lport].f->phy_stats_get) {
        rv = pcm_port_ctrl[lport].f->phy_stats_get(unit, lport, stats);
    }

    return rv;
}


/*
 * Function:
 *      pcm_port_link_scan
 * Purpose:
 *      Execute the software link scan manually.
 * Parameters:
 *      param - parameters for software link scan
 * Returns:
 *      No return value
 */
void
pcm_port_link_scan(void *param)
{
    if (pcm_phyctrl.port_link_scan) {
        pcm_phyctrl.port_link_scan(param);
    }
}