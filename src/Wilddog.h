/*
 * Wilddog.h
 *
 *  Created on: 2015年3月12日
 *      Author: x
 */

#ifndef __WILDDOG_H_
#define __WILDDOG_H_
#include "wilddog_config.h"
#include "port.h"
#include "pdu.h"
#include "cJSON.h"
#include "utlist.h"
typedef struct tagA wilddog_t;
typedef struct tagB request_t;
typedef void (*onDataFunc)(wilddog_t* wilddog,cJSON* value);
typedef void (*onCompleteFunc)(wilddog_t* wilddog,int handle,int err);

typedef struct {
	char* queryString;
} wilddog_query_t;

typedef struct tagB {
    coap_pdu_t* coap_msg;
    unsigned char flag; //observe 0x01
    unsigned int timestamp; //ms
    request_t *next;
    unsigned int maxAge;
    int handle;
    onCompleteFunc callback;
}tagB;

typedef struct tagA{
	char* appid;
	char* path;
	char* auth;
	wilddog_address_t remoteAddr;
	int socketId;
	char serverIp[48];
	unsigned short msgId;
	unsigned int token;
	cJSON* data;
	cJSON* newChild;
	request_t* sentQueue;
	onDataFunc onData;
	unsigned int timestamp;//ms
	unsigned int lastSend;
	unsigned int lastReceive;

}tagA;


wilddog_t* wilddog_init(char* appid,char* path, char* token);

void wilddog_setAuth(wilddog_t* wilddog,unsigned char* auth);
/**
 * return:
 * 	 >=0 handler <0 error code
 */
int wilddog_query(wilddog_t* wilddog,onCompleteFunc callback);

int wilddog_set(wilddog_t* wilddog,cJSON* data,onCompleteFunc callback);

int wilddog_push(wilddog_t* wilddog,cJSON* data,onCompleteFunc callback);

int wilddog_delete(wilddog_t* wilddog,onCompleteFunc callback);

int wilddog_on(wilddog_t* wilddog,onDataFunc onDataChange,onCompleteFunc callback);

int wilddog_off(wilddog_t* wilddog);

int wilddog_trySync(wilddog_t* wilddog);

int wilddog_destroy(wilddog_t*);

int wilddog_dump(wilddog_t* wilddog,char * buffer,size_t len);

#endif /* WILDDOG_H_ */
