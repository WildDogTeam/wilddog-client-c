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
#if SELFTEST_TYPE!=0

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
	u32 d_mallocblks_init;		/* init时的空闲malloc*/
	u32 d_stackblks_init;		/* init时的栈低*/
	u32 mallocblks;
	u32 stackblks;
	
}Ramtest_T;

static Ramtest_T d_ramtest;
static int ramtest_indx;
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
//	ramtest_getLastRamusage(&d_ramtest,&d_ramtest.d_average_ram);
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
	//if(WILDDOG_PORT == 5684)
	//	sectype = 'Y';
	printf("|\t%d",++index_tem);
	printf("\t%d",p->request_num);
	printf("\t%d",p->d_sendfalt);
	printf("\t\t%d",p->d_recverr);
	printf("\t\t%c",sectype);
	printf("\t%d",tree2len[p->tree_num]);
	printf("\t\t%d",p->d_peak_ram);
	printf("\t\t%d",p->d_average_ram);
	printf("\t\t%d",p->d_requestQeue_ram);
	printf("\t\t\t%d",p->d_x509_ram);
	printf("\t\t%d",p->d_node_ram);
	//printf("\n");
	printf("\t\t|\n");

}
/**  node
*/
#define TEST_GET_URL "coap://sky.wilddogio.com/"
static int count = 0;

STATIC void test_onQueryFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg, 
	Wilddog_Return_T err)
{
	static u8 firstin = 0;
	if(firstin == 0 )
	{
		//ramtest_caculate_peakRam();
		firstin = 1;
	}	
	count = (count <= 0)?0:count - 1;
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		//wilddog_debug("query error = %d!",err);
		d_ramtest.d_recverr++;
		return;
	}
#if 0	
	printf("\n");
	if(p_snapshot)
		wilddog_debug_printnode(p_snapshot);
	
	printf("\n");
#endif		
//	wilddog_debug("query success =%d!",count);
	return;
}

#define TEST_SET_URL_127	"coaps://mk.wilddogio.com/tree_127"
#define TEST_SET_URL_576	"coaps://mk.wilddogio.com/tree_576"
#define TEST_SET_URL_1280	"coaps://mk.wilddogio.com/tree_1280"


void ramtest_handle( u8 tree_num, u8 request_num)
{
	u8 m = 0;
	Wilddog_T wilddog = 0;
	Wilddog_Node_T * p_node = NULL;		
    ramtest_init(tree_num,request_num);
    u8 url[64]={0};
    sprintf(url, "coaps://mk.wilddogio.com/tree_%d", tree2len[tree_num]);
	wilddog_init();

	wilddog = wilddog_new(url);
		
	if(0 == wilddog)
	{
		return 0;
	}
	count = 0;
	for(m=0; m < request_num; m++)
	{
		int res = wilddog_query(wilddog, test_onQueryFunc, NULL);
		if(0 == res)
			count++;
		else
			d_ramtest.d_sendfalt++;
		//printf("send =%d;res =%d \n",count,res);
		//usleep(100);
	}
	ramtest_caculate_requestQueueRam();
	while(1)
	{
		if(count == 0)
		{
			//printf("break\n");
			ramtest_printf(&d_ramtest);
			break;
		}
		ramtest_getAveragesize();
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);
}
#if 0
void ramtest(u32 d_indx)
{
	u8 m=0,n=0;
	m = (d_indx-1)/4;
	n = (d_indx - 1)%4;
	
	if(d_indx > MAXINDXS)
		return ;
	u8 tree_num[3] = {1,2,3};
	u8 request_num[4] = {1,16,32,64};
	ramtest_handle(tree_num[m],request_num[n]);
	
}
#endif
/* sample*/
#if 0
int main(void)
{
	u8 m=0, n=0;
	u8 tree_num[3] = {1,2,3};
	u8 request_num[4] = {1,16,32,64};
	ramtest_titile_printf();
//	ramtest_handle(tree_num[0],1);
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
#endif
#endif

