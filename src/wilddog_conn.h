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
 * 0.4.0        lsx       2015-05-15  Create file.
 *
 */

#ifndef _WILDDOG_CONN_H_
#define _WILDDOG_CONN_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "wilddog.h"
#include "wilddog_config.h"
#include "wilddog_ct.h"
#include "wilddog_url_parser.h"

#define AUTHR_PATH  "/.cs"
#define AUTHR_QURES ".cs="
#define AUTHR_LEN   (4)

typedef enum WILDDOG_CONN_CMD_TYPE
{
    WILDDOG_CONN_CMD_GET,
    WILDDOG_CONN_CMD_SET,
    WILDDOG_CONN_CMD_PUSH,
    WILDDOG_CONN_CMD_REMOVE,
    WILDDOG_CONN_CMD_ON,
    WILDDOG_CONN_CMD_OFF,
    WILDDOG_CONN_CMD_AUTH,
    WILDDOG_CONN_CMD_PING
}Wilddog_Conn_Cmd_T;

typedef struct WILDDOG_CONN_NODE_T
{
    struct WILDDOG_CONN_NODE_T *next;
    u32 d_cn_regist_tm;
    u32 d_cn_nextsend_tm;
    u32 d_cn_retansmit_cnt;
    void    *p_cn_pkt;

    u8 d_cmd;
    u8 d_observe_flag;
    Wilddog_Func_T f_cn_callback;
    u8 *p_cn_path;
    void* p_cn_cb_arg;
    
}Wilddog_Conn_Node_T;

typedef struct WILDDOG_CONN_T
{
    Wilddog_Repo_T *p_conn_repo;
    
    u8 d_auth_st;
    u32 d_socketid;
    u32 d_wauth;
    u32 d_ralyRecv;
    u32 d_ralySend;

    Wilddog_Func_T f_conn_trysyc;
    Wilddog_Func_T f_conn_send;
    Wilddog_Address_T d_remoteAddr;
    
    struct WILDDOG_CONN_NODE_T *p_conn_node_hd;

}Wilddog_Conn_T;


typedef struct WILDDOG_CONN_PKTSEND_T{
    
    Wilddog_Conn_Cmd_T cmd;
    Wilddog_Url_T *p_url;
    
    u32 d_payloadlen;
    u8 *p_payload;  
}Wilddog_Conn_PktSend_T;

typedef struct WILDDOG_CONN_RECVDATA_T
{
    u8 *p_Recvdata;
    u32 d_recvlen;
    u32 d_RecvErr;
    
}Wilddog_Conn_RecvData_T;

typedef struct WILDDOG_CONN_CMD_ARG
{
    Wilddog_Url_T * p_url;
    Wilddog_Node_T * p_data;
    Wilddog_Func_T p_complete;
    void* p_completeArg;
}Wilddog_ConnCmd_Arg_T;


extern int _byte2bytestr(u8 *p_dst,u8 *p_dscr,u8 len);

/*@ api require*/
int _wilddog_conn_pkt_creat
    (
    Wilddog_Conn_PktSend_T *p_pktSend,
    void **pp_pkt_creat
    );
Wilddog_Return_T _wilddog_conn_pkt_init
    (
    int fd,
    Wilddog_Address_T * addr_in
    );
void _wilddog_conn_pkt_free(void **pp_pkt);
Wilddog_Return_T _wilddog_conn_pkt_deinit
    (
    int fd, 
    Wilddog_Address_T * addr_in
    );
Wilddog_Return_T _wilddog_conn_pkt_send
    (
    int fd,
    Wilddog_Address_T * addr_in,
    u8 *p_auth,
    void *p_cn_pkt
    );
Wilddog_Return_T _wilddog_conn_pkt_recv
    (u32 fd,
    Wilddog_Address_T * addr_in,
    void **pp_cn_pkt,
    Wilddog_Conn_RecvData_T *p__cpk_recv
    );
Wilddog_Conn_T * _wilddog_conn_init(Wilddog_Repo_T* p_repo);
Wilddog_Conn_T* _wilddog_conn_deinit(Wilddog_Repo_T*p_repo);



#endif /*_WILDDOG_CONN_H_*/

