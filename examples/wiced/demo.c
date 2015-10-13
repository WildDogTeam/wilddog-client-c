/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: demo.c
 *
 * Description: This demo show you how to control an led through server. We will
 *              creat a led node and push it to server,then subscribe the led 
 *              node, so you can change led's value in the server to control 
 *              led's on and off.
 *                  
 *
 *
 * Usage: 
 *          1. Copy SDK to wiced IDE's <apps> folder, probable path may be 
 *                  <WICED-SDK\apps\wilddog_client_c>.
 *          2. In <wilddog_client_c\examples\wiced\wilddog_demo_config.h>:
 *
 *                  default TEST_URL is coap://<your appid>.wilddogio.com/[path]
 *                  , change <your appid> to your <appid>, which is the appid 
 *                  of the app you created, and path is the path(
 *                  node path) in the app. if the tree like followed , <1> is 
 *                  your appid, <a> and <a/b> are both path.
 *                  
 *                  after runing demo, your data tree in cloud would like that:
 *
 *                  1.wilddogio.com
 *                  |
 *                  + a
 *                    |
 *                    + b
 *                        |
 *                         +led1:1
 *
 *          3. Modify CLIENT_AP_SSID and CLIENT_AP_PASSPHRASE to the SSID you 
 *             want to connect in wilddog_demo_config.h.
 *
 *          4. Change DEMO_LED1 to the gpio which connect to your LED.
 *
 *          5. Create a target in <Make Target>(may be in the right corner) like
 *              this:
 *              wilddog_client_c.project.wiced-<yourboard> download run
 *              
 *              <yourboard> is the board type, like BCM943362WCD4, etc.
 *
 *          6. Connect your board with PC, double click this target you created,
 *             it will build and write to your board, and then you can control
 *             your led.
 *          
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.3        lxs             2015-07-16  Create file.
 *
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
        ret = wiced_network_register_link_callback(link_callback_up, \
                                                   link_callback_down);

		ret = wiced_network_up(WICED_STA_INTERFACE, \
                               WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
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

