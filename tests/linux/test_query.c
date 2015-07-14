/*
*	get test file
*
*/
#include <stdio.h>
#include <stdlib.h>
 
#include "wilddog_debug.h"
#include "wilddog.h"
#include "test.h"

STATIC void test_onQueryFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg, 
	Wilddog_Return_T err)
{
	
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		
		TEST_RESULT_PRINTF("wilddog_query",TESTFUNCNAME_TABLECASE,TEST_ERR,err);
		//wilddog_debug("query error!");
		return;
	}
//	wilddog_debug("query success!");
	if(p_snapshot)
	{
		*(Wilddog_Node_T**)arg = wilddog_node_clone(p_snapshot);
	}
	
	return;
}

int main(void)
{
	Wilddog_T wilddog = 0;
	Wilddog_Node_T * p_node = NULL;
	
	wilddog_init();
	
	wilddog = wilddog_new(TEST_URL);
	if(0 == wilddog)
	{
		return 0;
	}
	wilddog_query(wilddog, test_onQueryFunc, (void*)(&p_node));
	while(1)
	{
		if(p_node)
		{
#if 0
			wilddog_debug("print node:");
			wilddog_debug_printnode(p_node);
			wilddog_node_delete(p_node);
			printf("\n");
#endif
			wilddog_node_delete(p_node);
			TEST_RESULT_PRINTF("wilddog_query",TESTFUNCNAME_TABLECASE,TEST_OK,0);
			break;
		}
		wilddog_trySync();
	}

	wilddog_destroy(&wilddog);
	
}

