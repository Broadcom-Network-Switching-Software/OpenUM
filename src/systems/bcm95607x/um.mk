# $Id: um.mk,v 1.15 Broadcom SDK $
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.

#
# UM's version number
#

CHIP = bcm5607x

CFG_BOARDNAME ?= BCM956070K

include ${TOP}/systems/${SYSTEM}/um_version.mk

#
# Default values for certain parameters
#

CFG_MLONG64 ?= 0
CFG_RELOC ?= 0
CFG_UNCACHED ?= 0
CFG_BOOTRAM ?= 0
CFG_PCI ?= 0
CFG_LDT ?= 0
CFG_LDT_REV_017 ?= 0
CFG_PCIDEVICE ?= 0
CFG_DOWNLOAD ?= 0
CFG_USB ?= 0
CFG_MSYS ?= 0
CFG_ZLIB ?= 0
CFG_VGACONSOLE ?= 0
CFG_BIENDIAN ?= 0
CFG_RAMAPP ?= 0
CFG_USB ?= 0
CFG_ZIPSTART ?= 0
CFG_ZIPPED_CFE ?= 0
CFG_RELEASE_STAGE ?= 0
CFG_COMPRESSED_IMAGE ?= 0

SDK_PATCH_DIR = $(TOP)/sdk/sdk-overwrite
SDK_RELEASE_DIR = $(TOP)/sdk/release

ifeq ($(SDK_SRC_DIR),)
SDK_DIR = $(TOP)/sdk/sdk-header
else
SDK_DIR = $(SDK_SRC_DIR)
endif

#
# Override some settings based on the value of CFG_RELOC.
#
# 'STATIC' relocation means no biendian, no bootram, ZIPstart
# '1' is SVR4 Relocation,  no RAMAPP, no BOOTRAM
# '0' (no relocation) : no changes
#

ifeq ($(strip ${CFG_RELOC}),1)
  CFG_RAMAPP = 0
  CFG_BOOTRAM = 0
endif

ifeq ($(strip ${CFG_RELOC}),STATIC)
  CFG_RAMAPP = 0
  CFG_BIENDIAN = 0
  CFG_BOOTRAM = 0
  CFG_ZIPSTART = 1
  CFE_CFLAGS += -DCFG_ZIPSTART=1
endif

#
# Default goal.
#

all : ALL

#
# Paths to other parts of the firmware.  Everything's relative to ${TOP}
# so that you can actually do a build anywhere you want.
#

BSP_SRC    = ${TOP}/systems/${SYSTEM}/src
BSP_INC    = ${TOP}/systems/${SYSTEM}/include
LIB_PATH   = ${BUILD_DIR}/lib
MAIN_SRC   = ${TOP}/src/kernel
MAIN_INC   = ${TOP}/include
SOC_SRC    = ${TOP}/src/driver/soc/${CHIP}
UIP_SRC    = ${TOP}/src/net
FLASH_SRC  = ${TOP}/src/driver/flash
XCMD_INC   = ${TOP}/src/appl/xcommands
SAL_CONFIG_H = ${TOP}/include/arm/sal_config.h
#
# Preprocessor defines for CFE's version number
#

VDEF = -DCFE_VER_MAJOR=${CFE_VER_MAJ} -DCFE_VER_MINOR=${CFE_VER_MIN} -DCFE_VER_BUILD=${CFE_VER_ECO}
VDEF += -DCFE_VER_SDK=${CFE_VER_SDK}

#ifneq ("$(strip ${CFE_VER_SDK})","")
#  VDEF += -DCFE_VER_SDK=${CFE_VER_SDK}
#endif

ifdef CFG_SERIAL_BAUD_RATE_OVERRIDE
  CFE_CFLAGS += -DCFG_SERIAL_BAUD_RATE=${CFG_SERIAL_BAUD_RATE_OVERRIDE}
endif

#
# Construct the list of paths that will eventually become the include
# paths and VPATH.
#

SRCDIRS = ${BSP_SRC} ${MAIN_SRC} ${TOP}/include

# ${TOP}/net ${TOP}/dev ${TOP}/ui ${TOP}/lib ${TOP}/httpd ${TOP}/httpd/callbacks

CFE_INC = ${TOP}/include ${TOP}/include/${ARCH} ${TOP}/src/net ${TOP}/src/appl/web/content ${XCMD_INC}
CFE_INC += $(SDK_PATCH_DIR)/include $(SDK_PATCH_DIR)/libs/phymod/include $(SDK_DIR)/include $(SDK_DIR)/libs/phymod/include ${TOP}/include/pcm/soc

#
# Configure tools and basic tools flags.  This include sets up
# macros for calling the C compiler, basic flags,
# and linker scripts.
#

include ${BUILD_DIR}/src/tools.mk

#
# Add some common flags that are used on any architecture.
#
CFLAGS += -DVENDOR_BROADCOM
CFLAGS += -I. $(INCDIRS)
CFLAGS += -D_CFE_ ${VDEF} -DCFG_BOARDNAME=\"${CFG_BOARDNAME}\"


ifeq ($(strip ${CFG_QT}), 1)
CFLAGS += -D__EMULATION__
endif
#
# Gross - allow more options to be supplied from command line
#

ifdef CFG_OPTIONS
OPTFLAGS = $(patsubst %,-D%,$(subst :, ,$(CFG_OPTIONS)))
CFLAGS += ${OPTFLAGS}
endif

CFLAGS += -DPHYMOD_SUPPORT
CFLAGS += -DPHYMOD_TIER1_SUPPORT
CFLAGS += -DPHYMOD_INCLUDE_CUSTOM_CONFIG

#
# Include the makefiles for the architecture-common, cpu-specific,
# and board-specific directories.  Each of these will supply
# some files to "ALLOBJS".  The BOARD and CHIPSET directories are optional
# as some ports are so simple they don't need boad-specific stuff.
#

# include ${ARCH_SRC}/Makefile
# include ${CPU_SRC}/Makefile

# ifneq ("$(strip ${BOARD})","")
# include ${BOARD_SRC}/Makefile
# endif

include ${TOP}/systems/${SYSTEM}/src/Makefile

#
# Pull in the common directories
#

# include ${MAIN_SRC}/Makefile
# include ${TOP}/lib/Makefile

KERNELOBJS = background.o link.o main.o rx.o timer.o tx.o intr.o

SALOBJS = sal_alloc.o sal_chksum.o sal_console.o sal_init.o sal_libc.o\
          sal_printf.o sal_timer.o sal_config.o sal_strtok_r.o

SRCDIRS += ${TOP}/src/sal/${ARCH}

ifeq ($(strip ${CFG_IP}), 1)
UIPOBJS = uip.o uip6.o uip_arp.o uip-ds6.o uip-nd6.o uip-icmp6.o uip_arch.o\
          uip_task.o
SRCDIRS += ${TOP}/src/net
endif

APPLOBJS = app_init.o igmpsnoop.o igmpsnoop_cbk.o
ifeq ($(strip ${CFG_IP}), 1)
APPLOBJS += dhcpc.o mdns.o mdns_utils.o
SRCDIRS += ${TOP}/src/appl/dhcpc
SRCDIRS += ${TOP}/src/appl/mdns
endif
ifndef CFG_SOC_SNAKE_TEST
APPLOBJS += snaketest.o
endif
SRCDIRS += ${TOP}/src/appl ${TOP}/src/appl/snaketest
SRCDIRS += ${TOP}/src/appl/igmpsnoop ${TOP}/src/appl/net

#
# SDKCLI : core file
#
SDKCLIDIRS = ${TOP}/src/appl/sdkcli ${TOP}/src/appl/sdkcli/cmd
SRCDIRS += ${SDKCLIDIRS}
SDKCLIOBJS = $(patsubst %.c,%.o,$(notdir $(foreach dir, $(SDKCLIDIRS), $(wildcard $(dir)/*.c))))

#
# SDKCLI : editline library
#
EDITLINEDIRS = ${TOP}/src/appl/editline
SRCDIRS += ${EDITLINEDIRS}
EDITLINEOBJS = $(patsubst %.c,%.o,$(notdir $(foreach dir, $(EDITLINEDIRS), $(wildcard $(dir)/*.c))))

#
# Shared library
#
SHRDIRS = ${TOP}/src/utils/shr
SRCDIRS += ${SHRDIRS}
SHROBJS = $(patsubst %.c,%.o,$(notdir $(foreach dir, $(SHRDIRS), $(wildcard $(dir)/*.c))))


#
# XCOMANND core file include
#

XCMDOBJS = xc_input_buffer.o xc_input_cli.o xc_output_buf.o xcmd_cli.o xcmd_core.o xcmd_auth.o xcmd_display_page.o
SRCDIRS += ${TOP}/src/appl/xcmd

#
# XCOMANND XML Tables generator : please use make xcommand to generate callback function
#

XCOMMANDS_XMLDIR = ${TOP}/src/appl/xcommands
XCOMMANDS_PARSER = ${TOP}/tools/xcommands/parse_context.pl
XCOMMAND_XML_TABLES := \
    $(XCOMMANDS_XMLDIR)/global.xml

XCOMMAND_DEF_TABLES := $(XCOMMANDS_XMLDIR)/defines.xml

XCOMMANDSDIRS = ${TOP}/src/appl/xcommands/callback ${TOP}/src/appl/xcommands/generated

SRCDIRS += ${XCOMMANDSDIRS}
XCOMMANDSOBJS = $(patsubst %.c,%.o,$(notdir $(foreach dir, $(XCOMMANDSDIRS), $(wildcard $(dir)/*.c))))

ifeq ($(strip ${CFG_WEB}), 1)
HTTPDOBJS = httpd.o httpd_arch.o ssp.o ssp_fs_root.o ssp_fstab.o
SRCDIRS += ${TOP}/src/appl/httpd ${TOP}/src/appl/web
GUIDIRS = ${TOP}/src/appl/web/callback ${TOP}/src/appl/web/content
SRCDIRS += ${GUIDIRS}
GUIOBJS = $(patsubst %.c,%.o,$(notdir $(foreach dir, $(GUIDIRS), $(wildcard $(dir)/*.c))))
endif

UIOBJS = cli.o ui_switch.o ui_system.o ui_flash.o ui_igmpsnoop.o ui_rx.o ui_tx.o ui_pcm.o


SRCDIRS += ${TOP}/src/appl/cli

PERSISOBJS = persistence.o serialize.o flash_medium.o ramtxt_medium.o mcast.o\
             mirror.o qos.o serializers.o vlan.o lag.o loopdetect.o\
             system.o

ifeq ($(strip ${CFG_IP}), 1)
PERSISOBJS += network.o
endif

SRCDIRS += ${TOP}/src/appl/persistence ${TOP}/src/appl/persistence/media/flash\
           ${TOP}/src/appl/persistence/media/ramtxt\
           ${TOP}/src/appl/persistence/serialize ${TOP}/src/serializers\

BRDIMPLOBJ = brd_misc.o brd_rxtx.o brd_vlan.o
SRCDIRS += ${TOP}/src/board

FLASHOBJ = iproc_qspi.o flash.o

SWITCHOBJ = flrxtx.o flloop.o flvlan.o flport.o fllinkscan.o flled.o \
            fllag.o flaccess.o flswitch.o \
            macutil.o xlmac.o clmac.o pbsmh.o\
            flblocks.o flportconf.o flpcm.o fltime.o flintr.o \
            set_tdm.o tdm_fl_chk.o tdm_fl_cmn.o \
            tdm_fl_filter.o tdm_fl_main.o \
            tdm_fl_parse.o tdm_fl_proc.o \
            tdm_fl_shim.o tdm_filter.o tdm_llist.o \
            tdm_main.o tdm_math.o tdm_ovsb.o \
            tdm_parse.o tdm_proc.o tdm_scan.o \
            tdm_tsfm.o tdm_ver.o tdm_vmap.o

ifeq ($(strip ${CFG_IP}), 1)
SWITCHOBJ += flmdns.o
endif

# Unit tests.
SRCDIRS  +=  ${TOP}/src/appl/unittest/utgpio
CFE_INC += ${TOP}/src/appl/unittest/include
UTOBJS += utgpio_intr.o utgpio_isr.o

# Add GPIO Support
SRCDIRS += ${TOP}/src/driver/hmi/cmicx/src
CFE_INC += ${TOP}/src/driver/hmi/cmicx/include
SWITCHOBJ += cmicx_gpio.o

# Add PCM Support
SRCDIRS += ${TOP}/src/driver/pcm
SWITCHOBJ += pcm_phyctrl_hounds.o pcm_common.o

CFE_INC += $(SDK_PATCH_DIR)/include $(SDK_PATCH_DIR)/libs/phymod/include $(SDK_DIR)/include $(SDK_DIR)/libs/phymod/include
SWITCHOBJ += sdk_phy.o bsl_debug.o

#
# RAMOBJS specifies objects which are used in exception context
# or any special purpose need to be placed in RAM.
#

# Exception.
RAMOBJS += exchandler.o sal_printf.o sal_console.o sal_libc.o

# Interrupt System.
RAMOBJS += intr.o board_intr.o board_gpio.o
RAMOBJS += cmicx_gpio.o flintr.o flaccess.o field.o

# Application.
RAMOBJS += utgpio_isr.o

ifeq ($(strip ${CFG_SOC_SNAKE_TEST}), 1)
SWITCHOBJ += flsnaketest.o
endif
ifeq ($(strip ${CFG_QT}), 1)
SWITCHOBJ += flqt.o
endif

SRCDIRS += ${TOP}/src/driver/flash
SRCDIRS += ${TOP}/src/driver/soc/${CHIP}
SRCDIRS += ${TOP}/src/driver/soc/${CHIP}/tdm

UTILSOBJS = ui_utils.o ports_utils.o ui_utils.o factory_utils.o pbmp.o  nvram_utils.o system_utils.o field.o

SRCDIRS += ${TOP}/src/utils/ports ${TOP}/src/utils/ui\
           ${TOP}/src/utils/nvram ${TOP}/src/utils/system ${TOP}/src/utils/share

ifeq ($(strip ${CFG_IP}), 1)
UTILSOBJS += net_utils.o
SRCDIRS += ${TOP}/src/utils/net
endif

#
# Add the common object files here.
#
ALLOBJS += $(KERNELOBJS) $(UTOBJS) $(SALOBJS) $(UIPOBJS) $(APPLOBJS) $(UTILSOBJS)\
           $(UIOBJS) $(BRDIMPLOBJ) $(PERSISOBJS) $(FLASHOBJ) $(SWITCHOBJ)\
           $(HTTPDOBJS) $(GUIOBJS) $(XCMDOBJS) $(XCOMMANDSOBJS) $(LINUX_OBJS)\
           $(SDKCLIOBJS) $(EDITLINEOBJS) $(SHROBJS) $(RAMOBJS) $(BSPOBJS)

#
# Add optional code.  Each option will add to ALLOBJS with its Makefile,
# and some append to SRCDIRS and/or CFE_INC.
#

ifeq ($(strip ${CFG_INTERRUPTS}),1)
CFLAGS += -DCFG_INTERRUPTS=1
endif

# Add phy support list
#ifndef BCM_PHY_LIST
#BCM_PHY_LIST=84848 54280 54282 QSGMII TSCE
#endif
#CFLAGS += $(foreach phy,$(BCM_PHY_LIST), -DINCLUDE_PHY_$(phy))

# specify endian of system and setup the flag

ifeq ($(strip ${CFG_LITTLE}),1)
CFLAGS += -DCFG_LITTLE_ENDIAN=1 -DCFG_BIG_ENDIAN=0
else
CFLAGS += -DCFG_LITTLE_ENDIAN=0 -DCFG_BIG_ENDIAN=1
endif

#
# Make the paths
#

INCDIRS = $(patsubst %,-I%,$(subst :, ,$(BSP_INC) $(CFE_INC)))

VPATH = $(SRCDIRS)

#
# This is the makefile's main target.  Note that we actually
# do most of the work in 'ALL' (from the build Makefile) not 'all'.
#

#all : build_date.c makereg pcidevs_data2.h ALL
all : build_date.c ALL

.PHONY : all
.PHONY : ALL
.PHONY : build_date.c

#
# Build the local tools that we use to construct other source files
#

HOST_CC = gcc
HOST_CFLAGS = -g -Wall -Werror -Wstrict-prototypes -Wmissing-prototypes

bin2codefile : ${TOP}/tools/bin2codefile.c
	$(HOST_CC) $(HOST_CFLAGS) -o bin2codefile ${TOP}/tools/bin2codefile.c

build_date.c :
	@echo "const char *builddate = \"`date`\";" > build_date.c
	@echo "const char *builduser = \"`whoami`@`hostname`\";" >> build_date.c

preparation:
	rm -f $(BUILD_DIR)/lib/libpcmphyctrl_physelect.*
	rm -f $(TOP)/sdk/make/src/soc/phy/phyident.*

#
# Make a define for the board name
#

CFLAGS += -D_$(patsubst "%",%,${CFG_BOARDNAME})_

#
# Make a define for the chip family name
#

CFLAGS += -D_$(shell echo $(CHIP) | tr a-z A-Z)_

#
# Rules for building normal CFE files
#

LIBCFE = $(LIB_PATH)/libcfe.a
LIBCFERAM = $(LIB_PATH)/libcferam.a


xcommands : $(patsubst %, %_cbkgen, ${XCOMMAND_XML_TABLES})
%.xml_cbkgen:
	@echo Convert $(patsubst %_cbkgen, %, $@)
	@perl $(XCOMMANDS_PARSER) $(patsubst %_cbkgen, %, $@) $(XCOMMAND_DEF_TABLES)


#
# Dependcy Rule
#

-include $(wildcard $(patsubst %.o, %.d, $(ALLOBJS)))

#
# Generic Complile
#

%.o : %.c
	@echo ""
	$(GCC) $(CFE_CFLAGS) $(CFLAGS) -include $(BUILD_DIR)/conf.h -MP -MD -o $@ $< -c
ifeq ($(RELEASE_BUILD),1)
	$(QUIET)$(TOP)/tools/release_tool.py --target=release_sdklibs --input_file=$(patsubst %.o,%.d, $@) --source_directory=$(SDK_DIR) --destination_directory=$(SDK_RELEASE_DIR)
endif

%.o : %.S
	@echo ""
	$(GCCAS) -D__ASSEMBLER__ $(CFE_CFLAGS) $(CFLAGS) -include $(BUILD_DIR)/conf.h -MP -MD -o $@ $< -c

#
# Rules for building ZIPSTART
#

LIBZIPSTART = libzipstart.a

ZS_%.o : %.c
	$(CC) $(ENDIAN) $(ZIPSTART_CFLAGS) -D_ZIPSTART_ $(CFLAGS) -o $@ $<

ZS_%.o : %.S
	$(CC) $(ENDIAN) $(ZIPSTART_CFLAGS) -D_ZIPSTART_ $(CFLAGS) -o $@ $<

$(LIBPHYECD) : $(LIBPHYECDOBJ)
ifneq (,$(LIBPHYECDOBJ))
	rm -f ${LIB_PATH}/$(LIBPHYECD)
	$(GAR) cr ${LIB_PATH}/$(LIBPHYECD) $(LIBPHYECDOBJ)
	$(GRANLIB) ${LIB_PATH}/$(LIBPHYECD)
endif

#
# This rule constructs "libcfe.a" which contains most of the object files.
# And "libcferam.a" gathers all stuff to be put in RAM
#

$(LIBCFE) : $(ALLOBJS) header_link pcmphyctrl_physelect pcmphyctrl_release_build
	rm -f $(LIBCFE)
	$(GAR) cr $(LIBCFE) $(filter-out $(RAMOBJS),$(ALLOBJS))
	$(GRANLIB) $(LIBCFE)

$(LIBCFERAM) : $(RAMOBJS) header_link pcmphyctrl_physelect pcmphyctrl_release_build
	rm -f $(LIBCFERAM)
	$(GAR) cr $(LIBCFERAM) $(RAMOBJS)
	$(GRANLIB) $(LIBCFERAM)
