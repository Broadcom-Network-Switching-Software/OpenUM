/*! \file utgpio.h
 *
 * GPIO unit tests.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef __UTGPIO_H__
#define __UTGPIO_H__

extern sys_error_t utgpio_intr_type_test(void);
extern sys_error_t utgpio_intr_latency_test(void);
extern sys_error_t utgpio_intr_regression_test(void);
#endif /* __UTGPIO_H__ */
