/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * Generic PHY driver.
 */

#include <phy/phy.h>
#include <phy/ge_phy.h>

/*
 * Function:     
 *      ge_phy_autoneg_ew (Autoneg-ed mode with E@W on).
 * Purpose:    
 *      Determine autoneg-ed mode between 
 *      two ends of a link.
 * Parameters:
 *      pc - PHY control structure
 *      speed - (OUT) greatest common speed
 *      duplex - (OUT) greatest common duplex
 * Returns:    
 *      CDK_E_xxx.
 */
int
ge_phy_autoneg_ew(phy_ctrl_t *pc, uint32_t *speed, int *duplex)
{
    int ioerr = 0;
    int t_speed, t_duplex;
    uint32_t mii_assr;

    PHY_CTRL_CHECK(pc);

    /* Read status from Auxiliary Status Register */
    ioerr += PHY_BUS_READ(pc, 0x19, &mii_assr);

    switch ((mii_assr >> 8) & 0x7) {
    case 0x7:
        t_speed = 1000;
        t_duplex = TRUE;
        break;
    case 0x6:
        t_speed = 1000;
        t_duplex = FALSE;
        break;
    case 0x5:
        t_speed = 100;
        t_duplex = TRUE;
        break;
    case 0x3:
        t_speed = 100;
        t_duplex = FALSE;
        break;
    case 0x2:
        t_speed = 10;
        t_duplex = TRUE;
        break;
    case 0x1:
        t_speed = 10;
        t_duplex = FALSE;
        break;
    default:
        t_speed = 0; /* 0x4 is 100BASE-T4 which is not supported */
        t_duplex = FALSE;
        break;
    }

    if (speed)  *speed  = t_speed;
    if (duplex)    *duplex = t_duplex;

    return ioerr ? CDK_E_IO : CDK_E_NONE;
}
