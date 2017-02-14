/*
* 测试项： 1、订阅/a，然后让该订阅超时，在超时回调中重新订阅，能重新订阅（注意：有可能触发error回调）。
*          2、断线重连测试，订阅节点后，断线应重新发送observe。
*
* 测试步骤： 运行测试
*
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h> 
#include "wilddog.h"

#define _TEST_FAIL (-1)
#define _TEST_SUCCESSFUL (1)
#define _TEST_SUB_KEY_A  "aa"
#define _TEST_SUB_KEY_B  "bb"

static int test_result = 1;
static BOOL isExit = FALSE;
STATIC void set_cb
    (
    void* arg,
    Wilddog_Return_T err
    )
{
	if(arg)
		*(BOOL*)arg = TRUE;
	if((err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED ) &&
        err != WILDDOG_ERR_RECONNECT)
    {
        wilddog_debug_level(WD_DEBUG_LOG,"set error code = %d",err);
        return;
    }
    return;
}
STATIC void observer_cb_2
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
	isExit = TRUE;
	if((err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED ) &&
        err != WILDDOG_ERR_RECONNECT)
    {
        wilddog_debug_level(WD_DEBUG_DEBUG,"addObserver error code = %d",err);
        return;
    }
	test_result = 0;
//	wilddog_debug("Second");
    return;
}
STATIC void observer_cb_1
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
	Wilddog_T wilddog = (Wilddog_T)arg;
	if(0 == wilddog){
		wilddog_debug_level(WD_DEBUG_LOG,"Reobserve callback triggered");
		test_result = 0;
		isExit = TRUE;
	}
	if((err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED ) &&
        err != WILDDOG_ERR_RECONNECT)
    {
        wilddog_debug_level(WD_DEBUG_LOG,"addObserver error code = %d",err);
		if(wilddog){
			wilddog_debug_level(WD_DEBUG_LOG,"Reobserve");
			wilddog_addObserver(wilddog,WD_ET_VALUECHANGE,observer_cb_1,NULL);
		}
        return;
    }
    return;
}

int main(int argc, char **argv) 
{
	Wilddog_T wilddog = 0,wilddog_child = 0;
	Wilddog_Str_T * url = NULL;
	Wilddog_Node_T* node = NULL;
#ifndef TEST_URL
	if(argc < 2){
		printf("Input : \t ./test_reobserver url\n");
		exit(0);
	}
	url = (Wilddog_Str_T *)argv[1];
#else
	url = (Wilddog_Str_T *)TEST_URL;
#endif
	node = wilddog_node_createUString(NULL,(Wilddog_Str_T*)"1");
	
	printf("Reobserver test 1/2: addObserver nest test start...\n");
	// 1/2 : addObserver嵌套测试，订阅/a，然后让该订阅超时，在超时回调中重新订阅
	wilddog = wilddog_initWithUrl(url);
	wilddog_child = wilddog_getChild(wilddog,(Wilddog_Str_T*)"a");
	wilddog_setValue(wilddog_child,node,set_cb, (void*)&isExit);
	while(isExit == FALSE){
		wilddog_trySync();
	}
	isExit = FALSE;
	test_result = 1;
	wilddog_addObserver(wilddog_child,WD_ET_VALUECHANGE,observer_cb_1,(void*)wilddog_child);
	wilddog_goOffline();
	wilddog_increaseTime(WILDDOG_RETRANSMITE_TIME);//let it timeout
	printf("Time pass %d seconds...\n",WILDDOG_RETRANSMITE_TIME/1000);
	fflush(stdout);
	sleep(WILDDOG_RETRANSMITE_TIME/1000);
	printf("Wake up.\n");
	wilddog_goOnline();
    while(1){
        wilddog_trySync();
		if(TRUE == isExit)
			break;
    }

    wilddog_destroy(&wilddog);
	wilddog_destroy(&wilddog_child);
	
	if(1 == test_result){
		printf("Reobserver test 1/3: addObserver nest test failed!\n");
		return test_result;
	}
	printf("Reobserver test 1/2: addObserver nest test success!\n");
	// 2/2 : addObserver断线重连测试，订阅
	printf("Reobserver test 2/2: addObserver offline reobserve test start...\n");
	isExit = FALSE;
	test_result = 1;
	wilddog = wilddog_initWithUrl(url);
	wilddog_child = wilddog_getChild(wilddog,(Wilddog_Str_T*)"a");

	wilddog_addObserver(wilddog_child,WD_ET_VALUECHANGE,observer_cb_2,(void*)&isExit);
	while(isExit == FALSE){
		wilddog_trySync();
	}
	isExit = FALSE;
	printf("Time pass 180 seconds quickly(send offline to server and wait %d seconds)...\n",WILDDOG_RETRANSMITE_TIME/1000);
	fflush(stdout);
	wilddog_goOffline();
	sleep(WILDDOG_RETRANSMITE_TIME/1000);
	wilddog_goOnline();
	printf("Wake up.\n");
	wilddog_setValue(wilddog_child,node,set_cb, NULL);
	while(isExit == FALSE){
		wilddog_trySync();
	}
    wilddog_destroy(&wilddog);
	wilddog_destroy(&wilddog_child);
	if(1 == test_result){
		printf("Reobserver test 2/2: addObserver offline reobserve test failed!\n");
		return test_result;
	}
	printf("Reobserver test 2/2: addObserver offline reobserve test success!\n");

	wilddog_node_delete(node);
	return test_result;
}

