#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "wilddog.h"
#include "wilddog_debug.h"
#include "test.h"

struct test_reult_t
{
	char* name;
	int result;
};

#define FUNC_NUM 8
STATIC struct test_reult_t test_results[FUNC_NUM];

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

int main(void)
{
	int i, j;
 
	Wilddog_T p_repo1_ref1 = 0, p_repo1_ref2 =0;
	Wilddog_T p_repo2_ref1 = 0, p_repo2_ref2 = 0,p_repo2_ref1_cpy = 0;
	Wilddog_T p_ref[6];
	Wilddog_T p_parent = 0, p_root = 0, p_child  = 0;

	Wilddog_Str_T *repo1_ref2_key = NULL, *repo2_ref2_key = NULL;

	for(j = 0; j < FUNC_NUM; j++)
	{
		memset(&test_results[j], 0, sizeof(struct test_reult_t));
	}
	test_results[0].name = "wilddog_getParent";
	test_results[1].name = "wilddog_getRoot";
	test_results[2].name = "wilddog_getChild";

	test_results[3].name = "wilddog_getKey";
	
	/*
	 *  test the url format
	 */
	for( i = 0; i < sizeof(test_url)/sizeof(char *); i++)
	{
		p_ref[i] = wilddog_initWithUrl((Wilddog_Str_T *)test_url[i]);	

		if(p_ref[i])
		{
			wilddog_debug_level(WD_DEBUG_LOG, "p_ref--  [%d] \n\n", i);
			wilddog_debug_printUrl(p_ref[i]);
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
		wilddog_debug_printUrl(p_repo1_ref1);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
	}

	if(p_repo1_ref2)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "repo1--ref2\n\n");
		wilddog_debug_printUrl(p_repo1_ref2);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
	}

	if(p_repo2_ref1)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "repo2--ref1\n\n");
		wilddog_debug_printUrl(p_repo2_ref1);
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
		wilddog_debug_printUrl(p_repo2_ref2);
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
	}

	p_parent = wilddog_getParent(p_repo1_ref2);
	
	if(p_parent == p_repo1_ref1)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "repo1--ref2--parent\n\n");
		wilddog_debug_printUrl(p_parent);
		test_results[0].result = TRUE;
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
		wilddog_debug_printUrl(p_root);
		test_results[1].result = TRUE;
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
	wilddog_debug_printUrl(p_repo1_ref1);
	p_child = wilddog_getChild(p_repo2_ref2, (Wilddog_Str_T *)"c3/d4");
	wilddog_debug_level(WD_DEBUG_LOG, "p_child = %u", ( unsigned int)p_child);
	if(p_child)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "p_repo2_ref2--child\n\n");
		wilddog_debug_printUrl(p_child);
		test_results[2].result = TRUE;
		wilddog_debug_level(WD_DEBUG_LOG, "----------------------\n\n\n");
		wilddog_destroy(&p_child);
	}
	else
	{
		wilddog_debug_level(WD_DEBUG_LOG, "could not get p_repo2_ref2's child /c3/d4\n\n");
		TEST_RESULT_PRINTF("test_all:get chile error",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}


	repo1_ref2_key = wilddog_getKey(p_repo1_ref2);
	repo2_ref2_key = wilddog_getKey(p_repo2_ref2);

	if((strncmp((const char *)repo1_ref2_key , "b2", strlen("b2")) == 0) && (strncmp((const char *)repo2_ref2_key , "e5", strlen("e5")) == 0))
	{
		test_results[3].result = TRUE;
	}
	else
	{
		test_results[3].result = FALSE;
	}
	wfree(repo1_ref2_key);
	wfree(repo2_ref2_key);

	p_parent = wilddog_getParent(p_repo2_ref2);
	if(p_parent)
	{
		wilddog_debug_level(WD_DEBUG_LOG, "repo2--ref2--parent\n\n");
		wilddog_debug_printUrl(p_parent);
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
	
	for(j = 0 ; j < FUNC_NUM; j++)
	{
		if(test_results[j].name)
		{
			printf("%-32s\t%s\n", test_results[j].name, (test_results[j].result == TRUE?("PASS"):("FAIL")));
			if(test_results[j].result != TRUE)
				return -1;
		}
	}
	return 0;
}
