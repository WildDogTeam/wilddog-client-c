#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>

#include<sys/time.h>

#include "wilddog.h"
#include "test_lib.h"
#include "test_config.h"


static u8 *p_ramtree_url[TEST_TREE_ITEMS] = 
					{
						TEST_TREE_T_127,
						TEST_TREE_T_256,
						TEST_TREE_T_576,
						TEST_TREE_T_810,
						TEST_TREE_T_1044,
						TEST_TREE_T_1280,
					};

static  u32 d_ramtree_num[TEST_TREE_ITEMS] = {127,256,576,810,1044,1280};

int test_ram(VOID)
{

#ifdef WILDDOG_SELFTEST
		int res = 0;
		u8 url[sizeof(TEST_URL)];
		u8 tree_m=0, n=0;
		u8 request_num[4] = {1,16,32,64};

	
		if( (res = test_buildtreeFunc(TEST_URL) ) < 0 )
				return res;
		ramtest_titile_printf();
	
		for( tree_m=0; tree_m < TEST_TREE_ITEMS; tree_m++)
		{
		
			if( (d_ramtree_num[tree_m] + TEST_PROTO_COVER) > WILDDOG_PROTO_MAXSIZE )
			{
				printf("please modify WILDDOG_PROTO_MAXSIZE to %d ,in wilddog_config.h \n",\
					(d_ramtree_num[tree_m] + TEST_PROTO_COVER));
				return -1;
			}
			
			for( n=0; n <4; n++)
			{
				memset(url,0,sizeof(url));
				sprintf(url,"%s%s",TEST_URL,p_ramtree_url[tree_m]);
				ramtest_handle(url,d_ramtree_num[tree_m],request_num[n]);
			}
		}
		ramtest_end_printf();
#endif
		return 0;

}

