/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_conn.h
 *
 * Description: connection functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        lxs       2015-05-15  Create file.
 *
 */

#ifndef _WILDDOG_CONN_H_
#define _WILDDOG_CONN_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "wilddog_conn_manage.h"
#include "wilddog_config.h"
#include "wilddog_ct.h"


typedef enum WILDDOG_CONN_CMD_TYPE
{
    WILDDOG_CONN_CMD_INIT,
    WILDDOG_CONN_CMD_DEINIT,
    
    WILDDOG_CONN_CMD_GET,
    WILDDOG_CONN_CMD_SET,
    WILDDOG_CONN_CMD_PUSH,
    WILDDOG_CONN_CMD_REMOVE,
    WILDDOG_CONN_CMD_ON,
    WILDDOG_CONN_CMD_OFF,
    WILDDOG_CONN_CMD_AUTH,
    
    WILDDOG_CONN_CMD_ONDISSET,
    WILDDOG_CONN_CMD_ONDISPUSH,
    WILDDOG_CONN_CMD_ONDISREMOVE,

    WILDDOG_CONN_CMD_CANCELDISSET,
    WILDDOG_CONN_CMD_CANCELDISPUSH,
    WILDDOG_CONN_CMD_CANCELDISREMOVE,    

    WILDDOG_CONN_CMD_OFFLINE,
    WILDDOG_CONN_CMD_ONLINE,
    WILDDOG_CONN_CMD_TRYSYNC,

    WILDDOG_CONN_CMD_MAX
    
}Wilddog_Conn_Cmd_T;

typedef struct WILDDOG_CONN_T
{
    Wilddog_Repo_T *p_conn_repo;
    WILDDOG_CM_NODE_T *p_cm_hd;
    
    Wilddog_Func_T f_conn_ioctl;
    
    
}Wilddog_Conn_T;

typedef struct WILDDOG_CONN_CMD_ARG
{
    
    Wilddog_Repo_T *p_repo,
    Wilddog_Url_T * p_url;
    Wilddog_Node_T * p_data;
    Wilddog_Func_T p_complete;
    void* p_completeArg;
}Wilddog_ConnCmd_Arg_T;

typedef struct WILDDOG_CONN_CMD_ARG
{
    
    Wilddog_Repo_T *p_repo,
    Wilddog_Url_T * p_url;
    Wilddog_Node_T * p_data;
    Wilddog_Func_T p_complete;
    void* p_completeArg;
}Wilddog_ConnCmd_Arg_T;

Wilddog_Conn_T* _wilddog_conn_init(Wilddog_Repo_T* p_repo);
Wilddog_Conn_T* _wilddog_conn_deinit(Wilddog_Repo_T*p_repo);

#endif /*_WILDDOG_CONN_H_*/

