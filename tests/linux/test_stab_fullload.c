/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: test_stab.c
 *
 * Description: connection functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        lxs       2015-07-18  Create file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#if defined(WILDDOG_PORT_TYPE_WICED)
#include "wiced.h"
#include "wifi_config_dct.h"
#else
#include <unistd.h>
#endif
#include "wilddog.h"
#include "wilddog_url_parser.h"
#include "wilddog_api.h"
#include "wilddog_ct.h"
#include "test_lib.h"
#include "test_config.h"

#ifdef WILDDOG_SELFTEST

#define STABTEST_ONEHOUR    (3600000)
#define STAB_DEBUG	0

#define STABTEST_PATH	"stabtest/"
#define STAB_KEY		"K"
#define STAB_DATA		"D"
#define STABTEST_KEY	"stability_key"
#define STABTEST_VALUE	"stability_value:"

#define STABTEST_ONREQUEST(cmd)		((cmd) == STABTEST_CMD_ON)
#define STABTEST_OFFREQUEST(cmd)	((cmd) == STABTEST_CMD_OFF)
#define STABTEST_NEXTREQUEST(cmd)	((cmd) = ((cmd) == STABTEST_CMD_OFF)? \
												STABTEST_CMD_ON:((cmd)+1))

typedef enum _STABTEST_CMD_TYPE
{
    STABTEST_CMD_NON = 0,
    STABTEST_CMD_ON,
    STABTEST_CMD_GET,
    STABTEST_CMD_SET,
    STABTEST_CMD_PUSH,
    STABTEST_CMD_DELE,
	STABTEST_CMD_OFF,
	    
}STABTEST_CMD_TYPE;
typedef struct STAB_SETDATA_T
{
	u8 key[10];
	u8 data[10];
	u8 setfault;
	Wilddog_Node_T *p_node;
	Wilddog_T client;
}Stab_Setdata_T;
STATIC u32 stab_runtime;
STATIC u32 stab_requests;
STATIC u32 stab_requestFault;
STATIC u32 stab_recvFault;
STATIC u32 stab_recvSucc;
STATIC u32 stab_settest_request;
STATIC u32 stab_settest_fault;
STATIC u32 stab_settest_getsuccess;
STATIC u32 stab_settest_setsuccess;

STATIC u32 stab_cmd;
STATIC Stab_Setdata_T stab_setdata[10];
STATIC u8 serialgetsend_cnt;



STATIC void stab_set_runtime(void)
{
#if defined(WILDDOG_PORT_TYPE_WICED)
	static u32 stab_startime =0;
	u32 currentTm_ms =0;
 	wiced_time_t t1;	
 	wiced_time_get_time(&t1);
	currentTm_ms = (u32)t1;

	if(stab_startime == 0 )
		stab_startime = currentTm_ms;

	stab_runtime = currentTm_ms - stab_startime;
#endif
}

STATIC void stab_get_requestRes(Wilddog_Return_T res)
{
	if(res < 0 )
	{
		printf("\tin %lu; send %lu requestErr= %d\n",stab_runtime,stab_cmd,res);
		stab_requestFault++;
	}
	else
	{
			/* off with no recv callback*/
		if(stab_cmd == STABTEST_CMD_OFF)
			stab_recvSucc++;
		}
	stab_requests++;	
}
STATIC void stab_get_recvErr(Wilddog_Return_T err,u32 methtype)
{
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		printf("in %lu; methtype = %lu recvErr= %d \n",stab_runtime,methtype,err);
		if(err == WILDDOG_ERR_RECVTIMEOUT)
			stab_recvFault++;
	}
	else
		stab_recvSucc++;
}

STATIC void stab_trysync(void)
{
	stab_set_runtime();
	
	ramtest_getAveragesize();
	/*Handle the event and callback function, it must be called in a special frequency*/
	wilddog_trySync();

}

void stab_titlePrint(void)
{
	printf("\t>----------------------------------------------------<\n");
	printf("\tcount\truntime\tram\tUnlaunchRatio\tLostRatio\tSuccessRatio\tSuccessSet\n");
}
void stab_endPrint(void)
{
	printf("\t>----------------------------------------------------<\n");
}

void stab_resultPrint(void)
{
	char unlaunchRatio[20];
	char lossRatio[20];	
	char successRatio[20];
	char settest_succRatio[20];
	static u32 run_cnt =0;
#if defined(WILDDOG_PORT_TYPE_WICED)
	if(stab_runtime/STABTEST_ONEHOUR <= run_cnt)
	       return ;
#endif	       
	memset(unlaunchRatio,0,20);
	memset(lossRatio,0,20);
	memset(successRatio,0,20);
	memset(settest_succRatio,0,20);

	sprintf(unlaunchRatio,"%lu/%lu",stab_requestFault,stab_requests);
	sprintf(lossRatio,"%lu/%lu",stab_recvFault,stab_requests);	
	sprintf(successRatio,"%lu/%lu",stab_recvSucc,stab_requests);
	sprintf(settest_succRatio,"(%lu)%lu/%lu",stab_settest_fault,stab_settest_getsuccess,stab_settest_request);
	
	printf("\t%lu",++run_cnt);		
	printf("\t%lu",stab_runtime);
	printf("\t%lu",(u32)ramtest_get_averageRam());
	printf("\t%s",unlaunchRatio);
	printf("\t\t%s",lossRatio);
	printf("\t\t%s",successRatio);
	printf("\t\t%s",settest_succRatio);
	printf("\n");
	return;
}
STATIC	void stab_settest_dataInit(u8 idx)
{
	int i;
    char temp_url[strlen(TEST_URL)+20];

    memset(temp_url,0,sizeof(temp_url));
	for(i=0;i<10;i++)
	{
		stab_setdata[i].key[0] = 'K';
		stab_setdata[i].data[0] = 'D';
		
		stab_setdata[i].key[1] = 0x30+i; 		
		stab_setdata[i].data[1] = 0x30+idx; 
		stab_setdata[i].data[2] = 0x30+i;
		sprintf((char*)temp_url,"%s/%s",TEST_URL,stab_setdata[i].key);
 		if(stab_setdata[i].client)
			wilddog_destroy(&(stab_setdata[i].client));
		stab_setdata[i].client = wilddog_initWithUrl((Wilddog_Str_T*)temp_url);
		if(stab_setdata[i].p_node)
		{
			wilddog_node_delete(stab_setdata[i].p_node);
			stab_setdata[i].p_node = NULL;
		}
		stab_setdata[i].p_node = wilddog_node_createUString(
									(Wilddog_Str_T *)stab_setdata[i].key,\
									(Wilddog_Str_T *)stab_setdata[i].data);
	}

}
STATIC void stab_settest_dataDeInit(void)
{
	int i;
	for(i=0;i<10;i++)
	{
		if(stab_setdata[i].p_node)
		{
 			wilddog_node_delete(stab_setdata[i].p_node);
 			stab_setdata[i].p_node = NULL;
		}
		if(stab_setdata[i].client)
		{
			
 			wilddog_destroy(&stab_setdata[i].client);
 		}
 		stab_setdata[i].setfault = 0;
	}
}

STATIC void stab_settest_serialSetValueFunc(void* arg, Wilddog_Return_T err)
{
                        
	stab_get_recvErr(err,STABTEST_CMD_SET);
	Stab_Setdata_T *p_set = (Stab_Setdata_T*)arg;
	stab_settest_setsuccess--;
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		p_set->setfault = 1;
#if STAB_DEBUG == 1
		wilddog_debug("set fault key = %s;data = %s ",p_set->key,p_set->data);
#endif
	}
	else
	
#if STAB_DEBUG == 1
		wilddog_debug("set success key = %s;data = %s ",p_set->key,p_set->data);
#endif
		
    return;
}
STATIC void stab_settest_serialSet_send(void)
{
	int i,res;
	stab_settest_setsuccess = 0;
	for(i=0;i<10;i++)
	{
		res = wilddog_setValue(stab_setdata[i].client,\
				stab_setdata[i].p_node,stab_settest_serialSetValueFunc,\
				(void*)&stab_setdata[i]);
		stab_get_requestRes(res);
		if(res>=0)
		{
			stab_setdata[i].setfault = 0;
			stab_settest_setsuccess++;
			stab_settest_request++; 
		}
		else
			stab_setdata[i].setfault=1;
	}
	while(1)
	{
 		if(stab_settest_setsuccess ==0 )
 			break;
		stab_trysync();
	}
}
STATIC void stab_settest_judge(Wilddog_Node_T* p_snapshot,void* arg)
{
    int len;
    
	Stab_Setdata_T *p_set1 = (Stab_Setdata_T*)arg;
	if(p_snapshot !=0 && arg != 0)
	{
		
		Stab_Setdata_T *p_set = (Stab_Setdata_T*)arg;
 		if( 0 == strcmp( (const char*)wilddog_node_getValue(p_snapshot,&len), \
			             (const char*)p_set->data))
		{
			stab_settest_getsuccess++;
			return ;
		}
		else
		{
			wilddog_debug("truevalue:%s,getvalue:%s\n", \
			              p_set1->data,wilddog_node_getValue(p_snapshot,&len));
			stab_settest_fault++;		
			}

	}
	
	return ;
}
STATIC void stab_settest_serialGetValueFunc
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err
    )
{
	stab_get_recvErr(err,STABTEST_CMD_GET);
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
		stab_settest_request = (stab_settest_request ==0 )?0:(stab_settest_request-1 );
		
	stab_settest_judge((Wilddog_Node_T*)p_snapshot,arg);
	
	serialgetsend_cnt--;
	 
    return;
}

STATIC void stab_settest_serialGet_send(void)
{
	Wilddog_Return_T res = 0;
	int i;
	serialgetsend_cnt =0;
	for(i=0;i<10;i++)
	{
		if(stab_setdata[i].setfault)
			continue;
			
		res = wilddog_getValue(stab_setdata[i].client,\
					stab_settest_serialGetValueFunc,(void*)&stab_setdata[i]);
		stab_get_requestRes(res);
		if(res>=0)
		{
			serialgetsend_cnt++;
			while(1)
			{
				if(serialgetsend_cnt == 0)
					break;
				stab_trysync();
			}
		}
	} 

}
void stab_test_fullLoad(void)
{
	int i;
	
	stab_titlePrint();
	ramtest_init(1,1);
	/* mark star time*/
	stab_set_runtime();
	while(1)
	{
		/*Create an node which type is an object*/
		for(i=0;i<10;i++)
		{
			stab_settest_dataInit(i);
 			stab_settest_serialSet_send();
 			stab_settest_serialGet_send(); 
#ifndef WILDDOG_PORT_TYPE_WICED
 			sleep(3);
#endif
#ifdef WILDDOG_PORT_TYPE_WICED
			wiced_rtos_delay_milliseconds(1000);
#endif

			stab_resultPrint();
			stab_settest_dataDeInit();
 		}

	}
}
#if defined(WILDDOG_PORT_TYPE_WICED)
#else
int main(void)
{
	stab_test_fullLoad();
	return 0;
}
#endif
#endif /* WILDDOG_SELFTEST*/

