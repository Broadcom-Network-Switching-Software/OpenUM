/*! \file led.h
 *
 * LED API.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef __LED_H__
#define __LED_H__

/*******************************************************************************
 * LED public definition
 */

/*!
 * \brief Invalid LED controller number definition.
 */
#define LED_UC_INVALID       -1

/*******************************************************************************
 * LED public APIs
 */

/*!
 * \brief Device-specific LED initialization.
 *
 * This function is used to initialize the chip-specific LED
 * configuration, which may include both software and hardware
 * operations.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX LED driver initialization failed.
 */
extern int
led_init(int unit);

/*!
 * \brief Device-specific LED driver cleanup.
 *
 * This function is used to clean up the LED driver resources
 * allocated by \ref led_init_f.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_XXX Failed to clean up.
 */
extern int
led_cleanup(int unit);

/*!
 * \brief Get number of LED controllers for a device unit.
 *
 * \param [in] unit Unit number.
 * \param [in] uc_num number of LED uC.
 *
 * \return Number of LED controllers for a device unit.
 */
extern int
led_uc_num_get(int unit, int *uc_num);

/*!
 *\brief Start a specified LED controller.
 *
 * \param [in]  unit Unit number.
 * \param [in]  led_uc LED controller number.
 *
 * \retval SYS_OK Start a LED controller successfully.
 * \retval SYS_ERR_XXX Fail to start a LED controller.
 */
extern int
led_fw_start(int unit, int led_uc);


/*!
 *\brief Stop a specified LED controller.
 *
 * \param [in]  unit Unit number.
 * \param [in]  led_uc LED controller number.
 *
 * \retval SYS_OK Stop a LED controller successfully.
 * \retval SYS_ERR_XXX Fail to stop a LED controller.
 */
extern int
led_fw_stop(int unit, int led_uc);

/*!
 *\brief Get LED controllers start or not.
 *
 * \param [in]  unit Unit number.
 * \param [in]  led_uc LED controller number.
 * \param [out] start Indicate LED controller start or not.
 *
 * \return SYS_OK if successful.
 */
extern int
led_fw_start_get(int unit, int led_uc, int *start);

/*!
 * \brief Write data into "control_data" space.
 *
 *  "control_data" space contains
 *      1. Data or information from LED application on the host processor.
 *      2. Internal variable of LED firmware.
 *
 *  For CMICd, default location of "control_data" space could vary from chip
 *  to chip. Please refer to device-specific documentation for details.
 *
 * \param [in] unit Unit number.
 * \param [in] led_uc LED controller number.
 * \param [in] offset Offset in byte.
 * \param [in] value Write value.
 *
 * \retval SYS_OK Write control_data successfully.
 * \retval SYS_ERR_XXX Fail to write data into control_data space.
 */
extern int
led_control_data_write(int unit, int led_uc, int offset, uint8 value);

/*!
 * \brief Read data from "control_data" space.
 *        and LED host application.
 *
 *
 * \param [in] unit Unit number.
 * \param [in] led_uc LED controller number.
 * \param [in] offset Offset in byte.
 * \param [out] value Read result.
 *
 * \retval SYS_OK Read data successfully.
 * \retval SYS_ERR_XXX Fail to read data from control_data space.
 */
extern int
led_control_data_read(int unit, int led_uc, int offset, uint8 *value);

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
 * \param [in] unit Unit number.
 * \param [in] port Physical port number
 * \param [out] led_uc LED controller number.
 * \param [out] led_uc_port Port number within a LED controller.

 * \retval SYS_OK Get LED controller and port successfully.
 * \retval SYS_ERR_XXX Fail to get LED controller and port.
 */
extern int
led_pport_to_led_uc_port(int unit, int port, int *led_uc, int *led_uc_port);

/*!
 * \brief Load LED firmware (binary format) into a LED controller.
 *
 * This function loads firmware into the run-time memory of a given
 * LED controller. The function assumes that the LED controller is not
 * running.
 *
 * \param [in] unit Unit number.
 * \param [in] led_uc LED controller number.
 * \param [in] buf LED firmware buffer.
 * \param [in] len Length of firmware buffer.
 *
 * \retval SYS_OK Firmware successfully loaded.
 * \retval SYS_ERR_XXX Failed to load firmware.
 */
extern int
led_fw_load(int unit, int led_uc, const uint8_t *buf, int len);

#endif /* __LED_H__ */
