# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.

#
# This Makefile snippet takes care of linking the firmware.
#

.PHONY: $(LDSCRIPT)


$(LDSCRIPT) : $(LDSCRIPT_TEMPLATE) 
	$(GCC) -E $(CFLAGS) -ansi -D__ASSEMBLY__ -P - <$(LDSCRIPT_TEMPLATE) >$(LDSCRIPT)

#
# If the relocation type is STATIC, we need ZIPSTART.
#
ifeq ($(strip ${CFG_RELOC}),STATIC)
  CFETARGET = ramcfe
  ZIPSTART  = cfe
else
  CFETARGET = cfe
  ZIPSTART  = zipstart
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


$(CFETARGET) : $(CRT0OBJS) $(BSPOBJS) $(LIBCFE) $(LIBPHYECD)  $(LDSCRIPT)
	$(GLD) -o $(CFETARGET) -Map $(CFETARGET).map $(LDFLAGS) $(CRT0OBJS) --start-group $(BSPOBJS) -L. -lcfe -L$(LIB_PATH) $(LDLIBS) $(PLATFORM_LIBS) --end-group
#	$(GLD) -o $(CFETARGET) -Map $(CFETARGET).map $(LDFLAGS) $(CRT0OBJS) $(BSPOBJS) -L. -lcfe -L$(LIB_PATH) -lphyecd $(PLATFORM_LIBS) $(LDLIBS)
#	$(GLD) $(ENDIAN) -o $(CFETARGET) -Map $(CFETARGET).map $(CFE_LDFLAGS) $(CRT0OBJS) $(BSPOBJS) -L. -L./lib -lcfe -lpower $(LDLIBS)
	$(GOBJDUMP) -d $(CFETARGET) > $(CFETARGET).dis
	$(GNM) $(CFETARGET) | sort > $(CFETARGET).nm

$(CFETARGET).bin : $(CFETARGET)
	$(GOBJCOPY) -O binary -R .reginfo -R .note -R .comment -R .mdebug -R .sram_data -S $(CFETARGET) $(CFETARGET).bin
#	$(GOBJCOPY) $(OCBINFLAGS) --output-target=binary $(CFETARGET) $(CFETARGET).bin
	$(GOBJCOPY) --input-target=binary --output-target=srec $(CFETARGET).bin $(CFETARGET).srec
ifeq ($(strip ${CFG_LOADER}),1)
	perl ${TOP}/tools/mkheader.pl $@ $(CFETARGET)-loader.bin ${CFE_VER_MAJ} ${CFE_VER_MIN} ${CFE_VER_ECO}
endif

OFMT = $(shell $(OBJDUMP) -i | head -2 | grep elf)

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
	$(GOBJDUMP) -d cfe > cfe.dis

${CFG_IMG}-${target}.flash : cfe.bin
	perl ${TOP}/tools/mkheader.pl $< $@ ${CFG_BOARDNAME} ${CFE_VER_MAJ} ${CFE_VER_MIN} ${CFE_VER_ECO}
	-cp $@ ${CFG_IMG}-fw.flash 
#	./mkflashimage -v ${ENDIAN} -B ${CFG_BOARDNAME} -V ${CFE_VER_MAJ}.${CFE_VER_MIN}.${CFE_VER_ECO} cfe.bin cfe.flash
#	$(GOBJCOPY) --input-target=binary --output-target=srec cfe.flash cfe.flash.srec

#
# Housecleaning
#

clean_wo_config:
	rm -f *.o *~ *.d $(CFETARGET) $(BOOTTARGET) *.bin *.flash *.dis *.map *.image um.lds
	rm -f build_date.c
	rm -f lib/libcfe.a
	rm -f payload.c
	rm -f $(CLEANOBJS)

clean: clean_wo_config
	rm -f $(CONFIG_FILES)

distclean : clean
