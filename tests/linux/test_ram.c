#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include<sys/time.h>

#include "wilddog.h"
#include "test_lib.h"
#include "test_config.h"


static const char *p_ramtree_url[] = 
					{
						TEST_TREE_T_127,
						TEST_TREE_T_256,
						TEST_TREE_T_576,
						TEST_TREE_T_810,
						TEST_TREE_T_1044,
						TEST_TREE_T_1280,
					};

static  u32 d_ramtree_num[] = {127,256,576,810,1044,1280};
/**
** building test tree
**/
#define TEST_BUILDTREE_ERROR	(-1)


BOOL isFinished = FALSE;
Wilddog_T wilddog = 0;
STATIC int l_res = 0;

STATIC void test_onSetFunc(void* arg, Wilddog_Return_T err)
{

	if(arg)
		printf("Building tree : %s  error = %d",(u8*)arg,err);
	else 
		printf("Building tree 	error = %d",err);

	
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
		l_res = TEST_BUILDTREE_ERROR;
		
	isFinished = TRUE;
	return;
}
/*************************************build complete binary tree**************************************************************************/
STATIC int test_buildtreeFunc(void)
{

	int m = 0;
	u8 url[strlen(TEST_URL)+20];
	Wilddog_Node_T *p_head = NULL;
	//Wilddog_Node_T *p1 = NULL, *p2 = NULL, *p3 = NULL;
	
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
	wilddog = wilddog_initWithUrl(url);
	if(0 == wilddog)
	{
		wilddog_debug("new wilddog error");
		return -1;
	}

	
	wilddog_setValue(wilddog,L5[0],test_onSetFunc,(void*)TEST_TREE_T_127);
	
	//wilddog_node_delete(p_head);
	while(1)
	{
		if(TRUE == isFinished)
		{
			printf("\t set success! \n");
			break;
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);

/*4*/
	if(TEST_TREE_ITEMS < 2)
		return 0;

	isFinished = FALSE;
	memset((void*)url,0,sizeof(url));
	sprintf((char*)url,"%s%s",TEST_URL,TEST_TREE_T_256);
	wilddog = wilddog_initWithUrl(url);
	
	if(0 == wilddog)
	{
		wilddog_debug("new wilddog error");
		return -1;
	}

	
	wilddog_setValue(wilddog,L4[0],test_onSetFunc,(void*)TEST_TREE_T_256);
	//wilddog_node_delete(p_head);
	
	while(1)
	{
		if(TRUE == isFinished)
		{
			printf("\t set success! \n");
			break;
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);

/*5*/
	if(TEST_TREE_ITEMS < 3)
		return 0;

	isFinished = FALSE;
	memset((void*)url,0,sizeof(url));
	sprintf((char*)url,"%s%s",TEST_URL,TEST_TREE_T_576);
	wilddog = wilddog_initWithUrl(url);

	
	if(0 == wilddog)
	{
		wilddog_debug("new wilddog error");
		return -1;
	}

	
	wilddog_setValue(wilddog,L3[0],test_onSetFunc,(void*)TEST_TREE_T_576);
	
	//wilddog_node_delete(p_head);
	while(1)
	{
		if(TRUE == isFinished)
		{
			printf("\t set success! \n");
			break;
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);
/*6*/
	if(TEST_TREE_ITEMS < 4)
		return 0;
	isFinished = FALSE;
	memset((void*)url,0,sizeof(url));
	sprintf((char*)url,"%s%s",TEST_URL,TEST_TREE_T_810);
	wilddog = wilddog_initWithUrl(url);

	if(0 == wilddog)
	{
		wilddog_debug("new wilddog error");
		return -1;
	}

	
	wilddog_setValue(wilddog,L2[0],test_onSetFunc,(void*)TEST_TREE_T_810);
	
	//wilddog_node_delete(p_head);
	while(1)
	{
		if(TRUE == isFinished)
		{
			printf("\t set success! \n");
			break;
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);
/*7*/
	if(TEST_TREE_ITEMS < 5)
		return 0;
	isFinished = FALSE;
	memset((void*)url,0,sizeof(url));
	sprintf((char*)url,"%s%s",TEST_URL,TEST_TREE_T_1044);
	wilddog = wilddog_initWithUrl(url);

	
	if(0 == wilddog)
	{
		wilddog_debug("new wilddog error");
		return -1;
	}

	
	wilddog_setValue(wilddog,L1[0],test_onSetFunc,(void*)TEST_TREE_T_1044);
	
	//wilddog_node_delete(p_head);
	while(1)
	{
		if(TRUE == isFinished)
		{
			printf("\t set success! \n");
			break;
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);

/*8*/
	if(TEST_TREE_ITEMS < 6)
		return 0;

	isFinished = FALSE;
	memset((void*)url,0,sizeof(url));
	sprintf((char*)url,"%s%s",TEST_URL,TEST_TREE_T_1280);
	wilddog = wilddog_initWithUrl(url);

	
	if(0 == wilddog)
	{
		wilddog_debug("new wilddog error");
		return -1;
	}

	
	wilddog_setValue(wilddog,p_head,test_onSetFunc,(void*)TEST_TREE_T_1280);
	
	//wilddog_node_delete(p_head);
	while(1)
	{
		if(TRUE == isFinished)
		{
			printf("\t set success! \n");
			break;
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);
	if(l_res < 0)
		printf("build tree error !!\n");
	else
		printf("\n\ttest_buildtree finished\n");
	return l_res;

}

/**
**building test tree end 
**/
int test_ram(void)
{

#ifdef WILDDOG_SELFTEST
		int res = 0;
		u8 tree_m=0, n=0;
		u8 request_num[4] = {1,16,32,64};
        u8 url[strlen(TEST_URL)+20];
#if 0
		if( (d_ramtree_num[TEST_TREE_ITEMS] + TEST_PROTO_COVER) > WILDDOG_PROTO_MAXSIZE )
		{
			printf("please modify WILDDOG_PROTO_MAXSIZE to %lu ,in wilddog_config.h \n",\
				(d_ramtree_num[TEST_TREE_ITEMS] + TEST_PROTO_COVER));
			return -1;
		}
 #endif       
		if( (res = test_buildtreeFunc() ) < 0 )
				return res;
		ramtest_titile_printf();
	
		for( tree_m=0; tree_m < TEST_TREE_ITEMS; tree_m++)
		{
			for( n=0; n <4; n++)
			{
				memset(url,0,sizeof(url));
				sprintf((char*)url,"%s%s",TEST_URL,p_ramtree_url[tree_m]);
				ramtest_handle(url,d_ramtree_num[tree_m],request_num[n]);
			}
		}
		ramtest_end_printf();
#endif
		return 0;

}
int main(void)
{
#ifdef WILDDOG_SELFTEST
	return test_ram();
	
#endif
}
