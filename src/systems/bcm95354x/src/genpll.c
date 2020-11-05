/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2020 Broadcom Inc. All rights reserved. 
 */
#include <system.h>
#include "socregs.h"


unsigned int genpll_clk_tab[] = {
	/*  mode,	ndiv,	MDIV
						0	1	2	3	4	5 */
		0,		96,		12,	0,	80,	6,	5,	40,		/* 400 MHz AXI */
		1,		96,		12,	0,	80,	12,	5,	40,		/* 200 MHz AXI */
		2,		96,		12,	0,	80,	24,	5,	40,		/* 100 MHz AXI */		
		0xffffffff
};

#define reg32_write(x,v) (*x) = (v)
#define reg32_read(x)  (*x)

uint32 iproc_config_genpll(uint32 mode)
{
   volatile uint32 rdata;
   volatile uint32 lock;
   int i = 0;

	while(1) {
		if(genpll_clk_tab[i] == mode)
			break;
		if(genpll_clk_tab[i] == 0xffffffff) {
			return(1);
		}
		i += 8;
	}

   // Clear Load_en Channel3 & Channel4
   rdata = READCSR(IPROC_WRAP_GEN_PLL_CTRL3);
   rdata &= 0xffc0ffff;
   WRITECSR(IPROC_WRAP_GEN_PLL_CTRL3,rdata);

	// Write fast_lock =1
   rdata = READCSR(IPROC_WRAP_GEN_PLL_CTRL0);
   rdata |= (1<<IPROC_WRAP_GEN_PLL_CTRL0__FAST_LOCK);
   WRITECSR(IPROC_WRAP_GEN_PLL_CTRL0,rdata);

   // Write NDIV
   rdata = READCSR(IPROC_WRAP_GEN_PLL_CTRL1);
   rdata &= 0xfffffc00;
   rdata |= (genpll_clk_tab[i+1] << IPROC_WRAP_GEN_PLL_CTRL1__NDIV_INT_R);
   WRITECSR(IPROC_WRAP_GEN_PLL_CTRL1,rdata);

   // Write MDIV
   rdata = READCSR(IPROC_WRAP_GEN_PLL_CTRL2);
   rdata &= 0xff0000ff;
   rdata |= ((genpll_clk_tab[i+5] <<IPROC_WRAP_GEN_PLL_CTRL2__CH3_MDIV_R)|
            (genpll_clk_tab[i+6]<<IPROC_WRAP_GEN_PLL_CTRL2__CH4_MDIV_R));
   WRITECSR(IPROC_WRAP_GEN_PLL_CTRL2,rdata);

   // Write PLL_LOAD
   rdata = READCSR(IPROC_WRAP_GEN_PLL_CTRL3);
   rdata |= (1<<IPROC_WRAP_GEN_PLL_CTRL3__SW_TO_GEN_PLL_LOAD);
   WRITECSR(IPROC_WRAP_GEN_PLL_CTRL3,rdata);

   // Load Channel3 & Channel4
   rdata &= 0xffc0ffff;
   rdata |= (0x18<<IPROC_WRAP_GEN_PLL_CTRL3__LOAD_EN_CH_R);
   WRITECSR(IPROC_WRAP_GEN_PLL_CTRL3,rdata);

   // Wait for IPROC_WWRAP GENPLL lock
   do{
     rdata = READCSR(IPROC_WRAP_GEN_PLL_STATUS);
     lock = ((rdata>>IPROC_WRAP_GEN_PLL_STATUS__GEN_PLL_LOCK)&1);
   }while(!lock);

	return(0);
}
#if 0
uint32_t iproc_get_axi_clk(uint32_t refclk)
{
#if defined(CONFIG_HURRICANE2_EMULATION) || defined(CONFIG_IPROC_EMULATION)
	return(IPROC_AXI_CLK); /* return the emulator clock defined in configuration file */
#else
   uint32_t ndiv, mdiv, pdiv;

   ndiv = (reg32_read((volatile uint32_t *)IPROC_WRAP_GEN_PLL_CTRL1) >> IPROC_WRAP_GEN_PLL_CTRL1__NDIV_INT_R) &
			((1 << IPROC_WRAP_GEN_PLL_CTRL1__NDIV_INT_WIDTH) -1);
   if(ndiv == 0)
	   ndiv = 1 << IPROC_WRAP_GEN_PLL_CTRL1__NDIV_INT_WIDTH;

   pdiv = (reg32_read((volatile uint32_t *)IPROC_WRAP_GEN_PLL_CTRL1) >> IPROC_WRAP_GEN_PLL_CTRL1__PDIV_R) &
			((1 << IPROC_WRAP_GEN_PLL_CTRL1__PDIV_WIDTH) -1);
   if(pdiv == 0)
	   pdiv = 1 << IPROC_WRAP_GEN_PLL_CTRL1__PDIV_WIDTH;

   mdiv = (reg32_read((volatile uint32_t *)IPROC_WRAP_GEN_PLL_CTRL2) >> IPROC_WRAP_GEN_PLL_CTRL2__CH3_MDIV_R) &
			((1 << IPROC_WRAP_GEN_PLL_CTRL2__CH3_MDIV_WIDTH) -1);
   if(mdiv == 0)
	   mdiv = 1 << IPROC_WRAP_GEN_PLL_CTRL2__CH3_MDIV_WIDTH;

	return refclk * ndiv / pdiv / mdiv;
#endif
}



uint32_t iproc_get_uart_clk(uint32_t uart)
{
	uint32_t uartclk, uartclkovr, uartclksel; 
	
	uartclk = iproc_get_axi_clk(CONFIG_SYS_REF_CLK) / 4; /* APB clock */
    
	return(uartclk);
}
#endif

