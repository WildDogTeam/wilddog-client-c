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

#define MAXINDXS	12
#ifdef WILDDOG_SELFTEST

static Ramtest_T d_ramtest;

static int tree2len[]={64,127,576,1280};

int ramtest_getLastMallocSize(Ramtest_T *p);

void ramtest_init(u32 tree_num,u32 request_num)
{
	Ramtest_T *p = &d_ramtest;
	memset(p,0,sizeof(Ramtest_T));
	p->d_mallocblks_init = ramtest_getLastMallocSize(p);
	p->d_stackblks_init = ramtest_getLastStackSize(p);
	p->d_protocol_size = WILDDOG_PROTO_MAXSIZE ;
	p->tree_num = tree_num;
	p->request_num = request_num;
}
int ramtest_getSysmallocSize(Ramtest_T *p)
{
	int res;
	struct mallinfo info;
	info = mallinfo();
	res = info.uordblks - p->d_mallocblks_init;
	return res;
}

int ramtest_getSysStackSize(Ramtest_T *p)
{
	return 0;
}
int ramtest_printfmallocState(void)
{
	struct mallinfo info;
	info = mallinfo();
	printf("arena=%d;uordblks=%d,ordblks =%d;fordblks=%d\n",
			info.arena,info.uordblks,info.ordblks,info.fordblks);
	return 0;
}
int ramtest_getLastMallocSize(Ramtest_T *p)
{
	int res;
	struct mallinfo info;
	info = mallinfo();
	res = info.uordblks - p->mallocblks;
	p->mallocblks = info.uordblks;
	return res;
}
void ramtest_gethostbyname(void)
{
	d_ramtest.d_gethostbyname = ramtest_getLastMallocSize(&d_ramtest);
}
void ramtest_skipLastmalloc(void)
{
	ramtest_getLastMallocSize(&d_ramtest);
}
void ramtest_getAveragesize(void)
{
	static u32 count = 0 ;
	ramtest_getSysRamusage(&d_ramtest,&d_ramtest.d_average_ram);
	if(count == 0)
		d_ramtest.d_average_ram /=2;
}
int ramtest_getLastStackSize(Ramtest_T *p)
{
	return 0;
}
int ramtest_getSysRamusage(Ramtest_T *p,u32 *p_uage)
{
	/* reduce gethhostbyname's ram */
	u32 usage = ((u32)ramtest_getSysStackSize(p) + \
				(u32)ramtest_getSysmallocSize(p)) - (u32)p->d_gethostbyname;
	 *p_uage += usage;
	 return usage;
}
int ramtest_getLastRamusage(Ramtest_T *p,u32 *p_uage)
{
	 *p_uage = ((u32)ramtest_getLastStackSize(p) + (u32)ramtest_getLastMallocSize(p));
	 return *p_uage;
}
void ramtest_caculate_nodeRam(void)
{
	ramtest_getLastRamusage(&d_ramtest,&d_ramtest.d_node_ram);
}
void ramtest_caculate_requestQueueRam(void)
{	
	u32 quequeram =0;
	ramtest_getLastRamusage(&d_ramtest,&quequeram);
	d_ramtest.d_requestQeue_ram += quequeram;

}
void ramtest_caculate_x509Ram(void)
{
	u32 ram_x509 =0;
	ramtest_getLastRamusage(&d_ramtest,&ram_x509);
	d_ramtest.d_x509_ram += ram_x509;
}
void ramtest_caculate_packetsize(unsigned short packetSize)
{
	d_ramtest.d_packet_size = packetSize ;
}
void ramtest_caculate_averageRam(void)
{
/*	todo */
/*	ramtest_getLastRamusage(&d_ramtest,&d_ramtest.d_average_ram);*/
}
int ramtest_get_averageRam(void)
{
	return d_ramtest.d_average_ram;

}
void ramtest_caculate_peakRam(void)
{
	u32 d_ramtem=0;
	ramtest_getSysRamusage(&d_ramtest,&d_ramtem);
	d_ramtest.d_peak_ram = (d_ramtest.d_peak_ram>d_ramtem)?d_ramtest.d_peak_ram:d_ramtem;
}
void ramtest_titile_printf(void)
{
	printf("\n|---------------------------RAM--测试-----------------------------------------------");
	printf("---------------------------------------------------------------------------------------------|\n");
	printf("|\t次数\t请求数\t未发出请求数\t接收错误\tDTLS\tUDP数据包大小\t内存峰值\t平均内存占用\t请求队列占用内存\tX509占用内存\tNode树占用内存\t|\n");	
}
void ramtest_end_printf(void)
{
	printf("|--------------------------------------------------------------------------");
	printf("---------------------------------------------------------------------------------------------|\n");	
}
void ramtest_printf(Ramtest_T *p)
{
	char sectype='N';
	static u8 index_tem = 0;

	printf("|\t%d",++index_tem);

	printf("\t%ld",p->request_num);
	printf("\t%ld",p->d_sendfalt);
	printf("\t\t%ld",p->d_recverr);
	printf("\t\t%c",sectype);
	printf("\t%d",tree2len[p->tree_num]);
	printf("\t\t%ld",p->d_peak_ram);
	printf("\t\t%ld",p->d_average_ram);
	printf("\t\t%ld",p->d_requestQeue_ram);
	printf("\t\t\t%ld",p->d_x509_ram);
	printf("\t\t%ld",p->d_node_ram);

	printf("\t\t|\n");

}
/**  node
*/

static int count = 0;

STATIC void test_onQueryFunc(
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



void ramtest_handle( u8 tree_num, u8 request_num)
{
	u8 m = 0;
	Wilddog_T wilddog = 0;
	
	u8 url[64]={0};

	ramtest_init(tree_num,request_num);
    sprintf((char*)url, "coaps://c_test.wilddogio.com/ramtest/tree_%d", tree2len[tree_num]);
	
	wilddog = wilddog_initWithUrl(url);
		
	if(0 == wilddog)
	{
		return;
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
	return;
}

#endif

