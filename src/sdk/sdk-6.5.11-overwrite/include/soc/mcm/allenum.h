#ifndef _SOC_ALLENUM_H
#define _SOC_ALLENUM_H


#ifndef EXTERN
# ifdef __cplusplus
#  define EXTERN extern "C"
# else
#  define EXTERN extern
# endif
#endif

typedef int soc_reg_t;
typedef int soc_field_t;
typedef int soc_mem_t;
typedef int soc_format_t;

#define SOC_MAX_MEM_WORDS 4
#define NUM_SOC_REG       4
#define SOC_MAX_NUM_PIPES 1
#define SOC_MAX_NUM_PP_PORTS SOC_MAX_NUM_PORTS
#define NUM_SOC_MEM             4
#define SOC_NUM_SUPPORTED_CHIPS 1
#endif
