# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.
#
# Build LED processor tools
#
.KEEP_STATE:

CC = gcc
RM = rm -f

CFLAGS = -g -Wall -I.

LED_TOOLS      = ledasm

all: $(LED_TOOLS)

remake: clobber all

ledasm: ledasm.c ledasm.h
	$(CC) $(CFLAGS) -o ledasm ledasm.c

clean:
	$(RM) *.o

clobber: clean
	$(RM) $(LED_TOOLS)
