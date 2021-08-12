/*! \file led.h
 *
 * LED board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef LED_H
#define LED_H

#include <types.h>
#include <error.h>

/*!
 * \brief Get number of LED processor.
 *
 * \param [out] led_uc_num Number of LED uC.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_led_uc_num_get(int *led_uc_num);

/*!
 * \brief Start/stop customer LED firmware.
 *
 * \param [in] led_uc LED uC number.
 * \param [in] start false for stop firmware, true for start firmware.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_led_fw_start_set(int led_uc, int start);

/*!
 * \brief Get customer firmware start status.
 *
 * \param [in] led_uc LED uC number.
 * \param [out] start Status of firmware start.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_led_fw_start_get(int led_uc, int *start);

/*!
 * \brief Write data to led control data.
 *
 * \param [in] led_uc LED uC number.
 * \param [in] offset Starting offset of write operation.
 * \param [in] data Write data buffer.
 * \param [in] len Length of write data.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_led_control_data_write(int led_uc, int offset,
                             const uint8 *data, int len);

/*!
 * \brief Read data from led control data.
 *
 * \param [in] led_uc LED uC number.
 * \param [in] offset Starting offset of read operation.
 * \param [out] data Read data buffer.
 * \param [in] len Length of read data.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_led_control_data_read(int led_uc, int offset,
                            uint8 *data, int len);

/*!
 * \brief Load LED firmware binary.
 *
 * \param [in] led_uc LED microcontroller number.
 * \param [in] data Firmware binary content.
 * \param [in] len Length of firmware binary.
 *
 * \retval SYS_OK on operation successful.
 *         SYS_ERR_PARAMETER on invalid input parameter value.
 *         SYS_ERR_UNAVAIL on feature Not supported.
 */
extern sys_error_t
board_led_fw_load(int led_uc, const uint8 *data, int len);

/*!
 * \brief Get LED controller number and port number within a LED controller
 *        for physical port.
 *
 * For CMICd, all physical ports are controlled by multiple LED controllers.
 * For CMICx, all physical ports are controlled by one single LED controller.
 * Each LED controller will take care part of physical port.
 * Each physical port hardware status will be placed in LED controller RAM
 * with an offset(port).
 *
 * \param [in] port Physical port number
 * \param [out] led_uc LED controller number.
 * \param [out] led_uc_port Port number within a LED controller.

 * \retval SYS_OK Get LED controller and port successfully.
 * \retval SYS_ERR_XXX Fail to get LED controller and port.
 */
extern sys_error_t
board_led_pport_to_led_uc_port(int port, int *led_uc, int *led_uc_port);

#endif /* LED_H */
