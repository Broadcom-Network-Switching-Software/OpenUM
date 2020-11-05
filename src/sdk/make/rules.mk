# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.

QUIET = @
DEPENDS :=  $(wildcard $(addprefix $(OBJ_OUTPUT_DIR)/,$(patsubst %.o,%.d,$(BOBJS))))  
VPATH = ${SDK_PATCH_DIR} ${SDK_DIR}

all : $(TARGETLIB)

$(TARGETLIB): $(BOBJS)
	$(QUIET)$(GAR) -rc $(TARGETLIB) $(addprefix $(OBJ_OUTPUT_DIR)/,$(BOBJS))
	$(QUIET)echo "Build $(TARGETLIB) successfully"

-include $(DEPENDS)

%.o : %.c
	$(QUIET)mkdir -p $(dir ${OBJ_OUTPUT_DIR}/$@)
	@echo "Compile $<"
	$(QUIET)$(GCC) $(OPTION_CFLAGS) $(CFLAGS) -MD -MF $(OBJ_OUTPUT_DIR)/$(patsubst %.o,%.d, $@) -o $@ $< -c
ifeq ($(RELEASE_ENABLE),1)
ifeq ($(RELEASE_BUILD),1)
	$(QUIET)$(TOP)/tools/release_tool.py --target=release_sdklibs --input_file=$(OBJ_OUTPUT_DIR)/$(patsubst %.o,%.d, $@) --source_directory=$(SDK_DIR) --destination_directory=$(SDK_RELEASE_DIR)   
endif
endif

