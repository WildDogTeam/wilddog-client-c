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
char resBuf[WILDDOG_BUF_SIZE];
void printBuf(unsigned char* iter, size_t size) {
	int i = 0;
	for (; i < size; i += 4) {
		printf("%2x %2x %2x %2x\n", iter[i], iter[i + 1], iter[i + 2],
				iter[i + 3]);
	}

}
inline unsigned int codeToInt(unsigned int code) {
	unsigned int readable = (code >> 5) * 100 + (code & 0x1F);
	return readable;
}
void _addOption(coap_pdu_t* coap_p, char * appid, char* path, char* token) {
	//add host
	char host[strlen(appid) + strlen(WILDDOG_HOST_SUFFIX) + 1];
	strcpy(host, appid);
	strcat(host, WILDDOG_HOST_SUFFIX);
	coap_add_option(coap_p, 3, strlen(host), host);

	//add path
	char toTok[strlen(path)];
	strcpy(toTok, path);
	size_t written;
	char* p = strtok(toTok, "/");
	while (p != NULL) {
		written = coap_add_option(coap_p, 11, strlen(p), p);
		p = strtok(NULL, "/");
	}
	//add token
	if (token != NULL) {
		char tokenQuery[strlen(token) + 20];
		strcpy(tokenQuery, "token=");
		strcat(tokenQuery, token);
		coap_add_option(coap_p, 15, strlen(tokenQuery), tokenQuery);
	}
}


int wilddog_get_raw(wilddog_t* wilddog,char* resultBuffer,size_t bufSize){

	int returnCode;
	//coap pack
	coap_pdu_t* coap_p = coap_pdu_init(0, 1, htons(++wilddog->msgId),
			COAP_MAX_SIZE);
	_addOption(coap_p, wilddog->appid, wilddog->path, wilddog->token);
	//if socket not created,create
	if (wilddog->socketId == 0) {
		returnCode = wilddog_openSocket(&(wilddog->socketId));
		if (returnCode < 0) {
			coap_delete_pdu(coap_p); //delete
			coap_p = NULL;
			WD_ERROR("wilddog_openSocket error:%d", returnCode);
			return -1;
		}
	}
	coap_show_pdu(coap_p);

	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			coap_p->hdr, coap_p->length);
	coap_delete_pdu(coap_p); //delete
	coap_p = NULL;
	if (returnCode < 0) {
		return -2;
	}
	size_t len;
	len = wilddog_receive(wilddog->socketId, &(wilddog->remoteAddr), resBuf,
			sizeof(resBuf), WILDDOG_RECV_TIMEOUT);
	//parse
	coap_pdu_t * receive_p = coap_new_pdu();
	returnCode = coap_pdu_parse(resBuf, len, receive_p);
	coap_show_pdu(receive_p);
	if (returnCode <= 0) {
		WD_ERROR("coap_pdu_parse error:%d", returnCode);
		coap_delete_pdu(receive_p);
		return -3;
	}

	printBuf((unsigned char *) receive_p->hdr, receive_p->length);
	//get Code
	unsigned int statusCode = codeToInt(receive_p->hdr->code);

	WD_DEBUG("statusCode:%d", statusCode);
	size_t dataLen;
	unsigned char * dataPtr;
	coap_get_data(receive_p, &dataLen, &dataPtr);
	if(dataLen>bufSize){
		coap_delete_pdu(receive_p);
		return -4;

	}

	memcpy(resultBuffer, dataPtr, dataLen);

	coap_delete_pdu(receive_p);
	if ((statusCode / 100) == 2) {
		//OK
		return dataLen;
	} else {
		return 0 - statusCode;
	}


}

/*
 *
 *
 */
int wilddog_put_raw(wilddog_t* wilddog, char* buffer, size_t length) {

	int returnCode;
	//coap pack
	coap_pdu_t* coap_p = coap_pdu_init(0, 3, htons(++wilddog->msgId),
			COAP_MAX_SIZE);
	_addOption(coap_p, wilddog->appid,wilddog->path, wilddog->token);
	coap_add_data(coap_p, length, buffer);
	//if socket not created,create
	if (wilddog->socketId == 0) {
		returnCode = wilddog_openSocket(&(wilddog->socketId));
		if (returnCode < 0) {
			coap_delete_pdu(coap_p); //delete
			coap_p = NULL;
			WD_ERROR("wilddog_openSocket error:%d", returnCode);
			return -1;
		}
	}

	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			coap_p->hdr, coap_p->length);
	coap_delete_pdu(coap_p); //delete
	coap_p = NULL;
	if (returnCode < 0) {
		return -2;
	}
	size_t len;
	len = wilddog_receive(wilddog->socketId, &(wilddog->remoteAddr), resBuf,
			sizeof(resBuf), WILDDOG_RECV_TIMEOUT);
//parse
	coap_pdu_t * receive_p = coap_new_pdu();
	returnCode = coap_pdu_parse(resBuf, len, receive_p);

	if (returnCode <= 0) {
		WD_ERROR("coap_pdu_parse error:%d", returnCode);
		coap_delete_pdu(receive_p);
		return -3;
	}

	//printBuf((unsigned char*) receive_p->hdr, receive_p->length);
//get Code
	unsigned int statusCode = codeToInt(receive_p->hdr->code);


	if ((statusCode / 100) == 2) {
		//OK
		returnCode = 0;
	} else {

		returnCode = 0 - statusCode;
	}
	if (returnCode < 0) {

		unsigned char* resultPtr;
		size_t bufSize;
		char errBuf[100];

		if (coap_get_data(receive_p, &bufSize, &resultPtr)) {
			strncpy(errBuf, resultPtr,bufSize);
		}
		WD_ERROR("wilddog put error:%d,%s", statusCode, resultPtr);

	}
	coap_delete_pdu(receive_p);
	return returnCode;

}
int wilddog_post_raw(wilddog_t* wilddog, char* path, char* buffer, size_t length,
		char* resultBuffer, size_t max) {
		int returnCode;
		//coap pack
		coap_pdu_t* coap_p = coap_pdu_init(0, 2, htons(++wilddog->msgId),
				COAP_MAX_SIZE);
		_addOption(coap_p, wilddog->appid, wilddog->path, wilddog->token);
		coap_add_data(coap_p, length, buffer);
		//if socket not created,create
		if (wilddog->socketId == 0) {
			returnCode = wilddog_openSocket(&(wilddog->socketId));
			if (returnCode < 0) {
				coap_delete_pdu(coap_p); //delete
				coap_p = NULL;
				WD_ERROR("wilddog_openSocket error:%d", returnCode);
				return -1;
			}
		}
		coap_show_pdu(coap_p);

		returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
				coap_p->hdr, coap_p->length);
		coap_delete_pdu(coap_p); //delete
		coap_p = NULL;
		if (returnCode < 0) {
			return -2;
		}
		size_t len;
		len = wilddog_receive(wilddog->socketId, &(wilddog->remoteAddr), resBuf,
				sizeof(resBuf), WILDDOG_RECV_TIMEOUT);
	//parse
		coap_pdu_t * receive_p = coap_new_pdu();
		returnCode = coap_pdu_parse(resBuf, len, receive_p);

		if (returnCode <= 0) {
			WD_ERROR("coap_pdu_parse error:%d", returnCode);
			coap_delete_pdu(receive_p);
			return -3;
		}
		unsigned char* resultPtr;
		size_t bufSize;
		if (coap_get_data(receive_p, &bufSize, &resultPtr)) {

			strncpy(resultBuffer, resultPtr,bufSize);
		}
		unsigned int statusCode = codeToInt(receive_p->hdr->code);
		if ((statusCode / 100) == 2) {
			//OK
			returnCode = bufSize;
		} else {

			returnCode = 0 - statusCode;
			WD_ERROR("wilddog put error:%d,%s", statusCode, resultBuffer);

		}
		coap_delete_pdu(receive_p);
		return returnCode;
}
int wilddog_delete_raw(wilddog_t* wilddog) {

	int returnCode;
	//coap pack
	coap_pdu_t* coap_p = coap_pdu_init(0, 4, htons(++wilddog->msgId),
			COAP_MAX_SIZE);
	_addOption(coap_p, wilddog->appid,wilddog-> path, wilddog->token);
	//if socket not created,create
	if (wilddog->socketId == 0) {
		returnCode = wilddog_openSocket(&(wilddog->socketId));
		if (returnCode < 0) {
			coap_delete_pdu(coap_p); //delete
			coap_p = NULL;
			WD_ERROR("wilddog_openSocket error:%d", returnCode);
			return -1;
		}
	}

	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			coap_p->hdr, coap_p->length);
	coap_delete_pdu(coap_p); //delete
	coap_p = NULL;
	if (returnCode < 0) {
		return -2;
	}
	size_t len;
	len = wilddog_receive(wilddog->socketId, &(wilddog->remoteAddr), resBuf,
			sizeof(resBuf), WILDDOG_RECV_TIMEOUT);
//parse
	coap_pdu_t * receive_p = coap_new_pdu();
	returnCode = coap_pdu_parse(resBuf, len, receive_p);

	if (returnCode <= 0) {
		WD_ERROR("coap_pdu_parse error:%d", returnCode);
		coap_delete_pdu(receive_p);
		return -3;
	}

	//printBuf((unsigned char*) receive_p->hdr, receive_p->length);
//get Code
	unsigned int statusCode = codeToInt(receive_p->hdr->code);


	if ((statusCode / 100) == 2) {
		//OK
		returnCode = 0;
	} else {

		returnCode = 0 - statusCode;
	}
	if (returnCode < 0) {

		unsigned char* resultPtr;
		size_t bufSize;
		char errBuf[100];

		if (coap_get_data(receive_p, &bufSize, &resultPtr)) {
			strncpy(errBuf, resultPtr,bufSize);
		}
		WD_ERROR("wilddog delete error:%d,%s", statusCode, resultPtr);

	}
	coap_delete_pdu(receive_p);
	return returnCode;

}

wilddog_t* wilddog_init(char* appid, char* path,char* token) {

	wilddog_t* res = malloc(sizeof(wilddog_t));
	memset(res, 0, sizeof(res));
	res->appid = appid;
	res->path=path;
	res->token = token;
	res->remoteAddr.ip = res->serverIp;
	res->remoteAddr.port = WILDDOG_SERVER_PORT;
	char host[strlen(appid) + strlen(WILDDOG_HOST_SUFFIX) + 1];
	strcpy(host, appid);
	strcat(host, WILDDOG_HOST_SUFFIX);
	if (wilddog_gethostbyname(res->serverIp, host) < 0) {
		free(res);
		return NULL;

	}
	return res;
}


int wilddog_query(wilddog_t* wilddog) {
	char buf[WILDDOG_BUF_SIZE];

	int res=wilddog_get_raw(wilddog,buf,sizeof(buf));
	if(res<0){
		return res;
	}
	cJSON* data=cJSON_Parse(buf);
	if(wilddog->data!=NULL){
		cJSON_Delete(wilddog->data);
		wilddog->data=data;
	}
	return 0;

}
int wilddog_set(wilddog_t* wilddog,cJSON* data){
	char* buf=cJSON_Print(data);
	size_t buflen=strlen(buf);
	int res=wilddog_put_raw(wilddog,buf,buflen);
	free(buf);
	if(res<0){
		return res;
	}
	if(wilddog->data!=NULL){
		cJSON_Delete(wilddog->data);
		wilddog->data=cJSON_Duplicate(data,1);
	}
}
int wilddog_push(wilddog_t* wilddog,cJSON* data){
	char resultBuf[64];
	char* buf=cJSON_Print(data);
	size_t buflen=strlen(buf);
	int res=wilddog_post_raw(wilddog,buf,buflen,resultBuf,sizeof(resultBuf));
	free(buf);
	if(res<0){
		return res;
	}
	cJSON* res=cJSON_Parse(resultBuf);
	if(!res){
		return -10;
	}
	cJSON* newKeyJson=cJSON_GetObjectItem(res,"newKey");
	if(newKeyJson==NULL){
		cJSON_Delete(res);
		return -11;
	}
	char* newKey=newKeyJson->valuestring;
	if(newKey==NULL||strlen(newKey)==0){
		cJSON_Delete(res);
		return -12;
	}
	if(wilddog->data==NULL){
		wilddog->data=cJSON_CreateObject();

	}
	cJSON* copy=cJSON_Duplicate(data,1);
	cJSON_AddItemToObject(wilddog->data,newKey,copy);
	wilddog->newChild=copy;
	return 0;
}
int wilddog_delete(wilddog_t* wilddog){
	int res=wilddog_delete_raw(wilddog);
	if(res<0){
		return res;
	}
	if(wilddog->data!=NULL){
		cJSON_Delete(wilddog->data);
		wilddog->data=NULL;
	}
	return 0;

}


int wilddog_observe(wilddog_t* wilddog, char* path, wilddog_query_t* query,
		char* resultBuffer, size_t max) {
	return 0;

}

int wilddog_waitNotice(wilddog_t* wilddog, char* buffer, size_t length) {

	return 0;
}

int wilddog_stopObserve(wilddog_t* wilddog) {
	return 0;
}

int wilddog_destroy(wilddog_t* wilddog) {
	if (wilddog) {
		//free data
		if(wilddog->data){
			cJSON_Delete(wilddog->data);
			wilddog->data=NULL;
		}
	    free(wilddog);
		wilddog = NULL;
	}
	return 0;

}

int wilddog_dump(wilddog_t* wilddog, char * buffer, size_t len) {
	snprintf(buffer, len,
			"[\nappid:%s\ntoken:%s\nip:%s\nport:%u\nsocketId:%d\n]\n",
			wilddog->appid, wilddog->token, wilddog->serverIp,
			wilddog->remoteAddr.port, wilddog->socketId);
}

