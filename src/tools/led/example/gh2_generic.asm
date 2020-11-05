;
;
; This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
; 
; Copyright 2007-2020 Broadcom Inc. All rights reserved.
;
;
; This is the default program for Foxhound2 embedded gphy:
;   Left LED : Link,   Right LED : TX/RX activity 
;
; Assume 2 LED per port. And 2 bits stream per LED.
;       Link-Down = Off (b'00 b'00)
;       Link-Up = On (b'01 b'00)
;       traffic = (b'01 b'01) (b'01 b'00)  switched
;       loop    = (b'01 b'01) (b'00 b'00)  switched
;
; No mattter how much port used, on-chip serial-to-parallel need 96 bits output

;
; Constants
;
;NUM_PORTS     equ 32
;
; Note: 

; Host parameters
EXT_CNTS                   equ 0x80
PORT_DATA                  equ 0xA0
PORT_NUM                   equ 0xE0    
LED_BITS_PER_LED           equ 0xE1
LED_BITS_LED_COLOR_0       equ 0xE2
LED_BITS_LED_COLOR_1       equ 0xE3
LED_BITS_LED_COLOR_2       equ 0xE4
LED_BITS_LED_COLOR_3       equ 0xE5
LED_BITS_LED_COLOR_OFF     equ 0xE6
LED_BITS_TOTAL_COUNTS      equ 0xE7
LEDS_PER_PORT              equ 0xE9
LED_BLINK_PERIOD           equ 0xEA
LED_TXRX_EXT_TIME_EVEN     equ 0xEB
LED_TXRX_EXT_TIME_ODD      equ 0xEC
LED_START_PORT_0               equ 0xF2
LED_END_PORT_0                 equ 0xF3

; Internal variables
LED_BLINK_CNT              equ 0xED
PORT_DATA_TMP              equ 0xEE
LED_NUM                    equ 0xEF 
LED_STATUS                 equ 0xF0
LED_BITS_OUT_CNT           equ 0xF1


;
; LED extension timers
;

; PORT_DATA layout
PORT_DATA_SW_LINKUP        equ 0x0    ; bit 0   valid if (LEDS_PER_PORT < 3) 
PORT_DATA_MODE_TOP         equ 0x5    ; bit 5~0
PORT_DATA_COLOR            equ 0x6    ; bit 7~6

; PORT_STATUS layout
PORT_STATUS_LINK_UP        equ 0x0
PORT_STATUS_TXRX           equ 0x1

; Symbolic names for the bits of the port status fields
RX      equ 0x0 ; received packet
TX      equ 0x1 ; transmitted packet
COLL    equ 0x2 ; collision indicator
SPEED_C equ 0x3 ; 100 Mbps
SPEED_M equ 0x4 ; 1000 Mbps
DUPLEX  equ 0x5 ; half/full duplex
FLOW    equ 0x6 ; flow control capable
LINKUP  equ 0x7 ; link down/up status
LINKEN  equ 0x8 ; link disabled/enabled status
ZERO    equ 0xE ; always 0
ONE     equ 0xF ; always 1

;
; LED process
;
start:  
    ld   a, 0              
    ld  (LED_BITS_OUT_CNT), a    
    ld   a, (LED_START_PORT_0)
            
main_loop:
    ; Change to port a and 
    port    a
    
    ; Store current port number 
    ld  (PORT_NUM), a
    
    ; Get PORT_DATA based on port number    
    add  a, PORT_DATA
    ld  (PORT_DATA_TMP), (a)
    
    ; if (*LEDS_PER_PORT < 3) then check SW link     
    ; Gather port status into (LED_STATUS)
    ld  b, 0        
    ld  a, (LEDS_PER_PORT)
    cmp a, 2
    jnc main_no_sw_link_check
    
    ; linkup = (sw_linkup | hw_linkup)    
    ld  a, (PORT_DATA_TMP)
    tst a, PORT_DATA_SW_LINKUP
    push CY
    pushst LINKEN
    tor
    jmp main_gather_port_info
    
main_no_sw_link_check:
    ; linkup = (hw_linkup)    
    pushst  LINKEN
    
main_gather_port_info:        
    pop
    bit  b, PORT_STATUS_LINK_UP
    pushst  RX
    pushst  TX
    tor
    pop
    bit  b, PORT_STATUS_TXRX
    ld  (LED_STATUS), b
        
    ; Start to send scan bits by led_mode 
    ; Let LED_NUM = 0     
    ld  a, 0
    ld  (LED_NUM), a
    
main_loop_0:
    ld   b, (LED_NUM)
    cmp  b, (LEDS_PER_PORT)
    jz  main_loop_3
    
main_loop_1:       
    ;
    ;  switch(mode[LED_NUM]) {
    ;       case LED_MODE_LINK:     send_link();
    ;       case LED_MODE_TXRX:     send_tx_rx();
    ;       case LED_MODE_BLINK:    send_blink();
    ;       case LED_MODE_FORCE_ON: led_on();
    ;  } 
    ;
    clc  
    rol  b    
    ; a = mode[LED_NUM] 
    ld   a, PORT_DATA_MODE_TOP
    sub  a, b
    tst  (PORT_DATA_TMP), a
    jc   main_mode_1x
main_mode_0x:     
    dec  a
    tst  (PORT_DATA_TMP), a
    jc   main_mode_01
main_mode_00:
    jmp    send_link    
main_mode_01:
    jmp    send_tx_rx        
main_mode_1x:    
    dec  a
    tst  (PORT_DATA_TMP), a
    jc  main_mode_11
main_mode_10:
    jmp    send_blink
main_mode_11:        
    jmp    led_on

main_loop_2:     
     ld   a, (LED_NUM)
     cmp  a, (LEDS_PER_PORT)
     jz  main_loop_3     
     inc (LED_NUM)    
     jmp main_loop_0
     
main_loop_3: 
    ld  a, (PORT_NUM)
    inc a    
    cmp a, (LED_END_PORT_0)    
    jc  main_loop
    
    ; Before sending the scan bits, clear remain bits
    ld  a, (LED_BITS_OUT_CNT)
    cmp a, (LED_BITS_TOTAL_COUNTS)
    jc  led_off       
             
main_handle_blink_counter:
    ; Update counter for blink effect    
    inc     (LED_BLINK_CNT)
    ld      a, (LED_BLINK_PERIOD)
    cmp     a, (LED_BLINK_CNT)
    jnc     main_end
    ld      a, 0
    ld      (LED_BLINK_CNT),a
    
main_end:          
    send   (LED_BITS_TOTAL_COUNTS)

;
; send_tx_rx
;
;  This routine calculates the activity LED for the current port.
;  It extends the activity lights using 4bits counter. 
;  Any new activity will lead to the timer value reload.
;  If port link down, the time value will be cleared to zero.
;

send_tx_rx:
        ld      b, (PORT_NUM)
        add     b, EXT_CNTS

        ld      a, (LED_STATUS)
        tst     a, PORT_STATUS_LINK_UP
        jnc     tx_rx_clear
        tst     a, PORT_STATUS_TXRX               
        jnc     tx_rx_led_off   
                       
        ; Extended counter reload and let led on                          
tx_rx_led_reload:  
        ; even/odd test                                                                                                       
        tst     b, 0
        jnc     tx_rx_reload_odd
tx_rx_reload_even:        
        ld      a, (b)
        and     a, 0xF0
        or      a, (LED_TXRX_EXT_TIME_EVEN)        
        ld      (b), a
        jmp      led_on
tx_rx_reload_odd:        
        ld      a, (b)
        and     a, 0x0F
        or      a, (LED_TXRX_EXT_TIME_ODD)                
        ld      (b), a
        jmp      led_on
        
        ; Extended counter clear
tx_rx_clear:
        ; even/odd test         
        tst     b, 0
        jnc     tx_rx_clear_odd
tx_rx_clear_even:        
        ld      a, (b)
        and     a, 0xF0
        ld      (b), a
        jmp      led_off
tx_rx_clear_odd:        
        ld      a, (b)
        and     a, 0x0F
        ld      (b), a
        jmp      led_off
                
       ; Extended counter decrease         
tx_rx_led_off:                                                      
        tst     b, 0
        jnc     tx_rx_dec_odd                
tx_rx_dec_even:        
        ld      a, (b)
        and     a, 0x0F
        jz      led_off
        ld      a, (b)
        sub     a, 0x01
        ld      (b), a        
        jmp     led_on
tx_rx_dec_odd:
        ld      a, (b)
        and     a, 0xF0
        jz      led_off
        ld      a, (b)
        sub     a, 0x10
        ld      (b), a
        jmp     led_on         
                        
;
; send_link: 
; 
;   This routine set led on/off to indicate port link up/down
;
send_link:
    ld      b, (LED_STATUS)
    tst     b, 0
    jnc     led_off       ; link down, led black        
    jmp     led_on        ; link up, led on

;
; send_blink: let LED blink in a rate controlled by BLINK_PERIOD
;
send_blink:
    clc
    ld      b, (LED_BLINK_PERIOD)
    ror     b
    cmp     b, (LED_BLINK_CNT)
    jnc     led_on       ; Fast alternation of green on left and right
    jmp     led_off
        
     
;
;  led_off: send bits to turn LED off
;  led_on:  send bits out to have LED display in different color 
;  

led_off:
    ld  a, (LED_BITS_LED_COLOR_OFF)
    jmp   led_common
led_on:
    ld  a, 0
    ld  b, PORT_DATA_COLOR        
    tst (PORT_DATA_TMP), b
    bit a, 0
    inc b
    tst (PORT_DATA_TMP), b
    bit a, 1
    add a, LED_BITS_LED_COLOR_0
    ld a, (a)   
led_common:    
    ld b, 0
led_common_loop:    
    cmp b, (LED_BITS_PER_LED)
    jz  led_common_ret
    tst a, b
    push CY
    pack
    inc b
    jmp led_common_loop
led_common_ret:
    ld  b, (LED_BITS_PER_LED)
    add b, (LED_BITS_OUT_CNT)
    ld (LED_BITS_OUT_CNT), b    
    jmp main_loop_2


