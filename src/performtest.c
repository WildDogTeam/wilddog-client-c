/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: performtest.c
 *
 * Description: connection functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        lsx       2015-07-6  Create file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include "wilddog.h"
#include "wilddog_url_parser.h"
#include "wilddog_api.h"
#include "wilddog_ct.h"
#include "test_lib.h"

#ifdef WILDDOG_PORT_TYPE_WICED
#include "wiced.h"
#endif

#ifdef WILDDOG_SELFTEST

#define CUCL_UNIT_MS	1000000		/* unit of time in report table */

#define APPEND_TM(a)	do{	if((a) ==0 ) (a) = performtest_cacl_tm();\
							else (a) = performtest_cacl_tm() - (a);}while(0)
#define GET_AVERAGE(a,b) do{if((a)==0) (a) = b;\
							else (a) = ((a) + (b))/2;}while(0)
#define SYS_ISIN(s)		(g_performtest.d_sysState == (s))

#define TM_DIFF(a,b)	((a>b)?(a-b):(b-a))

typedef enum{
	SYS_INIT,
	SYS_HSK,
	SYS_AUTHSENDING,	
	SYS_AUTHRECV,
	SYS_APPLICATIONSENDING,
	SYS_APPLICATIONRECV,
	SYS_APPLICATIONRECVDONE,
}Sys_State;

#ifndef WILDDOG_PORT_TYPE_WICED

#include <sys/time.h>
#endif
typedef struct PERFORMTERST_T
{
	u8 d_sendpackt;
	u8 d_send_fault;
	u8 d_recv;
	u8 d_recv_err;

	u8 tree_num;
	u8 request_num;
	u8 d_tm_indx;

	u8 d_sysState;
	
	u32 d_tm_dtels;
	u32 d_tm_packetsize;
	u32 d_tm_requestcnt;
	u32 d_tm_trysync_delay;
	u32 d_tm_dtls_hsk;
	u32 d_tm_dtls_hsk_verify;
	u32 d_tm_dtls_auth_send;
	u32 d_tm_dtls_auth_wait;
	u32 d_tm_dtls_auth_handle;

	u32 d_tm_send;
	u32 d_tm_dtls_send;
	u32 d_tm_recv_wait;
	u32 d_tm_recv_dtls;
	u32 d_tm_recv;
	u32 d_tm_star;

}Performtest_T;
static Performtest_T g_performtest;
static int tree2len[]={64,127,576,1280};
static int perform_count = 0;

#ifndef WILDDOG_PORT_TYPE_WICED
u32 performtest_sys_ustm(void)
{

	struct timeval temtm; 
	gettimeofday(&temtm,NULL); 

	return (u32)(temtm.tv_sec*1000000 + temtm.tv_usec);
}
#endif

#ifdef WILDDOG_PORT_TYPE_WICED

#define CPU_CCT	120000000					/* cpu frequency */
int statnd_cyclecount = 0;
volatile unsigned int *DWT_CYCCNT = (unsigned int *)0xE0001004; /* address of the register */
volatile unsigned int *DWT_CONTROL = (unsigned int *)0xE0001000; /* address of the register */
volatile unsigned int *SCB_DEMCR = (unsigned int *)0xE000EDFC; /* address of the registert */
/* rest clock */
int cpucycle_rst(void)
{
	*SCB_DEMCR = *SCB_DEMCR | 0x01000000;
	*DWT_CYCCNT = 0;  
	*DWT_CONTROL = *DWT_CONTROL | 1 ;  
	return 0;
}

void cpucycleCnt_get(const u8 *p)
{
	printf("%s; statnd_cyclecount = %#08x\n",p,*DWT_CYCCNT);
	fflush(stdout);
	cpucycle_rst();
}
u32 performtest_sys_ustm(void)
{
	/*printf("DWT_CYCCNT = %u;clock US=%u\n",(*DWT_CYCCNT),(*DWT_CYCCNT) /120);*/
	return((*DWT_CYCCNT) /120);
}
#endif

u32 performtest_getSys_tm(u32 d_systm)
{
	return performtest_sys_ustm();
}
void performtest_star_tm(void)
{

#ifdef WILDDOG_PORT_TYPE_WICED
	cpucycle_rst();
	g_performtest.d_tm_star = 0;
/*	printf("d_tm_star=%u\n",g_performtest.d_tm_star);*/
#else 
	g_performtest.d_tm_star = performtest_sys_ustm();
/*	printf("d_tm_star=%u\n",g_performtest.d_tm_star);*/

#endif
}

u32 performtest_cacl_tm(void)
{
	u32 diff_tm = performtest_sys_ustm();
	
/*	printf("performtest_cacl_tm :d_tm_star=%u;diff_tm=%u\n",g_performtest.d_tm_star,diff_tm);*/
#ifndef WILDDOG_PORT_TYPE_WICED
	diff_tm = TM_DIFF(diff_tm,g_performtest.d_tm_star);
#endif
	return diff_tm;
}
void performtest_setSysState(u8 state)
{
	g_performtest.d_sysState = state;
}		
void performtest_tm_getDtlsHsk(void)
{
	if(SYS_ISIN(SYS_HSK))
	{
		APPEND_TM(g_performtest.d_tm_dtls_hsk);
		performtest_setSysState(SYS_AUTHSENDING);
	}
}
void performtest_tm_getDtlsHskVerify(void)
{
	if(SYS_ISIN(SYS_HSK))
		APPEND_TM(g_performtest.d_tm_dtls_hsk_verify);
}
void performtest_tm_getAuthSend(void)
{
	APPEND_TM(g_performtest.d_tm_dtls_auth_send);
	performtest_setSysState(SYS_AUTHSENDING);
}
void performtest_tm_getAuthWait(void)
{
	if(SYS_ISIN(SYS_AUTHRECV)){
		APPEND_TM(g_performtest.d_tm_dtls_auth_wait);
		performtest_star_tm();
	}
}
void performtest_tm_getAuthHandle(void)
{
	if(SYS_ISIN(SYS_AUTHRECV))
	{
		APPEND_TM(g_performtest.d_tm_dtls_auth_handle);
		performtest_setSysState(SYS_APPLICATIONSENDING);
		performtest_star_tm();
	}
}
void performtest_tm_getSend(void)
{
	if(SYS_ISIN(SYS_APPLICATIONSENDING))
	{
		u32 temp = performtest_cacl_tm();
		GET_AVERAGE(g_performtest.d_tm_send,temp);
	}
}
void performtest_tm_getDtlsSend(void)
{
	if(SYS_ISIN(SYS_APPLICATIONSENDING) )
	{
		u32 temp = performtest_cacl_tm();
		GET_AVERAGE(g_performtest.d_tm_dtls_send,temp);
	}
 }
void performtest_tm_getRecv_wait(void)
{
	if(SYS_ISIN(SYS_APPLICATIONRECV))
	{
		APPEND_TM(g_performtest.d_tm_recv_wait);
		performtest_star_tm();
	}
}
void performtest_tm_getRecvDtls(void)
{
	if(SYS_ISIN(SYS_APPLICATIONRECV))
	{
		APPEND_TM(g_performtest.d_tm_recv_dtls);
	}
}
void performtest_tm_getRecv(void)
{
	if(SYS_ISIN(SYS_APPLICATIONRECV))
	{
		APPEND_TM(g_performtest.d_tm_recv);
		performtest_setSysState(SYS_APPLICATIONRECVDONE);
	}
}
void performtest_tm_printf(void)
{
	if(SYS_ISIN(SYS_APPLICATIONRECV))
	{
		static u32 tm_temp=0;
		APPEND_TM(tm_temp);
		printf("performtest_tm =%ld\n",tm_temp);
	}
}
void performtest_init( u32 delay_tm,u8 tree_num, u8 request_num)
{
	memset(&g_performtest,0,sizeof(g_performtest));
	g_performtest.tree_num = tree_num;
	g_performtest.request_num = request_num;
	g_performtest.d_tm_trysync_delay = delay_tm;
}
void performtest_titile_printf(void)
{
	printf("\n>--------------------------------------------------------------------------");
	printf("----------运行时间--测试-------------------------------------------------------------------------------<\n");
	printf("次数\t请求数\t未发请求\t数据包大小\t丢包率\tDTLS\tTrysncDelay\tDTLS握手\t签名认证\t发出Auth");
	printf("\t等待Auth\t处理Auth\t发出请求\tDTLS封包\t等待接收\tDTLS解包\t处理接收\t|\n");	
}
void performtest_end_printf(void)
{
	printf(">--------------------------------------------------------------------------");
	printf("---------------------------------------------------------------------------------------------<\n");	
}
void performtest_printf(Performtest_T *p)
{
	static char perform_indx=0;

	char tembuf[20];
	memset(tembuf,0,20);

	sprintf(tembuf,"%d/%d",p->d_recv_err,(p->request_num - p->d_send_fault)/*,p->d_recv*/);
	printf("%d",++perform_indx);
	printf("\t%d",p->request_num);
	printf("\t%d",p->d_send_fault);	
	printf("\t\t%d",tree2len[p->tree_num]);
	printf("\t\t%s",tembuf);

	printf("\t%ld",p->d_tm_dtels);
	printf("\t%ld",p->d_tm_trysync_delay);
	printf("\t\t%ld",p->d_tm_dtls_hsk);
	printf("\t\t%ld",p->d_tm_dtls_hsk_verify);	
	printf("\t\t%ld",p->d_tm_dtls_auth_send);
	printf("\t\t%ld",p->d_tm_dtls_auth_wait);
	printf("\t\t%ld",p->d_tm_dtls_auth_handle);

	printf("\t\t%ld",p->d_tm_send);
	printf("\t\t%ld",p->d_tm_dtls_send);
	printf("\t\t%ld",p->d_tm_recv_wait);
	printf("\t\t%ld",p->d_tm_recv_dtls);
	printf("\t\t%ld",p->d_tm_recv);

	printf("\t\t|\n");

}
STATIC void test_onQueryFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg, 
	Wilddog_Return_T err)
{
	performtest_tm_getRecv();
	perform_count = (perform_count <= 0)?0:perform_count - 1;
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		//wilddog_debug("query error = %d!",err);
		g_performtest.d_recv_err++;
		return;
	}
#if 0	
	printf("\n");
	if(p_snapshot)
		wilddog_debug_printnode(p_snapshot);
	
	printf("\n");
#endif		
//	wilddog_debug("query success =%d!",perform_count);
	return;
}

void performtest_handle( u32 delay_tm,u8 tree_num, u8 request_num)
{
	u8 m = 0;
	Wilddog_T wilddog = 0;
    u8 url[64]={0};
	
    sprintf((char*)url, "coaps://c_test.wilddogio.com/performtest/tree_%d", tree2len[tree_num]);
	
	performtest_init(delay_tm,tree_num,request_num);
	performtest_setSysState(SYS_HSK);

	wilddog = wilddog_initWithUrl(url);
		  
	if(0 == wilddog)
	{
		return;
	}
	perform_count = 0;
	performtest_setSysState(SYS_AUTHRECV);
	performtest_star_tm();
	while(1)
	{
		wilddog_trySync();
		if(SYS_ISIN(SYS_APPLICATIONSENDING))
			break;
	}
	performtest_star_tm();
	for(m=0; m < request_num; m++)
	{
		performtest_star_tm();
		/*printf("g_performtest.d_tm_star = %ul\n", g_performtest.d_tm_star);*/
		int res = wilddog_getValue(wilddog, test_onQueryFunc, NULL);
		performtest_tm_getSend();
		/*printf("g_performtest.d_tm_send = %ul\n", g_performtest.d_tm_send);*/
				if(0 == res)
			perform_count++;
		else
			g_performtest.d_send_fault++;
		/*printf("send =%d;res =%d \n",perform_count,res);*/
	}
	performtest_star_tm();
	performtest_setSysState(SYS_APPLICATIONRECV);
	while(1)
	{
		if(perform_count == 0)
		{
			//printf("break\n");
			performtest_printf(&g_performtest);
			break;
		}
#ifdef WILDDOG_PORT_TYPE_WICED
		wiced_rtos_delay_milliseconds(g_performtest.d_tm_trysync_delay);
#else 
		usleep(g_performtest.d_tm_trysync_delay);
#endif
		wilddog_increaseTime(g_performtest.d_tm_trysync_delay);
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);
	return;
}
void performtest_all(void)
{
	u8 m=0,n=0,d=0;	
	u8 tree_num[3] = {1,2,3};
	u8 request_num[4] = {1,16,32,64};
	u32 delay_tm[5] = {0,50,100,250,500};
	performtest_titile_printf();
	for(d=0; d<2; d++)
	{
		for(n=0; n<4; n++)
		{
			for(m=0; m<3; m++)
			{
				performtest_handle(delay_tm[d],tree_num[m],request_num[n]);
#ifdef WILDDOG_PORT_TYPE_WICED
				wiced_rtos_delay_milliseconds(2000);
#endif
			}
		}
	}
	
	performtest_end_printf();
	
}

#endif

