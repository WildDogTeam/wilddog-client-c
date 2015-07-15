/*
*	get test file
*
*/
#include <stdio.h>
#include <stdlib.h>
 
#include "wilddog.h"
#include "wilddog_debug.h"
#include "demo.h"

STATIC void test_getValueFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg, 
	Wilddog_Return_T err)
{
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("getValue fail!");
		return;
	}

	if(p_snapshot)
	{
		wilddog_debug_printnode(p_snapshot);
		*(Wilddog_Node_T**)arg = wilddog_node_clone(p_snapshot);
		printf("\ngetValue success!\n");
	}
	return;


}

int main(void)
{
	Wilddog_T wilddog = 0;
	Wilddog_Node_T * p_node = NULL;
	
	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)TEST_URL);

	if(0 == wilddog)
	{
		return 0;
	}
	wilddog_getValue(wilddog, test_getValueFunc, (void*)(&p_node));
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
			break;
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);
	
	return 0;
}

