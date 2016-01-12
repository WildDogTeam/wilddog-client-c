/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: test_limit.c
 *
 * Description: wilddog API boundary testing.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Jimmy.Pan       2015-05-15  Create file.
 * 0.8.0        Jimmy.Pan       2016-01-08  Add new API's tests.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wilddog.h"

struct test_reult_t
{
    char* name;
    Wilddog_Func_T func;
    int result;
};

STATIC const char* UNUSED_URL="coap://coap.wilddogio.com/unused";
STATIC const char* UNUSED_URL_HOST="coap.wilddogio.com";
STATIC const char* UNUSED_URL_PATH="/unused";
STATIC const char* UNUSED_URL_KEY="unused"; /*the url's key*/

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

/*wilddog_auth && wilddog_unauth*/
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
    Wilddog_Str_T *path = NULL;
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
    /*check path is /a/b?*/
    path = wilddog_getPath(parent);
    if(NULL == path)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&parent);
        return -1;
    }
    if(strcmp((const char *)path, "/a/b") != 0)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&parent);
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
    
    /*check path is /a?*/
    path = wilddog_getPath(parent2);
    if(NULL == path)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&parent2);
        return -1;
    }
    if(strcmp((const char *)path, "/a") != 0)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&parent2);
        return -1;
    }
    
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
    /*check path is /?*/
    path = wilddog_getPath(parent);
    if(NULL == path)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&parent);
        return -1;
    }
    if(strcmp((const char *)path, "/") != 0)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&parent);
        return -1;
    }

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
    Wilddog_Str_T *path = NULL;
    /* Invalid, should be 0 */
    root = wilddog_getParent(wilddog);
    if(root)
        return -1;

    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)"coap://coap.wilddogio.com/a/b");

    /* should be / */
    root = wilddog_getRoot(wilddog);
    if(!root)
    {
        wilddog_debug("test_getRoot fail");
        wilddog_destroy(&wilddog);
        return -1;
    }
    /*check path is /?*/
    path = wilddog_getPath(root);
    if(NULL == path)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&root);
        return -1;
    }
    if(strcmp((const char *)path, "/") != 0)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&root);
        return -1;
    }
    /* should be as same as root */
    root2 = wilddog_getRoot(root);

    if(root != root2)
    {
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
    Wilddog_Str_T *path = NULL;
    Wilddog_Str_T *localkey = NULL;
    Wilddog_Str_T currentKey[256] = {0};
    /* not inited, should be 0 */
    child = wilddog_getChild(wilddog, NULL);
    if(child)
        return -1;
    
    /* not inited, should be 0 */
    child = wilddog_getChild(wilddog, (Wilddog_Str_T *)"a");
    if(child)
        return -1;

    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);
    localkey = wilddog_getPath(wilddog);
    
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
    /*check path is valid */
    path = wilddog_getPath(child);
    if(NULL == path)
    {
        goto GET_CHILD_END;
    }
    memset(currentKey, 0 , 256);
    
    sprintf((char*)currentKey, "%s%s",localkey, "/a");
    if(strcmp((const char *)path, (const char *)currentKey) != 0)
    {
        wilddog_debug("path = %s,currentKey = %s", path, currentKey);
        goto GET_CHILD_END;
    }
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
    /*check path is valid */
    path = wilddog_getPath(child);
    if(NULL == path)
    {
        goto GET_CHILD_END;
    }
    memset(currentKey, 0 , 256);
    sprintf((char*)currentKey, "%s%s",localkey, "/a/b");
    if(strcmp((const char *)path, (const char *)currentKey) != 0)
    {
        wilddog_debug("path = %s,currentKey = %s", path, currentKey);
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
        return -1;
    }

    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

    /* valid */
    key = wilddog_getKey(wilddog);
    if(!key)
    {
        return -1;
    }
    if(strcmp((const char*)key, UNUSED_URL_KEY) != 0)
    {
        wilddog_destroy(&wilddog);
        return -1;
    }

    root = wilddog_getRoot(wilddog);

    /* valid */
    key = wilddog_getKey(root);
    if(!key)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&root);
        return -1;
    }
    if(strcmp((const char*)key, "/") != 0)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&root);
        return -1;
    }

    wilddog_destroy(&wilddog);
    wilddog_destroy(&root);
    return 0;
}

/*wilddog_getHost*/
int test_getHost()
{
    Wilddog_Str_T *key = NULL;
    Wilddog_T wilddog, root;

    /* Invalid */
    key = wilddog_getHost(0);

    if(key)
    {
        return -1;
    }
    
    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

    /* valid */
    key = wilddog_getHost(wilddog);
    if(!key)
    {
        wilddog_destroy(&wilddog);
        return -1;
    }
    
    if(strcmp(UNUSED_URL_HOST, (const char*)key) != 0)
    {
        wilddog_debug("gethost = %s, real host = %s\n", key, UNUSED_URL_HOST);
        wilddog_destroy(&wilddog);
        return -1;
    }

    root = wilddog_getRoot(wilddog);

    /* valid */
    key = wilddog_getHost(root);
    if(!key)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&root);
        return -1;
    }
    
    if(strcmp(UNUSED_URL_HOST, (const char*)key) != 0)
    {
        wilddog_debug("gethost = %s, real host = %s\n", key, UNUSED_URL_HOST);
        wilddog_destroy(&wilddog);
        return -1;
    }

    wilddog_destroy(&wilddog);
    wilddog_destroy(&root);
    return 0;
}
/*wilddog_getPath*/
int test_getPath()
{
    Wilddog_Str_T *key = NULL;
    Wilddog_T wilddog, root;

    /* Invalid */
    key = wilddog_getPath(0);

    if(key)
    {
        return -1;
    }

    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

    /* valid */
    key = wilddog_getPath(wilddog);
    if(!key)
    {
        wilddog_destroy(&wilddog);
        return -1;
    }
    if(strcmp(UNUSED_URL_PATH, (const char*)key) != 0)
    {
        wilddog_debug("gethost = %s, real host = %s\n", key, UNUSED_URL_PATH);
        wilddog_destroy(&wilddog);
        wilddog_destroy(&root);
        return -1;
    }

    root = wilddog_getRoot(wilddog);

    /* valid */
    key = wilddog_getPath(root);
    if(!key)
    {
        wilddog_destroy(&wilddog);
        wilddog_destroy(&root);
        return -1;
    }
    if(strcmp("/", (const char*)key) != 0)
    {
        wilddog_debug("gethost = %s, real host = %s\n", key, "/");
        wilddog_destroy(&wilddog);
        wilddog_destroy(&root);
        return -1;
    }

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

    /* invalid */
    if(WILDDOG_ERR_NOERR == wilddog_setValue(wilddog, NULL, NULL, NULL))
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

    /* invalid */
    if(WILDDOG_ERR_NOERR == wilddog_push(wilddog, NULL, NULL, NULL))
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
        WILDDOG_ERR_NOERR != wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE)         ||
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
/*wilddog_goOnline*/
int test_goOnline()
{
    wilddog_goOnline();
    return 0;
}

/*wilddog_goOffline*/
int test_goOffline()
{
    wilddog_goOffline();
    return 0;
}

/*wilddog_onDisconnectSetValue*/
int test_disconn_set()
{
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node;
    
    /* Invalid */
    if(WILDDOG_ERR_NOERR == wilddog_onDisconnectSetValue(wilddog, NULL, NULL, NULL))
    {
        return -1;
    }

    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

    /* invalid */
    if(WILDDOG_ERR_NOERR == wilddog_onDisconnectSetValue(wilddog, NULL, NULL, NULL))
    {
        wilddog_destroy(&wilddog);
        return -1;
    }
    p_node= wilddog_node_createTrue(NULL);

    /* valid */
    if(WILDDOG_ERR_NOERR != wilddog_onDisconnectSetValue(wilddog, p_node, NULL, NULL))
    {
        wilddog_node_delete(p_node);
        wilddog_destroy(&wilddog);
        return -1;
    }

    /* valid */
    if(WILDDOG_ERR_NOERR != wilddog_onDisconnectSetValue(wilddog, p_node, onCallback, NULL))
    {
        wilddog_node_delete(p_node);
        wilddog_destroy(&wilddog);
        return -1;
    }

    wilddog_node_delete(p_node);
    wilddog_destroy(&wilddog);
    return 0;
}

/*wilddog_onDisconnectPush*/
int test_disconn_push()
{
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node;
    

    /* Invalid */
    if(WILDDOG_ERR_NOERR == wilddog_onDisconnectPush(wilddog, NULL, NULL, NULL))
    {
        return -1;
    }

    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

    /* invalid */
    if(WILDDOG_ERR_NOERR == wilddog_onDisconnectPush(wilddog, NULL, NULL, NULL))
    {
        wilddog_destroy(&wilddog);
        return -1;
    }
    p_node= wilddog_node_createTrue(NULL);

    /* valid */
    if(WILDDOG_ERR_NOERR != wilddog_onDisconnectPush(wilddog, p_node, NULL, NULL))
    {
        wilddog_node_delete(p_node);
        wilddog_destroy(&wilddog);
        return -1;
    }

    /* valid */
    if(WILDDOG_ERR_NOERR != wilddog_onDisconnectPush(wilddog, p_node, onCallback, NULL))
    {
        wilddog_node_delete(p_node);
        wilddog_destroy(&wilddog);
        return -1;
    }

    wilddog_node_delete(p_node);
    wilddog_destroy(&wilddog);
    return 0;

}

/*wilddog_onDisconnectRemoveValue*/
int test_disconn_rmv()
{
    Wilddog_T wilddog;

    /*Invalid*/
    if(WILDDOG_ERR_NOERR == wilddog_onDisconnectRemoveValue(0, NULL, NULL))
        return -1;

    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

    /*Invalid*/
    if(WILDDOG_ERR_NOERR == wilddog_onDisconnectRemoveValue(0, NULL, NULL))
        return -1;

    /*valid*/
    if(WILDDOG_ERR_NOERR != wilddog_onDisconnectRemoveValue(wilddog, NULL, NULL))
        return -1;

    /*valid*/
    if(WILDDOG_ERR_NOERR != wilddog_onDisconnectRemoveValue(wilddog, onCallback, NULL))
        return -1;

    /*valid*/
    if(WILDDOG_ERR_NOERR != wilddog_onDisconnectRemoveValue(wilddog, onCallback, (void*)0))
        return -1;
    wilddog_destroy(&wilddog);
    
    return 0;
}

/*wilddog_cancelDisconnectOperations*/
int test_disconn_cancel()
{
    Wilddog_T wilddog;

    /*Invalid*/
    if(WILDDOG_ERR_NOERR == wilddog_cancelDisconnectOperations(0, NULL, NULL))
        return -1;

    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)UNUSED_URL);

    /*Invalid*/
    if(WILDDOG_ERR_NOERR == wilddog_cancelDisconnectOperations(0, NULL, NULL))
        return -1;

    /*valid*/
    if(WILDDOG_ERR_NOERR != wilddog_cancelDisconnectOperations(wilddog, NULL, NULL))
        return -1;

    /*valid*/
    if(WILDDOG_ERR_NOERR != wilddog_cancelDisconnectOperations(wilddog, (onDisConnectFunc)onCallback, NULL))
        return -1;

    /*valid*/
    if(WILDDOG_ERR_NOERR != wilddog_cancelDisconnectOperations(wilddog, (onDisConnectFunc)onCallback, (void*)0))
        return -1;
    wilddog_destroy(&wilddog);
    
    return 0;
}
/*wilddog_node_createObject*/
int test_node_createObj()
{
	Wilddog_Node_T * node = NULL;
	Wilddog_Str_T data[10] = {0};
	//int i = 0;
	/*valid, key == NULL*/
	node = wilddog_node_createObject(NULL);
	if(!node)
		return -1;
	wilddog_node_delete(node);
	
	/*valid, key is a string*/
	node = wilddog_node_createObject((Wilddog_Str_T*)"hello");
	if(!node)
		return -1;
	wilddog_node_delete(node);

	/*valid, key == "/a/b"*/
	memset(data, 0, 10);
	data[0] = '/';
	data[1] = 'a';
	data[2] = '/';
	data[3] = 'b';
	node = wilddog_node_createObject(data);
	if(!node)
	{
		return -1;
	}
    if(strcmp((const char*)node->p_wn_key, "a")!= 0 || \
       strcmp((const char*)node->p_wn_child->p_wn_key, "b")!= 0)
    {
        wilddog_node_delete(node);
        return -1;
    }
	wilddog_node_delete(node);
	
	/*invalid, key == "//"*/
	memset(data, 0, 10);
	data[0] = '/';
	data[1] = '/';
	node = wilddog_node_createObject(data);
	if(node)
	{
		return -1;
	}
	
	/*invalid input: key is not string*/
/*	memset(data, 0, 10);
	for(i = 0; i < 10; i++)
		data[i] = 127;
	node = wilddog_node_createObject(data);
	if(node)
	{
		return -1;
	}
	*/
	/*invalid input: key[0] == 0*/
	memset(data, 0, 10);
	data[1] = 1;
	node = wilddog_node_createObject(data);
	if(node)
	{
		return -1;
	}

	/*invalid input: key[0] < 32*/
	memset(data, 0, 10);
	data[0] = 1;
	data[1] = 0;
	node = wilddog_node_createObject(data);
	if(node)
	{
		return -1;
	}
	
	/*invalid input: key[0...n] == '.'*/
	memset(data, 0, 10);
	data[0] = 'a';
	data[1] = '.';
	node = wilddog_node_createObject(data);
	if(node)
	{
		return -1;
	}

	/*invalid input: key[0...n] == '$'*/
	memset(data, 0, 10);
	data[0] = 'a';
	data[1] = '$';
	node = wilddog_node_createObject(data);
	if(node)
	{
		return -1;
	}
	
	/*invalid input: key[0...n] == '#'*/
	memset(data, 0, 10);
	data[0] = 'a';
	data[1] = '#';
	node = wilddog_node_createObject(data);
	if(node)
	{
		return -1;
	}
	
	/*invalid input: key[0...n] == '['*/
	memset(data, 0, 10);
	data[0] = 'a';
	data[1] = '[';
	node = wilddog_node_createObject(data);
	if(node)
	{
		return -1;
	}
	
	/*invalid input: key[0...n] == ']'*/
	memset(data, 0, 10);
	data[0] = 'a';
	data[1] = ']';
	node = wilddog_node_createObject(data);
	if(node)
	{
		return -1;
	}
	return 0;
}
/*wilddog_node_createUString*/
int test_node_createUstr()
{
	Wilddog_Node_T * node = NULL;

	/*valid*/
	node = wilddog_node_createUString(NULL, (Wilddog_Str_T*)"hello");
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	
	/*valid*/
	node = wilddog_node_createUString((Wilddog_Str_T*)"aaa", (Wilddog_Str_T*)"hello");
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	
	/*invalid*/
	node = wilddog_node_createUString((Wilddog_Str_T*)"aaa", NULL);
	if(node)
	{
		return -1;
	}

	/*invalid*/
	node = wilddog_node_createUString(NULL, NULL);
	if(node)
	{
		return -1;
	}
	return 0;
}
/*wilddog_node_createBString*/
int test_node_createBstr()
{
	Wilddog_Node_T * node = NULL;
	Wilddog_Str_T data[10] = {0};
	int i ;
	/*valid*/
	node = wilddog_node_createBString(NULL, (Wilddog_Str_T*)"hello", strlen("hello"));
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	
	/*valid*/
	for(i = 0; i < 10; i++)
	{
		data[i] = i;
	}
	node = wilddog_node_createBString((Wilddog_Str_T*)"aaa", data, 10);
	if(!node)
	{
		return -1;
	}
	
	wilddog_node_delete(node);
	
	return 0;
}
/*wilddog_node_createFloat*/
int test_node_createFloat()
{
	Wilddog_Node_T * node = NULL;
	
	/*valid*/
	node = wilddog_node_createFloat(NULL, 1.1);
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);

	/*valid*/
	node = wilddog_node_createFloat((Wilddog_Str_T*)"hello", 1.1);
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	return 0;
}
/*wilddog_node_createNum*/
int test_node_createNum()
{
	Wilddog_Node_T * node = NULL;
	
	/*valid*/
	node = wilddog_node_createNum(NULL, 1);
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	
	/*valid*/
	node = wilddog_node_createNum((Wilddog_Str_T*)"hello", -2);
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	return 0;
}
/*wilddog_node_createNull*/
int test_node_createNull()
{
	Wilddog_Node_T * node = NULL;
	
	/*valid*/
	node = wilddog_node_createNull(NULL);
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	
	/*valid*/
	node = wilddog_node_createNull((Wilddog_Str_T*)"hello");
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	return 0;
}

/*wilddog_node_createTrue*/
int test_node_createTrue()
{
	Wilddog_Node_T * node = NULL;
	
	/*valid*/
	node = wilddog_node_createTrue(NULL);
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	
	/*valid*/
	node = wilddog_node_createTrue((Wilddog_Str_T*)"hello");
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	return 0;
}
/*wilddog_node_createFalse*/
int test_node_createFalse()
{
	Wilddog_Node_T * node = NULL;
	
	/*valid*/
	node = wilddog_node_createFalse(NULL);
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	
	/*valid*/
	node = wilddog_node_createFalse((Wilddog_Str_T*)"hello");
	if(!node)
	{
		return -1;
	}
	wilddog_node_delete(node);
	return 0;
}
/*wilddog_node_getValue*/
int test_node_getValue()
{
	Wilddog_Node_T * node = NULL;
	Wilddog_Str_T * value = NULL;
	int len = 0;

	node = wilddog_node_createUString(NULL, (Wilddog_Str_T*)"hello");
	
	/*invalid*/
	value = wilddog_node_getValue(NULL, NULL);
	if(value)
	{
		wilddog_node_delete(node);
		return -1;
	}
	
	/*invalid*/
	value = wilddog_node_getValue(node, NULL);
	if(value)
	{
		wilddog_node_delete(node);
		return -1;
	}
	
	/*valid*/
	value = wilddog_node_getValue(node, &len);
	if(!value)
	{
		wilddog_node_delete(node);
		return -1;
	}

	wilddog_node_delete(node);
	return 0;
}
/*wilddog_node_setValue*/
int test_node_setValue()
{
	Wilddog_Node_T * node = NULL;

	int ret = -1;
	
	node = wilddog_node_createUString(NULL, (Wilddog_Str_T*)"hello");
	
	/*invalid*/
	ret = wilddog_node_setValue(NULL, NULL, 0);
	if(0 == ret)
	{
		ret = -1;
		goto N_SETVALUE_END;
	}
	
	/*invalid*/
	ret = wilddog_node_setValue(node, NULL, 0);
	if(0 == ret)
	{
		ret = -1;
		goto N_SETVALUE_END;
	}
	
	/*invalid*/
	ret = wilddog_node_setValue(node, (Wilddog_Str_T*)"hello", 0);
	if(0 == ret)
	{
		ret = -1;
		goto N_SETVALUE_END;
	}

	/*valid*/
	ret = wilddog_node_setValue(node, (Wilddog_Str_T*)"aaa", strlen("aaa"));
	if(0 != ret)
	{
		ret = -1;
		goto N_SETVALUE_END;
	}
	if(strcmp("aaa", (const char*)node->p_wn_value) != 0)
	{
		goto N_SETVALUE_END;
	}
	ret = 0;
N_SETVALUE_END:
	wilddog_node_delete(node);
	return ret;
}
/*wilddog_node_addChild*/
int test_node_addChild()
{
	int ret = -1;
	Wilddog_Node_T *parent = NULL, *child = NULL, *child2 = NULL;
	
	parent = wilddog_node_createFalse(NULL);
	child = wilddog_node_createNum(NULL, 1);
	child2 = wilddog_node_createUString((Wilddog_Str_T*)"aaa", (Wilddog_Str_T*)"bbb");
	/*invalid*/
	ret = wilddog_node_addChild(NULL, NULL);
	if(0 == ret)
		goto N_ADDCHILD_END;

	/*invalid*/
	ret = wilddog_node_addChild(parent, NULL);
	if(0 == ret)
		goto N_ADDCHILD_END;
	
	/*invalid*/
	ret = wilddog_node_addChild(NULL, child);
	if(0 == ret)
		goto N_ADDCHILD_END;
	
	/*valid*/
	ret = wilddog_node_addChild(parent, child);
	if(0 != ret)
		goto N_ADDCHILD_END;
	
	if(parent->p_wn_child != child)
	{
		ret = -1;
		goto N_ADDCHILD_END;
	}
	
	/*valid*/
	ret = wilddog_node_addChild(parent, child2);
	if(0 != ret)
		goto N_ADDCHILD_END;
	
	/*head insert, new node is head*/
	if(parent->p_wn_child != child2)
	{
		wilddog_node_delete(parent);
		wilddog_node_delete(child2);
		return -1;
	}
	
	/*if add succeed, child will be free when parent free*/
	wilddog_node_delete(parent);
	return 0;
	
N_ADDCHILD_END:
	wilddog_node_delete(parent);
	wilddog_node_delete(child);
	wilddog_node_delete(child2);
	return ret;
}
/*wilddog_node_delete*/
int test_node_delete()
{
	int ret = -1;
	Wilddog_Node_T *parent = NULL, *child = NULL, *child2 = NULL;
	Wilddog_Node_T *clone = NULL;
	
	parent = wilddog_node_createFalse(NULL);
	child = wilddog_node_createNum(NULL, 1);
	child2 = wilddog_node_createUString((Wilddog_Str_T*)"aaa", (Wilddog_Str_T*)"bbb");
	
	wilddog_node_addChild(parent, child);
	wilddog_node_addChild(parent, child2);
	clone = wilddog_node_clone(child2);
	wilddog_node_addChild(child, clone);
	
	/*invalid*/
	ret = wilddog_node_delete(NULL);
	if(0 == ret)
	{
		wilddog_node_delete(parent);
		return -1;
	}
	
	/*valid*/
	ret = wilddog_node_delete(parent);
	if(0 != ret)
	{
		return -1;
	}

	return 0;
}
/*wilddog_node_clone*/
int test_node_clone()
{
	int ret = -1;
	Wilddog_Node_T *parent = NULL, *child = NULL, *child2 = NULL;
	Wilddog_Node_T *clone = NULL;
	
	parent = wilddog_node_createFalse((Wilddog_Str_T*)"head");
	child = wilddog_node_createNum((Wilddog_Str_T*)"a", 1);
	child2 = wilddog_node_createUString((Wilddog_Str_T*)"bbb", (Wilddog_Str_T*)"123");
	
	wilddog_node_addChild(parent, child);
	wilddog_node_addChild(parent, child2);
	/*head insert, so now parent->p_wn_child is child2*/
	
	/*invalid*/
	clone = wilddog_node_clone(NULL);
	if(clone)
		goto N_CLONE_END;

	/*valid*/
	clone = wilddog_node_clone(child);
	if(!clone)
	{
		goto N_CLONE_END;
	}
	if(
	    strcmp((const char*)clone->p_wn_key, (const char*)child->p_wn_key) != 0 ||
	    clone->d_wn_type != child->d_wn_type || \
		clone->d_wn_len != child->d_wn_len || \
		memcmp((const char*)clone->p_wn_value, (const char*)child->p_wn_value, child->d_wn_len) != 0
		)
	{
		wilddog_node_delete(clone);
		goto N_CLONE_END;
	}
	wilddog_node_delete(clone);
	
	/*valid*/
	clone = wilddog_node_clone(parent);
	if(!clone)
	{
		goto N_CLONE_END;
	}
	/*parent is ok?*/
	if(
	    strcmp((const char*)clone->p_wn_key, (const char*)parent->p_wn_key) != 0 ||
	    clone->d_wn_type != parent->d_wn_type || \
		clone->d_wn_len != parent->d_wn_len || \
		memcmp((const char*)clone->p_wn_value, (const char*)parent->p_wn_value, parent->d_wn_len) != 0
		)
	{
		wilddog_node_delete(clone);
		goto N_CLONE_END;
	}
	/*children ok?*/
	if(
	    strcmp((const char*)clone->p_wn_child->p_wn_key, (const char*)child2->p_wn_key) != 0 ||
	    clone->p_wn_child->d_wn_type != child2->d_wn_type || \
		clone->p_wn_child->d_wn_len != child2->d_wn_len || \
		memcmp((const char*)clone->p_wn_child->p_wn_value, (const char*)child2->p_wn_value, child2->d_wn_len) != 0
		)
	{
		wilddog_node_delete(clone);
		goto N_CLONE_END;
	}
	if(
	    strcmp((const char*)clone->p_wn_child->p_wn_next->p_wn_key, (const char*)child->p_wn_key) != 0 ||
	    clone->p_wn_child->p_wn_next->d_wn_type != child->d_wn_type || \
		clone->p_wn_child->p_wn_next->d_wn_len != child->d_wn_len || \
		memcmp((const char*)clone->p_wn_child->p_wn_next->p_wn_value, (const char*)child->p_wn_value, child->d_wn_len) != 0
		)
	{
		wilddog_node_delete(clone);
		goto N_CLONE_END;
	}
	wilddog_node_delete(clone);
	
	ret = 0;
N_CLONE_END:
	wilddog_node_delete(parent);
	return ret;
}
/*wilddog_node_find*/
int test_node_find()
{
	int ret = -1;
	Wilddog_Node_T *parent = NULL, *child = NULL, *child2 = NULL, *grandson = NULL;
	Wilddog_Node_T *clone = NULL;
	
	parent = wilddog_node_createFalse((Wilddog_Str_T*)"head");
	child = wilddog_node_createNum((Wilddog_Str_T*)"a", 1);
	child2 = wilddog_node_createUString((Wilddog_Str_T*)"bbb", (Wilddog_Str_T*)"123");
	
	wilddog_node_addChild(parent, child);
	wilddog_node_addChild(parent, child2);
	grandson = wilddog_node_clone(child2);
	wilddog_node_addChild(child, grandson);
	/*head insert, so now parent->p_wn_child is child2*/
	
	/*
	 * now tree is:
	 *				head
	 *				/   \
	 *			  bbb    a
	 *             |     /
	 *            123   bbb
	 *					 |
	 *					123
	*/
	
	/*invalid*/
	clone = wilddog_node_find(NULL, NULL);
	if(clone)
	{
		goto N_FIND_END;
	}
	
	/*invalid*/
	clone = wilddog_node_find(parent, NULL);
	if(clone)
	{
		goto N_FIND_END;
	}
	
	/*valid*/
	clone = wilddog_node_find(parent, "a");
	if(!clone)
	{
		goto N_FIND_END;
	}
	if(clone != child)
	{
		wilddog_debug("error!");
		goto N_FIND_END;
	}
	
	/*valid*/
	clone = wilddog_node_find(parent, "/a");
	if(!clone)
	{
		goto N_FIND_END;
	}
	if(clone != child)
	{
		wilddog_debug("error!");
		goto N_FIND_END;
	}
	
	/*valid*/
	clone = wilddog_node_find(parent, "a/bbb");
	if(!clone)
	{
		goto N_FIND_END;
	}
	if(clone != grandson)
	{
		wilddog_debug("error!");
		goto N_FIND_END;
	}
	
	/*valid*/
	clone = wilddog_node_find(parent, "/a/bbb");
	if(!clone)
	{
		goto N_FIND_END;
	}
	if(clone != grandson)
	{
		wilddog_debug("error!");
		goto N_FIND_END;
	}
	ret = 0;
N_FIND_END:
	wilddog_node_delete(parent);
	return ret;
}

struct test_reult_t test_results[] = 
{
    {"wilddog_initWithUrl",                 test_new,               0},
    {"wilddog_destroy",                     test_destroy,           0},
    {"wilddog_auth",                        test_auth,              0},
    {"wilddog_getParent",                   test_getParent,         0},
    {"wilddog_getRoot",                     test_getRoot,           0},
    {"wilddog_getChild",                    test_getChild,          0},
    {"wilddog_getKey",                      test_getKey,            0},
    {"wilddog_getHost",                     test_getHost,           0},
    {"wilddog_getPath",                     test_getPath,           0},
    {"wilddog_goOnline",                    test_goOnline,          0},
    {"wilddog_goOffline",                   test_goOffline,         0},
    {"wilddog_getValue",                    test_query,             0},
    {"wilddog_setValue",                    test_set,               0},
    {"wilddog_push",                        test_push,              0},
    {"wilddog_removeValue",                 test_remove,            0},
    {"wilddog_addObserver",                 test_on,                0},
    {"wilddog_removeObserver",              test_off,               0},
    {"wilddog_onDisconnectSetValue",        test_disconn_set,       0},
    {"wilddog_onDisconnectPush",            test_disconn_push,      0},
    {"wilddog_onDisconnectRemoveValue",     test_disconn_rmv,       0},
    {"wilddog_cancelDisconnectOperations",  test_disconn_cancel,    0},
	{"wilddog_node_createObject",			test_node_createObj,	0},
	{"wilddog_node_createUString", 			test_node_createUstr,	0},
	{"wilddog_node_createBString",			test_node_createBstr,	0},
	{"wilddog_node_createFloat", 			test_node_createFloat,	0},
	{"wilddog_node_createNum",				test_node_createNum,	0},
	{"wilddog_node_createNull",				test_node_createNull,	0},
	{"wilddog_node_createTrue",				test_node_createTrue,	0},
	{"wilddog_node_createFalse",			test_node_createFalse,	0},
	{"wilddog_node_getValue",				test_node_getValue,		0},
	{"wilddog_node_setValue",				test_node_setValue,		0},
	{"wilddog_node_addChild",				test_node_addChild,		0},
	{"wilddog_node_delete",					test_node_delete,		0},
	{"wilddog_node_clone",					test_node_clone,		0},
	{"wilddog_node_find",					test_node_find,			0},
    {NULL, NULL, -1},
};

int test_printResult()
{
    int i;
    printf("\n\nTest results:\n\n");
    for(i = 0; i < sizeof(test_results)/ sizeof(struct test_reult_t); i++)
    {
        if(test_results[i].name != NULL)
            test_results[i].result = test_results[i].func();
    }
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
    return test_printResult();
}

