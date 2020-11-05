/*
 * 
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *  
 * File:    thumb.h
 * Purpose: Macros for thumb mode
 */

#ifndef THUMB_H
#define THUMB_H

#include "board.h"

#ifdef THUMB_ALL

#define THUMB_MODE(_reg_)

#define THUMB_CALL(_reg_, _func_)                       \
    bl  _func_

#else

#define THUMB_MODE(_reg_)                                \
        ldr     _reg_,=99999f+1                         ;\
        bx      _reg_                                   ;\
        .thumb                                          ;\
        .thumb_func                                     ;\
99999:

#define THUMB_CALL(_reg_, _func_)                        \
    ldr _reg_,=_func_+1                                 ;\
    mov lr,pc                                           ;\
    bx  _reg_


#endif

#ifdef THUMB_ALL

#define ARM_MODE(_reg_)

#else

#define ARM_MODE(_reg_)                                  \
        ldr     _reg_,=99998f                           ;\
        bx      _reg_                                   ;\
        .arm                                            ;\
99998:

#endif

#if 0
#define THUMB_CALL(_reg_, _func_)                        \
        ldr     _reg_,=_func_+1                         ;\
        mov     lr,pc                                   ;\
        bx      _reg_                                   ;\
        .code   16                                      ;\
        .thumb_func                                     ;\
        ldr     _reg_,=99997f+1                         ;\
        bx      _reg_                                   ;\
99997:
#endif
        

#endif
