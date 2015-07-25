/*
 * main.c
 *
 *  Created on: 2015-06-13 lixiongsheng
 */

#include <string.h>
#include "wiced.h"
#include "wilddog.h"
#include "wiced_tcpip.h"
#include "wifi_config_dct.h"
#include "wilddog_demo_config.h"
int g_wifi_down = 1;

extern int demo(char *url, int* isUnConnected);
void link_callback_up(void)
{
    printf("link_callback_up \n");
    g_wifi_down = 0;
}
void link_callback_down(void)
{
    printf("link_callback_down \n");
    g_wifi_down = 1;
}
/**
 *  Application start
 */
void application_start( void )
{
    char* url=TEST_URL;
    int ret;
    /* Initialise the device */
	do
	{
		wiced_init();

		/* Run the main application function */
        ret = wiced_network_register_link_callback(link_callback_up,link_callback_down);

		ret = wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
		if(WICED_SUCCESS == ret)
		{
		    g_wifi_down = 0;
		    demo(url, &g_wifi_down);
		}
		
		wiced_network_deregister_link_callback(NULL, NULL);
		wiced_network_down(WICED_STA_INTERFACE);
		wiced_deinit();
	}while(0);
}

