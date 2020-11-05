/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
MEMORY {
       L2CRAM : ORIGIN = 0x80000000, LENGTH = 128K
       SRAM   : ORIGIN = 0x1b000000, LENGTH = 16K
}

SECTIONS 
{
/* Could be ROM or FLASH start address */
. = reset_addr;

.text reset_addr : {
        text_start = .;
        low_mem*.o(.text*);
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
        . = ALIGN(4);
}

.ARM.exidx . : {
   *(.ARM.exidx*);
}


text_end = .;
/*
 * L2CRAM extents - 2ram, data and bss
 */
. = 0x80000000;

.data . : AT ( text_end ) {
        data_start = .;
        *(.2ram*);
        data_rel_start = .;
        *(.data.rel*);
        . = ALIGN(4);
        data_rel_end = .;
        . = ALIGN(32);
        *(EXCLUDE_FILE (*phypkgsrc.a:bcm84846_ucode.o *phypkgsrc.a:bcmi_tsc_xgxs_ucode.o) .data.*);
        data_rel2_start = .;
        *(EXCLUDE_FILE (*phypkgsrc.a:bcm84846_ucode.o *phypkgsrc.a:bcmi_tsc_xgxs_ucode.o) .data*);
        data_rel2_end = .;
        . = ALIGN(4);
        data_end = .;
} > L2CRAM 

.rel.dyn : {
  __rel_dyn_start = .;
  *(.rel*)
  __rel_dyn_end = .;
 }  > L2CRAM 
 .dynsym : {
  __dynsym_start = .;
  *(.dynsym)
 } > L2CRAM 

.bss . : {
       . = ALIGN(4);
        bss_start = .;
       *(.bss*)
       *(COMMON)
       . = ALIGN(4);
       end = .;
       bss_end = .;


} > L2CRAM 

.heap (NOLOAD) : {
       . = ALIGN(32);
       low_mem*.o(mem_heap);
       . = ALIGN(32);
       low_mem*.o(mem_stack);
       dtcm_endaddr = .;
} > L2CRAM

.packet_buf (NOLOAD) : {
   . = . + 0x40; 
   itcm_start = . ;
   . = ALIGN(32);
   packetbuf_baseaddr = .;
   *(.packet_buf)
   . = ALIGN(32);
   low_mem*.o(mem_dmaheap);
   . = ALIGN(32);
   itcm_end = .;
} > SRAM 

 

}
