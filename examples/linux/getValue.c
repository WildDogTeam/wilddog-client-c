/*
*   get test file
*
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "wilddog.h"
#include "wilddog_debug.h"
#include "demo.h"


#define TEST_URL_HEAD		"coap://"
#define TEST_URL_END	".wilddogio.com"		


STATIC void test_getValueFunc(
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err)
{
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("getValue fail!");
        return;
    }

    if(p_snapshot)
    {
        wilddog_debug_printnode(p_snapshot);
        *(Wilddog_Node_T**)arg = wilddog_node_clone(p_snapshot);
        printf("\ngetValue success!\n");
    }
    return;


}

int main(int argc, char **argv)
{
	int opt;
    char uid[256];
    char url[1024];
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL;
   
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
        return 0;
    }
    wilddog_getValue(wilddog, test_getValueFunc, (void*)(&p_node));
    while(1)
    {
        if(p_node)
        {
#if 0
            wilddog_debug("print node:");
            wilddog_debug_printnode(p_node);
            wilddog_node_delete(p_node);
            printf("\n");
#endif
            wilddog_node_delete(p_node);
            break;
        }
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
    
    return 0;
}

