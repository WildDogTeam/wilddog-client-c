#include <stdio.h>
#include <stdlib.h>
 
#include "wilddog.h"
#include "wilddog_debug.h"
#include "demo.h"

STATIC void test_addObserverFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg,
	Wilddog_Return_T err)
{
	
	*(BOOL*)arg = TRUE;
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("addObserver failed!");
		return;
	}

	wilddog_debug_printnode(p_snapshot);
	wilddog_debug("addObserver data!");
	
	return;
}

int main(void)
{
	BOOL isFinished = FALSE;
	Wilddog_T wilddog;
	STATIC int count = 0;	
	
	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)TEST_URL);

	if(0 == wilddog)
	{
		wilddog_debug("new wilddog failed!");
		return 0;
	}
	wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, test_addObserverFunc, (void*)&isFinished);
	while(1)
	{
		if(TRUE == isFinished)
		{
			//wilddog_debug("get new data %d times!", count++);
			isFinished = FALSE;
			if(count > 0)
			{
				//wilddog_debug("off the data!");
				wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE);
				break;
			}
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);
	
	return 0;
}

