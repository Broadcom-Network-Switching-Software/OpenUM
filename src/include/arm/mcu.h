/*
 * $Id: mcu.h,v 1.4 Broadcom SDK $
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef _MCU_H_
#define _MCU_H_

/* For declaring re-usable API functions */
#define APIFUNC(_fn)    _fn
#define REENTRANT
#define USE_INTERNAL_RAM

/* For re-usable static function  */
#define APISTATIC

/* Normal static attribute */
#define STATICFN        static
#define STATIC          static
#define STATICCBK       static

/* Add "const" declaration to web related generated files from sspgen.pl
 * so those will be put in .rodata section and reduce RAM usage.
 */
#define RES_CONST_DECL const

#define XDATA
#define CODE

/* System register read/write */
#define SYS_REG_READ8(reg)       \
            (*(volatile uint8 *)(reg))
#define SYS_REG_WRITE8(reg,val)  \
            do { *(volatile uint8 *)(reg) = (val); } while(0)
#define SYS_REG_AND8(reg,val)    \
            SYS_REG_WRITE8((reg), SYS_REG_READ8(reg) & (val))
#define SYS_REG_OR8(reg,val)     \
            SYS_REG_WRITE8((reg), SYS_REG_READ8(reg) | (val))
#define SYS_REG_READ16(reg)      \
            (*(volatile uint16 *)(reg))
#define SYS_REG_WRITE16(reg,val) \
            do { *(volatile uint16 *)(reg) = (val); } while(0)
#define SYS_REG_AND16(reg,val)   \
            SYS_REG_WRITE16((reg), SYS_REG_READ16(reg) & (val))
#define SYS_REG_OR16(reg,val)    \
            SYS_REG_WRITE16((reg), SYS_REG_READ16(reg) | (val))
#define SYS_REG_READ32(reg)      \
            (*(volatile uint32 *)(reg))
#define SYS_REG_WRITE32(reg,val) \
            do { *(volatile uint32 *)(reg) = (val); } while(0)
#define SYS_REG_AND32(reg,val)   \
            SYS_REG_WRITE32((reg), SYS_REG_READ32(reg) & (val))
#define SYS_REG_OR32(reg,val)    \
            SYS_REG_WRITE32((reg), SYS_REG_READ32(reg) | (val))

/* Pointer to address conversion for hyper space (flash space) */
typedef uint32 hsaddr_t;
#define DATAPTR2HSADDR(x)   ((hsaddr_t)(x))
#define HSADDR2DATAPTR(x)   ((uint8 *)((hsaddr_t)(x)))

/* Pointer to address conversion for memory space (data space) */
typedef uint32 msaddr_t;
#define DATAPTR2MSADDR(x)   ((msaddr_t)(x))
#define MSADDR2DATAPTR(x)   ((uint8 XDATA *)((msaddr_t)(x)))

/* Type of system ticks */
typedef uint32 tick_t;

#endif /* _MCU_H_ */
