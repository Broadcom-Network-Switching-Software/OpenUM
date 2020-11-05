/*
 * 
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */

#ifndef _TYPES_H_
#define _TYPES_H_
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

typedef signed char         int8;
typedef signed short        int16;
typedef signed long         int32;

typedef uint8               BOOL;
#define TRUE                (1)
#define FALSE               (0)

#ifndef NULL
#define NULL                (0)
#endif

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int size_t;
#endif

/**
 * 8 bit datatype
 *
 * This typedef defines the 8-bit type used throughout uIP.
 *
 * \hideinitializer
 */
typedef uint8 u8_t;
typedef uint8 uint8_t;
typedef char int8_t;

/**
 * 16 bit datatype
 *
 * This typedef defines the 16-bit type used throughout uIP.
 *
 * \hideinitializer
 */
typedef uint16 u16_t;
typedef uint16 uint16_t;

/**
 * 32 bit datatype
 *
 * This typedef defines the 32-bit type used throughout uIP.
 *
 * \hideinitializer
 */
typedef uint32 u32_t;
typedef uint32 uint32_t;

typedef int int32_t;

#ifdef PTRS_ARE_64BITS
#define PTR_TO_INT(x)           ((uint32)(((uint64)(x))&0xFFFFFFFF))
#define PTR_HI_TO_INT(x)        ((uint32)((((uint64)(x))>>32)&0xFFFFFFFF))
#else
#define PTR_TO_INT(x)           ((uint32)(x))
#define PTR_HI_TO_INT(x)        (0)
#endif

#define INT_TO_PTR(x)        ((void *)((sal_vaddr_t)(x)))

#define PTR_TO_UINTPTR(x)       ((sal_vaddr_t)(x))
#define UINTPTR_TO_PTR(x)       ((void *)(x))


#define UNREFERENCED_PARAMETER(x)   do { x = x; } while(0)

#define min(a,b) ((a) < (b)? (a) : (b))
#define max(a,b) ((a) > (b)? (a) : (b))

#endif /* _TYPES_H_ */
