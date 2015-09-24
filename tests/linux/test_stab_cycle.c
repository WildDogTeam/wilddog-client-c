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
	STABTEST_CMD_OFF
	    
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
STATIC u32 stab_pushSuccess;
STATIC u32 stab_changetime;


STATIC u32 stab_cmd;

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
		{
			stab_recvSucc++;
			}
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
	{
		if(methtype != STABTEST_CMD_GET )
			stab_changetime++;
		stab_recvSucc++;
	}
}

STATIC void stab_getValueFunc
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err
    )
{
	stab_get_recvErr(err,STABTEST_CMD_GET);
    *(BOOL*)arg = TRUE;

    return;
}

STATIC void stab_removeValueFunc(void* arg, Wilddog_Return_T err)
{
	stab_get_recvErr(err,STABTEST_CMD_DELE);
    *(BOOL*)arg = TRUE;

    return;
}
STATIC void stab_setValueFunc(void* arg, Wilddog_Return_T err)
{
                        
	stab_get_recvErr(err,STABTEST_CMD_SET);
	*(BOOL*)arg = TRUE;

    return;
}

STATIC void stab_pushFunc(u8 *p_path,void* arg, Wilddog_Return_T err)
{
                        
	stab_get_recvErr(err,STABTEST_CMD_PUSH);
	*(BOOL*)arg = TRUE;

    return;
}

STATIC void stab_addObserverFunc
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
    
	stab_get_recvErr(err,STABTEST_CMD_ON);
	
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
		;
    else
    	stab_pushSuccess++;
    	
    *(BOOL*)arg = TRUE;

    return;
}
int stabtest_request(STABTEST_CMD_TYPE type,Wilddog_T client,BOOL *p_finishFlag)
{

	Wilddog_Node_T *p_head = NULL,*p_node = NULL;
	int res = 0;
    /*Create an node which type is an object*/
    p_head = wilddog_node_createObject(NULL);
    
    /*Create an node which type is UTF-8 Sring*/
    p_node = wilddog_node_createUString((Wilddog_Str_T *)STABTEST_KEY,(Wilddog_Str_T *)STABTEST_VALUE);
    
    /*Add p_node to p_head, then p_node is the p_head's child node*/
    wilddog_node_addChild(p_head, p_node);
	
	stab_cmd = type;
    switch(type)
    {
        case STABTEST_CMD_GET:
            /*Send the query method*/
            res = wilddog_getValue(client, (onQueryFunc)stab_getValueFunc, (void*)p_finishFlag);
            break;
        case STABTEST_CMD_SET:  
            /*Send the set method*/
            res = wilddog_setValue(client,p_head,stab_setValueFunc,(void*)p_finishFlag);
            break;
        case STABTEST_CMD_PUSH:
            /*Send the push method*/
            res = wilddog_push(client, p_head, stab_pushFunc, (void *)p_finishFlag);  
            break;
        case STABTEST_CMD_DELE:
            /*Send the remove method*/
            res = wilddog_removeValue(client, stab_removeValueFunc, (void*)p_finishFlag);
            break;
        case STABTEST_CMD_ON:
            /*Observe on*/
            res = wilddog_addObserver(client, WD_ET_VALUECHANGE, stab_addObserverFunc, (void*)p_finishFlag);
            break;
		case STABTEST_CMD_OFF:
			res = wilddog_removeObserver(client, WD_ET_VALUECHANGE);
			break;
		case STABTEST_CMD_NON:
		default:
			break;
    }
    /*Delete the node*/
    wilddog_node_delete(p_head);
    return res;
}
STATIC void stab_trysync(void)
{
	stab_set_runtime();
	
	ramtest_getAveragesize();
	/*Handle the event and callback function, it must be called in a special frequency*/
	wilddog_trySync();

}

int stab_oneCrcuRequest(void) 
{
	int res = 0;
	BOOL otherFinish = FALSE,onFinish = FALSE;
	BOOL *p_finish = &onFinish;
    Wilddog_T client = 0;
    STABTEST_CMD_TYPE cmd = STABTEST_CMD_ON;
    
    

	/* mark star time*/
	stab_set_runtime();
    /*Init a wilddog client*/
    client = wilddog_initWithUrl((Wilddog_Str_T *)TEST_URL);
	stab_get_requestRes(stabtest_request(cmd,client,p_finish));

    while(1)
    {
        if(TRUE == *p_finish)
        {
        	if(STABTEST_ONREQUEST(cmd))
        		p_finish = &otherFinish;

        	onFinish = FALSE;
        	otherFinish = FALSE;
			STABTEST_NEXTREQUEST(cmd);
			stab_get_requestRes(stabtest_request(cmd,client,p_finish));
			
			if(STABTEST_OFFREQUEST(cmd))
			{
				break;
			}	
        }
        stab_trysync();
    }
    /*Destroy the wilddog clent and release the memory*/
    res = wilddog_destroy(&client);

    return res;
}

void stab_titlePrint(void)
{
	printf("\t>----------------------------------------------------<\n");
	printf("\tcount\truntime\tram\tUnlaunchRatio\tLostRatio \n");
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
	//sprintf(successRatio,"%lu/%lu",stab_pushSuccess,stab_changetime);
	
	printf("\t%lu",++run_cnt);		
	printf("\t%lu",stab_runtime);
	printf("\t%lu",(u32)ramtest_get_averageRam());
	printf("\t%s",unlaunchRatio);
	printf("\t\t%s",lossRatio);
	//printf("\t\t%s",successRatio);
	printf("\n");
	return;
}
void stab_test_cycle(void)
{
	ramtest_init(1,1);
	stab_titlePrint();
	while(1)
	{
		stab_oneCrcuRequest();
		stab_resultPrint();
	}
	stab_endPrint();
}

int main(void)
{
	
	stab_test_cycle();
	return 0;
}

#endif /* WILDDOG_SELFTEST*/

