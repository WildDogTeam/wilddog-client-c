#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wilddog.h"

struct test_reult_t
{
	char* name;
	int result;
};
struct test_reult_t test_results[32];

STATIC const char* UNUSED_URL1="coap://coap.wilddogio.com/unused/a";
STATIC const char* UNUSED_URL2="coap://coap.wilddogio.com/unused/a/b";
STATIC const char* UNUSED_URL3="coap://coap.wilddogio.com/unused/a/b/c";
STATIC const char* UNUSED_URL4="coap://coap.wilddogio.com/unused/a/c";

/*wilddog_addObserver*/
int test_on()
{	
    Wilddog_T wilddog1 = 0, wilddog2 = 0, wilddog3 = 0, wilddog4 = 0;

	printf("\n\n--------1234-----------------------------------------------\n\n");

	wilddog1 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL1);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL2);
	wilddog3 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL3);
	wilddog4 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL4);

	wilddog_addObserver(wilddog1, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog2, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog3, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog4, WD_ET_VALUECHANGE, NULL, NULL);


	
	wilddog_destroy(&wilddog1);
	wilddog_destroy(&wilddog2);
	wilddog_destroy(&wilddog3);
	wilddog_destroy(&wilddog4);

	printf("\n\n--------1243-----------------------------------------------\n\n");
	wilddog1 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL1);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL2);
	wilddog3 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL3);
	wilddog4 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL4);

	wilddog_addObserver(wilddog1, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog2, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog4, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog3, WD_ET_VALUECHANGE, NULL, NULL);


	
	wilddog_destroy(&wilddog1);
	wilddog_destroy(&wilddog2);
	wilddog_destroy(&wilddog3);
	wilddog_destroy(&wilddog4);

	printf("\n\n--------1324-----------------------------------------------\n\n");
	wilddog1 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL1);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL2);
	wilddog3 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL3);
	wilddog4 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL4);

	wilddog_addObserver(wilddog1, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog3, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog2, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog4, WD_ET_VALUECHANGE, NULL, NULL);
	
	wilddog_destroy(&wilddog1);
	wilddog_destroy(&wilddog2);
	wilddog_destroy(&wilddog3);
	wilddog_destroy(&wilddog4);

	printf("\n\n--------1342-----------------------------------------------\n\n");
	wilddog1 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL1);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL2);
	wilddog3 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL3);
	wilddog4 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL4);

	wilddog_addObserver(wilddog1, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog3, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog4, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog2, WD_ET_VALUECHANGE, NULL, NULL);
	
	wilddog_destroy(&wilddog1);
	wilddog_destroy(&wilddog2);
	wilddog_destroy(&wilddog3);
	wilddog_destroy(&wilddog4);


	printf("\n\n--------1423-----------------------------------------------\n\n");
	wilddog1 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL1);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL2);
	wilddog3 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL3);
	wilddog4 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL4);

	wilddog_addObserver(wilddog1, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog4, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog2, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog3, WD_ET_VALUECHANGE, NULL, NULL);
	
	wilddog_destroy(&wilddog1);
	wilddog_destroy(&wilddog2);
	wilddog_destroy(&wilddog3);
	wilddog_destroy(&wilddog4);

	printf("\n\n--------1432-----------------------------------------------\n\n");
	wilddog1 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL1);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL2);
	wilddog3 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL3);
	wilddog4 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL4);

	wilddog_addObserver(wilddog1, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog4, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog3, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog2, WD_ET_VALUECHANGE, NULL, NULL);
	
	wilddog_destroy(&wilddog1);
	wilddog_destroy(&wilddog2);
	wilddog_destroy(&wilddog3);
	wilddog_destroy(&wilddog4);
	printf("\n\n--------4321-----------------------------------------------\n\n");
	wilddog1 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL1);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL2);
	wilddog3 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL3);
	wilddog4 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL4);

	wilddog_addObserver(wilddog4, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog3, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog2, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog1, WD_ET_VALUECHANGE, NULL, NULL);
	
	wilddog_destroy(&wilddog1);
	wilddog_destroy(&wilddog2);
	wilddog_destroy(&wilddog3);
	wilddog_destroy(&wilddog4);


	printf("\n\n--------4312-----------------------------------------------\n\n");
	wilddog1 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL1);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL2);
	wilddog3 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL3);
	wilddog4 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL4);

	wilddog_addObserver(wilddog4, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog3, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog1, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog2, WD_ET_VALUECHANGE, NULL, NULL);
	
	wilddog_destroy(&wilddog1);
	wilddog_destroy(&wilddog2);
	wilddog_destroy(&wilddog3);
	wilddog_destroy(&wilddog4);


	printf("\n\n--------4132-----------------------------------------------\n\n");
	wilddog1 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL1);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL2);
	wilddog3 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL3);
	wilddog4 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL4);

	wilddog_addObserver(wilddog4, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog1, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog3, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog2, WD_ET_VALUECHANGE, NULL, NULL);
	
	wilddog_destroy(&wilddog1);
	wilddog_destroy(&wilddog2);
	wilddog_destroy(&wilddog3);
	wilddog_destroy(&wilddog4);

	printf("\n\n--------3124-----------------------------------------------\n\n");
	wilddog1 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL1);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL2);
	wilddog3 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL3);
	wilddog4 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL4);

	wilddog_addObserver(wilddog3, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog1, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog2, WD_ET_VALUECHANGE, NULL, NULL);
	wilddog_addObserver(wilddog4, WD_ET_VALUECHANGE, NULL, NULL);
	
	wilddog_destroy(&wilddog1);
	wilddog_destroy(&wilddog2);
	wilddog_destroy(&wilddog3);
	wilddog_destroy(&wilddog4);

	return 0;

}



int main(void)
{
	test_on();
	return 0;
}

