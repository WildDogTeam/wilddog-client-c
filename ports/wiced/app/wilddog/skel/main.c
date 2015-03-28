/*
 * main.c
 *
 *  Created on: 2014年12月12日
 *      Author: x
 */

#include "wiced.h"
#include "wiced_tcpip.h"
#include "Wilddog.h"

void application_start() {

	//INITIALISER_IPV4_ADDRESS(target_ip_addr,MAKE_IPV4_ADDRESS(192, 168, 1, 188));

	wiced_init();
	wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);

	unsigned char* url="coap://testApp.wilddogio.com/users/jackxy/devices/skel01";
	wilddog_t* client=wilddog_new(url);
	while (1) {

	}
	wiced_deinit();
}

