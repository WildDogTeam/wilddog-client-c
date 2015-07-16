
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "wilddog.h"
#include "wilddog_debug.h"
#include "demo.h"


#define TEST_URL_HEAD		"coap://"
#define TEST_URL_END	".wilddogio.com"		

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


int main(int argc, char **argv)
{
	int opt;
    char uid[256];
    char url[1024];
    BOOL isFinished = FALSE;
    Wilddog_T wilddog;
    
	
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

