# $Id: Makefile,v 1.1 Broadcom SDK $
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2021 Broadcom Inc. All rights reserved.

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

CRT0OBJS += vector.o intr_entry.o
#ALLOBJS += exchandler.o

CRT0OBJS += tick.o cache.o
BSPOBJS += exchandler.o config.o wdt.o
BOARDAPIOBJS += board_init.o board.o board_gpio.o board_intr.o board_coe.o
BOARDAPIOBJS += board_port.o board_temp.o board_led.o board_link.o board_mcs.o

#ifneq ($(strip ${CFG_ROMCODE}),1)
#BSPOBJS += board.o
#endif


# thumb will hang at ddr_init
#CFLAGS += -DCONFIG_ARM -D__ARM__ -mthumb -mabi=aapcs-linux
#CFLAGS += -fno-common -ffixed-r8 -msoft-float -march=armv6k
#CFLAGS += -D__ARM_ARCH_7A__


