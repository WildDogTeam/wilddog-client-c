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

/* protocol package len .*/
#define PROTOCOL_QUERY_HEADLEN  (8)
#define PROTOCOL_PATH_HEADLEN  (8)


typedef enum WILDDOG_CONN_CMD_TYPE
{
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

    WILDDOG_CONN_CMD_CANCELDIS,

    WILDDOG_CONN_CMD_OFFLINE,
    WILDDOG_CONN_CMD_ONLINE,
    WILDDOG_CONN_CMD_TRYSYNC,
    
    WILDDOG_CONN_CMD_INIT,
    WILDDOG_CONN_CMD_DEINIT,
    
    WILDDOG_CONN_CMD_MAX
}Wilddog_Conn_Cmd_T;
typedef enum WILDDOG_CONN_CBCMD_TYPE
{
    WILDDOG_CONN_CBCMD_GET,
    WILDDOG_CONN_CBCMD_SET,
    WILDDOG_CONN_CBCMD_PUSH,
    WILDDOG_CONN_CBCMD_REMOVE,
    WILDDOG_CONN_CBCMD_ON,
    WILDDOG_CONN_CBCMD_OFF,
    WILDDOG_CONN_CBCMD_AUTH,
    
    WILDDOG_CONN_CBCMD_ONDISSET,
    WILDDOG_CONN_CBCMD_ONDISPUSH,
    WILDDOG_CONN_CBCMD_ONDISREMOVE,

    WILDDOG_CONN_CBCMD_CANCELDIS,
    WILDDOG_CONN_CBCMD_ONLINE,
    WILDDOG_CONN_CBCMD_OFFLINE,

    WILDDOG_CONN_CBCMD_MAX
}Wilddog_Conn_CBCmd_T;

typedef struct WILDDOG_CONN_T
{
    Wilddog_Repo_T *p_conn_repo;
    Wilddog_Cm_List_T *p_cm_l;
    Wilddog_Func_T f_conn_ioctl;
}Wilddog_Conn_T;


typedef struct WILDDOG_CONN_CMD_ARG
{
    
    Wilddog_Repo_T *p_repo;
    Wilddog_Url_T * p_url;
    Wilddog_Node_T * p_data;
    Wilddog_Func_T p_complete;
    void* p_completeArg;
}Wilddog_ConnCmd_Arg_T;

/* protocol peripheral interface.*/
typedef enum PROTOCOL_CMD_T{
    _PROTOCOL_CMD_INIT,
    _PROTOCOL_CMD_DEINIT,

    _PROTOCOL_CMD_COUNTSIZE,
    _PROTOCOL_CMD_CREAT,
    _PROTOCOL_CMD_DESTORY,
    _PROTOCOL_CMD_ADD_HOST,
    _PROTOCOL_CMD_ADD_PATH,
    _PROTOCOL_CMD_ADD_QUERY,
    _PROTOCOL_CMD_ADD_OBSERVER,
    _PROTOCOL_CMD_ADD_DATA,
    
    _PROTOCOL_CMD_AUTHUPDATA,
    _PROTOCOL_CMD_SEND,
    _PROTOCOL_CMD_RECV,
    _PROTOCOL_CMD_SEND_PING,
    _PROTOCOL_CMD_MAX
}Protocol_cmd_t;
typedef struct PROTOCOL_ARG_INIT_T{
    Wilddog_Str_T *p_host;
    Wilddog_Func_T f_handleRespond;
    u16 d_port;
}Protocol_Arg_Init_T;

typedef struct PROTOCOL_ARG_CREAT_T{
    u8 cmd;
    u16 d_index;
    u16 d_packageLen;
    u32 d_token;
}Protocol_Arg_Creat_T;

typedef struct PROTOCOL_ARG_OPTION_T{
   void *p_pkg;
   void *p_options;
}Protocol_Arg_Option_T;

typedef struct PROTOCOL_ARG_PAYLOADA_T{
   void *p_pkg;
   void *p_payload;
   u32 d_payloadLen;
}Protocol_Arg_Payload_T;

typedef struct PROTOCOL_ARG_COUNTSIZE_T{
    u8 *p_host;
    u8 *p_path;
    u8 *p_query;
    
    u32 d_payloadLen;
    u32 d_extendLen;
}Protocol_Arg_CountSize_T;
    
typedef struct PROTOCOL_ARG_SEND_T{
    u8 cmd;
    Wilddog_Url_T *p_url;

    u32 d_token;
    u32 d_payloadlen;
    u8 *p_payload;  
    void *p_user_arg;

    u16 d_messageid;
}Protocol_Arg_Send_T;


typedef struct PROTOCOL_ARG_AUTHARG_T{
    void *p_pkg;
    u8 *p_newAuth;
    int d_newAuthLen;
}Protocol_Arg_Auth_T;

/* protocol application function */
extern size_t WD_SYSTEM _wilddog_protocol_ioctl
    (
    Protocol_cmd_t cmd,
    void *p_args,
    int flags
    );

/*implemented interface.*/
extern Wilddog_Conn_T* _wilddog_conn_init(Wilddog_Repo_T* p_repo);
extern Wilddog_Conn_T* _wilddog_conn_deinit(Wilddog_Repo_T*p_repo);

#endif /*_WILDDOG_CONN_H_*/

