# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.

CFG_BOARDNAME = BCM95340X
CFG_IMG := bcm95340x
SYSTEM := bcm95340x
#
#
#  Config change detection  
#
#

ifneq ($(target), )
$(shell ln -s -f configs/config.$(target).old .config)
$(shell ln -s -f configs/config_$(target).h  include/config.h)
CFG_CONFIG_UPDATE = clean_wo_config
endif

target ?= umweb

ifeq ($(strip ${CFG_LOADER}),1)
target = loader
$(shell ln -s -f configs/config.$(target).old .config)
$(shell ln -s -f configs/config_$(target).h  include/config.h)
CFG_CONFIG_UPDATE = clean_wo_config
endif 

ifeq ($(wildcard .config), ) 
$(shell ln -s -f configs/config.$(target).old .config)
$(shell ln -s -f configs/config_$(target).h  include/config.h)
CFG_CONFIG_UPDATE = clean_wo_config
endif 

ifeq ($(wildcard include/config.h), )
$(shell ln -s -f configs/config.$(target).old .config)
$(shell ln -s -f configs/config_$(target).h  include/config.h)
CFG_CONFIG_UPDATE = clean_wo_config
endif

ARCH=arm


include .config


CFETARGET := cfe
CFG_INTERRUPTS ?= 0
CFG_LITTLE ?= 1
CFG_RELOC ?= 0
CFG_UNCACHED ?= 0
CFG_VAPI ?= 0 
CFG_BOOTRAM ?= 0
CFG_PCI ?= 0
CFG_ZLIB ?= 0
CFG_XIP ?= 1
CFG_WEB ?= 0
CFG_GNU_TOOLCHAIN ?= 0


TOP = $(abspath  ../..)
BUILD_DIR =  $(abspath  ./)
SYSTEM_DIR = $(abspath ./)
TOOL_DIR =  $(abspath  ../../tools)



CLEANOBJS += cfe.* cfez.* cfez zipstart.* conf.h 
CONFIG_FILES = .config include/config.h 

ifeq ($(strip ${CFG_XIP}),1)
ifeq ($(strip ${CFG_GNU_TOOLCHAIN}),1)
LD_SCRIPT = um_xip.lds
else
LD_SCRIPT = um_xip_pic.lds
endif
#TARGET = ${CFETARGET}
ALL : $(CFG_CONFIG_UPDATE) conf.h img_name ${CFETARGET} ${CFETARGET}.bin ${FLASH_IMG}
	mv cfe.bin ${CFG_IMG}-${target}.bin
else
LD_SCRIPT = um_payload.lds
TARGET = ${CFG_IMG}
OBJCOPYOPTION = -R .flashdata
ALL : $(CFG_UPDATE_CONFIG) conf.h img_name ${CFETARGET} ${CFETARGET}.bin ${BOOTTARGET} ${BOOTTARGET}.bin ${FLASH_IMG}
	mv cfe.bin ${CFG_IMG}-${target}.bin
endif
ifneq ($(strip ${CFG_XIP}),1)
	rm ${CFETARGET}.bin
	rm ${BOOTTARGET}.bin
endif


img_name:

conf.h: .config 
	perl $(TOOL_DIR)/config2h.pl

$(TOOL_DIR)/bin2codefile:
	gcc $(TOOL_DIR)/bin2codefile.c -o $(TOOL_DIR)/bin2codefile

${CFG_IMG}-${target}.hex: cfe.bin
	$(TOOL_DIR)/bin2hex.pl cfe.bin ${CFG_IMG}-${target}.hex

${CFG_IMG}-${target}-for-de.bin: $(TOOL_DIR)/bin2codefile
	$(TOOL_DIR)/bin2codefile  cfe.bin IPR_G40RMHS16384X64R532COVTSSY_WD0.codefile
	$(TOOL_DIR)/gen_emul_images.pl
	mv rom.bin $@

${CFG_IMG}-${target}.bin8: ${CFETARGET}.bin
	perl $(TOOL_DIR)/binswap8.pl ${CFETARGET}.bin ${CFG_IMG}-${target}.bin8

 
include $(BUILD_DIR)/um.mk

-include ${BUILD_DIR}/um_gen_phylibs.mk

include ${BUILD_DIR}/um_link.mk


