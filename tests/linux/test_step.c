#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wilddog.h"

#include "wilddog_debug.h"
#include "wilddog_api.h"
#include "wilddog_ct.h"

#define TEST_URL_HEAD		"coap://"
#define TEST_URL_END	".wilddogio.com"		


typedef struct WILDDOG_HANDLE_T
{
	BOOL isFinished;
	Wilddog_Node_T *p_node;
}Wilddog_Handle_T;



STATIC void test_onPushFunc(u8 *p_path,void* arg, Wilddog_Return_T err)
{
						
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("push failed! err = %d",err);
		return;
	}		
	wilddog_debug("new path is %s", p_path);
	*(BOOL*)arg = TRUE;
	return;
}


STATIC void test_onDeleteFunc(void* arg, Wilddog_Return_T err)
{
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("delete failed!, err = %d",err);
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
		wilddog_debug("set error!, err = %d", err);
		return;
	}
	wilddog_debug("set success!");
	*(BOOL*)arg = TRUE;
	return;
}

STATIC void test_onQueryFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg, 
	Wilddog_Return_T err)
{
	
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("query error! err = %d", err);
		return;
	}
	wilddog_debug("query success!");
	((Wilddog_Handle_T*)arg)->isFinished = TRUE;
	if(p_snapshot)
	{
		((Wilddog_Handle_T*)arg)->p_node = wilddog_node_clone(p_snapshot);
	}
	
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
		wilddog_debug("observe failed! err = %d", err);
		return;
	}
	
	printf("*******************************************\n");
	printf("Observe the data:\n");
	wilddog_debug_printnode(p_snapshot);
	printf("\n");
	printf("*******************************************\n");
	printf("\n\n");	
	return;
}


int test(char *uid)
{
	char url[1024];

	char en_key = 0;
	BOOL isFinished = FALSE;
	Wilddog_T wilddog;
	Wilddog_Handle_T s_handle;
	Wilddog_Node_T *p_head = NULL, *p_node = NULL/*, *p_snapshot = NULL*/;
	u8 value1[5] = {246,12,0,0,6};
	/*u8 value2[4] = {23,67,98,1};*/
	wFloat f = 2.3;

	Wilddog_Node_T *root;
	Wilddog_Node_T *L1c1,*L1c2;
	Wilddog_Node_T *L2c1,*L2c2,*L2c3;
	Wilddog_Node_T *L3c1,*L3c2,*L3c3,*L3c4,*L3c5;
	Wilddog_Node_T *L4c1,*L4c2,*L4c3,*L4c4,*L4c5;

	int count = 0;	
	sprintf(url,"%s%s%s",TEST_URL_HEAD,uid,TEST_URL_END);

	memset(&s_handle, 0, sizeof(s_handle));
	s_handle.isFinished = FALSE;

	printf("*******************************************\n");
	printf("First Step: Build the data tree \n");
	printf("Please press enter key to continue!\n");
	printf("*******************************************\n");
	printf("*******************************************\n");
	printf("\n\n");
	
#ifndef WILDDOG_PORT_TYPE_WICED
	while(1)
	{
		en_key= getchar();
		if(en_key == '\n')
			break;
	}
	en_key = 0;
#endif

	root = wilddog_node_createNum((Wilddog_Str_T *)"root",9999);
	L1c1 = wilddog_node_createFalse((Wilddog_Str_T *)"L1c1");
	L1c2 = wilddog_node_createTrue((Wilddog_Str_T *)"L1c2");
	L2c1 = wilddog_node_createNum((Wilddog_Str_T *)"L2c1",-10000);
	L2c2 = wilddog_node_createFloat((Wilddog_Str_T *)"L2c2",f);
	L2c3 = wilddog_node_createTrue((Wilddog_Str_T *)"L2c3");          //true
	L3c1 = wilddog_node_createBString((Wilddog_Str_T *)"L3c1",value1,sizeof(value1)/sizeof(u8));    //BString
	L3c2 = wilddog_node_createTrue((Wilddog_Str_T *)"L3c2");
	L3c3 = wilddog_node_createTrue((Wilddog_Str_T *)"L3c3");
	L3c4 = wilddog_node_createNull((Wilddog_Str_T *)"L3c4");   //NULL
	L3c5 = wilddog_node_createTrue((Wilddog_Str_T *)"L3c5");
	L4c1 = wilddog_node_createNum((Wilddog_Str_T *)"L4c1",875);//    +Num
	L4c2 = wilddog_node_createNum((Wilddog_Str_T *)"L4c2",-5693);    //   -Num
	L4c3 = wilddog_node_createFloat((Wilddog_Str_T *)"L4c3",f);     //float
	L4c4 = wilddog_node_createFalse((Wilddog_Str_T *)"L4c4");        //false
	L4c5 = wilddog_node_createUString((Wilddog_Str_T *)"L4c5",(Wilddog_Str_T *)"string");      //UString
	
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(root,L1c1))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(root,L1c2))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L1c1,L2c1))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L1c1,L2c2))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L1c2,L2c3))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c1,L3c1))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c1,L3c2))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c2,L3c3))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c2,L3c4))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c2,L3c5))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L3c2,L4c1))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L3c3,L4c2))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L3c3,L4c3))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L3c5,L4c4))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	if(WILDDOG_ERR_NOERR != wilddog_node_addChild(L3c5,L4c5))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"node add error");
	}
	

	printf("*******************************************\n");
	printf("The data is built as below:\n");
	wilddog_debug_printnode(root);
	printf("\n");
	printf("*******************************************\n");
	printf("\n\n");


/*
	printf("*******************************************\n");
	printf("Second Step: Use the Node API to change the data tree\n");
	printf("Change the L1c1 node's key from \"L1c1\" to \"L1c1New\" \n");
	wilddog_node_setKey(L1c1,(Wilddog_Str_T *)"L1c1New");
	printf("Change the L3c1 node's value from Bytestring \"{0xf6,0x0c,0x00,0x00,0x06}\" to Bytestring \"{0x17,0x43,0x62,0x01}\"\n ");
	wilddog_node_setValue(L3c1,value2,sizeof(value2)/sizeof(u8));
	printf("Please press enter key to continue!\n");
	printf("*******************************************\n");
	printf("\n\n");

#ifndef WILDDOG_PORT_TYPE_WICED

	while(1)
	{
		en_key= getchar();
		if(en_key == '\n')
			break;
	}
	en_key = 0;
#endif

	printf("*******************************************\n");
	printf("The data tree had changed as below:\n");
	wilddog_debug_printnode(root);
	printf("\n");
	printf("*******************************************\n");
	printf("\n\n");
*/
/*
	printf("*******************************************\n");
	printf("Clone the data tree from L2c2 node\n");
	printf("Print the L2c2 data tree\n");
	p_snapshot = wilddog_node_clone(L2c2);
	wilddog_debug_printnode(p_snapshot);
	wilddog_node_delete(p_snapshot);
	printf("\n");
	printf("*******************************************\n");
	printf("\n\n");
*/

	
	/*************************************************************/
	
	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)url);

	printf("*******************************************\n");
	printf("Third step: Remove the %s's data\n",uid);
	printf("Please press enter key to continue!\n");
	printf("*******************************************\n");
	printf("\n\n");
	
#ifndef WILDDOG_PORT_TYPE_WICED

	while(1)
	{
		en_key= getchar();
		if(en_key == '\n')
			break;
	}
	en_key = 0;
#endif

	wilddog_removeValue(wilddog, test_onDeleteFunc, (void*)&isFinished);
	while(1)
	{
		if(TRUE == isFinished)
		{
			wilddog_debug("remove success!");
			break;
		}
		wilddog_trySync();
	}
	isFinished = FALSE;
	/*************************************************************/

	printf("*******************************************\n");
	printf("It has removed all data in %s!\n",uid);
	printf("Please check the data on %s page\n",url);
	printf("*******************************************\n");
	printf("\n\n");


	printf("*******************************************\n");
	printf("Fourth step: Set the root node to the %s's data tree\n",uid);
	printf("Please press enter key to continue!\n");
	printf("*******************************************\n");
	printf("\n\n");

#ifndef WILDDOG_PORT_TYPE_WICED
	
	while(1)
	{
		en_key= getchar();
		if(en_key == '\n')
			break;
	}
	en_key = 0;
#endif

	wilddog_setValue(wilddog,root,test_onSetFunc,(void*)&isFinished);
	wilddog_node_delete(root);
	
	while(1)
	{
		if(TRUE == isFinished)
		{
			wilddog_debug("set success!");
			break;
		}
		wilddog_trySync();
	}

	isFinished = FALSE;
	
	printf("*******************************************\n");
	printf("It has set the data in %s!\n",uid);
	printf("Please check the data tree on %s page\n",url);
	printf("*******************************************\n");
	printf("\n\n");




/*****************************************************************/
	printf("*******************************************\n");
	printf("Fifth step: Get the %s's data\n",uid);
	printf("Please press enter key to continue!\n");
	printf("*******************************************\n");
	printf("\n\n");

#ifndef WILDDOG_PORT_TYPE_WICED

	while(1)
	{
		en_key= getchar();
		if(en_key == '\n')
			break;
	}
	en_key = 0;
#endif

	s_handle.p_node = NULL;	
	wilddog_getValue(wilddog, test_onQueryFunc, (void*)(&s_handle));
	
	while(1)
	{
		if(s_handle.isFinished)
		{
			if(s_handle.p_node)
			{
				wilddog_debug("print node:");
				wilddog_debug_printnode(s_handle.p_node);
				wilddog_node_delete(s_handle.p_node);
				s_handle.p_node= NULL;
				printf("\n");
				break;
			}
		}
		wilddog_trySync();
	}
	
	isFinished = FALSE;
	printf("*******************************************\n");
	printf("It has get the data in %s!\n",uid);
	printf("Please check the data tree!\n");
	printf("*******************************************\n");
	printf("\n\n");




/*****************************************************************/
	p_head = wilddog_node_createObject((Wilddog_Str_T *)"3");
	p_node = wilddog_node_createNum((Wilddog_Str_T *)"2",1234);
	wilddog_node_addChild(p_head, p_node);
	
	printf("*******************************************\n");
	printf("Sixth step: Build a node as \"{\"2\":1234}\"\n");
	printf("Push the new node to the %s's data tree's root node\n",uid);
	printf("Please press enter key to continue!\n");
	printf("*******************************************\n");
	printf("\n\n");

#ifndef WILDDOG_PORT_TYPE_WICED
	
	while(1)
	{
		en_key= getchar();
		if(en_key == '\n')
			break;
	}
	en_key = 0;
#endif
	
	wilddog_push(wilddog, p_head, test_onPushFunc, (void *)&isFinished);	
	wilddog_node_delete(p_head);
	
	while(1)
	{
		if(isFinished)
		{
			wilddog_debug("push success!");
			break;
		}
		wilddog_trySync();
	}

	printf("*******************************************\n");
	printf("It has pushed the data in %s!\n",uid);
	printf("Please check the data tree on %s page\n",url);
	printf("\{\"2\":1234}\" will have a parent which have a long number\n");
	printf("This is the server's strategy\n");
	printf("*******************************************\n");
	printf("\n\n");



	isFinished = FALSE;
	s_handle.p_node = NULL;	
	s_handle.isFinished = FALSE;

	printf("*******************************************\n");
	printf("Seventh step: Get the %s's new data\n",uid);
	printf("Please press enter key to continue!\n");
	printf("*******************************************\n");
	printf("\n\n");

#ifndef WILDDOG_PORT_TYPE_WICED
	
	while(1)
	{
		en_key= getchar();
		if(en_key == '\n')
			break;
	}
	en_key = 0;
#endif

	wilddog_getValue(wilddog, test_onQueryFunc, (void*)(&s_handle));
	
	while(1)
	{
		if(s_handle.isFinished)
		{
			if(s_handle.p_node)
			{
				wilddog_debug("print node:");
				wilddog_debug_printnode(s_handle.p_node);
				wilddog_node_delete(s_handle.p_node);
				printf("\n");
				break;
			}
		}
		wilddog_trySync();
	}
	printf("*******************************************\n");
	printf("It has get the data in %s!\n",uid);
	printf("Please check the data tree!\n");
	printf("*******************************************\n");
	printf("\n\n");

	isFinished = FALSE;
	printf("*******************************************\n");
	printf("Eighth step: Observe on the data tree\n");
	printf("Please change the data tree on %s page to trigger observe\n",url);
	printf("Please press enter key to continue!\n");
	printf("*******************************************\n");
	printf("\n\n");

#ifndef WILDDOG_PORT_TYPE_WICED

	while(1)
	{
		en_key= getchar();
		if(en_key == '\n')
			break;
	}
	en_key = 0;
#endif


	wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, test_onObserveFunc, (void*)&isFinished);
	while(1)
	{
		if(TRUE == isFinished)
		{
			wilddog_debug("get new data %d times!", count++);
			isFinished = FALSE;
			if(count > 1)
			{
				wilddog_debug("off the data!");
				wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE);
				break;
			}
		}
		wilddog_trySync();
	}

	
	printf("*******************************************\n");
	printf("The observe is finished!\n");
	printf("*******************************************\n");
	printf("\n\n");



	wilddog_destroy(&wilddog);
	printf("*******************************************\n");
	printf("The demo is finished!\n");
	printf("For more help, please read the guide.md document!\n");
	printf("*******************************************\n");
	printf("\n\n");

	return 0;
}



int main(int argc, char *argv[])
{
	int opt;
	char uid[256];
	//char url[1024];
	
	memset(uid,0,sizeof(uid));	
	//memset(url,0,sizeof(url));	
	//test("coaps://mk.wilddogio.com");
	//return 0;

	while ((opt = getopt(argc, argv, "hl:")) != -1) 
	{
		switch (opt) 
		{
		case 'h':
			fprintf(stderr, "Usage: %s  -l appid\n",
		           argv[0]);
			return 0;
		case 'l':
			strcpy(uid, (const char*)optarg);
			//printf("uid:%s\n",optarg);
			break;			
		default: /* '?' */
			fprintf(stderr, "Usage: %s  -l appid\n",
		           argv[0]);
			return 0;
		}
	}

	if( argc <3 )
	{
		printf("Usage: %s  -l appid\n", argv[0]);
		return 0;
	}

	//sprintf(url,"%s%s%s",TEST_URL_HEAD,uid,TEST_URL_END);
	test(uid);

	return 0;
}

