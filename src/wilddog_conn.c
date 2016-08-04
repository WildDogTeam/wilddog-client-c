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

#include "wilddog_conn_manage.h"
#include "wilddog_port.h"
#include "wilddog_url_parser.h"
#include "wilddog_payload.h"
#include "wilddog_api.h"
#include "test_lib.h"

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
    Wilddog_CM_Recv_T *p_cm_recv,
    int flag
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
    Wilddog_CM_Recv_T *p_cm_recv,
    int flag
    )
{
    if( p_cm_recv->f_user_callback )
    {
        p_cm_recv->f_user_callback(p_cm_recv->p_recvData,\
                                   p_cm_recv->p_user_cb_arg, \
                                   p_cm_recv->err);
    }
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
    Wilddog_CM_Recv_T *p_cm_recv,
    int flag
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
    {
        p_cm_recv->f_user_callback(p_snapshot,
                                   p_cm_recv->p_user_cb_arg,
                                   p_cm_recv->err);
    }

    if(p_snapshot)
        wilddog_node_delete(p_snapshot);
    
    return;
}
/*
 * Function:    _wilddog_conn_addUrl.
 * Description: add host and path option.
 *   
 * Input:   p_url: url option.
 *             p_pkg: protocol package.
 * Output:      N/A
 * Return:      N/A
*/ 
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_addUrl
    (
    Wilddog_Url_T * p_url,
    void *p_pkg
    )
{
    Protocol_Arg_Option_T pkg_option;
    
    if( p_url== NULL ||
        p_pkg == NULL)
        return WILDDOG_ERR_INVALID;

    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));
    /* add host.*/
    pkg_option.p_pkg = p_pkg;
    pkg_option.p_options = p_url->p_url_host;
    if( _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_HOST,&pkg_option,0) != \
        WILDDOG_ERR_NOERR )
    {
        return WILDDOG_ERR_NULL;
    }
    
    /* add path.*/
    pkg_option.p_pkg = p_pkg;
    pkg_option.p_options = p_url->p_url_path;
    if( _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_PATH,&pkg_option,0) != \
        WILDDOG_ERR_NOERR )
    {
        return WILDDOG_ERR_NULL;
    }
   
    return  WILDDOG_ERR_NOERR; 
}
/*
 * Function:    _wilddog_conn_getCborPayload.
 * Description: node transition to cbor payload.
 * Input:   p_node: node data.
 *             p_pkg: protocol package.
 * Output:      N/A
 * Return:      N/A
*/ 
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_conn_getCborPayload
    (
    Protocol_Arg_Payload_T *p_cbor_hd,
    Wilddog_Node_T *p_snode
    )
{
    Wilddog_Payload_T *p_nodeData = NULL;

    wilddog_assert(p_cbor_hd,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_snode,WILDDOG_ERR_INVALID);

    p_nodeData = _wilddog_node2Payload(p_snode);

    if( p_nodeData )
    {
        p_cbor_hd->p_payload = p_nodeData->p_dt_data;
        p_cbor_hd->d_payloadLen = p_nodeData->d_dt_len;
        wfree(p_nodeData);

        return WILDDOG_ERR_NOERR;
    }
    else
        return WILDDOG_ERR_NULL;
}

STATIC int WD_SYSTEM _wilddog_conn_send
    ( 
    Wilddog_Conn_Cmd_T cmd,
    void *p_pkg,
    u32 token,
    Wilddog_ConnCmd_Arg_T *p_arg
    )
{
    Wilddog_CM_UserArg_T sendArg;

    memset(&sendArg,0,sizeof(Wilddog_CM_UserArg_T));
    /* todo send to */
    sendArg.cmd = cmd;
    sendArg.p_cm_l =  p_arg->p_repo->p_rp_conn->p_cm_l;
    sendArg.p_pkg = (void*)p_pkg;
    sendArg.f_userCB = p_arg->p_complete;
    sendArg.f_userCB_arg = p_arg->p_completeArg;
    sendArg.d_token = token;
    return _wilddog_cm_ioctl( CM_CMD_USERSEND,&sendArg,0);

}
/* count protocol sizeof*/
STATIC int WD_SYSTEM _wilddog_conn_countPakgeSize
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    u32 s_payloadLen,
    u32 s_externLen
    )
{
    Protocol_Arg_CountSize_T pkg_sizeCount;
    
    memset(&pkg_sizeCount,0,sizeof(Protocol_Arg_CountSize_T));

    /*count package size.*/
    if(p_arg->p_url)
    {
        pkg_sizeCount.p_host = p_arg->p_url->p_url_host;
        pkg_sizeCount.p_path = p_arg->p_url->p_url_path;
    }
    pkg_sizeCount.p_query = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN,
                                           p_arg->p_repo->p_rp_conn->p_cm_l,0); 
    
    pkg_sizeCount.d_extendLen = s_externLen;
    pkg_sizeCount.d_payloadLen = s_payloadLen;
    
    return (int)_wilddog_protocol_ioctl(_PROTOCOL_CMD_COUNTSIZE,&pkg_sizeCount,0);
}
STATIC int WD_SYSTEM _wilddog_conn_get
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    void* p_pkg_index = NULL;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;

    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);
    
    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));

    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_GET;

    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
    
    pkg_arg.d_token = (u32)_wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

    /*count pakage size*/
    pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg,0,0);
    /* creat coap package.*/
    p_pkg_index = (void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

    /* add host.*/
    /* add path.*/
    if(_wilddog_conn_addUrl(p_arg->p_url,p_pkg_index) != WILDDOG_ERR_NOERR)
        goto _CONN_GET_ERR;
    /* add query - auth.*/
    pkg_option.p_pkg = p_pkg_index;
    pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN,
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_GET_ERR;
   
    /* send to */ 
    res = _wilddog_conn_send(WILDDOG_CONN_CMD_GET,
                             p_pkg_index,
                             pkg_arg.d_token,
                             p_arg);
    if(res < 0)
        goto _CONN_GET_ERR;
   
    return res;

_CONN_GET_ERR:

    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,p_pkg_index,0);
    return res;
}

STATIC int WD_SYSTEM _wilddog_conn_set(Wilddog_ConnCmd_Arg_T *p_arg,int flags)
{
    void* p_pkg_index = 0;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;
    Protocol_Arg_Payload_T pkg_cborPayload;
        
    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_data,WILDDOG_ERR_INVALID);

    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));        
    memset(&pkg_cborPayload,0,sizeof(Protocol_Arg_Payload_T));
    
    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_SET;

    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
    pkg_arg.d_token = _wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                        p_arg->p_repo->p_rp_conn->p_cm_l, \
                                        0);

    /*get payload len.*/
    if(  _wilddog_conn_getCborPayload(&pkg_cborPayload,p_arg->p_data) < 0 )
        return WILDDOG_ERR_INVALID;

    /*get package size.*/
    pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg, \
                                                 pkg_cborPayload.d_payloadLen, \
                                                 0);

    /* creat coap package.*/
    p_pkg_index = (void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

    /* add host.*/
    /* add path.*/
    if(_wilddog_conn_addUrl(p_arg->p_url,p_pkg_index) != WILDDOG_ERR_NOERR)
        goto _CONN_SET_ERR;
    /* add query - token.*/
    pkg_option.p_pkg = p_pkg_index;
    pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN, \
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if(res != WILDDOG_ERR_NOERR)
        goto _CONN_SET_ERR;
   
    /*add payload*/
    pkg_cborPayload.p_pkg = p_pkg_index;

    res = (int)_wilddog_protocol_ioctl(_PROTOCOL_CMD_ADD_DATA, \
                                       &pkg_cborPayload, \
                                       0);
    if( res < 0)
        goto _CONN_SET_ERR;

   
    /* todo send to */ 
    res = (int)_wilddog_conn_send(pkg_arg.cmd, \
                                 (void*)p_pkg_index, \
                                 pkg_arg.d_token, \
                                 p_arg);
    if(res < 0)
        goto _CONN_SET_ERR;
   
    wfree(pkg_cborPayload.p_payload);
   
    return res;
    
_CONN_SET_ERR:
    
    wfree(pkg_cborPayload.p_payload);
    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return res;

}

STATIC int WD_SYSTEM _wilddog_conn_push
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    void* p_pkg_index = 0;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;
    Protocol_Arg_Payload_T pkg_cborPayload;

    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_data,WILDDOG_ERR_INVALID);

    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));    
    memset(&pkg_cborPayload,0,sizeof(Protocol_Arg_Payload_T));
    
    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_PUSH;
 
    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
    
    pkg_arg.d_token = _wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                        p_arg->p_repo->p_rp_conn->p_cm_l, \
                                        0);
    
    /*get payload len.*/
    if(_wilddog_conn_getCborPayload(&pkg_cborPayload,p_arg->p_data) < 0 )
        return WILDDOG_ERR_INVALID;
    /*get package size.*/
    pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg, \
                                                 pkg_cborPayload.d_payloadLen, \
                                                 0);
    
    /* creat coap package.*/
    p_pkg_index =(void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == 0)
         return WILDDOG_ERR_NULL;
 
    /* add host.*/
    /* add path.*/
    if(_wilddog_conn_addUrl(p_arg->p_url,(void*)p_pkg_index) != WILDDOG_ERR_NOERR)
         goto _CONN_PUSH_ERR;
    /* add query - auth.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN,
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
         goto _CONN_PUSH_ERR;
    /*add payload*/
    pkg_cborPayload.p_pkg = p_pkg_index;

    res = (int)_wilddog_protocol_ioctl(_PROTOCOL_CMD_ADD_DATA, \
                                       &pkg_cborPayload, \
                                       0);
    
    if( res < 0)
         goto _CONN_PUSH_ERR;
    

    /* send to */ 
    res = (int)_wilddog_conn_send(pkg_arg.cmd, \
                                  (void*)p_pkg_index, \
                                  pkg_arg.d_token, \
                                  p_arg);
    
    if( res < 0)
         goto _CONN_PUSH_ERR;
	
    wfree(pkg_cborPayload.p_payload);

    return res;
     
_CONN_PUSH_ERR:
    
    wfree(pkg_cborPayload.p_payload);
    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return res;
}
STATIC int WD_SYSTEM _wilddog_conn_remove
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    void* p_pkg_index = 0;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;

    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);
    
    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));    
    
    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_REMOVE;
    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
    
    pkg_arg.d_token = (u32)_wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

    /*count package size.*/
    pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg,0,0);

    /* creat coap package.*/
    p_pkg_index = (void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

    /* add host.*/
    /* add path.*/
    if(_wilddog_conn_addUrl(p_arg->p_url,(void*)p_pkg_index) != WILDDOG_ERR_NOERR)
        goto _CONN_REMOVE_ERR;
    /* add query - token.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN, \
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_REMOVE_ERR;
    /* send to */ 
    res = (int)_wilddog_conn_send(pkg_arg.cmd, \
                                  (void*)p_pkg_index, \
                                  pkg_arg.d_token, \
                                  p_arg);
    
    if( res < 0 )
        goto _CONN_REMOVE_ERR;

    return res;

_CONN_REMOVE_ERR:

    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return res;

}

STATIC int WD_SYSTEM _wilddog_conn_on
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    void* p_pkg_index = 0;
    int res = 0;
    u8 observerValue = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;
    Wilddog_CM_UserArg_T sendArg;
	Wilddog_CM_FindNode_Arg_T nodeFind_arg;

    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);

	/*if the path have been  observer aleady,Ignore the request*/
	if( p_arg->p_repo->p_rp_conn->p_cm_l &&
		p_arg->p_repo->p_rp_conn->p_cm_l->p_cm_n_hd 
		)
	{
		nodeFind_arg.path = p_arg->p_url->p_url_path;
		nodeFind_arg.p_node_hd = p_arg->p_repo->p_rp_conn->p_cm_l->p_cm_n_hd;
		if(_wilddog_cm_ioctl(CM_CMD_OBSERVER_ALEADY,&nodeFind_arg,0))
			return WILDDOG_ERR_NOERR;
	}
    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));
    memset(&sendArg,0,sizeof(Wilddog_CM_UserArg_T));    
    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_ON;

    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
    pkg_arg.d_token = _wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                        p_arg->p_repo->p_rp_conn->p_cm_l, \
                                        0);

    /*count package size.*/
    pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg,0,0);

    /* creat coap package.*/
    p_pkg_index =(void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

    /* add host.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = p_arg->p_url->p_url_host;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_HOST,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_ON_ERR;
    /* add query - observer.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)&observerValue;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_OBSERVER,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_ON_ERR;
    /* add path.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = p_arg->p_url->p_url_path;
    
    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_PATH,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_ON_ERR;

    /* add token.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN, \
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_ON_ERR;

    /* todo send to */
    sendArg.cmd = WILDDOG_CONN_CMD_ON;
    sendArg.d_token = pkg_arg.d_token;
    sendArg.p_cm_l =  p_arg->p_repo->p_rp_conn->p_cm_l;
    sendArg.p_pkg = (void*)p_pkg_index;
    sendArg.f_userCB = p_arg->p_complete;
    sendArg.f_userCB_arg = p_arg->p_completeArg;
    sendArg.p_path = p_arg->p_url->p_url_path;
    
    if( (res = _wilddog_cm_ioctl( CM_CMD_USERSEND,&sendArg,0))< 0)
        goto _CONN_ON_ERR;

    return res;

_CONN_ON_ERR:

    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return res;
    
}
STATIC int WD_SYSTEM _wilddog_conn_off
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    int res = 0;
    void* p_pkg_index = 0;
    u8 observerValue = 1;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;
    Wilddog_CM_OffArg_T deleNodeArg;

    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);

    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));

    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_OFF;
    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
    pkg_arg.d_token = _wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                        p_arg->p_repo->p_rp_conn->p_cm_l, \
                                        0);

    /*count package size.*/
    pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg,0,0);

    /* creat coap package.*/
    p_pkg_index = (void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

    /* add host.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = p_arg->p_url->p_url_host;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_HOST,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
       goto _CONN_OFF_ERR;
    /* add query - observer.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)&observerValue;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_OBSERVER,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
       goto _CONN_OFF_ERR;
    /* add path.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = p_arg->p_url->p_url_path;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_PATH,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_OFF_ERR;

    /* add query - token.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN, \
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_OFF_ERR;

    /*delete on node */
    deleNodeArg.p_cm_l = p_arg->p_repo->p_rp_conn->p_cm_l;
    deleNodeArg.p_path = p_arg->p_url->p_url_path;

    res = _wilddog_cm_ioctl( CM_CMD_DELENODE_BYPATH,&deleNodeArg,0);
    if( res < 0 )
        goto _CONN_OFF_ERR;
    /* todo send to */ 

    res = (int)_wilddog_conn_send(pkg_arg.cmd, \
                                  (void*)p_pkg_index,pkg_arg.d_token, \
                                  p_arg);
    if( res < 0 )
        goto _CONN_OFF_ERR;

    return res;
    
_CONN_OFF_ERR:

    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return res;
    
}

STATIC int WD_SYSTEM _wilddog_conn_auth
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    void* p_pkg_index = 0;
    int res = 0;
    u8 *p_buf = NULL;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;
    Protocol_Arg_Payload_T authData;

    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);

    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));
    memset(&authData,0,sizeof(Protocol_Arg_Payload_T));
   /* init protocol package.*/
   pkg_arg.cmd = WILDDOG_CONN_CMD_AUTH;

   /* get messageid */
   pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
   pkg_arg.d_token = _wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                       p_arg->p_repo->p_rp_conn->p_cm_l,
                                       0);

   /* get auth data and payload size*/
   authData.d_payloadLen = p_arg->p_repo->p_rp_store->p_se_callback(
                                p_arg->p_repo->p_rp_store,\
                                WILDDOG_STORE_CMD_GETAUTH, \
                                &p_buf, \
                                0);
   
   authData.p_payload = p_buf;
   /* count pakge size.*/
   pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg, \
                                                        authData.d_payloadLen, \
                                                        0);
   /* creat coap package.*/
   p_pkg_index = (void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
   if( p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

   /* add host.*/
   pkg_option.p_pkg = (void*)p_pkg_index;
   pkg_option.p_options = p_arg->p_url->p_url_host;

   res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_HOST,&pkg_option,0);
   if( res != WILDDOG_ERR_NOERR )
       goto _CONN_AUTH_ERR;
   
   /* add path.*/
   pkg_option.p_pkg = (void*)p_pkg_index;
   pkg_option.p_options = _CM_AUTHR_PATH;

   res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_PATH,&pkg_option,0);
   if( res != WILDDOG_ERR_NOERR )
       goto _CONN_AUTH_ERR;

   /*add payload.*/
   authData.p_pkg = (void*)p_pkg_index; 

   res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_DATA,&authData,0);
   if(res < 0)
        goto _CONN_AUTH_ERR;
   /*20160711 skyli : delete all auth node to guarantee user auth send alone*/
   _wilddog_cm_ioctl( CM_CMD_AUTH_DELETE,p_arg->p_repo->p_rp_conn->p_cm_l,0);
   /* send to */ 
   res = (int)_wilddog_conn_send(pkg_arg.cmd, \
                                 (void*)p_pkg_index, \
                                 pkg_arg.d_token, \
                                 p_arg);
   if( res < 0)
        goto _CONN_AUTH_ERR;
   
   return res;
    
_CONN_AUTH_ERR:
    
   _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
   return res;
   
}

STATIC int WD_SYSTEM _wilddog_conn_onDisSet
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    void* p_pkg_index = 0;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;
    Protocol_Arg_Payload_T pkg_cborPayload;
    
    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);

    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));
    memset(&pkg_cborPayload,0,sizeof(Protocol_Arg_Payload_T));
   /* init protocol package.*/
   pkg_arg.cmd = WILDDOG_CONN_CMD_ONDISSET;

   /* get messageid */
   pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
   pkg_arg.d_token = _wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                       p_arg->p_repo->p_rp_conn->p_cm_l, \
                                       0);

   /*get payload len.*/
   if( _wilddog_conn_getCborPayload(&pkg_cborPayload,p_arg->p_data) < 0 )
        return WILDDOG_ERR_INVALID;

   /*count package size.*/
   pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg,
                                  pkg_cborPayload.d_payloadLen,
                                  (strlen(_CM_ONDIS)+PROTOCOL_QUERY_HEADLEN));

   /* creat coap package.*/
   p_pkg_index = (void*) _wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
   if( p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

   /* add host.*/
   /* add path.*/
   res = _wilddog_conn_addUrl(p_arg->p_url,(void*)p_pkg_index);
   if( res != WILDDOG_ERR_NOERR)
        goto _CONN_DISSET_ERR;
   /* add query - token.*/
   pkg_option.p_pkg = (void*)p_pkg_index;
   pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN, \
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

   res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
   if( res != WILDDOG_ERR_NOERR )
        goto _CONN_DISSET_ERR;
   /* add query -.dis.*/
   pkg_option.p_pkg = (void*)p_pkg_index;
   pkg_option.p_options = (void*)_CM_ONDIS;

   res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
   if( res != WILDDOG_ERR_NOERR )
        goto _CONN_DISSET_ERR;

   /*add payload*/
   pkg_cborPayload.p_pkg = p_pkg_index;

   res =(int)_wilddog_protocol_ioctl(_PROTOCOL_CMD_ADD_DATA,&pkg_cborPayload,0);
   if( res < 0)
        goto _CONN_DISSET_ERR;
    

   /* send to */ 
   res = (int)_wilddog_conn_send(pkg_arg.cmd, \
                                 (void*)p_pkg_index, \
                                 pkg_arg.d_token, \
                                 p_arg);
   if( res < 0)
        goto _CONN_DISSET_ERR;
   
   wfree(pkg_cborPayload.p_payload);
   return res;
    
_CONN_DISSET_ERR:
    
   wfree(pkg_cborPayload.p_payload);
   _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
   return res;   
}
STATIC int WD_SYSTEM _wilddog_conn_onDisPush
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    void* p_pkg_index = 0;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;
    Protocol_Arg_Payload_T pkg_cborPayload;

    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);

    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));
    memset(&pkg_cborPayload,0,sizeof(Protocol_Arg_Payload_T));
    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_ONDISPUSH;

    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
    pkg_arg.d_token = _wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                        p_arg->p_repo->p_rp_conn->p_cm_l, \
                                        0);

    /*get payload len.*/
    if(  _wilddog_conn_getCborPayload(&pkg_cborPayload,p_arg->p_data) < 0 )
         return WILDDOG_ERR_INVALID;
    
    /*count package size.*/
    pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg,
                                  pkg_cborPayload.d_payloadLen,
                                  (strlen(_CM_ONDIS)+PROTOCOL_QUERY_HEADLEN));

    /* creat coap package.*/
    p_pkg_index = (void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

    /* add host.*/
    /* add path.*/

    res = _wilddog_conn_addUrl(p_arg->p_url,(void*)p_pkg_index);
    if( res != WILDDOG_ERR_NOERR)
        goto _CONN_DISPUSH_ERR;
    /* add query - token.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN, \
                                            p_arg->p_repo->p_rp_conn->p_cm_l, \
                                            0);

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_DISPUSH_ERR;
    /* add query -.dis.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_CM_ONDIS;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_DISPUSH_ERR;
    
    /*add payload*/
    pkg_cborPayload.p_pkg = p_pkg_index;

    res=(int)_wilddog_protocol_ioctl(_PROTOCOL_CMD_ADD_DATA,&pkg_cborPayload,0);
    if( res < 0)
         goto _CONN_DISPUSH_ERR;
     

    /* send to */ 
    res = (int)_wilddog_conn_send(pkg_arg.cmd, \
                                  (void*)p_pkg_index, \
                                  pkg_arg.d_token, \
                                  p_arg);
    if( res < 0)
        goto _CONN_DISPUSH_ERR;

    wfree(pkg_cborPayload.p_payload);
    return res;

_CONN_DISPUSH_ERR:
    
    wfree(pkg_cborPayload.p_payload);
    
    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return res;

}

STATIC int WD_SYSTEM _wilddog_conn_onDisRemove
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    void* p_pkg_index = 0;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;

    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);

    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));    
    
    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_ONDISREMOVE;

    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
    pkg_arg.d_token = _wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                        p_arg->p_repo->p_rp_conn->p_cm_l, \
                                        0);

    /*count package size.*/
    pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg, \
                                 0, \
                                 (strlen(_CM_ONDIS)+PROTOCOL_QUERY_HEADLEN));
    /* creat coap package.*/
    p_pkg_index =(void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

    /* add host.*/
    /* add path.*/
    res = _wilddog_conn_addUrl(p_arg->p_url,(void*)p_pkg_index);
    if( res != WILDDOG_ERR_NOERR)
        goto _CONN_DISREMOVE_ERR;
    /* add query - token.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN,
        p_arg->p_repo->p_rp_conn->p_cm_l,0);
    
    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_DISREMOVE_ERR;
    /* add query -.dis.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_CM_ONDIS;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_DISREMOVE_ERR;

    /* send to */ 
    res = (int)_wilddog_conn_send(pkg_arg.cmd, \
                                  (void*)p_pkg_index, \
                                  pkg_arg.d_token, \
                                  p_arg);
    if( res < 0 )
        goto _CONN_DISREMOVE_ERR;

    return res;

_CONN_DISREMOVE_ERR:

    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return res;
   
}

STATIC int WD_SYSTEM _wilddog_conn_cancelDis
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    void* p_pkg_index = 0;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;

    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    
    wilddog_assert(p_arg->p_url,WILDDOG_ERR_INVALID);

    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));    
        
    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_CANCELDIS;

    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
    pkg_arg.d_token = _wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                        p_arg->p_repo->p_rp_conn->p_cm_l, \
                                        0);

     
    /*count package size.*/
    pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg, \
                               0, \
                               (strlen(_CM_DISCANCEL)+PROTOCOL_QUERY_HEADLEN));
    /* creat coap package.*/
    p_pkg_index =(void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if(p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

    /* add host.*/
    /* add path.*/
    res = _wilddog_conn_addUrl(p_arg->p_url,(void*)p_pkg_index);
    if( res != WILDDOG_ERR_NOERR)
        goto _CONN_CANCELDIS_ERR;
    /* add query - token.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN,\
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_CANCELDIS_ERR;
    /* add query -.dis.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_CM_DISCANCEL;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_CANCELDIS_ERR;

    /* send to */ 
    res = (int)_wilddog_conn_send(pkg_arg.cmd, \
                                  (void*)p_pkg_index, \
                                  pkg_arg.d_token, \
                                  p_arg);
    if( res < 0)
        goto _CONN_CANCELDIS_ERR;

    return res;

_CONN_CANCELDIS_ERR:

    _wilddog_protocol_ioctl( _PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return res;
    
}

STATIC int WD_SYSTEM _wilddog_conn_offLine
    ( 
    Wilddog_ConnCmd_Arg_T *p_arg, \
    int flags
    )
{
    void* p_pkg_index = 0;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;

    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    

    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));
    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_OFFLINE;

    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_ioctl(CM_CMD_GET_INDEX,NULL,0);
    pkg_arg.d_token = _wilddog_cm_ioctl(CM_CMD_GET_TOKEN, \
                                        p_arg->p_repo->p_rp_conn->p_cm_l, \
                                        0);

    /*count package size.*/
    pkg_arg.d_packageLen = _wilddog_conn_countPakgeSize(p_arg,
                             0,
                             (strlen(_CM_OFFLINE_PATH)+PROTOCOL_PATH_HEADLEN));

    /* creat coap package.*/
    p_pkg_index =(void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == 0)
        return WILDDOG_ERR_NULL;

    /* add host.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = p_arg->p_repo->p_rp_url->p_url_host;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_HOST,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
       goto _CONN_OFFLINE_ERR;

    /* add path.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = _CM_OFFLINE_PATH;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_PATH,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CONN_OFFLINE_ERR;

    /* add query - token.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = (void*)_wilddog_cm_ioctl(CM_CMD_SHORTTOKEN, \
                                             p_arg->p_repo->p_rp_conn->p_cm_l, \
                                             0);

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
         goto _CONN_OFFLINE_ERR;

    /* send to */
    res = (int)_wilddog_conn_send(pkg_arg.cmd, \
                                  (void*)p_pkg_index, \
                                  pkg_arg.d_token, \
                                  p_arg);
    if( res < 0)
        goto _CONN_OFFLINE_ERR;

    /* todo set system offline .*/
    return _wilddog_cm_ioctl(CM_CMD_OFFLINE,p_arg->p_repo->p_rp_conn->p_cm_l,0);

 _CONN_OFFLINE_ERR:

    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return res;
}

STATIC int WD_SYSTEM _wilddog_conn_onLine
    ( 
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    wilddog_assert(p_arg,WILDDOG_ERR_INVALID);    

    /*set online.*/
    return _wilddog_cm_ioctl(CM_CMD_ONLINE,p_arg->p_repo->p_rp_conn->p_cm_l,0);
}

/*
 * Function:    _wilddog_conn_trySync
 * Description: conn layer  try sync function
 *   
 * Input:       p_repo: the pointer of the repo struct
 * Output:      N/A
 * Return:      the result
*/
STATIC int WD_SYSTEM _wilddog_conn_trysync
    (
    Wilddog_ConnCmd_Arg_T *p_arg,
    int flags
    )
{
    if( !p_arg || !p_arg->p_repo ||\
        !p_arg->p_repo->p_rp_conn||\
        !p_arg->p_repo->p_rp_conn->p_cm_l)
        return WILDDOG_ERR_INVALID;
    
    return _wilddog_cm_ioctl(CM_CMD_TRYSYNC,p_arg->p_repo->p_rp_conn->p_cm_l,0);
}
/* call back  interface */
Wilddog_Func_T _wilddog_connCallBack_funcTable[WILDDOG_CONN_CBCMD_MAX + 1] = 
{
    (Wilddog_Func_T)_wilddog_conn_cb_get, //WILDDOG_CONN_CBCMD_GET
    (Wilddog_Func_T)_wilddog_conn_cb_set, //WILDDOG_CONN_CBCMD_SET
    (Wilddog_Func_T)_wilddog_conn_cb_push,//WILDDOG_CONN_CBCMD_PUSH
    (Wilddog_Func_T)_wilddog_conn_cb_set, //_wilddog_conn_cb_remove,
    (Wilddog_Func_T)_wilddog_conn_cb_get, //wilddog_conn_cb_on,
    (Wilddog_Func_T)_wilddog_conn_cb_set, //lddog_conn_cb_off,
    (Wilddog_Func_T)_wilddog_conn_cb_set, //lddog_conn_cb_auth,   

    (Wilddog_Func_T)_wilddog_conn_cb_set, //WILDDOG_CONN_CBCMD_ONDISSET,
    (Wilddog_Func_T)_wilddog_conn_cb_set, //WILDDOG_CONN_CBCMD_ONDISPUSH,
    (Wilddog_Func_T)_wilddog_conn_cb_set, //WILDDOG_CONN_CBCMD_ONDISREMOVE,

    (Wilddog_Func_T)_wilddog_conn_cb_set, //WILDDOG_CONN_CBCMD_CANCELDIS,
    (Wilddog_Func_T)_wilddog_conn_cb_set, //WILDDOG_CONN_CBCMD_ONLINE
    (Wilddog_Func_T)_wilddog_conn_cb_set, //WILDDOG_CONN_CBCMD_OFFLINE,
    NULL
};
STATIC int WD_SYSTEM _wilddog_conn_cb_ioctl
    (
    Wilddog_Conn_CBCmd_T cmd,
    void *p_args,
    int flags
    )
{
    if( cmd >  WILDDOG_CONN_CBCMD_MAX || \
        cmd < 0)
        return WILDDOG_ERR_INVALID;

    return (_wilddog_connCallBack_funcTable[cmd])(p_args,flags);
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

    (Wilddog_Func_T)_wilddog_conn_init,
    (Wilddog_Func_T)_wilddog_conn_deinit,

    NULL
};

STATIC int WD_SYSTEM _wilddog_conn_ioctl
    (
    Wilddog_Conn_Cmd_T cmd,
    void *p_args,
    int flags
    )
{
    if( cmd  > WILDDOG_CONN_CMD_MAX ||
        cmd < 0)
        return WILDDOG_ERR_INVALID;

    return (_wilddog_conn_funcTable[cmd])(p_args,flags);
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
    Wilddog_CM_InitArg_T cm_initArg;
    Wilddog_Conn_T* p_repo_conn = NULL;
    
    if(!p_repo)
        return NULL;

    p_repo_conn = (Wilddog_Conn_T*)wmalloc(sizeof(Wilddog_Conn_T));
    if( NULL ==p_repo_conn)
        return NULL;
    
    p_repo_conn->p_conn_repo = p_repo;
    p_repo->p_rp_conn = p_repo_conn;
    p_repo_conn->f_conn_ioctl = (Wilddog_Func_T)_wilddog_conn_ioctl;

    cm_initArg.p_repo= p_repo;
    cm_initArg.f_conn_cb = (Wilddog_Func_T)_wilddog_conn_cb_ioctl; 

    p_repo_conn->p_cm_l = (void*)_wilddog_cm_ioctl( CM_CMD_INIT,&cm_initArg,0);
    if( p_repo_conn->p_cm_l == NULL )
    {
        wfree(p_repo_conn);
        p_repo_conn = NULL;
    }
    
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
Wilddog_Conn_T* WD_SYSTEM _wilddog_conn_deinit(Wilddog_Repo_T*p_repo)
{
    if( !p_repo || !p_repo->p_rp_conn )
        return NULL;

    /* destory list.*/
    _wilddog_cm_ioctl(CM_CMD_DEINIT,p_repo->p_rp_conn->p_cm_l,0);
    
    wfree( p_repo->p_rp_conn);
    p_repo->p_rp_conn = NULL;
    
    return NULL;
}


