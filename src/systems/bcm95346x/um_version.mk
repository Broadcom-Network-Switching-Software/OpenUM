# 
# This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
# 
# Copyright 2007-2020 Broadcom Inc. All rights reserved.

#
# UM's version number
# Warning: Don't put spaces on either side of the equal signs
# Put just the version number in here.  This file is sourced 
# like a shell script in some cases!
#

# Base UM version
ifeq (${target}, rom)
CFE_VER_MAJ=1
CFE_VER_MIN=2
CFE_VER_ECO=0
else
CFE_VER_MAJ=3
CFE_VER_MIN=6
CFE_VER_ECO=1
endif
