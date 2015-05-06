/*
 * posix.c
 *
 *  Created on: 2015骞�鏈�2鏃� *      Author: x
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "port.h"
#include "wiced.h"



#define WILDDOG_MAKE_IPV4(a,b,c,d) (((uint8_t)(a) << 24) | ((uint8_t)(b) << 16) | ((uint8_t)(c) << 8) | ((uint8_t)(d)))
extern wiced_result_t dns_client_add_server_address( wiced_ip_address_t address );
int wilddog_gethostbyname(wilddog_address_t* addr,char* host){
	addr->len=4;
	wiced_ip_address_t address;

	uint32_t tmpIp = 0;
	uint32_t  dns_ip;
    /* Add free DNS server (114.114.114.114) */
	dns_ip = WILDDOG_MAKE_IPV4(114,114,114,114);
    SET_IPV4_ADDRESS( address, &dns_ip );
    dns_client_add_server_address( address );
	if(wiced_hostname_lookup( host, &address, 4000)!=WICED_SUCCESS){
		uint32_t ip=WILDDOG_MAKE_IPV4(211,151,208,196);
		WPRINT_APP_INFO(("failed to wiced_hostname_lookup, make ip %08x\n", (unsigned int)ip));
		address.ip.v4 = ip;
	}
	tmpIp = address.ip.v4;
	/*test */
	//tmpIp = WILDDOG_MAKE_IPV4(211,151,208,196);
	tmpIp = htonl(tmpIp);
	memcpy(addr->ip, &tmpIp,4);

	return 0;
}

int wilddog_openSocket(int* socketId){
	wiced_udp_socket_t* socket= NULL;

	socket = malloc(sizeof(wiced_udp_socket_t));
	if(NULL == socket)
	{
		printf("%s malloc error!\n", __func__);
		return -1;
	}
	if(wiced_udp_create_socket( socket, WICED_ANY_PORT,WICED_STA_INTERFACE)!=WICED_SUCCESS){
		return -1;
	}
	*socketId =(int)socket;//I can do this with 32bit machine

	return 0;
}
int wilddog_closeSocket(int socketId){
	wiced_udp_delete_socket((wiced_udp_socket_t*)socketId);
	if(socketId){
		free(socketId);
		socketId = 0;
	}
	return 0;
}
int wilddog_send(int socketId,wilddog_address_t* addr_in,void* tosend,size_t tosendLength){
	wiced_udp_socket_t* socket=(wiced_udp_socket_t*)socketId;
	wiced_ip_address_t ipaddr;
	ipaddr.version=WICED_IPV4;
	ipaddr.ip.v4=WILDDOG_MAKE_IPV4(addr_in->ip[0], addr_in->ip[1], addr_in->ip[2], addr_in->ip[3]);

	wiced_packet_t* packet;
	uint8_t* data;
	uint16_t aval;
	if (wiced_packet_create_udp(socket, 0, &packet, &data, &aval)!= WICED_SUCCESS) {
		WPRINT_APP_INFO(("error create packet ...\r\n"));
		return -1;
	}
	if(aval < tosendLength)
	{
		wiced_packet_delete(packet); /* Delete packet, since the send failed */
		WPRINT_APP_INFO(("too large length to translate! should be %d, want send %d\n", aval, tosendLength));
		return -1;
	}
//	wiced_packet_set_data_start(packet,(uint8_t * )data);
	memcpy(data, tosend, tosendLength);

	wiced_packet_set_data_end(packet, (uint8_t*) (data + tosendLength));

	if (wiced_udp_send(socket, &ipaddr, addr_in->port, packet)
			!= WICED_SUCCESS) {
		WPRINT_APP_INFO(("UDP packet send failed\r\n"));
		wiced_packet_delete(packet); /* Delete packet, since the send failed */
	} else {
		WPRINT_APP_INFO(("send packet success!\n"));
		return 0;
	}
	return -1;

}
int wilddog_receive(int socketId,wilddog_address_t* addr_in,void* buf,size_t bufLen){
	wiced_udp_socket_t* socket=(wiced_udp_socket_t*)socketId;
//	wiced_packet_t* receive = (wiced_packet_t*)buf;
	wiced_packet_t* receive = NULL;
	uint16_t aval;
	uint8_t* rxData;
	uint16_t rxDataLength;
	wiced_ip_address_t recieve_ip_addr;
	uint16_t receive_port;
	wiced_result_t result;
	
	result = wiced_udp_receive(socket, &receive, 500);
	if (result == WICED_SUCCESS) {
		memcpy(buf, receive, bufLen);
		wiced_packet_delete(receive);
		receive = (wiced_packet_t*)buf;
		wiced_udp_packet_get_info(receive, &recieve_ip_addr, &receive_port);
		{
			unsigned int tmpIp = (unsigned int)recieve_ip_addr.ip.v4;
		WPRINT_APP_INFO(
				("UDP Rx: receve packet from ip:%u.%u.%u.%u port:%u ", (tmpIp>>24) & 0xff, (tmpIp>>16) & 0xff, (tmpIp>> 8) & 0xff, tmpIp& 0xff, receive_port));
		}
		if(!recieve_ip_addr.ip.v4==MAKE_IPV4_ADDRESS(addr_in->ip[0], addr_in->ip[1], addr_in->ip[2], addr_in->ip[3])){
			return 0;
		}
		if(wiced_packet_get_data(receive, 0, (uint8_t**) &rxData,
				&rxDataLength, &aval)!=WICED_SUCCESS){
			WPRINT_APP_INFO(("get data from packet error \r\n"));
		}
		if(rxDataLength>bufLen){
			WPRINT_APP_INFO(("bufSize overflow\r\n"));
			return -1;
		}
		memcpy(buf,rxData,rxDataLength);
		return rxDataLength;

	}
	return 0;
}
