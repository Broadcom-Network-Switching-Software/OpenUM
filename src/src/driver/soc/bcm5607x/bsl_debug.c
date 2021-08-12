/*! \file bsl_debug.c
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#include <system.h>

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>

extern int um_console_print(const char *str);
extern int bsl_init(bsl_config_t *config);

/*
  * Function:
  *      bsl_out_hook
  * Purpose:
  *      To print the log on the UM console 
  * Parameters:
  * Returns:
  *      _SHR_E_NONE
  *      _SHR_E_XXX
  */

static int 
bsl_out_hook(bsl_meta_t *meta, const char *fmt, va_list arg_ptr)
{
#if CFG_CONSOLE_ENABLED
	char buf[1024];
	vsprintf(buf, fmt, arg_ptr);
	um_console_print(buf);
#else
	UNREFERENCED_PARAMETER(fmt);
#endif

     return 0;
}

/*
  * Function:
  *      bsl_out_hook
  * Purpose:
  *      To check if the log need to be print 
  * Parameters:
  * Returns:
  *      0: Don't print it
  *      1: print it 
  */

static int
bsl_check_hook(bsl_packed_meta_t val)
{
    int severity = BSL_SEVERITY_GET(val);
    int ls = val & 0xffffff00;

    if (ls == BSL_LS_SOC_PHYMOD ||
        ls == BSL_LS_SOC_PHY ||
        ls == BSL_LS_SOC_COMMON) {
        if (severity >= bslSeverityFatal && severity <= bslSeverityWarn) {
            return 1;
        }
    }

    return 0;
}

static bsl_config_t bsl_hook = { bsl_out_hook, bsl_check_hook };

void
bsl_debug_init(void)
{
    bsl_init(&bsl_hook);
}
