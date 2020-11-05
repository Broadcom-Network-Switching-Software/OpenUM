# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.

# CFLAGS 
ifeq ($(strip ${CFG_MDKSYM}),1)
CFLAGS += -DCFG_MDKSYM
else
CFG_MDKSYM := 0 
endif
# Targe LIBS
LDLIBS += -lphypkgsrc -lphyutil -lphygeneric

# Patch mdk source code
UM_MDK_DIR = $(TOP)/src/driver/mdk
UM_MDK_PATCH = $(patsubst $(UM_MDK_DIR)/%,%, $(shell find  $(UM_MDK_DIR) -name "*.c") $(shell find  $(UM_MDK_DIR) -name "*.h"))
UM_MDK_DRV_TARGETS = $(addsuffix .MDKCOPY, $(UM_MDK_PATCH)) 

# MDK Build parameters
TOOLCHAIN_BIN_DIR = $(TOOLS)

ifeq ($(strip ${CFG_GNU_TOOLCHAIN}),1)
LD_LIBRARY_PATH = $(TOOLS)/../arm-linux/lib
MDKSYM_INCLUDE =  -I$(TOOLS)/../lib/gcc/arm-linux/4.5.1/include
else
LD_LIBRARY_PATH = $(TOOLS)/../arm-none-eabi/lib
MDKSYM_INCLUDE =  -I$(TOOLS)/../lib/gcc/arm-none-eabi/4.8.3/include
endif
MDKSYM_INCLUDE += -I$(TOP)/mdk/diag/bmddiag/include -I$(TOP)/mdk/phy/include -I$(TOP)/mdk/cdk/include -I$(TOP)/mdk/bmd/include -I$(TOP)/mdk/appl/sys/include -I$(TOP)/mdk/board/include -I$(TOP)/mdk/appl/bmdshell

override PATH := $(TOOLCHAIN_BIN_DIR):$(PATH)
MDK_CFLAGS =  $(filter-out -D%,$(filter-out -I%,$(filter-out -Werror,$(CFLAGS)))) $(MDKSYM_INCLUDE) -DCDK_CONFIG_INCLUDE_BCM56150_A0=1 -DPHY_SYS_USLEEP=sal_usleep  -DBMD_CONFIG_INCLUDE_DMA=0 -DPHY_SYS_USLEEP=sal_usleep -DBMD_SYS_USLEEP=sal_usleep -DPHY_CONFIG_INCLUDE_CHIP_SYMBOLS=$(CFG_MDKSYM) -DCDK_CONFIG_INCLUDE_FIELD_INFO=$(CFG_MDKSYM) -DPHY_DEBUG_ENABLE -DBMD_SYS_DMA_ALLOC_COHERENT=sal_dma_malloc -DBMD_SYS_DMA_FREE_COHERENT=sal_dma_free

export TOOLCHAIN_BIN_DIR LD_LIBRARY_PATH
export PHY_PKG_OPTIONS = -c bcmi_tsc_xgxs,bcm54282,bcmi_qsgmii_serdes,bcm56150,bcm54880e -b ,
export PHY_CPPFLAGS = $(MDK_CFLAGS) 

# MDK Build tools
export CC = $(GCC) 
export LD = $(GLD)
export AR = $(GAR)

#phylibs : cleanpkgs instpkgs $(UM_MDK_DRV_TARGETS)
phylibs : cleanpkgs instpkgs
	@echo "Bulding MDK phy library."
	@make -C $(TOP)/mdk/phy phylibs MDK=$(TOP)/mdk
	@echo "Copying MDK phy library."
	cp $(TOP)/mdk/phy/build/libphygeneric.a lib/.
	cp $(TOP)/mdk/phy/build/libphypkgsrc.a lib/.
	cp $(TOP)/mdk/phy/build/libphyutil.a lib/.

phylibs_clean :
	@echo "Cleaning MDK phy library."
	make -C $(TOP)/mdk/phy clean MDK=$(TOP)/mdk
	make -C $(TOP)/mdk/phy cleanpkgs MDK=$(TOP)/mdk

cleanpkgs : 
	make -C $(TOP)/mdk/phy clean MDK=$(TOP)/mdk
	make -C $(TOP)/mdk/phy cleanpkgs MDK=$(TOP)/mdk

%.MDKCOPY :  
	echo "Patch MDK :"  $(TOP)/mdk/$(patsubst %.MDKCOPY,%,$@)  
	-cp -v $(UM_MDK_DIR)/$(patsubst %.MDKCOPY,%,$@) $(TOP)/mdk/$(patsubst %.MDKCOPY,%,$@) 

instpkgs:
	make -C $(TOP)/mdk/phy instpkgs MDK=$(TOP)/mdk

