#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
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

int main(int argc, char **argv)
{
    int opt;
    char url[1024];
    BOOL isFinished = FALSE;
    Wilddog_T wilddog;
    STATIC int count = 0;   
    
    memset( url, 0, sizeof(url));


    while ((opt = getopt(argc, argv, "hl:")) != -1) 
    {
        switch (opt) 
        {
		case 'h':
			fprintf(stderr, "Usage: %s  -l url\n",
		           argv[0]);
			return 0;
		case 'l':
			strcpy(url, (const char*)optarg);
			break;			
		default: /* '?' */
			fprintf(stderr, "Usage: %s  -l url\n",
		           argv[0]);
			return 0;
        }
    }

    if( argc <3 )
    {
        printf("Usage: %s  -l url\n", argv[0]);
        return 0;
    }


    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)url);

    if(0 == wilddog)
    {
        wilddog_debug("new wilddog failed!");
        return 0;
    }
	
    wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, test_addObserverFunc, (void*)&isFinished);
	isFinished = FALSE;
	
    while(1)
    {
        if(TRUE == isFinished)
        {
            wilddog_debug("get new data %d times!", count++);
			isFinished = FALSE;
            if(count > 12)
            {
                wilddog_debug("off the data!");
                wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE);
                break;
            }
        }
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
    
    return 0;
}

