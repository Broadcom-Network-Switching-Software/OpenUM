/*! \file temp.h
 *
 * Temperature monitor supported API.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef __FLTEMP_H__
#define __FLTEMP_H__

/*! The max number of monitored temperature */
#define CFG_MAX_TEMP_MONITOR_CNT    2

/*!
 * \brief Get chip monitored temperature.
 *
 * The returned temperature is 10 times of the measured temperature.
 *
 * \param [out] temp_data Pointer to temperature data array.
 * \param [in] temp_data_size Temperature data array size in 32 bit word.
 * \param [out] temp_mon_cnt Pointer to monitored temperature count.
 * \retval SYS_OK No errors.
 */
extern sys_error_t
board_temp_monitor_get(int32 *temp_data, int32 temp_data_size,
                        int32 *temp_mon_cnt);

#endif /* __FLTEMP_H__ */