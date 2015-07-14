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


#ifdef __cplusplus
extern "C"
{
#endif

#if SELFTEST_TYPE != 0

#define NODE_INDEX_RAMTST	1
#define REQUESTCNT_RAMTST		1

extern void ramtest_titile_printf(void);
extern void ramtest_end_printf(void);

extern void ramtest_caculate_nodeRam(void);
extern void ramtest_caculate_x509Ram(void);
extern void ramtest_caculate_requestQueueRam(void);
extern void ramtest_caculate_averageRam(void);
extern void ramtest_caculate_peakRam(void);
extern void ramtest_skipLastmalloc(void);
extern void ramtest_gethostbyname(void);
extern void ramtest_caculate_packetsize(unsigned short packetSize);
extern int ramtest_printfmallocState(void);


extern void ramtest_handle( u8 tree_num, u8 request_num);
extern void ramtest(u32 indx);
/* tm*/
extern void performtest_tm_printf(void);

extern void performtest_star_tm(void);
extern void performtest_tm_getDtlsHsk(void);
extern void performtest_tm_getDtlsHskVerify(void);
extern void performtest_tm_getAuthSend(void);
extern void performtest_tm_getAuthWait(void);
extern void performtest_tm_getAuthHandle(void);
extern void performtest_tm_getSend(void);
extern void performtest_tm_getDtlsSend(void);
extern void performtest_tm_getRecv_wait(void);
extern void performtest_tm_getRecvDtls(void);
extern void performtest_tm_getRecv(void);
extern void performtest_all(void);
extern void performtest_handle( u32 delay_tm,u8 tree_num, u8 request_num);


#endif /* SELFTEST_TYPE != 0*/
#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_TEST_LIB_H_*/

