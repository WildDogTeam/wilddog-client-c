/*
 * main.c
 *
 *  Created on: 2015年3月13日
 *      Author: x
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "Wilddog.h"

char toPrint[1024];

void onQueryComplete(wilddog_t* wilddog,int handle,int err){
	wilddog_dump(wilddog,toPrint,sizeof(toPrint));
	printf("%s",toPrint);
	if(err){
		printf("query error:%d\n",err);
	}
	else{
		printf("result:\n%s\n",cJSON_Print(wilddog->data));
	}
}
void onData(wilddog_t* wilddog,cJSON* value){
	char * data=cJSON_Print(value);
	printf("new data: %s \n",data);
	free(data);
}


void onSetComplete(wilddog_t* wilddog,int handle,int err){
	wilddog_dump(wilddog,toPrint,sizeof(toPrint));
	printf("aaaaaaa%s",toPrint);
	if(err){
		printf("set error:%d\n",err);
	}
	else{
		printf("result:\n%s\n",cJSON_Print(wilddog->data));
	}
}
//cmd post|get|put|observe appid -p path -t token -d data
int main(int argc, char **argv) {

#ifdef 	WORDS_BIGENDIAN
	printf("WORDS_BIGENDIAN \n");
#endif
	char url[1024];
	char dataInput[1024];
	char token[32];
	memset(url,0,sizeof(url));
	memset(dataInput,0,sizeof(dataInput));
	memset(token,0,sizeof(token));
	cJSON* data;
	int type=1;
	int opt,i;
	int argsDone=0;
	while ((opt = getopt(argc, argv, "t:d:")) != -1) {
		switch (opt) {
		case 't':
			strcpy(token, (const char*)optarg);
			break;
		case 'd':
			strcpy(dataInput, (const char*)optarg);
			printf("test:%s",optarg);
			break;
		}
	}

	for (i = 0; optind < argc; i++, optind++) {
		printf("operation: %s ",argv[optind]);
		if(i==0){
			if(strcmp(argv[optind],"set")==0){
				type=2;
			}
			else if(strcmp(argv[optind],"push")==0){
				type=3;
			}
			else if(strcmp(argv[optind],"delete")==0){
				type=4;
			}
			else if(strcmp(argv[optind],"observe")==0){
				type=5;

			}

		}
		if(i==1){
			strcpy(url,argv[optind]);
			argsDone=1;
		}

	}
	if(!argsDone){
		printf("Usage: cmd set|push|get|observe url -t token -d data \n");
		return 0;
	}

	wilddog_t* client= wilddog_new(url);

	if(!client){
		printf("can't connect to server \n");
		return 0;
	}
	if(strlen(token)>0){
		wilddog_setAuth(client,token);
	}
	wilddog_dump(client,toPrint,sizeof(toPrint));
	printf("%s",toPrint);
	switch (type) {
		case 2:
			//set
			data=cJSON_Parse(dataInput);
			wilddog_set(client,data,onSetComplete);
			break;
		case 3:
			//push
			data=cJSON_Parse(dataInput);
			wilddog_push(client,data,onSetComplete);
			break;
		case 4:
			//delete
			wilddog_delete(client,onSetComplete);
			break;
		case 5:
			//observe
			wilddog_on(client,onData,onQueryComplete);
			break;
		default:
			wilddog_query(client,onQueryComplete);
			break;
	}
	while(1){
		wilddog_trySync(client);

	}

	wilddog_destroy(client);
	return 0;
}
