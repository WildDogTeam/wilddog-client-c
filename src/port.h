
#ifndef _PORT_H_
#define _PORT_H_

#include "wilddog_config.h"
#include <stdio.h>


#define wd_malloc(size) malloc(size)
#define wd_free(size) free(size)

typedef struct {
	char* addr;
} wilddog_address_t;


int wilddog_initSocket(size_t* socketId);
int wilddog_freeSocket(size_t socketId);
int wilddog_send(size_t socketId,wilddog_address_t*,char* tosend,size_t tosendLength);
int wilddog_receive(size_t socketId,wilddog_address_t*,char* toreceive,size_t toreceiveLength);


#endif
