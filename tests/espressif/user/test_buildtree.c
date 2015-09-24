#ifndef WILDDOG_PORT_TYPE_ESP
#include <stdio.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "wilddog.h"
#include "user_config.h"

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
    
#include "espconn.h"



#ifdef WILDDOG_SELFTEST
extern os_timer_t test_timer1;
extern os_timer_t test_timer2;
Wilddog_T wilddog = 0;


#define TEST_BUILDTREE_ERROR	(-1)
#define TEST_TREE_ITEMS (4)

char *p_performtree_url[TEST_TREE_ITEMS] = 
					{
						TEST_TREE_T_127,
						TEST_TREE_T_256,
						TEST_TREE_T_576,
						TEST_TREE_T_810,
					};

u32 d_performtree_num[TEST_TREE_ITEMS] = {127,256,576,810};

u8 *p_ramtree_url[TEST_TREE_ITEMS] = 
					{
						TEST_TREE_T_127,
						TEST_TREE_T_256,
						TEST_TREE_T_576,
						TEST_TREE_T_810,
					};

u32 d_ramtree_num[TEST_TREE_ITEMS] = {127,256,576,810};


Wilddog_T tree_wilddog[4] = {0};


BOOL isFinished = FALSE;
int l_res = 0;


void WD_SYSTEM  sync(void)
{
	if(l_res == 4)
    {   
        os_timer_disarm(&test_timer2);

        wilddog_destroy(&tree_wilddog[0]);
        wilddog_destroy(&tree_wilddog[1]);
        wilddog_destroy(&tree_wilddog[2]);
        wilddog_destroy(&tree_wilddog[3]);
    #if TEST_TYPE == TEST_RAM
        u8 url[sizeof(TEST_URL)];
        u8 tree_m=0, n=0;
        os_timer_disarm(&test_timer2);
        ramtest_titile_printf();
			
        memset(url,0,sizeof(url));
        sprintf(url,"%s%s",TEST_URL,p_ramtree_url[TREE_SN]);
		ramtest_handle(url,d_ramtree_num[TREE_SN],REQ_NUMBER);
		ramtest_end_printf();
    #endif

    #if TEST_TYPE == TEST_TIME
        u8 url[sizeof(TEST_URL)+20];
        performtest_titile_printf();
        memset(url,0,sizeof(url));
        sprintf(url,"%s%s",TEST_URL,p_performtree_url[TREE_SN]);
        performtest_handle(DELAY_TIME_MS,url, \
            d_performtree_num[TREE_SN],REQ_NUMBER);
        performtest_end_printf();
    
    #endif
    }
	else
    {   
        wilddog_trySync();  

        os_timer_setfn(&test_timer2, (os_timer_func_t *)sync, NULL);
        
    	os_timer_arm(&test_timer2, 1000, 0);    	
    }

}



STATIC void WD_SYSTEM  test_setValueFunc(void* arg, Wilddog_Return_T err)
{

    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("setValue error! error:%d\n", err);
        return;
    }
    wilddog_debug("setValue success!");
    *(BOOL*)arg = TRUE;
    l_res++;
    return;
}


/*************************************build complete binary tree**************************************************************************/
int WD_SYSTEM  test_buildtreeFunc(const char *p_userUrl)
{
	int m = 0;
	u8 url[strlen(TEST_URL)+20];
	Wilddog_Node_T *p_head = NULL;
	
	Wilddog_Node_T *L1[2];
	Wilddog_Node_T *L2[4];
	Wilddog_Node_T *L3[8];
	Wilddog_Node_T *L4[16];
	Wilddog_Node_T *L5[32];
	Wilddog_Node_T *L6[64];
	Wilddog_Node_T *L7[128];

	Wilddog_Str_T *key = NULL;
	key = (Wilddog_Str_T*)wmalloc(20);


	p_head = wilddog_node_createObject(NULL);
	for(m = 0; m < 2; m++)
	{
		snprintf((char*)key, 20 , "%s%d", "L1",m);
		L1[m] = wilddog_node_createUString( key,(u8*)"L1L");
	}

	for(m = 0; m < 4; m++)
	{
		snprintf((char*)key, 20 , "%s%d", "L2",m);
		L2[m] = wilddog_node_createUString( key,(u8*)"L2L");
	}

	for(m = 0; m < 8; m++)
	{
		snprintf((char*)key, 20 , "%s%d", "L3",m);
		L3[m] = wilddog_node_createUString( key,(u8*)"L3L");
	}
	
	for(m = 0; m < 16; m++)
	{
		snprintf((char*)key, 20 , "%s%d", "L4",m%10);
		L4[m] = wilddog_node_createUString( key,(u8*)"L4L");
	}
	
	for(m = 0; m < 32; m++)
	{
		snprintf((char*)key, 20 , "%s%d", "L5",m%10);
		L5[m] = wilddog_node_createUString( key,(u8*)"L5L");
	}

	for(m = 0; m < 64; m++)
	{
		snprintf((char*)key, 20 , "%s%d", "L6",m%10);
		L6[m] = wilddog_node_createUString( key,(u8*)"L6L");
	}

	for(m = 0; m < 128; m++)
	{
		snprintf((char*)key, 20 , "%s%d", "L7",m%10);
		L7[m] = wilddog_node_createUString( key,(u8*)"L7L");
	}

	for(m = 0; m < 2; m++)
	{
		wilddog_node_addChild(p_head, L1[m]);
	}

	for(m=0; m < 4; m++)
	{
		wilddog_node_addChild(L1[m/2], L2[m]);
	}


	for(m=0; m < 8; m++)
	{
		wilddog_node_addChild(L2[m/2], L3[m]);
	}

	for(m=0; m < 16; m++)
	{
		wilddog_node_addChild(L3[m/2], L4[m]);
	}

	for(m=0; m < 32; m++)
	{
		wilddog_node_addChild(L4[m/2], L5[m]);
	}

	for(m=0; m < 64; m++)
	{
		wilddog_node_addChild(L5[m/2], L6[m]);
	}

	for(m=0; m < 128; m++)
	{
		wilddog_node_addChild(L6[m/2], L7[m]);
	}
    
/*3*/
	isFinished = FALSE;
	memset((void*)url,0,sizeof(url));
	sprintf((char*)url,"%s%s",TEST_URL,TEST_TREE_T_127);

    tree_wilddog[0] = 0;
	tree_wilddog[0] = wilddog_initWithUrl(url);
	if(0 == tree_wilddog[0])
	{
		wilddog_debug("new wilddog error");
		return -1;
	}

	BOOL *isFinish;
	isFinish = wmalloc(sizeof(BOOL));
	*isFinish= FALSE;
	
	wilddog_setValue(tree_wilddog[0],L5[0],\
        test_setValueFunc,(void*)isFinish);
	


    
    
/*4*/
	if(TEST_TREE_ITEMS < 2)
		return 0;

	isFinished = FALSE;
    //*isFinish = FALSE;
	memset((void*)url,0,sizeof(url));
	sprintf((char*)url,"%s%s",TEST_URL,TEST_TREE_T_256);
    tree_wilddog[1] = 0;
	tree_wilddog[1] = wilddog_initWithUrl(url);
	
	if(0 == tree_wilddog[1])
	{
		wilddog_debug("new wilddog error");
		return -1;
	}
    
	BOOL *isFinish2;
	isFinish2 = wmalloc(sizeof(BOOL));
	*isFinish2= FALSE;
	

	wilddog_setValue(tree_wilddog[1],L4[0],\
        test_setValueFunc,(void*)isFinish2);


/*5*/
	if(TEST_TREE_ITEMS < 3)
		return 0;

	isFinished = FALSE;
    *isFinish = FALSE;
	memset((void*)url,0,sizeof(url));
	sprintf((char*)url,"%s%s",TEST_URL,TEST_TREE_T_576);
    tree_wilddog[2] = 0;
	tree_wilddog[2] = wilddog_initWithUrl(url);

	
	if(0 == tree_wilddog[2])
	{
		wilddog_debug("new wilddog error");
		return -1;
	}

	
	wilddog_setValue(tree_wilddog[2],L3[0],\
        test_setValueFunc,(void*)isFinish);
	
	
    
/*6*/
	if(TEST_TREE_ITEMS < 4)
		return 0;
	isFinished = FALSE;
    *isFinish = FALSE;
	memset((void*)url,0,sizeof(url));
	sprintf((char*)url,"%s%s",TEST_URL,TEST_TREE_T_810);
    tree_wilddog[3] = 0;
	tree_wilddog[3] = wilddog_initWithUrl(url);

	if(0 == tree_wilddog[3])
	{
		wilddog_debug("new wilddog error");
		return -1;
	}

	
	wilddog_setValue(tree_wilddog[3],L2[0],\
        test_setValueFunc,(void*)isFinish);
	
	os_timer_disarm(&test_timer2);
    os_timer_setfn(&test_timer2, (os_timer_func_t *)sync, NULL);
    
	os_timer_arm(&test_timer2, 1000, 0);    		
	
}

#endif	/*# ifdef WILDDOG_SELFTEST */

