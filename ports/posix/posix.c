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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include "port.h"

int wilddog_gethostbyname(wilddog_address_t* addr,char* host){
	printf("start gethostbyname\n");
	struct hostent* hp;
	if((hp=gethostbyname(host))==NULL){
		return -1;
	}
	memcpy(addr->ip, hp->h_addr_list[0], hp->h_length);
	addr->len = hp->h_length;
	addr->port=htons(WILDDOG_SERVER_PORT);
	return 0;
}
int wilddog_openSocket(int* socketId){
	int fd;
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return -1;
	}
	*socketId = fd;
	return 0;
}
int wilddog_closeSocket(int socketId){
	return close(socketId);
}
int wilddog_send(int socketId,wilddog_address_t* addr_in,void* tosend,size_t tosendLength){

	struct sockaddr_in servaddr;    /* server address */
	/* fill in the server's address and data */
	memset((char*)&servaddr, 0, sizeof(servaddr));
	if(addr_in->len==4){
		servaddr.sin_family = AF_INET;
	}
	else{
		perror("unkown addr len");
		return -1;
	}
	servaddr.sin_port = addr_in->port;
	memcpy(&servaddr.sin_addr.s_addr,addr_in->ip,addr_in->len);

	if(sendto(socketId, tosend, tosendLength, 0, (struct sockaddr *)&servaddr,
	         sizeof(servaddr))<0){
		perror("sendto failed");
		return -1;
	}
	return 0;

}
int wilddog_receive(int socketId,wilddog_address_t* addr,void* buf,size_t bufLen){
	struct sockaddr_in remaddr;
	socklen_t addrlen = sizeof(remaddr);
	int recvlen;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 1;

	setsockopt(socketId, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

	recvlen = recvfrom(socketId, buf, bufLen, 0, (struct sockaddr *)&remaddr, &addrlen);
	return recvlen;
}
