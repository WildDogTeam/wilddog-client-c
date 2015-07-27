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
		wilddog_debug_level(WD_DEBUG_LOG, "NULL url!\n");
		return;
	}
//	wilddog_debug_level("print url:\nd_url_secureType = %d\n", url->d_url_secureType);
//	wilddog_debug_level("d_url_port = %d\n", url->d_url_port);
	wilddog_debug_level(WD_DEBUG_LOG, "p_url_host = %s\n", url->p_url_host);
	wilddog_debug_level(WD_DEBUG_LOG, "p_url_path = %s\n", url->p_url_path);
	wilddog_debug_level(WD_DEBUG_LOG, "p_url_query = %s\n", url->p_url_query);
}

  
int main(void)
{
	int i;
 
	Wilddog_T p_repo1_ref1 = 0, p_repo1_ref2 =0;
	Wilddog_T p_repo2_ref1 = 0, p_repo2_ref2 = 0,p_repo2_ref1_cpy = 0;
	Wilddog_T p_ref[6];
	Wilddog_T p_parent = 0, p_root = 0, p_child  = 0;

	/*
	 *  test the url format
	 */
	for( i = 0; i < sizeof(test_url)/sizeof(char *); i++)
	{
		p_ref[i] = wilddog_initWithUrl((Wilddog_Str_T *)test_url[i]);	

		if(p_ref[i])
		{
			wilddog_debug_level(WD_DEBUG_LOG, "p_ref--  [%d] \n\n", i);
			printUrl(((Wilddog_Ref_T *)p_ref[i])->p_ref_url);
			wilddog_debug_level(WD_DEBUG_LOG, "repo = %p, ref = %p\n", ((Wilddog_Ref_T *)p_ref[i])->p_ref_repo, ((Wilddog_Ref_T *)p_ref[i]));
			wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
		}
		else
		{
			wilddog_debug_level(WD_DEBUG_LOG, "------------------------\n");
			wilddog_debug_level(WD_DEBUG_LOG, "%s could not init\n\n", test_url[i]);
			wilddog_debug_level(WD_DEBUG_LOG, "------------------------\n");
			
			TEST_RESULT_PRINTF("test_all:wilddog_initWithUrl error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
			return ABORT_ERR;
		}
	}

	wilddog_debug_level(WD_DEBUG_LOG, "\n\ndestroy the test url before\n\n");
	for( i = 0; i < sizeof(test_url)/sizeof(char *); i++)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "destroy %d test url %lu\n", i, (long unsigned int)p_ref[i]);
		if(p_ref[i])
			wilddog_destroy(&(p_ref[i]));
	}
	wilddog_debug_level(WD_DEBUG_LOG, "\n\ndestroy the test url after\n\n");

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
		wilddog_debug_level(WD_DEBUG_LOG, "repo1--ref1\n\n");
		printUrl(((Wilddog_Ref_T *)p_repo1_ref1)->p_ref_url);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
	}

	if(p_repo1_ref2)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "repo1--ref2\n\n");
		printUrl(((Wilddog_Ref_T *)p_repo1_ref2)->p_ref_url);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
	}

	if(p_repo2_ref1)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "repo2--ref1\n\n");
		printUrl(((Wilddog_Ref_T *)p_repo2_ref1)->p_ref_url);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
	}
	if(p_repo2_ref1 != p_repo2_ref1_cpy)
	{
		TEST_RESULT_PRINTF( "test_all: new same url error ",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	if(p_repo2_ref2)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "repo2--ref2\n\n");
		printUrl(((Wilddog_Ref_T *)p_repo2_ref2)->p_ref_url);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
	}
#if 0  /* todo */

	p_parent = wilddog_getParent(p_repo1_ref1);
	if(p_parent)
	{
		wilddog_debug_level("repo1--ref1--parent\n\n");
		printUrl(((Wilddog_Ref_T *)p_parent)->p_ref_url);
		wilddog_debug_level("----------------------\n\n\n");
		wilddog_destroy(&p_parent);
	}
	else
	{
		wilddog_debug("could not get repo1_ref1's parent\n\n");
		TEST_RESULT_wilddog_debug_level("test_all:get parent error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
#endif
	p_parent = wilddog_getParent(p_repo1_ref2);
	
	if(p_parent == p_repo1_ref1)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "repo1--ref2--parent\n\n");
		printUrl(((Wilddog_Ref_T *)p_parent)->p_ref_url);
		TEST_RESULT_PRINTF("wilddog_getParent",TESTFUNCNAME_TABLECASE,TEST_OK,0);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");

	}
	else
	{
		
		wilddog_debug_level(WD_DEBUG_LOG, "could not get repo1_ref2's parent\n\n");
		TEST_RESULT_PRINTF("test_all:get parent error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}

	p_root = wilddog_getRoot(p_repo1_ref2);
	if(p_root)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "repo1--ref2--root\n\n");
		printUrl(((Wilddog_Ref_T *)p_root)->p_ref_url);
		TEST_RESULT_PRINTF("wilddog_getRoot",TESTFUNCNAME_TABLECASE,TEST_OK,0);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
		wilddog_destroy(&p_root);
	}
	else
	{
		wilddog_debug_level(WD_DEBUG_LOG, "could not get repo1_ref2's root\n\n");
		TEST_RESULT_PRINTF("test_all:get root error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	wilddog_debug_level(WD_DEBUG_LOG, "p_repo1_ref1 = %u",( unsigned int)p_repo1_ref1);
	printUrl(((Wilddog_Ref_T *)p_repo1_ref1)->p_ref_url);
	p_child = wilddog_getChild(p_repo2_ref2, (Wilddog_Str_T *)"c3/d4");
	wilddog_debug_level(WD_DEBUG_LOG, "p_child = %u", ( unsigned int)p_child);
	if(p_child)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "p_repo2_ref2--child\n\n");
		printUrl(((Wilddog_Ref_T *)p_child)->p_ref_url);
		TEST_RESULT_PRINTF("wilddog_getChild",TESTFUNCNAME_TABLECASE,TEST_OK,0);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
		wilddog_destroy(&p_child);
	}
	else
	{
		wilddog_debug_level(WD_DEBUG_LOG, "could not get p_repo2_ref2's child /c3/d4\n\n");
		TEST_RESULT_PRINTF("test_all:get chile error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	p_parent = wilddog_getParent(p_repo2_ref2);
	if(p_parent)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "repo2--ref2--parent\n\n");
		printUrl(((Wilddog_Ref_T *)p_parent)->p_ref_url);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
		wilddog_destroy(&p_parent);
	}
	else
	{
		wilddog_debug_level(WD_DEBUG_LOG, "could not get repo2_ref2's parent\n\n");
	}


	wilddog_debug_level(WD_DEBUG_LOG, "\n\ndestroy all ref\n\n");
		
	wilddog_destroy(&p_repo1_ref1);
	wilddog_destroy(&p_repo1_ref2);
	wilddog_destroy(&p_repo2_ref1);
	wilddog_destroy(&p_repo2_ref2);
		
	return 0;
}







