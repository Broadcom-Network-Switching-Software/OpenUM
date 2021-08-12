/*
 * $Id: flash.c,v 1.19 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include <utils/ui.h>
#include <intr.h>
#if CFG_FLASH_SUPPORT_ENABLED

#if defined(CFG_INTR_INCLUDED) && (CFG_POLLED_INTR == 0)
#define INTR_VAR_INIT()    uint32 _intr_status;
#define INTR_DISABLE()     do { \
                               sys_intr_handling_save_disable(&_intr_status); \
                           } while(0)
#define INTR_RESTORE()     do { \
                               sys_intr_handling_restore_enable(&_intr_status);\
                           } while(0)
#else
#define INTR_VAR_INIT()    do { } while(0)
#define INTR_DISABLE()     do { } while(0)
#define INTR_RESTORE()     do { } while(0)
#endif
extern flash_dev_t current_flash_dev;

/**
 * flash initialization
 *
 *
 * @param dev (IN)- NULL : To probe flash automaticly
 *                            !NULL: To specify the device we want to use, no auto-probe
 * @return sys_error_t
 *             SYS_OK : there is no error
 */

sys_error_t flash_init(flash_dev_t *dev) {

     INTR_VAR_INIT();

     if (dev == NULL) {
         dev = &current_flash_dev;
#if  defined(__SIM__) || (CONFIG_EMULATION==1)
         /* do not call the flash_init when running UM in BCMSIM mode */
#else
         INTR_DISABLE();
         dev->funs->flash_init(NULL);
         INTR_RESTORE();
#endif
         sal_printf("Flash detected: %s \n", dev->name);
     } else {
         INTR_DISABLE();
         dev->funs->flash_init(dev);
         INTR_RESTORE();
         sal_memcpy(&current_flash_dev, dev, sizeof(dev));
         return SYS_OK;
     }

     if (sal_strcmp(current_flash_dev.name, "Unknown Flash")==0) {
         sal_printf("ID: %x %x %x %x\n", current_flash_dev.jedec_id[0], current_flash_dev.jedec_id[1],current_flash_dev.jedec_id[2],current_flash_dev.jedec_id[3]);
         sal_printf("Please check the flash supporting table\n");
     }

     return SYS_OK;
}

/**
 * Get the pointer of the current flash driver instance
 *
 *
 * @return flash_dev_t* the pointer of the current flash driver instance
 *             NULL : for Unknown flash driver to forbid any write to flash
 */

flash_dev_t *flash_dev_get(void) {

    if (sal_strcmp(current_flash_dev.name, "Unknown Flash")==0) {
        return NULL;
    }

    return &current_flash_dev;
}
/*
 *  Return the size of the block which is at the given address
 */
size_t
flash_block_size(const hsaddr_t addr)
{
  int16 i;
  size_t offset;
  flash_dev_t * dev;

  dev = board_get_flash_dev();
  if (!dev) {
    return SYS_ERR;
  }

  SAL_ASSERT((addr >= dev->start) && (addr <= dev->end));

  offset = addr - dev->start;
  for (i=0; i < dev->num_block_infos; i++) {
    if (offset < (dev->block_info[i].blocks * dev->block_info[i].block_size))
      return dev->block_info[i].block_size;
    offset = offset -
      (dev->block_info[i].blocks * dev->block_info[i].block_size);
  }
  return 0;
}
/*
 * Return the first address of a block. The flash might not be aligned
 * in terms of its block size. So we have to be careful and use
 * offsets.
 */
hsaddr_t
flash_block_begin(hsaddr_t addr, flash_dev_t *dev)
{
  size_t block_size;
  hsaddr_t offset;

  block_size = flash_block_size(addr);

  offset = addr - dev->start;
  offset = (offset / block_size) * block_size;
  return offset + dev->start;
}

sys_error_t
flash_erase(hsaddr_t flash_base, size_t len)
{
    hsaddr_t block, end_addr;
    flash_dev_t * dev;
    int32 erase_count;
    sys_error_t rv = SYS_OK;

    INTR_VAR_INIT();

    dev = board_get_flash_dev();
    if (!dev) {
        return SYS_ERR;
    }

    if (flash_base < dev->start || (flash_base + len -1) > dev->end) {
        return SYS_ERR_PARAMETER;
    }
    /*
     * Check whether or not we are going past the end of this device, on
     * to the next one. If so the next device will be handled by a
     * recursive call later on.
     */
    if (len > (dev->end + 1 - flash_base)) {
        end_addr = dev->end;
    } else {
        end_addr = flash_base + len - 1;
    }
    /* erase can only happen on a block boundary, so adjust for this */
    block         = flash_block_begin(flash_base, dev);
    erase_count   = (end_addr + 1) - block;

    while (erase_count > 0) {
        size_t block_size = flash_block_size(block);

#ifdef CFG_WDT_INCLUDED
#ifndef __LINUX__
        board_wdt_ping();
#endif
#endif
        /* Pad to the block boundary, if necessary */
        if (erase_count < block_size) {
            erase_count = block_size;
        }

        INTR_DISABLE();
        rv = (*dev->funs->flash_erase_block)(dev,block);
        INTR_RESTORE();

        if (SYS_OK != rv) {
            break;
        }
        block       += block_size;
        erase_count -= block_size;
    }

    return rv;
}

/*
 * <b>Description:</b> Dump flash data and display it on console
 * @param offset - (IN) FLASH offset
 * @param len -     (IN) FLASH len
 * @return SYS_OK
 * \n      Operation completed successfully.
 */
sys_error_t
flash_dump(hsaddr_t offset, uint32 len)
{
    uint8 p;
    uint8 c, buf[17];

    p = 0;
    for(;;) {
#ifdef CFG_WDT_INCLUDED
#ifndef __LINUX__
        board_wdt_ping();
#endif
#endif
        if (len == 0) {
            break;
        }

        if (p == 0) {
            sal_printf("\n %08lX: ", (uint32)DATAPTR2MSADDR(offset));
        }

        flash_read(offset + CFG_FLASH_START_ADDRESS, &c, 1);
        if (p == 8) {
            sal_putchar(' ');
        }
        sal_printf("%02bX ", c);
        if (c < ' ' || c > '~') {
            c = '.';
        }

        buf[p] = c;

        p++;
        if (p == 16 || len == 1) {
            buf[p] = 0;
            sal_printf("   %s", buf);
            p = 0;
        }

        offset++;
        len--;
#if CFG_CLI_ENABLED
        /* Cancel per user request */
        while (sal_char_avail()) {
            char ch = sal_getchar();
            ui_backspace();
            if (ch == UI_KB_ESC || ch == UI_KB_CTRL_C) {
                sal_printf("\n");
                return SYS_OK;
            }
        }
#endif
    }

    sal_printf("\n");

    return SYS_OK;
}


sys_error_t
flash_program(hsaddr_t flash_base, const void *ram_base, size_t len)
{
    flash_dev_t * dev;
    hsaddr_t addr, end_addr, block;
    const uint8 *ram = ram_base;
    int32 write_count;
    size_t offset;
    sys_error_t rv = SYS_OK;

    INTR_VAR_INIT();

    dev = board_get_flash_dev();
    if (!dev) {
        return SYS_ERR;
    }

    if (flash_base < dev->start || (flash_base + len - 1) > dev->end) {
        return SYS_ERR_PARAMETER;
    }

    addr = flash_base;
    if (len > (dev->end + 1 - flash_base)) {
      end_addr = dev->end;
    } else {
      end_addr = flash_base + len - 1;
    }
    write_count = (end_addr + 1) - flash_base;


    /*
     * The first write may be in the middle of a block. Do the necessary
     * adjustment here rather than inside the loop.
     */
    block = flash_block_begin(flash_base, dev);
    if (addr == block) {
        offset = 0;
    } else {
        offset = addr - block;
    }

    while (write_count > 0) {
#ifdef CFG_WDT_INCLUDED
#ifndef __LINUX__
        board_wdt_ping();
#endif
#endif
        size_t block_size = flash_block_size(addr);
        size_t this_write;
        if (write_count > (block_size - offset)) {
            this_write = block_size - offset;
        } else {
            this_write = write_count;
        }
        /* Only the first block may need the offset. */
        offset = 0;

        INTR_DISABLE();
        rv = (*dev->funs->flash_program)(dev, addr, ram, this_write);
        INTR_RESTORE();

        if (SYS_OK != rv) {
            break;
        }
        write_count -= this_write;
        addr        += this_write;
        ram         += this_write;
    }

    return rv;
}

sys_error_t
flash_read(hsaddr_t flash_base, void *ram_base, size_t len)
{
    flash_dev_t * dev;
    hsaddr_t addr, end_addr;
    uint8 * ram = (uint8 *)ram_base;
    int32 read_count;
    sys_error_t rv = SYS_OK;

    INTR_VAR_INIT();

    dev = board_get_flash_dev();
    if (!dev) {
        return SYS_ERR;
    }

    if (flash_base < dev->start || (flash_base + len - 1) > dev->end) {
        return SYS_ERR_PARAMETER;
    }

    addr = flash_base;
    if (len > (dev->end + 1 - flash_base)) {
        end_addr = dev->end;
    } else {
        end_addr = flash_base + len - 1;
    }
    read_count = (end_addr + 1) - flash_base;

    if (!dev->funs->flash_read) {
        sal_memcpy(ram, (void*)addr, read_count);
    } else {
        /*
         * We have to indirect through the device driver.
         * The first read may be in the middle of a block. Do the necessary
         * adjustment here rather than inside the loop.
         */
        size_t offset;
        hsaddr_t  block = flash_block_begin(flash_base, dev);

        if (addr == block) {
            offset = 0;
        } else {
            offset = addr - block;
        }

        while (read_count > 0) {
#ifdef CFG_WDT_INCLUDED
#ifndef __LINUX__
            board_wdt_ping();
#endif
#endif
            size_t block_size = flash_block_size(addr);
            size_t this_read;
            if (read_count > (block_size - offset)) {
                this_read = block_size - offset;
            } else {
                this_read = read_count;
            }
            /* Only the first block may need the offset */
            offset = 0;

            INTR_DISABLE();
            rv = (*dev->funs->flash_read)(dev, addr, ram, this_read);
            INTR_RESTORE();

            if (SYS_OK != rv) {
                break;
            }
            read_count  -= this_read;
            addr        += this_read;
            ram         += this_read;
        }
  }
  return rv;
}
#endif /* CFG_FLASH_SUPPORT_ENABLED */
