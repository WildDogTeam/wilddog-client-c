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
#include "port.h"

int wilddog_initSocket(size_t* socketId){
	return 0;
}
int wilddog_freeSocket(size_t socketId){
	return 0;

}
int wilddog_send(size_t socketId,wilddog_address_t* addr,char* tosend,size_t tosendLength){
	return 0;

}
int wilddog_receive(size_t socketId,wilddog_address_t* addr,char* toreceive,size_t toreceiveLength){
	return 0;
}

