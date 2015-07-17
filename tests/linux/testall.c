#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
 
#include "wilddog_payload.h"
#include "wilddog_url_parser.h"
#include "wilddog_debug.h"
#include "wilddog.h"
#include "wilddog_ct.h"
#include "test.h"

char *test_url[] = {
	"coap://ap.wilddogio.com/",
	"cap://ap.wilddogio.com/a/b__cc",
	"coaps://ap.wilddogio.cn/",
	"coap://ap.wilddogio.com/abc123",
	"coaps://ap.wilddog.com/www",
	"coap://apc.wild.com/com"
};

u8 testcbor[] = {
0xa2,0x64,0x4c,0x31,0x63,0x31,0xa1,0x64,0x4c,0x32,0x63,0x31,
0x39,0x27,0x0f,0x64,0x4c,0x31,0x63,0x32,0xa2,0x64,0x4c,0x32,
0x63,0x32,0x68,0x32,0x2e,0x33,0x34,/*0x30,0x30,0x30,0x30,*/0x64,                  
0x4c,0x32,0x63,0x33,0xa1,0x64,0x4c,0x33,0x63,0x31,0x63,0x31,
0x30,0x30        
};
u8 testcbor1[] = {
0xbf, 0x64, 0x4c, 0x31, 0x63, 0x31, 0xbf, 0x64, 0x4c,
0x32, 0x63, 0x31, 0x39, 0x27, 0x0f, 0xff, 0x64, 0x4c,
0x31, 0x63, 0x32, 0xbf, 0x64, 0x4c, 0x32, 0x63, 0x32,
0xfb, 0x40, 0x02, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
0x64, 0x4c, 0x32, 0x63, 0x33, 0xbf, 0x64, 0x4c, 0x33,
0x63, 0x31, 0xf5, 0xff, 0xff, 0xff 
};


void printUrl(Wilddog_Url_T* url)
{
	if(NULL == url)
	{
		printf("NULL url!\n");
		return;
	}
//	printf("print url:\nd_url_secureType = %d\n", url->d_url_secureType);
//	printf("d_url_port = %d\n", url->d_url_port);
	printf("p_url_host = %s\n", url->p_url_host);
	printf("p_url_path = %s\n", url->p_url_path);
	printf("p_url_query = %s\n", url->p_url_query);
}

  
int main(void)
{
	int i;
	Wilddog_Node_T *root = NULL;
	Wilddog_Node_T *L1c1 = NULL,*L1c2 = NULL;
	Wilddog_Node_T *L2c1 = NULL,*L2c2 = NULL,*L2c3 = NULL;
	Wilddog_Node_T *L3c1 = NULL;
	wFloat f = 2.3;
	Wilddog_Payload_T *payload = NULL;
	Wilddog_Payload_T *p_data = NULL;


 
	Wilddog_T p_repo1_ref1 = 0, p_repo1_ref2 =0;
	Wilddog_T p_repo2_ref1 = 0, p_repo2_ref2 = 0,p_repo2_ref1_cpy = 0;
	Wilddog_T p_ref[6];
	Wilddog_T p_parent = 0, p_root = 0, p_child  = 0;
	Wilddog_Node_T *p_find =NULL;

	//testnode = wilddog_node_createUString("c", "2");
		

	/*
	 *  test the node api and cbor
	 */
	 p_data = malloc(sizeof(Wilddog_Payload_T));
	 p_data->d_dt_pos = 0;
	 p_data->d_dt_len = sizeof(testcbor1)/sizeof(u8);
	 printf("len:%d\n", p_data->d_dt_len);
	 p_data->p_dt_data = testcbor1;
	 printf("\nnode1\n");
	 wilddog_debug_printnode(_wilddog_payload2Node(p_data));
	 printf("\nnode2\n");


	root = wilddog_node_createNum((Wilddog_Str_T *)"root",9999);
	L1c1 = wilddog_node_createFalse((Wilddog_Str_T *)"L1c1");
	L1c2 = wilddog_node_createTrue((Wilddog_Str_T *)"L1c2");
	L2c1 = wilddog_node_createNum((Wilddog_Str_T *)"L2c1",-10000);
	L2c2 = wilddog_node_createFloat((Wilddog_Str_T *)"L2c2",f);
	L2c3 = wilddog_node_createUString((Wilddog_Str_T *)"L2c3",(Wilddog_Str_T *)"UStr");
	L3c1 = wilddog_node_createTrue((Wilddog_Str_T *)"L3c1");
	
	if(	root == NULL || L1c1 == NULL || L1c2 == NULL ||
		L2c1 == NULL || L2c2 == NULL || L2c3 == NULL )
		{
			TEST_RESULT_PRINTF("test_all:creat error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
			return ABORT_ERR;
		}
	
	if(	WILDDOG_ERR_NOERR != wilddog_node_addChild(root,L1c1) 	||
		WILDDOG_ERR_NOERR != wilddog_node_addChild(root,L1c2) 	||
		WILDDOG_ERR_NOERR != wilddog_node_addChild(L1c1,L2c1)	||
		WILDDOG_ERR_NOERR != wilddog_node_addChild(L1c2,L2c2)	||
		WILDDOG_ERR_NOERR != wilddog_node_addChild(L1c2,L2c3)	||
		WILDDOG_ERR_NOERR != wilddog_node_addChild(L2c3,L3c1)	)
	{
		
		TEST_RESULT_PRINTF("test_all:node add error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	wilddog_debug_printnode(root);	
	p_find = wilddog_node_find(root,"/L1c1/L2c1");
	if(p_find != L2c1 )
	{
		TEST_RESULT_PRINTF("test_all:find node error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	payload = _wilddog_node2Payload(root);
	if(payload == NULL)
	{		
		TEST_RESULT_PRINTF("test_all:node to cbor error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	printf("show the payload:\n");
	for(i = 0; i< payload->d_dt_len; i++)
	{
		printf("%02x ", payload->p_dt_data[i]);
	}
	printf("\n\n");
	for(i = 0; i< payload->d_dt_len; i++)
	{
		printf("0x%02x, ", payload->p_dt_data[i]);
	}
	printf("\n\n");

	/*
	 *  test the url format
	 */
	for( i = 0; i < sizeof(test_url)/sizeof(char *); i++)
	{
		p_ref[i] = wilddog_initWithUrl((Wilddog_Str_T *)test_url[i]);	

		if(p_ref[i])
		{
			printf("p_ref--  [%d] \n\n", i);
			printUrl(((Wilddog_Ref_T *)p_ref[i])->p_ref_url);
			printf("repo = %p, ref = %p\n", ((Wilddog_Ref_T *)p_ref[i])->p_ref_repo, ((Wilddog_Ref_T *)p_ref[i]));
			printf("----------------------\n\n\n");
		}
		else
		{
			printf("------------------------\n");
			printf("%s could not init\n\n", test_url[i]);
			printf("------------------------\n");
			
			TEST_RESULT_PRINTF("test_all:wilddog_initWithUrl error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
			return ABORT_ERR;
		}
	}

	printf("\n\ndestroy the test url before\n\n");
	for( i = 0; i < sizeof(test_url)/sizeof(char *); i++)
	{
		printf("destroy %d test url %lu\n", i, (long unsigned int)p_ref[i]);
		if(p_ref[i])
			wilddog_destroy(&(p_ref[i]));
		wilddog_debug();
	}
	printf("\n\ndestroy the test url after\n\n");

	/*
	 *  test the multi repo and multi ref
	 *  test getparent getchild getroot
	 */
	p_repo1_ref1 = wilddog_initWithUrl((Wilddog_Str_T *)"coaps://appid1.wilddogio.com/a1");
	p_repo1_ref2 = wilddog_initWithUrl((Wilddog_Str_T *)"coaps://appid1.wilddogio.com/a1/b2");

	p_repo2_ref1 = wilddog_initWithUrl((Wilddog_Str_T *)"coaps://appid2.wilddogio.com/c3");
	p_repo2_ref1_cpy = wilddog_initWithUrl((Wilddog_Str_T *)"coaps://appid2.wilddogio.com/c3");
	p_repo2_ref2 = wilddog_initWithUrl((Wilddog_Str_T *)"coaps://appid2.wilddogio.com/c3/d4/e5");
	
	if(p_repo1_ref1)
	{
		printf("repo1--ref1\n\n");
		printUrl(((Wilddog_Ref_T *)p_repo1_ref1)->p_ref_url);
		printf("----------------------\n\n\n");
	}

	if(p_repo1_ref2)
	{
		printf("repo1--ref2\n\n");
		printUrl(((Wilddog_Ref_T *)p_repo1_ref2)->p_ref_url);
		printf("----------------------\n\n\n");
	}

	if(p_repo2_ref1)
	{
		printf("repo2--ref1\n\n");
		printUrl(((Wilddog_Ref_T *)p_repo2_ref1)->p_ref_url);
		printf("----------------------\n\n\n");
	}
	if(p_repo2_ref1 != p_repo2_ref1_cpy)
	{
		TEST_RESULT_PRINTF("test_all: new same url error ",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	if(p_repo2_ref2)
	{
		printf("repo2--ref2\n\n");
		printUrl(((Wilddog_Ref_T *)p_repo2_ref2)->p_ref_url);
		printf("----------------------\n\n\n");
	}
#if 0  /* todo */

	p_parent = wilddog_getParent(p_repo1_ref1);
	if(p_parent)
	{
		printf("repo1--ref1--parent\n\n");
		printUrl(((Wilddog_Ref_T *)p_parent)->p_ref_url);
		printf("----------------------\n\n\n");
		wilddog_destroy(&p_parent);
	}
	else
	{
		wilddog_debug("could not get repo1_ref1's parent\n\n");
		TEST_RESULT_PRINTF("test_all:get parent error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
#endif
	p_parent = wilddog_getParent(p_repo1_ref2);
	
	if(p_parent == p_repo1_ref1)
	{
		printf("repo1--ref2--parent\n\n");
		printUrl(((Wilddog_Ref_T *)p_parent)->p_ref_url);
		printf("----------------------\n\n\n");
		wilddog_destroy(&p_parent);
	}
	else
	{
		
		wilddog_debug("could not get repo1_ref2's parent\n\n");
		TEST_RESULT_PRINTF("test_all:get parent error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}

	p_root = wilddog_getRoot(p_repo1_ref2);
	if(p_root)
	{
		printf("repo1--ref2--root\n\n");
		printUrl(((Wilddog_Ref_T *)p_root)->p_ref_url);
		printf("----------------------\n\n\n");
		wilddog_destroy(&p_root);
	}
	else
	{
		wilddog_debug("could not get repo1_ref2's root\n\n");
		TEST_RESULT_PRINTF("test_all:get root error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	wilddog_debug("p_repo1_ref1 = %u",( unsigned int)p_repo1_ref1);
	printUrl(((Wilddog_Ref_T *)p_repo1_ref1)->p_ref_url);
	p_child = wilddog_getChild(p_repo2_ref2, (Wilddog_Str_T *)"/c3/d4");
	wilddog_debug("p_child = %u", ( unsigned int)p_child);
	if(p_child)
	{
		wilddog_debug();
		printf("p_repo2_ref2--child\n\n");
		printUrl(((Wilddog_Ref_T *)p_child)->p_ref_url);
		printf("----------------------\n\n\n");
		wilddog_destroy(&p_child);
	}
	else
	{
		printf("could not get p_repo2_ref2's child /c3/d4\n\n");
		TEST_RESULT_PRINTF("test_all:get chile error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	p_parent = wilddog_getParent(p_repo2_ref2);
	if(p_parent)
	{
		printf("repo2--ref2--parent\n\n");
		printUrl(((Wilddog_Ref_T *)p_parent)->p_ref_url);
		printf("----------------------\n\n\n");
		wilddog_destroy(&p_parent);
	}
	else
	{
		printf("could not get repo2_ref2's parent\n\n");
	}
	/* clone test
	**/
	//wilddog_destroy(p_root);
	printf("\n\ndestroy all ref\n\n");
	wilddog_destroy(&p_root);
	wilddog_destroy(&p_parent);
	wilddog_destroy(&p_child);
		
	wilddog_destroy(&p_repo1_ref1);
	wilddog_destroy(&p_repo1_ref2);
	wilddog_destroy(&p_repo2_ref1);
	wilddog_destroy(&p_repo2_ref1_cpy);
	wilddog_destroy(&p_repo2_ref2);
		
	return 0;
}







