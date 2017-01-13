/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_conn.c
 *
 * Description: connection functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        lxs             2015-05-15  Create file.
 * 0.5.0        lxs             2015-10-09  cut down some function.
 * 0.7.5        lxs             2015-12-02  one cmd one functions.
 */
 
#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "wilddog_payload.h"
     
#include "utlist.h"
#include "wilddog_config.h"
#include "wilddog.h"
#include "wilddog_debug.h"
#include "wilddog_store.h"
#include "wilddog_common.h"
#include "wilddog_conn.h"

#include "wilddog_port.h"
#include "wilddog_url_parser.h"
#include "wilddog_payload.h"
#include "wilddog_api.h"
#include "test_lib.h"
#include "wilddog_protocol.h"

#define WILDDOG_RETRANSMIT_DEFAULT_INTERVAL (2*1000)//默认的重传间隔
#define WILDDOG_DEFALUT_PING_INTERVAL (19*1000) //初始化的ping间隔
#define WILDDOG_DEFAULT_PING_DELTA (10*1000) //ping的初始化步进间隔

STATIC INLINE u32 WD_SYSTEM _wilddog_conn_getNextSendTime(int count){
    return (_wilddog_getTime() + count * WILDDOG_RETRANSMIT_DEFAULT_INTERVAL);
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_auth_callback(){
    return 0;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_pkt_data_free(Wilddog_Conn_Pkt_Data_T * p_data){
    Wilddog_Conn_Pkt_Data_T *curr, *tmp;
    wilddog_assert(p_data, WILDDOG_ERR_NULL);
    
    LL_FOREACH_SAFE(p_data,curr,tmp){
        if(curr){
            LL_DELETE(p_data,curr);
            wfree(curr->data);
            wfree(curr);
        }
    }
    return WILDDOG_ERR_NOERR;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_packet_deInit(Wilddog_Conn_Pkt_T * pkt){
    wilddog_assert(pkt, WILDDOG_ERR_NULL);

    if(pkt->p_proto_data){
        wfree(pkt->p_proto_data);
        pkt->p_proto_data = NULL;
    }
    if(pkt->p_data){
        _wilddog_conn_pkt_data_free(pkt->p_data);
        pkt->p_data = NULL;
    }
    if(pkt->p_url){
        _wilddog_url_freeParsedUrl(pkt->p_url);
        pkt->p_url = NULL;
    }
    return WILDDOG_ERR_NOERR;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_packet_init(Wilddog_Conn_Pkt_T * pkt,Wilddog_Conn_T *p_conn){
    wilddog_assert(pkt&&p_conn, WILDDOG_ERR_NULL);

    pkt->p_url = (Wilddog_Url_T*)wmalloc(sizeof(Wilddog_Url_T));
    if(NULL == pkt->p_url){
        wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed!");
        return WILDDOG_ERR_NULL;
    }
    
    if(WILDDOG_ERR_NOERR != _wilddog_url_copy(p_conn->p_conn_repo->p_rp_url, pkt->p_url)){
        wfree(pkt->p_url);
        wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_data = (Wilddog_Conn_Pkt_Data_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_Data_T));
    if(NULL == pkt->p_data){
        _wilddog_url_freeParsedUrl(pkt->p_url);
        wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = NULL;
    pkt->d_count = 0;
    pkt->d_next_send_time = 0;
    pkt->d_message_id = 0;
    pkt->next = NULL;
   
    return WILDDOG_ERR_NOERR;
}
STATIC BOOL WD_SYSTEM _wilddog_conn_midCmp(u32 s_mid,u32 d_mid){
    if((s_mid & 0xffffffff) == (d_mid & 0xffffffff)){
        return TRUE;
    }
    return FALSE;
}
/*
 * find the send packet matched with received packet
*/
STATIC Wilddog_Conn_Pkt_T * WD_SYSTEM _wilddog_conn_recv_sendPktFind(Wilddog_Conn_T *p_conn,u32 mid){
    wilddog_assert(p_conn NULL);

    if(p_conn->d_session.d_session_status == WILDDOG_SESSION_AUTHED){
        if(TRUE == _wilddog_conn_midCmp(mid,p_conn->d_conn_sys->p_ping->d_message_id)){
            return p_conn->d_conn_sys.p_ping;
        }
        else{
            Wilddog_Conn_Pkt_T *curr, *tmp;
            //observe list check
            LL_FOREACH_SAFE(p_conn->d_conn_user->p_observer_list,curr,tmp){
                if(TRUE == _wilddog_conn_midCmp(mid,curr->d_message_id)){
                    return curr;
                }
            }
            //rest list check
            LL_FOREACH_SAFE(p_conn->d_conn_user->p_rest_list,curr,tmp){
                if(TRUE == _wilddog_conn_midCmp(mid,curr->d_message_id)){
                    return curr;
                }
            }
            //others, also include auth pkt, do not match them.
        }
    }else if(p_conn->d_session.d_session_status == WILDDOG_SESSION_AUTHING){
        if(TRUE == _wilddog_conn_midCmp(mid, p_conn->d_conn_sys.p_auth->d_message_id)){
            //match, is the auth callback!
            return p_conn->d_conn_sys.p_auth;
        }
        else{
            //bye bye, we don't care who you are.
            return NULL;
        }
    }else{
        wilddog_debug_level(WD_DEBUG_WARN, "Receive packet, but client was not registered yet.");
        return NULL;
    }
    return NULL;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_sessionInit(Wilddog_Conn_T *p_conn){    
    Wilddog_Session_T *p_session;
    Wilddog_Conn_Pkt_T *pkt;
    Wilddog_Proto_Cmd_Arg_T command;

    wilddog_assert(p_conn, WILDDOG_ERR_NULL);
    wilddog_assert(p_conn->p_conn_repo->p_rp_store, WILDDOG_ERR_NULL);
    
    p_session = &p_conn->d_session;
    memset(p_session, 0, sizeof(Wilddog_Session_T));

    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    if(NULL == pkt){
        return WILDDOG_ERR_NULL;
    }
    
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, p_conn)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)_wilddog_conn_auth_callback;
    //add to auth queue
    if(p_conn->d_conn_sys.p_auth){
        _wilddog_conn_packet_deInit(p_conn->d_conn_sys.p_auth);
    }
    p_conn->d_conn_sys.p_auth = pkt;

    //send to server
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = &(p_conn->d_conn_sys.p_auth->p_data->data);
    command.p_out_data_len = &(p_conn->d_conn_sys.p_auth->p_data->len);
    //get user auth token
    if(p_conn->p_conn_repo->p_rp_store->p_se_callback){
        command.d_data_len = (p_conn->p_conn_repo->p_rp_store->p_se_callback)(
                               p_conn->p_conn_repo->p_rp_store,
                               WILDDOG_STORE_CMD_GETAUTH,&command.p_data,0);
    }
    
    if(p_conn->p_protocol->callback){
        if(WILDDOG_ERR_NOERR == \
           (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_SESSION_INIT, &command, 0)){
            //change status to authing
            p_session->d_session_status = WILDDOG_SESSION_AUTHING;
        }
    }
    ++pkt->d_count;
    pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
    return WILDDOG_ERR_NOERR;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_trySync(void* data,int flag){
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    u32 message_id = 0;
    Wilddog_Conn_T *p_conn;
    u8* recvPkt = NULL;
    u32 recvPkt_len = 0;
    Wilddog_Conn_Pkt_T * sendPkt = NULL;
    
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;

    wilddog_assert(p_conn, WILDDOG_ERR_NULL);
    
    command.protocol = p_conn->p_protocol;

    wilddog_assert(p_conn && command.protocol,WILDDOG_ERR_NULL);
    
    command.p_out_data = &recvPkt;
    command.p_out_data_len = &recvPkt_len;
    command.d_data_len = 0;
    command.p_url = arg->p_url;
    command.p_message_id = &message_id;
    //1. receive packet
    if(p_conn->p_protocol->callback){
        ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_RECV_GETPKT, &command, 0);
    }
    if(WILDDOG_ERR_NOERR != ret){
        return ret;
    }

    //2. try to find the send pkt which has same message id, 
    //but if we are not authed, only accept auth pkt.
    sendPkt == _wilddog_conn_recv_sendPktFind(p_conn,message_id);
    if(NULL == sendPkt){
        //delete the recvPkt.        
        if(p_conn->p_protocol->callback){
            ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_RECV_FREEPKT, &command, 0);
        }
        wilddog_debug_level(WD_DEBUG_WARN, "Received an unmatched packet, mid = 0x%x!",message_id);
        return ret;
    }
    //3. handle packet
    command.p_data = sendPkt->p_data;
    command.p_proto_data = sendPkt->p_proto_data;
    if(p_conn->p_protocol->callback){
        ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_RECV_HANDLEPKT, &command, 0);
    }
    //4. state machine
    
    //5. if has retransmit packet, send
    return ret;
}
/* send interface */
Wilddog_Func_T _wilddog_conn_funcTable[WILDDOG_CONN_CMD_MAX + 1] = 
{
    (Wilddog_Func_T)NULL,//get
    (Wilddog_Func_T)NULL,//set
    (Wilddog_Func_T)NULL,//push
    (Wilddog_Func_T)NULL,//remove
    (Wilddog_Func_T)NULL,//on
    (Wilddog_Func_T)NULL,//off
    (Wilddog_Func_T)NULL,//auth
    (Wilddog_Func_T)NULL,//ondisset
    (Wilddog_Func_T)NULL,//ondispush
    (Wilddog_Func_T)NULL,//ondisremove
    (Wilddog_Func_T)NULL,//ondiscancel
    (Wilddog_Func_T)NULL,//offline
    (Wilddog_Func_T)NULL,//online
    (Wilddog_Func_T)_wilddog_conn_trySync,//trysync
    (Wilddog_Func_T)NULL
};

STATIC int WD_SYSTEM _wilddog_conn_ioctl(
    Wilddog_Conn_Cmd_T cmd,
    void *p_args,
    int flags
    )
{
    if( cmd  >= WILDDOG_CONN_CMD_MAX )
        return WILDDOG_ERR_INVALID;
    
    if(_wilddog_conn_funcTable[cmd]){
        return (_wilddog_conn_funcTable[cmd])(p_args,flags);
    }
    else{
        wilddog_debug_level(WD_DEBUG_ERROR, "Cannot find function %d!",cmd);
        return WILDDOG_ERR_NULL;
    }
}

/*
 * Function:    _wilddog_conn_init
 * Description: creat session and register send and trysync function.
 *   
 * Input:       p_repo: the pointer of the repo struct
 * Output:      N/A
 * Return:      the result
*/
Wilddog_Conn_T * WD_SYSTEM _wilddog_conn_init(Wilddog_Repo_T *p_repo)
{
    Wilddog_Conn_T* p_conn = NULL;
    
    wilddog_assert(p_repo, NULL);

    p_conn = (Wilddog_Conn_T*)wmalloc(sizeof(Wilddog_Conn_T));

    wilddog_assert(p_conn, NULL);

    p_conn->p_conn_repo = p_repo;
    p_conn->f_conn_ioctl = (Wilddog_Func_T)_wilddog_conn_ioctl;
    p_conn->d_conn_sys.d_curr_ping_interval = WILDDOG_DEFALUT_PING_INTERVAL;
    p_conn->d_conn_sys.d_ping_delta = WILDDOG_DEFAULT_PING_DELTA;

    //Init protocol layer.
    p_conn->p_protocol = _wilddog_protocol_init(p_conn);
    if(NULL == p_conn->p_protocol){
        wfree(p_conn);
        wilddog_debug_level(WD_DEBUG_ERROR, "Init protocol failed!");
        return NULL;
    }
    
    //Init session.
    if(WILDDOG_ERR_NOERR != _wilddog_conn_sessionInit(p_conn)){
        _wilddog_protocol_deInit(p_conn);
        wfree(p_conn);
        wilddog_debug_level(WD_DEBUG_ERROR, "Init session failed!");
        return NULL;
    }
    
    return p_conn;
}
/*
 * Function:    _wilddog_conn_deinit
 * Description: conn layer  deinit function
 *   
 * Input:       p_repo: the pointer of the repo struct
 * Output:      N/A
 * Return:      0 or errorcode
*/
Wilddog_Return_T WD_SYSTEM _wilddog_conn_deinit(Wilddog_Repo_T *p_repo)
{
    Wilddog_Conn_T* p_conn = NULL;
    
    wilddog_assert(p_repo, WILDDOG_ERR_NULL);

    p_conn = p_repo->p_rp_conn;

    wilddog_assert(p_conn, WILDDOG_ERR_NULL);
    //TODO: Deinit pkts 
    if(p_conn->d_conn_sys.p_auth){
        _wilddog_conn_packet_deInit(p_conn->d_conn_sys.p_auth);
        wfree(p_conn->d_conn_sys.p_auth);
        p_conn->d_conn_sys.p_auth = NULL;
    }
    if(p_conn->d_conn_sys.p_ping){
        _wilddog_conn_packet_deInit(p_conn->d_conn_sys.p_ping);
        wfree(p_conn->d_conn_sys.p_ping);
        p_conn->d_conn_sys.p_ping = NULL;
    }
    if(p_conn->d_conn_user.p_observer_list){
        Wilddog_Conn_Pkt_T *curr, *p_tmp;
        LL_FOREACH_SAFE(p_conn->d_conn_user.p_observer_list,curr,p_tmp){
            if(curr){
                LL_DELETE(p_conn->d_conn_user.p_observer_list, curr);
                _wilddog_conn_packet_deInit(curr);
                wfree(curr);
            }
        }
    }
    p_conn->d_conn_user.p_observer_list = NULL;
    
    if(p_conn->d_conn_user.p_rest_list){
        Wilddog_Conn_Pkt_T *curr, *p_tmp;
        LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,p_tmp){
            if(curr){
                LL_DELETE(p_conn->d_conn_user.p_rest_list, curr);
                _wilddog_conn_packet_deInit(curr);
                wfree(curr);
            }
        }
    }
    p_conn->d_conn_user.p_rest_list = NULL;
    //TODO: Deinit session.
    
    //TODO: Deinit protocol layer.
    _wilddog_protocol_deInit(p_conn);
    wfree(p_conn);
    p_repo->p_rp_conn = NULL;

    return WILDDOG_ERR_NOERR;
}

