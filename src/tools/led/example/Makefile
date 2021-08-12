#
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2021 Broadcom Inc. All rights reserved.
#
# CMIC LED firmware makefile.
#
# The default target is to generate the LED firmware binary files.
# All source code is under this folder.
# Type "make" to generate LED binary files.
# Type "make clean" to clean all LED binaries and compiler outputs.
#

#
# Toolchain setup for compiling CMICd/CMICm LED code.
#
Q := @

ASM := $(Q)../tools/ledasm/ledasm
HEX2BIN := $(Q)python ../tools/hex2bin.py
BIN2HEX := $(Q)perl ../tools/bin2hex.pl
BIN2H   := $(Q)python ../tools/bin2h.py
ECHO    := $(Q)echo
#
# Use native gcc for building assembler
#
ASM_GCC := $(shell which gcc)

#
# CMICx LED uses Broadcom XLDK ARM toolchain by default.
#
ifndef UM
$(error UM variable is not defined.)
endif

# Include ARM toolchain path.
-include cmicx_arm_uclibc.mk

ifndef CROSS_COMPILE
CROSS_COMPILE = $(Q)arm-linux-
endif

ifndef CC
CC = $(CROSS_COMPILE)gcc
endif

ifndef LD
LD = $(CROSS_COMPILE)ld
endif

ifndef OBJCOPY
OBJCOPY = $(CROSS_COMPILE)objcopy
endif

#
# CMICd/CMICm LED code and object files.
#
RMOBJS += $(patsubst %.asm,%.bin,$(wildcard *.asm))
RMOBJS += $(patsubst %.asm,%.c,$(wildcard *.asm))
RMOBJS += $(patsubst %.asm,%.lst,$(wildcard *.asm))
RMOBJS += $(patsubst %.asm,%.hex,$(wildcard *.asm))
OBJS += $(patsubst %.asm,%_bin.h,$(wildcard *.asm))

#
# CMICx LED code and object file.
#
CMICX_INCS = $(UM)/src/driver/led/include
CMICX_SRCS += cmicx_customer_led.c cmicx_customer_led.h
CMICX_SRCS += cmicx_customer_led.lds
CMICX_SRCS += $(UM)/src/driver/led/include/cmicx_customer_led_common.h

# Add removable objects.
RMOBJS += cmicx_customer_led.bin cmicx_customer_led.elf
RMOBJS += cmicx_customer_led.o cmicx_customer_led.map
RMOBJS += cmicx_customer_led.hex

OBJS += cmicx_customer_led_bin.h

# Default target.
all: tools $(OBJS)

tools: $(ASM)

$(ASM):
ifneq "$(wildcard *.asm)" ""
	$(MAKE) -C ../tools/ledasm CC=$(ASM_GCC)
endif

cmicx_customer_led_bin.h: $(CMICX_SRCS)
	$(ECHO) "Compiling  $< ...."
ifeq ($(findstring arm,$(CC)),)
	$(ECHO) "GNU ARM Embedded toolchain is not ready."
	$(ECHO) "Please contact Broadcom for XLDK ARM toolchain."
else
	$(CC) -O1 -g -mthumb -mcpu=cortex-m0 -I$(CMICX_INCS) -c -o cmicx_customer_led.o $<
	$(LD) -T cmicx_customer_led.lds -Bstatic -nostdlib --entry 0 -EL \
            -Map cmicx_customer_led.map \
            -o cmicx_customer_led.elf cmicx_customer_led.o
	$(OBJCOPY) -O binary cmicx_customer_led.elf cmicx_customer_led.bin
	$(BIN2HEX) cmicx_customer_led.bin cmicx_customer_led.hex
	$(BIN2H) cmicx_customer_led.bin
endif

%_bin.h: %.asm
	$(ECHO) "Compiling $< ..."
	$(ASM) $*
	$(HEX2BIN) $*.hex $*.bin
	$(BIN2H) $*.bin

clean:
	$(Q)rm -f $(OBJS) $(RMOBJS)

.PHONY: all tools clean
