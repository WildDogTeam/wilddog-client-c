/*
 * wilddog_config.h
 *
 *  Created on: 2015年3月12日
 *      Author: x
 */

#ifndef WILDDOG_CONFIG_H_
#define WILDDOG_CONFIG_H_

#define WILDDOG_URL_SUFFIX ".io.wilddog.com"
#ifndef size_t
#define size_t unsigned int
#endif

typedef struct {
	char* addr;
} wilddog_address_t;


int (*wilddog_initSocket)(size_t* socketId);
int (*wilddog_freeSocket)(size_t socketId);
int (*wilddog_send)(size_t socketId,wilddog_address_t,char* tosend,size_t tosendLength);
int (*wilddog_receive)(size_t socketId,wilddog_address_t,char* toreceive,size_t toreceiveLength);

void*(*wilddog_malloc)(size_t size);
void (*wilddog_free)(void*);


#endif /* WILDDOG_CONFIG_H_ */
