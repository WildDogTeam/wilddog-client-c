/*
*   get test file
*
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
 
#include "wilddog.h"
#include "wilddog_debug.h"
#include "test_lib.h"
#include "test_config.h"


#define TEST_MTS_SETKEY	"MTS_SET_KEY"
#define TEST_MTS_SETVALUE	"MTS_SET_VALUES"
#define TEST_SELFREDUCE(n) (n = (n>0)?(n-1):0)
#define TEST_MTS_RESULT(v)	(( v == 0)?"FAIL":"PASS")

typedef enum{
	TEST_FALT =0x00,
	TEST_TRUE
}TEST_BOOL;
typedef struct TEST_CLIENT_T
{
	Wilddog_T wd_client;

	u8 getValue_success;
	u8 setValue_success;
	u8 delete_success;
	u8 addObserver_success;
	u8 offObserver_success;
	u8 push_success;

	u8 request_cnt;
	u8 offrequest_flag;
	u8 destory_flag;
}Test_Client_T;

STATIC int testThread_getRequestState(Test_Client_T *p_client,int res)
{
	if(res >= 0)
		p_client->request_cnt++;
	return res;
}
STATIC void testThread_addObserverFunc(
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err)
{
	
	Test_Client_T *p_client = (Test_Client_T*)arg;

	p_client->addObserver_success = TEST_TRUE;
	TEST_SELFREDUCE(p_client->request_cnt);
  	printf("\t addObserver  err = %d \n",err);
	
    return;
}

STATIC void testThread_getValueFunc(
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err)
{
	
	Test_Client_T *p_client = (Test_Client_T*)arg;

	p_client->getValue_success = TEST_TRUE;
	TEST_SELFREDUCE(p_client->request_cnt);
  	printf("\t getValue err = %d \n",err);
	
    return;
}
STATIC void testThread_setValueFunc(
    void* arg, 
    Wilddog_Return_T err)
{
	
	Test_Client_T *p_client = (Test_Client_T*)arg;

	p_client->setValue_success = TEST_TRUE;
	TEST_SELFREDUCE(p_client->request_cnt);
  	printf("\t setValue err = %d \n",err);
	
    return;
}
STATIC void testThread_removeFunc(
    void* arg, 
    Wilddog_Return_T err)
{
	
	Test_Client_T *p_client = (Test_Client_T*)arg;

	p_client->delete_success = TEST_TRUE;
	TEST_SELFREDUCE(p_client->request_cnt);
  	printf("\t delete err = %d \n",err);
	
    return;
}


STATIC void testThread_pushFunc(u8 *p_path,void* arg, Wilddog_Return_T err)
{
	Test_Client_T *p_client = (Test_Client_T*)arg;
    p_client->push_success = TEST_TRUE;
	TEST_SELFREDUCE(p_client->request_cnt);
  	printf("\t push err = %d \n",err);                    


    return;
}

STATIC void *thread_trysync(void *args)
{
	
	Test_Client_T *p_client = (Test_Client_T*)args;
	while(1)
	{
		
		wilddog_trySync();
		if( p_client->request_cnt == 0)
		{
			p_client->destory_flag = TEST_TRUE;
			break;
		}
	}
	return NULL;
}

STATIC void *thread_main(void *args)
{
	Test_Client_T *p_client = (Test_Client_T*)args;
	Wilddog_Node_T *p_head = NULL,*p_node = NULL;
	/*Create an node which type is an object*/
	p_head = wilddog_node_createObject(NULL);
	
	/*Create an node which type is UTF-8 Sring*/
	p_node = wilddog_node_createUString((Wilddog_Str_T *)TEST_MTS_SETKEY,\
											(Wilddog_Str_T *)TEST_MTS_SETVALUE);
	
	/*Add p_node to p_head, then p_node is the p_head's child node*/
	wilddog_node_addChild(p_head, p_node);

	/*observer*/
	if(
		testThread_getRequestState
		(
		p_client,
		wilddog_addObserver( p_client->wd_client,WD_ET_VALUECHANGE, \
				testThread_addObserverFunc,(void*)p_client)
		) >=0
		)
			p_client->offrequest_flag = TRUE;
	/* get value*/
	testThread_getRequestState	\
		( 
		p_client,
		wilddog_getValue( p_client->wd_client, \
				testThread_getValueFunc,(void*)p_client)
		);

	/*set value */
	testThread_getRequestState	\
		( 
		p_client,
		wilddog_setValue( p_client->wd_client, \
				p_head,
				testThread_setValueFunc,(void*)p_client)
		);

	/* push*/
	testThread_getRequestState	\
		( 
		p_client,
		wilddog_push( p_client->wd_client,p_head, \
				testThread_pushFunc,(void*)p_client)
		);
	/*delete */
	testThread_getRequestState	\
		( 
		p_client,
		wilddog_removeValue( p_client->wd_client, \
				testThread_removeFunc,(void*)p_client)
				);
#if 1

	/* offObserver */
	if(p_client->offrequest_flag)
	{
		int res ;
		res = wilddog_removeObserver( p_client->wd_client,WD_ET_VALUECHANGE);
		if(p_client->addObserver_success == TEST_FALT)
		{
			TEST_SELFREDUCE(p_client->request_cnt);
			p_client->addObserver_success = TEST_TRUE;
		}
		if(res == WILDDOG_ERR_NOERR )
			p_client->offObserver_success = TEST_TRUE;
	} 
#endif
	/* delete node */
	wilddog_node_delete(p_head);

	return NULL;
}
STATIC int test_mts_res(Test_Client_T *p_client)
{
	if( !p_client->getValue_success ||
		!p_client->setValue_success ||
		!p_client->delete_success||
		!p_client->addObserver_success||
		!p_client->offObserver_success||
		!p_client->push_success
	)
		return -1;
	else
		return 0;
}
void test_mts_resPrintf(Test_Client_T *p_client)
{
	printf("**********MTS TEST RESULT********** \n");
	printf("\t Observer: \t%s \n",TEST_MTS_RESULT(p_client->addObserver_success));
	printf("\t getValue: \t%s \n",TEST_MTS_RESULT(p_client->getValue_success));
	printf("\t setValue: \t%s \n" ,TEST_MTS_RESULT(p_client->setValue_success));
	printf("\t push: \t\t%s \n",TEST_MTS_RESULT(p_client->push_success));
	printf("\t delete: \t%s \n",TEST_MTS_RESULT(p_client->delete_success));
 	printf("\t offObserver: \t%s \n",TEST_MTS_RESULT(p_client->offObserver_success));
	printf("**********MTS TEST DONE********** \n");
}
int main(int argc, char **argv)
{
    
    Test_Client_T client;
   	pthread_t trysync_pid, main_pid;
	
	memset(&client,0,sizeof(client));
    

	printf("**********MTS TEST **************** \n");
    
    client.wd_client= wilddog_initWithUrl((Wilddog_Str_T *)TEST_URL);

    if(0 ==  client.wd_client)
    {
        return -1;
    }

	if( pthread_create(&trysync_pid, NULL, thread_trysync,(void*)&client ))  
    {  
        return -1;  
    }  
  
    if( pthread_create(&main_pid, NULL, thread_main,(void*)&client) )  
    {  
        return -1;  
    } 
	while(1)
	{
		sleep(1);
		if(client.destory_flag == TEST_TRUE)
			break;
	}
	test_mts_resPrintf(&client);
    wilddog_destroy(&client.wd_client);
	return test_mts_res(&client);
}

