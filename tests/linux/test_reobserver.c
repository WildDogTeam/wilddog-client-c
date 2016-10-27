/*
* 测试目的： 断网情况下，observer 接收到-10 再次调用observer 是否正常发送出 observer 申请。
* 测试项： 1、observer 接收到错误是否完全释放 observer。
*          2、释放后继续订阅子节点，之前订阅的父节点也不能收到推送，验证再次订阅是否受前一次订阅的影响。
*
* 测试步骤： 1、断网： sudo ifconfig eth0 down 
*            2、运行测试              
*
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h> 
#include "wilddog.h"
#include "test_config.h"

#define _TEST_FAIL (-1)
#define _TEST_SUCCESSFUL (1)
#define _TEST_SUB_KEY_A  "aa"
#define _TEST_SUB_KEY_B  "bb"
 
Wilddog_T wilddog = 0,wilddog_a,wilddog_b;


static int observer_root=0, observer_cb =0, observer_main = 0;
typedef enum TEST_RSULT{
	RESULT_NO_REOBSERVER,	
	RESULT_MAIN_REOBSERVER,
	RESULT_REOBSERVER,	
	RESULT_MAX
}TEST_RSULT_T;


static int test_result[RESULT_MAX];
static int test_count[RESULT_MAX];

STATIC void observer_cb_root
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
	if(observer_root )
	{
	 	test_result[RESULT_NO_REOBSERVER] = _TEST_FAIL;

	}
	
	test_count[RESULT_NO_REOBSERVER] = 1;
	if((err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED ) &&
        err != WILDDOG_ERR_RECONNECT)
    {
        wilddog_debug("addObserver error code = %d",err);
		observer_root = 1;
        return;
    }

	wilddog_debug("receive notify");
    return;
}


STATIC void observer_cb_a
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
	if(observer_main == 2 )
	{
	 	test_result[RESULT_MAIN_REOBSERVER] = _TEST_SUCCESSFUL;
		test_count[RESULT_MAIN_REOBSERVER] = 1;

	}

	if((err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED ) &&
        err != WILDDOG_ERR_RECONNECT)
    {
        wilddog_debug("addObserver error code = %d",err);
		
		if(observer_main <1 )
			observer_main = 1;
        return;
    }

	wilddog_debug("receive notify");
    return;
}
STATIC void observer_cb_b
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
	if(observer_cb )
	{
	 	test_result[RESULT_REOBSERVER] = _TEST_SUCCESSFUL;
		test_count[RESULT_REOBSERVER] = 1;

	}

	if((err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED ) &&
        err != WILDDOG_ERR_RECONNECT)
    {
        wilddog_debug("addObserver error code = %d",err);
		wilddog_debug("reobserver new");
		wilddog_addObserver(wilddog_b,WD_ET_VALUECHANGE,observer_cb_b,NULL);
		observer_cb = 1;
        return;
    }

	wilddog_debug("receive notify");
    return;
}
STATIC int test_finish(void)
{
	int i;
	for(i=0;i<(RESULT_MAX);i++){
		if(test_count[i] == 0){
			return 0;
		}
		
	}
	return 1;
}
STATIC int test_restult_printf(void)
{
	int i,res=0;
	printf("\n\toffline result :\t");
	for(i=0;i<RESULT_MAX;i++)
		if(test_result[i] == _TEST_FAIL){
			switch(i){
				case RESULT_NO_REOBSERVER:
					printf("\n\t\t\tfailt observer get notify which should remove\n");
					res =-1;
					break;	
				case RESULT_MAIN_REOBSERVER:
					printf("\n\t\t\tmain  reobserver failt\n");					
					res =-1;
					break;
				case RESULT_REOBSERVER:
					printf("\n\t\t\tcall back reobserver failt\n");
					res =-1;
					break;	
			}
				
		}
	if(res == 0)
		printf("reobserver test successfuly\n");
	return res;
}
int main(int argc, char **argv) 
{
    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)TEST_URL);
	wilddog_a = wilddog_getChild(wilddog,(Wilddog_Str_T*)_TEST_SUB_KEY_A);	
	wilddog_b = wilddog_getChild(wilddog,(Wilddog_Str_T*)_TEST_SUB_KEY_B);
	
	wilddog_addObserver(wilddog,WD_ET_VALUECHANGE,observer_cb_root,NULL);
	wilddog_addObserver(wilddog_a,WD_ET_VALUECHANGE,observer_cb_a,NULL);	
	wilddog_addObserver(wilddog_b,WD_ET_VALUECHANGE,observer_cb_b,NULL);

    while(1)
    {
    
		wilddog_increaseTime(3*1000);
        wilddog_trySync();
		if(observer_main == 1){
			observer_main = 2;
			wilddog_addObserver(wilddog_a,WD_ET_VALUECHANGE,observer_cb_a,NULL);
		}
		if(test_finish())
			break;
    }

	wilddog_destroy(&wilddog_a);
	wilddog_destroy(&wilddog_b);
    wilddog_destroy(&wilddog);

	return test_restult_printf();
	
}

