/*
 *
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 */
#include "system.h"

#ifdef CFG_LED_MICROCODE_INCLUDED

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

typedef struct {
    uint8 i[64];
} u8a_t;

typedef struct {
  /* The number of serdes cores, eg. 8, 7, 6, 5 (reversed) , 5, 6, 7, 8 (non-reversed) */    
  uint8 core_number;
  /* ledup scan chain may be arranged in reversed way: eg. 8, 7, 6, 5 (reversed) , 5, 6, 7, 8 (non-reversed) */
  uint8 pport_reverse; 
  /* The bitmap to indicate which core is disabled */
  pbmp_t core_disable; 
  /* The first/start physical port of first serdes core */  
  int  pport_start;
  /* The number of physical port of first serdes core */    
  int  pport_count_per_core;   
} ledup_auto_remap_entry_t;

ledup_ctrl_t int_ledup_ctrl;

/* To indicate customer firmware has overwrited generic code */
int disable_generic_led = 0;

extern bcm5357x_sw_info_t gh2_sw_info;

const const unsigned char gh2_generic[] = {

   0x02, 0x00, 0x60, 0xF1, 0x06, 0xF2, 0x28, 0x60,
   0xE0, 0xF2, 0xA0, 0x64, 0xEE, 0x12, 0x00, 0x06,
   0xE9, 0xD2, 0x02, 0x75, 0x1F, 0x06, 0xEE, 0x0A,
   0x00, 0x27, 0x32, 0x08, 0xB7, 0x77, 0x21, 0x32,
   0x08, 0x97, 0x1E, 0x00, 0x32, 0x00, 0x32, 0x01,
   0xB7, 0x97, 0x1E, 0x01, 0x61, 0xF0, 0x02, 0x00,
   0x60, 0xEF, 0x16, 0xEF, 0xDE, 0xE9, 0x70, 0x5D,
   0x07, 0x89, 0x02, 0x05, 0xE1, 0x68, 0xEE, 0x71,
   0x4A, 0x90, 0x68, 0xEE, 0x71, 0x48, 0x77, 0xC4,
   0x77, 0x78, 0x90, 0x68, 0xEE, 0x71, 0x51, 0x77,
   0xCC, 0x77, 0xDA, 0x06, 0xEF, 0xD6, 0xE9, 0x70,
   0x5D, 0x86, 0xEF, 0x77, 0x32, 0x06, 0xE0, 0x80,
   0xD6, 0xF3, 0x71, 0x06, 0x06, 0xF1, 0xD6, 0xE7,
   0x71, 0xD6, 0x86, 0xED, 0x06, 0xEA, 0xD6, 0xED,
   0x75, 0x76, 0x02, 0x00, 0x60, 0xED, 0x3E, 0xE7,
   0x16, 0xE0, 0xFA, 0x80, 0x06, 0xF0, 0x0A, 0x00,
   0x75, 0x9A, 0x0A, 0x01, 0x75, 0xAA, 0x1A, 0x00,
   0x75, 0x92, 0x05, 0xC2, 0xF0, 0xB6, 0xEB, 0x50,
   0x77, 0xDA, 0x05, 0xC2, 0x0F, 0xB6, 0xEC, 0x50,
   0x77, 0xDA, 0x1A, 0x00, 0x75, 0xA4, 0x05, 0xC2,
   0xF0, 0x50, 0x77, 0xD6, 0x05, 0xC2, 0x0F, 0x50,
   0x77, 0xD6, 0x1A, 0x00, 0x75, 0xB9, 0x05, 0xC2,
   0x0F, 0x70, 0xD6, 0x05, 0xE2, 0x01, 0x50, 0x77,
   0xDA, 0x05, 0xC2, 0xF0, 0x70, 0xD6, 0x05, 0xE2,
   0x10, 0x50, 0x77, 0xDA, 0x16, 0xF0, 0x1A, 0x00,
   0x75, 0xD6, 0x77, 0xDA, 0x07, 0x16, 0xEA, 0x99,
   0xDE, 0xED, 0x75, 0xDA, 0x77, 0xD6, 0x06, 0xE6,
   0x77, 0xEA, 0x02, 0x00, 0x12, 0x06, 0x69, 0xEE,
   0x0E, 0x00, 0x81, 0x69, 0xEE, 0x0E, 0x01, 0xF2,
   0xE2, 0x04, 0x12, 0x00, 0xDE, 0xE1, 0x70, 0xF6,
   0x09, 0x27, 0x87, 0x81, 0x77, 0xEC, 0x16, 0xE1,
   0xFE, 0xF1, 0x61, 0xF1, 0x77, 0x53, 0x00, 0x00,

};

#define LED_RAM_SIZE     0x100
/* Load Generic LED firmware */
void bcm5357x_load_firmware(uint8 unit) {

   int i;

   CMIC_LEDUP0_PROGRAM_RAMr_t cmic_ledup0_program_ram;
   CMIC_LEDUP0_DATA_RAMr_t cmic_ledup0_data_ram;
   CMIC_LEDUP1_PROGRAM_RAMr_t cmic_ledup1_program_ram;
   CMIC_LEDUP1_DATA_RAMr_t cmic_ledup1_data_ram;


   CMIC_LEDUP0_DATA_RAMr_CLR(cmic_ledup0_data_ram);
   CMIC_LEDUP1_DATA_RAMr_CLR(cmic_ledup1_data_ram);

   for (i = 0; i < LED_RAM_SIZE ; i++) {
        CMIC_LEDUP0_PROGRAM_RAMr_CLR(cmic_ledup0_program_ram);
        CMIC_LEDUP0_PROGRAM_RAMr_DATAf_SET(cmic_ledup0_program_ram, (i >= sizeof(gh2_generic)) ? 0 : gh2_generic[i]);
        WRITE_CMIC_LEDUP0_PROGRAM_RAMr(unit, i, cmic_ledup0_program_ram);
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, i, cmic_ledup0_data_ram);
                         
        CMIC_LEDUP1_PROGRAM_RAMr_CLR(cmic_ledup1_program_ram);
        CMIC_LEDUP1_PROGRAM_RAMr_DATAf_SET(cmic_ledup1_program_ram, (i >= sizeof(gh2_generic)) ? 0 : gh2_generic[i]);
        WRITE_CMIC_LEDUP1_PROGRAM_RAMr(unit, i, cmic_ledup1_program_ram);
        WRITE_CMIC_LEDUP1_DATA_RAMr(unit, i, cmic_ledup1_data_ram);
   } 
   

}

/* Load Customer LED firmware and disable all the generic LED code*/
void bcm5357x_load_customer_firmware(uint8 unit) {

    int i, byte_count;
    uint8 ext_led_program[256];

    CMIC_LEDUP0_PROGRAM_RAMr_t cmic_ledup0_program_ram;
    CMIC_LEDUP1_PROGRAM_RAMr_t cmic_ledup1_program_ram;

    byte_count = sal_config_bytes_get(SAL_CONFIG_LED_PROGRAM, ext_led_program, 256);
    sal_printf("Vendor Config : Load customer LED0 ucdoe for with length %d.\n", byte_count);
    for (i = 0; i < LED_RAM_SIZE ; i++) {
        CMIC_LEDUP0_PROGRAM_RAMr_CLR(cmic_ledup0_program_ram);
        CMIC_LEDUP0_PROGRAM_RAMr_DATAf_SET(cmic_ledup0_program_ram, (i >= byte_count) ? 0 : ext_led_program[i]);
        WRITE_CMIC_LEDUP0_PROGRAM_RAMr(unit, i, cmic_ledup0_program_ram);
    }
    byte_count = sal_config_bytes_get(SAL_CONFIG_LED_1_PROGRAM, ext_led_program, 256);
    sal_printf("Vendor Config : Load customer LED1 ucdoe for with length %d.\n", byte_count);       
    for (i = 0; i < LED_RAM_SIZE ; i++) {
        CMIC_LEDUP1_PROGRAM_RAMr_CLR(cmic_ledup1_program_ram);
        CMIC_LEDUP1_PROGRAM_RAMr_DATAf_SET(cmic_ledup1_program_ram, (i >= byte_count) ? 0 : ext_led_program[i]);
        WRITE_CMIC_LEDUP1_PROGRAM_RAMr(unit, i, cmic_ledup1_program_ram);
    } 
    /* disable generic LED */
    disable_generic_led = 1;
}
/*
*   The define below should comply with gh2_generic.asm
*/
#define EXT_CNTS                   0x80
#define PORT_DATA                  0xA0
#define PORT_NUM                   0xE0    
#define LED_BITS_PER_LED           0xE1
#define LED_BITS_LED_COLOR_0       0xE2
#define LED_BITS_LED_COLOR_1       0xE3
#define LED_BITS_LED_COLOR_2       0xE4
#define LED_BITS_LED_COLOR_3       0xE5
#define LED_BITS_LED_COLOR_OFF     0xE6
#define LED_BITS_TOTAL_COUNTS      0xE7
#define LEDS_PER_PORT              0xE9
#define LED_BLINK_PERIOD           0xEA
#define LED_TXRX_EXT_TIME_EVEN     0xEB
#define LED_TXRX_EXT_TIME_ODD      0xEC
#define LED_START_PORT_0           0xF2
#define LED_END_PORT_0             0xF3

#define LEDUP_AUTO_REMAP(u, v, x, y, z) bcm5357x_ledup_generic_auto_remap(u,v,x,y,z)

sys_error_t
bcm5357x_pport_to_ledup_port(uint8 unit, int pport, uint8 *ledup_no, uint8 *ledup_port) 
{
     int i, j;

    for (i=0; i< BCM5357X_LEDUP_NUMBER; i++) {
         for (j=0; j < int_ledup_ctrl.port_count[i]; j++) {
              if (pport == int_ledup_ctrl.pport_seq[i][j]) {
                  *ledup_no = i;
                  *ledup_port = j;
                  return SYS_OK;
              }
         }                
    }
    return SYS_ERR;
};

 
/** 
 * <b>Description:</b> Init.
 * @param uport - Port number.
 * @param bits_sec - (OUT) rate number.
 *  \li 0: No limit
 *  \li Rate value
 * @return SYS_OK
 * \n      Operation completed successfully.
 */

void bcm5357x_ledup_init(const ledup_ctrl_t *p_ledup_ctrl) {

    int i, j, pport;
    uint8 unit, lport;   
    uint8 led_option = 0;      
    

    if (p_ledup_ctrl != NULL) {
        sal_memcpy(&int_ledup_ctrl, p_ledup_ctrl, sizeof(ledup_ctrl_t));
    } else {   
        int_ledup_ctrl.leds_per_port     = 2;
        int_ledup_ctrl.bits_per_led      = 2;
        int_ledup_ctrl.num_of_color      = 2;
        int_ledup_ctrl.led_blink_period  = 50;    /* 50 *30ms = 1.5 s */
        int_ledup_ctrl.led_tx_rx_extension     = 2;     /* 2 *30ms = 60ms */          
        int_ledup_ctrl.bits_color[0]     = 0x2;   /* Green */
        int_ledup_ctrl.bits_color[1]     = 0x1;   /* Orange */          
        int_ledup_ctrl.bits_color_off    = 0x0;   /* 0, 0, 0, 0 */    
        int_ledup_ctrl.fix_bits_total[0] = 0;
        int_ledup_ctrl.fix_bits_total[1] = 0;  

        /* Sort the scan bits in uport order */  
        for (j=0; j < BCM5357X_LEDUP_NUMBER; j++) {
            int_ledup_ctrl.pport_seq[j] = sal_malloc(64);
            int_ledup_ctrl.port_count[j] = 0;
        }
    
        for (i=1; i <= board_uport_count(); i++) {
            lport = 0;
            board_uport_to_lport(i, &unit, &lport);
            pport = SOC_PORT_L2P_MAPPING(lport);
            if (pport >= PHY_GPORT0_BASE && pport < PHY_XLPORT0_BASE) 
            {   
                /* ledup 0 */
               int_ledup_ctrl.pport_seq[0][int_ledup_ctrl.port_count[0]] = pport;
               int_ledup_ctrl.port_count[0]++;

            } else if (SOC_PORT_L2P_MAPPING(lport) >= PHY_XLPORT0_BASE && 
               SOC_PORT_L2P_MAPPING(lport) < (PHY_CLPORT0_BASE + 4)) 
            {
                /* ledup 1 */
                int_ledup_ctrl.pport_seq[1][int_ledup_ctrl.port_count[1]] =  pport;
                int_ledup_ctrl.port_count[1]++;
            }                
        }
       
    }
    
#ifdef CFG_VENDOR_CONFIG_SUPPORT_INCLUDED           
    if (sal_config_uint8_get(SAL_CONFIG_LED_OPTION, &led_option) == SYS_OK) {
        sal_printf("Vendor Config : Overwrite serial led option with value %d.\n", led_option);
    }
#endif    
    if (led_option == 1) {
        int_ledup_ctrl.led_mode[0] = LED_MODE_TXRX;  /* For LED 0 */      
        int_ledup_ctrl.led_mode[1] = LED_MODE_LINK;  /* For LED 1 */                    
        /* int_ledup_ctrl.led_mode[2] = LED_MODE_FORCE_ON; */  /* For LED 2 */                                  
    } else {
        int_ledup_ctrl.led_mode[0] = LED_MODE_LINK;      /* For LED 0 */      
        int_ledup_ctrl.led_mode[1] = LED_MODE_TXRX;       /* For LED 1 */                    
        /* int_ledup_ctrl.led_mode[2] = LED_MODE_FORCE_ON; */  /* For LED 2 */                                                
    }
         
    bcm5357x_load_ledup_program(0); 
}

uint8 
bcm5357x_ledup_generic_auto_remap(uint8 unit, uint8 ledup_idx, u8a_t *ledup_remap, 
                                    ledup_auto_remap_entry_t *remap_entry, int remap_entry_num) 
{
   int pport, pport_start, pport_end, pport_inc;
   int i, j, led_remap_cnt = 0, lport;
   pbmp_t remap_pbmp;
   uint8 customer_ledup_no, customer_ledup_port;  
   CMIC_LEDUP0_DATA_RAMr_t cmic_ledup0_data_ram;
   CMIC_LEDUP1_DATA_RAMr_t cmic_ledup1_data_ram;
   CMIC_LEDUP0_SCANCHAIN_ASSEMBLY_ST_ADDRr_t ledup0_st_addr;
   CMIC_LEDUP1_SCANCHAIN_ASSEMBLY_ST_ADDRr_t ledup1_st_addr;   
   uint8 port_max = int_ledup_ctrl.port_count[ledup_idx];
   uint32 bits_out;

   led_remap_cnt = 0;

   sal_memset(ledup_remap, port_max, sizeof(u8a_t));
   PBMP_CLEAR(remap_pbmp);
   for (j= (remap_entry_num - 1); j >= 0; j--) {
        for (i = (remap_entry[j].core_number - 1); i >= 0; i--) {        
             if (PBMP_MEMBER(remap_entry[j].core_disable,i)) {
                 led_remap_cnt++;
             } else {
                 if (remap_entry[j].pport_reverse) {
                     pport_start = remap_entry[j].pport_start + (i+1) * remap_entry[j].pport_count_per_core - 1;
                     pport_end   = remap_entry[j].pport_start + i * remap_entry[j].pport_count_per_core - 1;                     
                     pport_inc   = -1;
                 } else {
                     pport_start = remap_entry[j].pport_start + i * remap_entry[j].pport_count_per_core;
                     pport_end   = remap_entry[j].pport_start + (i+1) * remap_entry[j].pport_count_per_core;
                     pport_inc   = 1;
                 }
                 for (pport = pport_start; 
                      pport != pport_end; 
                      pport = pport + pport_inc) 
                 {

                      if ((bcm5357x_pport_to_ledup_port(unit, pport, &customer_ledup_no, &customer_ledup_port) == SYS_OK) &&
                          customer_ledup_no == ledup_idx) {
                          ledup_remap->i[led_remap_cnt++] = customer_ledup_port;
                      } else {
                          ledup_remap->i[led_remap_cnt++] = port_max;              
                      }                                     
                 }                         
              }
        }
   }

   if (int_ledup_ctrl.fix_bits_total[ledup_idx] == 0) {

       bits_out = (uint32) int_ledup_ctrl.leds_per_port *
                  (uint32) int_ledup_ctrl.port_count[ledup_idx] *
                  (uint32) int_ledup_ctrl.bits_per_led ;

       if (bits_out >= 256) {
           sal_printf("ERROR: LEDUP%d's scan out bit count >= 256.\n", ledup_idx);
           int_ledup_ctrl.fix_bits_total[ledup_idx] = 0;
       } else {
           int_ledup_ctrl.fix_bits_total[ledup_idx] = bits_out;
       }                                                                              
   }

   switch (ledup_idx) {
           case 0:
               READ_CMIC_LEDUP0_SCANCHAIN_ASSEMBLY_ST_ADDRr(unit, ledup0_st_addr);
               CMIC_LEDUP0_SCANCHAIN_ASSEMBLY_ST_ADDRr_SET(ledup0_st_addr, 0);
               WRITE_CMIC_LEDUP0_SCANCHAIN_ASSEMBLY_ST_ADDRr(unit, ledup0_st_addr);               
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, 0);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_START_PORT_0, cmic_ledup0_data_ram);
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, port_max);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_END_PORT_0, cmic_ledup0_data_ram);        
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, int_ledup_ctrl.fix_bits_total[ledup_idx]);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_BITS_TOTAL_COUNTS, cmic_ledup0_data_ram);    
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, int_ledup_ctrl.bits_per_led);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_BITS_PER_LED, cmic_ledup0_data_ram);               
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, int_ledup_ctrl.bits_color[3]);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_BITS_LED_COLOR_3, cmic_ledup0_data_ram);                              
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, int_ledup_ctrl.bits_color[2]);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_BITS_LED_COLOR_2, cmic_ledup0_data_ram);               
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, int_ledup_ctrl.bits_color[1]);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_BITS_LED_COLOR_1, cmic_ledup0_data_ram);               
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, int_ledup_ctrl.bits_color[0]);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_BITS_LED_COLOR_0, cmic_ledup0_data_ram);               
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, int_ledup_ctrl.bits_color_off);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_BITS_LED_COLOR_OFF, cmic_ledup0_data_ram);                 
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, int_ledup_ctrl.leds_per_port);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LEDS_PER_PORT, cmic_ledup0_data_ram);  
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, int_ledup_ctrl.led_blink_period);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_BLINK_PERIOD, cmic_ledup0_data_ram);                 
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, int_ledup_ctrl.led_tx_rx_extension);
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_TXRX_EXT_TIME_EVEN, cmic_ledup0_data_ram);                 
               CMIC_LEDUP0_DATA_RAMr_DATAf_SET(cmic_ledup0_data_ram, (((uint8) int_ledup_ctrl.led_tx_rx_extension) << 4));
               WRITE_CMIC_LEDUP0_DATA_RAMr(unit, LED_TXRX_EXT_TIME_ODD, cmic_ledup0_data_ram);                                               
           break;
           default:
           case 1:
               READ_CMIC_LEDUP1_SCANCHAIN_ASSEMBLY_ST_ADDRr(unit, ledup1_st_addr);
               CMIC_LEDUP1_SCANCHAIN_ASSEMBLY_ST_ADDRr_SET(ledup1_st_addr, 0);
               WRITE_CMIC_LEDUP1_SCANCHAIN_ASSEMBLY_ST_ADDRr(unit, ledup1_st_addr);            
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, 0);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_START_PORT_0, cmic_ledup1_data_ram);
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, port_max);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_END_PORT_0, cmic_ledup1_data_ram);        
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, int_ledup_ctrl.fix_bits_total[ledup_idx]);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_BITS_TOTAL_COUNTS, cmic_ledup1_data_ram);    
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, int_ledup_ctrl.bits_per_led);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_BITS_PER_LED, cmic_ledup1_data_ram);                
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, int_ledup_ctrl.bits_color[3]);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_BITS_LED_COLOR_3, cmic_ledup1_data_ram);                              
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, int_ledup_ctrl.bits_color[2]);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_BITS_LED_COLOR_2, cmic_ledup1_data_ram);               
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, int_ledup_ctrl.bits_color[1]);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_BITS_LED_COLOR_1, cmic_ledup1_data_ram);               
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, int_ledup_ctrl.bits_color[0]);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_BITS_LED_COLOR_0, cmic_ledup1_data_ram);               
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, int_ledup_ctrl.bits_color_off);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_BITS_LED_COLOR_OFF, cmic_ledup1_data_ram);                 
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, int_ledup_ctrl.leds_per_port);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LEDS_PER_PORT, cmic_ledup1_data_ram);  
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, int_ledup_ctrl.led_blink_period);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_BLINK_PERIOD, cmic_ledup1_data_ram);                 
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, int_ledup_ctrl.led_tx_rx_extension);
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_TXRX_EXT_TIME_EVEN, cmic_ledup1_data_ram);                 
               CMIC_LEDUP1_DATA_RAMr_DATAf_SET(cmic_ledup1_data_ram, (((uint8) int_ledup_ctrl.led_tx_rx_extension) << 4));
               WRITE_CMIC_LEDUP1_DATA_RAMr(unit, LED_TXRX_EXT_TIME_ODD, cmic_ledup1_data_ram);                                                                          
           break;          
   }

   SOC_LPORT_ITER(lport) {
        for (i=0; i<int_ledup_ctrl.leds_per_port; i++) {
             bcm5357x_ledup_mode_set(unit, lport, i, int_ledup_ctrl.led_mode[i]);
        }
   }   
   return int_ledup_ctrl.port_count[ledup_idx] - 1;
}

void
bcm5357x_load_ledup_program(uint8 unit) {


   PGW_CTRL_0r_t pgw_ctrl_0;
   int i = 0;
   u8a_t port_remap_0;
   ledup_auto_remap_entry_t remap_entry[2];
   uint8 led_option;

   CMIC_LEDUP0_PORT_ORDER_REMAPr_t cmic_ledup0_port_order_remap;
   CMIC_LEDUP1_PORT_ORDER_REMAPr_t cmic_ledup1_port_order_remap;
   CMIC_LEDUP0_CTRLr_t cmic_ledup0_ctrl;   
   CMIC_LEDUP1_CTRLr_t cmic_ledup1_ctrl;   

  
   bcm5357x_load_firmware(unit);
   
   READ_PGW_CTRL_0r(0, pgw_ctrl_0);
   /* 
    *  LED0 = QTC1 (42-57) --> QTC0(26-41) --> SGMIIX4P2(18-25) --> SGMIIX4P1(26- --> SGMIIX4P0
    */
   remap_entry[1].core_number = 2;
   PBMP_CLEAR(remap_entry[1].core_disable);
   /* PBMP_WORD_SET(remap_entry[1].core_disable, 0, PGW_CTRL_0r_SW_QTC_DISABLEf_GET(pgw_ctrl_0)); */
   remap_entry[1].pport_start = PHY_GPORT3_BASE;
   remap_entry[1].pport_count_per_core = 16;
   remap_entry[1].pport_reverse = 0;

   remap_entry[0].core_number = 3;
   PBMP_CLEAR(remap_entry[0].core_disable);
   remap_entry[0].pport_start = PHY_GPORT0_BASE;
   remap_entry[0].pport_count_per_core = 8;
   remap_entry[0].pport_reverse = 0;

   LEDUP_AUTO_REMAP(unit, 0, &port_remap_0, remap_entry, 2);

   for (i = 0; i < sizeof(port_remap_0.i); i+=4) {
        CMIC_LEDUP0_PORT_ORDER_REMAPr_CLR(cmic_ledup0_port_order_remap);    
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_0f_SET(cmic_ledup0_port_order_remap, port_remap_0.i[i]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(cmic_ledup0_port_order_remap, port_remap_0.i[i+1]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_2f_SET(cmic_ledup0_port_order_remap, port_remap_0.i[i+2]);
        CMIC_LEDUP0_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(cmic_ledup0_port_order_remap, port_remap_0.i[i+3]);
        WRITE_CMIC_LEDUP0_PORT_ORDER_REMAPr(unit, i/4, cmic_ledup0_port_order_remap);
   }
   
   /*   LED1 = TSCF0(32-29) --> TSCE6(28-25)  --> ... --> TSCE0 */
   remap_entry[1].core_number = 1;
   PBMP_CLEAR(remap_entry[1].core_disable);
   PBMP_WORD_SET(remap_entry[1].core_disable, 0, PGW_CTRL_0r_SW_PM4X25_DISABLEf_GET(pgw_ctrl_0));
   remap_entry[1].pport_start = PHY_CLPORT0_BASE;
   remap_entry[1].pport_count_per_core = 4;
   remap_entry[1].pport_reverse = 1;

   remap_entry[0].core_number = 7;
   PBMP_CLEAR(remap_entry[0].core_disable);
   PBMP_WORD_SET(remap_entry[0].core_disable, 0, PGW_CTRL_0r_SW_PM4X10_DISABLEf_GET(pgw_ctrl_0));   
   remap_entry[0].pport_start = PHY_XLPORT0_BASE;
   remap_entry[0].pport_count_per_core = 4;
   remap_entry[0].pport_reverse = 1;

   LEDUP_AUTO_REMAP(unit, 1, &port_remap_0, remap_entry, 2);

   for (i = 0; i < sizeof(port_remap_0.i); i+=4) {
        CMIC_LEDUP1_PORT_ORDER_REMAPr_CLR(cmic_ledup1_port_order_remap);    
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_0f_SET(cmic_ledup1_port_order_remap, port_remap_0.i[i]);
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_1f_SET(cmic_ledup1_port_order_remap, port_remap_0.i[i+1]);
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_2f_SET(cmic_ledup1_port_order_remap, port_remap_0.i[i+2]);
        CMIC_LEDUP1_PORT_ORDER_REMAPr_REMAP_PORT_3f_SET(cmic_ledup1_port_order_remap, port_remap_0.i[i+3]);
        WRITE_CMIC_LEDUP1_PORT_ORDER_REMAPr(unit, i/4, cmic_ledup1_port_order_remap);
   }

   sal_config_uint8_get(SAL_CONFIG_LED_OPTION, &led_option);

   /* load customer firmware and disable all function of generic serial LED function */
   if (led_option == 3) {    
       bcm5357x_load_customer_firmware(unit);      
   }

   /* enable LED processor */
   READ_CMIC_LEDUP0_CTRLr(unit, cmic_ledup0_ctrl);
   CMIC_LEDUP0_CTRLr_LEDUP_ENf_SET(cmic_ledup0_ctrl, 1);
   WRITE_CMIC_LEDUP0_CTRLr(unit, cmic_ledup0_ctrl);
        
   READ_CMIC_LEDUP1_CTRLr(unit, cmic_ledup1_ctrl);
   CMIC_LEDUP1_CTRLr_LEDUP_ENf_SET(cmic_ledup1_ctrl, 1);
   WRITE_CMIC_LEDUP1_CTRLr(unit, cmic_ledup1_ctrl);
      
}


/*

     Per-LED setting 0 : LINK
                           1 : TX/RX
                           2 : BLINK
                           3 : FORCE ON
*/
void 
bcm5357x_ledup_mode_set(uint8 unit, int lport, int led_no, int mode) {

   uint8  ledup_no, ledup_port;
   CMIC_LEDUP0_DATA_RAMr_t cmic_ledup0_data_ram;
   CMIC_LEDUP1_DATA_RAMr_t cmic_ledup1_data_ram;   
   LEDUP_PORT_DATA_t  ledup_port_data;

   if (disable_generic_led) return;
   
   if (bcm5357x_pport_to_ledup_port(unit, SOC_PORT_L2P_MAPPING(lport), &ledup_no, &ledup_port) != SYS_OK) 
   {
       return; 
   }

   if (mode > LED_MODE_FORCE_ON) return;
   
   if (led_no >= int_ledup_ctrl.leds_per_port) return;
    
   if (ledup_no == 0) {
       READ_CMIC_LEDUP0_DATA_RAMr(unit, PORT_DATA + ledup_port, cmic_ledup0_data_ram);
       LEDUP_PORT_DATA_SET(ledup_port_data, CMIC_LEDUP0_DATA_RAMr_GET(cmic_ledup0_data_ram));
       if (led_no == 0) {
           LEDUP_PORT_DATA_LED0_MODEf_SET(ledup_port_data, mode);
       } else if (led_no == 1) {
           LEDUP_PORT_DATA_LED1_MODEf_SET(ledup_port_data, mode);
       } else if (led_no == 2) {
           LEDUP_PORT_DATA_LED2_MODEf_SET(ledup_port_data, mode);
       }
       CMIC_LEDUP0_DATA_RAMr_SET(cmic_ledup0_data_ram, LEDUP_PORT_DATA_GET(ledup_port_data));
       WRITE_CMIC_LEDUP0_DATA_RAMr(unit, PORT_DATA + ledup_port , cmic_ledup0_data_ram);           
   } else if (ledup_no == 1) {      
       READ_CMIC_LEDUP1_DATA_RAMr(unit, PORT_DATA + ledup_port, cmic_ledup1_data_ram);
       LEDUP_PORT_DATA_SET(ledup_port_data, CMIC_LEDUP1_DATA_RAMr_GET(cmic_ledup1_data_ram));
       if (led_no == 0) {
           LEDUP_PORT_DATA_LED0_MODEf_SET(ledup_port_data, mode);
       } else if (led_no == 1) {
           LEDUP_PORT_DATA_LED1_MODEf_SET(ledup_port_data, mode);
       } else if (led_no == 2) {
           LEDUP_PORT_DATA_LED2_MODEf_SET(ledup_port_data, mode);
       }
       CMIC_LEDUP1_DATA_RAMr_SET(cmic_ledup1_data_ram, LEDUP_PORT_DATA_GET(ledup_port_data));
       WRITE_CMIC_LEDUP1_DATA_RAMr(unit, PORT_DATA + ledup_port , cmic_ledup1_data_ram);           
   }

}

void 
bcm5357x_ledup_color_set(uint8 unit, int lport, int color) {
    uint8  ledup_no, ledup_port;
    CMIC_LEDUP0_DATA_RAMr_t cmic_ledup0_data_ram;
    CMIC_LEDUP1_DATA_RAMr_t cmic_ledup1_data_ram;   
    LEDUP_PORT_DATA_t  ledup_port_data;

    if (disable_generic_led) return;
    
    if (bcm5357x_pport_to_ledup_port(unit, SOC_PORT_L2P_MAPPING(lport), &ledup_no, &ledup_port) != SYS_OK) 
    {
        return; 
    }

    if (color >= int_ledup_ctrl.num_of_color) return;

    if (ledup_no == 0) {
        READ_CMIC_LEDUP0_DATA_RAMr(unit, PORT_DATA + ledup_port, cmic_ledup0_data_ram);
        LEDUP_PORT_DATA_SET(ledup_port_data, CMIC_LEDUP0_DATA_RAMr_GET(cmic_ledup0_data_ram));
        LEDUP_PORT_DATA_COLOR_SELf_SET(ledup_port_data, color);
        CMIC_LEDUP0_DATA_RAMr_SET(cmic_ledup0_data_ram, LEDUP_PORT_DATA_GET(ledup_port_data));
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, PORT_DATA + ledup_port , cmic_ledup0_data_ram);           
    } else if (ledup_no == 1) {      
        READ_CMIC_LEDUP1_DATA_RAMr(unit, PORT_DATA + ledup_port, cmic_ledup1_data_ram);
        LEDUP_PORT_DATA_SET(ledup_port_data, CMIC_LEDUP1_DATA_RAMr_GET(cmic_ledup1_data_ram));
        LEDUP_PORT_DATA_COLOR_SELf_SET(ledup_port_data, color);
        CMIC_LEDUP1_DATA_RAMr_SET(cmic_ledup1_data_ram, LEDUP_PORT_DATA_GET(ledup_port_data));
        WRITE_CMIC_LEDUP1_DATA_RAMr(unit, PORT_DATA + ledup_port , cmic_ledup1_data_ram);           
   } 

}

void 
bcm5357x_ledup_sw_linkup(uint8 unit, int lport, int linkup) {

    uint8  ledup_no, ledup_port;
    CMIC_LEDUP0_DATA_RAMr_t cmic_ledup0_data_ram;
    CMIC_LEDUP1_DATA_RAMr_t cmic_ledup1_data_ram;   
    LEDUP_PORT_DATA_t  ledup_port_data;

    if (disable_generic_led) return;
    
    if (bcm5357x_pport_to_ledup_port(unit, SOC_PORT_L2P_MAPPING(lport), &ledup_no, &ledup_port) != SYS_OK) 
    {
       return; 
    }

    if (int_ledup_ctrl.leds_per_port == 3) return;

    linkup = (linkup == PORT_LINK_UP);
    
    if (ledup_no == 0) {
        READ_CMIC_LEDUP0_DATA_RAMr(unit, PORT_DATA + ledup_port, cmic_ledup0_data_ram);
        LEDUP_PORT_DATA_SET(ledup_port_data, CMIC_LEDUP0_DATA_RAMr_GET(cmic_ledup0_data_ram));
        LEDUP_PORT_DATA_SW_LINK_UPf_SET(ledup_port_data, linkup);
        CMIC_LEDUP0_DATA_RAMr_SET(cmic_ledup0_data_ram, LEDUP_PORT_DATA_GET(ledup_port_data));
        WRITE_CMIC_LEDUP0_DATA_RAMr(unit, PORT_DATA + ledup_port , cmic_ledup0_data_ram);           
    } else if (ledup_no == 1) {      
        READ_CMIC_LEDUP1_DATA_RAMr(unit, PORT_DATA + ledup_port, cmic_ledup1_data_ram);
        LEDUP_PORT_DATA_SET(ledup_port_data, CMIC_LEDUP1_DATA_RAMr_GET(cmic_ledup1_data_ram));
        LEDUP_PORT_DATA_SW_LINK_UPf_SET(ledup_port_data, linkup);
        CMIC_LEDUP1_DATA_RAMr_SET(cmic_ledup1_data_ram, LEDUP_PORT_DATA_GET(ledup_port_data));
        WRITE_CMIC_LEDUP1_DATA_RAMr(unit, PORT_DATA + ledup_port , cmic_ledup1_data_ram);           
    }
 
}
#endif /* CFG_LED_MICROCODE_INCLUDED */

