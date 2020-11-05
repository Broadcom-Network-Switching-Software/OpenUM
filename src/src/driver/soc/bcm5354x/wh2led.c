/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"


/* Change the port name to the lport# */
#define SOC_PORT_NAME(unit, port)       (lport)

/* from sdk/include/sal/compiler.h */
#define COMPILER_REFERENCE(_a)    ((void)(_a))
#define COMPILER_ATTRIBUTE(_a)    __attribute__ (_a)
#ifndef FUNCTION_NAME
#define FUNCTION_NAME() (__FUNCTION__)
#endif

#include <shared/bsl.h>
#include <shared/bslenum.h>
#include <shared/bsltypes.h>

#define BSL_LS_SOC_SERIAL_LED_DEBUG

#if defined(LOG_VERBOSE) && defined(BSL_LS_SOC_SERIAL_LED_DEBUG)
/* Outout the debug message with LOG_VERBOSE() */
#else
#undef LOG_VERBOSE
#define LOG_VERBOSE(ls_, stuff_)        
#endif


/*
*   Be careful, this LED_LEFT_LINK_OFFSET need to be redefined manually in *led*.asm
*/
#define LED_LEFT_LINK_OFFSET    0xF0
#define LED_START_PORT_0        0xF2
#define LED_END_PORT_0          0xF3
#define LED_START_PORT_1        0xF4
#define LED_END_PORT_1          0xF5
#define LED_OUT_BITS            0xF6

#define DEFAULT_BOARD			"BCM953547R"


typedef struct {
    uint8 i[64];
} u8a_t;


extern bcm5354x_sw_info_t wh2_sw_info;


/* Left LED(first) : Link,   Right LED(second) : TX/RX activity */
static const uint8 wh2_53547_led0[] = {
 0x16, 0xF2, 0x61, 0xE1, 0x16, 0xE1, 0x06, 0x5A,
 0xD8, 0x70, 0x50, 0x06, 0xF3, 0xD8, 0x70, 0x1D,
 0x06, 0xF5, 0xD8, 0x70, 0x25, 0x06, 0xF2, 0x16,
 0xF3, 0x61, 0xE1, 0x77, 0x34, 0x06, 0xF4, 0x16,
 0xF5, 0x61, 0xE1, 0x77, 0x34, 0x02, 0x5A, 0x12,
 0x5A, 0xD1, 0x70, 0x50, 0x02, 0x5A, 0x12, 0x5A,
 0x61, 0xE1, 0x77, 0x34, 0x28, 0x60, 0xE0, 0x67,
 0x8B, 0x75, 0x3F, 0x67, 0x78, 0x77, 0x41, 0x67,
 0x5D, 0x06, 0xE0, 0x16, 0xE1, 0xD1, 0x70, 0x04,
 0x75, 0x4D, 0x80, 0x77, 0x34, 0x90, 0x77, 0x34,
 0x12, 0xE3, 0x85, 0x05, 0xD2, 0x05, 0x71, 0x5A,
 0x52, 0x00, 0x16, 0xF6, 0x39, 0x32, 0x00, 0x32,
 0x01, 0xB7, 0x97, 0x75, 0x67, 0x77, 0x69, 0x77,
 0x71, 0x16, 0xE3, 0xDA, 0x05, 0x70, 0xA1, 0x77,
 0x93, 0x32, 0x08, 0x97, 0x75, 0x9A, 0x77, 0xA1,
 0x12, 0xA0, 0xFE, 0xE0, 0x15, 0x1A, 0x01, 0x75,
 0x81, 0x77, 0x83, 0x16, 0xE3, 0xDA, 0x02, 0x75,
 0x93, 0x77, 0x9A, 0x12, 0xA0, 0xFE, 0xE0, 0x15,
 0x1A, 0x01, 0x57, 0x22, 0x00, 0x87, 0x22, 0x00,
 0x87, 0x57, 0x22, 0x01, 0x87, 0x22, 0x01, 0x87,
 0x57, 0x16, 0xF0, 0x21, 0x87, 0xAA, 0x01, 0x21,
 0x87, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void
bcm5354x_load_led_program(uint8 unit)
{
    const uint8 *led_0_program;
    int i, offset, led_0_code_size;
    int byte_count0 = 0;
    /*
    *   led_option = 1;       -->left link
    *   led_option = 2;       -->right link
    */
    uint8 led_option = 1;

    uint8 led_0_program_3[256];
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED       
    sys_error_t sal_config_rv = SYS_OK;
    const char *board_name = NULL;
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED */      

	CMIC_LEDUP0_PORT_ORDER_REMAPr_t cmic_ledup0_port_order_remap;
	CMIC_LEDUP0_PROGRAM_RAMr_t cmic_ledup0_program_ram;
	CMIC_LEDUP0_DATA_RAMr_t cmic_ledup0_data_ram;
	CMIC_LEDUP0_CTRLr_t cmic_ledup0_ctrl;
	
	u8a_t port_remap_0;
#if 0
    TOP_PARALLEL_LED_CTRLr_t top_parallel_led_ctrl;
#endif

    LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:%s..:\n", __FILE__, __func__));
    
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED       
    board_name = sal_config_get(SAL_CONFIG_BOARD_NAME);
    if (board_name != NULL) {
        sal_printf("Vendor Config : board_name=%s\n", board_name);
    } else {
        board_name = NULL; 
        return;
    }  
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED   */

    /* By default, using BCM953547K board, 24 LEDs for front ports */
    /*  port 24th output first
        PGW_GE4P[P0~P3] -> SLED[0~3]
        PGW_GE12P_1[P18~P21] -> SLED[4~7]
        PGW_GE12P_1[P22~P25] -> SLED[8~11]
        PGW_GE12P_1[P26~P29] -> SLED[12~15]
        PGW_GE12P_0[P2~P5] -> SLED[16~19]
        PGW_GE12P_0[P6~P9] -> SLED[20~23]
        PGW_GE12P_0[P10~P13] -> SLED[24~27]
        ---------- remapping to ------------
        PGW_GE4P[P0~P3] -> SLED[0~3]            -> [25-28]
        PGW_GE12P_1[P18~P21] -> SLED[4~7]       -> [12-9]
        PGW_GE12P_1[P22~P25] -> SLED[8~11]      -> [8-5]
        PGW_GE12P_1[P26~P29] -> SLED[12~15]     -> [4-1]
        PGW_GE12P_0[P2~P5] -> SLED[16~19]       -> [13-16]
        PGW_GE12P_0[P6~P9] -> SLED[20~23]       -> [17-20]
        PGW_GE12P_0[P10~P13] -> SLED[24~27]     -> [21-24]
    */
    
    port_remap_0 = (u8a_t){i:
        { 
        (25),   (26),	(27),   (28),
        (12),   (11),	(10),   (9),   
        (8),    (7),	(6),    (5),
		(4),    (3),	(2),    (1),
		(13),   (14),   (15),   (16),
		(17),   (18),	(19),   (20),
		(21),   (22),   (23),   (24),
		(0),    (0),    (0),    (0),
		(0),    (0),    (0),    (0),
        (0),    (0),    (0),    (0),
        (0),    (0),    (0),    (0),
        (0),    (0),    (0),    (0),
        (0),    (0),    (0),    (0),
        (0),    (0),    (0),    (0),
        (0),    (0),    (0),    (0),
        (0),    (0),    (0),    (0),
        }
    };
        
    /* The LED scanning out order is
     * LED0_chain: PGW_GE12P_1, PGW_GE12P_0 , amac x1, 
     */
    if(sal_strcmp(board_name, DEFAULT_BOARD) == 0){
		/* using the default BCM953547R board, 24 port LEDs */
        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:board_name==%s\n", __func__, board_name));
        
        port_remap_0 = (u8a_t){i:
            { 
            (25),   (26),	(27),   (28),
            (12),   (11),	(10),   (9),   
            (8),    (7),	(6),    (5),
			(4),    (3),	(2),    (1),
			(13),   (14),   (15),   (16),
			(17),   (18),	(19),   (20),
			(21),   (22),   (23),   (24),
			(0),    (0),    (0),    (0),
			(0),    (0),    (0),    (0),
            (0),    (0),    (0),    (0),
            (0),    (0),    (0),    (0),
            (0),    (0),    (0),    (0),
            (0),    (0),    (0),    (0),
            (0),    (0),    (0),    (0),
            (0),    (0),    (0),    (0),
            (0),    (0),    (0),    (0),
            }
        };
    }else if(sal_strcmp(board_name, "BCM953549K") == 0){
        if( (sal_strcmp(sku_port_config->op_str, OPTION_13) == 0) || 
            (sal_strcmp(sku_port_config->op_str, OPTION_17) == 0) )
        {
            /* port 24th output first
            PGW_GE4P[P0~P3] -> SLED[0~3]
            PGW_GE12P_1[P18~P21] -> SLED[4~7]
            PGW_GE12P_1[P22~P25] -> SLED[8~11]
            PGW_GE12P_1[P26~P29] -> SLED[12~15]
            PGW_GE12P_0[P2~P5] -> SLED[16~19]
            PGW_GE12P_0[P6~P9] -> SLED[20~23]
            PGW_GE12P_0[P10~P13] -> SLED[24~27]
            ---------- remapping to ------------
            PGW_GE4P[P0~P3] -> SLED[0~3]            -> [25-28]
            PGW_GE12P_1[P18~P21] -> SLED[4~7]       -> [12-9]
            PGW_GE12P_1[P22~P25] -> SLED[8~11]      -> [8-5]
            PGW_GE12P_1[P26~P29] -> SLED[12~15]     -> [4-1]        //STRAP GPHY OFF
            PGW_GE12P_0[P2~P5] -> SLED[16~19]       -> [13-16]
            PGW_GE12P_0[P6~P9] -> SLED[20~23]       -> [17-20]
            PGW_GE12P_0[P10~P13] -> SLED[24~27]     -> [21-24]
            */
            
            port_remap_0 = (u8a_t){i:
                { 
                (25),   (26),	(27),   (28),
                (12),   (11),	(10),   (9),   
                (8),    (7),	(6),    (5),
    			(0),    (0),	(0),    (0),
    			(13),   (14),   (15),   (16),
    			(17),   (18),	(19),   (20),
    			(21),   (22),   (23),   (24),
    			(0),    (0),    (0),    (0),
    			(0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                }
            };
        }else if(   (sal_strcmp(sku_port_config->op_str, OPTION_15) == 0) || 
                (sal_strcmp(sku_port_config->op_str, OPTION_16) == 0) ) 
        {
            /*  port 24th output first
            PGW_GE4P[P0~P3] -> SLED[0~3]
            PGW_GE12P_1[P18~P21] -> SLED[4~7]
            PGW_GE12P_1[P22~P25] -> SLED[8~11]
            PGW_GE12P_1[P26~P29] -> SLED[12~15]
            PGW_GE12P_0[P2~P5] -> SLED[16~19]
            PGW_GE12P_0[P6~P9] -> SLED[20~23]
            PGW_GE12P_0[P10~P13] -> SLED[24~27]
            ---------- remapping to ------------
            PGW_GE4P[P0~P3] -> SLED[0~3]            -> [25-28]
            PGW_GE12P_1[P18~P21] -> SLED[4~7]       -> [12-9]
            PGW_GE12P_1[P22~P25] -> SLED[8~11]      -> [8-5]
            PGW_GE12P_1[P26~P29] -> SLED[12~15]     -> [4-1]    //QGPHY Limited
            PGW_GE12P_0[P2~P5] -> SLED[16~19]       -> [13-16]
            PGW_GE12P_0[P6~P9] -> SLED[20~23]       -> [17-20]
            PGW_GE12P_0[P10~P13] -> SLED[24~27]     -> [21-24]
            */
            
            port_remap_0 = (u8a_t){i:
                { 
                (25),   (26),	(27),   (28),
                (12),   (11),	(10),   (9),   
                (8),    (7),	(6),    (5),
    			(4),    (3),	(0),    (0),
    			(13),   (14),   (15),   (16),
    			(17),   (18),	(19),   (20),
    			(21),   (22),   (23),   (24),
    			(0),    (0),    (0),    (0),
    			(0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                }
            };
        }else{
        
            /* By default, using BCM953549K board, 8 LEDs for front ports */
            /*  port 24th output first
                PGW_GE4P[P0~P3] -> SLED[0~3]
                PGW_GE12P_1[P18~P21] -> SLED[4~7]
                PGW_GE12P_1[P22~P25] -> SLED[8~11]
                PGW_GE12P_1[P26~P29] -> SLED[12~15]
                PGW_GE12P_0[P2~P5] -> SLED[16~19]
                PGW_GE12P_0[P6~P9] -> SLED[20~23]
                PGW_GE12P_0[P10~P13] -> SLED[24~27]
                ---------- remapping to ------------
                PGW_GE4P[P0~P3] -> SLED[0~3]            -> [25-28]
                PGW_GE12P_1[P18~P21] -> SLED[4~7]       -> [12-9]
                PGW_GE12P_1[P22~P25] -> SLED[8~11]      -> [8-5]
                PGW_GE12P_1[P26~P29] -> SLED[12~15]     -> [4-1]
                PGW_GE12P_0[P2~P5] -> SLED[16~19]       -> [13-16]
                PGW_GE12P_0[P6~P9] -> SLED[20~23]       -> [17-20]
                PGW_GE12P_0[P10~P13] -> SLED[24~27]     -> [21-24]
            */
            port_remap_0 = (u8a_t){i:
                { 
                (25),   (26),	(27),   (28),
                (12),   (11),	(10),   (9),   
                (8),    (7),	(6),    (5),
        		(4),    (3),	(2),    (1),
        		(13),   (14),   (15),   (16),
        		(17),   (18),	(19),   (20),
        		(21),   (22),   (23),   (24),
        		(0),    (0),    (0),    (0),
        		(0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                }
            };
        }
    }else if(sal_strcmp(board_name, "BCM953547K") == 0){
        if( (sal_strcmp(sku_port_config->op_str, OPTION_1) == 0) || 
            (sal_strcmp(sku_port_config->op_str, OPTION_5) == 0) ||
            (sal_strcmp(sku_port_config->op_str, OPTION_7) == 0) || 
            (sal_strcmp(sku_port_config->op_str, OPTION_11) == 0) ||
            (sal_strcmp(sku_port_config->op_str, OPTION_13) == 0) || 
            (sal_strcmp(sku_port_config->op_str, OPTION_17) == 0)  )
    	{	    
    		LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:sku_port_config->op_str == %s(op1, 5, 7, 11, 13, 17)\n", __func__, sku_port_config->op_str));
          
            /* port 24th output first
            PGW_GE4P[P0~P3] -> SLED[0~3]
            PGW_GE12P_1[P18~P21] -> SLED[4~7]
            PGW_GE12P_1[P22~P25] -> SLED[8~11]
            PGW_GE12P_1[P26~P29] -> SLED[12~15]
            PGW_GE12P_0[P2~P5] -> SLED[16~19]
            PGW_GE12P_0[P6~P9] -> SLED[20~23]
            PGW_GE12P_0[P10~P13] -> SLED[24~27]
            ---------- remapping to ------------
            PGW_GE4P[P0~P3] -> SLED[0~3]            -> [25-28]
            PGW_GE12P_1[P18~P21] -> SLED[4~7]       -> [12-9]
            PGW_GE12P_1[P22~P25] -> SLED[8~11]      -> [8-5]
            PGW_GE12P_1[P26~P29] -> SLED[12~15]     -> [4-1]        //STRAP GPHY OFF
            PGW_GE12P_0[P2~P5] -> SLED[16~19]       -> [13-16]
            PGW_GE12P_0[P6~P9] -> SLED[20~23]       -> [17-20]
            PGW_GE12P_0[P10~P13] -> SLED[24~27]     -> [21-24]
            */
            
            port_remap_0 = (u8a_t){i:
                { 
                (25),   (26),	(27),   (28),
                (12),   (11),	(10),   (9),   
                (8),    (7),	(6),    (5),
    			(0),    (0),	(0),    (0),
    			(13),   (14),   (15),   (16),
    			(17),   (18),	(19),   (20),
    			(21),   (22),   (23),   (24),
    			(0),    (0),    (0),    (0),
    			(0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                }
            };
    	}else if(
        	(sal_strcmp(sku_port_config->op_str, OPTION_3) == 0) || 
            (sal_strcmp(sku_port_config->op_str, OPTION_4) == 0) ||
            (sal_strcmp(sku_port_config->op_str, OPTION_9) == 0) || 
            (sal_strcmp(sku_port_config->op_str, OPTION_10) == 0) ||
            (sal_strcmp(sku_port_config->op_str, OPTION_15) == 0) || 
            (sal_strcmp(sku_port_config->op_str, OPTION_16) == 0)  )
    	{
    		LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:sku_port_config->op_str == %s(op3, 4, 9, 10, 15, 16)\n", __func__, sku_port_config->op_str));
            
    		/*  port 24th output first
            PGW_GE4P[P0~P3] -> SLED[0~3]
            PGW_GE12P_1[P18~P21] -> SLED[4~7]
            PGW_GE12P_1[P22~P25] -> SLED[8~11]
            PGW_GE12P_1[P26~P29] -> SLED[12~15]
            PGW_GE12P_0[P2~P5] -> SLED[16~19]
            PGW_GE12P_0[P6~P9] -> SLED[20~23]
            PGW_GE12P_0[P10~P13] -> SLED[24~27]
            ---------- remapping to ------------
            PGW_GE4P[P0~P3] -> SLED[0~3]            -> [25-28]
            PGW_GE12P_1[P18~P21] -> SLED[4~7]       -> [12-9]
            PGW_GE12P_1[P22~P25] -> SLED[8~11]      -> [8-5]
            PGW_GE12P_1[P26~P29] -> SLED[12~15]     -> [4-1]    //QGPHY Limited
            PGW_GE12P_0[P2~P5] -> SLED[16~19]       -> [13-16]
            PGW_GE12P_0[P6~P9] -> SLED[20~23]       -> [17-20]
            PGW_GE12P_0[P10~P13] -> SLED[24~27]     -> [21-24]
            */
            
            port_remap_0 = (u8a_t){i:
                { 
                (25),   (26),	(27),   (28),
                (12),   (11),	(10),   (9),   
                (8),    (7),	(6),    (5),
    			(4),    (3),	(0),    (0),
    			(13),   (14),   (15),   (16),
    			(17),   (18),	(19),   (20),
    			(21),   (22),   (23),   (24),
    			(0),    (0),    (0),    (0),
    			(0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                (0),    (0),    (0),    (0),
                }
            };
        }else{
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:sku_port_config->op_str == %s(default remapping )\n", __func__, sku_port_config->op_str));
            
        }
    }else{
        sal_printf("%s..: Unsupported board_name=%s\n", __func__, board_name);
    }
        

    for (i = 0; i < (sizeof(port_remap_0.i)/(4 * sizeof(port_remap_0.i[0]))); i++) {
         CMIC_LEDUP0_PORT_ORDER_REMAPr_CLR(cmic_ledup0_port_order_remap);    
         CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_0f_SET(cmic_ledup0_port_order_remap, port_remap_0.i[(i*4)]);
         CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(cmic_ledup0_port_order_remap, port_remap_0.i[(i*4)+1]);
         CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_2f_SET(cmic_ledup0_port_order_remap, port_remap_0.i[(i*4)+2]);
         CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(cmic_ledup0_port_order_remap, port_remap_0.i[(i*4)+3]);
         WRITE_CMIC_LEDUP0_PORT_ORDER_REMAPr(unit, i, cmic_ledup0_port_order_remap);
    }
    
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED       
    sal_config_rv = sal_config_uint8_get(SAL_CONFIG_LED_OPTION, &led_option);
    if (sal_config_rv == SYS_OK) {
        sal_printf("Vendor Config : Overwrite serial led option with value %d.\n", led_option);
    }

    byte_count0 = sal_config_bytes_get(SAL_CONFIG_LED_PROGRAM, led_0_program_3, 256);
#endif /* CFG_VENDOR_CONFIG_SUPPORT_INCLUDED   */


    if(sal_strcmp(board_name, DEFAULT_BOARD) == 0){

        LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:Default LED Program: board_name==%s\n", __func__, board_name));
        /* Default LED Program for BCM953547R */    
        led_0_program = wh2_53547_led0;
        led_0_code_size = sizeof(wh2_53547_led0);
    }else{
        //sal_printf("board_name mis-match. Set default led_0_program.\n");
        /* Default LED Program for BCM953547R */    
        led_0_program = wh2_53547_led0;
        led_0_code_size = sizeof(wh2_53547_led0);
    }
        
    if ((led_option == 3) && byte_count0) {
        sal_printf("Vendor Config : Load customer LED ucdoe with length %d.\n", byte_count0);
        led_0_program = led_0_program_3;
        led_0_code_size = sizeof(led_0_program_3);
    }
        
#define LED_RAM_SIZE     0x100
	CMIC_LEDUP0_DATA_RAMr_CLR(cmic_ledup0_data_ram);

    for (offset = 0; offset < LED_RAM_SIZE ; offset++) {
        CMIC_LEDUP0_PROGRAM_RAMr_CLR(cmic_ledup0_program_ram);
        CMIC_LEDUP0_PROGRAM_RAMr_DATAf_SET(cmic_ledup0_program_ram, (offset >= led_0_code_size) ? 0 : *(led_0_program + offset));
        WRITE_CMIC_LEDUP0_PROGRAM_RAMr(unit, offset, cmic_ledup0_program_ram);
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, offset, cmic_ledup0_data_ram);
        
    }
    
    if(led_option != 3){
        /*
        * when led_option=1, (LED_LEFT_LINK_OFFSET) =1, else
        * when led_option=2, (LED_LEFT_LINK_OFFSET) =0
        */
        CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, (led_option==1) ? 1:0);
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_LEFT_LINK_OFFSET, cmic_ledup0_data_ram);
#if UM_DEBUG
        READ_CMIC_LEDUP0_DATA_RAMr(unit, LED_LEFT_LINK_OFFSET, cmic_ledup0_data_ram);
        sal_printf("cmic_ledup0_data_ram LED_LEFT_LINK_OFFSET=0x%02x ", cmic_ledup0_data_ram.v[0]);              
#endif

        if(sal_strcmp(board_name, DEFAULT_BOARD) == 0){
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:board_name==%s\n", __func__, board_name));
            
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 1);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_0, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 4);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_0, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 5);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_1, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 28);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_1, cmic_ledup0_data_ram);
            
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 48);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_OUT_BITS, cmic_ledup0_data_ram);
            
        }else if(sal_strcmp(board_name, "BCM953549K") == 0){
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:board_name==%s\n", __func__, board_name));
            
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 1);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_0, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 4);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_0, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 9);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_1, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 12);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_1, cmic_ledup0_data_ram);
            
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 16);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_OUT_BITS, cmic_ledup0_data_ram);
            
        }else if(sal_strcmp(board_name, "BCM953547K") == 0){
            LOG_VERBOSE(BSL_LS_SOC_COMMON,("%s..:board_name==%s\n", __func__, board_name));
            
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 1);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_0, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 4);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_0, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 5);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_1, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 28);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_1, cmic_ledup0_data_ram);
            
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 48);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_OUT_BITS, cmic_ledup0_data_ram);
        }else{
            sal_printf("board_name mis-match. Set default led data ram.\n");
        
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 1);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_0, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 4);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_0, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 5);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_1, cmic_ledup0_data_ram);
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 28);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_1, cmic_ledup0_data_ram);
            
            CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 48);
            WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_OUT_BITS, cmic_ledup0_data_ram);
        }
        
    }else{
        sal_printf("led_option=0x%02x: using user defined LED program\n", led_option);
        
        /* By default, use Left LED as link LED. */
        CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, (led_option==3) ? 1:0);
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_LEFT_LINK_OFFSET, cmic_ledup0_data_ram);
        
        CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 1);
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_0, cmic_ledup0_data_ram);
        CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 4);
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_0, cmic_ledup0_data_ram);
        CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 5);
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_1, cmic_ledup0_data_ram);
        CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 28);
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_1, cmic_ledup0_data_ram);
        
        CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 48);
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_OUT_BITS, cmic_ledup0_data_ram);
    }

#if 0
    /* led_current_sel=0x3, led_faultdet_pdwn_l=0x3f */
    /* led_ser2par_sel=1 */
    TOP_PARALLEL_LED_CTRLr_CLR(top_parallel_led_ctrl);
    TOP_PARALLEL_LED_CTRLr_LED_CURRENT_SELf_SET(top_parallel_led_ctrl, 3);
    TOP_PARALLEL_LED_CTRLr_LED_FAULTDET_PDWN_Lf_SET(top_parallel_led_ctrl, 0x3F);
    TOP_PARALLEL_LED_CTRLr_LED_SER2PAR_SELf_SET(top_parallel_led_ctrl, 0x1);
    WRITE_TOP_PARALLEL_LED_CTRLr(unit, top_parallel_led_ctrl);
#endif
    /* enable LED processor */
    READ_CMIC_LEDUP0_CTRLr(unit, cmic_ledup0_ctrl);
    CMIC_LEDUP0_CTRLr_LEDUP_ENf_SET(cmic_ledup0_ctrl, 1);
    WRITE_CMIC_LEDUP0_CTRLr(unit, cmic_ledup0_ctrl);
    
}


