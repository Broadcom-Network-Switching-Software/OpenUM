# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.

CFG_BOARDNAME = BCM95333X
CFG_IMG := bcm95333x

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


include .config

CFG_INTERRUPTS ?= 0
CFG_LITTLE ?= 1
CFG_RELOC ?= 0
CFG_UNCACHED ?= 0
CFG_VAPI ?= 0 
CFG_BOOTRAM ?= 0
CFG_PCI ?= 0
CFG_ZLIB ?= 0
CFG_LDT ?= 0
CFG_WEB ?= 0
CFG_GNU_TOOLCHAIN ?= 0


TOP = $(abspath  ../..)
BUILD_DIR =  $(abspath  ./)
TOOL_DIR =  $(abspath  ../../tools)



CLEANOBJS += cfe.* cfez.* cfez zipstart.* conf.h  
CONFIG_FILES = .config include/config.h 
ALL : $(CFG_CONFIG_UPDATE) conf.h img_name cfe cfe.bin ${FLASH_IMG}
	mv cfe.bin ${CFG_IMG}-${target}.bin


img_name:

conf.h: .config
	perl $(TOOL_DIR)/config2h.pl


include $(BUILD_DIR)/um.mk
  
-include ${BUILD_DIR}/um_gen_phylibs.mk

include ${BUILD_DIR}/um_link.mk


