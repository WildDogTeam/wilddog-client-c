
#ifndef _PORT_H_
#define _PORT_H_


#include <stdio.h>

#include "wilddog_config.h"


typedef struct {
	char len;//4 or 16
	char ip[16];
	unsigned short port;
} wilddog_address_t;

int wilddog_gethostbyname(wilddog_address_t* addr,char* host);
int wilddog_openSocket(int* socketId);
int wilddog_closeSocket(int socketId);
int wilddog_send(int socketId,wilddog_address_t*,void* tosend,size_t tosendLength);

/*
 * return <0 have not receive anything >=0 the length
 *
 *
 */

int wilddog_receive(int socketId,wilddog_address_t*,void* toreceive,size_t toreceiveLength);


#endif
