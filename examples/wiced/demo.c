/*
 * main.c
 *
 *  Created on: 2015-06-13 lixiongsheng
 */

#include <string.h>
#include "wiced.h"
#include "wilddog.h"
#include "wiced_tcpip.h"
#include "wilddog_api.h"
#include "wifi_config_dct.h"

extern int test_demo(char *url);
/**
 *  Application start
 */
void application_start( void )
{
    unsigned char* url=TEST_URL;
    /* Initialise the device */
	while(1)
	{
		wiced_init();
		/* Run the main application function */
		wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
		test_demo(url);
	}
}
