/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#include "system.h"
#include "arm.h"

extern unsigned int app_text_start;
extern int app_text_length;
extern const unsigned char app_text[];

extern unsigned int app_text2_start;
extern int app_text2_length;
extern const unsigned char app_text2[];

extern unsigned int app_data_start;
extern int app_data_length;
extern const unsigned char app_data[];

/*@api
 * mos_boot
 *
 * @brief
 * Copy the ROM text/data to memory and execute
 *
 * @param=arg - priority to start any additional tasks at
 * @returns void
 *
 * @desc
 */
int main(void)
{
    uint32 *dst, *src;
    int length;
    void (*text_base)() = (void (*)())(app_text_start);

    /* copy the text out */
    dst = (uint32 *)app_text_start;
    src = (uint32 *)app_text;
    length = 0;
    while (length != app_text_length) {
        *dst++ = *src++;
        length += 4;
    }

    dst = (uint32 *)app_text2_start;
    src = (uint32 *)app_text2;
    length = 0;
    while (length != app_text2_length) {
        *dst++ = *src++;
        length += 4;
    }

    /* copy the data out */
    dst = (uint32 *)app_data_start;
    src = (uint32 *)app_data;
    length = 0;
    while (length != app_data_length) {
        *dst++ = *src++;
        length += 4;
    }
    (*text_base)();
}

/* Dummy entries for linker */
void mos_exception(void) { }
void mos_fiq_handler(void) { }
#if 0
void mos_irq_handler(void) { }
void mos_rupt_yield(void) { }
void mos_lock_push_irq(void) { }
void mos_lock_pop_irq(void) { }
#endif