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

int wilddog_gethostbyname(char* ipString,char* host){
	printf("start gethostbyname\n");
	struct hostent* hp;
	struct sockaddr_in servaddr;
	memset((char*)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(WILDDOG_SERVER_PORT);
	if((hp=gethostbyname(host))==NULL){
		return -1;
	}
	memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
	char* tmp=inet_ntoa(servaddr.sin_addr);
	strcpy(ipString,tmp);
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
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(addr_in->port);
	servaddr.sin_addr.s_addr = inet_addr(addr_in->ip);

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
