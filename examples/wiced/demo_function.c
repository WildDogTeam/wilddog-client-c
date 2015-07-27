
#include <stdio.h>
#include <stdlib.h>
#include "wilddog_api.h"

extern void wilddog_debug_printnode(const Wilddog_Node_T* node);

STATIC void test_onQueryFunc(
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err)
{
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("query error!err = %d", err);
        return;
    }
    wilddog_debug("query success!");
    if(p_snapshot)
    {
        *(Wilddog_Node_T**)arg = wilddog_node_clone(p_snapshot);
    }
    
    return;
}
STATIC void test_onSetFunc(void* arg, Wilddog_Return_T err)
{
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("set error!err = %d", err);
        return;
    }
    wilddog_debug("set success!");
    *(BOOL*)arg = TRUE;
    return;
}

int demo(char* url, int* isUnConnected)
{
    BOOL isFinish = FALSE;
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL, *p_head = NULL, *p_node_query = NULL;

    

    p_head = wilddog_node_createObject(NULL);
    /* create a new child to "wilddog" , key is "1", value is "1" */
    p_node = wilddog_node_createUString((Wilddog_Str_T*)"led1",(Wilddog_Str_T*)"1");
    /*p_node was p_head's child  */
    wilddog_node_addChild(p_head, p_node);
    
    /*creat new a client*/
    wilddog = wilddog_initWithUrl((Wilddog_Str_T*)url);
    
    if(0 == wilddog)
    {
        wilddog_debug("new wilddog error");
        return 0;
    }
    printf("\n\t seting led1=1 to %s\n",url);
    /* expect <appId>.wilddogio.com/ has a new node "1"
     * test_onSetFunc : handle recv data while response
     * */
    wilddog_setValue(wilddog,p_head,test_onSetFunc,(void*)&isFinish);
    /* dele node */
    wilddog_node_delete(p_head);
    while(1)
    {
		if(*isUnConnected)
		{
			wilddog_debug("wlan off!");
			break;
		}
        if(TRUE == isFinish)
        {
            wilddog_debug("\tset led1=1 success!");
            break;
        }
        /*retransmit¡¢ received and handle response
         * */
        wilddog_trySync();
    }
    
    printf("\n\t send query to %s \n",url);
    /* send query */
    wilddog_getValue(wilddog, test_onQueryFunc, (void*)(&p_node_query));
    while(1)
    {
        if(p_node_query)
        {

            printf("\t get %s data: \n",url);
            /* printf recv node as json */
            wilddog_debug_printnode(p_node_query);
            /* free p_node_query */
            wilddog_node_delete(p_node_query);
            printf("\n");
            break;
        }
        wilddog_trySync();
    }
    /* free wilddog*/
    wilddog_destroy(&wilddog);

    return 0;
}
