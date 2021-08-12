/*
 * $Id: tdm_top.h,v 1.1 Broadcom SDK $
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 * All Rights Reserved.$
 *
 * TDM top header for core scheduler
 */
#ifndef __TDM_TOP_H__
#define __TDM_TOP_H__

#ifndef TDM_PREPROCESSOR_DIRECTIVES_H
#define TDM_PREPROCESSOR_DIRECTIVES_H

/*
 * These are the possible debug types/flags for cdk_debug_level (below).
 */
#define CDK_DBG_ERR       (1 << 0)    /* Print errors */
#define CDK_DBG_WARN      (1 << 1)    /* Print warnings */
#define CDK_DBG_VERBOSE   (1 << 2)    /* General verbose output */
#define CDK_DBG_VVERBOSE  (1 << 3)    /* Very verbose output */
#define CDK_DBG_DEV       (1 << 4)    /* Device access */
#define CDK_DBG_REG       (1 << 5)    /* Register access */
#define CDK_DBG_MEM       (1 << 6)    /* Memory access */
#define CDK_DBG_SCHAN     (1 << 7)    /* S-channel operations */
#define CDK_DBG_MIIM      (1 << 8)    /* MII managment access */
#define CDK_DBG_DMA       (1 << 9)    /* DMA operations */
#define CDK_DBG_HIGIG     (1 << 10)   /* HiGig information */
#define CDK_DBG_PACKET    (1 << 11)   /* Packet data */

extern unsigned int cdk_debug_level;
extern int (*cdk_debug_printf)(const char *format, ...);
extern int cdk_printf(const char *fmt, ...);

#define CDK_DEBUG_CHECK(flags) (((flags) & cdk_debug_level) == (flags))

#define CDK_DEBUG(flags, stuff) \
    if (CDK_DEBUG_CHECK(flags) && cdk_debug_printf != 0) \
    (*cdk_debug_printf) stuff

#define CDK_ERR(stuff) CDK_DEBUG(CDK_DBG_ERR, stuff)
#define CDK_WARN(stuff) CDK_DEBUG(CDK_DBG_WARN, stuff)
#define CDK_VERB(stuff) CDK_DEBUG(CDK_DBG_VERBOSE, stuff)
#define CDK_VVERB(stuff) CDK_DEBUG(CDK_DBG_VVERBOSE, stuff)
#define CDK_DEBUG_DEV(stuff) CDK_DEBUG(CDK_DBG_DEV, stuff)
#define CDK_DEBUG_REG(stuff) CDK_DEBUG(CDK_DBG_REG, stuff)
#define CDK_DEBUG_MEM(stuff) CDK_DEBUG(CDK_DBG_MEM, stuff)
#define CDK_DEBUG_SCHAN(stuff) CDK_DEBUG(CDK_DBG_SCHAN, stuff)
#define CDK_DEBUG_MIIM(stuff) CDK_DEBUG(CDK_DBG_MIIM, stuff)
#define CDK_DEBUG_DMA(stuff) CDK_DEBUG(CDK_DBG_DMA, stuff)
#define CDK_DEBUG_HIGIG(stuff) CDK_DEBUG(CDK_DBG_HIGIG, stuff)
#define CDK_DEBUG_PACKET(stuff) CDK_DEBUG(CDK_DBG_PACKET, stuff)

/* Prints */
#define TDM_PRINT0(a) CDK_VERB((a))
#define TDM_VERBOSE(_frmt,a) CDK_VVERB((_frmt, a))
#define TDM_ERROR8(a,b,c,d,e,f,g,h,i) CDK_ERR((a, b, c, d, e, f, g, h, i))
#define TDM_ERROR7(a,b,c,d,e,f,g,h) CDK_ERR((a, b, c, d, e, f, g, h))
#define TDM_ERROR6(a,b,c,d,e,f,g) CDK_ERR((a, b, c, d, e, f, g))
#define TDM_ERROR5(a,b,c,d,e,f) CDK_ERR((a, b, c, d, e, f))
#define TDM_ERROR4(a,b,c,d,e) CDK_ERR((a, b, c, d, e))
#define TDM_ERROR3(a,b,c,d) CDK_ERR((a, b, c, d))
#define TDM_ERROR2(a,b,c) CDK_ERR((a, b, c))
#define TDM_ERROR1(a,b) CDK_ERR((a, b))
#define TDM_ERROR0(a) CDK_ERR((a))
#define TDM_WARN6(a,b,c,d,e,f,g) CDK_WARN((a, b, c, d, e, f, g))
#define TDM_WARN5(a,b,c,d,e,f) CDK_WARN((a, b, c, d, e, f))
#define TDM_WARN4(a,b,c,d,e) CDK_WARN((a, b, c, d, e))
#define TDM_WARN3(a,b,c,d) CDK_WARN((a, b, c, d))
#define TDM_WARN2(a,b,c) CDK_WARN((a, b, c))
#define TDM_WARN1(a,b) CDK_WARN((a, b))
#define TDM_WARN0(a) CDK_WARN((a))
#define TDM_PRINT9(a,b,c,d,e,f,g,h,i,j) CDK_VERB((a, b, c, d, e, f, g, h, i, j))
#define TDM_PRINT8(a,b,c,d,e,f,g,h,i) CDK_VERB((a, b, c, d, e, f, g, h, i))
#define TDM_PRINT7(a,b,c,d,e,f,g,h) CDK_VERB((a, b, c, d, e, f, g, h))
#define TDM_PRINT6(a,b,c,d,e,f,g) CDK_VERB((a, b, c, d, e, f, g))
#define TDM_PRINT5(a,b,c,d,e,f) CDK_VERB((a, b, c, d, e, f))
#define TDM_PRINT4(a,b,c,d,e) CDK_VERB((a, b, c, d, e))
#define TDM_PRINT3(a,b,c,d) CDK_VERB((a, b, c, d))
#define TDM_PRINT2(a,b,c) CDK_VERB((a, b, c))
#define TDM_PRINT1(a,b) CDK_VERB((a, b))
#define TDM_BIG_BAR TDM_PRINT0(("#################################################################################################################################\n"));
#define TDM_SML_BAR TDM_PRINT0(("---------------------------------------------------------------------------------------------------------------------------------\n"));
#define TDM_DEBUG TDM_PRINT0(("--- DEBUG ---\n"));
/* Compiler */
#define LINKER_DECL
#define TDM_ALLOC(_sz,_id) sal_malloc(_sz)
#define TDM_FREE(_sz) sal_free(_sz)
#define TDM_COPY(_dst,_src,_len) sal_memcpy(_dst, _src, _len)
#define TDM_MSET(_str, _val, _len) \
    do { \
        int _idx; \
        for (_idx = 0; _idx < _len; _idx++) \
        { \
            *(_str + _idx) = _val; \
        } \
    } while (0)

/* TDM headers */
#include "tdm_defines.h"
#include "tdm_soc.h"
#include "tdm_methods.h"

#define TOKEN_CHECK(a)                                              \
            if (a>=_tdm->_chip_data.soc_pkg.soc_vars.fp_port_lo &&    \
                a<=_tdm->_chip_data.soc_pkg.soc_vars.fp_port_hi)    \

#define TDM_SEL_CAL(_cal_id,_cal_pntr)                                        \
            switch (_cal_id) {                                                \
                case 0: _cal_pntr=_tdm->_chip_data.cal_0.cal_main; break;    \
                case 1: _cal_pntr=_tdm->_chip_data.cal_1.cal_main; break;    \
                case 2: _cal_pntr=_tdm->_chip_data.cal_2.cal_main; break;    \
                case 3: _cal_pntr=_tdm->_chip_data.cal_3.cal_main; break;    \
                case 4: _cal_pntr=_tdm->_chip_data.cal_4.cal_main; break;    \
                case 5: _cal_pntr=_tdm->_chip_data.cal_5.cal_main; break;    \
                case 6: _cal_pntr=_tdm->_chip_data.cal_6.cal_main; break;    \
                case 7: _cal_pntr=_tdm->_chip_data.cal_7.cal_main; break;    \
                default:                                                    \
                    _cal_pntr=NULL;                                            \
                    TDM_PRINT1("Invalid calendar ID - %0d\n",_cal_id);        \
                    return (TDM_EXEC_CORE_SIZE+1);                            \
            }
#define TDM_SEL_GRP(_grp_id,_grp_pntr)                                        \
            switch (_grp_id) {                                                \
                case 0: _grp_pntr=_tdm->_chip_data.cal_0.cal_grp; break;    \
                case 1: _grp_pntr=_tdm->_chip_data.cal_1.cal_grp; break;    \
                case 2: _grp_pntr=_tdm->_chip_data.cal_2.cal_grp; break;    \
                case 3: _grp_pntr=_tdm->_chip_data.cal_3.cal_grp; break;    \
                case 4: _grp_pntr=_tdm->_chip_data.cal_4.cal_grp; break;    \
                case 5: _grp_pntr=_tdm->_chip_data.cal_5.cal_grp; break;    \
                case 6: _grp_pntr=_tdm->_chip_data.cal_6.cal_grp; break;    \
                case 7: _grp_pntr=_tdm->_chip_data.cal_7.cal_grp; break;    \
                default:                                                    \
                    _grp_pntr=NULL;                                            \
                    TDM_PRINT1("Invalid group ID - %0d\n",_grp_id);            \
                    return (TDM_EXEC_CORE_SIZE+1);                            \
            }
#define TDM_PUSH(a,b,c)                                                 \
                {                                                        \
                    int TDM_PORT_POP=_tdm->_core_data.vars_pkg.port;    \
                    _tdm->_core_data.vars_pkg.port=a;                    \
                    c=_tdm->_core_exec[b](_tdm);                        \
                    _tdm->_core_data.vars_pkg.port=TDM_PORT_POP;        \
                }

extern tdm_mod_t
*SOC_SEL_TDM(tdm_soc_t *chip, tdm_mod_t *_tdm);
extern tdm_mod_t
*_soc_set_tdm_tbl(tdm_mod_t *_tdm);
extern void
_soc_tdm_ver(int ver[8]);


#endif /* TDM_PREPROCESSOR_DIRECTIVES_H*/

/* ESW chip support */
#include "tdm_fl_top.h"

#endif /* __TDM_TOP_H__ */
