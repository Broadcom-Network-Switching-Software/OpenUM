/*
 * 
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 *
 * File:    low_mem_ca9.S
 * Purpose: H/W exception handling
 */

#define ChipcommonA_CoreCtrl     0x18000008
#define ChipcommonA_ClkDiv       0x180000a4

#define IHOST_PROC_CLK_WR_ACCESS 0x19000000
#define IHOST_PROC_CLK_PLLARMA   0x19000c00
#define IHOST_PROC_CLK_PLLARMA__pllarm_lock 28
#define IHOST_PROC_CLK_PLLARMA__pllarm_soft_post_resetb 1

#define IHOST_PROC_CLK_PLLARMCTRL5 0x19000c20

#define IHOST_PROC_CLK_POLICY_FREQ   0x19000008
#define IHOST_PROC_CLK_POLICY_CTL    0x1900000c
#define IHOST_PROC_CLK_POLICY_CTL__GO_AC 1
#define IHOST_PROC_CLK_POLICY_CTL__GO    0
#define IHOST_PROC_CLK_CORE0_CLKGATE 0x19000200
#define IHOST_PROC_CLK_CORE1_CLKGATE 0x19000204
#define IHOST_PROC_CLK_ARM_SWITCH_CLKGATE 0x19000210
#define IHOST_PROC_CLK_ARM_PERIPH_CLKGATE 0x19000300
#define IHOST_PROC_CLK_APB0_CLKGATE  0x19000400

#define IHOST_SCU_CONTROL        0x19020000
#define IHOST_SCU_INVALIDATE_ALL 0x1902000c
#define IHOST_L2C_CACHE_ID       0x19022000

#define IPROC_WRAP_GEN_PLL_CTRL0 0x1803fc00
#define IPROC_WRAP_GEN_PLL_CTRL0__FAST_LOCK 28
#define IPROC_WRAP_GEN_PLL_CTRL1 0x1803fc04
#define IPROC_WRAP_GEN_PLL_CTRL1__NDIV_INT_R 0
#define IPROC_WRAP_GEN_PLL_CTRL2 0x1803fc08
#define IPROC_WRAP_GEN_PLL_CTRL2__CH3_MDIV_R 8
#define IPROC_WRAP_GEN_PLL_CTRL2__CH4_MDIV_R 16
#define IPROC_WRAP_GEN_PLL_CTRL3 0x1803fc0c
#define IPROC_WRAP_GEN_PLL_CTRL3__SW_TO_GEN_PLL_LOAD 28
#define IPROC_WRAP_GEN_PLL_CTRL3__LOAD_EN_CH_R 16

#define IPROC_WRAP_GEN_PLL_STATUS 0x1803fc18
#define IPROC_WRAP_GEN_PLL_STATUS__GEN_PLL_LOCK 0

#define IHOST_PROC_CLK_WR_ACCESS                        0x19000000
#define IHOST_PROC_CLK_POLICY_FREQ                      0x19000008
#define IHOST_PROC_CLK_POLICY_CTL                       0x1900000c
#define IHOST_PROC_CLK_POLICY_CTL__GO                   0
#define IHOST_PROC_CLK_POLICY_CTL__GO_AC                1

#define IHOST_PROC_CLK_CORE0_CLKGATE                    0x19000200
#define IHOST_PROC_CLK_CORE1_CLKGATE                    0x19000204
#define IHOST_PROC_CLK_ARM_SWITCH_CLKGATE               0x19000210
#define IHOST_PROC_CLK_ARM_PERIPH_CLKGATE               0x19000300
#define IHOST_PROC_CLK_APB0_CLKGATE                     0x19000400

#define IHOST_PROC_CLK_PLLARMA                          0x19000c00
#define IHOST_PROC_CLK_PLLARMA__pllarm_soft_post_resetb 1
#define IHOST_PROC_CLK_PLLARMA__pllarm_lock             28

