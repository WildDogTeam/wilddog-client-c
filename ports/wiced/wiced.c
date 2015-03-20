/*
 * posix.c
 *
 *  Created on: 2015年3月12日
 *      Author: x
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "port.h"
#include "wiced.h"

#define DEFAULT_IP 192 168 1 189
int wilddog_gethostbyname(wilddog_address_t* addr,char* host){
	addr->len=4;
	wiced_ip_address_t address;
	if(wiced_hostname_lookup( host, &address, 2000)!=WICED_SUCCESS){
		uint32_t ip=MAKE_IPV4_ADDRESS(192,168,1,189);
		memcpy(addr->ip,&ip,4);
	}
	memcpy(addr->ip, &address.ip.v4,4);
	return 0;
}
int wilddog_openSocket(int* socketId){
	wiced_udp_socket_t* socket;
	if(wiced_udp_create_socket( socket, WICED_ANY_PORT,WICED_STA_INTERFACE)!=WICED_SUCCESS){
		return -1;
	}
	*socketId =(int)socket;//I can do this with 32bit machine
	return 0;
}
int wilddog_closeSocket(int socketId){
	wiced_udp_delete_socket((wiced_udp_socket_t*)socketId);
	return 0;
}
int wilddog_send(int socketId,wilddog_address_t* addr_in,void* tosend,size_t tosendLength){
	wiced_udp_socket_t* socket=(wiced_udp_socket_t*)socketId;
	wiced_ip_address_t ipaddr;
	ipaddr.version=WICED_IPV4;
	ipaddr.ip.v4=MAKE_IPV4_ADDRESS(addr_in->ip[0], addr_in->ip[1], addr_in->ip[2], addr_in->ip[3]);

	wiced_packet_t* packet;
	uint8_t* data;
	uint16_t aval;
	if (wiced_packet_create_udp(socket, 0, &packet, &data, &aval)!= WICED_SUCCESS) {
		WPRINT_APP_INFO(("error create packet ...\r\n"));
		return -1;
	}
	memcpy(data, tosend, tosendLength);
	wiced_packet_set_data_end(packet, (uint8_t*) (data + tosendLength));
	if (wiced_udp_send(&socket, &ipaddr, addr_in->port, packet)
			!= WICED_SUCCESS) {
		WPRINT_APP_INFO(("UDP packet send failed\r\n"));
		wiced_packet_delete(packet); /* Delete packet, since the send failed */
	} else {
		return 0;
	}
	return -1;

}
int wilddog_receive(int socketId,wilddog_address_t* addr_in,void* buf,size_t bufLen){
	wiced_udp_socket_t* socket=(wiced_udp_socket_t*)socketId;
	wiced_packet_t* receive;
	uint16_t aval;
	uint8_t* rxData;
	uint16_t rxDataLength;
	wiced_ip_address_t recieve_ip_addr;
	uint16_t receive_port;

	if (wiced_udp_receive(&socket, &receive, 10) == WICED_SUCCESS) {
		wiced_udp_packet_get_info(receive, &recieve_ip_addr, &receive_port);
		WPRINT_APP_INFO(
				("UDP Rx: receve packet from ip:%d port:%d ", recieve_ip_addr.ip.v4, receive_port));
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
