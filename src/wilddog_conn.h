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

#define WILDDOG_AUTH_2_0

#define WILDDOG_CONN_SESSION_SHORT_LEN (8 + 1)
#define WILDDOG_CONN_SESSION_LONG_LEN (32 + 1)

#define WILDDOG_CONN_PKT_FLAG_NEVERTIMEOUT (0x01)//this flag mean packet never timeout.

/*
    Session State machine:
 
     ----bad request-----  ------------------retry----------------
     |                  |  |                                     |
    \|/                 | \|/                                    |
   ------             ---------                             -----------
   |Init|----init---->|Authing|-----timeout/unknown err---->|NotAuthed|
   ------             ---------                             -----------
    /|\                 |  /|\                                  /|\
     |                  |   |                                    |
     |              200 OK Auth                                  |
     |                  |   |                                    |
     |                 \|/  |                                    |
     |                --------                                   |
     ---goOffline-----|Authed|------------unauthed err------------
                      --------
    Policy:
        1. Init/NotAuthed/Authing: only can send auth packet,other packets must
           be stored but do not send out.During this time, ping packet will not be
           generated. When stored packets time out, trigger timeout function.
        2. Authed: We can send any type of packets, and will generate ping packet.
        3. Authing->Authed: All stored packets will be send right now.
        4. Authed->Authing/NotAuthed: the same as 1.
*/
typedef enum WILDDOG_SESSION_STATE{
    WILDDOG_SESSION_INIT = 0,
    WILDDOG_SESSION_NOTAUTHED,
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
typedef enum{
    WILDDOG_PING_TYPE_SHORT = 0,
    WILDDOG_PING_TYPE_LONG
}Wilddog_Ping_Type_T;

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
    u32 d_register_time;
    u32 d_flag;
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
    int d_auth_fail_count;
    u32 d_offline_time;
    int d_online_retry_count;
    Wilddog_Ping_Type_T d_ping_type;
    u32 d_ping_next_send_time;
    Wilddog_Conn_Pkt_T *p_ping;
    Wilddog_Conn_Pkt_T *p_auth;
}Wilddog_Conn_Sys_T;

typedef struct WILDDOG_CONN_USER_T{
    struct WILDDOG_CONN_T *p_conn;
    int d_count;
    Wilddog_Conn_Pkt_T *p_observer_list;
    Wilddog_Conn_Pkt_T *p_rest_list;
}Wilddog_Conn_User_T;

typedef struct WILDDOG_SESSION_T{
    Wilddog_Session_State  d_session_status;
    u8 short_sid[WILDDOG_CONN_SESSION_SHORT_LEN];
    u8 long_sid[WILDDOG_CONN_SESSION_LONG_LEN];
}Wilddog_Session_T;

typedef struct WILDDOG_CONN_T
{
    Wilddog_Repo_T *p_conn_repo;
    u32 d_timeout_count;
    Wilddog_Session_T d_session;
    Wilddog_Conn_Sys_T d_conn_sys;
    Wilddog_Conn_User_T d_conn_user;
    Wilddog_Protocol_T *p_protocol;
    Wilddog_Func_T f_conn_ioctl;
}Wilddog_Conn_T;

/*implemented interface.*/
extern Wilddog_Conn_T* _wilddog_conn_init(Wilddog_Repo_T* p_repo);
extern Wilddog_Return_T _wilddog_conn_deinit(Wilddog_Repo_T*p_repo);

#endif /*_WILDDOG_CONN_H_*/

