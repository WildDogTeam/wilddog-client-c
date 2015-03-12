/*
 * Wilddog.h
 *
 *  Created on: 2015年3月12日
 *      Author: x
 */

#ifndef WILDDOG_H_
#define WILDDOG_H_
#include "wilddog_config.h"

typedef struct {
	char* queryString;
} wilddog_query_t;
typedef struct {

} wilddog_address_t;

typedef struct {
	char* appid;
	char* token;
	size_t socketId;
} wilddog_t;

wilddog_t* wilddog_init(char* appid, char* token);

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
int wilddog_get(wilddog_t* wilddog, char* path, wilddog_query_t* query,
		char* resultBuffer, size_t max);

/*
 *
 *
 */
int wilddog_put(wilddog_t* wilddog, char* path, char* buffer, size_t length);



int wilddog_post(wilddog_t* wilddog, char* path, char* buffer, size_t length,
		char* resultBuffer, size_t max);


int wilddog_delete(wilddog_t* wilddog, char* path,char* buffer,size_t length);


int wilddog_observe(wilddog_t* wilddog, char* path, wilddog_query_t* query,
		char* resultBuffer, size_t max);

int wilddog_waitNotice(wilddog_t* wilddog, char* buffer,size_t length);


int wilddog_stopObserve();


int wilddog_free(wilddog_t*);

#endif /* WILDDOG_H_ */
