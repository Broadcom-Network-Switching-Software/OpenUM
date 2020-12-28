/*
 * $Id: phymod_custom_config.h,v 1.1 Broadcom SDK $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * System interface definitions for Switch SDK
 */

#ifndef __PHYMOD_CUSTOM_CONFIG_H__
#define __PHYMOD_CUSTOM_CONFIG_H__

#include <shared/bsl.h>
#include <shared/error.h>

#include <sal/core/libc.h>
#include <sal/core/alloc.h>
#ifndef PHYMOD_CONFIG_INCLUDE_CHIP_SYMBOLS
#define PHYMOD_CONFIG_INCLUDE_CHIP_SYMBOLS 0
#endif

#define PHYMOD_DEBUG_ERROR(stuff_) \
    LOG_ERROR(BSL_LS_SOC_PHYMOD, stuff_)

#define PHYMOD_DEBUG_VERBOSE(stuff_) \
    LOG_VERBOSE(BSL_LS_SOC_PHYMOD, stuff_)

#define PHYMOD_DEBUG_WARN(stuff_) \
    LOG_WARN(BSL_LS_SOC_PHYMOD, stuff_)

#define PHYMOD_DIAG_OUT(stuff_) \
    cli_out stuff_

/* Do not map directly to SAL function */
#define PHYMOD_USLEEP       sal_usleep
#define PHYMOD_SLEEP        sal_sleep_sec
#define PHYMOD_STRCMP       sal_strcmp
#define PHYMOD_MEMSET       sal_memset
#define PHYMOD_MEMCPY       sal_memcpy
#define PHYMOD_FREE         sal_free

/* These functions map directly to SAL functions */
#define strncpy             sal_strncpy
#define memset              sal_memset
#define memcpy              sal_memcpy
#define strlen              sal_strlen
#define strtoul             sal_strtoul
#define PHYMOD_STRLEN       sal_strlen
#define PHYMOD_STRSTR       sal_strstr
#define PHYMOD_STRCHR       sal_strchr
#define PHYMOD_STRNCMP      sal_strncmp
#define PHYMOD_SNPRINTF     sal_snprintf
#define PHYMOD_SPRINTF      sal_sprintf
#define PHYMOD_STRCAT       sal_strcat
#define PHYMOD_STRNCAT      sal_strncat
#define PHYMOD_STRCPY       sal_strcpy
#define PHYMOD_STRNCPY      sal_strncpy
#define PHYMOD_ATOI         sal_atoi
#define PHYMOD_TIME_USECS   sal_time_usecs
#define PHYMOD_SRAND        sal_srand
#define PHYMOD_RAND         sal_rand
#define PHYMOD_STRTOUL      sal_strtoul

#define PHYMOD_SPL          phymod_spl
#define PHYMOD_SPLHI        phymod_splhi

#if defined(PTRS_ARE_64BITS)
#define PHYMOD_MALLOC       phymod_alloc
static inline void *phymod_alloc(size_t size, char *descr)
{
    return sal_alloc((unsigned int)size, descr);
}
#else
#define PHYMOD_MALLOC       sal_alloc
#endif

static inline int phymod_spl(int level)
{
    return 0;
}

static inline int phymod_splhi(void)
{
    return 0;
}

#include <sal/appl/io.h>
#define PHYMOD_PRINTF   sal_printf

/* Use SDK-versions of stdint types */
#define PHYMOD_CONFIG_DEFINE_UINT8_T    0
#define uint8_t uint8
#define PHYMOD_CONFIG_DEFINE_UINT16_T   0
#define uint16_t uint16
#define PHYMOD_CONFIG_DEFINE_UINT32_T   0
#define uint32_t uint32
#define PHYMOD_CONFIG_DEFINE_UINT64_T   0
#define uint64_t uint64

#define PHYMOD_CONFIG_DEFINE_INT32_T	0
#define int32_t  int
#define PHYMOD_CONFIG_DEFINE_INT64_T    0
#define int64_t  int64

/* No need to define size_t */
#define PHYMOD_CONFIG_DEFINE_SIZE_T     0

/* Allow floating point except for Linux kernel builds */
#ifndef __KERNEL__
#define PHYMOD_CONFIG_INCLUDE_FLOATING_POINT    1
#endif

/* Include register reset values in PHY symbol tables */
#define PHYMOD_CONFIG_INCLUDE_RESET_VALUES  1

/* Match PHYMOD error code with shared error codes */
#define PHYMOD_CONFIG_DEFINE_ERROR_CODES    0

typedef enum {
    PHYMOD_E_NONE       = _SHR_E_NONE,
    PHYMOD_E_INTERNAL   = _SHR_E_INTERNAL,
    PHYMOD_E_MEMORY     = _SHR_E_MEMORY,
    PHYMOD_E_IO         = _SHR_E_INTERNAL,
    PHYMOD_E_PARAM      = _SHR_E_PARAM,
    PHYMOD_E_CORE       = _SHR_E_PORT,
    PHYMOD_E_PHY        = _SHR_E_PORT,
    PHYMOD_E_BUSY       = _SHR_E_BUSY,
    PHYMOD_E_FAIL       = _SHR_E_FAIL,
    PHYMOD_E_TIMEOUT    = _SHR_E_TIMEOUT,
    PHYMOD_E_RESOURCE   = _SHR_E_RESOURCE,
    PHYMOD_E_CONFIG     = _SHR_E_CONFIG,
    PHYMOD_E_UNAVAIL    = _SHR_E_UNAVAIL,
    PHYMOD_E_INIT       = _SHR_E_INIT,
    PHYMOD_E_EMPTY      = _SHR_E_EMPTY,
    PHYMOD_E_LIMIT      = _SHR_E_LIMIT           /* Must come last */
} phymod_error_t;

#endif /* __PHYMOD_CUSTOM_CONFIG_H__ */
