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
#include <assert.h>
#include "wilddog_config.h"

int posix_inistSocket(size_t* socketId){
	return 0;
}

int posix_freeSocket(size_t socketId){
	return 0;
}
int posix_send(size_t socketId,wilddog_address_t,char* tosend,size_t tosendLength){
	return 0;
}
int posix_receive(size_t socketId,wilddog_address_t,char* toreceive,size_t toreceiveLength){
	return 0;
}



wilddog_initSocket=posix_initSocket;
wilddog_freeSocket=posix_freeSocket;
wilddog_send=posix_send;
wilddog_receive=posix_receive;
wilddog_malloc=malloc;
wilddog_free=free;

