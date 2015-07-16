
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "wilddog.h"
#include "wilddog_debug.h"
#include "demo.h"


#define TEST_URL_HEAD		"coap://"
#define TEST_URL_END	".wilddogio.com"		

STATIC void test_setValueFunc(void* arg, Wilddog_Return_T err)
{
                        
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("setValue error!");
        return;
    }
    wilddog_debug("setValue success!");
    *(BOOL*)arg = TRUE;
    return;
}


int main(int argc, char **argv)
{
	int opt;
    char uid[256];
    char url[1024];
    BOOL isFinish = FALSE;
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL, *p_head = NULL;

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

    p_head = wilddog_node_createObject(NULL);

    /* create a new child to "wilddog" , key is "1", value is "123456" */
    p_node = wilddog_node_createUString((Wilddog_Str_T *)"1",(Wilddog_Str_T *)"123456");

    wilddog_node_addChild(p_head, p_node);
    
    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)url);

    if(0 == wilddog)
    {
        wilddog_debug("new wilddog error");
        return 0;
    }
    /* expect test1234.wilddogio.com/ has a new node "1" */
    wilddog_setValue(wilddog,p_head,test_setValueFunc,(void*)&isFinish);
    wilddog_node_delete(p_head);
    while(1)
    {
        if(TRUE == isFinish)
        {
            //wilddog_debug("set success!");
            break;
        }
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
    
    return 0;
}

