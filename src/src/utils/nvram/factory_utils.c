/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "utils/factory.h"

#if (CFG_FLASH_SUPPORT_ENABLED && defined(CFG_FACTORY_CONFIG_INCLUDED))

sys_error_t
factory_config_get(factory_config_t *pcfg)
{
    factory_config_t cfg;
    
    /*
     * Check block magic
     */
    flash_read((hsaddr_t)FACTORY_CONFIG_BASE_ADDR+FACTORY_CONFIG_OFFSET,
               (void *)&cfg, sizeof(factory_config_t)); 

    if (cfg.magic != FACTORY_CONFIG_MAGIC) {
        /* Block magic must be matched */
        return SYS_ERR;
    }

    if (pcfg) {
        sal_memcpy(pcfg, &cfg, sizeof(factory_config_t));
    }

    return SYS_OK;
}

sys_error_t
factory_config_set(factory_config_t *cfg)
{
    factory_config_t *pcfg;
    uint8 *buf;
    sys_error_t rv;
    size_t  block_size;

    block_size = flash_block_size(BOARD_FIRMWARE_ADDR);
    /* Allocate and copy block data */
    buf = sal_malloc(block_size);
    if (!buf) {
#if CFG_CONSOLE_ENABLED
        sal_printf("factory_config_set: malloc failed!\n");
#endif
        return SYS_ERR_OUT_OF_RESOURCE;
    }

    flash_read((hsaddr_t)FACTORY_CONFIG_BASE_ADDR, buf, block_size);

    pcfg = (factory_config_t *)(buf+FACTORY_CONFIG_OFFSET);

    pcfg->magic = FACTORY_CONFIG_MAGIC;

    sal_memcpy(pcfg->mac, cfg->mac, 6);

#ifdef CFG_PRODUCT_REGISTRATION_INCLUDED
    pcfg->serial_num_magic = cfg->serial_num_magic;
    sal_memcpy(pcfg->serial_num, cfg->serial_num, 20);
#endif /* CFG_PRODUCT_REGISTRATION_INCLUDED */    

    rv = flash_erase((hsaddr_t)FACTORY_CONFIG_BASE_ADDR, block_size);
    if (rv != SYS_OK) {
#if CFG_CONSOLE_ENABLED
        sal_printf("factory_config_set: failed to erase region 0x%x\n",
                   (hsaddr_t)FACTORY_CONFIG_BASE_ADDR);
#endif
        goto done;
    }

    rv = flash_program((hsaddr_t)FACTORY_CONFIG_BASE_ADDR,
                        (const void *)buf,
                        block_size);

    if (rv != SYS_OK) {
#if CFG_CONSOLE_ENABLED
        sal_printf("factory_config_set: failed to program region 0x%x\n",
                   (hsaddr_t)FACTORY_CONFIG_BASE_ADDR);
#endif
    }
done:
    sal_free(buf);
    return rv;
}

#endif /* CFG_FLASH_SUPPORT_ENABLED  && defined(CFG_FACTORY_CONFIG_INCLUDED) */
