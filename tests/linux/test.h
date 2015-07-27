/*
*
*/
#ifndef _TEST_H_
#define _TEST_H_
#include "wilddog.h"
#define TEST_URL "coaps://c_test.wilddogio.com/"



#define ABORT_ERR	-1
#define TEST_ERR	'N'
#define TEST_OK		'Y'
#define TESTFUNCNAME_TABLECASE	2	
#define TEST_RESULT_PRINTF(p,inv,res ,err)	do{ \
											printf("%-32s",(p)); \
											if(err) printf("%c\t%d\n",(res),(err));\
											else printf("%c\n",(res));}while(0)


#endif /* _TEST_H_ */
