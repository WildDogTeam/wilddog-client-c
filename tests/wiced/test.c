/*
 * test.c
 *
 *  Created on: 2015-06-13 lixiongsheng
 */

#include <string.h>
#include "wiced.h"
#include "wilddog.h"
#include "wiced_tcpip.h"
#include "wilddog_api.h"
#include "wifi_config_dct.h"

#include "test_lib.h"
#if 0
//#pragma optimize("O0")
#define PRINTSTACK     do{volatile u32 p; \
__asm__ volatile("mov %0, sp\n":"=r"(p):);\
printf("func:%s,sp = %p\n", __func__,p);fflush(stdout);}while(0)
#include <stdio.h>
#include <stdlib.h>
#include "wilddog_api.h"

int statnd_cyclecount = 0;
volatile unsigned int *DWT_CYCCNT = (int *)0xE0001004; //address of the register
volatile unsigned int *DWT_CONTROL = (int *)0xE0001000; //address of the register
volatile unsigned int *SCB_DEMCR = (int *)0xE000EDFC; //address of the registert

int cpucycle_rst(void)
{
	*SCB_DEMCR = *SCB_DEMCR | 0x01000000;
	*DWT_CYCCNT = 0; // reset the counter
	*DWT_CONTROL = *DWT_CONTROL | 1 ; // enable the counte
}
void cpucycleCnt_get(const u8 *p)
{
	printf("%s; statnd_cyclecount = %#08x\n",p,*DWT_CYCCNT);
	fflush(stdout);
	cpucycle_rst();
}

#endif
/**
 *  Application start
 */
void application_start( void )
{
    /* Initialise the device */
    wiced_init();
    /* Run the main application function */
    wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
    u8 m=0, n=0;
	u8 tree_num[3] = {1,2,3};
	u8 request_num[4] = {1,16,32,64};
	ramtest_titile_printf();
//	ramtest(tree_num[0],1);
#if 1
	for( m=0; m < 3; m++)
	{
		for( n=0; n < 4; n++)
		{
		    ramtest_handle(tree_num[m],request_num[n]);
		}
	}
	ramtest_end_printf();
#endif	
}
