# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
QUIET := @
CHIP_NAME = BCM_53570_A0
SOC_MAX_NUM_BLKS = 35 
SDK_MAKE_DIR  := $(TOP)/sdk/make
HEADER_SHARED_SRC := $(TOP)/include/soc/bcm5357x_drv.h
HEADER_SHARED_DST := $(SDK_PATCH_DIR)/include/soc/drv_chip_depend.h 
 
INT_PHY_LIST = XGXS_VIPER SERDES_QTCE XGXS_TSCE XGXS_TSCF 
EXT_PHY_LIST = 542XX
PHYMOD_LIST =  VIPER EAGLE EAGLE_DPLL QTCE FALCON TSCE TSCE_DPLL TSCF 

LDLIBS += -lpcmphyctrl_common -lpcmphyctrl_physelect -lpcmphyctrl_chip -lpcmphyctrl_intphy -lpcmphyctrl_extphy -lpcmphyctrl_phymod

export GCC TOP BUILD_DIR SDK_BUILD_DIR SYSTEM_DIR CHIP_NAME EXT_PHY_LIST INT_PHY_LIST PHYMOD_LIST SDK_BUILD_DIR SDK_PATCH_DIR SDK_DIR SOC_MAX_NUM_BLKS RELEASE_BUILD

ifeq ($(RELEASE_BUILD),1)
ifeq ($(SDK_SRC_DIR),)
$(error SDK_SRC_DIR is undefined, please define it before make with RELEASE_BUILD=1)
endif
endif

phylibs: phylibs_check header_link preparation pcmphyctrl_clean pcmphyctrl_common pcmphyctrl_intphy pcmphyctrl_extphy pcmphyctrl_phymod pcmphyctrl_chip pcmphyctrl_physelect pcmphyctrl_release_build

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
