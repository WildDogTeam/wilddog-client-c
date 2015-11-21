/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: ramtest.c
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
//#include "test_config.h"
#ifdef WILDDOG_PORT_TYPE_ESP    
#include "user_interface.h"
#include "os_type.h"
#define   ESP_HEAP_SIZE   (40*1024)
extern os_timer_t test_timer2;
#endif

#ifdef WILDDOG_PORT_TYPE_MXCHIP
#define MXCHIP_HEAP_SIZE    (0x14A00)
#endif

#define MAXINDXS    12
#ifdef WILDDOG_SELFTEST

static Ramtest_T d_ramtest;

#ifdef WILDDOG_PORT_TYPE_ESP    
extern os_timer_t test_timer2;
extern int l_res;
#endif

int ramtest_getLastMallocSize(Ramtest_T *p);
int ramtest_getSysRamusage(Ramtest_T *p,u32 *p_uage);


void WD_SYSTEM ramtest_init(u32 tree_num,u32 request_num)
{
    Ramtest_T *p = &d_ramtest;
    memset(p,0,sizeof(Ramtest_T));
    p->d_mallocblks_init = ramtest_getLastMallocSize(p);
    p->d_stackblks_init = ramtest_getLastStackSize(p);
    p->d_protocol_size = WILDDOG_PROTO_MAXSIZE ;
    p->tree_num = tree_num;
    p->request_num = request_num;
}
int WD_SYSTEM ramtest_getSysmallocSize(Ramtest_T *p)
{
    int res;
    
#if !defined(WILDDOG_PORT_TYPE_ESP) && !defined(WILDDOG_PORT_TYPE_MXCHIP)   
    struct mallinfo info;
    info = mallinfo();
    res = info.uordblks - p->d_mallocblks_init;
#else
#if defined(WILDDOG_PORT_TYPE_ESP)
    res = ESP_HEAP_SIZE - system_get_free_heap_size() - p->d_mallocblks_init;
#else
    res = MXCHIP_HEAP_SIZE - p->d_mallocblks_init;
#endif
#endif
    return res;
}
int WD_SYSTEM ramtest_getSysStackSize(Ramtest_T *p)
{
    return 0;
}
int WD_SYSTEM ramtest_printfmallocState(void)
{
#if !defined(WILDDOG_PORT_TYPE_ESP) && !defined(WILDDOG_PORT_TYPE_MXCHIP)   
    struct mallinfo info;
    info = mallinfo();
    printf("arena=%d;uordblks=%d,ordblks =%d;fordblks=%d\n",
            info.arena,info.uordblks,info.ordblks,info.fordblks);
#else
#endif
    return 0;
}
int WD_SYSTEM ramtest_getLastMallocSize(Ramtest_T *p)
{
    int res;
#if !defined(WILDDOG_PORT_TYPE_ESP) && !defined(WILDDOG_PORT_TYPE_MXCHIP)   
    struct mallinfo info;
    info = mallinfo();
    res = info.uordblks - p->mallocblks;
    p->mallocblks = info.uordblks;

#else 
#if defined(WILDDOG_PORT_TYPE_ESP)
    res = ESP_HEAP_SIZE - system_get_free_heap_size() - p->mallocblks;
    p->mallocblks = ESP_HEAP_SIZE - system_get_free_heap_size();
#else
    res = MXCHIP_HEAP_SIZE  - p->mallocblks;
    p->mallocblks = MXCHIP_HEAP_SIZE ;
#endif
#endif
    return res;
}
void WD_SYSTEM ramtest_gethostbyname(void)
{
    d_ramtest.d_gethostbyname = ramtest_getLastMallocSize(&d_ramtest);
}
void WD_SYSTEM ramtest_skipLastmalloc(void)
{
    ramtest_getLastMallocSize(&d_ramtest);
}
void WD_SYSTEM ramtest_getAveragesize(void)
{
    static u32 count = 0 ;
    ramtest_getSysRamusage(&d_ramtest,&d_ramtest.d_average_ram);
    if(count == 0)
        d_ramtest.d_average_ram /=2;
}
int WD_SYSTEM ramtest_getLastStackSize(Ramtest_T *p)
{
    return 0;
}
int WD_SYSTEM ramtest_getSysRamusage(Ramtest_T *p,u32 *p_uage)
{
    /* reduce gethhostbyname's ram */
    u32 usage = ((u32)ramtest_getSysStackSize(p) + \
                (u32)ramtest_getSysmallocSize(p)) - (u32)p->d_gethostbyname;
     *p_uage += usage;
     return usage;
}
int WD_SYSTEM ramtest_getLastRamusage(Ramtest_T *p,u32 *p_uage)
{
     *p_uage = ((u32)ramtest_getLastStackSize(p) + (u32)ramtest_getLastMallocSize(p));
     return *p_uage;
}
void WD_SYSTEM ramtest_caculate_nodeRam(void)
{
    ramtest_getLastRamusage(&d_ramtest,&d_ramtest.d_node_ram);
}
void WD_SYSTEM ramtest_caculate_requestQueueRam(void)
{    
    u32 quequeram =0;
    ramtest_getLastRamusage(&d_ramtest,&quequeram);
    d_ramtest.d_requestQeue_ram += quequeram;

}
void WD_SYSTEM ramtest_caculate_x509Ram(void)
{
    u32 ram_x509 =0;
    ramtest_getLastRamusage(&d_ramtest,&ram_x509);
    d_ramtest.d_x509_ram += ram_x509;
}
void WD_SYSTEM ramtest_caculate_packetsize(unsigned short packetSize)
{
    d_ramtest.d_packet_size = packetSize ;
}
void WD_SYSTEM ramtest_caculate_averageRam(void)
{
/*  todo */
/*  ramtest_getLastRamusage(&d_ramtest,&d_ramtest.d_average_ram);*/
}
int WD_SYSTEM ramtest_get_averageRam(void)
{
    return d_ramtest.d_average_ram;

}
void WD_SYSTEM ramtest_caculate_peakRam(void)
{
    u32 d_ramtem=0;
    ramtest_getSysRamusage(&d_ramtest,&d_ramtem);
    d_ramtest.d_peak_ram = 
        (d_ramtest.d_peak_ram>d_ramtem)?d_ramtest.d_peak_ram:d_ramtem;
}
void WD_SYSTEM ramtest_titile_printf(void)
{
    printf("\n---------------------------RAM--test-------------------------\n");
    printf("NO\tQueries\tUnSend\tErrorRecv\tUDPSize\tPeakMemory\tAverageMemory"
           "\tRequestQueueMemory\tX509Memory\tNodeTreeMemory\t| \n");
}
void WD_SYSTEM ramtest_end_printf(void)
{
    printf("----------------------------------------------------------------\n");   
}
void WD_SYSTEM ramtest_printf(Ramtest_T *p)
{
    static u8 index_tem = 0;

    printf("%d",++index_tem);

    printf("\t%ld",p->request_num);
    printf("\t%ld",p->d_sendfalt);
    printf("\t%ld",p->d_recverr);
    printf("\t\t%ld",p->tree_num);
    printf("\t%ld",p->d_peak_ram);
    printf("\t\t%ld",p->d_average_ram);
    printf("\t\t%ld",p->d_requestQeue_ram);
    printf("\t\t\t%ld",p->d_x509_ram);
    printf("\t\t%ld",p->d_node_ram);

    printf("\n");

}

static int count = 0;

STATIC void WD_SYSTEM test_onQueryFunc(
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err)
{
    static u8 firstin = 0;
    if(firstin == 0 )
    {
        firstin = 1;
    }   
    count = (count <= 0)?0:count - 1;
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        d_ramtest.d_recverr++;
        return;
    }
#if 0   
    printf("\n");
    if(p_snapshot)
        wilddog_debug_printnode(p_snapshot);
    
    printf("\n");
#endif      

    return;
}


#ifndef WILDDOG_PORT_TYPE_ESP

int WD_SYSTEM ramtest_handle( const u8 *p_url,u32 tree_num, u8 request_num)
{
    u8 m = 0;
    Wilddog_T wilddog = 0;
    

    ramtest_init(tree_num,request_num);
    
    wilddog = wilddog_initWithUrl((Wilddog_Str_T*)p_url);
        
    if(0 == wilddog)
    {
        return -1;
    }
    count = 0;
    for(m=0; m < request_num; m++)
    {
        int res = wilddog_getValue(wilddog, test_onQueryFunc, NULL);
        if(0 == res)
            count++;
        else
            d_ramtest.d_sendfalt++;
    }
    ramtest_caculate_requestQueueRam();
    while(1)
    {
        if(count == 0)
        {
            
            ramtest_printf(&d_ramtest);
            break;
        }
        ramtest_getAveragesize();
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
    return 0;
}
#endif



#ifdef WILDDOG_PORT_TYPE_ESP

void WD_SYSTEM ram_sync(void)
{
    if(count == 0)
    {
        os_timer_disarm(&test_timer2);
        ramtest_printf(&d_ramtest);
    }
    else
    {
        ramtest_getAveragesize();
        wilddog_trySync();
        os_timer_setfn(&test_timer2, (os_timer_func_t *)ram_sync, NULL);
        os_timer_arm(&test_timer2, 1000, 0);            
    }
}


int WD_SYSTEM ramtest_handle( const u8 *p_url,u32 tree_num, u8 request_num)
{
    u8 m = 0;
    Wilddog_T wilddog = 0;
	
    ramtest_init(tree_num,request_num);
	
    wilddog = wilddog_initWithUrl((Wilddog_Str_T*)p_url);
		
    if(0 == wilddog)
    {
        return -1;
    }
    count = 0;
    for(m=0; m < request_num; m++)
    {
        int res = wilddog_getValue(wilddog, test_onQueryFunc, NULL);
        if(0 == res)
            count++;
        else
            d_ramtest.d_sendfalt++;
    }
    ramtest_caculate_requestQueueRam();


    os_timer_disarm(&test_timer2);
    os_timer_setfn(&test_timer2, (os_timer_func_t *)ram_sync, NULL);
    os_timer_arm(&test_timer2, 1000, 0); 

    return 0;
}

#endif


#endif

