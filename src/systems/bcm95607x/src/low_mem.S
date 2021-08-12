/*
 * $Id: low_mem.S,v 1.6 Broadcom SDK $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 *
 * File:    low_mem.S
 * Purpose: H/W exception handling
 */

        .syntax unified

#include "thumb.h"
#include "board.h"
#include "arm.h"
#include "bsp_config.h"
#include "config.h"

#define STACK(_b_, _size_)                              \
        .section mem_stack                              ;\
        .balign 16                                      ;\
        .global _b_ ## _base                            ;\
_b_  ## _base:                                          ;\
        .rept _size_                                    ;\
        .byte   0                                       ;\
        .endr                                           ;\
        .balign 16                                      ;\
        .global _b_                                     ;\
_b_:

#define HEAP(_b_, _size_)                               \
        .section mem_heap                               ;\
        .balign 16                                      ;\
        .global _b_ ## _base                            ;\
_b_  ## _base:                                          ;\
        .rept _size_                                    ;\
        .byte   0                                       ;\
        .endr                                           ;\
        .balign 16                                      ;\
_b_  ## _end:


#define DMA_HEAP(_b_, _size_)                           \
        .section mem_dmaheap                            ;\
        .balign 16                                      ;\
        .global _b_ ## _base                            ;\
_b_  ## _base:                                          ;\
        .rept _size_                                    ;\
        .byte   0                                       ;\
        .endr                                           ;\
        .balign 16                                      ;\
_b_  ## _end:

#define UMINFO_START()                                  \
        .ascii "UM"                                     ;\
        .byte 0xA                                       ;

#define UMINFO_WORD(VARIABLE, VALUE)                    \
        .ascii #VARIABLE##"_W"                          ;\
        .byte 0xA                                       ;\
        .word  VALUE                                    ;

#define UMINFO_STRING(VARIABLE, VALUE)                  \
        .ascii #VARIABLE##"_S"                          ;\
        .byte 0xA                                       ;\
        .ascii VALUE                                    ;\
        .byte 0xA

#define UMINFO_END()                                    \
        .ascii "END"                                    ;\
        .byte 0xA

#define REF_GEN(sym)                                    \
        .global sym ## _ref                             ;\
        .global sym                                     ;\
sym ## _ref:                                            ;\
        .word sym


#define EXCEPTION_MODE                                  \
        .code   32

#define SA_SP (13*4)
#define SA_R0 (0*4)

        EXCEPTION_MODE
        .section ".text","ax"
        .global reset

//  Fixed-location H/W exception vectors
reset:
        b     start
#ifdef __BOOTLOADER__
        b     undef_instr
        b     sw_rupt
        b     prefetch_abort
        b     data_abort
        b     reset
        b     irq
        b     fiq
#else
        /* To make vector.o linked in, should not run to here */
        b     reset_v
#endif
        EXCEPTION_MODE

        /* Firmware version , board name and flash layout */
        .align  (6)
        UMINFO_START()
        UMINFO_STRING(CFG_BOARDNAME, CFG_BOARDNAME);
        UMINFO_WORD(CFE_VER, (CFE_VER_MAJOR << 24) | (CFE_VER_MINOR << 16) | (CFE_VER_BUILD << 8));
        UMINFO_STRING(target, target);
        UMINFO_WORD(BOARD_LOADER_ADDR, BOARD_LOADER_ADDR);
        UMINFO_WORD(BOARD_FIRMWARE_ADDR, BOARD_FIRMWARE_ADDR);
        UMINFO_WORD(BOARD_SECONDARY_FIRMWARE_ADDR, BOARD_SECONDARY_FIRMWARE_ADDR);
        UMINFO_WORD(CFG_FACTORY_CONFIG_BASE, CFG_FACTORY_CONFIG_BASE);
        UMINFO_WORD(CFG_FACTORY_CONFIG_OFFSET, CFG_FACTORY_CONFIG_OFFSET);
        UMINFO_WORD(CFG_CONFIG_BASE, CFG_CONFIG_BASE);
        UMINFO_WORD(CFG_CONFIG_OFFSET, CFG_CONFIG_OFFSET);
        UMINFO_WORD(CFG_CONFIG_SIZE, CFG_CONFIG_SIZE);
        UMINFO_WORD(CFG_FACTORY_CONFIG_BASE, CFG_FACTORY_CONFIG_BASE);
        UMINFO_WORD(CFG_FACTORY_CONFIG_OFFSET, CFG_FACTORY_CONFIG_OFFSET);
        UMINFO_END();
        .align (6)
        /* End of firmware version , board name and flash layout */

// System reset
        .global  start
start:
        /* preserve [r0-r3] from here to call to mos_boot (except for MASK_ROM) */
        mrs     r4,cpsr
        bic     r4,r4,#0x1f             // Clear mode
        orr     r4,r4,#0xd3             // SVC mode, suppress rupts
        msr     cpsr_c,r4

        // Make sure that the E bit is set appropriately
        SET_ENDIAN
       /* Disable MPU */
        mrc p15, 0, r0, c1, c0, 0   /* Read CP15 Control Register */
        bic r0, r0, #0x01           /* Set M bit to enable MPU */
        DSB                         /* Ensure all previous loads/stores have completed */
        mcr p15, 0, r0, c1, c0, 0   /* Write CP15 Control Register */
        ISB                         /* Ensure subsequent insts execute wrt new MPU settings */


        // Init the CPU
        mrc     p15,0,r5,c1,c0, 0       // Read SCTLR
        ldr     r6,p15clr
        bic     r5,r5,r6                // Clear RE+VE+NMFI+endian+IC+C
        orr     r5,r5,#0x0800           // Branch prediction enable

#if defined(DCACHE) || defined(ICACHE)
        // Disable the caches
        DSB
        bic     r5,r5,#4                // DCACHE disable
        bic     r5,r5,#0x1000           // ICACHE disable
#endif

        ASM_DCACHE_INVALIDATE
        ASM_BRANCH_PREDICT_CLEAR

clear_all_memory:
        /* Clear all the ATCM */
        ldr     r5,=CFG_ATCM_START
        ldr     r6,=CFG_ATCM_END
        cmp     r6,r5
        beq     2f                      /* skip if region is empty */

        mov     r7,#0

1:      str     r7,[r5],#4
        cmp     r6,r5
        bhi     1b
2:
        /* Clear all the BTCM */
        ldr     r5,=CFG_BTCM_START
        ldr     r6,=CFG_BTCM_END
        cmp     r6,r5
        beq     2f                      /* skip if region is empty */

        mov     r7,#0

1:      str     r7,[r5],#4
        cmp     r6,r5
        bhi     1b
2:
        /* Clear all the SRAM */
        ldr     r5,=CFG_SRAM_START
        ldr     r6,=CFG_SRAM_END
        cmp     r6,r5
        beq     2f                      /* skip if region is empty */

        mov     r7,#0

1:      str     r7,[r5],#4
        cmp     r6,r5
        bhi     1b
2:

dtcm_setup:
        adr     r0, dtcm_setup
        ldr     r1, =dtcm_setup
        sub     r9, r0, r1
        ldr     r5,text_end_ref
        add     r5, r5, r9
        ldr     r6,data_start_ref
        ldr     r7,bss_start_ref
        sub     r7,r6               /* init data length */
        add     r7,r5               /* end of init data in ROM */

        cmp     r7,r5
        beq     2f                  /* skip if region is empty */

1:
        ldr     r4,[r5],#4
        str     r4,[r6],#4
        cmp     r7,r5
        bhi     1b
2:

        /* The below fix_rel related codes are dummy because r9 will be 0 */
        cmp     r9, #0
        beq     4f
        ldr     r6, data_rel_start_ref
        ldr     r7, data_rel_end_ref
        cmp     r6, r7
        beq     2f
        bl      fix_rel
2:
        ldr     r6, data_end_ref
        ldr     r7, __rel_dyn_start_ref
        cmp     r6, r7
        beq     3f
        bl      fix_rel

3:
        ldr     r6, data_rel2_start_ref
        ldr     r7, data_rel2_end_ref
        cmp     r6, r7
        beq     4f
        bl      fix_rel

4:

        /* Clear other memory */
        ldr     sp, boot_stack_ref
        /* Enable MPU */
        bl      mpu_init

#ifndef __BOOTLOADER__
        /* Copy vector related code/data to ATCM */
vect_cp_start:
        adr     r4, vect_cp_start
        ldr     r5, =vect_cp_start
        /* Store addr offset in r4 */
        sub     r4, r4, r5
        ldr     r0, vector_start_ref
        add     r0, r0, r4
        ldr     r1, vector_size_ref
        add     r1, r1, r0
        ldr     r2, vector_ram_start_ref
vect_cp_loop:
        ldr     r3, [r0], #4
        str     r3, [r2], #4
        cmp     r0, r1
        bne     vect_cp_loop
#endif

        // Set the V bit to map the vectors appropriately
        mrc     p15,0,r5,c1,c0, 0       // Read SCTLR
#ifdef __BOOTLOADER__
        orr     r5,r5,0x2000            // Vectors map to 0xffff0000
#else
        bic     r5,r5,0x2000            // Vectors map to 0x0
#endif
        mcr     p15,0,r5,c1,c0,0        // Write SCTLR
        ISB

        /* Enable cache */
        mrc     p15,0,r4,c1,c0, 0       // Read SCTLR
#if defined(DCACHE) || defined(ICACHE)
#ifdef DCACHE
        orr     r4,r4,#4                // DCACHE enable
#endif
#ifdef ICACHE
        orr     r4,r4,#0x1000           // ICACHE enable
#endif
#endif
        DSB
        ASM_DCACHE_INVALIDATE
        ASM_ICACHE_INVALIDATE
        mcr     p15,0,r4,c1,c0,0       // Write SCTLR
        ISB

        .global mos_boot_call;
mos_boot_call:
        ldr     r9, data_end_ref       // load GOT into r9
        blx     main                   // Never returns
        str     r0,[r1]
        ldr     r0,[r1]

// Reset data fields
p15clr:         .word   0x4b001004
#ifdef BIG_ENDIAN
p15big:         .word   0x82000000
#endif
        REF_GEN(bss_start)
        REF_GEN(bss_end)
        REF_GEN(itcm_start)
        REF_GEN(itcm_end)
        REF_GEN(text_end)
        REF_GEN(data_start)
        REF_GEN(data_end)
        REF_GEN(__rel_dyn_start)
        REF_GEN(data_rel_start)
        REF_GEN(data_rel_end)
        REF_GEN(data_rel2_start)
        REF_GEN(data_rel2_end)
        REF_GEN(sram_start)
        REF_GEN(sram_end)
#ifndef __BOOTLOADER__
        REF_GEN(vector_start)
        REF_GEN(vector_size)
        REF_GEN(vector_ram_start)
#endif

        .global mem_heapstart;
mem_heapstart: .word mem_heapstart_base
        .global mem_dmaheapstart;
mem_dmaheapstart: .word mem_dmaheapstart_base
        REF_GEN(dtcm_endaddr)
        REF_GEN(boot_stack)

#ifdef __BOOTLOADER__
// Undefined instruction
        EXCEPTION_MODE
        .global  undef_instr
undef_instr:
        mov     sp, #1
        b       exception

// Software interrupt
        EXCEPTION_MODE
        .global  sw_rupt
sw_rupt:
        mov     sp,#2
        b       exception

// Instruction prefetch abort
        EXCEPTION_MODE
        .global  prefetch_abort
prefetch_abort:
        mov     sp,#3
        b       exception

// Data load abort
        EXCEPTION_MODE
        .global  data_abort
data_abort:
        mov     sp,#4
        b       exception

// Unified exception handling
        EXCEPTION_MODE
exception:

        mov     r0, sp
        mov     r1, lr
        ldr     sp, exception_stack_ref
        bl      mos_exception           // Never returns

        REF_GEN(exception_stack)

        .code        32
        .global mos_low_power_mode
mos_low_power_mode:
        cpsID   if                      // FIQ and IRQ off
        wfi

        str     lr,mos_wfi_irq          // Set flag for IRQ

        mov     r0,#0                   // Default RC from low_power_mode

        cpsIE   if                      // FIQ and IRQ back on


        // IRQ and/or FIQ happens here.  The IRQ handler changes R0 and
        // turns IRQs back off

        cpsID   i                       // IRQ back off

        mov     r1, #0
        str     r1,mos_wfi_irq          // Clear flag

        bx      lr


        .global mos_idle_poll
mos_idle_poll:
        mov     r0,#0                   // Default RC from low_power_mode
        str     lr,mos_wfi_irq          // Set flag for IRQ

        cpsIE   i                       // IRQ on (FIQ already on)

mos_idle_loop:
        cmp     r0,#0                   // See if IRQ or FIQ changed it
        beq     mos_idle_loop

        // An IRQ or FIQ happened

        cpsID   i                       // IRQ back off

        mov     r1, #0
        str     r1,mos_wfi_irq          // Clear flag

        bx      lr

        .global mos_wfi_irq
mos_wfi_irq:    .word 0


// IRQ
        EXCEPTION_MODE
        .global  irq
irq:
        b       .
        .pool

// FIQ
        EXCEPTION_MODE
        .global  fiq
fiq:
        // The only FIQ is the timer tick

        ldr     sp, FIQStack_ref

/* CR4 has it's own IP register, so no need to save/restore */
#ifdef CORTEX_R4
#define IPREG
#else
#define IPREG   ip,
#endif

        push    {r0-r3,IPREG lr}       // Save some registers
        subs    r0,lr,#4            // pass lr to mos_fiq_handler

        bl      mos_fiq_handler

        pop     {r0-r3,IPREG lr}           // Restore and return
        ldr     sp, mos_wfi_irq         // Check if thread will handle
        cmp     sp, #0
        subseq  pc,lr,#4
        orr     r0, r0, #2              // Set FIQ bit in WFI RC
        subs    pc,lr,#4
        REF_GEN(FIQStack)

#endif /* __BOOTLOADER__ */

        .pool

        .syntax unified
        .code   32
        .section ".text","ax"

LEAF(fix_rel)
        ldr     r3,=fix_rel  /* Fix locations starting with 0xF0xx-xxxx */
        adr     r4,fix_rel
        sub     r5, r4, r3    /* r5 is the offset */
        cmp     r5, #0
        bne     start_fix
        bx      lr
start_fix:
        ldr     r4,=reset
        ldr     r3,=text_end
fixloop:
        ldr     r0, [r6]
        cmp     r0, r4
        bcc     fixnext
        cmp     r0, r3
        bhi     fixnext
        add     r0,r0,r5
        str     r0,[r6]
fixnext:
        add     r6, r6, #4
        cmp     r6, r7
        blo     fixloop
        bx      lr
END(fix_rel)
/*
 * Initialize MPU with 6 Data regions
 * Region-0: 0x0000_0000 - 0xffff_ffff Default(all 4G): Execute never, Full Access, Stronly Ordered
 * Region-1: 0x0000_0000 - 0x0003_ffff ATCM: Execute, Full Access, UNCACHABLE (256KB)
 * Region-2: 0x0004_0000 - 0x0007_ffff BTCM: Execute, Full Access, UNCACHABLE (256KB)
 * Region-3: 0x0120_0000 - 0x012F_FFFF SRAM: Execute, Full Access, WB/WA      (1MB)
 * Region-4: 0x1C00_0000 - 0x1CFF_FFFF QSPI flash: Execute, Read-only, L1 cacheable (64MB)
 * Region-5: 0xfffB_0000 - 0xffff_ffff ROM: Execute, Read-only, L1 cacheable  (512K)
 *
 * Changes:
 *  r1, r2
 * Note: Only normal and non-shared memory can be cached
 */
LEAF(mpu_init)
        ldr r4,=_mpudcfg      /* mpu cfg table */
        add r4, r4, r9
        eor r0,r0             /* DRegion index */
1:
        ldmia   r4!, {r1,r2,r3}
        cmp r1,#0             /* Last entry has size=0 */
        beq mpu_enable
        mcr p15,0,r0,c6,c2,0  /* Select DRegion */
        mcr p15,0,r1,c6,c1,2  /* Set size:enable */
        mcr p15,0,r2,c6,c1,0  /* Set base addr */
        mcr p15,0,r3,c6,c1,4  /* set C,B,TEX[2:0],AP */
        add r0,#1
        b   1b

mpu_enable:
        /*  Enable MPU */
        mcr p15, 0, r0, c7, c5, 6   /* Invalidate branch predictor array */
        mcr p15, 0, r0, c7, c5, 4   /* Flush prefetch buffer */
        mrc p15, 0, r0, c1, c0, 0   /* Read CP15 Control Register */
        orr r0, r0, #0x01           /* Set M bit to enable MPU */
        DSB                         /* Ensure all previous loads/stores have completed */
        mcr p15, 0, r0, c1, c0, 0   /* Write CP15 Control Register */
        ISB                         /* Ensure subsequent insts execute wrt new MPU settings */
        bx  lr

        /* MPU DRegion config table */
        .align 4
_mpudcfg:
        .word   MPU_SIZE_4G   /* DRegion-0 size(4GB) */
        .word   0x00000000    /* DRegion-0 base_addr(0x0000_0000) */
        .word   0x300         /* XN:0 C:0 B:0 TEX[2:0]:0 AP:0x3 */
        .word   MPU_SIZE_256K /* DRegion-1 size(256KB) */
        .word   0x00000000    /* DRegion-1 base_addr(0x0000_0000) */
        .word   0x30C         /* XN:0 S:1 C:0 B:0 TEX[2:0]:1 AP:0x3 */
        .word   MPU_SIZE_256K /* DRegion-2 size(256KB) */
        .word   0x00040000    /* DRegion-2  base_addr(0x0004_0000) */
        .word   0x30C         /* XN:0 S:1 C:0 B:0 TEX[2:0]:1 AP:0x3 */
        .word   MPU_SIZE_1M   /* DRegion-3  size(1MB) */
        .word   0x01200000    /* DRegion-3  base_addr(0x0120_0000) */
        .word   0x325         /* XN:0 S:1 C:0 B:1 TEX[2:0]:100 AP:0x3 */
        .word   MPU_SIZE_64M  /* DRegion-4 size(64MB) */
        .word   0x1C000000    /* DRegion-4 base_addr(0x1C00_0000) */
        .word   0x322         /* XN:0 C:0 B:1 TEX[2:0]:100 AP:0x6 */
        .word   MPU_SIZE_512K /* DRegion-5 size(512K) */
        .word   0xFFFB0000    /* DRegion-4 base_addr(0xFFFB_0000) */
        .word   0x325         /* XN:0 C:0 B:1 TEX[2:0]:100 AP:0x6 */
        .word   0             /* DRegion table  END */
        .word   0
        .word   0
END(mpu_init)

        .thumb
        .thumb_func

//
//
//   BSS section
//
//

    .section .bss

STACK(boot_stack, CFG_STACK_SIZE)
STACK(exception_stack, 128)
STACK(FIQStack, 128)
STACK(IRQStack, CFG_INTR_STACK_SIZE)
HEAP(mem_heapstart, (CFG_HEAP_SIZE * 1024));
DMA_HEAP(mem_dmaheapstart, CFG_DMA_HEAP_SIZE);


