/*! \file cmicx_led_internal.h
 *
 *  CMICx LED host base driver.
 *
 *    This driver is a generic part of CMICx-LED host driver.
 * This driver will
 *    - Download Broadcom CMICx-LED FW when LED driver initialization.
 *    - Download Customer CMICx-LED FW as API/CLI request.
 *    - Enable/Disable LED FW by mailbox driver.
 *    - Read/write LED FW's port data(control_data) by mailbox driver.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef CMICX_LED_INTERNAL_H
#define CMICX_LED_INTERNAL_H

#include <led_internal.h>
#include <m0ssq_internal.h>

/* Max number of LED uC. */
#define CMICX_MAX_LEDUPS           (1)

/* M0 core to run LED firmware. */
#define LED_UC                     (0)

/* Address where customer LED FW is downloaded. */
#define LED_FW_OFFSET              (0x3800)

/* Disable CMICx software link overwrite feature. */
#define LED_SW_LINK_DISABLE        (1)

/* Maximum size of customer led firwmare. */
#define CMICX_CUSTOMER_LED_FW_SIZE_MAX  (2048 - 4)

/* Size of led control data memory. */
#define CMICX_LED_CONTROL_DATA_SIZE (1024)

/*! Message id for CMICx LED mailbox. */
typedef enum led_msg_id_e {

    /*! (Obsolete) get port status. */
    LED_MSG_LNK_STS = 0,

    /*! (Obsolete) set software link pbmp. */
    LED_MSG_STATUS,

    /*! Enable LED FW. */
    LED_MSG_ENABLE,

    /*! (Obsolete) set port status. */
    LED_MSG_STATUS_SET,

    /*! (Obsolete) set port speed. */
    LED_MSG_SPEED,

    /*! LED FW led control data write. */
    LED_MSG_CONTROL_DATA_WRITE,

    /*! LED FW led control data read. */
    LED_MSG_CONTROL_DATA_READ,

} led_msg_id_t;

/*! Definition struct for LED mbox */
typedef struct led_control_data_s {

    /*! Offset of led control data. */
    uint32 offset;

    /*! Data of led control data (only valid in bit0~bit7). */
    uint32 data;

} led_control_data_t;

/*! CMICX-LED device-specific data. */
typedef struct cmicx_led_dev_s {

    /*! Flag indicates led enable/disable. */
    bool enable;

    /*! led mailbox id. */
    uint32 mbox_id;

    /*! Shared memory object for LED communication. */
    m0ssq_mem_t *led_shmem;

} cmicx_led_dev_t;

/*!
 * \brief Get CMICx-LED host base driver.
 *
 * CMICx-LED base driver provides basic methods to control CMICx-LED FW.
 *
 * \param [in] unit Unit number.
 *
 * \retval Pointer to the base driver.
 */
extern const led_drv_t *
cmicx_led_base_drv_get(int unit);

#endif /* CMICX_LED_INTERNAL_H */
