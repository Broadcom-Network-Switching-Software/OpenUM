/*! \file exchandler.h
 *
 * Exception dump data structure
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef __EXCHANDLER_H__
#define __EXCHANDLER_H__

/*! Memory base for exception infomation dump */
#define CFG_EXCEPTION_DUMP_BASE 0x10000

/*! The first entry value of exception dump - EXC. */
#define EXC_HEADER  0x4558432E

#ifndef __ASSEMBLER__

/*!
 * \brief Exception info dump structure.
 */
typedef struct exc_core_dump {
    /*! Exception dump header, should be 0x4558432E (EXC.) */
    uint32  exc_header;

    /*! Exception type */
    uint32  exc_type;

    /*! CPU register 0 - 12 */
    uint32  regs[13];

    /*! Stack pointer register (r13) */
    uint32  r_sp;

    /*! Link register (r14) */
    uint32  r_lr;

    /*! Program counter (r15) */
    uint32  r_pc;

    /*! Program status register */
    uint32  cpsr;

    /*! Data fault status register */
    uint32  dfsr;

    /*! Data fault address register */
    uint32  dfar;

    /*! Instruction fault status register */
    uint32  ifsr;

    /*! Instruction fault address register */
    uint32  ifar;

    /*! Auxiliary data fault status register */
    uint32  adfsr;

    /*! Auxiliary instruction fault status register */
    uint32  aifsr;
} exc_core_dump_t;

/*!
 * \brief Exception types.
 */
enum exc_types {
    /*! Reset exception */
    EXC_RESET = 0,

    /*! Undefined instruction exception */
    EXC_UNDEF_INSTR,

    /*! Software interrupt exception */
    EXC_SW_INTR,

    /*! Instruction prefetch abort exception */
    EXC_PREFETCH_ABORT,

    /*! Data access abort exception */
    EXC_DATA_ABORT,

    /*! Unhandled IRQ */
    EXC_UNHANDLED_IRQ = 6
};

/*!
 * \brief Exception handling routine
 *
 * \param [in] exc_dump_base memory base address for exception dump
 *
 * \return never
 */
extern void
exc_handler(uint32 exc_dump_base);

#endif /* __ASSEMBLER__ */

#endif /* __EXCHANDLER_H__ */
