#ifndef WILDDOG_PORT_TYPE_ESP
#include <stdio.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "wilddog.h"
#include "test_lib.h"

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"
#include "gpio.h"
#include "user_config.h"



extern os_timer_t test_timer1;
extern os_timer_t test_timer2;


Wilddog_T led_wilddog = 0;
BOOL isFinished = FALSE;


STATIC void WD_SYSTEM test_onObserveFunc	
    (   
    const Wilddog_Node_T* p_snapshot,   
    void* arg,  
    Wilddog_Return_T err
    )
{
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)	
    {       
        wilddog_debug("observe failed!, err = %d", err);        
        return; 
    }   
    wilddog_debug("observe data!"); 
    wilddog_debug_printnode(p_snapshot);  
    printf("\n");

    if(strncmp(p_snapshot->p_wn_child->p_wn_key, "led", 3) == 0)
    {
        if(strncmp(p_snapshot->p_wn_child->p_wn_value, "0", 1) == 0)
        {
            #if (defined DEF_LED_HARDWARE == 1)
            GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 0);
            #else
            wilddog_debug("led off");
            #endif
        }
        else
        {
        
            #if (defined DEF_LED_HARDWARE == 1)
            GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 1);
            #else
            wilddog_debug("led on");
            #endif
        }
    }
    
}





void WD_SYSTEM observe_sync(void *arg)
{
	if(*(BOOL *)arg == TRUE)
    {   
        os_timer_disarm(&test_timer2);

        wilddog_destroy(&led_wilddog);

    }
	else
    {   
        wilddog_trySync();  

        os_timer_setfn(&test_timer2, (os_timer_func_t *)observe_sync, arg);
        
    	os_timer_arm(&test_timer2, 1000, 0);    	
    }

}

void WD_SYSTEM sync(void *arg)
{
	if(*(BOOL *)arg == TRUE)
    {   
        os_timer_disarm(&test_timer2);
        isFinished = FALSE;

        wilddog_addObserver(led_wilddog, WD_ET_VALUECHANGE, test_onObserveFunc, (void*)&isFinished);
        os_timer_setfn(&test_timer2, (os_timer_func_t *)observe_sync, (void*)&isFinished);
    	os_timer_arm(&test_timer2, 1000, 0);    	
    }
	else
    {   
        wilddog_trySync();  
        os_timer_setfn(&test_timer2, (os_timer_func_t *)sync, arg);
    	os_timer_arm(&test_timer2, 1000, 0);    	
    }
}



STATIC void WD_SYSTEM test_setValueFunc(void* arg, Wilddog_Return_T err)
{

    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("setValue error! error:%d\n", err);
        return;
    }
    wilddog_debug("setValue success!");
    *(BOOL*)arg = TRUE;
    return;
}


/*************************************build complete binary tree**************************************************************************/
int WD_SYSTEM test_buildtreeFunc(const char *p_userUrl)
{
	int m = 0;
	u8 url[strlen(TEST_URL)+20];
	Wilddog_Node_T *p_head = NULL, *p_node = NULL;
	    
	isFinished = FALSE;
	memset((void*)url,0,sizeof(url));
	sprintf((char*)url,"%s%s",TEST_URL,TEST_LED);

#if defined DEF_LED_HARDWARE == 1
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);    
#endif

    led_wilddog = 0;
	led_wilddog = wilddog_initWithUrl(url);
	if(0 == led_wilddog)
	{
		wilddog_debug("new wilddog error");
		return -1;
	}


    p_head = wilddog_node_createObject(NULL);
    p_node = wilddog_node_createUString((Wilddog_Str_T *)"led","1");
	wilddog_node_addChild(p_head, p_node);
	wilddog_setValue(led_wilddog,p_head,\
        test_setValueFunc,(void*)&isFinished);
		
	os_timer_disarm(&test_timer2);
    os_timer_setfn(&test_timer2, (os_timer_func_t *)sync, (void*)&isFinished);
	os_timer_arm(&test_timer2, 1000, 0);    		
}


