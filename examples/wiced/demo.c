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

extern int demo(char *url);
/**
 *  Application start
 */
void application_start( void )
{
    char* url=TEST_URL;
    /* Initialise the device */
	do
	{
		wiced_init();
		/* Run the main application function */
		wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
		demo(url);
	}while(0);
}

