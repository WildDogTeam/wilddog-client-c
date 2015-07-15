/*
*	get test file
*
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "wilddog_debug.h"
#include "wilddog_api.h"

#define TEST_URL_HD		"coap://"
#define TEST_URL_END	".wilddogio.com"		
#define TEST_GET_URL "coap://test1234.wilddogio.com/"
#define QUERY_CNT  100

typedef enum _TEST_CMD_TYPE
{
	TEST_CMD_NON = 0,
	TEST_CMD_GET,
	TEST_CMD_SET,
	TEST_CMD_PUSH,
	TEST_CMD_DELE,
	TEST_CMD_ON,
	
}TEST_CMD_TYPE;
/*
STATIC void test_onAuth(void* arg, Wilddog_Return_T err)
{
	if(err < 0 || err > 400)
	{
		printf("auth fail!\n");
		return;
	}
	printf("auth success!\n");
 	*(u32*)arg = 1;	 
	return;    
}
*/
static void test_onQueryFunc(const Wilddog_Node_T* p_snapshot, 
						void* arg, Wilddog_Return_T err)
{
	if(err < 0 || err >= 400)
	{
		wilddog_debug("query fail!");
		return;
	}
	*(BOOL*)arg = TRUE;

	if(p_snapshot)
		wilddog_debug_printnode(p_snapshot);
	printf("\nquery success!\n");

	return;
}

STATIC void test_onDeleteFunc(void* arg, Wilddog_Return_T err)
{
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("delete failed!");
		return;
	}
	wilddog_debug("delete success!");
	*(BOOL*)arg = TRUE;
	return;
}
STATIC void test_onSetFunc(void* arg, Wilddog_Return_T err)
{
						
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("set error!");
		return;
	}
	wilddog_debug("set success!");
	*(BOOL*)arg = TRUE;
	return;
}

STATIC void test_onPushFunc(u8 *p_path,void* arg, Wilddog_Return_T err)
{
						
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("push failed");
		return;
	}		
	wilddog_debug("new path is %s", p_path);
	*(BOOL*)arg = TRUE;
	return;
}

STATIC void test_onObserveFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg,
	Wilddog_Return_T err)
{
	
	*(BOOL*)arg = TRUE;
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("observe failed!");
		return;
	}
	wilddog_debug_printnode(p_snapshot);
		wilddog_debug("observe data!");
	
	return;
}


//cmd post|get|put|observe appid -p path -d data
int main(int argc, char **argv) {

#ifdef 	WORDS_BIGENDIAN
	printf("WORDS_BIGENDIAN \n");
#endif
	char uid[256];
	char path[256];
	char times[126];
	char url[1024];
	char value[1024];
	char keys[256];
	char *p_inputtype = NULL;
	memset(url,0,sizeof(url));	
	memset(path,0,sizeof(path));
	memset(times,0,sizeof(times));	
	memset(uid,0,sizeof(uid));
	memset(value,0,sizeof(value));
	memset(keys,0,sizeof(keys));
	int type = 0;
	int opt,i,cnt=0,cntmax=0;
	Wilddog_Return_T ret;
	int argsDone=0;
	
	ret = 0;
	BOOL isFinish = FALSE;
	Wilddog_T wilddog = 0;
	Wilddog_Node_T * p_node = NULL,*p_head = NULL;
	while ((opt = getopt(argc, argv, "i:v:k:p:n")) != -1) {
		switch (opt) {
			case 'v':
				strcpy(value, (const char*)optarg);
				printf("input data:%s\n",optarg);			
				break;
			case 'k':
				strcpy(keys, (const char*)optarg);
				printf("url:%s\n",optarg);
				argsDone++;
				break;
				
			case 'i':
				strcpy(uid, (const char*)optarg);
				printf("url:%s\n",optarg);
				argsDone++;
				break;
			case 'p':
				strcpy(path, (const char*)optarg);
				printf("url:%s\n",optarg);
				argsDone++;
				break;				
			case 'n':
					strcpy(times, (const char*)optarg);
					cntmax = atoi(times);
					printf("times:%s\n",optarg);
					break;

		}
	}

	for (i = 0; optind < argc; i++, optind++) {
		printf("argc=%d;optind=%d\n",argc,optind);
		printf("operation: %s \n",argv[optind]);
		if(i==0){
			if(strcmp(argv[optind],"get")==0){
				p_inputtype = argv[optind];
				type= TEST_CMD_GET;
				cntmax =0;
			}
			if(strcmp(argv[optind],"set")==0){
				p_inputtype = argv[optind];
				type= TEST_CMD_SET;
				cntmax =0;
			}
			else if(strcmp(argv[optind],"push")==0){
				p_inputtype = argv[optind];
				type=TEST_CMD_PUSH;
				cntmax =0;
			}
			else if(strcmp(argv[optind],"delete")==0){
				p_inputtype = argv[optind];
				type=TEST_CMD_DELE;
				cnt =0;
			}
			else if(strcmp(argv[optind],"on")==0){
				p_inputtype = argv[optind];
				type= TEST_CMD_ON;
			}

		}
		

	}
	if( argsDone <2 || !type){
		printf("Usage: cmd get|set|push|delete|on -i id -p path -d data -n cnt(only meaning to on event ) \n");
		return 0;
	}
	sprintf(url,"%s%s%s%s",TEST_URL_HD,uid,TEST_URL_END,path);
	printf("type=%d;\t url=%s;\t data=%s\n",type,url,value);

	
	wilddog_init();
	p_head = wilddog_node_createObject(NULL);
	p_node = wilddog_node_createUString((Wilddog_Str_T *)keys,(Wilddog_Str_T *)value);
	wilddog_node_add(p_head, p_node);
	wilddog = wilddog_new((Wilddog_Str_T *)url);
	switch(type)
	{
		case TEST_CMD_GET:
			wilddog_debug();
			ret = wilddog_query(wilddog, test_onQueryFunc, (void*)&isFinish);
			break;
		case TEST_CMD_SET:
			
			ret = wilddog_set(wilddog,p_head,test_onSetFunc,(void*)&isFinish);
			break;
		case TEST_CMD_PUSH:
			ret = wilddog_push(wilddog, p_head, test_onPushFunc, (void *)&isFinish);	
			break;
		case TEST_CMD_DELE:
			ret = wilddog_remove(wilddog, test_onDeleteFunc, (void*)&isFinish);
			break;
		case TEST_CMD_ON:
			ret = wilddog_on(wilddog, WD_ET_VALUECHANGE, test_onObserveFunc, (void*)&isFinish);
			break;
	}
	wilddog_node_delete(p_head);
	while(1)
		{
			if(TRUE == isFinish)
			{
				wilddog_debug("get new data %d times!", cnt++);
				isFinish = FALSE;
				if(cnt > cntmax)
				{
					printf("event :%s success\n",p_inputtype);
					if(type ==  TEST_CMD_ON)
					{
						wilddog_debug("off the data!");
						wilddog_off(wilddog, WD_ET_VALUECHANGE);
					}
					break;
				}
			}
			wilddog_trySync();
		}
	wilddog_destroy(&wilddog);

	return ret;
}
