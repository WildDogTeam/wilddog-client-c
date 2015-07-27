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

STATIC const char* UNUSED_URL="coap://coap.wilddogio.com/unused";

const char *test_diffHost_url[]=
{
	"coap://c_test.wilddogio.com/",
	"coap://sky.wilddogio.com/unused"
};

STATIC const char *authData = "aaabbb";
const char* test_new_url[]=
{
 0, /*not used*/
 0, /*null*/
 "abc", /*not a url*/
 "a.wilddogio.com/?a=1", /*no scheme*/
 "http:///?a=1", /*no host*/
 "ht/://a.wilddogio.com/?a=1", /*invaild scheme*/
 "http:///a.wilddogio.com/?a=1", /*invalid seprator*/
 "http://a.wilddogio.com//a", /*invalid path*/
 "http://a.wilddogio.com//a//", /*invalid path*/
 "http://a.wilddogio.com//a//b", /*invalid path*/
};

STATIC void onCallback()
{

}


/*wilddog_increaseTime*/

/**/

/*wilddog_initWithUrl*/
int test_new()
{
	Wilddog_T wilddog = 0, wilddog2 = 0;
	int i;
	

	/*check invalid url*/
	for(i = 1; i < sizeof(test_new_url) / sizeof(char*); i++)
	{
		wilddog = wilddog_initWithUrl((Wilddog_Str_T *)test_new_url[i]);

		if(wilddog)
			return i;
	}

	/*check duplicated url*/

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	if(!wilddog || (wilddog != wilddog2))
	{
		wilddog_destroy(&wilddog);
		wilddog_destroy(&wilddog2);
		return -1;
	}
	wilddog_destroy(&wilddog);

	/* diff host */
	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)test_diffHost_url[0]);
	wilddog2 = wilddog_initWithUrl((Wilddog_Str_T *)test_diffHost_url[1]);

	if(!wilddog ||!wilddog2  || (wilddog == wilddog2))
	{
		wilddog_destroy(&wilddog);
		wilddog_destroy(&wilddog2);
		return -1;
	}
	
	wilddog_destroy(&wilddog);
	wilddog_destroy(&wilddog2);
	
	return 0;
}

/* wilddog_destroy */
/* test if it will segment fault */
int test_destroy()
{
	Wilddog_T wilddog = 0;

	/*destroy before init*/
	wilddog_destroy((void*)0);

	/*destroy empty ref*/
	wilddog_destroy((void*)0);

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	/*destory 0*/
	wilddog_destroy((void*)0);
	/*normal destroy*/
	wilddog_destroy(&wilddog);
	/*double destroy*/
	wilddog_destroy(&wilddog);

	return 0;
}

/*wilddog_auth*/
/*
 * 1. sdk not inited
 * 2. host null
 * 3. host not inited
 * 4. auth data null
*/

int test_auth()
{
	Wilddog_T wilddog = 0;

	/* host null */
	if (WILDDOG_ERR_NOERR == wilddog_auth(NULL, (u8*)authData, \
	         strlen((const char*)authData), NULL, NULL)
	   )
	   return -1;

	/*host not inited*/
	if (WILDDOG_ERR_NOERR == wilddog_auth((Wilddog_Str_T *)"coap.wilddogio.com", (u8 *)authData, \
	         strlen((const char*)authData), NULL, NULL)
	   )
	   return -1;
	
	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	/*auth data null*/
	if (WILDDOG_ERR_NOERR == wilddog_auth((Wilddog_Str_T *)"coap.wilddogio.com", NULL, \
		 strlen((const char*)authData), NULL, NULL)
   	    )
	{
		wilddog_destroy(&wilddog);
        return -1;
	}

	/*valid*/
	if (WILDDOG_ERR_NOERR != wilddog_auth((Wilddog_Str_T *)"coap.wilddogio.com", (u8*)authData, \
		 strlen((const char*)authData), NULL, NULL)
   	    )
	{
		wilddog_destroy(&wilddog);
        return -1;
	}

	wilddog_destroy(&wilddog);
	return 0;
}

/*wilddog_getParent*/
int test_getParent()
{
	Wilddog_T wilddog = 0, parent,parent2;

	parent = wilddog_getParent(wilddog);
	if(parent)
		return -1;

	


	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)"coap://coap.wilddogio.com/a/b/c");

	/* should /a/b */
	parent = wilddog_getParent(wilddog);
	/*wilddog_debug_printUrl(parent);*/
	if(!parent)
	{
		wilddog_destroy(&wilddog);
		return -1;
	}
	/* should /a */
	parent2 = wilddog_getParent(parent);
	/*wilddog_debug_printUrl(parent2);*/
	if(!parent2)
	{
		wilddog_destroy(&wilddog);
		wilddog_destroy(&parent);
		return -1;
	}

	wilddog_destroy(&parent);
	/* should / */
	parent = wilddog_getParent(parent2);
	/*wilddog_debug_printUrl(parent);*/
	if(!parent)
	{
		wilddog_destroy(&wilddog);
		wilddog_destroy(&parent2);
		return -1;
	}

	wilddog_destroy(&parent2);

	/* should no parent */
	parent2 = wilddog_getParent(parent);
	
	if(parent2)
	{
		/*wilddog_debug_printUrl(parent2);*/
		wilddog_destroy(&wilddog);
		wilddog_destroy(&parent);
		wilddog_destroy(&parent2);
		return -1;
	}
	wilddog_destroy(&wilddog);
	wilddog_destroy(&parent);
	wilddog_destroy(&parent2);
	return 0;
}

/*wilddog_getRoot*/
int test_getRoot()
{
	Wilddog_T wilddog = 0, root, root2;

	

	/* Invalid, should be 0 */
	root = wilddog_getParent(wilddog);
	if(root)
		return -1;

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)"coap://coap.wilddogio.com/a/b");

	/* should be / */
	root = wilddog_getRoot(wilddog);
	if(!root)
	{
		wilddog_debug("");
		wilddog_destroy(&wilddog);
		return -1;
	}
	/* should be as same as root */
	root2 = wilddog_getRoot(root);

	if(root != root2)
	{
		wilddog_debug("");
		wilddog_destroy(&wilddog);
		wilddog_destroy(&root);
		wilddog_destroy(&root2);
		return -1;
	}
	wilddog_destroy(&wilddog);
	wilddog_destroy(&root);
	return 0;
}

/*wilddog_getChild*/
int test_getChild()
{
	int err = -1;
	Wilddog_T wilddog = 0, child;

	

	/* not inited, should be 0 */
	child = wilddog_getChild(wilddog, NULL);
	if(child)
		return -1;
	
	/* not inited, should be 0 */
	child = wilddog_getChild(wilddog, (Wilddog_Str_T *)"a");
	if(child)
		return -1;

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	/* Invalid, should be 0 */
	child = wilddog_getChild(wilddog, NULL);
	if(child)
		goto GET_CHILD_END;

	/* Invalid, should be 0 */
	child = wilddog_getChild(wilddog, (Wilddog_Str_T *)"/");
	if(child)
		goto GET_CHILD_END;

	/* Invalid, should be 0 */
	child = wilddog_getChild(wilddog, (Wilddog_Str_T *)"/a");
	if(child)
		goto GET_CHILD_END;

	/* valid */
	child = wilddog_getChild(wilddog, (Wilddog_Str_T *)"a");
	if(!child)
		goto GET_CHILD_END;
	wilddog_destroy(&child);

	/* Invalid, should be 0 */
	child = wilddog_getChild(wilddog, (Wilddog_Str_T *)"a/b/");
	if(child)
	{
		goto GET_CHILD_END;
	}

	/* valid */
	child = wilddog_getChild(wilddog, (Wilddog_Str_T *)"a/b");
	if(!child)
	{
		goto GET_CHILD_END;
	}
	wilddog_destroy(&child);
	/* Invalid, should be 0 */
	child = wilddog_getChild(wilddog, (Wilddog_Str_T *)"a//b");
	if(child)
	{
		goto GET_CHILD_END;
	}
	
	err = 0;
	
GET_CHILD_END:
	wilddog_destroy(&wilddog);
	wilddog_destroy(&child);
	return err;
}

/*wilddog_getKey*/
int test_getKey()
{
	Wilddog_Str_T *key = NULL;
	Wilddog_T wilddog, root;
	

	/* Invalid */
	key = wilddog_getKey(0);

	if(key)
	{
		wfree(key);
		return -1;
	}

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	/* valid */
	key = wilddog_getKey(wilddog);
	if(!key)
	{
		return -1;
	}

	wfree(key);

	root = wilddog_getRoot(wilddog);

	/* valid */
	key = wilddog_getKey(root);
	if(!key)
	{
		return -1;
	}
	wfree(key);

	wilddog_destroy(&wilddog);
	wilddog_destroy(&root);
	return 0;
}

/*wilddog_getValue*/

int test_query()
{
	Wilddog_T wilddog;

	
	
	/*Invalid*/
	if(WILDDOG_ERR_NOERR == wilddog_getValue(0, NULL, NULL))
		return -1;

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	/*Invalid*/
	if(WILDDOG_ERR_NOERR == wilddog_getValue(0, NULL, NULL))
		return -1;

	/*valid*/
	if(WILDDOG_ERR_NOERR != wilddog_getValue(wilddog, NULL, NULL))
		return -1;

	/*valid*/
	if(WILDDOG_ERR_NOERR != wilddog_getValue(wilddog, (onQueryFunc)onCallback, NULL))
		return -1;

	/*valid*/
	if(WILDDOG_ERR_NOERR != wilddog_getValue(wilddog, (onQueryFunc)onCallback, (void*)0))
		return -1;
	wilddog_destroy(&wilddog);
	
	return 0;
}

/*wilddog_setValue*/

int test_set()
{
	Wilddog_T wilddog = 0;
	Wilddog_Node_T * p_node;
	

	/* Invalid */
	if(WILDDOG_ERR_NOERR == wilddog_setValue(wilddog, NULL, NULL, NULL))
	{
		return -1;
	}

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	/* valid */
	if(WILDDOG_ERR_NOERR != wilddog_setValue(wilddog, NULL, NULL, NULL))
	{
		wilddog_destroy(&wilddog);
		return -1;
	}
	p_node= wilddog_node_createTrue(NULL);

	/* valid */
	if(WILDDOG_ERR_NOERR != wilddog_setValue(wilddog, p_node, NULL, NULL))
	{
		wilddog_node_delete(p_node);
		wilddog_destroy(&wilddog);
		return -1;
	}

	/* valid */
	if(WILDDOG_ERR_NOERR != wilddog_setValue(wilddog, p_node, onCallback, NULL))
	{
		wilddog_node_delete(p_node);
		wilddog_destroy(&wilddog);
		return -1;
	}

	wilddog_node_delete(p_node);
	wilddog_destroy(&wilddog);
	return 0;
}

/*wilddog_push*/

int test_push()
{
	Wilddog_T wilddog = 0;
	Wilddog_Node_T * p_node;
	

	/* Invalid */
	if(WILDDOG_ERR_NOERR == wilddog_push(wilddog, NULL, NULL, NULL))
	{
		return -1;
	}

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	/* valid */
	if(WILDDOG_ERR_NOERR != wilddog_push(wilddog, NULL, NULL, NULL))
	{
		wilddog_destroy(&wilddog);
		return -1;
	}
	p_node= wilddog_node_createTrue(NULL);

	/* valid */
	if(WILDDOG_ERR_NOERR != wilddog_push(wilddog, p_node, NULL, NULL))
	{
		wilddog_node_delete(p_node);
		wilddog_destroy(&wilddog);
		return -1;
	}

	/* valid */
	if(WILDDOG_ERR_NOERR != wilddog_push(wilddog, p_node, onCallback, NULL))
	{
		wilddog_node_delete(p_node);
		wilddog_destroy(&wilddog);
		return -1;
	}

	wilddog_node_delete(p_node);
	wilddog_destroy(&wilddog);
	return 0;

}

/* wilddog_removeValue */
int test_remove()
{
	Wilddog_T wilddog;

	
	
	/*Invalid*/
	if(WILDDOG_ERR_NOERR == wilddog_removeValue(0, NULL, NULL))
		return -1;

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	/*Invalid*/
	if(WILDDOG_ERR_NOERR == wilddog_removeValue(0, NULL, NULL))
		return -1;

	/*valid*/
	if(WILDDOG_ERR_NOERR != wilddog_removeValue(wilddog, NULL, NULL))
		return -1;

	/*valid*/
	if(WILDDOG_ERR_NOERR != wilddog_removeValue(wilddog, onCallback, NULL))
		return -1;

	/*valid*/
	if(WILDDOG_ERR_NOERR != wilddog_removeValue(wilddog, onCallback, (void*)0))
		return -1;
	wilddog_destroy(&wilddog);
	
	return 0;

}
/*wilddog_addObserver*/
int test_on()
{
	Wilddog_T wilddog = 0;
	
	

	/* Invalid */
	if(WILDDOG_ERR_NOERR == wilddog_addObserver(wilddog, 0, NULL, NULL))
	{
		return -1;
	}

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	/* valid */
	if(WILDDOG_ERR_NOERR != wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, NULL, NULL))
	{
		wilddog_destroy(&wilddog);
		return -1;
	}

	/* valid */
	if(WILDDOG_ERR_NOERR != wilddog_addObserver(wilddog, 0, NULL, NULL))
	{
		wilddog_destroy(&wilddog);
		return -1;
	}

	/* valid */
	if(WILDDOG_ERR_NOERR != wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, onCallback, NULL))
	{
		wilddog_destroy(&wilddog);
		return -1;
	}

	wilddog_destroy(&wilddog);
	return 0;

}
/*wilddog_removeObserver*/
int test_off()
{
	Wilddog_T wilddog;

	
	
	/*Invalid*/
	if(WILDDOG_ERR_NOERR == wilddog_removeObserver(0, 0))
		return -1;

	wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

	/*Invalid*/
	if(WILDDOG_ERR_NOERR == wilddog_removeObserver(0, WD_ET_VALUECHANGE))
		return -1;

	/*Invalid*/

	if(WILDDOG_ERR_NOERR == wilddog_removeObserver(wilddog, 0))
		return -1;
	
	/*Invalid*/
	if(WILDDOG_ERR_NOERR == wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE))
		return -1;

	if(WILDDOG_ERR_NOERR != wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, NULL, NULL))
	{
		wilddog_destroy(&wilddog);
		return -1;
	}
	/*valid*/
	if(WILDDOG_ERR_NOERR != wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE))
		return -1;

	if( WILDDOG_ERR_NOERR != wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, NULL, NULL)||
		WILDDOG_ERR_NOERR != wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE)			||
		WILDDOG_ERR_NOERR != wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, NULL, NULL))
	{
		wilddog_destroy(&wilddog);
		return -1;
	}

	if(WILDDOG_ERR_NOERR != wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE))
		return -1;
	wilddog_destroy(&wilddog);
	
	return 0;

}
void test_initialize()
{
	int i;
	for(i = 0; i < sizeof(test_results)/ sizeof(struct test_reult_t); i++)
	{
		memset(&(test_results[i]), 0, sizeof(struct test_reult_t));
		test_results[i].result = FALSE;
	}
}

int test_printResult()
{
	int i;
	printf("\n\nTest results:\n\n");
	for(i = 0; i < sizeof(test_results)/ sizeof(struct test_reult_t); i++)
	{
		if(test_results[i].name != NULL)
		{
			printf("%-32s\t%s\n", test_results[i].name, \
					test_results[i].result == 0? ("PASS"):("FAIL"));
			
			if(test_results[i].result != 0)
				return -1;
		}
	}
	printf("\nTest results print end!\n\n");
	fflush(stdout);
	return 0;
}

int main(void)
{
	test_initialize();

	test_results[0].name = "wilddog_initWithUrl";
	test_results[0].result = test_new();
	test_results[1].name = "wilddog_destroy";
	test_results[1].result = test_destroy();
	test_results[2].name = "wilddog_auth";
	test_results[2].result = test_auth();
	test_results[3].name = "wilddog_getParent";
	test_results[3].result = test_getParent();
	test_results[4].name = "wilddog_getRoot";
	test_results[4].result = test_getRoot();
	test_results[5].name = "wilddog_getChild";
	test_results[5].result = test_getChild();
	test_results[6].name = "wilddog_getKey";
	test_results[6].result = test_getKey();
	test_results[7].name = "wilddog_getValue";
	test_results[7].result = test_query();
	test_results[8].name = "wilddog_setValue";
	test_results[8].result = test_set();
	test_results[9].name = "wilddog_push";
	test_results[9].result = test_push();
	test_results[10].name = "wilddog_removeValue";
	test_results[10].result = test_remove();
	test_results[11].name = "wilddog_addObserver";
	test_results[11].result = test_on();
	test_results[12].name = "wilddog_removeObserver";
	test_results[12].result = test_off();

	return test_printResult();
}

