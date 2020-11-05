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
bsl_out_hook(bsl_meta_t *meta, const char *fmt, va_list arg_ptr) {

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

static int bsl_check_hook(bsl_packed_meta_t val) {

    if ((val & 0xFFFFFF00) == BSL_LS_SOC_COMMON) return 0;
    if ((val & 0xFFFFFF00) == BSL_LS_SOC_MIIM)   return 0;
    if ((val & 0xFFFFFF00) == BSL_LS_SOC_MII)    return 0;
    if ((val & 0xFFFFFF00) == BSL_LS_SOC_PHY)    return 0;
    if ((val & 0xFFFFFF00) == BSL_LS_BCM_PORT)   return 0;
    if ((val & 0xFFFFFF00) == BSL_LS_BCM_PHY)    return 0;
    if ((val & 0xFFFFFF00) == BSL_LS_SOC_PHYMOD) return 0;

    return 0;
}


static bsl_config_t bsl_hook = { bsl_out_hook, bsl_check_hook };


void  bsl_debug_init(void) {
      bsl_init(&bsl_hook);
}


