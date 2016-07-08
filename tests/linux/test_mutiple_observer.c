/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: test_mutiple_observer.c
 *
 * 测试目的 :  用于挂测 同一个 hosts  订阅一个节点时 推送是否成功，也长时间挂机推送是否仍然有效。
 * 测试步骤 : 	1. cmd:./test_mutiple_observer host/path1 host/path2
 *			 	2. 在野狗网页端修改 host/path1/value1 和 host/path2/value2 	 留意是否推送成功。
 *            	3. 间隔1天后重复2的操作.
 *
 *************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wilddog.h"


STATIC void addObserver_callback_a
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
	
    *(BOOL*)arg = TRUE;
     if((err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED ) &&
        err != WILDDOG_ERR_RECONNECT)
    {
        wilddog_debug(" A addObserver failed! error code = %d",err);
        return;
    }
	wilddog_debug(" get A Observe !");
    wilddog_debug_printnode(p_snapshot);
    printf("\n");

    return;
}

STATIC void addObserver_callback_b
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
    *(BOOL*)arg = TRUE;
     if((err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED ) &&
        err != WILDDOG_ERR_RECONNECT)
    {
        wilddog_debug(" B addObserver_callback2 failed! error code = %d",err);
        return;
    }
	 wilddog_debug(" get B Observe !");
    wilddog_debug_printnode(p_snapshot);
    printf("\n");

    return;
}
int main(int argc, char **argv) 
{
    BOOL aisFinish = FALSE,bisFinish = FALSE;
    Wilddog_T wilddog_a = 0;
	Wilddog_T wilddog_b = 0;

	if( argc != 3 )
	{
		printf("input : \t ./test_mutiple_observer url1/path1 url2/path2 \n");
		return -1;
	}
	wilddog_a = wilddog_initWithUrl((Wilddog_Str_T*)argv[1]);
	wilddog_b = wilddog_initWithUrl((Wilddog_Str_T*)argv[2]);
	wilddog_addObserver(wilddog_a, WD_ET_VALUECHANGE,\
                                      addObserver_callback_a, \
                                      (void*)&aisFinish);	
	
    wilddog_addObserver(wilddog_b, WD_ET_VALUECHANGE,\
                                      addObserver_callback_b, \
                                      (void*)&bisFinish);
    while(1)
    {

        wilddog_trySync();
		
    }

    wilddog_destroy(&wilddog_a);
    wilddog_destroy(&wilddog_b);
	

    return 0;
}

