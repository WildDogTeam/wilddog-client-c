/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_ur_parser.c
 *
 * Description: url functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Jimmy.Pan       2015-05-15  Create file.
 * 0.4.3        Jimmy.Pan       2015-07-09  Add annotation.
 *
 */
#ifndef WILDDOG_PORT_TYPE_ESP
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "wilddog.h"
#include "wilddog_common.h"
#include "wilddog_conn.h"
#include "wilddog_store.h"
#include "wilddog_event.h"

STATIC Wilddog_Return_T _wilddog_store_ioctl
    (
    Wilddog_Store_T *p_store, 
    Wilddog_Store_Cmd_T cmd, 
    void* arg, 
    int flags
    );

/*
 * Function:    _wilddog_store_init
 * Description: Init a store structure.
 * Input:       p_repo: The pointer of the repo structure.
 * Output:      N/A
 * Return:      Pointer to the store structure.
*/
Wilddog_Store_T * WD_SYSTEM _wilddog_store_init(Wilddog_Repo_T* p_repo)
{
    Wilddog_Store_T *p_store = NULL;
    wilddog_assert(p_repo, NULL);
    
    p_store = (Wilddog_Store_T *)wmalloc(sizeof(Wilddog_Store_T));
    if(NULL == p_store)
    {
        return NULL;
    }
    p_store->p_se_auth = (Wilddog_Store_Auth_T *)wmalloc( \
        sizeof(Wilddog_Store_Auth_T));
    if(NULL == p_store->p_se_auth)
    {
        wfree(p_store);
        return NULL;
    }
    p_store->p_se_repo = p_repo;
    p_store->p_se_head = NULL;
    p_store->p_se_event = _wilddog_event_init(p_store);
    p_store->p_se_callback = (Wilddog_Func_T)_wilddog_store_ioctl;
    return p_store;
}

/*
 * Function:    _wilddog_store_setAuth
 * Description: Store the auth data and send auth to server.
 * Input:       p_store: The pointer of the store structure.
 *              arg: args.
 *              flag: unused.
 * Output:      N/A
 * Return:      If success return 0.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_store_setAuth
    (
    Wilddog_Store_T *p_store, 
    void* arg, 
    int flag
    )
{
    Wilddog_Store_AuthArg_T * p_authArg = (Wilddog_Store_AuthArg_T*)arg;
    Wilddog_Conn_T *p_conn = p_store->p_se_repo->p_rp_conn;
    Wilddog_ConnCmd_Arg_T connCmd;
    
    if(p_authArg->d_len > WILDDOG_AUTH_LEN)
        return WILDDOG_ERR_INVALID;

    memset(p_store->p_se_auth->p_auth, 0, WILDDOG_AUTH_LEN);

    if(!p_authArg->p_data)
    {
        if(p_authArg->d_len != 0)
            return WILDDOG_ERR_NULL;
    }
    else
        memcpy(p_store->p_se_auth->p_auth,p_authArg->p_data, p_authArg->d_len);
    p_store->p_se_auth->d_len = p_authArg->d_len;
    
    connCmd.p_url = p_authArg->p_url;
    connCmd.p_complete = (Wilddog_Func_T)p_authArg->p_onAuth;
    connCmd.p_completeArg = p_authArg->p_onAuthArg;
    connCmd.p_data = NULL;

    /*auth data will be called by lower layer*/
    if(p_conn && p_conn->f_conn_send)
    {
        return p_conn->f_conn_send(WILDDOG_CONN_CMD_AUTH, p_store->p_se_repo, \
            &connCmd);
    }
    return WILDDOG_ERR_INVALID;
}

/*
 * Function:    _wilddog_store_deinit
 * Description: Deinit the store structure.
 * Input:       p_repo: The pointer of the repo structure.
 * Output:      N/A
 * Return:      return NULL.
*/
Wilddog_Store_T* WD_SYSTEM _wilddog_store_deinit(Wilddog_Repo_T* p_repo)
{
    Wilddog_Store_T *p_store = NULL;

    wilddog_assert(p_repo, NULL);
    
    p_store = p_repo->p_rp_store;
    if(NULL == p_store)
        return NULL;
    p_store->p_se_head = NULL;
    p_store->p_se_event = _wilddog_event_deinit(p_store);
    if(p_store->p_se_auth)
        wfree(p_store->p_se_auth);
    p_store->p_se_auth = NULL;
    wfree(p_store);
    return NULL;
}

/*
 * Function:    _wilddog_store_getAuth
 * Description: Get the auth data.
 * Input:       p_store: The pointer of the store structure.
 *              arg: the pointer of the pointer to the auth buffer to store.
 * Output:      N/A
 * Return:      return the auth data len.
*/
STATIC u16 WD_SYSTEM _wilddog_store_getAuth
    (
    Wilddog_Store_T *p_store, 
    void* arg
    )
{
    *(u8**)arg = p_store->p_se_auth->p_auth;
    
    wilddog_assert(arg, 0);
    return p_store->p_se_auth->d_len;
}

/*
 * Function:    _wilddog_store_ioctl
 * Description: The store ioctl function.
 * Input:       p_store: The pointer of the store structure.
 *              cmd: command
 *              arg: the pointer of the pointer to the auth buffer to store.
 *              flags: unused.
 * Output:      N/A
 * Return:      return data defined by the cmd.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_store_ioctl
    (
    Wilddog_Store_T *p_store, 
    Wilddog_Store_Cmd_T cmd, 
    void* arg, 
    int flags
    )
{
    Wilddog_Conn_T *p_conn = p_store->p_se_repo->p_rp_conn;
    Wilddog_Event_T *p_rp_event = p_store->p_se_event;
    switch(cmd)
    {
        case WILDDOG_STORE_CMD_GETAUTH:
            return _wilddog_store_getAuth(p_store, arg);
        case WILDDOG_STORE_CMD_GETNODE:
            return WILDDOG_ERR_INVALID;
        case WILDDOG_STORE_CMD_SETAUTH:
            return _wilddog_store_setAuth(p_store, arg, flags);
        case WILDDOG_STORE_CMD_SENDGET:
            if(p_conn && p_conn->f_conn_send)
                return p_conn->f_conn_send(WILDDOG_CONN_CMD_GET, \
                                            p_store->p_se_repo, arg);
            break;
        case WILDDOG_STORE_CMD_SENDSET:
            if(p_conn && p_conn->f_conn_send)
                return p_conn->f_conn_send(WILDDOG_CONN_CMD_SET, \
                                            p_store->p_se_repo, arg);
            break;
        case WILDDOG_STORE_CMD_SENDPUSH:
            if(p_conn && p_conn->f_conn_send)
                return p_conn->f_conn_send(WILDDOG_CONN_CMD_PUSH, \
                                            p_store->p_se_repo, arg);
            break;
        case WILDDOG_STORE_CMD_SENDREMOVE:
            if(p_conn && p_conn->f_conn_send)
                return p_conn->f_conn_send(WILDDOG_CONN_CMD_REMOVE, \
                                            p_store->p_se_repo, arg);
            break;
        case WILDDOG_STORE_CMD_SENDON:
            if(p_rp_event && p_rp_event->p_ev_cb_on)
                return (p_rp_event->p_ev_cb_on)
                            (
                            p_rp_event, \
                            ((Wilddo_Store_EventArg_T*)arg)->d_event, \
                            &((Wilddo_Store_EventArg_T*)arg)->d_connCmd
                            );
            break;
        case WILDDOG_STORE_CMD_SENDOFF:
            if(p_rp_event && p_rp_event->p_ev_cb_off)
                return (p_rp_event->p_ev_cb_off)
                            (
                            p_rp_event, \
                            ((Wilddo_Store_EventArg_T*)arg)->d_event, \
                            &((Wilddo_Store_EventArg_T*)arg)->d_connCmd
                            );
            break;
    }
    return WILDDOG_ERR_INVALID;
}

