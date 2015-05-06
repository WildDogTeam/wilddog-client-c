/*
 * main.c
 *
 *  Created on: 2014骞�2鏈�2鏃� *      Author: x
 */

#include <string.h>
#include "wiced.h"
#include "wiced_tcpip.h"
#include "Wilddog.h"
#include "wilddog_client.h"
#include "wilddog_debug.h"
typedef struct
{
    uint8_t is_initialized;
    wiced_thread_t thread;

}wilddog_thread_t;

static wilddog_thread_t wilddog_thread = {0};
char toPrint[1024];

typedef int (*FUNCPTR)();

/*add led func here, just for test*/
typedef struct
{
	int enable;
	int led_num;
	wiced_gpio_t led_gpio;
	unsigned char led_status;
	cJSON * data;
	FUNCPTR get_led_status;
	FUNCPTR set_led_status;
}led_data_type;

#define LED1 WICED_GPIO_34
#define LED2 WICED_GPIO_35
#define LED3 WICED_GPIO_36
#define LED4 WICED_GPIO_37
#define LED_NUM 4


extern void led_on(wiced_gpio_t gpio);
extern void led_off(wiced_gpio_t gpio);
// just for test ,use buffer
static led_data_type led_data[LED_NUM];
static uint8_t l_string[][5]= 
{
 "led1",
 "led2",
 "led3",
 "led4"
};
led_data_type* led_get_data_from_index(int index)
{
	return &(led_data[index]);
}


int led_get_status(int index)
{
	led_data_type* data = NULL;
	data = led_get_data_from_index(index);
	
	return data->led_status;
}

int led_set_status(unsigned char status, int index)
{
	led_data_type* data = led_get_data_from_index(index);
//	printf("set %d, index = %d\n", status, index);
	data->led_status = status;
	if(TRUE == data->led_status)
	{
		led_on(data->led_gpio);
	}
	else
	{
		led_off(data->led_gpio);
	}
	return 0;
}

int led_data_init()
{
	int i;
	memset(led_data, 0, sizeof(led_data_type) * LED_NUM);
	for(i = 0; i < LED_NUM; i++)
	{
		led_data[i].led_num = i;
		led_data[i].led_gpio = LED1 + i;
		led_data[i].data = cJSON_CreateBool(cJSON_False);
		led_data[i].data->string = (char*)l_string[i];
		led_data[i].get_led_status = (FUNCPTR)led_get_status;
		led_data[i].set_led_status = (FUNCPTR)led_set_status;
		led_data[i].enable = TRUE;
	}
	return 0;
}

void led_init(wiced_gpio_t gpio)
{
	
	if(WICED_SUCCESS != wiced_gpio_init(gpio, OUTPUT_PUSH_PULL))
	{
		WD_DEBUG("gpio %d\n", gpio);
	}
}
void led_on(wiced_gpio_t gpio)
{
	led_init(gpio);
	if(WICED_SUCCESS != wiced_gpio_output_high(gpio))
	{
		WD_DEBUG("gpio %d\n", gpio);
	}
}
void led_off(wiced_gpio_t gpio)
{
	led_init(gpio);
	wiced_gpio_output_low(gpio);
}

void printJson(cJSON* data)
{
	printf("type=%d\nvaluestring=%s\nvalueint=%d\nvaluedouble=%f\nstring=%s\n", data->type, data->valuestring, data->valueint, data->valuedouble, data->string);
}
static int findSubJson(cJSON * src, cJSON* data)
{
	cJSON* pCurrent;
	cJSON* pNext = src;
	do
	{
		pCurrent = pNext;
		if(pCurrent->type == 6)
		{
			pCurrent = pCurrent->child;
		}
//		printf("current:\n%s\n",pCurrent->string);
//		printJson(pCurrent);
//		printf("data:\n%s\n",data->string);
		if(strcmp(pCurrent->string, data->string) == 0)
		{
//			printf("match! %d\n", pCurrent->type);
			switch(pCurrent->type)
			{
				case cJSON_False:
				case cJSON_True:
					return pCurrent->type;
				case cJSON_NULL:
					return -1;
				case cJSON_Number:
					return pCurrent->valueint;
				case cJSON_String:
					return atoi(pCurrent->valuestring);
				default:
					return -1;
			}

		}
		pNext = pCurrent->next;
	}while(pNext != NULL);
	return -1;
}

/*end led func*/

void onQueryComplete(wilddog_t* wilddog,int handle,int err){
	int i;
	int status;
	printf("!!onQueryComplete\n");
    wilddog_dump(wilddog,toPrint,sizeof(toPrint));
    //printf("%s",toPrint);
    if(err){
        printf("query error:%d\n",err);
		return;
    }
    else{
        printf("result:\n%s\n",cJSON_Print(wilddog->data));
    }
	for(i = 0; i < LED_NUM; i++)
	{
		status = findSubJson(wilddog->data, led_data[i].data);
		if(status >= 0)
		{
			//printf("%s, %d\n",__func__, i);
			status = status > 0? 1: 0;
			led_set_status(status, i);
		}
	}
	
}
void onQueryComplete2(wilddog_t* wilddog,int handle,int err){

	printf("!!onQueryComplete2\n");
    wilddog_dump(wilddog,toPrint,sizeof(toPrint));
    printf("%s",toPrint);
    if(err){
        printf("query error:%d\n",err);
		return;
    }
    else{
        printf("result:\n%s\n",cJSON_Print(wilddog->data));
    }



}

void onData(wilddog_t* wilddog,cJSON* value){
		int i;
	int status;
	char * data=cJSON_Print(value);
	printf("new data: %s \n",data);
	for(i = 0; i < LED_NUM; i++)
	{
		status = findSubJson(wilddog->data, led_data[i].data);
		if(status >= 0)
		{
			//printf("%s, %d\n",__func__, i);
			led_set_status(status, i);
		}
	}

	free(data);
}

void wilddog_get(int argc, char **argv)
{
	int i;
	char* url = argv[1];
	wilddog_t* client=wilddog_new((unsigned char* )url);
	wilddog_query(client,onQueryComplete);
	for(i = 0; i < 1000; i++)
	{
		wilddog_trySync(client);
	}
}
void wilddog_client() {

    int i = 0;
	wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);

	unsigned char* url="coap://demo-iot.wilddogio.com/";
	wilddog_t* client=wilddog_new(url);

	led_data_init();
    wilddog_query(client,onQueryComplete);
//	wilddog_on(client, onData, onQueryComplete2);
	while (1) {
	    i++;
	    if( i % 4 == 0)
	        wilddog_query(client,onQueryComplete);

	    wilddog_trySync(client);
	}
	wiced_deinit();
}

wiced_result_t wilddog_client_init(void)
{
    wiced_result_t result;

    if(TRUE == wilddog_thread.is_initialized)
        return WICED_SUCCESS;
    result = wiced_rtos_create_thread(&(wilddog_thread.thread), WICED_DEFAULT_LIBRARY_PRIORITY, "wilddog_d", wilddog_client, 0x3000, NULL);

    if(WICED_SUCCESS == result)
        wilddog_thread.is_initialized = TRUE;

    return result;
}

/**
 *  Application start
 */
void application_start( void )
{
    /* Initialise the device */
    wiced_init( );

    /* Run the main application function */
    wilddog_client_init( );

}
