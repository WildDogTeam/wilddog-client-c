#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "wilddog.h"
#include "wilddog_debug.h"
#include "demo.h"


#define TEST_URL_HEAD		"coap://"
#define TEST_URL_END	".wilddogio.com"		







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
    char uid[256];
    char url[1024];
    BOOL isFinished = FALSE;
    Wilddog_T wilddog;
    STATIC int count = 0;   
    
    memset( uid, 0, sizeof(uid));
    memset( url, 0, sizeof(url));



    while ((opt = getopt(argc, argv, "hl:")) != -1) 
    {
        switch (opt) 
        {
		case 'h':
			fprintf(stderr, "Usage: %s  -l appid\n",
		           argv[0]);
			return 0;
		case 'l':
			strcpy(uid, (const char*)optarg);
			//printf("uid:%s\n",optarg);
			break;			
		default: /* '?' */
			fprintf(stderr, "Usage: %s  -l appid\n",
		           argv[0]);
			return 0;
        }
    }

    if( argc <3 )
    {
        printf("Usage: %s  -l appid\n", argv[0]);
        return 0;
    }



    sprintf(url,"%s%s%s",TEST_URL_HEAD,uid,TEST_URL_END);




    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)url);

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

