/*
 * Wilddog.c
 *
 *  Created on: 2015年3月12日
 *      Author: x
 */

#include "Wilddog.h"
#include "port.h"
#include "wilddog_debug.h"
#include "pdu.c"

void _addOption(coap_pdu_t* coap_p,char * appid,char* path,char* token){
	//add host
	char host[strlen(appid)+strlen(WILDDOG_HOST_SUFFIX)+1];
	strcpy(host,appid);
	strcat(host,WILDDOG_HOST_SUFFIX);
	coap_add_option(coap_p,3,strlen(host),host);

	//add path
	char toTok[strlen(path)];
	strcpy(toTok,path);
	char* p=strtok(toTok,"/");
	while(p!=NULL){
		coap_add_option(coap_p,11,strlen(p),p);
		p=strtok(toTok,"/");
	}
	//add token
	if(token!=NULL){
		char tokenQuery[strlen(token)+20];
		strcpy(tokenQuery,"token=");
		strcat(tokenQuery,token);
		coap_add_option(coap_p,15,strlen(tokenQuery),tokenQuery);
	}
}


wilddog_t* wilddog_init(char* appid, char* token){

	wilddog_t* res=wd_malloc(sizeof(wilddog_t));
	memset(res,0,sizeof(res));
	/*
	res->appid=wd_malloc(strlen(appid)+1);
	strcpy(res->appid,appid);
	res->token=wd_malloc(strlen(token)+1);
	strcpy(res->token,token);
	*/
	res->appid=appid;
	res->token=token;
	res->remoteAddr.ip=res->serverIp;
	res->remoteAddr.port=WILDDOG_SERVER_PORT;
	char host[strlen(appid)+strlen(WILDDOG_HOST_SUFFIX)+1];
	strcpy(host,appid);
	strcat(host,WILDDOG_HOST_SUFFIX);
	if(wilddog_gethostbyname(res->serverIp,host)<0){
		wd_free(res);
		return NULL;

	}
	return res;
}

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
	int returnCode;
	//coap pack
	coap_pdu_t* coap_p=coap_new_pdu();
	coap_p->hdr->type=0;//CON
	coap_p->hdr->code=1;//GET
	coap_p->hdr->id=++wilddog->msgId;//message id

	//add host
	_addOption(coap_p,wilddog->appid,path,wilddog->token);
	//if socket not created,create
	if(wilddog->socketId==0){
		returnCode=wilddog_openSocket(&(wilddog->socketId));
		if(returnCode<0){
			WD_ERROR("wilddog_openSocket error:%d",returnCode);
			return -1;
		}
	}
	returnCode=wilddog_send(wilddog->socketId,&wilddog->remoteAddr,&coap_p->hdr,coap_p->length);
	if(returnCode<0){
		return -2;
	}
	size_t len;
	len=wilddog_receive(wilddog->socketId,&wilddog->remoteAddr,resultBuffer,max);
	return len;
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


int wilddog_destroy(wilddog_t* wilddog){
	if(wilddog){
		wd_free(wilddog);
		wilddog=0;
	}
	return 0;


}
