;
;
; This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
; 
; Copyright 2007-2020 Broadcom Inc. All rights reserved.
;
;
; This is the default program for Wolfhound2 embedded gphy:
;   Left LED : Link,   Right LED : TX/RX activity 
;
; Assume 1 LED per port. And 1 bits stream per LED.
;       Link-Down = Off (b'1 b'1)
;       Link-Up = On (b'0 b'1)
;       traffic = (b'0 b'0) (b'0 b'1)  switched
;       loop    = (b'0 b'0) (b'1 b'1)  switched
;

;
; Constants
;
;NUM_PORTS     equ 32
;
; Note: The END_PORT_0 ... END_PORT_1 must be the same as the value of LED_START_PORT_0 ... LED_END_PORT_1 in wh2led.c
;
START_PORT_0  equ 0xF2
END_PORT_0    equ 0xF3
START_PORT_1  equ 0xF4
END_PORT_1    equ 0xF5
LED_OUT_BITS  equ 0xF6
;#define LED_START_PORT_0        0xF2
;#define LED_END_PORT_0          0xF3
;#define LED_START_PORT_1        0xF4
;#define LED_END_PORT_1          0xF5
;#define LED_OUT_BITS            0xF6

; when START_PORT_2 == END_PORT_2,  stop the iterations
START_PORT_2  equ 90
END_PORT_2    equ 90

TICKS           EQU     1
SECOND_TICKS    EQU     (30*TICKS)
TXRX_ALT_TICKS  EQU     (SECOND_TICKS/6)
; The TX/RX activity lights will be extended for ACT_EXT_TICKS
; so they will be more visible.
ACT_EXT_TICKS   EQU     (SECOND_TICKS/3)

;
; LED process
;
start:
    ld  b, (START_PORT_0)
    ld  (SEC_MAX), b    
       
iter_sec0:
    ld  b, (SEC_MAX)
    ld  a, (END_PORT_2)
    cmp b, a
    jz  end

    ld  a, (END_PORT_0)
    cmp b, a
    jz  iter_sec1
    
    ld  a, (END_PORT_1)
    cmp b, a
    jz  iter_sec2
    
    ld  a, (START_PORT_0)
    ld  b, (END_PORT_0)
    ld  (SEC_MAX), b    
    jmp iter_common
    
iter_sec1:
    ld  a, (START_PORT_1)
    ld  b, (END_PORT_1)
    ld  (SEC_MAX), b    
    jmp iter_common
    
iter_sec2:
    ; when START_PORT_2 == END_PORT_2,  stop the iterations
    ld  a, START_PORT_2
    ld  b, END_PORT_2
    cmp a, b
    jz  end
    
    ld  a, START_PORT_2
    ld  b, END_PORT_2
    ld  (SEC_MAX), b    
    jmp iter_common

iter_common:
    port    a
    ld  (PORT_NUM), a
    
        call    get_loop
        jnc     up4             ; no loop, check tx/rx
        call    loop_detect
        jmp     up3

up4:        
        call    activity       

up3:        
    ld  a, (PORT_NUM)
    ld  b, (SEC_MAX)
    cmp a, b
    ; a = b
    jz  iter_sec0 
    ; a > b then up_dec
    jnc up_dec    
    

up_add:
    inc a
    jmp iter_common    

up_dec:
    dec a
    jmp iter_common 
        
    
end:
    ; Update various timers
    ld      b,TXRX_ALT_COUNT
    inc     (b)
    ld      a,(b)
    cmp     a,TXRX_ALT_TICKS
    jc      end1
    ld      (b),0
    
end1:
	ld  b, (LED_OUT_BITS)
    send    b

;
; activity
;
;  This routine calculates the activity LED for the current port.
;  It extends the activity lights using timers (new activity overrides
;  and resets the timers).
;
;  Inputs: (PORT_NUM)
;  Outputs: Four bits sent to LED stream
;

activity:
        pushst  RX
        pushst  TX
        tor
        pop
        jnc     act1
        
        jmp     act2
        
act1:
        jmp    get_link

act2:                           
        ld      b,(TXRX_ALT_COUNT)
        cmp     b, TXRX_ALT_TICKS
        
        jz      led_link_noact        ; Fast alternation of green on right
        jmp     led_green        ; keep green on left
        
;
; get_link
;
get_link:
    pushst  LINKEN
    pop
    jnc     led_black             ; no link, all black
        
    jmp     led_link_noact         ; link up, left keep green


; loop_detect
;
loop_detect:
        ld      b,PORTDATA
        add     b,(PORT_NUM) 
        ld      b,(b)
        tst     b,1
        jnc     loop1
        
loop1:
        jmp      loop2

loop2:                           ; Both TX and RX active
        ld      b,(TXRX_ALT_COUNT)
        cmp     b,TXRX_ALT_TICKS/2
        jnc     led_green       ; Fast alternation of green on left and right
        jmp      led_black
        

get_loop:        
        ld      b,PORTDATA
        add     b,(PORT_NUM)
        ld      b,(b)
        tst     b,1
        ret
     
;
; led_green
;
;  Outputs: Bits to the LED stream indicating ON
;
led_green:
    push 0
    pack
    push 0 
    pack
    ret

;
; led_black
;
;  Outputs: Bits to the LED stream indicating OFF
;
led_black:
    push 1
    pack
    push 1
    pack
    ret

led_link_noact:
    ;When b=1 at beginning, output 10(left bit first)
    ;When b=0 at beginning, output 01(left bit first)
    ld      b,(LED_LEFT_LINK_OFFSET)
    push    b
    pack
    
    xor     b, 1
    push    b
    pack
    
    ret

; Variables (UM software initializes LED memory from 0xA0-0xff to 0)
PORTDATA    equ 0xA0
PORT_NUM    equ 0xE0    
SEC_MAX     equ 0xE1

;stored flag that is read from "led_option" in config.um in wh2led.c
; when led_option=1, (LED_LEFT_LINK_OFFSET) =1
; when led_option=2, (LED_LEFT_LINK_OFFSET) =0
LED_LEFT_LINK_OFFSET   equ 0xF0    

;
; LED extension timers
;
TXRX_ALT_COUNT  equ     0xE3

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
