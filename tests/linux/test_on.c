#include <stdio.h>
#include <stdlib.h>
 
#include "wilddog_debug.h"
#include "wilddog.h"
#include "test.h"

STATIC void test_onObserveFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg,
	Wilddog_Return_T err)
{
	
	*(BOOL*)arg = TRUE;
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		//wilddog_debug("observe failed!");
		TEST_RESULT_PRINTF("wilddog_on",TESTFUNCNAME_TABLECASE,TEST_ERR,err);
		return;
	}
	else
		TEST_RESULT_PRINTF("wilddog_push",TESTFUNCNAME_TABLECASE,TEST_OK,err);
	wilddog_debug_printnode(p_snapshot);
	//wilddog_debug("observe data!");
	
	return;
}

int main(void)
{
	BOOL isFinished = FALSE;
	Wilddog_T wilddog;
	STATIC int count = 0;	
	wilddog_init();

	wilddog = wilddog_new(TEST_URL);
	if(0 == wilddog)
	{
		wilddog_debug("new wilddog failed!");
		return 0;
	}
	wilddog_on(wilddog, WD_ET_VALUECHANGE, test_onObserveFunc, (void*)&isFinished);
	while(1)
	{
		if(TRUE == isFinished)
		{
			//wilddog_debug("get new data %d times!", count++);
			isFinished = FALSE;
			if(count > 0)
			{
				//wilddog_debug("off the data!");
				wilddog_off(wilddog, WD_ET_VALUECHANGE);
				break;
			}
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);
	
}

