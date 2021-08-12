/*
 * $Id: flash_table.h,v 1.5 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */


extern struct flash_dev_funs mx25l_funs;

/* Flash device flags */
#define FLAG_FSR_POLL   (1 << (0))

/* Block information */

/* 256Mb flash memory */
//static flash_block_info_t s25fl256_info[2] = {{ 4096, 32 }, { 65536, 510 }};

/*  512Mb flash memory, use 4K subsector for erase */
static flash_block_info_t uniform_4K_512Mb_info = { 4096, 16384 };

/* 256Mb flash memory, use 4K subsector for erase */
static flash_block_info_t uniform_4K_256Mb_info = { 4096, 8192 };

/* 128Mb flash memory, use 4K subsector for erase */
static flash_block_info_t uniform_4K_128Mb_info = { 4096, 4096 };

/* 64Mb flash memory, use 4K sector for erase */
static flash_block_info_t uniform_4K_64Mb_info = { 4096, 2048 };

/* SPANSION */
#define MANUFACTURER_ID_SPANSION     0x01
/* MXIC (MACRONIX) */
#define MANUFACTURER_ID_MACRONIX     0xC2
/* SST (acquired by Microchip)*/
#define MANUFACTURER_ID_SST          0xBF
/* Winbond*/
#define MANUFACTURER_ID_WINBOND      0xEF
/* Fund by Intel, STM and then acquired by Micron */
#define MANUFACTURER_ID_NUMONYX      0x89     
/* Micron */
#define MANUFACTURER_ID_MICRON       0x20     
/* EON */
#define MANUFACTURER_ID_EON          0x1C
/* UNKNOW flash type */
#define MANUFACTURER_ID_UNKNOWN      0x00

const flash_dev_t n25q256_dev =  { {MANUFACTURER_ID_MICRON,   0xba, 0x19, 0x00}, {0xFF, 0xFF, 0xFF, 0x00}, &mx25l_funs, FLAG_FSR_POLL, CFG_FLASH_START_ADDRESS, 0, 1, &uniform_4K_256Mb_info, "N25Q256", NULL};
const flash_dev_t w25q64cv_dev = { {MANUFACTURER_ID_WINBOND,  0x40, 0x17, 0x0},  {0xFF, 0xFF, 0xFF, 0x0}, &mx25l_funs, 0, CFG_FLASH_START_ADDRESS, 0, 1, &uniform_4K_64Mb_info, "W25Q64CV", NULL};


const flash_dev_t flash_dev_support_table[] = {
    { {MANUFACTURER_ID_WINBOND,  0x40, 0x17, 0x0},  {0xFF, 0xFF, 0xFF, 0x0}, &mx25l_funs, 0, CFG_FLASH_START_ADDRESS, 0, 1, &uniform_4K_64Mb_info, "W25Q64CV", NULL}, 
    { {MANUFACTURER_ID_MICRON,   0xba, 0x19, 0x00}, {0xFF, 0xFF, 0xFF, 0x00}, &mx25l_funs, FLAG_FSR_POLL, CFG_FLASH_START_ADDRESS, 0, 1, &uniform_4K_256Mb_info, "N25Q256", NULL}, 
    { {MANUFACTURER_ID_MICRON,   0xba, 0x20, 0x00}, {0xFF, 0xFF, 0xFF, 0x00}, &mx25l_funs, FLAG_FSR_POLL, CFG_FLASH_START_ADDRESS, 0, 1, &uniform_4K_512Mb_info, "N25Q512", NULL},    /* HR2 is not support N25Q512 */ 
    { {MANUFACTURER_ID_MACRONIX, 0x20, 0x18, 0x0},  {0xFF, 0xFF, 0xFF, 0x0}, &mx25l_funs, 0, CFG_FLASH_START_ADDRESS, 0, 1, &uniform_4K_128Mb_info, "MX25L128", NULL},     
    { {MANUFACTURER_ID_MACRONIX, 0x20, 0x19, 0x0},  {0xFF, 0xFF, 0xFF, 0x0}, &mx25l_funs, 0, CFG_FLASH_START_ADDRESS, 0, 1, &uniform_4K_256Mb_info, "MX25L256", NULL}, 
/*    { {MANUFACTURER_ID_MACRONIX, 0x20, 0x20, 0x0},  {0xFF, 0xFF, 0xFF, 0x0}, &mx25l_funs, 0, CFG_FLASH_START_ADDRESS, 0, 1, &uniform_4K_512Mb_info, "MX25L512", NULL},     */
    { {MANUFACTURER_ID_UNKNOWN,  0x0, 0x0, 0x0}, {0x00, 0x0, 0x0, 0x0}, &mx25l_funs, 0, CFG_FLASH_START_ADDRESS, 0, 1, &uniform_4K_64Mb_info, "Unknown Flash", NULL}
};


