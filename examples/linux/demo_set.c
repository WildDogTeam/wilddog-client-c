
#include <stdio.h>
#include <stdlib.h>

#include "wilddog_debug.h"
#include "wilddog.h"
#include "demo.h"


STATIC void test_onSetFunc(void* arg, Wilddog_Return_T err)
{
						
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		
		TEST_RESULT_PRINTF("wilddog_set",TESTFUNCNAME_TABLECASE,TEST_ERR,err);
		//wilddog_debug("set error!");
		return;
	}
	else 
		TEST_RESULT_PRINTF("wilddog_set",TESTFUNCNAME_TABLECASE,TEST_OK,err);
	*(BOOL*)arg = TRUE;
	return;
}

int main(void)
{
	BOOL isFinish = FALSE;
	Wilddog_T wilddog = 0;
	Wilddog_Node_T * p_node = NULL, *p_head = NULL;

	wilddog_init();

	p_head = wilddog_node_createObject(NULL);

	/* create a new child to "wilddog" , key is "1", value is "123456" */
	p_node = wilddog_node_createUString((Wilddog_Str_T *)"1",(Wilddog_Str_T *)"123456");

	wilddog_node_add(p_head, p_node);
	
	wilddog = wilddog_new((Wilddog_Str_T *)TEST_URL);
	if(0 == wilddog)
	{
		wilddog_debug("new wilddog error");
		return 0;
	}
	/* expect test1234.wilddogio.com/ has a new node "1" */
	wilddog_set(wilddog,p_head,test_onSetFunc,(void*)&isFinish);
	wilddog_node_delete(p_head);
	while(1)
	{
		if(TRUE == isFinish)
		{
			//wilddog_debug("set success!");
			break;
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);
	
	return 0;
}

