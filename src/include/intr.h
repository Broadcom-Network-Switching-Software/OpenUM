/*
 * $Id: intr.h,v 1.6 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _INTR_H_
#define _INTR_H_

/**
 * Save and disable interrupt handling
 *
 * This function will disable interrupt handling. All interrupts
 * request will be in pending. Original enable status will be returned
 * in the status parameter if it's not NULL.
 *
 * \param unit    Unit number.
 * \param status  Enable status of interrupt handling.
 *                status can be NULL if user don't need backup status.
 *
 * \retval SYS_OK  Operation completed successfully.
 */
typedef sys_error_t (*sys_intr_handling_save_disable_f) (int unit,
                                                         uint32 *status);

/**
 * Restore or enable interrupt handling.
 *
 * This function will restore previous enable status of interrupt handling.
 * If no previous status(status = NULL) is provided, then interrupt handling
 * will be enabled by default.
 *
 * \param unit    Unit number.
 * \param status  Previous enable status of interrupt handling.
 *
 * \retval SYS_OK  Operation completed successfully.
 */
typedef sys_error_t (*sys_intr_handling_restore_enable_f) (int unit,
                                                          uint32 *status);

/**
 * Set interrupt disable/enable for a interrupt.
 *
 * \param unit    Unit number.
 * \param intrnum Interrupt number.
 * \param enable  0 disable interrupt otherwise enable interrupt.
 *
 * \retval SYS_OK  Operation completed successfully.
 */
typedef sys_error_t (*sys_intr_enable_set_f) (int unit,
                                              int intrnum,
                                              int enable);
/**
 * Get interrupt disable/enable status for a interrupt.
 *
 * \param unit    Unit number.
 * \param intrnum Interrupt number.
 * \param enable  Interrupt disable/enable status.
 *
 * \retval SYS_OK  Operation completed successfully.
 */
typedef sys_error_t (*sys_intr_enable_get_f) (int unit,
                                              int intrnum,
                                              int *enable);

/**
 * Interrupt handler function prototype.
 *
 * \param param Parameters for interrupt handler.
 */
typedef void (*sys_intr_f) (uint32 param);

/**
 * Device-specific entry pointer for all interrupts.
 *
 * \param unit Unit number.
 *
 * \retval SYS_OK Operation completed successfully.
 */
typedef sys_error_t (*sys_intr_entry_f) (int unit);

/**
 * Install/uninstall interrupt handler function.
 *
 * \param intrnum Interrupt number.
 * \param fn      Handler function. If fn == NULL then uninstall
 *                handler.
 * \param param   Interrupt handler parameter.
 *
 * \retval SYS_OK Operation completed successfully.
 */
typedef sys_error_t (*sys_intr_func_set_f) (int unit,
                                            int intrnum,
                                            sys_intr_f fn,
                                            uint32 param);

/*! Device specific interrupt driver. */
typedef struct {

    /* Enter critical section. */
    sys_intr_handling_save_disable_f intr_handling_save_disable;

    /* Leave critical section. */
    sys_intr_handling_restore_enable_f intr_handling_restore_enable;

    /* Set interrupt disable/enable for a interrupt. */
    sys_intr_enable_set_f intr_enable_set;

    /* Get interrupt disable/enable status for a interrupt. */
    sys_intr_enable_get_f intr_enable_get;

    /* Install/uninstall interrupt callback function. */
    sys_intr_func_set_f intr_func_set;

    /* Interrupt entry for all interrupts. */
    sys_intr_entry_f intr_entry;

} sys_intr_drv_t;

/**
 * Install device-specific interrupt driver.
 *
 * \param drv - Device-specific driver.
 *
 * \retval SYS_OK Operation completed successfully.
 */
extern sys_error_t
sys_intr_drv_init(sys_intr_drv_t *drv);

/**
 * Entry pointer for all interrupts.
 *
 * \retval SYS_OK Operation completed successfully.
 */
extern sys_error_t sys_intr_entry(void);

/**
 * Install/uninstall interrupt handler function.
 *
 * \param intrnum Interrupt number.
 * \param fn      Handler function. If fn == NULL then uninstall
 *                handler.
 * \param param   Interrupt handler parameter.
 *
 * \retval SYS_OK Operation completed successfully.
 */
extern sys_error_t
sys_intr_func_set(int intrnum, sys_intr_f fn, uint32 param);

/**
 * Save and disable interrupt handling
 *
 * This function will disable interrupt handling. All interrupts
 * request will be in pending. Original enable status will be returned
 * in the status parameter if it's not NULL.
 *
 * \param status  Enable status of interrupt handling.
 *                status can be NULL if user don't need backup status.
 *
 * \retval SYS_OK  Operation completed successfully.
 */
extern sys_error_t
sys_intr_handling_save_disable(uint32 *status);

/**
 * Restore or enable interrupt handling.
 *
 * This function will restore previous enable status of interrupt handling.
 * If no previous status(status = NULL) is provided, then interrupt handling
 * will be enabled by default.
 *
 * \param status  Previous enable status of interrupt handling.
 *
 * \retval SYS_OK  Operation completed successfully.
 */
extern sys_error_t
sys_intr_handling_restore_enable(uint32 *status);

/**
 * Set interrupt disable/enable for a interrupt.
 *
 * \param intrnum Interrupt number.
 * \param enable  0 disable interrupt otherwise enable interrupt.
 *
 * \retval SYS_OK  Operation completed successfully.
 */
extern sys_error_t
sys_intr_enable_set(int intrnum, int enable);

/**
 * Get interrupt disable/enable status for a interrupt.
 *
 * \param intrnum Interrupt number.
 * \param enable  Interrupt disable/enable status.
 *
 * \retval SYS_OK  Operation completed successfully.
 */
extern sys_error_t
sys_intr_enable_get(int intrnum, int *enable);

#endif /* _INTR_H_ */
