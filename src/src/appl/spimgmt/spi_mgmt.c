/*! \file spi_mgmt.c
 *
 * A simple SPI management example code.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"

#ifdef CFG_SPI_MGMT_INCLUDED

#include <appl/spi_mgmt.h>

static BOOL spi_mgmt_enabled = FALSE;

/*
 * SPI master side:
 *  - poll status till done (active field = 0)
 *  - fill command and data (if any).
 *  - set status active field to 1.
 *  - poll status active field till done, and check status error field.
 *  - if error, do error handling (if any).
 *  - if response data is expected and no error, then retrieve data.
 *
 * SPI slave side:
 *  - poll status active field till it is 1 (active).
 *  - retrieve/process command and data (if any).
 *  - set response data (if any).
 *  - update status error field.
 *  - set status active field to 0 (done).
 */

/*! \brief SPI management data structure. */
typedef struct spi_mgmt_s {
    /*! Various SPI management commands */
    uint32 cmd;

    /* bit[0]: active
     *     1: active, 0: done
     * bit[1]: error
     *     1: error, 0: ok
     */
    /*! SPI management operation status */
    uint32 stat;

    /*! Data for SPI management command request/response */
    int32 data[CFG_SPI_MGMT_DATA_WORD_LEN];
} spi_mgmt_t;

static sys_error_t
spi_mgmt_cmd_process(uint32 cmd, volatile int32 *data)
{
    sys_error_t ret = SYS_OK;
    int32 uport = 0;

    switch (cmd) {
        case SPI_CMD_PORT_SPEED_SET:
            sal_printf("Set port=%d speed=%d\n", data[0], data[1]);
            ret = board_port_speed_set((uint16)data[0], (int)data[1]);
            break;
        case SPI_CMD_PORT_SPEED_GET:
            uport = data[0];
            ret = board_port_speed_get((uint16)uport, (int *)data);
            sal_printf("Get port=%d speed=%d\n", uport, data[0]);
            break;
        case SPI_CMD_VLAN_CREATE:
            ret = board_vlan_create((uint16)data[0]);
            if (ret == SYS_OK)
                sal_printf("Successfully create VLAN %d\n", data[0]);
            else
                sal_printf("Fail to create VLAN %d\n", data[0]);
            break;
        case SPI_CMD_VLAN_COUNT:
            data[0] = board_vlan_count();
            sal_printf("VLAN count = %d\n", data[0]);
            break;
        case SPI_CMD_WDT_RESET_FLAG_GET:
            data[0] = board_wdt_reset_triggered();
            if (data[0])
                sal_printf("Watchdog reset triggered\n");
            else
                sal_printf("Watchdog reset NOT triggered\n");
            break;
        case SPI_CMD_CHIP_MONITOR_TEMP_GET:
            // Get temperature
            break;
        default:
            sal_printf("Unsupported SPI management command %d\n", cmd);
            break;
    };

    return ret;
}

static void
spi_mgmt_task(void *param)
{
    sys_error_t ret;
    volatile spi_mgmt_t *spi = (spi_mgmt_t *)SPI_MGMT_MEM_BASE;

    if (spi->stat & SPI_MGMT_CMD_ACTIVE) {
        ret = spi_mgmt_cmd_process(spi->cmd, spi->data);

        /* Set error field */
        if (ret != SYS_OK)
            spi->stat |= SPI_MGMT_OP_ERROR;
        else
            spi->stat &= !SPI_MGMT_OP_ERROR;

        /* Mark command done */
        spi->stat &= ~SPI_MGMT_CMD_ACTIVE;
    }
}

void
spi_mgmt_enable(void)
{
    if (spi_mgmt_enabled)
        return;

    spi_mgmt_enabled = TRUE;
    sal_printf("Enable SPI mgmt ..............\n");

    /* Clear SPI mgmt memory area */
    sal_memset((void*)SPI_MGMT_MEM_BASE, 0, sizeof(spi_mgmt_t));

    task_add(spi_mgmt_task, (void *)NULL);
}

void
spi_mgmt_disable(void)
{
    if (!spi_mgmt_enabled)
        return;

    spi_mgmt_enabled = FALSE;
    sal_printf("Disable SPI mgmt ..............\n");

    /* Clear SPI mgmt memory area */
    sal_memset((void*)SPI_MGMT_MEM_BASE, 0, sizeof(spi_mgmt_t));

    task_remove(spi_mgmt_task);
}

#endif /* CFG_SPI_MGMT_INCLUDED */
