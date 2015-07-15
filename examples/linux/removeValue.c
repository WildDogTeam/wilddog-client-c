
#include <stdio.h>
#include <stdlib.h>

#include "wilddog.h"
#include "demo.h"
STATIC void test_removeValueFunc(void* arg, Wilddog_Return_T err)
{
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("removeValue failed!");
        return ;
    }
    wilddog_debug("removeValue success!");
    *(BOOL*)arg = TRUE;
    return;
}


int main(void)
{
    BOOL isFinished = FALSE;
    Wilddog_T wilddog;
    
    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)TEST_URL);

    wilddog_removeValue(wilddog, test_removeValueFunc, (void*)&isFinished);
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
    
    return 0;
}

