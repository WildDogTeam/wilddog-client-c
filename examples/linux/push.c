
#include <stdio.h>
#include <stdlib.h>

#include "wilddog.h"
#include "demo.h"

STATIC void test_pushFunc(u8 *p_path,void* arg, Wilddog_Return_T err)
{
						
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("push failed");
		return;
	}		
	wilddog_debug("new path is %s", p_path);
	*(BOOL*)arg = TRUE;
	return;
}



int main(void)
{
	BOOL isFinish = FALSE;
	Wilddog_T wilddog;
	Wilddog_Node_T * p_node = NULL, *p_head = NULL;
	

	p_head = wilddog_node_createObject(NULL);

	p_node = wilddog_node_createNum((Wilddog_Str_T *)"2",1234);
	wilddog_node_addChild(p_head, p_node);
	
	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)TEST_URL);

	wilddog_push(wilddog, p_head, test_pushFunc, (void *)&isFinish);	
	wilddog_node_delete(p_head);
	
	while(1)
	{
		if(isFinish)
		{
			//wilddog_debug("push success!");
			break;
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);

	return 0;
}

