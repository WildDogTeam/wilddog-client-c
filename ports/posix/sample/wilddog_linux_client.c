/*
 * main.c
 *
 *  Created on: 2015年3月13日
 *      Author: x
 */

#include <stdio.h>
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
//cmd post|get|put|observe appid -p path -t token -d data
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
	int argsDone=0;
	while ((opt = getopt(argc, argv, "p:t:d:")) != -1) {
		switch (opt) {
		case 'p':
			strcpy(path, (const char*)optarg);
			break;
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
			argsDone=1;
		}

	}
	if(!argsDone){
		printf("Usage: cmd post|get|put|observe appid -p path -t token -d data \n");
		return 0;
	}

	wilddog_t* client= wilddog_init(appid,path,token);
	wilddog_dump(client,toPrint,sizeof(toPrint));
	printf("%s",toPrint);
	switch (type) {
		case 2:

			break;
		case 3:

			break;
		case 4:

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
