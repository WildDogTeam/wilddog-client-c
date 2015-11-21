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

#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#ifndef WILDDOG_PORT_TYPE_MXCHIP
#include <unistd.h>
#include <malloc.h>
#endif
#include "wilddog.h"
#include "wilddog_url_parser.h"
#include "wilddog_api.h"
#include "wilddog_ct.h"
#include "test_lib.h"

#ifdef WILDDOG_PORT_TYPE_MXCHIP
#include "MicoDriverRtc.h"
#endif


#ifdef WILDDOG_PORT_TYPE_WICED
#include "wiced.h"
#endif

#if defined (WILDDOG_PORT_TYPE_ESP) && (defined WILDDOG_SELFTEST)
#include "user_interface.h"
extern os_timer_t test_timer2;
extern Wilddog_T wilddog;
#endif

#ifdef WILDDOG_SELFTEST

#define CUCL_UNIT_MS    1000000     /* unit of time in report table */

#define APPEND_TM(a)    do{ if((a) ==0 ) (a) = performtest_calDiffTime();\
                            else (a) = performtest_calDiffTime() - (a);}while(0)
#define GET_AVERAGE(a,b) do{if((a)==0) (a) = b;\
                            else (a) = ((a) + (b))/2;}while(0)
#define SYS_ISIN(s)     (g_performtest.d_sysState == (s))

#define TM_DIFF(a,b)    ((a>b)?(a-b):(b-a))

typedef enum{
    SYS_INIT,
    SYS_HSK,
    SYS_AUTHSENDING,    
    SYS_AUTHRECV,
    SYS_APPLICATIONSENDING,
    SYS_APPLICATIONRECV,
    SYS_APPLICATIONRECVDONE,
}Sys_State;

#if !defined(WILDDOG_PORT_TYPE_WICED) && !defined(WILDDOG_PORT_TYPE_MXCHIP)
#include <sys/time.h>
#endif

typedef struct PERFORMTERST_T
{
    u8 d_sendpackt;
    u8 d_send_fault;
    u8 d_recv;
    u8 d_recv_err;

    u8 request_num;
    u8 d_tm_indx;

    u8 d_sysState;

    
    u32 tree_num;
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
static int perform_count = 0;

#if !defined(WILDDOG_PORT_TYPE_WICED) && \
    !defined(WILDDOG_PORT_TYPE_QUCETEL) && \
    !defined(WILDDOG_PORT_TYPE_ESP) && \
    !defined(WILDDOG_PORT_TYPE_MXCHIP)
u32 WD_SYSTEM performtest_getSystime(void)
{

    struct timeval temtm; 
    gettimeofday(&temtm,NULL); 

    return (u32)(temtm.tv_sec*1000000 + temtm.tv_usec);
}
#endif

#ifdef WILDDOG_PORT_TYPE_ESP
u32 WD_SYSTEM performtest_getSystime(void)
{
    return system_get_time();

}

#endif

#ifdef WILDDOG_PORT_TYPE_QUCETEL
#include "Ql_time.h"
u32 WD_SYSTEM performtest_getSystime(void)
{
    u32 seconds;
    ST_Time time;
    Ql_GetLocalTime(&time);
    seconds = Ql_Mktime(&time);
    return seconds * 1000000;
}
#endif

#ifdef WILDDOG_PORT_TYPE_MXCHIP
u32 WD_SYSTEM performtest_getSystime(void)
{
    mico_rtc_time_t time;
    MicoRtcGetTime(&time);
    return time.sec  * 1000000;
}


#endif
#ifdef WILDDOG_PORT_TYPE_WICED

#define CPU_CCT 120000000                   /* cpu frequency */
int statnd_cyclecount = 0;
/* address of the register */
volatile unsigned int *DWT_CYCCNT = (unsigned int *)0xE0001004; 
/* address of the register */
volatile unsigned int *DWT_CONTROL = (unsigned int *)0xE0001000; 
/* address of the registert */
volatile unsigned int *SCB_DEMCR = (unsigned int *)0xE000EDFC; 
/* rest clock */
int WD_SYSTEM cpucycle_rst(void)
{
    *SCB_DEMCR = *SCB_DEMCR | 0x01000000;
    *DWT_CYCCNT = 0;  
    *DWT_CONTROL = *DWT_CONTROL | 1 ;  
    return 0;
}

void WD_SYSTEM cpucycleCnt_get(const u8 *p)
{
    printf("%s; statnd_cyclecount = %#08x\n",p,*DWT_CYCCNT);
    fflush(stdout);
    cpucycle_rst();
}
u32 WD_SYSTEM performtest_getSystime(void)
{
    /*printf("DWT_CYCCNT = %u;clock US=%u\n",(*DWT_CYCCNT),(*DWT_CYCCNT) /120);*/
    return((*DWT_CYCCNT) /120);
}
#endif

void WD_SYSTEM performtest_timeReset(void)
{

#ifdef WILDDOG_PORT_TYPE_WICED
    cpucycle_rst();
    g_performtest.d_tm_star = 0;
/*  printf("d_tm_star=%u\n",g_performtest.d_tm_star);*/
#else 
    g_performtest.d_tm_star = performtest_getSystime();
/*  printf("d_tm_star=%u\n",g_performtest.d_tm_star);*/

#endif
}

u32 WD_SYSTEM performtest_calDiffTime(void)
{
    u32 diff_tm = performtest_getSystime();
    
#ifndef WILDDOG_PORT_TYPE_WICED
    diff_tm = TM_DIFF(diff_tm,g_performtest.d_tm_star);
#endif
    return diff_tm;
}
void WD_SYSTEM performtest_setSysState(u8 state)
{
    g_performtest.d_sysState = state;
}       
void WD_SYSTEM performtest_getDtlsHskTime(void)
{
    if(SYS_ISIN(SYS_HSK))
    {
        APPEND_TM(g_performtest.d_tm_dtls_hsk);
        performtest_setSysState(SYS_AUTHSENDING);
    }
}
void WD_SYSTEM performtest_getDtlsHskVerifyTime(void)
{
    if(SYS_ISIN(SYS_HSK))
        APPEND_TM(g_performtest.d_tm_dtls_hsk_verify);
}
void WD_SYSTEM performtest_getSessionQueryTime(void)
{
    APPEND_TM(g_performtest.d_tm_dtls_auth_send);
    performtest_setSysState(SYS_AUTHSENDING);
}
void WD_SYSTEM performtest_getWaitSessionQueryTime(void)
{
    if(SYS_ISIN(SYS_AUTHRECV)){
        APPEND_TM(g_performtest.d_tm_dtls_auth_wait);
        performtest_timeReset();
    }
}
void WD_SYSTEM performtest_getHandleSessionResponTime(void)
{
    if(SYS_ISIN(SYS_AUTHRECV))
    {
        APPEND_TM(g_performtest.d_tm_dtls_auth_handle);
        performtest_setSysState(SYS_APPLICATIONSENDING);
        performtest_timeReset();
    }
}
void WD_SYSTEM performtest_getSendTime(void)
{
    if(SYS_ISIN(SYS_APPLICATIONSENDING))
    {
        u32 temp = performtest_calDiffTime();
        GET_AVERAGE(g_performtest.d_tm_send,temp);
    }
}
void WD_SYSTEM performtest_getDtlsSendTime(void)
{
    if(SYS_ISIN(SYS_APPLICATIONSENDING) )
    {
        u32 temp = performtest_calDiffTime();
        GET_AVERAGE(g_performtest.d_tm_dtls_send,temp);
    }
 }
void WD_SYSTEM performtest_getWaitRecvTime(void)
{
    if(SYS_ISIN(SYS_APPLICATIONRECV))
    {
        APPEND_TM(g_performtest.d_tm_recv_wait);
        performtest_timeReset();
    }
}
void WD_SYSTEM performtest_getHandleRecvDtlsTime(void)
{
    if(SYS_ISIN(SYS_APPLICATIONRECV))
    {
        APPEND_TM(g_performtest.d_tm_recv_dtls);
    }
}
void WD_SYSTEM performtest_getHandleRecvTime(void)
{
    if(SYS_ISIN(SYS_APPLICATIONRECV))
    {
        APPEND_TM(g_performtest.d_tm_recv);
        performtest_setSysState(SYS_APPLICATIONRECVDONE);
    }
}
void WD_SYSTEM performtest_init( u32 delay_tm,u32 tree_num, u8 request_num)
{
    memset(&g_performtest,0,sizeof(g_performtest));
    g_performtest.tree_num = tree_num;
    g_performtest.request_num = request_num;
    g_performtest.d_tm_trysync_delay = delay_tm;
}
void WD_SYSTEM performtest_titile_printf(void)
{
    printf("\n>--------------------------Perform--Test-------------------\n");
    printf("NO\tQueries\tUnSend\tUDPSize\tLossRate\tTrysncDelay\t");
    printf("DtlsHandShake\tCertification\tSendAuth\tWaitAuth\tHandleAuth\t");
    printf("RequestSend\tDtlsEncrypt\tRecvWait\tDtlsDecypt\tHandleRecv\t| \n");
}
void WD_SYSTEM performtest_end_printf(void)
{
    printf(">--------------------------------------------------\n");
}
void WD_SYSTEM performtest_printf(Performtest_T *p)
{
    static char perform_indx=0;

    char tembuf[20];
    memset(tembuf,0,20);

    sprintf(tembuf,"%d/%d",p->d_recv_err,(p->request_num - p->d_send_fault));
    printf("%d",++perform_indx);
    printf("\t%d",p->request_num);
    printf("\t%d",p->d_send_fault); 
    printf("\t%ld",p->tree_num);
    printf("\t%s",tembuf);
    printf("\t\t%ld",p->d_tm_trysync_delay);
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

    printf("\n");

}
STATIC void WD_SYSTEM test_onQueryFunc(
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err)
{
    performtest_getHandleRecvTime();
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
//  wilddog_debug("query success =%d!",perform_count);
    return;
}


#ifndef WILDDOG_PORT_TYPE_ESP
void WD_SYSTEM performtest_handle
	(
	u32 delay_tm,
	const u8 *p_url,
	u32 tree_num, 
	u8 request_num
	)
{
    u8 m = 0;
    Wilddog_T wilddog = 0;
        
    performtest_init(delay_tm,tree_num,request_num);
    performtest_setSysState(SYS_HSK);

    wilddog = wilddog_initWithUrl((Wilddog_Str_T*)p_url);
    if(0 == wilddog)
    {
        return;
    }
    perform_count = 0;
    performtest_setSysState(SYS_AUTHRECV);
    performtest_timeReset();
    while(1)
    {
        wilddog_trySync();
        if(SYS_ISIN(SYS_APPLICATIONSENDING))
            break;
    }
    performtest_timeReset();
    for(m=0; m < request_num; m++)
    {
        performtest_timeReset();
        /*printf("g_performtest.d_tm_star = %ul\n", g_performtest.d_tm_star);*/
        int res = wilddog_getValue(wilddog, test_onQueryFunc, NULL);
        performtest_getSendTime();
        /*printf("g_performtest.d_tm_send = %ul\n", g_performtest.d_tm_send);*/
        if(0 == res)
            perform_count++;
        else
            g_performtest.d_send_fault++;
        /*printf("send =%d;res =%d \n",perform_count,res);*/
    }
    performtest_timeReset();
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
#if defined WILDDOG_PORT_TYPE_ESP
        os_delay_us(1000 * g_performtest.d_tm_trysync_delay);
#else
        usleep(g_performtest.d_tm_trysync_delay);
#endif
#endif
        wilddog_increaseTime(g_performtest.d_tm_trysync_delay);
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
    return;
}
#endif


#if defined (WILDDOG_PORT_TYPE_ESP) && (defined WILDDOG_SELFTEST)

void WD_SYSTEM perform_sync2(void)
{
    if(perform_count == 0)
    {
        performtest_printf(&g_performtest);
        os_timer_disarm(&test_timer2);
    }
    else
    {
        os_delay_us(1000 * g_performtest.d_tm_trysync_delay);

        wilddog_increaseTime(g_performtest.d_tm_trysync_delay);
        wilddog_trySync();

        os_timer_setfn(&test_timer2, (os_timer_func_t *)perform_sync2, NULL);
        os_timer_arm(&test_timer2, 1000, 0); 
    }


}
  
  
void WD_SYSTEM perform_sync(void)
{
    if(SYS_ISIN(SYS_APPLICATIONSENDING))
    {
        int m;
        u8 request_num= 1;//REQ_NUMBER;
        os_timer_disarm(&test_timer2);

        performtest_timeReset();
        for(m=0; m < request_num; m++)
        {
            performtest_timeReset();
            int res = wilddog_getValue(wilddog, test_onQueryFunc, NULL);
            performtest_getSendTime();
            if(0 == res)
                perform_count++;
            else
                g_performtest.d_send_fault++;
        }
        performtest_timeReset();
        performtest_setSysState(SYS_APPLICATIONRECV);

      
        os_timer_disarm(&test_timer2);
        os_timer_setfn(&test_timer2, (os_timer_func_t *)perform_sync2, NULL);
        os_timer_arm(&test_timer2, 1000, 0); 
      //wilddog_destroy(&wilddog);
    }
    else
    {
        wilddog_trySync();
        os_timer_setfn(&test_timer2, (os_timer_func_t *)perform_sync, NULL);        
        os_timer_arm(&test_timer2, 1000, 0);            
    }
}
  
  
void WD_SYSTEM performtest_handle
    (
    u32 delay_tm,
    const u8 *p_url,
    u32 tree_num, 
    u8 request_num
    )
{
    u8 m = 0;
  
      
    performtest_init(delay_tm,tree_num,request_num);
    performtest_setSysState(SYS_HSK);

    wilddog = wilddog_initWithUrl((Wilddog_Str_T*)p_url);
    if(0 == wilddog)
    {
        return;
    }
    perform_count = 0;
    wilddog_debug();
    performtest_setSysState(SYS_AUTHRECV);
    wilddog_debug();
    performtest_timeReset();
    wilddog_debug();

    os_timer_disarm(&test_timer2);
    os_timer_setfn(&test_timer2, (os_timer_func_t *)perform_sync, NULL);
    os_timer_arm(&test_timer2, 1000, 0); 

    return;
}
#endif


#endif

