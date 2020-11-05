# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.

#
# This is just a Makefile fragment -- it is included by the master
# makefile, um.mk
#
# This file should just append object file names to "ALLOBJS",
# but since it is mean to be linked *first*, it will append
# modules to "CRT0OBJS"
#
#
#  Macro               Contains
#  CRT0OBJS            Files that must be linked first for CFE
#  ALLOBJS             Other files for CFE
#  ZCRTO0OBJS          Files that must be linked first for ZIPSTART
#  ZSOBJS              ZIPSTART object files
#

#
# Allow makefile to override init_mips.S.  If not overridden, it's
# either 'init_mips.S' or 'init_ram.o' for RAMAPP and ZIPSTART versions.
#
ifneq (${CFG_CPU_ARM_CORTEX_A9_ENABLE},)
CRT0OBJS  += low_mem_ca9.o 
else
CRT0OBJS += low_mem.o 
# arm architecture
endif

CRT0OBJS  += cache.o 
#ALLOBJS += exchandler.o

BSPOBJS += board_init.o board.o exchandler.o config.o 

#ifneq ($(strip ${CFG_ROMCODE}),1)
#BSPOBJS += board.o
#endif


# thumb will hang at ddr_init
#CFLAGS += -DCONFIG_ARM -D__ARM__ -mthumb -mabi=aapcs-linux
#CFLAGS += -fno-common -ffixed-r8 -msoft-float -march=armv6k
#CFLAGS += -D__ARM_ARCH_7A__


