/*! \file link.h
 *
 * Link board APIs.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef LINK_H
#define LINK_H

#include <types.h>
#include <error.h>

/*!
 * \brief Interrupt handler for hardware link change detection.
 *
 * \param [in] link_uplist Link status port bit map.
 */
typedef void
(*board_hw_linkchange_intr_func_f) (uint8 *link_uplist);


/*!
 * \brief Install/uninstall HW linkchange interrupt handler.
 *
 * \param [in] handler Pointer for interrupt handler.
 *                     Set handler as NULL to uninstall.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_hw_linkchange_intr_func_set(board_hw_linkchange_intr_func_f handler);

/*!
 * \brief HW link change configuration.
 *
 * This function enables hardware linkscan on ports.
 * If the input is empty port bitmap then it disable HW link change.
 * If the input is not empty port bitmap, the link change detection
 * will be started after this function.
 *
 * \param [in] uplist Port bit map with hardware linkscan enabled.
 *
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_hw_linkchange_config(uint8 *uplist);

#endif /* LINK_H */
