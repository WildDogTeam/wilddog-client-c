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

#include "wilddog_config.h"
#include "wilddog_ct.h"
#include "wilddog_protocol.h"
#if 0
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
    _PROTOCOL_CMD_MODIFY_MIDTOKEN,
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
typedef struct WILDDOG_CM_FINDNODE_ARG_T
{
	Wilddog_CM_Node_T *p_node_hd;
	u8 *path;
}Wilddog_CM_FindNode_Arg_T;

/* protocol application function */
extern size_t WD_SYSTEM _wilddog_protocol_ioctl
    (
    Protocol_cmd_t cmd,
    void *p_args,
    int flags
    );
#else
#define WILDDOG_CONN_SESSION_SHORT_LEN 4
#define WILDDOG_CONN_SESSION_LONG_LEN 32

typedef enum WILDDOG_SESSION_STATE{
    WILDDOG_SESSION_NOTAUTHED = 0,
    WILDDOG_SESSION_AUTHING,
    WILDDOG_SESSION_AUTHED
}Wilddog_Session_State;

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
    
    WILDDOG_CONN_CMD_MAX
}Wilddog_Conn_Cmd_T;

typedef struct WILDDOG_CONN_CMD_ARG
{
    Wilddog_Repo_T *p_repo;
    Wilddog_Url_T * p_url;
    Wilddog_Node_T * p_data;
    Wilddog_Func_T p_complete;
    void* p_completeArg;
}Wilddog_ConnCmd_Arg_T;
typedef struct WILDDOG_CONN_PKT_DATA_T{
    struct WILDDOG_CONN_PKT_DATA_T *next;
    u8* data;
    //int seq;//sequence number
    u32 len;
}Wilddog_Conn_Pkt_Data_T;
typedef struct WILDDOG_CONN_PKT_T{
    struct WILDDOG_CONN_PKT_T *next;
    int d_count;
    u32 d_message_id;
    u32 d_next_send_time;
    Wilddog_Url_T *p_url;
    Wilddog_Func_T p_complete;//pkt matched function
    Wilddog_Func_T p_user_callback;
    void *p_user_arg;
    Wilddog_Conn_Pkt_Data_T *p_data; //packet data serialized and packeted by protocol.
    u8 *p_proto_data;
}Wilddog_Conn_Pkt_T;

typedef struct WILDDOG_CONN_SYS_T{
    struct WILDDOG_CONN_T *p_conn;
    int d_curr_ping_interval;
    int d_ping_delta;
    Wilddog_Conn_Pkt_T *p_ping;
    Wilddog_Conn_Pkt_T *p_auth;
}Wilddog_Conn_Sys_T;

typedef struct WILDDOG_CONN_USER_T{
    struct WILDDOG_CONN_T *p_conn;
    Wilddog_Conn_Pkt_T *p_observer_list;
    Wilddog_Conn_Pkt_T *p_rest_list;
}Wilddog_Conn_User_T;

typedef struct WILDDOG_SESSION_T{
    Wilddog_Session_State  d_session_status;
    char short_sid[WILDDOG_CONN_SESSION_SHORT_LEN];
    char long_sid[WILDDOG_CONN_SESSION_LONG_LEN];
}Wilddog_Session_T;

typedef struct WILDDOG_CONN_T
{
    Wilddog_Repo_T *p_conn_repo;
    Wilddog_Session_T d_session;
    Wilddog_Conn_Sys_T d_conn_sys;
    Wilddog_Conn_User_T d_conn_user;
    Wilddog_Protocol_T *p_protocol;
    Wilddog_Func_T f_conn_ioctl;
}Wilddog_Conn_T;

#endif
/*implemented interface.*/
extern Wilddog_Conn_T* _wilddog_conn_init(Wilddog_Repo_T* p_repo);
extern Wilddog_Return_T _wilddog_conn_deinit(Wilddog_Repo_T*p_repo);

#endif /*_WILDDOG_CONN_H_*/

