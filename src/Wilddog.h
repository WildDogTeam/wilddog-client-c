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
#include "cJSON.h"



typedef struct {
	char* queryString;
} wilddog_query_t;

typedef struct {
	char* appid;
	char* path;
	char* token;
	wilddog_address_t remoteAddr;
	int socketId;
	char serverIp[48];
	unsigned short msgId;
	cJSON* data;
	cJSON* newChild;

} wilddog_t;
typedef void (*Wilddog_value_cb)(wilddog_t* wilddog,int code);
typedef void (*Wilddog_childAdded_cb)(wilddog_t* wilddog,int code,char* newKey);
typedef void (*Wilddog_childRemoved_cb)(wilddog_t* wilddog,int code,char* removedKey);
typedef void (*Wilddog_childChanged_cb)(wilddog_t* wilddog,int code,char* changedKey);

wilddog_t* wilddog_init(char* appid,char* path, char* token);

/*
 * appid:appid
 * path:path
 * resBuffer: response buffer
 * maxLength: the max length of the buffer
 * return:
 * reallength of response
 * if <0 error
 *
 */
int wilddog_setAuth(wilddog_t* wilddog,char* auth);

int wilddog_query(wilddog_t* wilddog) ;

int wilddog_set(wilddog_t* wilddog,cJSON* data);

int wilddog_push(wilddog_t* wilddog,cJSON* data);

int wilddog_delete(wilddog_t* wilddog);

int wilddog_onChildChanged(wilddog_t* wilddog,Wilddog_childChanged_cb cb);

int wilddog_onValue(wilddog_t* wilddog,Wilddog_value_cb);

int wilddog_onChildRemoved(wilddog_t* wilddog,Wilddog_childRemoved_cb cb);

int wilddog_onChildAdded(wilddog_t* wilddog,Wilddog_childAdded_cb cb);

int wilddog_trySync(wilddog_t* wilddog);

int wilddog_off(wilddog_t* wilddog);

int wilddog_destroy(wilddog_t*);

int wilddog_dump(wilddog_t* wilddog,char * buffer,size_t len);

#endif /* WILDDOG_H_ */
