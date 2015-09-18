/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_api.c
 *
 * Description: Wilddog API functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Jimmy.Pan       2015-05-15  Create file.
 *
 */

#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdio.h>
#endif
#include <stdlib.h>

#include "wilddog.h"
#include "wilddog_ct.h"
#include "wilddog_common.h"

/*
 * Function:    wilddog_increaseTime
 * Description: Optional, user defined time increase. Because SDK need a 
 *              time to ageing request packets, if you have a requirement
 *              in accuracy,  you should call this func, in a timer or other 
 *              methods.
 * Input:       ms: time want to increase, in mili second
 * Output:      N/A
 * Return:      N/A
*/
void wilddog_increaseTime(u32 ms)
{
    _wilddog_setTimeIncrease(ms);
    return;
}

/*
 * Function:    wilddog_trySync
 * Description: When called, try to sync interal time and receive data from 
 *              internet.
 * Input:       N/A
 * Output:      N/A
 * Return:      N/A
*/
void wilddog_trySync(void)
{
    _wilddog_ct_ioctl(WILDDOG_APICMD_SYNC, NULL, 0);
}

/*
 * Function:    wilddog_initWithUrl
 * Description: Init a wilddog client. A client is the path in the HOST tree.
 * Input:       url: A url such as coaps://<appid>.wilddogio.com/<path>
 * Output:      N/A
 * Return:      Id of the client.
*/
Wilddog_T wilddog_initWithUrl(Wilddog_Str_T *url)
{
    wilddog_assert(url, 0);
    
    return (Wilddog_T )_wilddog_ct_ioctl(WILDDOG_APICMD_CREATEREF, url,0);
}

/*
 * Function:    wilddog_destroy
 * Description: Destory a wilddog client.
 * Input:       p_wilddog: a pointer which point to the client id.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
Wilddog_Return_T wilddog_destroy(Wilddog_T *p_wilddog)
{
    wilddog_assert(p_wilddog, 0);

    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_DESTROYREF, \
                                               p_wilddog,0);
}

/*
 * Function:    wilddog_auth
 * Description: Set the auth data to a host(such as aaa.wilddogio.com).
 * Input:       p_host: a pointer to host .
 *              p_auth: the auth data
 *              len: the auth data length
 *              onAuth: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
 * Others:      Input url such as appId.wilddogio.com
*/
Wilddog_Return_T wilddog_auth
    (
    Wilddog_Str_T * p_host, 
    u8 *p_auth, 
    int len, 
    onAuthFunc onAuth, 
    void* args
    )
{
    Wilddog_Arg_SetAuth_T arg;

    wilddog_assert(p_host, WILDDOG_ERR_NULL);
    wilddog_assert(p_auth, WILDDOG_ERR_NULL);
    
    arg.p_host = p_host;
    arg.p_auth = p_auth;
    arg.d_len = len;
    arg.onAuth = onAuth;
    arg.arg = args;

    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_SETAUTH, &arg,0);
}

/*
 * Function:    wilddog_unauth
 * Description: Unauth to a host(such as aaa.wilddogio.com).
 * Input:       p_host: a pointer to host .
 *              onAuth: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
 * Others:      Input url such as appId.wilddogio.com
*/
Wilddog_Return_T wilddog_unauth
    (
    Wilddog_Str_T * p_host, 
    onAuthFunc onAuth, 
    void* args
    )
{
    Wilddog_Arg_SetAuth_T arg;

    wilddog_assert(p_host, WILDDOG_ERR_NULL);
    
    arg.p_host = p_host;
    arg.p_auth = NULL;
    arg.d_len = 0;
    arg.onAuth = onAuth;
    arg.arg = args;

    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_SETAUTH, &arg,0);
}

/*
 * Function:    wilddog_getValue
 * Description: Get the data of the client from server.
 * Input:       wilddog: the id of wilddog client.
 *              callback: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
Wilddog_Return_T wilddog_getValue
    (
    Wilddog_T wilddog, 
    onQueryFunc callback, 
    void* arg
    )
{
    Wilddog_Arg_Query_T args;

    wilddog_assert(wilddog, WILDDOG_ERR_NULL);
    
    args.p_ref = wilddog;
    args.p_callback = (Wilddog_Func_T)callback;
    args.arg = arg;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_QUERY, &args,0);
}

/*
 * Function:    wilddog_setValue
 * Description: Post the data of the client to server.
 * Input:       wilddog: Id of the client.
 *              p_node: a point to node(Wilddog_Node_T structure), you can
 *                      create a node tree by call node APIs.
 *              callback: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
Wilddog_Return_T wilddog_setValue
    (
    Wilddog_T wilddog, 
    Wilddog_Node_T *p_node, 
    onSetFunc callback, 
    void* arg
    )
{
    Wilddog_Arg_Set_T args;
    
    wilddog_assert(wilddog, WILDDOG_ERR_NULL);
    
    args.p_ref = wilddog;
    args.p_node = p_node;
    args.p_callback = (Wilddog_Func_T)callback;
    args.arg = arg;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_SET, &args,0);
}

/*
 * Function:    wilddog_push
 * Description: Push the data of the client to server.
 * Input:       wilddog: Id of the client.
 *              p_node: a point to node(Wilddog_Node_T structure), you can 
 *                      create a node tree by call node APIs.
 *              callback: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
Wilddog_Return_T wilddog_push
    (
    Wilddog_T wilddog,
    Wilddog_Node_T *p_node, 
    onPushFunc callback,
    void* arg
    )
{
    Wilddog_Arg_Push_T args;
    
    wilddog_assert(wilddog, WILDDOG_ERR_NULL);
    
    args.p_ref = wilddog;
    args.p_node = p_node;
    args.p_callback = (Wilddog_Func_T)callback;
    args.arg = arg;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_PUSH, &args,0);
}

/*
 * Function:    wilddog_removeValue
 * Description: Remove the data of the client from server.
 * Input:       wilddog: Id of the client.
 *              callback: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
Wilddog_Return_T wilddog_removeValue
    (
    Wilddog_T wilddog,
    onRemoveFunc callback, 
    void* arg
    )
{
    Wilddog_Arg_Remove_T args;
    
    wilddog_assert(wilddog, WILDDOG_ERR_NULL);

    args.p_ref = wilddog;
    args.p_callback = (Wilddog_Func_T)callback;
    args.arg = arg;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_REMOVE, &args,0);
}

/*
 * Function:    wilddog_addObserver
 * Description: Subscibe the client's data change, if data changed, server 
 *              will notify the client.
 * Input:       wilddog: Id of the client.
 *              event: Event type, see the struct.
 *              onDataChange: The callback function called when the server 
 *                          sends a data change packet.
 *              dataChangeArg: The arg defined by user, if you do not need, 
 *                          can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
Wilddog_Return_T wilddog_addObserver
    (
    Wilddog_T wilddog,
    Wilddog_EventType_T event, 
    onEventFunc onDataChange, 
    void* dataChangeArg
    )
{
    Wilddog_Arg_On_T args;
    wilddog_assert(wilddog, WILDDOG_ERR_NULL);
    
    args.p_ref = wilddog;
    args.d_event = event;
    args.p_onData = (Wilddog_Func_T)onDataChange;
    args.p_dataArg = dataChangeArg;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_ON, &args,0);
}

/*
 * Function:    wilddog_removeObserver
 * Description: Unsubscibe the client's data change.
 * Input:       wilddog: Id of the client.
 *              event: Event type, see the struct.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
Wilddog_Return_T wilddog_removeObserver
    (
    Wilddog_T wilddog, 
    Wilddog_EventType_T event
    )
{
    Wilddog_Arg_Off_T args;
    
    wilddog_assert(wilddog, WILDDOG_ERR_NULL);
    
    args.p_ref = wilddog;
    args.d_event = event;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_OFF, &args,0);
}

/*
 * Function:    wilddog_getParent
 * Description: Get the client's parent.
 * Input:       p_wilddog: a pointer to the client
 * Output:      N/A
 * Return:      A pointer point to the client's parent, if the client is root ,
 *              return NULL.
*/
Wilddog_T wilddog_getParent(Wilddog_T wilddog)
{
    Wilddog_Arg_GetRef_T args;
    
    wilddog_assert(wilddog, 0);
    
    args.p_ref = wilddog;
    args.d_cmd = WILDDOG_REFCHG_PARENT;
    args.p_str = NULL;

    return (Wilddog_T )_wilddog_ct_ioctl(WILDDOG_APICMD_GETREF, &args,0);
}

/*
 * Function:    wilddog_getRoot
 * Description: Get the client's Root.
 * Input:       wilddog: Id of the client.
 * Output:      N/A
 * Return:      an id of your client's root.
*/
Wilddog_T wilddog_getRoot(Wilddog_T wilddog)
{
    Wilddog_Arg_GetRef_T args;
    
    wilddog_assert(wilddog, 0);

    args.p_ref = wilddog;
    args.d_cmd = WILDDOG_REFCHG_ROOT;
    args.p_str = NULL;
    
    return (Wilddog_T )_wilddog_ct_ioctl(WILDDOG_APICMD_GETREF, &args,0);
}

/*
 * Function:    wilddog_getChild
 * Description: Get the client's child.
 * Input:       wilddog: Id of the client.
 * Output:      N/A
 * Return:      an id of your client's child.
 * Others:      The sdk do not check wether the child is really in the server 
 *              or not, only create it.
*/
Wilddog_T wilddog_getChild(Wilddog_T wilddog, Wilddog_Str_T * childName)
{
    Wilddog_Arg_GetRef_T args;
    
    wilddog_assert(wilddog, 0);
    
    args.p_ref = wilddog;
    args.d_cmd = WILDDOG_REFCHG_CHILD;
    args.p_str = childName;
    
    return (Wilddog_T )_wilddog_ct_ioctl(WILDDOG_APICMD_GETREF, &args, 0);
}

/*
 * Function:    wilddog_getKey
 * Description: Get the client's key(the node's name).
 * Input:       wilddog: Id of the client.
 * Output:      N/A
 * Return:      a pointer point to a name string(should be freed by user).
 * Others:      N/A
*/
Wilddog_Str_T *wilddog_getKey(Wilddog_T wilddog)
{
    wilddog_assert(wilddog, NULL);

    return (Wilddog_Str_T *)_wilddog_ct_ioctl(WILDDOG_APICMD_GETKEY, \
                                                (void*)wilddog,0);
}

