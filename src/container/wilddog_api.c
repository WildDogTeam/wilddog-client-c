/*
 * wilddog_api.c
 *
 *  Created on: 2015-05-15
 *      Author: jimmy.pan
 */
#include <stdio.h>
#include <stdlib.h>

#include "wilddog_ct.h"
#include "wilddog_api.h"

extern void inline _wilddog_setTimeIncrease(u32 ms);

void wilddog_timeIncrease(u32 ms)
{
    _wilddog_setTimeIncrease(ms);
    return;
}
void wilddog_trySync(void)
{
    _wilddog_ct_ioctl(WILDDOG_APICMD_SYNC, NULL, 0);
}
void wilddog_init(void)
{
    _wilddog_ct_ioctl(WILDDOG_APICMD_INIT, NULL, 0);
}

Wilddog_T wilddog_new(Wilddog_Str_T *url)
{
    wilddog_assert(url);
    
    return (Wilddog_T )_wilddog_ct_ioctl(WILDDOG_APICMD_CREATEREF, url,0);
}

Wilddog_Return_T wilddog_destroy(Wilddog_T *p_wilddog)
{
    wilddog_assert(p_wilddog);

    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_DESTROYREF, p_wilddog,0);
}

//input url such as coap://appid.wilddogio.com/
Wilddog_Return_T wilddog_setAuth
    (
    Wilddog_Str_T * p_host, 
    u8 *p_auth, 
    int len, 
    onAuthFunc onAuth, 
    void* args
    )
{
    Wilddog_Arg_SetAuth_T arg;

    wilddog_assert(p_host);
    wilddog_assert(p_auth);
    
    arg.p_host = p_host;
    arg.p_auth = p_auth;
    arg.d_len = len;
    arg.onAuth = onAuth;
    arg.arg = args;

    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_SETAUTH, &arg,0);
}

Wilddog_Return_T wilddog_query
    (
    Wilddog_T wilddog, 
    onQueryFunc callback, 
    void* arg
    )
{
    Wilddog_Arg_Query_T args;

    wilddog_assert(wilddog);
    
    args.p_ref = wilddog;
    args.p_callback = (Wilddog_Func_T)callback;
    args.arg = arg;
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_QUERY, &args,0);
}

Wilddog_Return_T wilddog_set
    (
    Wilddog_T wilddog, 
    Wilddog_Node_T *p_node, 
    onSetFunc callback, 
    void* arg
    )
{
    Wilddog_Arg_Set_T args;
    
    wilddog_assert(wilddog);
    
    args.p_ref = wilddog;
    args.p_node = p_node;
    args.p_callback = (Wilddog_Func_T)callback;
    args.arg = arg;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_SET, &args,0);
}

Wilddog_Return_T wilddog_push
    (
    Wilddog_T wilddog,
    Wilddog_Node_T *p_node, 
    onPushFunc callback,
    void* arg
    )
{
    Wilddog_Arg_Push_T args;
    
    wilddog_assert(wilddog);
    
    args.p_ref = wilddog;
    args.p_node = p_node;
    args.p_callback = (Wilddog_Func_T)callback;
    args.arg = arg;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_PUSH, &args,0);
}

Wilddog_Return_T wilddog_remove
    (
    Wilddog_T wilddog,
    onRemoveFunc callback, 
    void* arg
    )
{
    Wilddog_Arg_Remove_T args;
    
    wilddog_assert(wilddog);

    args.p_ref = wilddog;
    args.p_callback = (Wilddog_Func_T)callback;
    args.arg = arg;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_REMOVE, &args,0);
}

Wilddog_Return_T wilddog_on
    (
    Wilddog_T wilddog,
    Wilddog_EventType_T event, 
    onEventFunc onDataChange, 
    void* dataChangeArg
    )
{
    Wilddog_Arg_On_T args;
    
    wilddog_assert(wilddog);
    
    args.p_ref = wilddog;
    args.d_event = event;
    args.p_onData = (Wilddog_Func_T)onDataChange;
    args.p_dataArg = dataChangeArg;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_ON, &args,0);
}

Wilddog_Return_T wilddog_off(Wilddog_T wilddog, Wilddog_EventType_T event)
{
    Wilddog_Arg_Off_T args;
    wilddog_assert(wilddog);
    
    args.p_ref = wilddog;
    args.d_event = event;
    
    return (Wilddog_Return_T)_wilddog_ct_ioctl(WILDDOG_APICMD_OFF, &args,0);
}

Wilddog_T wilddog_getParent(Wilddog_T wilddog)
{
    Wilddog_Arg_GetRef_T args;
    
    wilddog_assert(wilddog);
    
    args.p_ref = wilddog;
    args.d_cmd = WILDDOG_REFCHG_PARENT;
    args.p_str = NULL;

    return (Wilddog_T )_wilddog_ct_ioctl(WILDDOG_APICMD_GETREF, &args,0);
}

Wilddog_T wilddog_getRoot(Wilddog_T wilddog)
{
    Wilddog_Arg_GetRef_T args;
    
    wilddog_assert(wilddog);

    args.p_ref = wilddog;
    args.d_cmd = WILDDOG_REFCHG_ROOT;
    args.p_str = NULL;
    
    return (Wilddog_T )_wilddog_ct_ioctl(WILDDOG_APICMD_GETREF, &args,0);
}

Wilddog_T wilddog_getChild(Wilddog_T wilddog, Wilddog_Str_T * childName)
{
    Wilddog_Arg_GetRef_T args;
    
    wilddog_assert(wilddog);
    
    args.p_ref = wilddog;
    args.d_cmd = WILDDOG_REFCHG_CHILD;
    args.p_str = childName;
    
    return (Wilddog_T )_wilddog_ct_ioctl(WILDDOG_APICMD_GETREF, &args, 0);
}

Wilddog_Str_T *wilddog_getKey(Wilddog_T wilddog)
{
    wilddog_assert(wilddog);

    return (Wilddog_Str_T *)_wilddog_ct_ioctl(WILDDOG_APICMD_GETKEY, \
                                                (void*)wilddog,0);
}

