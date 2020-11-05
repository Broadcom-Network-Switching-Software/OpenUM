/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
MEMORY {
       L2CRAM : ORIGIN = 0x50000000, LENGTH = 128K
       SRAM   : ORIGIN = 0x02000000, LENGTH = 32K
}

SECTIONS 
{
/* Could be ROM or FLASH start address */
. = reset_addr;

.text reset_addr : {
        text_start = .;
        low_mem*.o(.text);
        low_mem*.o(.rodata);
        *(.text*);
        *(i.*);
        *(i.__ARM_common*);
        *(.rodata*);
        *(.constdata);
        *(.conststring);
        *(.flashdata);
        *phypkgsrc.a:bcm84846_ucode.o(.data*)
        *phypkgsrc.a:bcmi_tsc_xgxs_ucode.o(.data*)
        . = ALIGN(8);
        text_end = .;
}

/*
 * L2CRAM extents - 2ram, data and bss
 */
. = 0x50000000;

.data . : AT ( text_end ) {
        data_start = .;
        *(.2ram*);
        *(EXCLUDE_FILE (*phypkgsrc.a:bcm84846_ucode.o *phypkgsrc.a:bcmi_tsc_xgxs_ucode.o) .data*);
        . = ALIGN(8);
        data_end = .;
} > L2CRAM 

data_rel_start = .;
data_rel_end = .;
__rel_dyn_start = .;

.bss . : {
        bss_start = .;
       *(.bss*)
       *(COMMON)
       . = ALIGN(8);
       end = .;
       bss_end = .;
       dtcm_endaddr = .;
} > L2CRAM 

.packet_buf (NOLOAD) : {
   itcm_start = . ;
   . = . + 0x40;
   . = ALIGN(32);
   low_mem*.o(mem_stack);
   . = ALIGN(32);
   packetbuf_baseaddr = .;
   *(.packet_buf)
   low_mem*.o(mem_heap);
   . = ALIGN(32);
   low_mem*.o(mem_dmaheap);
   . = ALIGN(32);
   itcm_end = .;
} > SRAM 

}
