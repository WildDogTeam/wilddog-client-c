/*
 * main.c
 *
 *  Created on: 2015年3月13日
 *      Author: x
 */

#include <stdio.h>
#include <unistd.h>
#include "Wilddog.h"


void showGet(wilddog_t* client,char* path) {
	printf("GET %s %s \n",client->appid,path);
	char buffer[100];
	int result=wilddog_get(client,path,NULL,buffer,sizeof(buffer));
	if(result>=0){
		printf("result:\n %s \n",buffer);
	}
	else{
		printf("error:%d\n",result);
	}
}
void showPut(wilddog_t* client,char* path,char* data,size_t len) {
	printf("PUT %s %s d:%s\n",client->appid,path,data);

	int result=wilddog_put(client,path,data,len);

	if(result>=0){
		printf("put success\n");
	}
	else{
		printf("error:%d\n",result);
	}
}
void showDelete(wilddog_t* client,char* path) {
	printf("DELETE %s %s \n",client->appid,path,data);

	int result=wilddog_put(client,path);

	if(result>=0){
		printf("delete success\n");
	}
	else{
		printf("error:%d\n",result);
	}
}
void showPost(wilddog_t* client,char* path,char* data,size_t len) {
	printf("POST %s %s d:%s\n",client->appid,path,data);
	unsigned char resultBuf[32];
	int result=wilddog_post(client,path,data,len,resultBuf,sizeof(resultBuf));

	if(result>=0){
		printf("post success\n%s\n",resultBuf);
	}
	else{
		printf("error:%d\n",result);
	}
}

//cmd post|get|put|observe appid -p path -t token
int main(int argc, char **argv) {

#ifdef 	WORDS_BIGENDIAN
	printf("WORDS_BIGENDIAN \n");
#endif
	char path[100];
	char token[32];
	char appid[32];
	char dataInput[100];
	int type=1;
	int opt,i;
	while ((opt = getopt(argc, argv, "p:t:d:")) != -1) {
		switch (opt) {
		case 'p':
			strcpy(path, optarg);
			break;
		case 't':
			strcpy(token, optarg);
			break;
		case 'd':
			strcpy(dataInput, optarg);
			printf("test:%s",optarg);
			break;
		}
	}

	for (i = 0; optind < argc; i++, optind++) {
		if(i==0){
			if(strcmp(argv[optind],"post")==0){
				type=2;
			}
			else if(strcmp(argv[optind],"put")==0){
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
			strcpy(appid,argv[optind]);
		}

	}
	char toPrint[200];
	wilddog_t* client= wilddog_init(appid,token);
	wilddog_dump(client,toPrint,sizeof(toPrint));
	printf(toPrint);
	switch (type) {
		case 2:
			showPost(client,path,dataInput,strlen(dataInput));
			break;
		case 3:
			showPut(client,path,dataInput,strlen(dataInput));
			break;
		case 4:
			showDelete(client,path);
			break;
		default:
			showGet(client,path);
			break;
	}


	wilddog_destroy(client);

}
