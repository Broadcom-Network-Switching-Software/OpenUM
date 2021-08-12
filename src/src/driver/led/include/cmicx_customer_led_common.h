/*! \file cmicx_customer_led_common.h
 *
 * The shared information in between customer CMICx LED firmware
 * and UM firmware.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef CMICX_CUSTOMER_LED_COMMON_H
#define CMICX_CUSTOMER_LED_COMMON_H

/*! Customer defined software flag. */
#define LED_SW_LINK_UP   0x1

/*!
 * LED color of Broadcom LED box.
 * Customer should define their own LED state.
 */
#define LED_OFF                 (0)
#define LED_ORANGE              (1)
#define LED_GREEN               (2)

/*!
 * Offset of LED option within led_control_data.
 * Option 1: 1st LED is link status, 2nd LED is trafic.
 * Option 2: 2nd LED is link status, ist LED is trafic.
 * Option 3: Use customer LED firmware in vendor config.
 */
#define LED_OPTION              (1020)

#endif /* CMICX_CUSTOMER_LED_COMMON_H */
