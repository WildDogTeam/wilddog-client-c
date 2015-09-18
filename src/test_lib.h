/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: list_lib.h
 *
 * Description: connection functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        lsx       2015-07-6  Create file.
 *
 */

#ifndef _WILDDOG_TEST_LIB_H_
#define _WILDDOG_TEST_LIB_H_

#include "wilddog.h"
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef WILDDOG_SELFTEST

typedef struct RAMTEST_T{
    u32 tree_num;
    u32 request_num;
    u32 d_sendfalt;
    u32 d_recverr;
    u32 d_node_ram;
    u32 d_x509_ram;
    u32 d_requestQeue_ram;
    u32 d_average_ram;
    u32 d_peak_ram;
    u32 d_packet_size;
    u32 d_protocol_size;
    u32 d_gethostbyname;

    u32 d_sys_ramusage;
    u32 d_mallocblks_init;      
    u32 d_stackblks_init;   
    u32 mallocblks;
    u32 stackblks;
    
}Ramtest_T;


#define NODE_INDEX_RAMTST   1
#define REQUESTCNT_RAMTST       1

#define TEST_URL_END ".wilddogio.com"
/* tree path */
#define TEST_TREE_T_127 "/tree_127"
#define TEST_TREE_T_256 "/tree_256"
#define TEST_TREE_T_576 "/tree_576"
#define TEST_TREE_T_810 "/tree_810"
#define TEST_TREE_T_1044    "/tree_1044"
#define TEST_TREE_T_1280    "/tree_1280"

#define TEST_TREE_ITEMS     5
#define TEST_PROTO_COVER        100


/* ram test_api*/
extern void ramtest_init(u32 tree_num,u32 request_num);
extern void ramtest_titile_printf(void);
extern void ramtest_end_printf(void);
extern void ramtest_getAveragesize(void);
extern int ramtest_getLastStackSize(Ramtest_T *p);
extern int ramtest_getSysRamusage(Ramtest_T *p,u32 *p_uage);
extern void ramtest_caculate_nodeRam(void);
extern void ramtest_caculate_x509Ram(void);
extern void ramtest_caculate_requestQueueRam(void);
extern void ramtest_caculate_averageRam(void);
extern int ramtest_get_averageRam(void);
extern void ramtest_caculate_peakRam(void);
extern void ramtest_skipLastmalloc(void);
extern void ramtest_gethostbyname(void);
extern void ramtest_caculate_packetsize(unsigned short packetSize);
extern int ramtest_printfmallocState(void);
extern int ramtest_handle( const u8 *p_url,u32 tree_num, u8 request_num);

extern void performtest_timeReset(void);
extern void performtest_getDtlsHskTime(void);
extern void performtest_getDtlsHskVerifyTime(void);
extern void performtest_getSessionQueryTime(void);
extern void performtest_getWaitSessionQueryTime(void);
extern void performtest_getHandleSessionResponTime(void);
extern void performtest_getSendTime(void);
extern void performtest_getDtlsSendTime(void);
extern void performtest_getWaitRecvTime(void);
extern void performtest_getHandleRecvDtlsTime(void);
extern void performtest_getHandleRecvTime(void);
extern void performtest_titile_printf(void);
extern void performtest_end_printf(void);
extern void performtest_handle
    (
    u32 delay_tm,
    const u8 *p_url,
    u32 tree_num, 
    u8 request_num
    );

#endif /* WILDDOG_SELFTEST*/
#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_TEST_LIB_H_*/

