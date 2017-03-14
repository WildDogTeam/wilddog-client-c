#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h> 
#include "wilddog.h"
#include <signal.h>

BOOL isExit = FALSE;

static void catch_function(int signal) {
    puts("Interactive attention signal caught.");
	isExit = TRUE;
}

STATIC void addObserver_callback
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
     if((err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED ) &&
        err != WILDDOG_ERR_RECONNECT)
    {
        wilddog_debug("addObserver failed! error code = %d",err);
        return;
    }
    wilddog_debug("Observe new data!");
    wilddog_debug_printnode(p_snapshot);
    printf("\n");
    return;
}

int main(int argc, char **argv) 
{
    Wilddog_T wilddog = 0;
    Wilddog_Str_T* url = NULL;
#ifndef TEST_URL
	if(argc < 2){
		printf("Input : \t ./test_reobserver url\n");
		exit(0);
	}
	url = (Wilddog_Str_T *)argv[1];
#else
	url = (Wilddog_Str_T *)TEST_URL;
#endif
	signal(SIGINT, catch_function);
	wilddog = wilddog_initWithUrl((Wilddog_Str_T*)url);
	wilddog_addObserver(wilddog,WD_ET_VALUECHANGE,addObserver_callback,(void*)wilddog);

    while(1)
    {
        /*
         * Handle the event and callback function, it must be called in a 
         * special frequency
        */
        wilddog_trySync();
		if(isExit)
			break;
    }
    /*Destroy the wilddog clent and release the memory*/
    wilddog_destroy(&wilddog);

    return 0;
}

