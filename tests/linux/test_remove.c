
#include <stdio.h>
#include <stdlib.h>

#include "wilddog_debug.h"
#include "wilddog.h"
#include "test.h"

STATIC void test_onDeleteFunc(void* arg, Wilddog_Return_T err)
{
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		//wilddog_debug("delete failed!");
		TEST_RESULT_PRINTF("wilddog_removeValue",TESTFUNCNAME_TABLECASE,TEST_ERR,err);
		return;
	}
	else
		TEST_RESULT_PRINTF("wilddog_removeValue",TESTFUNCNAME_TABLECASE,TEST_OK,err);
	//wilddog_debug("delete success!");
	*(BOOL*)arg = TRUE;
	return;
}

int main(void)
{
	BOOL isFinished = FALSE;
	Wilddog_T wilddog;

	
	wilddog = wilddog_initWithUrl(TEST_URL);

	wilddog_removeValue(wilddog, test_onDeleteFunc, (void*)&isFinished);
	while(1)
	{
		if(TRUE == isFinished)
		{
			//wilddog_debug("remove success!");
			break;
		}
		wilddog_trySync();
	}
	wilddog_destroy(&wilddog);
	
}

