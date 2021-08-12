# $Id: um_link.mk,v 1.1 Broadcom SDK $
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2021 Broadcom Inc. All rights reserved.

#
# This Makefile snippet takes care of linking the firmware.
#

.PHONY: $(LDSCRIPT)

ifeq ($(strip ${CFG_GNU_TOOLCHAIN}),1)
LDFLAGS +=  -e reset -$(ENDIAN_FLAG) --gc-sections -L$(TOOLCHAIN_DIR)/$(ARMEB)-elf/lib/$(MULTILIB) -L$(TOOLCHAIN_DIR)/lib/gcc/$(ARMEB)-elf/$(GCC_VERSION)/$(MULTILIB)
LDLIBS +=  -lc -lgcc
else
LDFLAGS +=  -e reset -$(ENDIAN_FLAG) --gc-sections
LDLIBS += -lgcc -L$(TOOLCHAIN_DIR)/lib/gcc/arm-none-eabi/$(GCC_VERSION)
endif

#$(LDSCRIPT) : $(LDSCRIPT_TEMPLATE)
#	$(GCC) -E $(CFLAGS) -ansi -P - <$(LDSCRIPT_TEMPLATE) >$(LDSCRIPT)

#
# If code is to be copied out to TCM, we need a bootloader
#
CFETARGET = cfe
BOOTTARGET = boot
ifeq ($(strip ${CFG_DUAL_IMAGE}),1)
CFETARGET-2 = cfe-2
endif

#
# ZIPSTART linker stuff
#

ZZSOBJS = $(patsubst %,ZS_%,$(ZSOBJS))
ZZCRT0OBJS = $(patsubst %,ZS_%,$(ZCRT0OBJS))

$(LIBZIPSTART) : $(ZZSOBJS) $(BSPOBJS)
	rm -f $(LIBZIPSTART)
	$(GAR) cr $(LIBZIPSTART) $(ZZSOBJS)
	$(GRANLIB) $(LIBZIPSTART)

$(ZIPSTART) : $(ZZCRT0OBJS) $(LIBZIPSTART) $(ZIPSTART_LDSCRIPT) $(CFETARGET).bin.o
	$(GLD) $(ENDIAN) -o $@ -Map $@.map -g --script $(ZIPSTART_LDSCRIPT) $(ZZCRT0OBJS) $(CFETARGET).bin.o -L. -lzipstart
	$(GOBJDUMP) -d $@ > $@.dis
	$(GOBJCOPY) $(OCBINFLAGS) --output-target=binary $@ $@.bin
	$(GOBJCOPY) --input-target=binary --output-target=srec $@.bin $@.srec

#
# CFE linker stuff
#


$(CFETARGET) : $(CRT0OBJS) $(BSPOBJS) $(LIBCFE) $(LIBCFERAM) $(LIBPHYECD)  $(LDSCRIPT)
	$(GLD) -o $(CFETARGET) -Map $(CFETARGET).map $(LDFLAGS) --defsym reset_addr=$(CFG_TEXT_START) -T $(BSP_SRC)/$(LD_SCRIPT) $(filter-out $(RAMOBJS),$(CRT0OBJS)) --start-group -L. -lcfe -lcferam -L$(LIB_PATH) $(LDLIBS) --end-group
#	$(GLD) -o $(CFETARGET) -Map $(CFETARGET).map $(LDFLAGS) $(CRT0OBJS) $(BSPOBJS) -L. -lcfe -L$(LIB_PATH) -lphyecd $(PLATFORM_LIBS) $(LDLIBS)
#	$(GLD) $(ENDIAN) -o $(CFETARGET) -Map $(CFETARGET).map $(CFE_LDFLAGS) $(CRT0OBJS) $(BSPOBJS) -L. -L./lib -lcfe -lpower $(LDLIBS)
	$(GOBJDUMP) -d $(CFETARGET) > $(CFETARGET).dis
	$(GNM) $(CFETARGET) | sort > $(CFETARGET).nm

$(CFETARGET).bin : $(CFETARGET)
	$(GOBJCOPY) -O binary $(OBJCOPYOPTION) -S $(CFETARGET) $(CFETARGET).bin
#	$(GOBJCOPY) -O binary -R .reginfo -R .note -R .comment -R .mdebug -R .packet_buf -S $(CFETARGET) $(CFETARGET).bin
#	$(GOBJCOPY) $(OCBINFLAGS) --output-target=binary $(CFETARGET) $(CFETARGET).bin
#	$(GOBJCOPY) --input-target=binary --output-target=srec $(CFETARGET).bin $(CFETARGET).srec
#ifeq ($(strip ${CFG_LOADER}),1)
#	perl ${TOP}/tools/mkheader.pl $@ $(CFETARGET)-loader.bin ${CFE_VER_MAJ} ${CFE_VER_MIN} ${CFE_VER_ECO}
#endif

# for second image
ifeq ($(strip ${CFG_DUAL_IMAGE}),1)
$(CFETARGET-2) : $(CRT0OBJS) $(BSPOBJS) $(LIBCFE) $(LIBCFERAM) $(LIBPHYECD)  $(LDSCRIPT)
	$(GLD) -o $(CFETARGET-2) -Map $(CFETARGET-2).map $(LDFLAGS) --defsym reset_addr=$(CFG_TEXT_START_2) -T $(BSP_SRC)/$(LD_SCRIPT) $(filter-out $(RAMOBJS),$(CRT0OBJS)) --start-group -L. -lcfe -lcferam -L$(LIB_PATH) $(LDLIBS) --end-group
	$(GOBJDUMP) -d $(CFETARGET-2) > $(CFETARGET-2).dis
	$(GNM) $(CFETARGET-2) | sort > $(CFETARGET-2).nm

$(CFETARGET-2).bin : $(CFETARGET-2)
	$(GOBJCOPY) -O binary $(OBJCOPYOPTION) -S $(CFETARGET-2) $(CFETARGET-2).bin
endif

DATA_START = $(shell grep -m 1 data_start cfe.map | awk '{print $$1}' | sed 's/^0x0*/0x/')

DATA_OFFSET = $(shell grep -m 1 flashdata_offset cfe.map | awk '{print $$1}' | sed 's/^0x0*/0x/')

low_mem_redirect.o: low_mem.S
	$(GCCAS) -c -D__ASSEMBLER__ -DLOW_MEM_REDIRECT -DTEXT_START=0x00000000 -DDATA_START=$(DATA_START) $(CFLAGS) -o $@ $<

payload.c: $(CFETARGET)
	$(OBJCOPY) -O binary -j .text $(CFETARGET) um-payload-text
	$(OBJCOPY) -O binary -j .text2 $(CFETARGET) um-payload-text2
	$(OBJCOPY) -O binary -j .data $(CFETARGET) um-payload-data
	$(OBJCOPY) -O binary -j .flashdata $(CFETARGET) um-payload-flashdata
	$(TOP)/tools/mkpayload.py --text um-payload-text --text-start 0x00000000 --text2 um-payload-text2 --text2-start 0x00040000 --data um-payload-data --data-start $(DATA_START) --output payload.c
	rm um-payload-text um-payload-text2 um-payload-data

$(BOOTTARGET) : low_mem_redirect.o cache.o boot.o payload.o
	$(GLD) -o $(BOOTTARGET) -Map $(BOOTTARGET).map $(LDFLAGS) -T $(BSP_SRC)/um_load.lds --defsym reset_addr=$(CFG_TEXT_START) low_mem_redirect.o cache.o boot.o payload.o -L. -lcfe -L$(LIB_PATH) -lphyecd $(LDLIBS)
	$(GOBJDUMP) -d $(BOOTTARGET) > $(BOOTTARGET).dis

$(BOOTTARGET).bin : $(BOOTTARGET)
	$(GOBJCOPY) -O binary $(BOOTTARGET) $(BOOTTARGET).bin
	$(TOP)/tools/mkflashimage.pl $(BOOTTARGET).bin um-payload-flashdata $(DATA_OFFSET) $(CFG_IMG).bin
	rm um-payload-flashdata

OFMT = $(shell $(GOBJDUMP) -i | head -2 | grep elf)

$(CFETARGET).bin.o : $(CFETARGET).bin
ifeq ($(strip ${CFG_ZIPPED_CFE}),1)
	gzip -c $(CFETARGET).bin > $(CFETARGET).bin.gz
	$(GLD) $(ENDIAN) -T ${BSP_SRC}/binobj.lds -b binary --oformat $(OFMT) -o $(CFETARGET).bin.o $(CFETARGET).bin.gz
else
	$(GLD) $(ENDIAN) -T ${BSP_SRC}/binobj.lds -b binary --oformat $(OFMT) -o $(CFETARGET).bin.o $(CFETARGET).bin
endif

#
# Build the flash image
#

cfe.dis : cfe
	$(OBJDUMP) -d cfe > cfe.dis

ifeq ($(strip ${CFG_DUAL_IMAGE}),1)
${CFG_IMG}-${target}.flash : cfe.bin
	perl ${TOP}/tools/mkheader.pl $< $@ ${CFG_BOARDNAME} ${CFE_VER_MAJ} ${CFE_VER_MIN} ${CFE_VER_ECO} ${CFG_IMAGE_1_ID}
	-cp $@ ${CFG_IMG}-fw.flash
${CFG_IMG}-${target}-2.flash : ${CFETARGET-2}.bin
	perl ${TOP}/tools/mkheader.pl $< $@ ${CFG_BOARDNAME} ${CFE_VER_MAJ} ${CFE_VER_MIN} ${CFE_VER_ECO} ${CFG_IMAGE_2_ID}
	cp $@ ${CFG_IMG}-fw-2.flash
else
${CFG_IMG}-${target}.flash : cfe.bin
ifeq ($(strip ${CFG_IMAGE_ID}),1)
	perl ${TOP}/tools/mkheader.pl $< $@ ${CFG_BOARDNAME} ${CFE_VER_MAJ} ${CFE_VER_MIN} ${CFE_VER_ECO} ${CFG_IMAGE_1_ID}
else
	perl ${TOP}/tools/mkheader.pl $< $@ ${CFG_BOARDNAME} ${CFE_VER_MAJ} ${CFE_VER_MIN} ${CFE_VER_ECO}
endif
	cp $@ ${CFG_IMG}-fw.flash
endif

#	./mkflashimage -v ${ENDIAN} -B ${CFG_BOARDNAME} -V ${CFE_VER_MAJ}.${CFE_VER_MIN}.${CFE_VER_ECO} cfe.bin cfe.flash
#	$(GOBJCOPY) --input-target=binary --output-target=srec cfe.flash cfe.flash.srec

#
# Housecleaning
#

clean_wo_config:
	rm -f *.o *~ *.d $(CFETARGET) $(CFETARGET-2) $(BOOTTARGET) *.bin *.flash *.dis *.map *.image um.lds
	rm -f build_date.c
	rm -f $(LIBCFE)
	rm -f $(LIBCFERAM)
	rm -f payload.c
	rm -f binfs_file_list.c
	rm -f $(CLEANOBJS)

clean: clean_wo_config
	rm -f $(CONFIG_FILES)

distclean : clean
