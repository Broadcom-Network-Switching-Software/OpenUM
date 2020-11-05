# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.

#
# Addresses of things unless overridden
#

ifeq ($(strip ${CFG_LITTLE}),1)
ARMEB = arm
ENDIAN_FLAG=EL
ENDIAN = little
BSPFLAGS_gnu += -DLITTLE_ENDIAN -DLE_HOST
else
ARMEB = armeb
ENDIAN_FLAG=EB
ENDIAN = big
BSPFLAGS_gnu += -DBIG_ENDIAN
endif

MACHINE	= $(shell uname -m)-unknown

#
# builds on i686 machines end up with i686-pc-linux-gnu instead
# of i686-unknown-linux-gnu, rewrite as needed.
#
ifeq ($(MACHINE),i686-unknown)
MACHINE	= i686-pc
endif

MULTILIB = mcpu-cortex-r5/mthumb

# Hurricane2: CFG_TEST_START and CFG_DATA_START will be overrided.

ifeq ($(strip ${CFG_RAMAPP}),1)
CFG_TEXT_START ?= 0x00000000
endif

CFG_DATA_START ?= 0x80000000
CFG_ROM_START  ?= 0xFFFD0000

#
# Tools locations
#

ifeq ($(strip ${CFG_GNU_TOOLCHAIN}),1)
TOOLCHAIN_DIR ?= /projects/ntsw-tools/gnu/$(MACHINE)-linux-gnu/$(ARMEB)-elf
TOOLPREFIX ?= $(ARMEB)-elf-
GCC_VERSION = 4.7.0
else
TOOLCHAIN_DIR ?= /projects/ntsw-tools/gnu/gcc-arm-none-eabi-4_8-2014q1
TOOLPREFIX ?= $(ARMEB)-none-eabi-
GCC_VERSION = 4.8.3
endif
TOOLS ?= $(TOOLCHAIN_DIR)/bin

#
# BOOTRAM mode (runs from ROM vector assuming ROM is writable)
# implies no relocation.
#

ifeq ($(strip ${CFG_BOOTRAM}),1)
  CFG_RELOC = 0
endif


#
# Basic compiler options and preprocessor flags
#

#gccincdir = $(shell $(GCC) -print-file-name=include)

# Need back here ?
#sys-include = $(gccincdir)/../../../../../arm-linux/sys-include

# Add GCC lib
#ifdef USE_PRIVATE_LIBGCC
#  ifeq ("$(USE_PRIVATE_LIBGCC)", "yes")
#    PLATFORM_LIBGCC = $(OBJTREE)/arch/$(ARCH)/lib/libgcc.o
#  else
#    PLATFORM_LIBGCC = -L $(USE_PRIVATE_LIBGCC) -lgcc
#  endif
#else
#  PLATFORM_LIBGCC = -L $(shell dirname `$(GCC) $(CFLAGS) -print-libgcc-file-name`) -lgcc
#endif

#PLATFORM_LIBS += $(PLATFORM_LIBGCC)
override PATH := $(TOOLCHAIN_BIN_DIR):$(PATH)
export PLATFORM_LIBS LD_LIBRARY_PATH TOOLCHAIN_BIN_DIR

#CFLAGS += -fno-builtin -ffreestanding -nostdinc -isystem $(gccincdir) -isystem $(sys-include)
#CFLAGS += -Wall -Wstrict-prototypes -fno-stack-protector


ifeq ($(strip ${CFG_GNU_TOOLCHAIN}),1)
CFLAGS += -g -std=gnu99 -Wunused -Wall -Werror -ffunction-sections -fdata-sections -msoft-float -mtpcs-frame -m$(ENDIAN)-endian -fno-builtin -fms-extensions -DTOOLCHAIN_gnu
else
CFLAGS += -g -std=gnu99 -Wunused -Wall -Werror -fpic -msingle-pic-base -mno-pic-data-is-text-relative -ffunction-sections -fdata-sections -msoft-float -mtpcs-frame -m$(ENDIAN)-endian -fno-builtin -fms-extensions
endif

# arm architecture
CFLAGS += -D__ARM__ -mthumb -mcpu=cortex-r4 -Wa,-meabi=5

ifeq ($(strip ${CFG_GNU_TOOLCHAIN}),1)
GLD = $(TOOLS)/$(TOOLPREFIX)ld-new
GCC = $(TOOLS)/$(TOOLPREFIX)gcc-new
GAS = $(TOOLS)/$(TOOLPREFIX)as-new
GCCAS = $(TOOLS)/$(TOOLPREFIX)gcc-new
else
GLD = $(TOOLS)/$(TOOLPREFIX)ld
GCC = $(TOOLS)/$(TOOLPREFIX)gcc
GAS = $(TOOLS)/$(TOOLPREFIX)as
GCCAS = $(TOOLS)/$(TOOLPREFIX)gcc
endif
GAR = $(TOOLS)/$(TOOLPREFIX)ar
GOBJCOPY = $(TOOLS)/$(TOOLPREFIX)objcopy
GOBJDUMP = $(TOOLS)/$(TOOLPREFIX)objdump
GSTRIP = $(TOOLS)/$(TOOLPREFIX)strip
GRANLIB = $(TOOLS)/$(TOOLPREFIX)ranlib
GNM = $(TOOLS)/$(TOOLPREFIX)nm
GCXX = $(TOOLS)/$(TOOLPREFIX)g++

#
# Check for 64-bit mode
#

ifeq ($(strip ${CFG_MLONG64}),1)
  CFLAGS += -mlong64 -D__long64
endif

#
# Determine parameters for the linker script, which is generated
# using the C preprocessor.
#
# Supported combinations:
#
#  CFG_RAMAPP   CFG_UNCACHED   CFG_RELOC   Description
#    Yes        YesOrNo        MustBeNo    CFE as a separate "application"
#    No         YesOrNo        Yes         CFE relocates to RAM as firmware
#    No         YesOrNo        No          CFE runs from flash as firmware
#

# NorthStar: ./cfe.lds is dynamiclly generated

LDSCRIPT = ./src/um.lds
#LDFLAGS += -g --script $(LDSCRIPT) -pie -Ttext ${CFG_TEXT_START} --stub-group-size=4
LDSCRIPT_TEMPLATE = ${BSP_SRC}/um_ldscript.template

# NorthStar: ?
ifeq ($(strip ${CFG_UNCACHED}),1)
#  GENLDFLAGS += -DCFG_RUNFROMKSEG0=0
else
#  GENLDFLAGS += -DCFG_RUNFROMKSEG0=1
endif

# NorthStar: ?
ifeq ($(strip ${CFG_RAMAPP}),1)
   GENLDFLAGS += -DCFG_RAMAPP=1
#   GENLDFLAGS += -DCFG_RUNFROMKSEG0=1
else 
 ifeq ($(strip ${CFG_RELOC}),0)
    ifeq ($(strip ${CFG_BOOTRAM}),1)
      GENLDFLAGS += -DCFG_BOOTRAM=1
    else
      GENLDFLAGS += -DCFG_BOOTRAM=0
    endif
  else
    CFLAGS += -membedded-pic -mlong-calls 
    GENLDFLAGS += -DCFG_EMBEDDED_PIC=1
    LDFLAGS +=  --embedded-relocs
  endif
endif

#
# Add GENLDFLAGS to CFLAGS (we need this stuff in the C code as well)
#

# CFLAGS += ${GENLDFLAGS}
CFLAGS += -DCFE_TEXT_START=${CFG_TEXT_START}

#
# Add the text/data/ROM addresses to the GENLDFLAGS so they
# will make it into the linker script.
#

GENLDFLAGS += -DCONFIG_SYS_TEXT_BASE=${CFG_TEXT_START}
GENLDFLAGS += -DCFE_ROM_START=${CFG_ROM_START}
GENLDFLAGS += -DCFE_TEXT_START=${CFG_TEXT_START}
GENLDFLAGS += -DCFE_DATA_START=${CFG_DATA_START}

