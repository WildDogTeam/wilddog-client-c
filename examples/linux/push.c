
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "wilddog.h"
#include "wilddog_debug.h"
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



int main(int argc, char **argv)
{
	int opt;
    char url[1024];
    BOOL isFinish = FALSE;
    Wilddog_T wilddog;
    Wilddog_Node_T * p_node = NULL, *p_head = NULL;
    
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
			//printf("uid:%s\n",optarg);
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

    p_head = wilddog_node_createObject(NULL);

    p_node = wilddog_node_createNum((Wilddog_Str_T *)"2",1234);
    wilddog_node_addChild(p_head, p_node);
    
    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)url);

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

