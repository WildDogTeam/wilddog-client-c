#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wilddog.h"

#include "wilddog_debug.h"
#include "wilddog_api.h"
#include "wilddog_ct.h"
#include "test_lib.h"
#include "test_config.h"

#define TEST_STEP_RESULT(v)	(( v == 0)?"FAIL":"PASS")

typedef enum
{
	_STEP_FALSE,
	_STEP_TRUE,
}_STEP_BOOT;
typedef struct WILDDOG_HANDLE_T
{
	BOOL isFinished;
	Wilddog_Node_T *p_node;
}Wilddog_Handle_T;
typedef struct WILDDOG_STEP_TESTRESULT_T
{
	u8 res_node_creatSuccess;
	u8 res_node_addSuccess;
	u8 res_node_setKeySuccess;
	u8 res_node_getValueSuccess;
	u8 res_setAuthSuccess;
	u8 res_getValueSuccess;
	u8 res_setValueSuccess;
	u8 res_pushSuccess;
	u8 res_deleteSuccess;
	u8 res_addObserverSuccess;
	u8 res_offObserverSuccess;
	
}Wilddog_step_res_T;

Wilddog_step_res_T l_step;

STATIC void test_step_pushFunc(u8 *p_path,void* arg, Wilddog_Return_T err)
{
	*(BOOL*)arg = TRUE;
	printf("\t push err =%d \n",err);	
	l_step.res_pushSuccess = TRUE;
	
	return;
}


STATIC void test_step_deleteFunc(void* arg, Wilddog_Return_T err)
{
	*(BOOL*)arg = TRUE;
	printf("\t delete err =%d \n",err);	
	l_step.res_deleteSuccess = TRUE;
	
	return;
}

STATIC void test_step_setFunc(void* arg, Wilddog_Return_T err)
{
	
	*(BOOL*)arg = TRUE;
	printf("\t set err =%d \n",err);	
	l_step.res_setValueSuccess = TRUE;
	return;
}

STATIC void test_step_getFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg, 
	Wilddog_Return_T err)
{
	
	*(BOOL*)arg = TRUE;
	printf("\t get err =%d \n",err);	
	l_step.res_getValueSuccess = TRUE;

	return;
}
STATIC void test_step_getObserverFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg, 
	Wilddog_Return_T err)
{
	
	*(BOOL*)arg = TRUE;
	printf("\t res_addObserverSuccess err =%d \n",err);	
	l_step.res_addObserverSuccess = TRUE;

	return;
}

STATIC void test_step_authFunc
	(
	    void* arg,
   	 	Wilddog_Return_T err
    )
{
	*(BOOL*)arg = TRUE;
	printf("\t set auth err =%d \n",err);	
	l_step.res_setAuthSuccess = TRUE;
}

STATIC int test_step_res(void)
{
	if( 
		!l_step.res_node_addSuccess ||
		!l_step.res_setAuthSuccess ||
		!l_step.res_getValueSuccess ||
		!l_step.res_setValueSuccess ||
		!l_step.res_pushSuccess ||
		!l_step.res_deleteSuccess ||	
		!l_step.res_addObserverSuccess||
		!l_step.res_offObserverSuccess
	)
		return -1;
	else
		return 0;
}
STATIC void test_step_printf(void)
{
	printf("**********STEP TEST RESULT********** \n");

	/*
	printf("\t res_node_creatSuccess: \t%s \n", 
		TEST_STEP_RESULT(l_step.res_node_creatSuccess));  
	*/
	printf("\t res_node_addSuccess: \t%s \n", 
		TEST_STEP_RESULT(l_step.res_node_addSuccess));

	printf("\t res_getValueSuccess: \t%s \n", 
		TEST_STEP_RESULT(l_step.res_getValueSuccess));
	printf("\t res_setAuthSuccess: \t%s \n", 
		TEST_STEP_RESULT(l_step.res_setAuthSuccess));
	printf("\t res_setValueSuccess: \t%s \n", 
		TEST_STEP_RESULT(l_step.res_setValueSuccess));
	printf("\t res_pushSuccess: \t%s \n", 
		TEST_STEP_RESULT(l_step.res_pushSuccess));
	printf("\t res_deleteSuccess: \t%s \n", 
		TEST_STEP_RESULT(l_step.res_deleteSuccess));
	printf("\t res_addObserverSuccess: \t%s \n", 
		TEST_STEP_RESULT(l_step.res_addObserverSuccess));
	printf("\t res_offObserverSuccess: \t%s \n", 
		TEST_STEP_RESULT(l_step.res_offObserverSuccess));
	
	printf("**********STEP TEST DONE********** \n");
}
STATIC void test_gethost(char *p_host,const char *url)
{
	char *star_p = NULL,*end_p = NULL;
	star_p =  strchr(url,'/')+2;
	end_p = strchr(star_p,'.');
	memcpy(p_host,star_p,end_p - star_p);	
    memcpy(&p_host[end_p - star_p],TEST_URL_END,strlen(TEST_URL_END));
}

int main(void)
{
	char host[32];
	int res = 0;
	BOOL isFinished = FALSE;
	Wilddog_T wilddog;
	Wilddog_Node_T *p_head = NULL, *p_node = NULL/*, *p_snapshot = NULL*/;
	u8 value1[5] = {246,12,0,0,6};
	/*u8 value2[4] = {23,67,98,1};*/
	wFloat f = 2.3;

	Wilddog_Node_T *root = NULL;
	Wilddog_Node_T *L1c1 = NULL,*L1c2 = NULL;
	Wilddog_Node_T *L2c1 = NULL,*L2c2 = NULL,*L2c3 = NULL;
	Wilddog_Node_T *L3c1 = NULL,*L3c2 = NULL, 
					*L3c3 = NULL,*L3c4 = NULL,*L3c5 = NULL;
	Wilddog_Node_T *L4c1 = NULL,*L4c2 = NULL,
					*L4c3 = NULL,*L4c4 = NULL,*L4c5 = NULL;

        
	memset(host,0,sizeof(host));
    test_gethost(host,TEST_URL);
    printf(" \t host %s \n",host);

 	printf("**********STEP TEST **************** \n");
	printf("\t url = %s\n",TEST_URL);
	
	root = wilddog_node_createNum((Wilddog_Str_T *)"root",9999);
	L1c1 = wilddog_node_createFalse((Wilddog_Str_T *)"L1c1");
	L1c2 = wilddog_node_createTrue((Wilddog_Str_T *)"L1c2");
	L2c1 = wilddog_node_createNum((Wilddog_Str_T *)"L2c1",-10000);
	L2c2 = wilddog_node_createFloat((Wilddog_Str_T *)"L2c2",f);
	L2c3 = wilddog_node_createTrue((Wilddog_Str_T *)"L2c3");          //true
	L3c1 = wilddog_node_createBString((Wilddog_Str_T *)"L3c1",value1,sizeof(value1)/sizeof(u8));    //BString
	L3c2 = wilddog_node_createTrue((Wilddog_Str_T *)"L3c2");
	L3c3 = wilddog_node_createTrue((Wilddog_Str_T *)"L3c3");
	L3c4 = wilddog_node_createNull((Wilddog_Str_T *)"L3c4");   //NULL
	L3c5 = wilddog_node_createTrue((Wilddog_Str_T *)"L3c5");
	L4c1 = wilddog_node_createNum((Wilddog_Str_T *)"L4c1",875);//    +Num
	L4c2 = wilddog_node_createNum((Wilddog_Str_T *)"L4c2",-5693);    //   -Num
	L4c3 = wilddog_node_createFloat((Wilddog_Str_T *)"L4c3",f);     //float
	L4c4 = wilddog_node_createFalse((Wilddog_Str_T *)"L4c4");        //false
	L4c5 = wilddog_node_createUString((Wilddog_Str_T *)"L4c5",(Wilddog_Str_T *)"string");      //UString

	l_step.res_node_addSuccess = _STEP_TRUE;
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(root,L1c1))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(root,L1c2))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L1c1,L2c1))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L1c1,L2c2))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L1c2,L2c3))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c1,L3c1))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c1,L3c2))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c2,L3c3))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c2,L3c4))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c2,L3c5))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L3c2,L4c1))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L3c3,L4c2))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L3c3,L4c3))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L3c5,L4c4))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L3c5,L4c5))
	{
		l_step.res_node_addSuccess = _STEP_FALSE;
	}

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)TEST_URL);
	
	/*auth*/
	isFinished = FALSE;
	wilddog_auth((Wilddog_Str_T*)host, \
		(u8*)TEST_AUTH,strlen(TEST_AUTH), \
		test_step_authFunc,(void*)&isFinished);
	while(1)
	{
		if(TRUE == isFinished)
			break;
		wilddog_trySync();
	}
	isFinished = FALSE;
	wilddog_removeValue(wilddog, test_step_deleteFunc,(void*)&isFinished);
	while(1)
	{
		if(TRUE == isFinished)
			break;
		
		wilddog_trySync();
	}
	
	isFinished = FALSE;
	wilddog_setValue(wilddog,root,test_step_setFunc,(void*)&isFinished);
	wilddog_node_delete(root);
	
	while(1)
	{
		if(TRUE == isFinished)
			break;
		wilddog_trySync();
	}
	
	isFinished = FALSE;
	p_head = wilddog_node_createObject((Wilddog_Str_T *)"3");
	p_node = wilddog_node_createNum((Wilddog_Str_T *)"2",1234);
	wilddog_node_addChild(p_head, p_node);
	
	wilddog_push(wilddog, p_head, test_step_pushFunc, (void *)&isFinished);	
	wilddog_node_delete(p_head);
	
	while(1)
	{
		if(isFinished)
		{
			break;
		}
		wilddog_trySync();
	}
	isFinished = FALSE;
	wilddog_getValue(wilddog, test_step_getFunc, (void*)&isFinished);
	while(1)
	{
		if(isFinished == TRUE)
				break;
		wilddog_trySync();
	}


	isFinished = FALSE;
	wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, test_step_getObserverFunc, \
		(void*)&isFinished);
	
	while(1)
	{
		if(TRUE == isFinished)
			break;
		
		wilddog_trySync();
	}

	res = wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE);
	if(res == WILDDOG_ERR_NOERR)
		l_step.res_offObserverSuccess = TRUE;
	wilddog_destroy(&wilddog);
	
	test_step_printf();

	return test_step_res();
}

