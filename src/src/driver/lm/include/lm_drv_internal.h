/*! \file lm_drv_internal.h
 *
 * Link Manager common driver definitions.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef LM_DRV_INTERNAL_H
#define LM_DRV_INTERNAL_H

/* Maximum supported unit number. */
#define CONFIG_MAX_UNITS                          1

/* Maximum ports. */
#define CONFIG_MAX_PORTS                          SOC_MAX_NUM_PORTS

/* Unit number check macro. */
#define UNIT_VALID(_u)                            (_u < CONFIG_MAX_UNITS)

/* Invalid port. */
#define INVALID_LPORT                             -1

/*!
 * \brief Initialize hardware linkscan.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors, otherwise failure in initializing hardware
 *         linkscan.
 */
typedef int (*ls_hw_init_f)(int unit);

/*!
 * \brief Clean up hardware linkscan.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors, otherwise failure in clean up hardware
 *         linkscan.
 */
typedef int (*ls_hw_cleanup_f)(int unit);

/*!
 * \brief Configure hardware linkscan.
 *
 * \param [in] unit Unit number.
 * \param [in] pbm Bitmap of ports to be enabled with hardware linkscan.
 *
 * \retval SYS_OK No errors, otherwise failure in configuring hardware
 *         linkscan.
 */
typedef int (*ls_hw_config_f)(int unit, pbmp_t pbm);

/*!
 * \brief Get link state from hardware linkscan.
 *
 * \param [in] unit Unit number.
 * \param [out] pbm Bitmap of ports in link up state.
 *
 * \retval SYS_OK No errors, otherwise failure in getting link state.
 */
typedef int (*ls_hw_link_get_f)(int unit, pbmp_t *pbm);

/*!
 * \brief Register callback for hardware linkscan interrupt.
 *
 * \param [in] unit Unit number.
 * \param [in] intr_func Callback function.
 *
 * \retval SYS_OK No errors, otherwise failure in registering callback.
 */
typedef int (*ls_hw_link_intr_func_set_f)(int unit, sys_intr_f intr_func);

/*!
 * \brief Stop hardware linkscan.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 * \retval SHR_E_TIMEOUT Failed to disable hardware linkscan.
 */
typedef int (*ls_hw_scan_stop_f)(int unit);

/*!
 * \brief Start hardware linkscan.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 */
typedef int (*ls_hw_scan_start_f)(int unit);

/*!
 * \brief Link Manager driver object
 *
 * The Link Manager driver object is used to initialize and access hardware
 * linkscan or firmware linkscan for a switch device, and access MAC/PHY via
 * Port Control for software linkscan or port update due to link state change.
 */
typedef struct lm_drv_s {
    /*! Unit number */
    int unit;

    /*! Initialize hardware linkscan */
    ls_hw_init_f hw_init;

    /*! Clean up hardware linkscan */
    ls_hw_cleanup_f hw_cleanup;

    /*! Configure hardware linkscan */
    ls_hw_config_f hw_config;

    /*! Get link status from hardware linkscan */
    ls_hw_link_get_f hw_link_get;

    /*! Register interrupt callback */
    ls_hw_link_intr_func_set_f hw_intr_cb_set;

    /*! Stop hardware linkscan */
    ls_hw_scan_stop_f hw_scan_stop;

    /*! Start hardware linkscan */
    ls_hw_scan_stop_f hw_scan_start;

} lm_drv_t;

/*!
 * \brief Initialize LM device driver.
 *
 * Install base driver functions and initialize device features.
 *
 * \param [in] unit Unit number.
 */
extern int
lm_drv_attach(int unit, lm_drv_t *drv);

/*!
 * \brief Perform device specific initialization for linkscan.
 *
 * \param [in] unit Unit number.
 */
extern int
lm_dev_init(int unit);

/*!
 * \brief Perform device specific clean up for linkscan.
 *
 * \param [in] unit Unit number.
 */
extern int
lm_dev_cleanup(int unit);

/*!
 * \brief Hardware linkscan initialization.
 *
 * \param [in] unit Unit number.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_UNAVAIL Hardware linkscan initialization is not supported.
 */
extern int
lm_hw_init(int unit);

/*!
 * \brief Configure hardware linkscan.
 *
 * \param [in] unit Unit number.
 * \param [in] hw Bitmap of ports to be enabled with hardware linkscan.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_UNAVAIL Hardware linkscan config is not supported.
 */
extern int
lm_hw_config(int unit, pbmp_t hw);

/*!
 * \brief Get link state from hardware linkscan.
 *
 * \param [in] unit Unit number.
 * \param [out] hw Bitmap of ports in link up state (phy_link + fault).
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_UNAVAIL Get link from hardware linkscan is not supported.
 */
extern int
lm_hw_link_get(int unit, pbmp_t *hw);

/*!
 * \brief Register callback function for interrupt from hardware linkscan.
 *
 * \param [in] unit Unit number.
 * \param [in] cb Callback function.
 *
 * \retval SYS_OK No errors.
 * \retval SYS_ERR_UNAVAIL Register interrupt callback is not supported.
 */
extern int
lm_hw_intr_cb_set(int unit, sys_intr_f cb);

#endif /* LM_DRV_INTERNAL_H */
