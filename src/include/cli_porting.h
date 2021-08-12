/*! \file cli_porting.h
 *
 * code convention intended for porting from SDKLT.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef CLI_PORTING_H
#define CLI_PORTING_H

#define BSL_META(_str_) (_str_)
#define LOG_INFO(_bsl_,_str_) cli_out(_str_)
#define cli_out sal_printf
#define sal_alloc(_size_, _desc_) sal_malloc(_size_)
#define assert(_x_) SAL_ASSERT(_x_)

#endif /* CLI_PORTING_H */
