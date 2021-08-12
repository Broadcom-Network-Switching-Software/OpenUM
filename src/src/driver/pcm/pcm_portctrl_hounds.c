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
#include <soc/port.h>


/*
 * Function:
 *      _portctrl_port_ifg_set
 * Purpose:
 *      Main part of pcm_portctrl_port_ifg_set.
 */
static int
_portctrl_port_ifg_set(int unit, int lport, int ifg)
{
    int speed, duplex;

    SOC_IF_ERROR_RETURN(
        MAC_DUPLEX_GET(PORT(unit, lport).p_mac, unit, lport, &duplex));
    SOC_IF_ERROR_RETURN(
        MAC_SPEED_GET(PORT(unit, lport).p_mac, unit, lport, &speed));

    return MAC_IFG_SET(PORT(unit, lport).p_mac, unit, lport, speed, duplex, ifg);
}

/*
 * Function:
 *      _portctrl_port_ifg_get
 * Purpose:
 *      Main part of pcm_portctrl_port_ifg_get.
 */
static int
_portctrl_port_ifg_get(int unit, int lport, int *ifg)
{
    int speed, duplex;

    SOC_IF_ERROR_RETURN(
        MAC_DUPLEX_GET(PORT(unit, lport).p_mac, unit, lport, &duplex));
    SOC_IF_ERROR_RETURN(
        MAC_SPEED_GET(PORT(unit, lport).p_mac, unit, lport, &speed));

    return MAC_IFG_GET(PORT(unit, lport).p_mac, unit, lport, speed, duplex, ifg);
}

/*
 * Function:
 *      pcm_portctrl_port_ifg_set
 * Description:
 *      Sets the new ifg (Inter-frame gap) value
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      ifg    - Inter-frame gap in bit-times
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      The function makes sure the IFG value makes sense and updates the
 *      IPG register in case the speed/duplex match the current settings
 */
sys_error_t
pcm_portctrl_port_ifg_set(int unit, int lport, int ifg)
{
    int rv;

    rv = _portctrl_port_ifg_set(unit, lport, ifg);
    return (rv) ? SYS_ERR : SYS_OK;
}

/*
 * Function:
 *      pcm_portctrl_port_ifg_get
 * Description:
 *      Gets the new ifg (Inter-frame gap) value
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      ifg    - Inter-frame gap in bit-times
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 */
sys_error_t
pcm_portctrl_port_ifg_get(int unit, int lport, int *ifg)
{
    int rv;

    rv = _portctrl_port_ifg_get(unit, lport, ifg);
    return (rv) ? SYS_ERR : SYS_OK;
}

/*
 * Function:
 *      pcm_portctrl_port_pause_set
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
sys_error_t
pcm_portctrl_port_pause_set(int unit, int lport, int pause_tx, int pause_rx)
{
    return MAC_PAUSE_SET(PORT(unit, lport).p_mac, unit, lport,
                         pause_tx, pause_rx);
}

/*
 * Function:
 *      pcm_portctrl_port_pause_get
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

sys_error_t
pcm_portctrl_port_pause_get(int unit, int lport, int *pause_tx, int *pause_rx)
{
    return MAC_PAUSE_GET(PORT(unit, lport).p_mac, unit, lport,
                         pause_tx, pause_rx);
}

