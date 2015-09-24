
#include <stdio.h>
#include <stdlib.h>
#include "wilddog_demo_config.h"
#include "wilddog_api.h"

extern void wilddog_debug_printnode(const Wilddog_Node_T* node);

STATIC void test_gpioSet(const Wilddog_Node_T* p_snapshot)
{
    Wilddog_Str_T *p_value = NULL;
    int len = 0;
    if(p_snapshot->p_wn_child == NULL)
        return ;
    p_value = wilddog_node_getValue(p_snapshot->p_wn_child,&len);
    wilddog_debug_printnode(p_snapshot);
    if( p_value[0] == 0x30)
    {

        wiced_gpio_output_low(DEMO_LED1); 
        printf("\n set gpio to low \n");
     }
      else
    {
        
        printf("\n set gpio to hight \n");
        wiced_gpio_output_high(DEMO_LED1);
    }
}

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
       test_gpioSet(p_snapshot);
    
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
    return;
}

int demo(char* url, int* isUnConnected)
{
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL, *p_head = NULL;

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
    wilddog_debug("\n\t seting led1=1 to %s\n",url);
    /* expect <appId>.wilddogio.com/ has a new node "1"
     * test_onSetFunc : handle recv data while response
     * */
    wilddog_setValue(wilddog,p_head,test_onSetFunc,0);
    /* dele node */
    wilddog_node_delete(p_head);
    
    wilddog_debug("\n\t send observe to %s \n",url);
    /* send query */
    wilddog_addObserver(wilddog, WD_ET_VALUECHANGE,test_onQueryFunc, 0);
    while(1)
    {
		if(*isUnConnected)
		{
			wilddog_debug("wlan off!");
		}

        wilddog_trySync();
    }
    /* free wilddog*/
    wilddog_destroy(&wilddog);

    return 0;
}
