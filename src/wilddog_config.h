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
typedef int (*initSocketFunc_t)(size_t* socketId);
typedef int (*freeSocketFunc_t)(size_t socketId);
typedef int (*sendFunc_t)(size_t socketId,wilddog_address_t,char* tosend,size_t tosendLength);
typedef int (*receiveFunc_t)(size_t socketId,wilddog_address_t,char* toreceive,size_t toreceiveLength);
typedef int (*handlePackFunc_t)(coap_pdu_t* pack);
typedef void*(*mallocFunc_t)(size_t size);
typedef void (*freeFunc_t)(void*);


initSocketFunc_t* wilddog_initSocket;
freeSocketFunc_t* wilddog_freeSocket;
sendFunc_t* wilddog_send;
receiveFunc_t* wilddog_receive;
mallocFunc_t* wilddog_malloc;
freeFunc_t* wilddog_free;

#endif /* WILDDOG_CONFIG_H_ */
