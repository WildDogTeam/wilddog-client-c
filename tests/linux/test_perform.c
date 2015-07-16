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
#ifdef WILDDOG_SELFTEST
	performtest_all();
#endif
	return 0;
}
