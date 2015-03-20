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
#include "cJSON.h"
#include "utlist.h"
#include "url_parser.h"

char resBuf[WILDDOG_BUF_SIZE];
int wilddog_handle = 0;
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
inline unsigned char nextToken(wilddog_t* wilddog) {
	wilddog->token += 1;
	return (unsigned char) (wilddog->token && (0xFF));
}
void wilddog_delete_request(request_t* req) {
	if (req->coap_msg) {
		coap_delete_pdu(req->coap_msg);
	}
	free(req);
}

#ifndef SYNC_TIME
#define SYNC_TIME
void syncTime(wilddog_t* wilddog) {
	wilddog->timestamp += (1000 / WILDDOG_LOOP_PER_SECOND);
}

#endif

void _addOption(coap_pdu_t* coap_p, char * host, char* path, char* auth,
		char* observe) {
	//add host
	coap_add_option(coap_p, 3, strlen(host), host);
	//add observe
	if (observe) {
		coap_add_option(coap_p, 6, 1, observe);
	}

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
	if (auth != NULL) {
		char tokenQuery[strlen(auth) + 20];
		strcpy(tokenQuery, "token=");
		strcat(tokenQuery, auth);
		coap_add_option(coap_p, 15, strlen(tokenQuery), tokenQuery);
	}
}

#define CHILDADDED 1;
#define CHILDREMOVED 2;
#define CHILDCHANGED 4;

unsigned int diff(cJSON* old, cJSON* new) {
	unsigned int res = 0;
	if (old && new) {
		//not new and not old
		if (old->type == new->type) {
			//same type
			if (old->type == cJSON_Object) {
				//object
				if (old->child != NULL) {
					cJSON* oldChild = old->child;
					if (new->child != NULL) {
						//old has child new has too
						cJSON* newChild = new->child;
						cJSON* curOld;
						cJSON* curNew;
						for (curOld = oldChild; curOld->next != NULL; curOld =
								curOld->next) {
							curNew = cJSON_GetObjectItem(new, curOld->string);
							if (curNew != NULL) {
								//same key item
								unsigned int childRes = diff(curOld, curNew);
								if (childRes > 0) {
									res |= CHILDCHANGED
									;
								}

							} else {
								//deleted
								res |= CHILDREMOVED
								;
							}

						}
						for (curNew = newChild; curNew->next != NULL; curNew =
								curNew->next) {
							curOld = cJSON_GetObjectItem(old, curNew->string);
							if (curOld == NULL) {
								//new child
								res |= CHILDADDED
								;
							} else { //has processed
								continue;
							}

						}

					} else {
						//old has child new not
						res |= CHILDREMOVED
						;

					}

				}
			} else {
				//not object no array allowed
				if (old->type == new->type) {
					if (old->type == cJSON_Number) {
						if (old->valueint == new->valueint) {

						} else {
							res |= CHILDCHANGED
							;
						}

					} else if (old->type == cJSON_String) {
						if (strcmp(old->valuestring, new->valuestring) == 0) {
							//same
						} else {
							res |= CHILDCHANGED
							;
						}
					}
				} else {
					res |= CHILDCHANGED
					;
				}
			}
		} else {
			//old type not eq new type
			res |= CHILDCHANGED
			;
		}
	} else {
		if (old) {
			res |= CHILDREMOVED
			;
		} else if (new) {
			res |= CHILDADDED
			;
		} else {
			// both null
		}
	}
	return res;
}

void onAck(wilddog_t* wilddog, int handle, onCompleteFunc callback,
		coap_pdu_t* req, coap_pdu_t* resp) {
	int errorCode = 0;
	WD_DEBUG("received ack code:%d ", codeToInt(resp->hdr->code));
	if (codeToInt(resp->hdr->code) / 100 != 2) {
		//TODO handle err
		WD_ERROR("return code:%d", codeToInt(resp->hdr->code));
		errorCode = -codeToInt(resp->hdr->code);
		if (callback)
			callback(wilddog, handle, errorCode);

	} else {
		unsigned char* respData;
		size_t respSize;
		unsigned char* reqData;
		size_t reqSize;
		if (!coap_get_data(resp, &respSize, &respData)) {
			//WD_DEBUG("coap_get_data from resp error");
		}
		if (!coap_get_data(req, &reqSize, &reqData)) {
			//TODO
			//WD_DEBUG("coap_get_data from req error");
		}

		char respStr[respSize + 1]; //for safety
		strncpy(respStr, respData, respSize);
		respStr[respSize] = 0;

		char reqStr[reqSize + 1]; //for safety
		strncpy(reqStr, reqData, reqSize);
		reqStr[reqSize] = 0;
		unsigned _diff = 0;
		if (req->hdr->code == 1) {
			//GET
			//get payload and replace current data

			cJSON* newData = cJSON_Parse(respStr);
			if (newData == NULL) {
				WD_DEBUG("cjson parse resp error");
				if (callback) {
					callback(wilddog, handle, WILDDOG_CODE_JSON_PARSE_ERROR);
				}
				return;
			}
			if (wilddog->onData) {
				_diff = diff(wilddog->data, newData);
			}
			if (wilddog->data != NULL) {
				cJSON_Delete(wilddog->data);
				wilddog->data = NULL;
			}
			wilddog->data = newData;
			if (_diff) {
				wilddog->onData(wilddog, wilddog->data);
			}
			if (callback)
				callback(wilddog, handle, errorCode);

		} else if (req->hdr->code == 2) {
			//post
			//get key from response
			cJSON* res = cJSON_Parse(respStr);
			if (!res) {
				WD_ERROR("cjson parsing error");
				if (callback) {

					callback(wilddog, handle, WILDDOG_CODE_JSON_PARSE_ERROR);
				}
				return;
			}
			cJSON* childNode = cJSON_Parse(reqStr);
			if (!childNode) {
				WD_ERROR("cjson parsing error");
				if (callback) {
					callback(wilddog, handle, WILDDOG_CODE_JSON_PARSE_ERROR);
				}
				cJSON_Delete(res);
				return;
			}
			cJSON* keyNode = cJSON_GetObjectItem(res, "newkey");

			if (!keyNode) {
				WD_ERROR("no new key returned");
				if (callback) {
					callback(wilddog, handle, WILDDOG_CODE_NO_NEWKEY_RETURNED);
				}
				cJSON_Delete(res);
				cJSON_Delete(childNode);
				return;

			}
			if (wilddog->data == NULL) {
				wilddog->data = cJSON_CreateObject();
				if (wilddog->data == NULL) {
					if (callback)
						callback(wilddog, handle, WILDDOG_CODE_MALLOC_ERROR);
					cJSON_Delete(res);
					cJSON_Delete(childNode);
					return;
				}
			}
			cJSON_AddItemToObject(wilddog->data, keyNode->valuestring,
					childNode);
			cJSON_Delete(res);

			if (_diff) {
				wilddog->onData(wilddog, wilddog->data);
			}
			if (callback) {
				callback(wilddog, handle, errorCode);
			}
		} else if (req->hdr->code == 3) {
			//put

			cJSON* newData = cJSON_Parse(reqStr);
			if (!newData) {
				WD_ERROR("cjson parsing error");
				if (callback)
					callback(wilddog, handle, WILDDOG_CODE_JSON_PARSE_ERROR);
				return;
			}

			if (wilddog->onData) {
				_diff = diff(wilddog->data, newData);
			}
			if (wilddog->data != NULL) {
				cJSON_Delete(wilddog->data);
				wilddog->data = NULL;
			}
			wilddog->data = newData;
			if (_diff) {
				wilddog->onData(wilddog, wilddog->data);
			}
			if (callback)
				callback(wilddog, handle, errorCode);

		} else if (req->hdr->code == 4) {
			//delete
			_diff = diff(wilddog->data, NULL);
			if (_diff) {
				wilddog->onData(wilddog, wilddog->data);
			}
			if (wilddog->data != NULL) {
				cJSON_Delete(wilddog->data);
				wilddog->data = NULL;
			}

			if (callback)
				callback(wilddog, handle, errorCode);
		}
	}
}
void onTimeout(wilddog_t* wilddog, request_t* req) {
	int code = WILDDOG_CODE_TIMEOUT
	;
	req->callback(wilddog, req->handle, code);
}

void wilddog_addSent(wilddog_t* wilddog, request_t* request) {
	LL_APPEND(wilddog->sentQueue, request);

}
void wilddog_queueAge(wilddog_t* wilddog) {
	request_t* tmp;
	request_t* curr;

	LL_FOREACH_SAFE(wilddog->sentQueue,curr,tmp)
	{

		if ((curr->maxAge
				&& (curr->maxAge + curr->timestamp) < wilddog->timestamp)) {

			LL_DELETE(wilddog->sentQueue, curr);
			if (curr) {

				onTimeout(wilddog, curr);
				wilddog_delete_request(curr);
				curr = NULL;
			}
		}
	}
}
void wilddog_handleResp(wilddog_t* wilddog, coap_pdu_t* resp) {
	unsigned int id = resp->hdr->id;
	char* token = resp->hdr->token;
	size_t tkl = resp->hdr->token_length;
	request_t* tmp;
	request_t* curr;
	int processed = 0;
	LL_FOREACH_SAFE(wilddog->sentQueue,curr,tmp)
	{
		if (curr && curr->coap_msg->hdr->id == id) {
			if (resp->hdr->type == 2) {
				onAck(wilddog, curr->handle, curr->callback, curr->coap_msg,
						resp);
				processed = 1;
				if (curr->flag && 0x01) {
					//do not remove from queue
				} else {

					LL_DELETE(wilddog->sentQueue, curr);
					if (curr) {
						coap_delete_pdu(curr->coap_msg);
						free(curr);
						curr = NULL;
					}
				}
			}

		} else if (tkl > 0) {
			if (curr->flag | 0x01) {
				//observe
				if (tkl == curr->coap_msg->hdr->token_length) {
					char* currToken = curr->coap_msg->hdr->token;
					//find if token eql
					int dif = 0;
					int i = 0;
					for (; i < tkl; i++) {
						if (token[i] != currToken[i]) {
							dif = 1;
						}
					}
					if (dif == 0) {
						// token eql,this is a notify
						onAck(wilddog, curr->handle, NULL, curr->coap_msg,
								resp);
						if (curr->coap_msg->hdr->type == 0) {
							//confirmable send ack
							wilddog_sendAck(wilddog, resp);

						}
						processed = 1;

					}
				}

			}

		}

	}
	if (!processed) {
		//client don't remeber the message id send rst

	}
}
int wilddog_socketReconnect(wilddog_t* wilddog) {
	//if socket not created,create
	int returnCode = 0;
	if (wilddog->socketId == 0) {
		returnCode = wilddog_openSocket(&(wilddog->socketId));
		if (returnCode < 0) {
			WD_ERROR("wilddog_openSocket error:%d", returnCode);
			return WILDDOG_CODE_NO_ALIVE_SOCKET;
		}
	}
	return 0;
}

int wilddog_sendGet(wilddog_t* wilddog, int handle, void* callback) {
	int returnCode;
//coap pack
	coap_pdu_t* coap_p = coap_pdu_init(0, 1, htons(++wilddog->msgId),
	COAP_MAX_SIZE);
	_addOption(coap_p, wilddog->url->host, wilddog->url->path, wilddog->auth, NULL);
	if ((returnCode = wilddog_socketReconnect(wilddog)) < 0) {
		coap_delete_pdu(coap_p); //delete
		coap_p = NULL;
		return returnCode;
	}
	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			coap_p->hdr, coap_p->length);
	wilddog->lastSend = wilddog->timestamp;
	if (returnCode < 0) {
		coap_delete_pdu(coap_p);
		return WILDDOG_CODE_SENDERROR;
	}
	request_t* request = malloc(sizeof(request_t));
	memset(request, 0, sizeof(request_t));
	request->coap_msg = coap_p;
	request->callback = callback;
	request->handle = handle;
	request->timestamp = wilddog->timestamp;
	request->maxAge = WILDDOG_RECV_TIMEOUT;
	wilddog_addSent(wilddog, request);
	return 0;
}
int wilddog_sendPost(wilddog_t* wilddog, char* buffer, size_t length,
		int handle, void* callback) {
	int returnCode;
//coap pack
	coap_pdu_t* coap_p = coap_pdu_init(0, 2, htons(++wilddog->msgId),
	COAP_MAX_SIZE);
	_addOption(coap_p, wilddog->url->host, wilddog->url->path, wilddog->auth, NULL);
	coap_add_data(coap_p, length, buffer);
	if ((returnCode = wilddog_socketReconnect(wilddog)) < 0) {
		coap_delete_pdu(coap_p); //delete
		coap_p = NULL;
		return returnCode;
	}
	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			coap_p->hdr, coap_p->length);
	wilddog->lastSend = wilddog->timestamp;
	if (returnCode < 0) {
		coap_delete_pdu(coap_p);
		return WILDDOG_CODE_SENDERROR;
	}

	request_t* request = malloc(sizeof(request_t));
	memset(request, 0, sizeof(request_t));
	request->coap_msg = coap_p;
	request->callback = callback;
	request->handle = handle;
	request->timestamp = wilddog->timestamp;
	request->maxAge = WILDDOG_RECV_TIMEOUT;
	wilddog_addSent(wilddog, request);
	return returnCode;
}
int wilddog_sendPut(wilddog_t* wilddog, char* buffer, size_t length, int handle,
		void* callback) {

	int returnCode;
	//coap pack
	coap_pdu_t* coap_p = coap_pdu_init(0, 3, htons(++wilddog->msgId),
	COAP_MAX_SIZE);
	_addOption(coap_p, wilddog->url->host, wilddog->url->path, wilddog->auth, NULL);
	coap_add_data(coap_p, length, buffer);

	if ((returnCode = wilddog_socketReconnect(wilddog)) < 0) {
		coap_delete_pdu(coap_p); //delete
		coap_p = NULL;
		return returnCode;
	}
	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			coap_p->hdr, coap_p->length);

	if (returnCode < 0) {
		coap_delete_pdu(coap_p); //delete
		return WILDDOG_CODE_SENDERROR;
	}
	request_t* request = malloc(sizeof(request_t));
	memset(request, 0, sizeof(request_t));
	request->coap_msg = coap_p;
	request->callback = callback;
	request->handle = handle;
	request->timestamp = wilddog->timestamp;
	request->maxAge = WILDDOG_RECV_TIMEOUT;
	wilddog_addSent(wilddog, request);
	return returnCode;
}

int wilddog_sendDelete(wilddog_t* wilddog, int handle, void* callback) {

	int returnCode;
	//coap pack
	coap_pdu_t* coap_p = coap_pdu_init(0, 4, htons(++wilddog->msgId),
	COAP_MAX_SIZE);
	_addOption(coap_p, wilddog->url->host, wilddog->url->path, wilddog->auth, NULL);
	//if socket not created,create
	if ((returnCode = wilddog_socketReconnect(wilddog)) < 0) {
		coap_delete_pdu(coap_p); //delete
		coap_p = NULL;
		return returnCode;
	}

	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			coap_p->hdr, coap_p->length);

	if (returnCode < 0) {
		coap_delete_pdu(coap_p); //delete
		return WILDDOG_CODE_SENDERROR;
	}
	request_t* request = malloc(sizeof(request_t));
	memset(request, 0, sizeof(request_t));
	request->coap_msg = coap_p;
	request->callback = callback;
	request->handle = handle;
	request->timestamp = wilddog->timestamp;
	request->maxAge = WILDDOG_RECV_TIMEOUT;
	wilddog_addSent(wilddog, request);
	return returnCode;

}

int wilddog_sendObserve(wilddog_t* wilddog, int handle, void* callback) {

	int returnCode;
//coap pack
	coap_pdu_t* coap_p = coap_pdu_init(0, 1, htons(++wilddog->msgId),
	COAP_MAX_SIZE);
	unsigned char token = nextToken(wilddog);
	coap_add_token(coap_p, 1, &token);
	unsigned char observeid = 0;
	_addOption(coap_p, wilddog->url->host, wilddog->url->path, wilddog->auth,
			&observeid);

	//if socket not created,create
	if ((returnCode = wilddog_socketReconnect(wilddog)) < 0) {
		coap_delete_pdu(coap_p); //delete
		coap_p = NULL;
		return returnCode;
	}
	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			coap_p->hdr, coap_p->length);
	wilddog->lastSend = wilddog->timestamp;
	if (returnCode < 0) {
		coap_delete_pdu(coap_p);
		return WILDDOG_CODE_SENDERROR;
	}
	request_t* request = malloc(sizeof(request_t));
	memset(request, 0, sizeof(request_t));
	request->coap_msg = coap_p;
	request->callback = callback;
	request->handle = handle;
	request->timestamp = wilddog->timestamp;
	request->maxAge = WILDDOG_OBSERVE_TIMEOUT;
	request->flag |= 0x01;
	wilddog_addSent(wilddog, request);
	return 0;

}
int wilddog_sendRstToObserve(wilddog_t* wilddog, request_t* request) {
	int returnCode = 0;
	coap_pdu_t* toSend = coap_pdu_init(3, 1, htons(++wilddog->msgId),
	COAP_MAX_SIZE);
	if (toSend == NULL) {
		WD_ERROR("coap_addToken error");
		return WILDDOG_CODE_COAP_TOBUF_ERROR;
	}
	coap_pdu_t* coap_p = request->coap_msg;
	size_t tkn = coap_p->hdr->token_length;
	unsigned char* tk = coap_p->hdr->token;
	if (!coap_add_token(toSend, tkn, tk)) {
		WD_ERROR("coap_addToken error");
		coap_delete_pdu(toSend);
		return WILDDOG_CODE_COAP_TOBUF_ERROR;
	}

	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			toSend->hdr, toSend->length);
	coap_delete_pdu(toSend);
	return returnCode;
}

int wilddog_sendPing(wilddog_t* wilddog) {
	int returnCode = 0;
	coap_pdu_t* toSend = coap_pdu_init(0, 1, htons(++wilddog->msgId),
	COAP_MAX_SIZE);
	if (toSend == NULL) {
		WD_ERROR("coap_addToken error");
		return WILDDOG_CODE_COAP_TOBUF_ERROR;
	}
	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			toSend->hdr, toSend->length);
	coap_delete_pdu(toSend);

	return returnCode;

}
int wilddog_sendAck(wilddog_t* wilddog, coap_pdu_t* resp) {
	int returnCode;
	unsigned int id = resp->hdr->id;
	size_t tkl = resp->hdr->token_length;
	char* tk = resp->hdr->token;
	coap_pdu_t* toSend = coap_pdu_init(2, 0, id, COAP_MAX_SIZE);
	if (toSend == NULL) {
		WD_ERROR("coap_addToken error");
		return WILDDOG_CODE_COAP_TOBUF_ERROR;
	}
	returnCode = wilddog_send(wilddog->socketId, &(wilddog->remoteAddr),
			toSend->hdr, toSend->length);
	coap_delete_pdu(toSend);

	return returnCode;

}


wilddog_t* wilddog_new(char* url) {

	struct parsed_url* url_st=parse_url(url);
	char h[strlen(url_st->host)+1];
	wilddog_t* res = malloc(sizeof(wilddog_t));
	memset(res, 0, sizeof(res));
	res->url=url_st;
	res->remoteAddr.port=WILDDOG_SERVER_PORT;
	res->msgId = 1;
	res->token = 1;
	res->connected = 1;

	if (wilddog_gethostbyname(&res->remoteAddr, res->url->host) < 0) {
		free(res);
		return NULL;
	}
	syncTime(res);
	return res;
}
void wilddog_setAuth(wilddog_t* wilddog, unsigned char* auth) {
	wilddog->auth = auth;
}
int wilddog_query(wilddog_t* wilddog, onCompleteFunc callback) {
	int handle = ++wilddog_handle;
	int res = wilddog_sendGet(wilddog, handle, callback);
	if (res < 0) {
		return res;
	}
	return handle;
}
int wilddog_set(wilddog_t* wilddog, cJSON* data, onCompleteFunc callback) {
	int handle = ++wilddog_handle;
	char * dataToSend = cJSON_Print(data);
	if (dataToSend == NULL) {
		return WILDDOG_CODE_MALLOC_ERROR;
	}
	int res = wilddog_sendPut(wilddog, dataToSend, strlen(dataToSend), handle,
			callback);
	free(dataToSend);
	dataToSend = NULL;
	if (res < 0) {
		return res;
	}
	return handle;
}
int wilddog_push(wilddog_t* wilddog, cJSON* data, onCompleteFunc callback) {
	int handle = ++wilddog_handle;
	char * dataToSend = cJSON_Print(data);
	if (dataToSend == NULL) {
		return WILDDOG_CODE_MALLOC_ERROR;
	}
	int res = wilddog_sendPost(wilddog, dataToSend, strlen(dataToSend), handle,
			callback);
	free(dataToSend);
	dataToSend = NULL;
	if (res < 0) {
		return res;
	}
	return handle;
}
int wilddog_delete(wilddog_t* wilddog, onCompleteFunc callback) {
	int handle = ++wilddog_handle;
	int res = wilddog_sendDelete(wilddog, handle, callback);
	if (res < 0) {
		return res;
	}
	return handle;
}

int wilddog_on(wilddog_t* wilddog, onDataFunc onDataChange,
		onCompleteFunc callback) {
	int handle = ++wilddog_handle;
	int res = wilddog_sendObserve(wilddog, handle, callback);
	if (res < 0) {
		return res;
	}
	wilddog->onData = onDataChange;
	wilddog->observeHandle = handle;

	return handle;
}

int wilddog_off(wilddog_t* wilddog) {
	request_t* head = wilddog->sentQueue;
	request_t* tmp;
	request_t* cur;
	LL_FOREACH_SAFE(head,cur,tmp)
	{
		//delete observe
		if (cur->flag | 0x01) {
			wilddog_sendRstToObserve(wilddog, cur);
			LL_DELETE(head, cur);
			wilddog_delete_request(cur);
		}
	}
	wilddog->onData = NULL;

	return 0;
}

int wilddog_trySync(wilddog_t* wilddog) {
	int returnCode = 0;
	char buf[WILDDOG_BUF_SIZE];
	int receiveSize;
//sync time
	syncTime(wilddog);

	//WD_DEBUG("test try synctimestamp:%u,lastsend:%u,lastreceive:%u",wilddog->timestamp,wilddog->lastSend,wilddog->lastReceive);
	if (wilddog->timestamp - wilddog->lastReceive > WILDDOG_RECONNECT_THRESHOLD) {
		wilddog->connected = 0;
		wilddog_socketReconnect(wilddog);
		wilddog->lastReceive = wilddog->timestamp;

	}

	if (wilddog->timestamp - wilddog->lastSend > WILDDOG_PING_INTV) {
		//send ping
		wilddog->lastSend = wilddog->timestamp;
		if (wilddog_sendPing(wilddog) < 0) {
			WD_DEBUG("send ping error");
		}

	}
	if ((receiveSize = wilddog_receive(wilddog->socketId, &wilddog->remoteAddr,
			buf, sizeof(buf))) > 0) {

		if (wilddog->connected == 0) {
			if (wilddog->onData) {
				//wilddog_sendObserve(wilddog, wilddog->observeHandle,wilddog->onData);
				WD_DEBUG("onData");
			}
			wilddog->connected = 1;
		}

		coap_pdu_t* resP = coap_new_pdu();
		if (coap_pdu_parse(buf, (size_t) receiveSize, resP)) {
			//success
			wilddog->lastReceive = wilddog->timestamp;

			wilddog_handleResp(wilddog, resP);

		} else {
			//error
			returnCode = -1;
		}
		coap_delete_pdu(resP);
		resP = NULL;
	}
	wilddog_queueAge(wilddog);

	return returnCode;

}

int wilddog_destroy(wilddog_t* wilddog) {
	if (wilddog) {
		if(wilddog->url){
			parsed_url_free(wilddog->url);
		}
		if (wilddog->data) {
			cJSON_Delete(wilddog->data);
			wilddog->data = NULL;
		}
		if (wilddog->sentQueue) {
			request_t* cur;
			request_t* tmp;
			LL_FOREACH_SAFE(wilddog->sentQueue,cur,tmp)
			{
				coap_delete_pdu(cur->coap_msg);
				LL_DELETE(wilddog->sentQueue, cur);
			}
		}
		if (wilddog->socketId) {
			wilddog_closeSocket(wilddog->socketId);
		}
		free(wilddog);
		wilddog = NULL;
	}
	return 0;

}



