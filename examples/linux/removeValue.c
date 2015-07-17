
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "wilddog.h"
#include "wilddog_debug.h"
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


int main(int argc, char **argv)
{
	int opt;
    char url[1024];
    BOOL isFinished = FALSE;
    Wilddog_T wilddog;
    

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

