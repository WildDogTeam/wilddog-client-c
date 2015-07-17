/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: demo.c
 *
 * Description: Wilddog demo file.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.3        Baikal.Hu       2015-07-16  Create file.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h> 
#include "wilddog.h"

typedef enum _TEST_CMD_TYPE
{
    TEST_CMD_NON = 0,
    TEST_CMD_GET,
    TEST_CMD_SET,
    TEST_CMD_PUSH,
    TEST_CMD_DELE,
    TEST_CMD_ON,
    
}TEST_CMD_TYPE;

STATIC void test_getValueFunc
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err
    )
{
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("getValue fail!");
        return;
    }
    *(BOOL*)arg = TRUE;

    if(p_snapshot)
        wilddog_debug_printnode(p_snapshot);
    printf("\ngetValue success!\n");

    return;
}

STATIC void test_removeValueFunc(void* arg, Wilddog_Return_T err)
{
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("removeValue failed!");
        return ;
    }
    wilddog_debug("removeValue success!");
    *(BOOL*)arg = TRUE;
    return;
}
STATIC void test_setValueFunc(void* arg, Wilddog_Return_T err)
{
                        
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("setValue error!");
        return;
    }
    wilddog_debug("setValue success!");
    *(BOOL*)arg = TRUE;
    return;
}

STATIC void test_pushFunc(u8 *p_path,void* arg, Wilddog_Return_T err)
{
                        
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("push failed");
        return;
    }       
    wilddog_debug("new path is %s", p_path);
    *(BOOL*)arg = TRUE;
    return;
}

STATIC void test_addObserverFunc
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
    
    *(BOOL*)arg = TRUE;
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("addObserver failed!");
        return;
    }
    wilddog_debug_printnode(p_snapshot);
        wilddog_debug("addObserver data!");
    
    return;
}


int main(int argc, char **argv) 
{

#ifdef  WORDS_BIGENDIAN
    printf("WORDS_BIGENDIAN \n");
#endif
    char url[1024];
    char value[1024];
    char keys[256];
    char *p_inputtype = NULL;
    memset(url,0,sizeof(url));  
    memset(value,0,sizeof(value));
    memset(keys,0,sizeof(keys));
    int type = 0;
    int opt,i,res = 0,cnt=0,cntmax=0;
    int option_index = 0;
    BOOL isFinish = FALSE;
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL,*p_head = NULL;


    static struct option long_options[] = 
    {
        {"value",   required_argument, 0,  0 },
        {"key",     required_argument, 0,  0 },
        {0,         0,                 0,  0 }
    };

    while ((opt = getopt_long(argc, argv, "hl:",long_options, &option_index)) != -1) 
    {
        switch (opt) 
        {
        case 0:
            //printf("option %s", long_options[option_index].name);
            if (optarg)
            {
                if(strcmp(long_options[option_index].name,"key")==0)
                    memcpy(keys, optarg,strlen(optarg));
                if(strcmp(long_options[option_index].name,"value")==0)
                    memcpy(value, optarg,strlen(optarg));
            }
            break;

        case 'h':
            fprintf(stderr, "Usage: %s getValue|setValue|push|removeValue|addObserver -l coap://yourappid.wilddogio.com/ --key a --value 124\n",
                   argv[0]);
            return 0;
        case 'l':
            strcpy(url, (const char*)optarg);
            //printf("url : %s\n", url);
            break;          
        default: /* '?' */
            fprintf(stderr, "Usage: %s getValue|setValue|push|removeValue|addObserver -l coap://yourappid.wilddogio.com/ --key a --value 124\n",
                   argv[0]);
            return 0;
        }
    }

    for (i = 0; optind < argc; i++, optind++) 
    {
        if(i==0)
        {
            if(strcmp(argv[optind],"getValue")==0)
            {
                p_inputtype = argv[optind];
                type= TEST_CMD_GET;
                cntmax =0;
            }
            else if(strcmp(argv[optind],"setValue")==0)
            {
                p_inputtype = argv[optind];
                type= TEST_CMD_SET;
                cntmax =0;
            }
            else if(strcmp(argv[optind],"push")==0)
            {
                p_inputtype = argv[optind];
                type=TEST_CMD_PUSH;
                cntmax =0;
            }
            else if(strcmp(argv[optind],"removeValue")==0)
            {
                p_inputtype = argv[optind];
                type=TEST_CMD_DELE;
                cnt =0;
            }
            else if(strcmp(argv[optind],"addObserver")==0)
            {
                p_inputtype = argv[optind];
                type= TEST_CMD_ON;
            }
        }
    }
    if( !type)
    {
        printf("Usage: %s getValue|setValue|push|removeValue|addObserver -l coap://yourappid.wilddogio.com/ --key a --value 124\n", argv[0]);
        return 0;
    }


    /*Init wilddog SDK*/
    
    
    /*Create an node which type is an object*/
    p_head = wilddog_node_createObject(NULL);
    
    /*Create an node which type is UTF-8 Sring*/
    p_node = wilddog_node_createUString((Wilddog_Str_T *)keys,(Wilddog_Str_T *)value);
    
    /*Add p_node to p_head, then p_node is the p_head's child node*/
    wilddog_node_addChild(p_head, p_node);

    /*Init a wilddog client*/
    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)url);
    switch(type)
    {
        case TEST_CMD_GET:
            /*Send the query method*/
            res = wilddog_getValue(wilddog, (onQueryFunc)test_getValueFunc, (void*)&isFinish);
            break;
        case TEST_CMD_SET:  
            /*Send the set method*/
            res = wilddog_setValue(wilddog,p_head,test_setValueFunc,(void*)&isFinish);
            break;
        case TEST_CMD_PUSH:
            /*Send the push method*/
            res = wilddog_push(wilddog, p_head, test_pushFunc, (void *)&isFinish);  
            break;
        case TEST_CMD_DELE:
            /*Send the remove method*/
            res = wilddog_removeValue(wilddog, test_removeValueFunc, (void*)&isFinish);
            break;
        case TEST_CMD_ON:
            /*Observe on*/
            res = wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, test_addObserverFunc, (void*)&isFinish);
            break;
    }
    /*Delete the node*/
    wilddog_node_delete(p_head);
    while(1)
    {
        if(TRUE == isFinish)
        {
            wilddog_debug("get new data %d times!", cnt++);
            isFinish = FALSE;
            if(cnt > cntmax)
            {
                printf("event :%s success\n",p_inputtype);
                if(type ==  TEST_CMD_ON)
                {
                    wilddog_debug("off the data!");
                    /*Observe off*/
                    wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE);
                }
                break;
            }
        }
        /*Handle the event and callback function, it must be called in a special frequency*/
        wilddog_trySync();
    }
    /*Destroy the wilddog clent and release the memory*/
    wilddog_destroy(&wilddog);

    return res;
}
