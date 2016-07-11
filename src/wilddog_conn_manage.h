/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_manage.h
 *
 * Description: connection management.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        lxs       2015-05-15  Create file.
 *
 */

#ifndef _WILDDOG_CONN_MANAGE_H_
#define _WILDDOG_CONN_MANAGE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "wilddog.h"
#include "wilddog_ct.h"
#include "wilddog_url_parser.h"


#define _CM_AUTHR_PATH  "/.cs"
#define _CM_AUTHR_QURES ".cs="
#define _CM_ONDIS       ".dis=add"
#define _CM_DISCANCEL   ".dis=rm"
#define _CM_OFFLINE_PATH     "/.off"
#define _CM_ONLINE      ".on"


#define AUTHR_LEN   (4)
#define AUTHR_LENINBYTE (2*AUTHR_LEN)


typedef enum CM_CMD_T{
	CM_CMD_INIT,
	CM_CMD_DEINIT,

	CM_CMD_GET_INDEX,
	CM_CMD_GET_TOKEN,

	CM_CMD_SHORTTOKEN,
	CM_CMD_AUTHQUERY,

	CM_CMD_USERSEND,
	CM_CMD_DELENODE_BYPATH,

	CM_CMD_OFFLINE,
	CM_CMD_ONLINE,
	CM_CMD_TRYSYNC,

	
	CM_CMD_OBSERVER_ALEADY,
	CM_CMD_AUTH_DELETE,
	
	CM_CMD_MAX
}CM_Cmd_T;
typedef enum CM_ONLINE_STATUS{
    CM_OFFLINE,
    CM_ONLINE
}CM_Online_Status;
typedef enum CM_SESSION_STATE{
    CM_SESSION_UNAUTH,
    CM_SESSION_DOAUTH,
    CM_SESSION_AUTHING,
    CM_SESSION_AUTHED
}CM_Session_State;

typedef enum CM_SERVER_EVENT_T{
    CM_SERVER_EVENT_NOMAL,
    CM_SERVER_EVENT_PRECONDITION_FAIL,
    CM_SERVER_EVENT_MAX
   
}CM_Server_Event_T;

typedef enum WILDDOG_CMSYS_CMD_TYPE
{
    WILDDOG_CM_SYS_CMD_SHORTPING = 0XA0,
    WILDDOG_CM_SYS_CMD_LONGPING = 0XA1,

    WILDDOG_CM_SYS_CMD_MAX
}Wilddog_Cmsys_Cmd_Type;

typedef struct WILDDOG_CM_NODE_T{

    u32 cmd;
    u32 d_registerTm;
    u32 d_sendTm;
    u32 d_token;
    void *p_pkg;
    void* p_userCB_arg;
    Wilddog_Func_T f_userCB;

    u32 d_subscibe_index;
    u32 d_maxAge;
    u8 *p_path;
    
    struct WILDDOG_CM_NODE_T *next;

    
	u8 reObserver_flg;
    u8 d_nodeType;
    u8 d_retransmit_cnt; 
    u8 _reserve;
}Wilddog_CM_Node_T;

typedef struct WILDDOG_CM_LIST_T
{
    Wilddog_CM_Node_T *p_cm_n_hd;
    u8 *p_long_token;
    u8 *p_short_token;
    Wilddog_Str_T *p_host;
    Wilddog_Repo_T *p_repo;
    
    CM_Session_State d_authStatus;
    CM_Server_Event_T d_serverEvent;
}Wilddog_Cm_List_T;

typedef struct WILDDOG_CM_INITARG_T{

    Wilddog_Func_T f_conn_cb;
    Wilddog_Repo_T *p_repo;
}Wilddog_CM_InitArg_T;

typedef struct WILDDOG_CM_OFFARG_T{
    
    u8 *p_path;
    Wilddog_Cm_List_T *p_cm_l;

}Wilddog_CM_OffArg_T;


typedef struct WILDDOG_CM_USERARG_T{

    u32 cmd;
    u32 d_token;
    void *p_pkg;
    
    void* f_userCB_arg;
    u8 *p_path;
    Wilddog_Func_T f_userCB;
    Wilddog_Cm_List_T *p_cm_l;

}Wilddog_CM_UserArg_T;

typedef struct WILDDOG_CM_RECV_T
{        
    Wilddog_Payload_T *p_recvData;    
    Wilddog_Func_T f_user_callback;    
    Wilddog_Str_T *p_url_path;    
    void* p_user_cb_arg;    
    u32 err;        
    u8 cmd;
}Wilddog_CM_Recv_T;

typedef struct PROTOCOL_RECVARG_T{

    u32 d_observerIndx;
    u32 d_maxAge;   
	u32 d_token;
    u32 err;
    
	u32 d_recvDataLen;
	
    u8 d_isObserver;   
    u8 d_blockIdx;
	u8 d_blockNum;
	u8 reserve;
	
	u8 *p_r_path;	
	u8 *p_recvData;
	
}Protocol_recvArg_T;

typedef struct WILDDOG_CM_SEND_PING_ARG_T
{
    void* p_pkg;
    u16 d_mid;
    u32 d_token;
}Wilddog_CM_Send_Ping_Arg_T;
extern size_t WD_SYSTEM _wilddog_cm_ioctl(u8 cmd,void *p_args,int flags);

#endif /* _WILDDOG_CONN_MANAGE_H_ */

