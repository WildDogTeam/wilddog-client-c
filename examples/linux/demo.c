/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: demo.c
 *
 * Description: Wilddog demo file.
 *
 * Usage: demo <operation> <-h|-l url> [--auth=<auth data>--key=<arg1>] [--value=<arg2>]
 *      
 *          operations:
 *                  getValue:       get value of the url.
 *                  setValue:       create a new node in the url, need 
 *                                  [--key=<arg1>] and [--value=<arg2>] 
 *                                  followed, arg1 means the node's key which 
 *                                  will add to url, arg2 is arg1's value.
 *                  push:           push a new node in the url, usage similar
 *                                  to <setValue>, but a little different is:
 *                                  <push> will automatically create a node by
 *                                  the server.
 *                  removeValue:    remove the value(and it's children) of the
 *                                  url.
 *                  addObserver:    subscribe the url, any change will be 
 *                                  pushed to the client.
 *                  setAuth:        send the auth token to the server, and get
 *                                  the permission to operate, if you set the
 *                                  control rules in the cloud. 
 *                                  need [--value=<token>] followed.
 *                  disSetValue : Set the device's disconnect action to cloud, when the device is 
 *                                       offline, the value will be set to the cloud.
 *                  disPush : Set the device's disconnect action to cloud, when the device is 
 *                                  offline, the value will be push to the cloud.
 *                  disRemove :Set the device's disconnect action to cloud, when the device is 
 *                                      offline, all data belong to that path will be remove in the cloud.
 *                  cacelDis :  Cancel the wilddog client's disconnect actions.
 *                  offLine : let the device offline. 
 *                  onLine :let the device online.
 *                  
 *                  --auth=<auth data> : secret key,send the aut token to server first and get
 *                                  the permission to operate, if you set the
 *                                  control rules in the cloud. 
 *          -h: help
 *          -l: note that a url followed
 *          url:
 *                  like coap://<your appid>.wilddogio.com/[path], <your appid>
 *                  is the appid of the app you created, and path is the path(
 *                  node path) in the app. if the tree like this, <1> is your 
 *                  appid, <a> and <a/b> are both path.
 *                  
 *                  your data tree in cloud:
 *
 *                  1.wilddogio.com
 *                  |
 *                  + a
 *                    |
 *                    + b: 1234
 *
 *      example: if you input :
 *                  demo setValue -l coap://1.wilddogio.com/a --key=b --value=1
 *               you can find the app <1> has a node which key is b,value is 1.
 *               and then if you input :
 *                  demo addObserver -l coap://1.wilddogio.com/a/b
 *               when you change node <b>'s value to 2, the client will receive
 *               the change, and print in the console( and then quit because of
 *               our setting, we set the var <cntmax> in main() to 10, so it 
 *               will break after 10 times, you can remove this setting.)
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.3        Baikal.Hu       2015-07-16  Create file.
 * 0.5.1        Jimmy.Pan       2015-09-22  Add notes.
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
    TEST_CMD_SETAUTH,
    TEST_CMD_ONDISSET,
    TEST_CMD_ONDISPUSH,
    TEST_CMD_ONDISREMOVE,
    TEST_CMD_CANCELDIS,
    TEST_CMD_OFFLINE,
    TEST_CMD_ONLINE
}TEST_CMD_TYPE;

STATIC void getHostFromAppid(char *p_host,const char *url)
{
    char *star_p = NULL,*end_p = NULL;
    star_p =  strchr(url,'/')+2;
    end_p = strchr(star_p,'.');
    memcpy(p_host,star_p,end_p - star_p);   
}
STATIC void getValue_callback
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err
    )
{
    *(BOOL*)arg = TRUE;

    wilddog_debug("<><> get return code : %d",err);
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("getValue fail!");
        return;
    }

    if(p_snapshot)
        wilddog_debug_printnode(p_snapshot);
    printf("\ngetValue success!\n");

    return;
}

STATIC void removeValue_callback(void* arg, Wilddog_Return_T err)
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
STATIC void setValue_callback(void* arg, Wilddog_Return_T err)
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

STATIC void push_callback(u8 *p_path,void* arg, Wilddog_Return_T err)
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

STATIC void addObserver_callback
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err
    )
{
    *(BOOL*)arg = TRUE;
     if((err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED ) &&
        err != WILDDOG_ERR_RECONNECT)
    {
        wilddog_debug("addObserver failed! error code = %d",err);
        return;
    }
    wilddog_debug("Observe new data!");
    wilddog_debug_printnode(p_snapshot);
    printf("\n");

    return;
}
STATIC void auth_callback
    (
        void* arg,
        Wilddog_Return_T err
    )
{
    *(BOOL*)arg = TRUE;
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("auth failed!;error =%d \n",err);
        return;
    }
}
STATIC void onDis_callback(void* arg, Wilddog_Return_T err)
{
                        
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("push failed");
        return;
    }       
    *(BOOL*)arg = TRUE;
    return;
}

int main(int argc, char **argv) 
{
    char url[1024];
    char value[1024];
    char keys[256];
    char host[512];    
    char authData[512];
    BOOL authFlag = FALSE;
    
    memset(url,0,sizeof(url));  
    memset(value,0,sizeof(value));
    memset(keys,0,sizeof(keys));
    memset(host,0,sizeof(host));
    memset(authData,0,sizeof(authData));
    
    int type = 0;
    int opt,i,res = 0,cnt=0,cntmax=0;
    int option_index = 0;
    BOOL isFinish = FALSE,authFinish;
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL,*p_head = NULL;

    static struct option long_options[] = 
    {
    
        {"auth",    required_argument, 0,  0 },
        {"value",   required_argument, 0,  0 },
        {"key",     required_argument, 0,  0 },
        {0,         0,                 0,  0 }
    };

    while((opt=getopt_long(argc,argv,"hl:",long_options,&option_index)) != -1)
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
                if(strcmp(long_options[option_index].name,"auth")==0)
                {
                    authFlag = TRUE;
                    memcpy(authData, optarg,strlen(optarg));
                 }
            }
            break;

        case 'h':
            fprintf(stderr, \
                 "Usage: %s setAuth|getValue|setValue|push|removeValue|addObser\n"
                "\t|disSetValue|disPush|disRemove|cacelDis|offLine|onLine\n"
                "\tver -l coap://<your appid>.wilddogio.com/ [ --auth=<auth data> "
                "--key=<key>  --value=<value>]\n",
                    argv[0]);
            return 0;
        
        case 'l':
            strcpy(url, (const char*)optarg);
            getHostFromAppid(host,url);
            sprintf(&host[strlen(host)],".wilddogio.com");
            break;          
        default: /* '?' */
            fprintf(stderr, \
               "Usage: %s setAuth|getValue|setValue|push|removeValue|addObser\n"
               "\t|disSetValue|disPush|disRemove|cacelDis|offLine|onLine\n"
               "\tver -l coap://<your appid>.wilddogio.com/ [ --auth=<auth data> "
               "--key=<key>  --value=<value>]\n",
                    argv[0]);
            return 0;
        }
    }

    for (i = 0; optind < argc; i++, optind++) 
    {
        if(i == 0)
        {
            if(strcmp(argv[optind],"getValue")==0)
            {
                type= TEST_CMD_GET;
                cntmax = 0;
            }
            else if(strcmp(argv[optind],"setValue")==0)
            {
                type= TEST_CMD_SET;
                cntmax = 0;
            }
            else if(strcmp(argv[optind],"push")==0)
            {
                type=TEST_CMD_PUSH;
                cntmax = 0;
            }
            else if(strcmp(argv[optind],"removeValue")==0)
            {
                type=TEST_CMD_DELE;
                cnt =0;
            }
            else if(strcmp(argv[optind],"addObserver")==0)
            {
                type= TEST_CMD_ON;
                cntmax = 10;
            }
            else if(strcmp(argv[optind],"setAuth")==0)
            {
                type= TEST_CMD_SETAUTH;
                cntmax = 0;
            }
            else if(strcmp(argv[optind],"disSetValue")==0)
            {
                type= TEST_CMD_ONDISSET;
                cntmax = 0;
            }
            else if(strcmp(argv[optind],"disPush")==0)
            {
                type= TEST_CMD_ONDISPUSH;
                cntmax = 0;
            }
            else if(strcmp(argv[optind],"disRemove")==0)
            {
                type= TEST_CMD_ONDISREMOVE;
                cntmax = 0;
            }
            else if(strcmp(argv[optind],"cacelDis")==0)
            {
                type= TEST_CMD_CANCELDIS;
                cntmax = 0;
            }
            else if(strcmp(argv[optind],"offLine")==0)
            {
                type= TEST_CMD_OFFLINE;
                cntmax = 0;
            }
            else if(strcmp(argv[optind],"onLine")==0)
            {
                type= TEST_CMD_ONLINE;
                cntmax = 0;
            }
        }
    }
    if( !type)
    {
        printf("Usage: %s setAuth|getValue|setValue|push|removeValue|addObser\n"
                "\t|disSetValue|disPush|disRemove|cacelDis|offLine|onLine\n"
               "\tver -l coap://<your appid>.wilddogio.com/ [ --auth=<auth data> "
               "--key=<key>  --value=<value>]\n", 
               argv[0]);
        return 0;
    }

    /*Create an node which type is an object*/
    p_head = wilddog_node_createObject(NULL);
    
    /*Create an node which type is UTF-8 Sring*/
    p_node = wilddog_node_createUString((Wilddog_Str_T *)keys, \
                                        (Wilddog_Str_T *)value);
    
    /*Add p_node to p_head, then p_node is the p_head's child node*/
    wilddog_node_addChild(p_head, p_node);

    /*Init a wilddog client*/
    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)url);
    if(authFlag == TRUE)
        res = wilddog_auth((u8*)host,(u8*)authData, \
                               strlen((const char *)authData),
                               auth_callback,(void*)&authFinish);
    switch(type)
    {
        case TEST_CMD_GET:
            /*Send the query method*/
            res = wilddog_getValue(wilddog, (onQueryFunc)getValue_callback, \
                                   (void*)&isFinish);
            break;
        case TEST_CMD_SET:  
            /*Send the set method*/
            res = wilddog_setValue(wilddog,p_head,setValue_callback, \
                                   (void*)&isFinish);
            break;
        case TEST_CMD_PUSH:
            /*Send the push method*/
            res = wilddog_push(wilddog, p_head, push_callback, \
                               (void *)&isFinish);  
            break;
        case TEST_CMD_DELE:
            /*Send the remove method*/
            res = wilddog_removeValue(wilddog, removeValue_callback, \
                                      (void*)&isFinish);
            break;
        case TEST_CMD_ON:
            /*Observe on*/
            res = wilddog_addObserver(wilddog, WD_ET_VALUECHANGE,\
                                      addObserver_callback, \
                                      (void*)&isFinish);
            break;
        case TEST_CMD_SETAUTH:
            wilddog_debug("TEST_CMD_SETAUTH token = %s",value);
            res = wilddog_auth((u8*)host,(u8*)value, \
                               strlen((const char *)value),
                               auth_callback,(void*)&isFinish);
        case TEST_CMD_ONDISSET:
            /*Send the remove method*/
            res = wilddog_onDisconnectSetValue(wilddog,p_head,onDis_callback, \
                                      (void*)&isFinish);
            break;
        case TEST_CMD_ONDISPUSH:
            /*Send the remove method*/
            res = wilddog_onDisconnectPush(wilddog,p_head,onDis_callback, \
                                      (void*)&isFinish);
            break;

       case TEST_CMD_ONDISREMOVE:
            /*Send the remove method*/
            res = wilddog_onDisconnectRemoveValue(wilddog, onDis_callback, \
                                      (void*)&isFinish);
            break;
        case TEST_CMD_CANCELDIS:
            /*Send the remove method*/
            res = wilddog_cancelDisconnectOperations(wilddog, onDis_callback, \
                                      (void*)&isFinish);
            break;

         case TEST_CMD_OFFLINE:
            /*Send the remove method*/
            res = wilddog_goOffline();
            break;
        case TEST_CMD_ONLINE:
            /*Send the remove method*/
            res = wilddog_goOnline();
            break;
    }
    /*Delete the node*/
    wilddog_node_delete(p_head);
    while(1)
    {
        if(TRUE == isFinish)
        {
            if(type ==  TEST_CMD_ON)
                wilddog_debug("get new data %d times! after %d times, will " \
                              "remove Observer.", cnt, cntmax);
            cnt++;
            isFinish = FALSE;
#if 1
            if(cnt > cntmax)/*force break when received new data.*/
            {
                if(type ==  TEST_CMD_ON)
                {
                    wilddog_debug("off the data!");
                    /*Observe off*/
                    wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE);
                }
                break;
            }
#endif
        }
        /*
         * Handle the event and callback function, it must be called in a 
         * special frequency
        */
        wilddog_trySync();
    }
    /*Destroy the wilddog clent and release the memory*/
    wilddog_destroy(&wilddog);

    return res;
}

