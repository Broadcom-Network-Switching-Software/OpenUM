/*! \file spi_mgmt.h
 *
 * SPI management example header file.
 */
 /*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _SPI_MGMT_H_
#define _SPI_MGMT_H_

/*! Base memory address for SPI management */
#define SPI_MGMT_MEM_BASE       0x012ffc00

/*! Memory address for SPI management command */
#define SPI_MGMT_MEM_CMD        SPI_MGMT_MEM_BASE

/*! Memory address for SPI management status */
#define SPI_MGMT_MEM_STATUS     (SPI_MGMT_MEM_BASE + 4)

/*! Base memory address for SPI management data */
#define SPI_MGMT_MEM_DATA       (SPI_MGMT_MEM_BASE + 8)

#define SPI_MGMT_CMD_ACTIVE     0x1
#define SPI_MGMT_OP_ERROR       0x2

/*! Supposed max data length */
#define CFG_SPI_MGMT_DATA_WORD_LEN      16

/* Various SPI management commands */
enum spi_mgmt_cmds {
    SPI_CMD_PORT_SPEED_SET = 1,
    SPI_CMD_PORT_SPEED_GET,
    SPI_CMD_VLAN_CREATE,
    SPI_CMD_VLAN_COUNT,
    SPI_CMD_WDT_RESET_FLAG_GET,
    SPI_CMD_CHIP_MONITOR_TEMP_GET
};

/*!
 * \brief Enable SPI management.
 */
extern void spi_mgmt_enable(void);

/*!
 * \brief Disable SPI management.
 */
extern void spi_mgmt_disable(void);

#endif /* _SPI_MGMT_H_ */
