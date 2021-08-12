/*! \file led_internal.h
 *
 * Internal LED driver definitions.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef LED_INTERNAL_H
#define LED_INTERNAL_H

#include <system.h>
#include <led.h>

/*******************************************************************************
 * LED internal data structure
 */

/* Maximum supported unit number. */
#define CONFIG_MAX_UNITS                          1

/* Maximum ports. */
#define CONFIG_MAX_PORTS                          SOC_MAX_NUM_PORTS

/* Unit number check macro. */
#define UNIT_VALID(_u)                            (_u < CONFIG_MAX_UNITS)

/* sal_alloc for porting. */
#define sal_alloc(_size_, _desc_)                 sal_malloc(_size_)

/*!
 * \brief Size of CMIC LED uC data ram.
 */
#define CMIC_LED_UC_DATA_RAM_SIZE  256

/*!
 * \brief Type defininition of internal control structure for LED module.
 */
typedef struct led_dev_s {

    /*! A map from phyiscal port to LED controller number */
    int pport_to_led_uc[CONFIG_MAX_PORTS];

    /*! A map from phyiscal port to LED controller number */
    int pport_to_led_uc_port[CONFIG_MAX_PORTS];

    /*! Starting address of "control_data" space */
    int control_data_start[CONFIG_MAX_PORTS];

} led_dev_t;

/*******************************************************************************
 * LED driver function prototypes
 */
/*!
 * \brief Get maximun number of LED controller.
 *
 * \param [in] unit Unit number.
 * \p
 * \return maximun LED controller number of a device unit.
 */
typedef int
(*led_uc_num_get_f)(int unit, int *uc_num);


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
typedef int
(*led_init_f)(int unit);

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
typedef int
(*led_cleanup_f)(int unit);

/*!
 * \brief Load LED firmware into a LED controller.
 *
 * This function loads formware into the run-time memory of a given
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
typedef int
(*led_fw_load_f)(int unit, int led_uc, const uint8 *buf, int len);

/*!
 *\brief Start/Stop a LED controller.
 *
 * \param [in]  unit Unit number.
 * \param [in]  led_uc LED controller number.
 * \param [in]  start !0: start a controller.
 *                    0: stop a controller.
 *
 * \retval SYS_OK Start/stop a LED controller successfully.
 * \retval SYS_ERR_XXX Fail to start/stop a LED controller.
 */
typedef int
(*led_fw_start_set_f)(int unit, int led_uc, int start);


/*!
 *\brief Stop a specified LED controller.
 *
 * \param [in]   unit Unit number.
 * \param [in]   led_uc LED controller number.
 * \param [out]  start !0: start a controller.
 *                      0: stop a controller.
 *
 * \retval SYS_OK Stop a LED controller successfully.
 * \retval SYS_ERR_XXX Fail to stop a LED controller.
 */
typedef int
(*led_fw_start_get_f)(int unit, int led_uc, int *start);

/*!
 * \brief Write data into "control_data" space.
 *
 * "control_data" space contains:
 *     1. Data or information from LED application on the host processor.
 *     2. Internal variable of LED firmware.
 *
 * For CMICd, default location of "control_data" space could vary from chip
 * to chip. Please refer to device-specific documentation for details.
 *
 * \param [in] unit Unit number.
 * \param [in] led_uc LED controller number.
 * \param [in] offset Offset in byte.
 * \param [in] value Write value.
 *
 * \retval SYS_OK Write control_data successfully.
 * \retval SYS_ERR_XXX Fail to write data into control_data space.
 */
typedef int
(*led_control_data_write_f)(int unit, int led_uc,
                            int offset, uint8 value);


/*!
 * \brief Read data from "control_data" space.
 *
 *
 * \param [in] unit Unit number.
 * \param [in] led_uc LED controller number.
 * \param [in] offset Offset in byte.
 * \param [out] value Read value.
 *
 * \retval SYS_OK Write control_data successfully.
 * \retval SYS_ERR_XXX Fail to write data into control_data space.
 */

typedef int
(*led_control_data_read_f) (int unit, int led_uc,
                            int offset, uint8 *value);

/*!
 * \brief Given a physical port number, get correponding LED controller
 *        number and LED controller port.
 *
 * For CMICd, all physical ports could be controlled by multiple LED controllers.
 * For CMICx, all physical ports are controlled by one single LED controller.
 *
 * This function is to get LED controller number and relative port within a
 * LED controller for a physical port.
 *
 *
 * \param [in] unit Unit number.
 * \param [in] pport Physical number
 * \param [out] led_uc LED controller number.
 * \param [out] port Port number within a LED controller.

 * \retval SYS_OK Get LED controller and port successfully.
 * \retval SYS_ERR_XXX Fail to get LED controller and port.
 */
typedef int
(*led_pport_to_led_uc_port_f) (int unit, int pport,
                               int *led_uc, int *led_uc_port);

/*! LED driver object. */
typedef struct led_drv_s {

    /*! Initialize LED driver. */
    led_init_f init;

    /*! Clean up LED driver. */
    led_cleanup_f cleanup;

    /*! LED firmware loader. */
    led_fw_load_f fw_load;

    /*! Get LED uC number */
    led_uc_num_get_f uc_num_get;

    /*! Stop/start LED uC. */
    led_fw_start_set_f uc_start_set;

    /*! Get LED uC is started or not. */
    led_fw_start_get_f uc_start_get;

    /*! Read "control_data" space of LED uC */
    led_control_data_read_f uc_control_data_read;

    /*! Write "control_data" space of LED uC */
    led_control_data_write_f uc_control_data_write;

    /*! Physical port to LED uc number and port */
    led_pport_to_led_uc_port_f pport_to_led_uc_port;

} led_drv_t;

/*!
 * \brief Install device-specific LED driver.
 *
 * Install device-specific LED driver into top-level LED API.
 *
 * Use \c led_drv = NULL to uninstall a driver.
 *
 * \param [in] unit Unit number.
 * \param [in] led_drv LED driver object.
 *
 * \retval SYS_OK No errors
 */
extern int
led_drv_init(int unit, const led_drv_t *led_drv);

/*!
 * \brief Get device-specific data of LED driver.
 *
 * \param [in] unit Unit number.
 * \param [out] led_dev Device data of LED driver.
 *
 * \retval SYS_OK No errors
 */
extern int
led_dev_get(int unit, led_dev_t **led_dev);

#endif /* LED_INTERNAL_H */
