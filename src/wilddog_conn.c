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
 * 0.4.0        lxs       2015-05-15  Create file.
 * 0.5.0        lxs       2015-10-9   cut down some function.
 * 0.5.0        lxs       2015-12-2   one cmd one functions.
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

#include "wilddog_conn_coap.h"

//#include "wilddog_conn_manage.h"
#include "wilddog_port.h"
#include "wilddog_url_parser.h"
#include "wilddog_payload.h"
#include "wilddog_api.h"
#include "test_lib.h"

#if 0
/* call back  interface */
Wilddog_Func_T _wilddog_connCallBack_funcTable[WILDDOG_CONN_CMD_MAX + 1] = 
{
    (Wilddog_Func_T)_wilddog_conn_cb_get,
    (Wilddog_Func_T)_wilddog_conn_cb_set,
    (Wilddog_Func_T)_wilddog_conn_cb_push,
    (Wilddog_Func_T)_wilddog_conn_cb_remove,
    (Wilddog_Func_T)_wilddog_conn_cb_on,
    (Wilddog_Func_T)_wilddog_conn_cb_off,
    (Wilddog_Func_T)_wilddog_conn_cb_auth,    
    NULL
};

extern Wilddog_Return_T WD_SYSTEM _wilddog_node_setKey
    (
    Wilddog_Node_T *node, 
    Wilddog_Str_T *key
    );

/*
 * Function:    _wilddog_conn_cb_set
 * Description: conn layer set callback function .
 *   
 * Input:      p_cm_recv: recv packet.
 * Output:      N/A
 * Return:      N/A
*/
STATIC void WD_SYSTEM _wilddog_conn_cb_set
    (
    Wilddog_CM_Recv_T *p_cm_recv
    )
{
    if( p_cm_recv->f_user_callback )
        p_cm_recv->f_user_callback(p_cm_recv->p_user_cb_arg,p_cm_recv->err);
}

/*
 * Function:    _wilddog_conn_cb_push.
 * Description: conn layer push callback function.
 *   
 * Input:       p_cm_recv: recv packet.
 * Output:      N/A.
 * Return:      N/A.
*/
STATIC void WD_SYSTEM _wilddog_conn_cb_push
    (
    Wilddog_CM_Recv_T *p_cm_recv
    )
{
    if( p_cm_recv->f_user_callback )
       p_cm_recv->f_user_callback(p_cm_recv->p_recvData,\
            p_cm_recv->p_user_cb_arg,p_cm_recv->err);
}

/*
 * Function:    _wilddog_conn_cb_get
 * Description: serialize cbor to node type and call user call backet.
 *   
 * Input: p_cm_recv: recv packet.
 * Output:      N/A
 * Return:      N/A
*/
STATIC void WD_SYSTEM _wilddog_conn_cb_get
    (
    Wilddog_CM_Recv_T *p_cm_recv
    )
{
    Wilddog_Node_T* p_snapshot = NULL;
    Wilddog_Str_T *p_path = NULL;
    
    if( p_cm_recv->p_recvData && \
        p_cm_recv->p_recvData->d_dt_len > 0 )
    {
#ifdef WILDDOG_SELFTEST
        ramtest_skipLastmalloc();
#endif

        p_snapshot = _wilddog_payload2Node(p_cm_recv->p_recvData );
        p_path = _wilddog_url_getKey(p_cm_recv->p_url_path);
        
        if( p_path )
            _wilddog_node_setKey(p_snapshot, p_path);
        
#ifdef WILDDOG_SELFTEST        
        ramtest_caculate_nodeRam();
#endif

    }


#ifdef WILDDOG_SELFTEST                        
    ramtest_caculate_peakRam();
#endif
    
    if(p_cm_recv->f_user_callback)
        p_cm_recv->f_user_callback(p_snapshot,p_cm_recv->p_user_cb_arg,p_cm_recv->err);

    if(p_snapshot)
        wilddog_node_delete(p_snapshot);
    
    if(p_path)
        wfree(p_path);
    
    return;
}
    
/*
 * Function:    _wilddog_conn_cb
 * Description:  Call a different callback function acdording to the cmd.
 *   
 * Input: p_cm_recv: recv packet.
 * Output:      N/A
 * Return:      N/A
*/
int WD_SYSTEM _wilddog_conn_cb
    (
    Wilddog_CM_Recv_T *p_cm_recv
    )
{
    
    wilddog_debug_level( WD_DEBUG_WARN,"conn CB ERROR=%lu \n",p_cm_recv->err);
    
    switch( p_cm_recv->cmd)
    {    
        
        case WILDDOG_CONN_CMD_PUSH:
            _wilddog_conn_cb_push(p_cm_recv);
            break;
            
        case WILDDOG_CONN_CMD_AUTH:    
        case WILDDOG_CONN_CMD_SET:
        case WILDDOG_CONN_CMD_REMOVE:
            _wilddog_conn_cb_set(p_cm_recv);
            break;
        
        case WILDDOG_CONN_CMD_GET:
        case WILDDOG_CONN_CMD_ON:
            _wilddog_conn_cb_get(p_cm_recv);
            break;
        case WILDDOG_CONN_CMD_OFF:
            break;
        default:
            break;
        
    }
    
    return 0;
} 
/*
 * Function:    _wilddog_conn_creatSendPayload
 * Description: conn layer build send payload
 *   
 * Input:     p_conn: the pointer of the conn struct
 *              p_nodeData: the pointer of node data
 *              p_sendArg: the pointer of the conn send packet
 * Output:      N/A
 * Return:      N/A
*/
STATIC int WD_SYSTEM _wilddog_conn_buildSendPayload
    (  
    Wilddog_Conn_Cmd_T cmd,
    Wilddog_Repo_T *p_repo,
    Wilddog_Node_T *p_nodeData,
    Wilddog_CM_Send_T *p_sendArg
    )
{
    u8 *p_buf = NULL;
    Wilddog_Payload_T *p_payload = NULL;

    if(cmd == WILDDOG_CONN_CMD_AUTH)
    {
        p_sendArg->d_payloadlen = p_repo->p_rp_store->p_se_callback(    \
                         p_repo->p_rp_store, WILDDOG_STORE_CMD_GETAUTH, \
                         p_buf,0);
       p_sendArg->p_payload = wmalloc(p_sendArg->d_payloadlen);
       if(p_sendArg->p_payload && p_sendArg->d_payloadlen)
            memcpy(p_sendArg,p_buf,p_sendArg->d_payloadlen);
       return WILDDOG_ERR_NOERR;
    }
    else
    {
        p_payload = _wilddog_node2Payload(p_nodeData);
        if( p_payload )
        {
           p_sendArg->d_payloadlen = p_payload->d_dt_len;
           p_sendArg->p_payload = p_payload->p_dt_data;
           wfree(p_payload);
           return WILDDOG_ERR_NOERR;
        }
        else
           return WILDDOG_ERR_NULL;
     }
}
/*
 * Function:    _wilddog_conn_send
 * Description: conn layer send function.
 *   
 * Input:     cmd: the conn command
 *              p_repo: the pointer of the repo struct
 *              p_arg:  the pointer of the conn command arg
 * Output:      N/A
 * Return:      if success, return WILDDOG_ERR_NOERR, else return 
 *              WILDDOG_ERR_INVALID
*/
STATIC int WD_SYSTEM _wilddog_conn_send
    (
    Wilddog_Conn_Cmd_T cmd,
    Wilddog_Repo_T *p_repo,
    Wilddog_ConnCmd_Arg_T *p_arg
    )
{
    int res = 0;
    Wilddog_CM_Send_T d_sendArg;
    if(p_arg == NULL || p_repo == NULL)
        return WILDDOG_ERR_INVALID;

    memset(&d_sendArg,0,sizeof(Wilddog_CM_Send_T));
    
    if( p_arg->p_data &&
        (res =_wilddog_conn_buildSendPayload(cmd,p_repo,p_arg->p_data,&d_sendArg)) \
            != WILDDOG_ERR_NOERR )
        return res;

    d_sendArg.cmd = cmd;
    d_sendArg.p_url = p_arg->p_url;
    d_sendArg.p_user_arg = p_arg->p_completeArg;
    d_sendArg.f_user_callback = p_arg->p_complete;
    d_sendArg.p_cm_hd = p_repo->p_rp_conn->p_cm_hd;

    res = _wilddog_cm_send(&d_sendArg);
    wfree(d_sendArg.p_payload);
    if(res > 0 )
        res = WILDDOG_ERR_NOERR;
    
    return res;
}
#endif
STATIC int _wilddog_conn_get(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
   return WILDDOG_ERR_NOERR;
}
STATIC int _wilddog_conn_set(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
  
    /*todo add list. */

    /* todo send to */   
    return WILDDOG_ERR_NOERR;
}
STATIC int _wilddog_conn_push(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}
STATIC int _wilddog_conn_remove(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}
STATIC int _wilddog_conn_on(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}
STATIC int _wilddog_conn_off(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}
STATIC int _wilddog_conn_auth(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}

STATIC int _wilddog_conn_onDisSet(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}
STATIC int _wilddog_conn_onDisPush(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}

STATIC int _wilddog_conn_onDisRemove(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}

STATIC int _wilddog_conn_cancelDis(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}

STATIC int _wilddog_conn_offLine(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}
STATIC int _wilddog_conn_onLine(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_conn_trySync
 * Description: conn layer  try sync function
 *   
 * Input:       p_repo: the pointer of the repo struct
 * Output:      N/A
 * Return:      the result
*/
STATIC int _wilddog_conn_trysync(Wilddog_Conn_T *p_conn,int flags)
{
    return WILDDOG_ERR_NOERR;
}

/* send interface */
Wilddog_Func_T _wilddog_conn_funcTable[WILDDOG_CONN_CMD_MAX + 1] = 
{
    (Wilddog_Func_T)_wilddog_conn_get,
    (Wilddog_Func_T)_wilddog_conn_set,
    (Wilddog_Func_T)_wilddog_conn_push,
    (Wilddog_Func_T)_wilddog_conn_remove,
    (Wilddog_Func_T)_wilddog_conn_on,
    (Wilddog_Func_T)_wilddog_conn_off,
    (Wilddog_Func_T)_wilddog_conn_auth,
    
    (Wilddog_Func_T)_wilddog_conn_onDisSet,
    (Wilddog_Func_T)_wilddog_conn_onDisPush,
    (Wilddog_Func_T)_wilddog_conn_onDisRemove,
    
    (Wilddog_Func_T)_wilddog_conn_cancelDis,
    (Wilddog_Func_T)_wilddog_conn_offLine,
    (Wilddog_Func_T)_wilddog_conn_onLine,
    (Wilddog_Func_T)_wilddog_conn_trysync,

    NULL
};

STATIC int WD_SYSTEM _wilddog_conn_ioctl
    (
    Wilddog_Conn_Cmd_T cmd,
    Wilddog_ConnCmd_Arg_T *p_args,
    int flag
    )
{
    if( cmd >= WILDDOG_CONN_CMD_MAX ||
        cmd < 0)
        return WILDDOG_ERR_INVALID;

    return (_wilddog_conn_funcTable[cmd])(p_args,flag);
}

/*
 * Function:    _wilddog_conn_init
 * Description: creat session and register send and trysync function.
 *   
 * Input:       p_repo: the pointer of the repo struct
 * Output:      N/A
 * Return:      the result
*/
Wilddog_Conn_T * WD_SYSTEM _wilddog_conn_init(Wilddog_Repo_T* p_repo)
{
    if(!p_repo)
        return NULL;
    Wilddog_Conn_T* p_repo_conn = (Wilddog_Conn_T*)wmalloc(sizeof(Wilddog_Conn_T));
    if(!p_repo_conn)
        return NULL;

    p_repo_conn->p_conn_repo = p_repo;
    p_repo->p_rp_conn = p_repo_conn;
    p_repo_conn->f_conn_ioctl = _wilddog_conn_ioctl;
    
    /*todo init cm :: dns or creat node.*/
    //_wilddog_cm_init(p_repo_conn->p_cm_hd,p_repo_conn->p_conn_repo->p_rp_url,
    //        WILDDOG_PORT,_wilddog_conn_cb);
    
    return p_repo_conn;
}

/*
 * Function:    _wilddog_conn_deinit
 * Description: conn layer  deinit function
 *   
 * Input:       p_repo: the pointer of the repo struct
 * Output:      N/A
 * Return:      the pointer of the conn struct
*/
Wilddog_Conn_T* WD_SYSTEM _wilddog_conn_deinit(Wilddog_Repo_T* p_repo)
{
    if( !p_repo || !p_repo->p_rp_conn )
        return NULL;
    /* todo destory system ping node */
    /* todo destory list that belong to that repo*/
    /* todo de init cm */    
    //_wilddog_cm_deInit(p_repo->p_rp_conn->p_cm_hd);
    
    wfree( p_repo->p_rp_conn);
    p_repo->p_rp_conn = NULL;
    
    return NULL;
}


