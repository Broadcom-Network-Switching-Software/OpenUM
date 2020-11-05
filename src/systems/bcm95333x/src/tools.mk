# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.

#
# Addresses of things unless overridden
#

ifeq ($(strip ${CFG_GNU_TOOLCHAIN}),1)
include ${BUILD_DIR}/src/tools_gnu.mk
else
include ${BUILD_DIR}/src/tools_arm.mk
endif

