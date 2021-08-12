# $Id: um_gen_phylibs.mk,v 1.4 Broadcom SDK $
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2021 Broadcom Inc. All rights reserved.
QUIET := @
CHIP_NAME = BCM_56070_A0
SOC_MAX_NUM_BLKS = 30
SDK_MAKE_DIR := $(TOP)/sdk/make
HEADER_SHARED_SRC := $(TOP)/include/soc/bcm5607x_drv.h
HEADER_SHARED_DST := $(SDK_PATCH_DIR)/include/soc/drv_chip_depend.h

PHYMOD_LIST = TSCE16 MERLIN16 TSCF16_GEN3 FALCON16_V1L4P1
INT_PHY_LIST = XGXS_TSCE XGXS_TSCF
EXT_PHY_LIST =

PHYMOD_CFLAGS += -DPHYMOD_SUPPORT
PHYMOD_CFLAGS += -DPHYMOD_TIER1_SUPPORT
PHYMOD_CFLAGS += -DPHYMOD_INCLUDE_CUSTOM_CONFIG
PHYMOD_CFLAGS += -DPHYMOD_CONFIG_INCLUDE_CHIP_SYMBOLS=1
PHYMOD_CFLAGS += -DPHYMOD_CONFIG_INCLUDE_FIELD_INFO=1
PHYMOD_CFLAGS += $(foreach phy,$(PHYMOD_LIST), -DPHYMOD_$(phy)_SUPPORT)
CFLAGS += $(PHYMOD_CFLAGS)

PHYLIBS = pcmphyctrl_phymod
PHYLIBS += pcmphyctrl_common
PHYLIBS += pcmphyctrl_physelect
PHYLIBS += pcmphyctrl_chip
PHYLIBS += pcmphyctrl_intphy
PHYLIBS += pcmphyctrl_extphy
LD_PHYLIBS := $(foreach lib,$(PHYLIBS),-l${lib})
LDLIBS += $(LD_PHYLIBS)

export GCC
export RELEASE_BUILD
export TOP BUILD_DIR SDK_PATCH_DIR SYSTEM_DIR SDK_DIR
export CHIP_NAME SOC_MAX_NUM_BLKS
export EXT_PHY_LIST INT_PHY_LIST PHYMOD_LIST PHYMOD_CFLAGS

ifeq ($(RELEASE_BUILD),1)
ifeq ($(SDK_SRC_DIR),)
$(error SDK_SRC_DIR is undefined, please define it before make with RELEASE_BUILD=1)
endif
endif

phylibs: phylibs_check header_link preparation pcmphyctrl_clean $(PHYLIBS) pcmphyctrl_release_build

phylibs_check:
ifeq ($(SDK_SRC_DIR),)
	$(error SDK_SRC_DIR is undefined, please define it before make phylibs)
endif

header_link:
ifneq ($(shell readlink $(HEADER_SHARED_DST)), $(HEADER_SHARED_SRC))
	$(QUIET)rm -f $(HEADER_SHARED_DST)
	$(QUIET)ln -s $(HEADER_SHARED_SRC) $(HEADER_SHARED_DST)
endif

pcmphyctrl_physelect:
	$(QUIET)make --no-print-directory -C $(SDK_MAKE_DIR) pcmphyctrl_physelect

pcmphyctrl_common:
	$(QUIET)make --no-print-directory -C $(SDK_MAKE_DIR) pcmphyctrl_common

pcmphyctrl_intphy:
	$(QUIET)make --no-print-directory -C $(SDK_MAKE_DIR) pcmphyctrl_intphy

pcmphyctrl_extphy:
	$(QUIET)make --no-print-directory -C $(SDK_MAKE_DIR) pcmphyctrl_extphy

pcmphyctrl_chip:
	$(QUIET)make --no-print-directory -C $(SDK_MAKE_DIR) pcmphyctrl_chip

pcmphyctrl_phymod:
	$(QUIET)make --no-print-directory -C $(SDK_MAKE_DIR) pcmphyctrl_phymod

pcmphyctrl_release_build:
ifeq ($(RELEASE_BUILD),1)
	$(QUIET)make --no-print-directory -C $(SDK_MAKE_DIR) pcmphyctrl_release_build
endif

pcmphyctrl_clean:
	$(QUIET)make --no-print-directory -C $(SDK_MAKE_DIR) clean
