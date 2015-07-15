#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>

#include<sys/time.h>

#include "wilddog.h"
#include "test_lib.h"

#define TREE_NUM 1	/* {1,2,3}; */
#define REQUEST_NUM 16  /* {1,16,32,64}; */
#define TRYSNCDELAY	 0

int main(void)
{
	u8 m=0, n=0;
	u8 tree_num[3] = {1,2,3};
	u8 request_num[4] = {1,16,32,64};
#ifdef WILDDOG_SELFTEST
	ramtest_titile_printf();
	//ramtest_handle(TREE_NUM,REQUEST_NUM);

	for( m=0; m < 3; m++)
	{
		for( n=0; n < 4; n++)
		{
			ramtest_handle(tree_num[m],request_num[n]);
		}
	}
	ramtest_end_printf();
#endif
	return 0;
}
