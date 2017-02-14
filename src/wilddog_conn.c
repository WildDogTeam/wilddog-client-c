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
 * 1.2.0        jimmy           2017-01-09  Rewrite connect layer logic.
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

#define WILDDOG_RETRANSMIT_DEFAULT_INTERVAL (1)//默认的数据包重传间隔,s
#define WILDDOG_SESSION_OFFLINE_TIMES (3)//session 建立失败多少次后，认为离线
#define WILDDOG_SESSION_MAX_RETRY_TIME_INTERVAL (150)//最大离线重试间隔
#define WILDDOG_SMART_PING

#ifdef WILDDOG_SMART_PING
#define WILDDOG_DEFAULT_PING_INTERVAL (19) //初始化的ping间隔,s
#define WILDDOG_DEFAULT_PING_DELTA (8) //ping的初始化步进间隔,s
//180s is server session keepalive time, so ping interval cannot more than it,
//and we must consider the transmit timeout.
#else
#define WILDDOG_DEFAULT_PING_INTERVAL (25) //默认ping间隔,s
#define WILDDOG_DEFAULT_PING_DELTA (0) //ping的步进间隔,s
#define WILDDOG_FORCE_FALLBACK_TIME (2)// if ping failed, force fallback
#endif
#define WILDDOG_MAX_PING_INTERVAL (175 - (WILDDOG_RETRANSMITE_TIME/1000))//最大ping间隔,s

#define WILDDOG_AUTH_SHORT_TKN_KEY "s"
#define WILDDOG_AUTH_LONG_TKN_KEY "l"

extern Wilddog_Return_T WD_SYSTEM _wilddog_node_setKey
    (
    Wilddog_Node_T *node, 
    Wilddog_Str_T *key
    );
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_packet_deInit(Wilddog_Conn_Pkt_T * pkt);
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_packet_init(Wilddog_Conn_Pkt_T * pkt,Wilddog_Url_T *s_url);

STATIC INLINE u32 WD_SYSTEM _wilddog_conn_getNextSendTime(int count){
    return (_wilddog_getTime() + (WILDDOG_RETRANSMIT_DEFAULT_INTERVAL << count) * 1000);
}
/*
    Ping policy: When authed:
    1. ping interval = WILDDOG_DEFAULT_PING_INTERVAL, delta = WILDDOG_DEFAULT_PING_DELTA,
    2. Send GET coap://<appid>.wilddogio.com/.ping?.cs=<short token>
    3.1 if recv 200 OK, interval += delta, interval max is 165. continue 2
    3.2 else if recv 401 UNAUTHED, interval -= delta, delta /= 2, goto 4
    4. Send POST coap://<appid>.wilddogio.com/.rst, payload is long token
    5.1 if recv 200 OK, interval += delta, continue 2
    5.2 else if recv 400 BADREQUEST, reset interval and delta, set to unauth.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_ping_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;

    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);

    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive ping packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);

    switch(error_code){
        case WILDDOG_HTTP_OK:
        {
            //interval += delta, send short token next time
            if(WILDDOG_MAX_PING_INTERVAL <= p_conn->d_conn_sys.d_curr_ping_interval){
                p_conn->d_conn_sys.d_ping_delta = 0;
                p_conn->d_conn_sys.d_curr_ping_interval = WILDDOG_MAX_PING_INTERVAL;
            }else
                p_conn->d_conn_sys.d_curr_ping_interval += p_conn->d_conn_sys.d_ping_delta;
            p_conn->d_conn_sys.d_ping_next_send_time = _wilddog_getTime() + \
                                        p_conn->d_conn_sys.d_curr_ping_interval * 1000;
            p_conn->d_conn_sys.d_ping_type = WILDDOG_PING_TYPE_SHORT;
            ret = WILDDOG_ERR_NOERR;
            break;
        }
        case WILDDOG_HTTP_BAD_REQUEST:
        {
            //reset interval and delta
            p_conn->d_conn_sys.d_curr_ping_interval = WILDDOG_DEFAULT_PING_INTERVAL;
            p_conn->d_conn_sys.d_ping_delta = WILDDOG_DEFAULT_PING_DELTA;
            p_conn->d_conn_sys.d_ping_type = WILDDOG_PING_TYPE_SHORT;
            p_conn->d_conn_sys.d_ping_next_send_time = 0;
            //if status is init, means appid is not exist, so keep the status.
            if(WILDDOG_SESSION_INIT != p_conn->d_session.d_session_status){
                p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
            }
            ret = WILDDOG_ERR_NOERR;
            break;
        }
        case WILDDOG_HTTP_UNAUTHORIZED:
        {
            //delta = 0,means it was stable, but now env changed.
            if(0 == p_conn->d_conn_sys.d_ping_delta){
                p_conn->d_conn_sys.d_ping_delta = WILDDOG_DEFAULT_PING_DELTA;
                p_conn->d_conn_sys.d_curr_ping_interval = WILDDOG_DEFAULT_PING_INTERVAL;
            }
            else{
                //interval -= delta, delta /= 2, send long token right now.
                p_conn->d_conn_sys.d_curr_ping_interval -= p_conn->d_conn_sys.d_ping_delta;
                p_conn->d_conn_sys.d_ping_delta /= 2;
            }
            p_conn->d_conn_sys.d_ping_type = WILDDOG_PING_TYPE_LONG;
            p_conn->d_conn_sys.d_ping_next_send_time = _wilddog_getTime();
            ret = WILDDOG_ERR_NOERR;
            break;
        }
        default:
        {
            wilddog_debug_level(WD_DEBUG_WARN, "Get an error [%d].",(int)error_code);
        }
    }

    //the pkt was handled, free pkt
    if(p_conn->d_conn_sys.p_ping){
        _wilddog_conn_packet_deInit(p_conn->d_conn_sys.p_ping);
        wfree(p_conn->d_conn_sys.p_ping);
        p_conn->d_conn_sys.p_ping = NULL;
    }
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_get_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Node_T *p_node = NULL;
    Wilddog_Conn_Pkt_T *curr,*tmp;
    
    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);

    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive get packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);
    
    if(WILDDOG_HTTP_OK == error_code){
        //handle the payload
        Wilddog_Payload_T node_payload;
        Wilddog_Str_T *p_path = NULL;
        
        p_path = _wilddog_url_getKey(pkt->p_url->p_url_path);
        
        if(NULL == payload){    
            p_node = wilddog_node_createNull(NULL);
            wilddog_assert(p_node, WILDDOG_ERR_NULL);
        }else{
            node_payload.p_dt_data = payload;
            node_payload.d_dt_len = payload_len;
            node_payload.d_dt_pos = 0;
            
            //malloced a p_node
            p_node = _wilddog_payload2Node(&node_payload);
            wilddog_assert(p_node, WILDDOG_ERR_NULL);
            //sepecial: data {} change to null
            if(WILDDOG_NODE_TYPE_OBJECT == p_node->d_wn_type && \
               NULL == p_node->p_wn_child)
            {
                wilddog_node_delete(p_node);
                p_node = wilddog_node_createNull(NULL);
                wilddog_assert(p_node, WILDDOG_ERR_NULL);
            }
        }
        if(p_path){
            _wilddog_node_setKey(p_node, p_path);
        }
        ret = WILDDOG_ERR_NOERR;

    }else if(WILDDOG_HTTP_UNAUTHORIZED == error_code){
        p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
        return WILDDOG_ERR_IGNORE;
    }else{
        wilddog_debug_level(WD_DEBUG_WARN, "Get an error [%d].",(int)error_code);
    }
    
    //user callback
    if(pkt->p_user_callback){
        wilddog_debug_level(WD_DEBUG_LOG, "Tigger getValue callback.");
        (pkt->p_user_callback)(p_node,pkt->p_user_arg,error_code);
    }

    if(p_node)
        wilddog_node_delete(p_node);

    LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,tmp){
        if(curr == pkt){
            //match, remove it
            LL_DELETE(p_conn->d_conn_user.p_rest_list, curr);
            _wilddog_conn_packet_deInit(curr);
            wfree(curr);
            p_conn->d_conn_user.d_count--;
            break;
        }
    }
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_set_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Conn_Pkt_T *curr,*tmp;
    
    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);

    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive set packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);

    if(WILDDOG_HTTP_OK == error_code){
        ret = WILDDOG_ERR_NOERR;
    }else if(WILDDOG_HTTP_UNAUTHORIZED == error_code){
        p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
        return WILDDOG_ERR_IGNORE;
    }else{
        wilddog_debug_level(WD_DEBUG_WARN, "Get an error [%d].",(int)error_code);
    }
    
    //user callback
    if(pkt->p_user_callback){
        wilddog_debug_level(WD_DEBUG_LOG, "Tigger setValue callback.");
        (pkt->p_user_callback)(pkt->p_user_arg,error_code);
    }

    LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,tmp){
        if(curr == pkt){
            //match, remove it
            LL_DELETE(p_conn->d_conn_user.p_rest_list, curr);
            _wilddog_conn_packet_deInit(curr);
            wfree(curr);
            p_conn->d_conn_user.d_count--;
            break;
        }
    }
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_push_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Conn_Pkt_T *curr,*tmp;
    
    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);

    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive push packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);
    
    if(WILDDOG_HTTP_OK == error_code){
        ret = WILDDOG_ERR_NOERR;
    }else if(WILDDOG_HTTP_UNAUTHORIZED == error_code){
        p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
        return WILDDOG_ERR_IGNORE;
    }else{
        wilddog_debug_level(WD_DEBUG_WARN, "Get an error [%d].",(int)error_code);
    }
    
    //user callback
    if(pkt->p_user_callback){
        wilddog_debug_level(WD_DEBUG_LOG, "Tigger push callback.");
        (pkt->p_user_callback)(payload,pkt->p_user_arg,error_code);
    }

    LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,tmp){
        if(curr == pkt){
            //match, remove it
            LL_DELETE(p_conn->d_conn_user.p_rest_list, curr);
            _wilddog_conn_packet_deInit(curr);
            wfree(curr);
            p_conn->d_conn_user.d_count--;
            break;
        }
    }
    return ret;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_remove_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Conn_Pkt_T *curr,*tmp;
    
    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);

    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive remove packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);

    switch(error_code){
        case WILDDOG_HTTP_OK:
        {
            ret = WILDDOG_ERR_NOERR;
            break;
        }
        case WILDDOG_HTTP_UNAUTHORIZED:
        {
            //need reauth, this happened only when server lost our session.
            p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
            return WILDDOG_ERR_IGNORE;
        }
        case WILDDOG_HTTP_INTERNAL_SERVER_ERR:
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "Receive server internal error");
            //ret = WILDDOG_ERR_IGNORE;//do not ignore, just handle it normally.
            break;
        }
        case WILDDOG_HTTP_PRECONDITION_FAIL:
        {
            wilddog_debug_level(WD_DEBUG_WARN, "Precondition failed ! May be the pay package is overflow.");
            break;
        }
        case WILDDOG_ERR_RECVTIMEOUT:
        {
            wilddog_debug_level(WD_DEBUG_WARN,"Receive timeout.");
            break;
        }
        default:
        {
            wilddog_debug_level(WD_DEBUG_WARN, "Get an error [%d].",(int)error_code);
            break;
        }
    }
    
    //user callback
    if(pkt->p_user_callback){
        wilddog_debug_level(WD_DEBUG_LOG, "Tigger removeValue callback.");
        (pkt->p_user_callback)(pkt->p_user_arg,error_code);
    }

    LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,tmp){
        if(curr == pkt){
            //match, remove it
            LL_DELETE(p_conn->d_conn_user.p_rest_list, curr);
            _wilddog_conn_packet_deInit(curr);
            wfree(curr);
            p_conn->d_conn_user.d_count--;
            break;
        }
    }
    return ret;

}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_addObserver_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Node_T *p_node = NULL;
    Wilddog_Conn_Pkt_T *curr,*tmp;
    
    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);

    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive addObserver packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);
    
    if(WILDDOG_HTTP_OK == error_code){
        //handle the payload
        Wilddog_Payload_T node_payload;
        Wilddog_Str_T *p_path = NULL;
        
        p_path = _wilddog_url_getKey(pkt->p_url->p_url_path);
        
        if(NULL == payload){    
            p_node = wilddog_node_createNull(NULL);
            wilddog_assert(p_node, WILDDOG_ERR_NULL);
        }else{
            node_payload.p_dt_data = payload;
            node_payload.d_dt_len = payload_len;
            node_payload.d_dt_pos = 0;
            
            //malloced a p_node
            p_node = _wilddog_payload2Node(&node_payload);
            wilddog_assert(p_node, WILDDOG_ERR_NULL);
            //sepecial: data {} change to null
            if(WILDDOG_NODE_TYPE_OBJECT == p_node->d_wn_type && \
               NULL == p_node->p_wn_child)
            {
                wilddog_node_delete(p_node);
                p_node = wilddog_node_createNull(NULL);
                wilddog_assert(p_node, WILDDOG_ERR_NULL);
            }
        }
        if(p_path){
            _wilddog_node_setKey(p_node, p_path);
        }
        //change pkt to never timeout, next send time to maximum.
        pkt->d_flag |= WILDDOG_CONN_PKT_FLAG_NEVERTIMEOUT;
        pkt->d_next_send_time = 0xfffffff;
        ret = WILDDOG_ERR_NOERR;

    }else if(WILDDOG_HTTP_UNAUTHORIZED == error_code){
        p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
        return WILDDOG_ERR_IGNORE;
    }else{
        wilddog_debug_level(WD_DEBUG_WARN, "Get an error [%d].",(int)error_code);
    }
    
    //user callback
    if(pkt->p_user_callback){
        wilddog_debug_level(WD_DEBUG_LOG, "Tigger addObserver callback.");
        (pkt->p_user_callback)(p_node,pkt->p_user_arg,error_code);
    }

    if(p_node)
        wilddog_node_delete(p_node);
    //if received error, remove it
    //if reconnect, do not remove.
    if(WILDDOG_HTTP_OK != error_code){
        LL_FOREACH_SAFE(p_conn->d_conn_user.p_observer_list,curr,tmp){
            if(curr == pkt){
                //match, remove it
                LL_DELETE(p_conn->d_conn_user.p_observer_list, curr);
                _wilddog_conn_packet_deInit(curr);
                wfree(curr);
                p_conn->d_conn_user.d_count--;
                break;
            }
        }
    }

    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_removeObserver_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Conn_Pkt_T *curr,*tmp;
    
    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);

    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive removeObserver packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);

    if(WILDDOG_HTTP_OK == error_code){
        ret = WILDDOG_ERR_NOERR;
    }else if(WILDDOG_HTTP_UNAUTHORIZED == error_code){
        p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
        return WILDDOG_ERR_IGNORE;
    }else{
        wilddog_debug_level(WD_DEBUG_WARN, "Get an error [%d].",(int)error_code);
    }
    
    //user callback
    if(pkt->p_user_callback){
        wilddog_debug_level(WD_DEBUG_LOG, "Tigger removeObserver callback.");
        (pkt->p_user_callback)(pkt->p_user_arg,error_code);
    }

    LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,tmp){
        if(curr == pkt){
            //match, remove it
            LL_DELETE(p_conn->d_conn_user.p_rest_list, curr);
            _wilddog_conn_packet_deInit(curr);
            wfree(curr);
            p_conn->d_conn_user.d_count--;
            break;
        }
    }
    return ret;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_disCommon_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Conn_Pkt_T *curr,*tmp;
    
    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);

    if(WILDDOG_HTTP_OK == error_code){
        ret = WILDDOG_ERR_NOERR;
    }else if(WILDDOG_HTTP_UNAUTHORIZED == error_code){
        p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
        return WILDDOG_ERR_IGNORE;
    }else{
        wilddog_debug_level(WD_DEBUG_WARN, "Get an error [%d].",(int)error_code);
    }
    
    //user callback
    if(pkt->p_user_callback){
        (pkt->p_user_callback)(pkt->p_user_arg,error_code);
    }

    LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,tmp){
        if(curr == pkt){
            //match, remove it
            LL_DELETE(p_conn->d_conn_user.p_rest_list, curr);
            _wilddog_conn_packet_deInit(curr);
            wfree(curr);
            p_conn->d_conn_user.d_count--;
            break;
        }
    }
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_disSet_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive dis set packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);
    return _wilddog_conn_disCommon_callback(p_conn,pkt,payload,payload_len,error_code);
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_disPush_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive dis push packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);
    return _wilddog_conn_disCommon_callback(p_conn,pkt,payload,payload_len,error_code);
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_disRemove_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive dis remove packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);
    return _wilddog_conn_disCommon_callback(p_conn,pkt,payload,payload_len,error_code);
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_disCancel_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive dis cancel packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);
    return _wilddog_conn_disCommon_callback(p_conn,pkt,payload,payload_len,error_code);
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_offline_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Conn_Pkt_T *curr,*tmp;
    
    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);

    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive offline packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);

    if(WILDDOG_HTTP_OK == error_code){
        ret = WILDDOG_ERR_NOERR;
    }else if(WILDDOG_HTTP_UNAUTHORIZED == error_code){
        p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
        return WILDDOG_ERR_IGNORE;
    }else{
        wilddog_debug_level(WD_DEBUG_WARN, "Get an error [%d].",(int)error_code);
    }

    LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,tmp){
        if(curr == pkt){
            //match, remove it
            LL_DELETE(p_conn->d_conn_user.p_rest_list, curr);
            _wilddog_conn_packet_deInit(curr);
            wfree(curr);
            p_conn->d_conn_user.d_count--;
            break;
        }
    }
    return ret;

}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_auth_callback
    (
    Wilddog_Conn_T *p_conn, 
    Wilddog_Conn_Pkt_T *pkt, 
    u8* payload, 
    u32 payload_len, 
    Wilddog_Return_T error_code
    )
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Conn_Pkt_T *curr,*tmp;
    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);
    wilddog_assert(p_conn->d_conn_sys.p_auth == pkt, WILDDOG_ERR_INVALID);

    wilddog_debug_level(WD_DEBUG_LOG, \
        "Receive auth packet [0x%x], return code is [%d]", \
        (unsigned int)pkt->d_message_id,error_code);
    switch(error_code){
        case WILDDOG_HTTP_OK:
        {
            //1. store short token and long token
            //2. change auth status to authed
            //3. trigger user callback
            //4. if ok, free pkt.
            Wilddog_Payload_T node_payload;
            Wilddog_Node_T *p_node = NULL;
            wilddog_assert(payload, WILDDOG_ERR_IGNORE);
    
            node_payload.p_dt_data = payload;
            node_payload.d_dt_len = payload_len;
            node_payload.d_dt_pos = 0;
    
            //malloced a p_node
            p_node = _wilddog_payload2Node(&node_payload);
    
            wilddog_assert(p_node, WILDDOG_ERR_IGNORE);
            if(!p_node->p_wn_child || !p_node->p_wn_child->p_wn_next){
                wilddog_debug_level(WD_DEBUG_ERROR,"Node's child is null!");
                wilddog_node_delete(p_node);
                return WILDDOG_ERR_IGNORE;
            }
            //p_node contain a 's' and 'l' node, which are short and long token.
            if(WILDDOG_NODE_TYPE_UTF8STRING != p_node->p_wn_child->d_wn_type ||
                WILDDOG_NODE_TYPE_UTF8STRING != p_node->p_wn_child->p_wn_next->d_wn_type){
                wilddog_debug_level(WD_DEBUG_ERROR, \
                    "Node type is %d and %d,not string!",
                    p_node->p_wn_child->d_wn_type,
                    p_node->p_wn_child->p_wn_next->d_wn_type);
                wilddog_node_delete(p_node);
                return WILDDOG_ERR_IGNORE;
            }
            if(!p_node->p_wn_child->p_wn_key|| \
               !p_node->p_wn_child->p_wn_value|| \
               !p_node->p_wn_child->p_wn_next->p_wn_key|| \
               !p_node->p_wn_child->p_wn_next->p_wn_value){
                wilddog_debug_level(WD_DEBUG_ERROR, \
                    "Node child: key[%s]value[%s],next:key[%s]value[%s] is NULL!", \
                    p_node->p_wn_child->p_wn_key,
                    p_node->p_wn_child->p_wn_value,
                    p_node->p_wn_child->p_wn_next->p_wn_key,
                    p_node->p_wn_child->p_wn_next->p_wn_value);
                wilddog_node_delete(p_node);
                return WILDDOG_ERR_IGNORE;
            }
            if(!strcmp((const char*)p_node->p_wn_child->p_wn_key,WILDDOG_AUTH_SHORT_TKN_KEY)){
                if(strcmp((const char*)p_node->p_wn_child->p_wn_next->p_wn_key,WILDDOG_AUTH_LONG_TKN_KEY)){
                    //short match, but long not find
                    wilddog_debug_level(WD_DEBUG_ERROR, "long token not find!");
                    wilddog_node_delete(p_node);
                    return WILDDOG_ERR_IGNORE;
                }
                memset(p_conn->d_session.short_sid, 0,WILDDOG_CONN_SESSION_SHORT_LEN);
                memset(p_conn->d_session.long_sid, 0,WILDDOG_CONN_SESSION_LONG_LEN);
                //short token, store it.
                strncpy((char*)p_conn->d_session.short_sid, \
                        (char*)p_node->p_wn_child->p_wn_value, \
                        WILDDOG_CONN_SESSION_SHORT_LEN - 1);
                //long token, store it.
                strncpy((char*)p_conn->d_session.long_sid, \
                        (char*)p_node->p_wn_child->p_wn_next->p_wn_value,\
                        WILDDOG_CONN_SESSION_LONG_LEN - 1);
            }else if(!strcmp((const char*)p_node->p_wn_child->p_wn_key,WILDDOG_AUTH_LONG_TKN_KEY)){
                if(strcmp((const char*)p_node->p_wn_child->p_wn_next->p_wn_key,WILDDOG_AUTH_SHORT_TKN_KEY)){
                    //long match, but short not find
                    wilddog_debug_level(WD_DEBUG_ERROR, "short token not find!");
                    wilddog_node_delete(p_node);
                    return WILDDOG_ERR_IGNORE;
                }
                memset(p_conn->d_session.short_sid, 0,WILDDOG_CONN_SESSION_SHORT_LEN);
                memset(p_conn->d_session.long_sid, 0,WILDDOG_CONN_SESSION_LONG_LEN);
                //short token, store it.
                strncpy((char*)p_conn->d_session.short_sid, \
                        (char*)p_node->p_wn_child->p_wn_next->p_wn_value, \
                        WILDDOG_CONN_SESSION_SHORT_LEN - 1);
                //long token, store it.
                strncpy((char*)p_conn->d_session.long_sid, \
                        (char*)p_node->p_wn_child->p_wn_value, \
                        WILDDOG_CONN_SESSION_LONG_LEN - 1);
            }else{
                //short and long not find
                wilddog_debug_level(WD_DEBUG_ERROR, "short and long token not find!");
                wilddog_node_delete(p_node);
                return WILDDOG_ERR_IGNORE;
            }
            p_conn->d_session.d_session_status = WILDDOG_SESSION_AUTHED;
            wilddog_debug_level(WD_DEBUG_LOG, \
                "Auth success!Short token is %s, long token is %s", \
                p_conn->d_session.short_sid,
                p_conn->d_session.long_sid);
    
            //free p_node
            wilddog_node_delete(p_node);
            ret = WILDDOG_ERR_NOERR;
            //change observe/rest stored packets' send time to now.
            LL_FOREACH_SAFE(p_conn->d_conn_user.p_observer_list,curr,tmp){
                if(curr){
                    curr->d_flag &= ~WILDDOG_CONN_PKT_FLAG_NEVERTIMEOUT;
                    curr->d_count = 0;
                    curr->d_next_send_time = _wilddog_getTime();
                }
            }
            LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,tmp){
                if(curr){
                    curr->d_next_send_time = _wilddog_getTime();
                }
            }
            //online, change status
            p_conn->d_conn_sys.d_auth_fail_count = 0;
            p_conn->d_conn_sys.d_offline_time = 0;
            p_conn->d_conn_sys.d_online_retry_count = 0;
            break;
        }
        case WILDDOG_HTTP_BAD_REQUEST:
        {
            //cannot find this repo, stop to send auth data.
            p_conn->d_session.d_session_status = WILDDOG_SESSION_INIT;
            wilddog_debug_level(WD_DEBUG_ERROR, \
                "Can not find host %s", p_conn->p_conn_repo->p_rp_url->p_url_host);
            ret = WILDDOG_ERR_INVALID;
            break;
        }
        case WILDDOG_HTTP_INTERNAL_SERVER_ERR:
        {
            //Oh, server down! Wait time to resend this packet.
            wilddog_debug_level(WD_DEBUG_ERROR, "Receive server internal error");
            return WILDDOG_ERR_IGNORE;
        }
        case WILDDOG_ERR_RECVTIMEOUT:
        {
            wilddog_debug_level(WD_DEBUG_WARN, "Receive timeout.");
            p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
            break;
        }
        default:
        {
            //WTF! We don't recognize this error code, can do nothing but reauth.
            p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
            wilddog_debug_level(WD_DEBUG_ERROR, "Receive unknown error %d",error_code);
            ret = WILDDOG_ERR_INVALID;
            break;
        }
    }
    
    //user callback
    if(pkt->p_user_callback){
        wilddog_debug_level(WD_DEBUG_LOG, "Trigger auth callback");
        (pkt->p_user_callback)(pkt->p_user_arg,error_code);
    }
    //the pkt was handled, free pkt
    if(p_conn->d_conn_sys.p_auth){
        _wilddog_conn_packet_deInit(p_conn->d_conn_sys.p_auth);
        wfree(p_conn->d_conn_sys.p_auth);
        p_conn->d_conn_sys.p_auth = NULL;
    }
    return ret;
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

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_packet_init(Wilddog_Conn_Pkt_T * pkt,Wilddog_Url_T *s_url){
    wilddog_assert(pkt&&s_url, WILDDOG_ERR_NULL);

    pkt->p_url = (Wilddog_Url_T*)wmalloc(sizeof(Wilddog_Url_T));
    if(NULL == pkt->p_url){
        wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed!");
        return WILDDOG_ERR_NULL;
    }
    
    if(WILDDOG_ERR_NOERR != _wilddog_url_copy(s_url, pkt->p_url)){
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
    pkt->d_register_time = _wilddog_getTime();
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
    Wilddog_Conn_Pkt_T *curr, *tmp;

    wilddog_assert(p_conn,NULL);
        
    if(p_conn->d_conn_sys.p_auth){
        if(TRUE == _wilddog_conn_midCmp(mid, p_conn->d_conn_sys.p_auth->d_message_id)){
            //match, is the auth callback!
            return p_conn->d_conn_sys.p_auth;
        }
    }
    if(p_conn->d_conn_sys.p_ping){
        if(TRUE == _wilddog_conn_midCmp(mid,p_conn->d_conn_sys.p_ping->d_message_id)){
            return p_conn->d_conn_sys.p_ping;
        }
    }
    //observe list check
    LL_FOREACH_SAFE(p_conn->d_conn_user.p_observer_list,curr,tmp){
        if(TRUE == _wilddog_conn_midCmp(mid,curr->d_message_id)){
            return curr;
        }
    }
    //rest list check
    LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,tmp){
        if(TRUE == _wilddog_conn_midCmp(mid,curr->d_message_id)){
            return curr;
        }
    }
    return NULL;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_reconnect(Wilddog_Conn_T *p_conn){
    //reconnect socket 
    Wilddog_Proto_Cmd_Arg_T command;

    wilddog_assert(p_conn&&p_conn->p_protocol, WILDDOG_ERR_NULL);
    
    //send
    command.p_data = NULL;
    command.d_data_len = 0;
    command.p_message_id= NULL;
    command.p_url = NULL;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = NULL;
    command.p_out_data_len = NULL;
    command.p_proto_data = NULL;
    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = NULL;
    command.d_session_len = 0;
    if(p_conn->p_protocol->callback){
        (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_RECONNECT, &command, 0);
    }
    return WILDDOG_ERR_NOERR;
}
STATIC BOOL WD_SYSTEM _wilddog_conn_isTimeout(u32 src, u32 dst, u32 timeout){
    if(src >= dst){
        if(src - dst > timeout)
            return TRUE;
        return FALSE;
    }else{
        //fallback
        if(src + (0xffffffff - dst) > timeout)
            return TRUE;
        return FALSE;
    }
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_retransmitPkt(Wilddog_Conn_T *p_conn,Wilddog_Conn_Pkt_T *pkt){
    Wilddog_Proto_Cmd_Arg_T command;
    wilddog_assert(p_conn&&pkt, WILDDOG_ERR_NULL);
    
    //send to server, get method has no p_data
    command.p_data = (u8*)pkt->p_data->data;
    command.d_data_len = pkt->p_data->len;
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = NULL;
    command.p_out_data_len = NULL;
    command.p_proto_data = NULL;
    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = p_conn->d_session.short_sid;
    command.d_session_len = WILDDOG_CONN_SESSION_SHORT_LEN - 1;

    if(_wilddog_conn_isTimeout(_wilddog_getTime(),pkt->d_register_time,WILDDOG_RETRANSMITE_TIME) && \
        0 == (WILDDOG_CONN_PKT_FLAG_NEVERTIMEOUT & pkt->d_flag)){
        //timeout, trigger callback
        if(pkt->p_complete){
            Wilddog_Return_T ret;
            if(WILDDOG_SESSION_INIT == p_conn->d_session.d_session_status){
                //observe/ping/rest recv timeout and status is init,means
                //auth received bad request.
                ret = WILDDOG_HTTP_BAD_REQUEST;
            }else{
                ret = WILDDOG_ERR_RECVTIMEOUT;
            }
            (pkt->p_complete)(p_conn,pkt,NULL,0, ret);
        }
        p_conn->d_timeout_count++;
        return WILDDOG_ERR_RECVTIMEOUT;
    }else if(_wilddog_getTime() >= pkt->d_next_send_time){
        //if authed, we can handle retransmit.
        if(WILDDOG_SESSION_AUTHED == p_conn->d_session.d_session_status){
            pkt->d_count++;
            pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
            if(p_conn->p_protocol->callback){
                wilddog_debug_level(WD_DEBUG_LOG, "Retransmit pkt 0x%x",(unsigned int)pkt->d_message_id);
                (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_RETRANSMIT, &command, 0);
            }
        }else{
            //not authed
//            wilddog_debug_level(WD_DEBUG_WARN, "Not authed, do not retransmit packet 0x%x.",(unsigned int)pkt->d_message_id);
            //pkt->d_count++;
            //pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
        }
    }
    return WILDDOG_ERR_NOERR;
}
/*
    Ping policy: When authed:
    1. ping interval = WILDDOG_DEFAULT_PING_INTERVAL, delta = WILDDOG_DEFAULT_PING_DELTA,
    2. Send GET coap://<appid>.wilddogio.com/.ping?.cs=<short token>
    3.1 if recv 200 OK, interval += delta, continue 2
    3.2 else if recv 401 UNAUTHED, interval -= delta, delta /= 2, goto 4
    4. Send POST coap://<appid>.wilddogio.com/.rst, payload is long token
    5.1 if recv 200 OK, interval += delta, continue 2
    5.2 else if recv 400 BADREQUEST, reset interval and delta, set to unauth.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_pingHandler(Wilddog_Conn_T* p_conn){
    Wilddog_Conn_Pkt_T *pkt = p_conn->d_conn_sys.p_ping;

    wilddog_assert(p_conn&&p_conn->p_protocol, WILDDOG_ERR_NULL);
    //already handled ping retransmit and timeout, now we handle send
    if(WILDDOG_SESSION_AUTHED == p_conn->d_session.d_session_status){
        Wilddog_Proto_Cmd_Arg_T command;
        command.p_data = NULL;
        command.d_data_len = 0;
        command.p_message_id= NULL;
        command.p_url = NULL;
        command.protocol = p_conn->p_protocol;
        command.p_out_data = NULL;
        command.p_out_data_len = NULL;
        command.p_proto_data = NULL;        
        if(0 == p_conn->d_conn_sys.d_ping_next_send_time || p_conn->d_conn_sys.d_ping_next_send_time < _wilddog_getTime()){
            //have not initialized
            if(0 == p_conn->d_conn_sys.d_ping_next_send_time){
                p_conn->d_conn_sys.d_curr_ping_interval = WILDDOG_DEFAULT_PING_INTERVAL;
                p_conn->d_conn_sys.d_ping_delta = WILDDOG_DEFAULT_PING_DELTA;
                p_conn->d_conn_sys.d_ping_next_send_time =  _wilddog_getTime() + p_conn->d_conn_sys.d_curr_ping_interval * 1000;
                p_conn->d_conn_sys.d_ping_type = WILDDOG_PING_TYPE_SHORT;
            }
            //send to server
            if(pkt){
                _wilddog_conn_packet_deInit(pkt);
                wfree(pkt);
                p_conn->d_conn_sys.p_ping = NULL;
            }
            pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
            wilddog_assert(pkt, WILDDOG_ERR_NULL);
            
            if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, p_conn->p_conn_repo->p_rp_url)){
                wfree(pkt);
                wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
                return WILDDOG_ERR_NULL;
            }
            pkt->p_complete = (Wilddog_Func_T)_wilddog_conn_ping_callback;
            //add to ping queue
            p_conn->d_conn_sys.p_ping = pkt;

            command.p_message_id= &pkt->d_message_id;
            command.p_url = pkt->p_url;
            //send pkt must need session info, exclude auth pkt.
            if(WILDDOG_PING_TYPE_SHORT == p_conn->d_conn_sys.d_ping_type){
                command.p_session_info = p_conn->d_session.short_sid;
                command.d_session_len = WILDDOG_CONN_SESSION_SHORT_LEN - 1;
                command.p_out_data = (u8**)&(pkt->p_data);
                command.p_out_data_len = NULL;
                if(p_conn->p_protocol->callback){
                    (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_PING, &command,FALSE);
                    wilddog_debug_level(WD_DEBUG_LOG, "Send short ping pkt 0x%x at %ld ms, interval is %d", \
                        (unsigned int)pkt->d_message_id, _wilddog_getTime(),p_conn->d_conn_sys.d_curr_ping_interval);
                }
            }else{
                command.p_session_info = p_conn->d_session.long_sid;
                command.d_session_len = WILDDOG_CONN_SESSION_LONG_LEN - 1;
                command.p_out_data = (u8**)&(pkt->p_data);
                command.p_out_data_len = NULL;
                if(p_conn->p_protocol->callback){
                    (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_PING, &command,TRUE);
                    wilddog_debug_level(WD_DEBUG_LOG, "Send long ping pkt 0x%x at %ld ms, interval is %d", \
                        (unsigned int)pkt->d_message_id,_wilddog_getTime(),p_conn->d_conn_sys.d_curr_ping_interval);
                }
            }
            pkt->d_count++;
            pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
        }
    }
    return WILDDOG_ERR_NOERR;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_retransmitHandler(Wilddog_Conn_T *p_conn){
    u32 last_timeout_count = 0;
    Wilddog_Conn_Pkt_T *pkt = NULL;
    Wilddog_Conn_Pkt_T *curr, *tmp;
    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    last_timeout_count = p_conn->d_timeout_count;
    //1. timeout handle
    //2. retransmit handle
    //if authed we can handle retransmit event
    pkt = p_conn->d_conn_sys.p_auth;
    if(pkt){
        if(WILDDOG_SESSION_AUTHED == p_conn->d_session.d_session_status){
            //WTF, we already authed, do not need this pkt
            _wilddog_conn_packet_deInit(pkt);
            wfree(pkt);
            p_conn->d_conn_sys.p_auth = NULL;
        }else{
            //check timeout and retransmit
            if(_wilddog_conn_isTimeout(_wilddog_getTime(),pkt->d_register_time,WILDDOG_RETRANSMITE_TIME)){
                //timeout: 
                //1.trigger callback 
                //2.set status to unauth, and add timeout count
                //3.delete it.
                //4.add auth fail count, 
                // if more than WILDDOG_SESSION_OFFLINE_TIMES times, offline.
                //callback
                if(pkt->p_complete){
                    (pkt->p_complete)(p_conn,pkt,NULL,0, WILDDOG_ERR_RECVTIMEOUT);
                }
                p_conn->d_timeout_count++;
                
                if(p_conn->d_conn_sys.p_auth){
                    _wilddog_conn_packet_deInit(p_conn->d_conn_sys.p_auth);
                    wfree(p_conn->d_conn_sys.p_auth);
                    p_conn->d_conn_sys.p_auth = NULL;
                }
                //auth fail handle
                p_conn->d_conn_sys.d_auth_fail_count++;
                if(WILDDOG_SESSION_OFFLINE_TIMES <= p_conn->d_conn_sys.d_auth_fail_count && \
                    0 == p_conn->d_conn_sys.d_offline_time){
                    //first offline
                    p_conn->d_conn_sys.d_offline_time = _wilddog_getTime();
                    p_conn->d_conn_sys.d_online_retry_count = 0;
                }
            }else if(_wilddog_getTime() >= pkt->d_next_send_time){
                //retransmit
                Wilddog_Proto_Cmd_Arg_T command;
                //send to server
                command.p_data = (u8*)pkt->p_data->data;
                command.d_data_len = pkt->p_data->len;
                command.p_message_id= &pkt->d_message_id;
                command.p_url = pkt->p_url;
                command.protocol = p_conn->p_protocol;
                command.p_out_data = NULL;
                command.p_out_data_len = NULL;
                command.p_proto_data = NULL;
                //send pkt must need session info, exclude auth pkt.
                command.p_session_info = NULL;
                command.d_session_len = 0;
                pkt->d_count++;
                pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
                if(p_conn->p_protocol->callback){
                    wilddog_debug_level(WD_DEBUG_LOG, "Retransmit auth pkt 0x%x",(unsigned int)pkt->d_message_id);
                    (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_RETRANSMIT, &command, 0);
                }
            }
        }
    }
    pkt = p_conn->d_conn_sys.p_ping;
    if(pkt){
        _wilddog_conn_retransmitPkt(p_conn,pkt);
    }
    //observe list
    LL_FOREACH_SAFE(p_conn->d_conn_user.p_observer_list,curr,tmp){
        if(curr){
            //flash observe's regster time, never timeout
            _wilddog_conn_retransmitPkt(p_conn,curr);
        }
    }
    //rest list
    LL_FOREACH_SAFE(p_conn->d_conn_user.p_rest_list,curr,tmp){
        if(curr){
            _wilddog_conn_retransmitPkt(p_conn,curr);
        }
    }

    if(last_timeout_count < p_conn->d_timeout_count){
        _wilddog_conn_reconnect(p_conn);
        if(p_conn->d_timeout_count > 3){
            p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
        }
    }
    return WILDDOG_ERR_NOERR;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_commonSet(void* data,int flag, Wilddog_Func_T func,BOOL isDis){
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    Wilddog_Conn_T *p_conn;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_Conn_Pkt_T *pkt;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Payload_T *payload = NULL;
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;
    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    if(p_conn->d_conn_user.d_count > WILDDOG_REQ_QUEUE_NUM){
        wilddog_debug_level(WD_DEBUG_WARN, "Too many requests! Max is %d",WILDDOG_REQ_QUEUE_NUM);
        return WILDDOG_ERR_QUEUEFULL;
    }
    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    wilddog_assert(pkt, WILDDOG_ERR_NULL);
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, arg->p_url)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)func;
    pkt->p_user_callback = arg->p_complete;
    pkt->p_user_arg = arg->p_completeArg;

    //add to rest queue
    LL_APPEND(p_conn->d_conn_user.p_rest_list,pkt);
    p_conn->d_conn_user.d_count++;
    
    if(arg->p_data){
#if (DEBUG_LEVEL)<=(WD_DEBUG_LOG)
        if(arg->p_url->p_url_path)
            wilddog_debug_level(WD_DEBUG_LOG,"Print data want set: \npath %s", \
                                arg->p_url->p_url_path);
        wilddog_debug_printnode(arg->p_data);
        printf("\n");
#endif
        payload = _wilddog_node2Payload(arg->p_data);
#if (DEBUG_LEVEL)<=(WD_DEBUG_LOG)
        {
            int i;
            wilddog_debug_level(WD_DEBUG_LOG,"Send data is:");
            for(i = 0; i < payload->d_dt_len;i++){
                printf("%02x ", *(u8*)(payload->p_dt_data + i));
            }
            printf("\n");
        }
#endif
    }
    //send to server
    command.p_data = NULL;
    command.d_data_len = 0;
    if(payload){
        command.p_data = payload->p_dt_data;
        command.d_data_len = payload->d_dt_len;
    }
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = (u8**)&pkt->p_data;
    command.p_out_data_len = NULL;

    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = p_conn->d_session.short_sid;
    command.d_session_len = WILDDOG_CONN_SESSION_SHORT_LEN - 1;

    if(p_conn->p_protocol->callback){
        BOOL isSend = FALSE;//send to server or not
        if(p_conn->d_session.d_session_status == WILDDOG_SESSION_AUTHED){
            isSend = TRUE;
            ++pkt->d_count;
            pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
        }
        if(TRUE == isDis){
            ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_DIS_SET, &command, isSend);
            wilddog_debug_level(WD_DEBUG_LOG, "Send dis setValue pkt 0x%x",(unsigned int)pkt->d_message_id);
        }else{
            ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_SET, &command, isSend);
            wilddog_debug_level(WD_DEBUG_LOG, "Send setValue pkt 0x%x",(unsigned int)pkt->d_message_id);
        }
    }

    if(payload){
        if(payload->p_dt_data)
            wfree(payload->p_dt_data);
        wfree(payload);
    }
        
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_commonPush(void* data,int flag, Wilddog_Func_T func, BOOL isDis){
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    Wilddog_Conn_T *p_conn;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_Conn_Pkt_T *pkt;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Payload_T *payload = NULL;
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;
    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    if(p_conn->d_conn_user.d_count > WILDDOG_REQ_QUEUE_NUM){
        wilddog_debug_level(WD_DEBUG_WARN, "Too many requests! Max is %d",WILDDOG_REQ_QUEUE_NUM);
        return WILDDOG_ERR_QUEUEFULL;
    }

    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    wilddog_assert(pkt, WILDDOG_ERR_NULL);
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, arg->p_url)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)func;
    pkt->p_user_callback = arg->p_complete;
    pkt->p_user_arg = arg->p_completeArg;

    //add to rest queue
    LL_APPEND(p_conn->d_conn_user.p_rest_list,pkt);
    p_conn->d_conn_user.d_count++;
    
    if(arg->p_data){
#if (DEBUG_LEVEL)<=(WD_DEBUG_LOG)
        if(arg->p_url->p_url_path)
            wilddog_debug_level(WD_DEBUG_LOG,"Print data want push: \npath %s", \
                                arg->p_url->p_url_path);
        wilddog_debug_printnode(arg->p_data);
        printf("\n");
#endif
        payload = _wilddog_node2Payload(arg->p_data);
#if (DEBUG_LEVEL)<=(WD_DEBUG_LOG)
        {
            int i;
            wilddog_debug_level(WD_DEBUG_LOG,"Send data is:");
            for(i = 0; i < payload->d_dt_len;i++){
                printf("%02x ", *(u8*)(payload->p_dt_data + i));
            }
            printf("\n");
        }
#endif
    }

    //send to server
    command.p_data = NULL;
    command.d_data_len = 0;
    if(payload){
        command.p_data = payload->p_dt_data;
        command.d_data_len = payload->d_dt_len;
    }
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = (u8**)&pkt->p_data;
    command.p_out_data_len = NULL;

    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = p_conn->d_session.short_sid;
    command.d_session_len = WILDDOG_CONN_SESSION_SHORT_LEN - 1;

    if(p_conn->p_protocol->callback){
        BOOL isSend = FALSE;//send to server or not
        if(p_conn->d_session.d_session_status == WILDDOG_SESSION_AUTHED){
            isSend = TRUE;
            ++pkt->d_count;
            pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
        }
        if(TRUE == isDis){
            ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_DIS_PUSH, &command, isSend);
            wilddog_debug_level(WD_DEBUG_LOG, "Send dis setValue pkt 0x%x",(unsigned int)pkt->d_message_id);
        }else{
            ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_PUSH, &command, isSend);
            wilddog_debug_level(WD_DEBUG_LOG, "Send setValue pkt 0x%x",(unsigned int)pkt->d_message_id);
        }

    }

    if(payload){
        if(payload->p_dt_data)
            wfree(payload->p_dt_data);
        wfree(payload);
    }
        
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_commonRemove(void* data,int flag, Wilddog_Func_T func, BOOL isDis){
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    Wilddog_Conn_T *p_conn;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_Conn_Pkt_T *pkt;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;
    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    if(p_conn->d_conn_user.d_count > WILDDOG_REQ_QUEUE_NUM){
        wilddog_debug_level(WD_DEBUG_WARN, "Too many requests! Max is %d",WILDDOG_REQ_QUEUE_NUM);
        return WILDDOG_ERR_QUEUEFULL;
    }

    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    wilddog_assert(pkt, WILDDOG_ERR_NULL);
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, arg->p_url)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)func;
    pkt->p_user_callback = arg->p_complete;
    pkt->p_user_arg = arg->p_completeArg;

    //add to rest queue
    LL_APPEND(p_conn->d_conn_user.p_rest_list,pkt);
    p_conn->d_conn_user.d_count++;
    
    //send to server, delete method has no p_data
    command.p_data = NULL;
    command.d_data_len = 0;
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = (u8**)&pkt->p_data;
    command.p_out_data_len = NULL;

    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = p_conn->d_session.short_sid;
    command.d_session_len = WILDDOG_CONN_SESSION_SHORT_LEN - 1;

    if(p_conn->p_protocol->callback){
        BOOL isSend = FALSE;//send to server or not
        if(p_conn->d_session.d_session_status == WILDDOG_SESSION_AUTHED){
            isSend = TRUE;
            ++pkt->d_count;
            pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
        }
        if(TRUE == isDis){
            ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_DIS_REMOVE, &command, isSend);
            wilddog_debug_level(WD_DEBUG_LOG, "Send dis removeValue pkt 0x%x",(unsigned int)pkt->d_message_id);
        }else{
            ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_REMOVE, &command, isSend);
            wilddog_debug_level(WD_DEBUG_LOG, "Send removeValue pkt 0x%x",(unsigned int)pkt->d_message_id);
        }
        
    }
    return ret;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_sessionInit(Wilddog_Conn_T *p_conn){    
    Wilddog_Session_T *p_session;
    Wilddog_Conn_Pkt_T *pkt;
    Wilddog_Proto_Cmd_Arg_T command;

    wilddog_assert(p_conn, WILDDOG_ERR_NULL);
    wilddog_assert(p_conn->p_conn_repo->p_rp_store, WILDDOG_ERR_NULL);
    
    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    wilddog_assert(pkt, WILDDOG_ERR_NULL);
    //All requests want to send out need a pkt structure.We use this structure to
    //find out the response.
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, p_conn->p_conn_repo->p_rp_url)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)_wilddog_conn_auth_callback;
    //add to auth queue
    if(p_conn->d_conn_sys.p_auth){
        _wilddog_conn_packet_deInit(p_conn->d_conn_sys.p_auth);
        wfree(p_conn->d_conn_sys.p_auth);
        p_conn->d_conn_sys.p_auth = NULL;
    }
    p_conn->d_conn_sys.p_auth = pkt;
    
    p_session = &p_conn->d_session;
    p_session->d_session_status = WILDDOG_SESSION_AUTHING;

    //send to server
    command.p_data = NULL;
    command.d_data_len = 0;
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = (u8**)&(pkt->p_data);
    command.p_out_data_len = NULL;
    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = NULL;
    command.d_session_len = 0;

    if(p_conn->p_protocol->callback){
        (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_SESSION_INIT, &command, 0);
        wilddog_debug_level(WD_DEBUG_LOG, "Send init pkt 0x%x",(unsigned int)pkt->d_message_id);
    }
    ++pkt->d_count;
    pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
    return WILDDOG_ERR_NOERR;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_sessionRetry(Wilddog_Conn_T *p_conn){
    wilddog_assert(p_conn,WILDDOG_ERR_NULL);

    if(p_conn->d_conn_sys.d_offline_time != 0){
        //we are now offline, so we need control retry interval.
        u32 sended_time = (1 << p_conn->d_conn_sys.d_online_retry_count);
        sended_time = (sended_time > WILDDOG_SESSION_MAX_RETRY_TIME_INTERVAL)? \
                      (WILDDOG_SESSION_MAX_RETRY_TIME_INTERVAL):(sended_time);
        sended_time = sended_time * 1000;
        //left maybe neg value, but it is unsigned, also bigger than offline time, so it works.
        //and 
        if(_wilddog_getTime() - sended_time < p_conn->d_conn_sys.d_offline_time){
            //is not time to send
            return WILDDOG_ERR_NOERR;
        }else{
            wilddog_debug_level(WD_DEBUG_LOG, "Retry [%d] times to establish session.",p_conn->d_conn_sys.d_online_retry_count);
            p_conn->d_conn_sys.d_online_retry_count++;
            p_conn->d_conn_sys.d_offline_time = _wilddog_getTime();
        }
    }
    return _wilddog_conn_sessionInit(p_conn);
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_get(void* data,int flag){
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    Wilddog_Conn_T *p_conn;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_Conn_Pkt_T *pkt;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;
    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    if(p_conn->d_conn_user.d_count > WILDDOG_REQ_QUEUE_NUM){
        wilddog_debug_level(WD_DEBUG_WARN, "Too many requests! Max is %d",WILDDOG_REQ_QUEUE_NUM);
        return WILDDOG_ERR_QUEUEFULL;
    }
    
    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    wilddog_assert(pkt, WILDDOG_ERR_NULL);
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, arg->p_url)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)_wilddog_conn_get_callback;
    pkt->p_user_callback = arg->p_complete;
    pkt->p_user_arg = arg->p_completeArg;

    //add to rest queue
    LL_APPEND(p_conn->d_conn_user.p_rest_list,pkt);
    p_conn->d_conn_user.d_count++;
    
    //send to server, get method has no p_data
    command.p_data = NULL;
    command.d_data_len = 0;
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = (u8**)&pkt->p_data;
    command.p_out_data_len = NULL;

    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = p_conn->d_session.short_sid;
    command.d_session_len = WILDDOG_CONN_SESSION_SHORT_LEN - 1;
    if(p_conn->p_protocol->callback){
        BOOL isSend = FALSE;//send to server or not
        if(p_conn->d_session.d_session_status == WILDDOG_SESSION_AUTHED){
            isSend = TRUE;
            ++pkt->d_count;
            pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
        }
        
        ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_GET, &command, isSend);
        wilddog_debug_level(WD_DEBUG_LOG, "Send getValue pkt 0x%x",(unsigned int)pkt->d_message_id);
    }
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_set(void* data,int flag){
    return _wilddog_conn_commonSet(data, flag, (Wilddog_Func_T)_wilddog_conn_set_callback,FALSE);
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_push(void* data,int flag){
    return _wilddog_conn_commonPush(data,flag,(Wilddog_Func_T)_wilddog_conn_push_callback,FALSE);
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_remove(void* data,int flag){
    return _wilddog_conn_commonRemove(data, flag, (Wilddog_Func_T)_wilddog_conn_remove_callback,FALSE);
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_addObserver(void* data,int flag){
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    Wilddog_Conn_T *p_conn;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_Conn_Pkt_T *pkt;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;
    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    if(p_conn->d_conn_user.d_count > WILDDOG_REQ_QUEUE_NUM){
        wilddog_debug_level(WD_DEBUG_WARN, "Too many requests! Max is %d",WILDDOG_REQ_QUEUE_NUM);
        return WILDDOG_ERR_QUEUEFULL;
    }

    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    wilddog_assert(pkt, WILDDOG_ERR_NULL);
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, arg->p_url)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)_wilddog_conn_addObserver_callback;
    pkt->p_user_callback = arg->p_complete;
    pkt->p_user_arg = arg->p_completeArg;

    //add to rest queue
    LL_APPEND(p_conn->d_conn_user.p_observer_list,pkt);
    p_conn->d_conn_user.d_count++;
    
    //send to server
    command.p_data = NULL;
    command.d_data_len = 0;
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = (u8**)&pkt->p_data;
    command.p_out_data_len = NULL;
    command.p_proto_data = &(pkt->p_proto_data);
    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = p_conn->d_session.short_sid;
    command.d_session_len = WILDDOG_CONN_SESSION_SHORT_LEN - 1;

    if(p_conn->p_protocol->callback){
        BOOL isSend = FALSE;//send to server or not
        if(p_conn->d_session.d_session_status == WILDDOG_SESSION_AUTHED){
            isSend = TRUE;
            ++pkt->d_count;
            pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
        }
        
        ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_ON, &command, isSend);
        wilddog_debug_level(WD_DEBUG_LOG, "Send addObserver pkt 0x%x",(unsigned int)pkt->d_message_id);
    }
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_removeObserver(void* data,int flag){
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    Wilddog_Conn_T *p_conn;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_Conn_Pkt_T *pkt;
    Wilddog_Conn_Pkt_T *curr,*tmp;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;
    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    if(p_conn->d_conn_user.d_count > WILDDOG_REQ_QUEUE_NUM){
        wilddog_debug_level(WD_DEBUG_WARN, "Too many requests! Max is %d",WILDDOG_REQ_QUEUE_NUM);
        return WILDDOG_ERR_QUEUEFULL;
    }

    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    wilddog_assert(pkt, WILDDOG_ERR_NULL);
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, arg->p_url)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)_wilddog_conn_removeObserver_callback;
    pkt->p_user_callback = arg->p_complete;
    pkt->p_user_arg = arg->p_completeArg;

    //add to rest queue
    LL_APPEND(p_conn->d_conn_user.p_rest_list,pkt);
    p_conn->d_conn_user.d_count++;
    
    //send to server, get method has no p_data
    command.p_data = NULL;
    command.d_data_len = 0;
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = (u8**)&pkt->p_data;
    command.p_out_data_len = NULL;
    command.p_proto_data = NULL;
    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = p_conn->d_session.short_sid;
    command.d_session_len = WILDDOG_CONN_SESSION_SHORT_LEN - 1;

    if(p_conn->p_protocol->callback){
        BOOL isSend = FALSE;//send to server or not
        if(p_conn->d_session.d_session_status == WILDDOG_SESSION_AUTHED){
            isSend = TRUE;
            ++pkt->d_count;
            pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
        }
        ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_OFF, &command, isSend);
        wilddog_debug_level(WD_DEBUG_LOG, "Send removeObserver pkt 0x%x",(unsigned int)pkt->d_message_id);
    }
    //remove observer
    LL_FOREACH_SAFE(p_conn->d_conn_user.p_observer_list,curr,tmp){
        if(curr){
            if(TRUE == _wilddog_url_diff(curr->p_url, arg->p_url)){
                //diff
                LL_DELETE(p_conn->d_conn_user.p_observer_list,curr);
                _wilddog_conn_packet_deInit(curr);
                wfree(curr);
                break;
            }
        }
    }
    return ret;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_auth(void* data,int flag){
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    Wilddog_Conn_T *p_conn;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_Session_T *p_session;
    Wilddog_Conn_Pkt_T *pkt;
    
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;
    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    wilddog_assert(pkt, WILDDOG_ERR_NULL);
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, arg->p_url)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)_wilddog_conn_auth_callback;
    pkt->p_user_callback = arg->p_complete;
    pkt->p_user_arg = arg->p_completeArg;
    //add to auth queue
    if(p_conn->d_conn_sys.p_auth){
        _wilddog_conn_packet_deInit(p_conn->d_conn_sys.p_auth);
        wfree(p_conn->d_conn_sys.p_auth);
        p_conn->d_conn_sys.p_auth = NULL;
    }
    p_conn->d_conn_sys.p_auth = pkt;
    
    p_session = &p_conn->d_session;
    //memset(p_session, 0, sizeof(Wilddog_Session_T));
    p_session->d_session_status = WILDDOG_SESSION_AUTHING;

    //send to server
    command.p_data = NULL;
    command.d_data_len = 0;
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = (u8**)&pkt->p_data;
    command.p_out_data_len = NULL;
    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = NULL;
    command.d_session_len = 0;

    //get user auth token
    if(p_conn->p_conn_repo->p_rp_store->p_se_callback){
        command.d_data_len = (p_conn->p_conn_repo->p_rp_store->p_se_callback)(
                              p_conn->p_conn_repo->p_rp_store,
                              WILDDOG_STORE_CMD_GETAUTH,&command.p_data,0);
    }

    if(p_conn->p_protocol->callback){
        (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_SESSION_INIT, &command, 0);
        wilddog_debug_level(WD_DEBUG_LOG, "Send auth pkt 0x%x",(unsigned int)pkt->d_message_id);
    }
    ++pkt->d_count;
    pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
    return WILDDOG_ERR_NOERR;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_disSet(void* data,int flag){
    return _wilddog_conn_commonSet(data, flag,(Wilddog_Func_T)_wilddog_conn_disSet_callback,TRUE);
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_disPush(void* data,int flag){
    return _wilddog_conn_commonPush(data,flag,(Wilddog_Func_T)_wilddog_conn_disPush_callback,TRUE);
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_disRemove(void* data,int flag){
    return _wilddog_conn_commonRemove(data, flag, (Wilddog_Func_T)_wilddog_conn_disRemove_callback,TRUE);
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_disCancel(void* data,int flag){
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    Wilddog_Conn_T *p_conn;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_Conn_Pkt_T *pkt;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;
    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    if(p_conn->d_conn_user.d_count > WILDDOG_REQ_QUEUE_NUM){
        wilddog_debug_level(WD_DEBUG_WARN, "Too many requests! Max is %d",WILDDOG_REQ_QUEUE_NUM);
        return WILDDOG_ERR_QUEUEFULL;
    }

    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    wilddog_assert(pkt, WILDDOG_ERR_NULL);
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, arg->p_url)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)_wilddog_conn_disCancel_callback;
    pkt->p_user_callback = arg->p_complete;
    pkt->p_user_arg = arg->p_completeArg;

    //add to rest queue
    LL_APPEND(p_conn->d_conn_user.p_rest_list,pkt);
    p_conn->d_conn_user.d_count++;
    
    command.p_data = NULL;
    command.d_data_len = 0;
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = (u8**)&pkt->p_data;
    command.p_out_data_len = NULL;

    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = p_conn->d_session.short_sid;
    command.d_session_len = WILDDOG_CONN_SESSION_SHORT_LEN - 1;

    if(p_conn->p_protocol->callback){
        BOOL isSend = FALSE;//send to server or not
        if(p_conn->d_session.d_session_status == WILDDOG_SESSION_AUTHED){
            isSend = TRUE;
            ++pkt->d_count;
            pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
        }
        ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_DIS_CANCEL, &command, isSend);
        wilddog_debug_level(WD_DEBUG_LOG, "Send dis cancel pkt 0x%x",(unsigned int)pkt->d_message_id);
    }
        
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_offline(void* data,int flag){
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    Wilddog_Conn_T *p_conn;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_Conn_Pkt_T *pkt;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;
    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    if(p_conn->d_conn_user.d_count > WILDDOG_REQ_QUEUE_NUM){
        wilddog_debug_level(WD_DEBUG_WARN, "Too many requests! Max is %d",WILDDOG_REQ_QUEUE_NUM);
        return WILDDOG_ERR_QUEUEFULL;
    }

    pkt = (Wilddog_Conn_Pkt_T*)wmalloc(sizeof(Wilddog_Conn_Pkt_T));
    wilddog_assert(pkt, WILDDOG_ERR_NULL);
    if(WILDDOG_ERR_NOERR != _wilddog_conn_packet_init(pkt, arg->p_url)){
        wfree(pkt);
        wilddog_debug_level(WD_DEBUG_ERROR, "Connect layer packet init failed!");
        return WILDDOG_ERR_NULL;
    }
    pkt->p_complete = (Wilddog_Func_T)_wilddog_conn_offline_callback;
    pkt->p_user_callback = arg->p_complete;
    pkt->p_user_arg = arg->p_completeArg;

    //add to rest queue
    LL_APPEND(p_conn->d_conn_user.p_rest_list,pkt);
    p_conn->d_conn_user.d_count++;
    
    command.p_data = NULL;
    command.d_data_len = 0;
    command.p_message_id= &pkt->d_message_id;
    command.p_url = pkt->p_url;
    command.protocol = p_conn->p_protocol;
    command.p_out_data = (u8**)&pkt->p_data;
    command.p_out_data_len = NULL;

    //send pkt must need session info, exclude auth pkt.
    command.p_session_info = p_conn->d_session.short_sid;
    command.d_session_len = WILDDOG_CONN_SESSION_SHORT_LEN - 1;

    if(p_conn->p_protocol->callback){
        BOOL isSend = FALSE;//send to server or not
        if(p_conn->d_session.d_session_status == WILDDOG_SESSION_AUTHED){
            isSend = TRUE;
            ++pkt->d_count;
            pkt->d_next_send_time = _wilddog_conn_getNextSendTime(pkt->d_count);
        }
        ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_SEND_OFFLINE, &command, isSend);
        wilddog_debug_level(WD_DEBUG_LOG, "Send offline pkt 0x%x",(unsigned int)pkt->d_message_id);
    }
        
    return ret;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_online(void* data,int flag){
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    Wilddog_Conn_T *p_conn;
    
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;

    wilddog_assert(p_conn, WILDDOG_ERR_NULL);

    p_conn->d_session.d_session_status = WILDDOG_SESSION_NOTAUTHED;
    return _wilddog_conn_auth(data, flag);
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_trySync(void* data,int flag){
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Return_T error_code = WILDDOG_ERR_INVALID;
    Wilddog_Proto_Cmd_Arg_T command;
    Wilddog_ConnCmd_Arg_T *arg = (Wilddog_ConnCmd_Arg_T*)data;
    u32 message_id = 0;
    Wilddog_Conn_T *p_conn;
    u8* recvPkt = NULL, *payload = NULL;
    u32 recvPkt_len = 0, payload_len = 0;
    Wilddog_Conn_Pkt_T * sendPkt = NULL;
    
    wilddog_assert(data, WILDDOG_ERR_NULL);

    p_conn = arg->p_repo->p_rp_conn;

    wilddog_assert(p_conn, WILDDOG_ERR_NULL);
    
    command.protocol = p_conn->p_protocol;

    wilddog_assert(p_conn && command.protocol,WILDDOG_ERR_NULL);
    
    command.p_out_data = &recvPkt;
    command.p_out_data_len = &recvPkt_len;
    command.p_data = NULL;
    command.d_data_len = 0;
    command.p_url = arg->p_url;
    command.p_message_id = &message_id;
    //1. receive packet
    if(p_conn->p_protocol->callback){
        ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_RECV_GETPKT, &command, 0);
    }
    if(WILDDOG_ERR_NOERR != ret){
        if(WILDDOG_ERR_RECVTIMEOUT == ret){
            goto next;
        }
        else{
            wilddog_debug_level(WD_DEBUG_WARN,"Received error [%d]",ret);
            return ret;
        }
    }
    //We have received packet, reset timeout count
    p_conn->d_timeout_count = 0;
    
    command.p_data = recvPkt;
    command.d_data_len = recvPkt_len;
    //2. try to find the send pkt which has same message id
    sendPkt = _wilddog_conn_recv_sendPktFind(p_conn,message_id);
    if(NULL == sendPkt){
        //delete the recvPkt. Remember the recv pkt is in p_data.
        if(p_conn->p_protocol->callback){
            ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_RECV_FREEPKT, &command, FALSE);
        }
        wilddog_debug_level(WD_DEBUG_WARN, "Received an unmatched packet, mid = 0x%x!",(unsigned int)message_id);
        return ret;
    }

    //3. handle packet
    command.p_out_data = &payload;
    command.p_out_data_len = &payload_len;
    command.p_proto_data = &sendPkt->p_proto_data;
    if(p_conn->p_protocol->callback){
        error_code = (p_conn->p_protocol->callback)(WD_PROTO_CMD_RECV_HANDLEPKT, &command, 0);
    }

    //if sendPkt want to be freed, it must be freed in callback, because
    //we cannot operate the linklist which sendPkt belonged to.
    if(error_code >= WILDDOG_ERR_NOERR){
        //callback the p_complete
        if(sendPkt->p_complete){
            ret = (sendPkt->p_complete)(p_conn, sendPkt, payload, payload_len, error_code);
        }
    }
    sendPkt = NULL;
    
    //Free recvPkt.Remember the recv pkt is in p_data.
    if(p_conn->p_protocol->callback){
        ret = (p_conn->p_protocol->callback)(WD_PROTO_CMD_RECV_FREEPKT, &command, TRUE);
    }
    recvPkt = NULL;
    recvPkt_len = 0;
next:
    //4. retransmit or timeout logic
    _wilddog_conn_retransmitHandler(p_conn);
    //5. session maintain
    if(p_conn->d_session.d_session_status == WILDDOG_SESSION_NOTAUTHED){
        //retry
        _wilddog_conn_sessionRetry(p_conn);
    }
    //ping status
    _wilddog_conn_pingHandler(p_conn);
    return ret;
}
/* send interface */
Wilddog_Func_T _wilddog_conn_funcTable[WILDDOG_CONN_CMD_MAX + 1] = 
{
    (Wilddog_Func_T)_wilddog_conn_get,//get
    (Wilddog_Func_T)_wilddog_conn_set,//set
    (Wilddog_Func_T)_wilddog_conn_push,//push
    (Wilddog_Func_T)_wilddog_conn_remove,//remove
    (Wilddog_Func_T)_wilddog_conn_addObserver,//on
    (Wilddog_Func_T)_wilddog_conn_removeObserver,//off
    (Wilddog_Func_T)_wilddog_conn_auth,//auth
    (Wilddog_Func_T)_wilddog_conn_disSet,//ondisset
    (Wilddog_Func_T)_wilddog_conn_disPush,//ondispush
    (Wilddog_Func_T)_wilddog_conn_disRemove,//ondisremove
    (Wilddog_Func_T)_wilddog_conn_disCancel,//ondiscancel
    (Wilddog_Func_T)_wilddog_conn_offline,//offline
    (Wilddog_Func_T)_wilddog_conn_online,//online
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
    p_conn->d_conn_sys.d_curr_ping_interval = WILDDOG_DEFAULT_PING_INTERVAL;
    p_conn->d_conn_sys.d_ping_delta = WILDDOG_DEFAULT_PING_DELTA;
    p_conn->d_session.d_session_status = WILDDOG_SESSION_INIT;
    sprintf((char*)p_conn->d_session.short_sid, "00000000");
    sprintf((char*)p_conn->d_session.long_sid, "00000000000000000000000000000000");
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
    //Deinit pkts 
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
    //TODO: Deinit session.We don't need deinit, let it timeout.--jimmy
    
    //Deinit protocol layer.
    _wilddog_protocol_deInit(p_conn);
    wfree(p_conn);
    p_repo->p_rp_conn = NULL;

    return WILDDOG_ERR_NOERR;
}

