/*
 * Wilddog.c
 *
 *  Created on: 2015年3月12日
 *      Author: x
 */

#include "Wilddog.h"


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
		char* resultBuffer, size_t max){
	return 0;

}

/*
 *
 *
 */
int wilddog_put(wilddog_t* wilddog, char* path, char* buffer, size_t length){

	return 0;
}



int wilddog_post(wilddog_t* wilddog, char* path, char* buffer, size_t length,
		char* resultBuffer, size_t max){
	return 0;

}


int wilddog_delete(wilddog_t* wilddog, char* path,char* buffer,size_t length){
	return 0;

}


int wilddog_observe(wilddog_t* wilddog, char* path, wilddog_query_t* query,
		char* resultBuffer, size_t max){
	return 0;

}

int wilddog_waitNotice(wilddog_t* wilddog, char* buffer,size_t length){

	return 0;
}


int wilddog_stopObserve(wilddog_t* wilddog){
	return 0;
}


int wilddog_free(wilddog_t*){
	return 0;


}
