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
 * 0.4.0        lsx       2015-05-15  Create file.
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wilddog_payload.h"
     
#include "utlist.h"
#include "wilddog_config.h"
#include "wilddog.h"
#include "wilddog_conn_coap.h"
#include "wilddog_debug.h"
#include "wilddog_conn.h"
#include "wilddog_store.h"
#include "wilddog_common.h"
#include "wilddog_port.h"
#include "wilddog_url_parser.h"
#include "wilddog_payload.h"
#include "wilddog_api.h"
#include "test_lib.h"

#define DIFF(a,b)   ((a>b)?(a-b):(b-a))
#define RESP_IS_ERR(err)    ( err>=400)
#define _GETBYTE_H(b)   ((b>>4)&0x0f)		 
#define _GETBYTE_L(b)   (b&0x0f)

#define WILDDOG_OBSERVE_REQ (0X01)			/* observer sign*/
#define WILDDOG_OBSERVE_WAITNOTIF (0X11)

#define FIRSTRTRANSMIT_INV  (2000)		 /* retransmit cover (FIRSTRTRANSMIT_INV**n) */
#define WILDDOG_PING_INTERVAL 60000 

typedef enum _CONN_OBSERVE_FLAG_T{
    WILDDOG_Conn_Observe_Req,
    WILDDOG_Conn_Observe_Notif,
}_Conn_Observe_Flag_T;

typedef enum _CONN_AUTH_STATE{
    WILDDOG_CONN_AUTH_NOAUTH,
    WILDDOG_CONN_AUTH_QURES,
    WILDDOG_CONN_AUTH_AUTHED,
}Wilddog_Conn_AuthState_T;

STATIC Wilddog_Address_T l_defaultAddr_t[2] = 
{
	{4, {211,151,208,196}, 5683},
	{4, {211,151,208,197}, 5683}
};
STATIC u8 l_recvData[WILDDOG_PROTO_MAXSIZE];

STATIC void _wilddog_conn_urlFree(Wilddog_Url_T **pp_url);
STATIC int _wilddog_conn_urlMalloc
    (
    Wilddog_Conn_Cmd_T cmd,
    Wilddog_Conn_T *p_conn,
    Wilddog_Url_T * p_urlarg,
    Wilddog_Url_T **pp_urlsend
    );

STATIC int _wilddog_conn_send(Wilddog_Conn_Cmd_T cmd,
                                Wilddog_Repo_T *p_repo,
                                Wilddog_ConnCmd_Arg_T *p_arg);
                                
STATIC INLINE u8 _byte2char(u8 byte)
{

    if(byte>=0 && byte < 10)
        return  (byte+'0');
    else if(byte>= 0x0a && byte <= 0xf )            
        return  ((byte - 0x0a)+'A');
    return 0;
}
int _byte2bytestr(u8 *p_dst,u8 *p_dscr,u8 len)
{
    int i,j;
    for(i=0,j=0;i<len;i++)
    {
        p_dst[j++] = _byte2char(_GETBYTE_H(p_dscr[i]));
        p_dst[j++] = _byte2char(_GETBYTE_L(p_dscr[i]));
    }
    return 0;
}

STATIC int _wilddog_conn_hashIndex(Wilddog_Str_T *p_host, int totalNum)
{
	int len, i, total = 0;

	if(!p_host || totalNum <= 1)
		return 0;
	len = strlen((const char*)p_host);
	for(i = 0; i < len; i++)
	{
		total += p_host[i];
	}
	return total & ((1 << (totalNum - 1)) - 1);
}

STATIC int _wilddog_conn_getDefaultIpIndex(Wilddog_Str_T *p_host)
{
	int index;
	STATIC int count = 0;
	int totalNum = sizeof(l_defaultAddr_t)/sizeof(Wilddog_Address_T);
	index = _wilddog_conn_hashIndex(p_host, totalNum);

	index = (index + count) % totalNum;
	count++;

	return index;
}

STATIC int _wilddog_conn_getHost
    (
    Wilddog_Address_T *p_remoteAddr,
    Wilddog_Str_T *p_host,
    u16 d_port
    )
{   
    int res = -1;  
	int i;
#define WILDDOG_COAP_LOCAL_HOST "s-dal5-coap-1.wilddogio.com"

    res = wilddog_gethostbyname(p_remoteAddr,WILDDOG_COAP_LOCAL_HOST);
	if(-1 == res)
	{
		i = _wilddog_conn_getDefaultIpIndex(p_host);
		p_remoteAddr->len = l_defaultAddr_t[i].len;
		memcpy(p_remoteAddr->ip, l_defaultAddr_t[i].ip, l_defaultAddr_t[i].len);
	}

    p_remoteAddr->port = d_port;
	
#undef WILDDOG_COAP_LOCAL_HOST
    return res;
}


/*@ check auth status*/
STATIC INLINE void _wilddog_conn_auth_set
    (
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_AuthState_T status,
    u8* p_wauth
    )
{
    p_conn->d_auth_st = status;
    if(p_wauth)
        memcpy(&p_conn->d_wauth,p_wauth,AUTHR_LEN);
    
}
STATIC Wilddog_Conn_AuthState_T _wilddog_conn_auth_get
    (
    Wilddog_Conn_T *p_conn
    )
{
    return (p_conn->d_auth_st);
}
STATIC int _wilddog_conn_auth_send(Wilddog_Repo_T *p_repo)
{
    Wilddog_ConnCmd_Arg_T d_arg;
    Wilddog_Url_T d_url;

    memset(&d_arg,0,sizeof(Wilddog_ConnCmd_Arg_T));
    memset(&d_url,0,sizeof(Wilddog_Url_T));
    
    d_url.p_url_host = p_repo->p_rp_url->p_url_host;
    d_arg.p_url = &d_url;
    return _wilddog_conn_send(WILDDOG_CONN_CMD_AUTH,p_repo,&d_arg);

}
STATIC int _wilddog_conn_auth_delete(Wilddog_Repo_T *p_repo)
{
    if(p_repo->p_rp_conn->d_auth_st == WILDDOG_CONN_AUTH_NOAUTH )
        {
            
            return _wilddog_conn_auth_send(p_repo);
        }
    return 0;
}
/* since observer flag have been set,this request node will not delete
** while receive respond or notify.To delete it , an off request must 
** sent.*/
STATIC int _wilddog_conn_observeFlagSet
    (
    _Conn_Observe_Flag_T flag,
    Wilddog_Conn_Node_T *p_cn_node
    )
{
    int res =0;
    if(p_cn_node->d_cmd == WILDDOG_CONN_CMD_ON)
    {
        p_cn_node->d_observe_flag = flag;
        res = 1;
        }
    return res;
}
STATIC int _wilddog_conn_observerISNotify
    (
    Wilddog_Conn_Node_T *p_cn_node
    )
{
    return (p_cn_node->d_observe_flag == WILDDOG_Conn_Observe_Notif);
}

STATIC int _wilddog_conn_urlMalloc
    (
    Wilddog_Conn_Cmd_T cmd,
    Wilddog_Conn_T *p_conn,
    Wilddog_Url_T * p_urlarg,
    Wilddog_Url_T **pp_urlsend
    )
{
    if(cmd == WILDDOG_CONN_CMD_AUTH)
    {
        _wilddog_conn_auth_set(p_conn,WILDDOG_CONN_AUTH_QURES,NULL);
        *pp_urlsend = wmalloc(sizeof(Wilddog_Url_T));
        if(!(*pp_urlsend))
            return WILDDOG_ERR_NULL;
        (*pp_urlsend)->p_url_path = wmalloc(sizeof(AUTHR_PATH));
        (*pp_urlsend)->p_url_host = wmalloc( \
                                strlen((const char *)p_urlarg->p_url_host)+1);
        
        if(!(*pp_urlsend)->p_url_path || !(*pp_urlsend)->p_url_host )   
        {
            _wilddog_conn_urlFree(pp_urlsend);
            return WILDDOG_ERR_NULL;
        }
        
        memcpy((*pp_urlsend)->p_url_path,AUTHR_PATH,sizeof(AUTHR_PATH));
        memcpy((*pp_urlsend)->p_url_host,p_urlarg->p_url_host, \
                                strlen((const char *)p_urlarg->p_url_host));
        
        return WILDDOG_ERR_NOERR;
        
    }
    else /* authed*/
    {
        int len = strlen(AUTHR_QURES);
        
        (*pp_urlsend) = wmalloc(sizeof(Wilddog_Url_T));
        if(!(*pp_urlsend))
            return WILDDOG_ERR_NULL;
        (*pp_urlsend)->p_url_host = wmalloc( \
                                strlen((const char *)p_urlarg->p_url_host)+1);
        (*pp_urlsend)->p_url_path = wmalloc( \
                                strlen((const char *)p_urlarg->p_url_path)+1);
        (*pp_urlsend)->p_url_query= wmalloc(len+ 10);
        if( !(*pp_urlsend)->p_url_path || \
            !(*pp_urlsend)->p_url_host || \
            !(*pp_urlsend)->p_url_query
            )   
        {
            _wilddog_conn_urlFree(pp_urlsend);
            return WILDDOG_ERR_NULL;
        }
        memcpy((*pp_urlsend)->p_url_host,p_urlarg->p_url_host, \
                    strlen((const char *)p_urlarg->p_url_host));
        memcpy((*pp_urlsend)->p_url_path,p_urlarg->p_url_path,\
                    strlen((const char *)p_urlarg->p_url_path));
        memcpy((*pp_urlsend)->p_url_query,AUTHR_QURES,len);
        _byte2bytestr((u8*)&((*pp_urlsend)->p_url_query[len]), \
                            (u8*)&p_conn->d_wauth,4);
    }
    return WILDDOG_ERR_NOERR;
}
STATIC void _wilddog_conn_urlFree(Wilddog_Url_T **pp_url)
{
    
    if(*pp_url){
        Wilddog_Url_T *p_url = *pp_url;
        if(p_url->p_url_host)
            wfree(p_url->p_url_host);
        if(p_url->p_url_path)
            wfree(p_url->p_url_path);
        if(p_url->p_url_query)
            wfree(p_url->p_url_query);
        wfree(p_url);
        *pp_url = NULL;
    }
}

STATIC INLINE void _wilddog_conn_node_addlist
    (
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *nodeadd
    )
{
    LL_APPEND(p_conn->p_conn_node_hd,nodeadd);
}

STATIC int _wilddog_conn_node_add
    (
    Wilddog_Conn_Cmd_T cmd ,
    Wilddog_ConnCmd_Arg_T *p_arg,
    void *p_pkt,
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T **pp_conn_node
    )
{   
    int res = WILDDOG_ERR_NOERR;

    *pp_conn_node = wmalloc(sizeof(Wilddog_Conn_Node_T));
    if(!(*pp_conn_node))
        return WILDDOG_ERR_NULL;
        
    p_conn->d_ralySend = _wilddog_getTime();
    (*pp_conn_node)->d_cmd = cmd;
    (*pp_conn_node)->p_cn_pkt = p_pkt;
    
    (*pp_conn_node)->f_cn_callback = p_arg->p_complete;
    (*pp_conn_node)->p_cn_cb_arg = p_arg->p_completeArg;
    (*pp_conn_node)->d_cn_regist_tm=  _wilddog_getTime();
    
    (*pp_conn_node)->d_cn_retansmit_cnt = 1;
    (*pp_conn_node)->d_cn_nextsend_tm = FIRSTRTRANSMIT_INV + _wilddog_getTime();
    wilddog_debug_level(WD_DEBUG_LOG,"AddNode=%p\n",(*pp_conn_node));
    _wilddog_conn_node_addlist(p_conn,(*pp_conn_node));
    return res;
}

void _wilddog_conn_node_remove
    (
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T **pp_conn_node
    )
{
    if(*pp_conn_node)
    {
        LL_DELETE(p_conn->p_conn_node_hd,*pp_conn_node);
        if((*pp_conn_node)->p_cn_path)
        {
            wfree((*pp_conn_node)->p_cn_path);
            (*pp_conn_node)->p_cn_path = NULL;
        }
        
        wilddog_debug_level(WD_DEBUG_WARN, "!!!!!!remove node=%p\n",pp_conn_node);
        wfree(*pp_conn_node);
        *pp_conn_node = NULL;
    }
}

STATIC void _wilddog_conn_freePayload(Wilddog_Payload_T **pp_data)
{

    if(*pp_data)
    {
        Wilddog_Payload_T* p_data = *pp_data;
        if(p_data->p_dt_data)
        {            
            wfree(p_data->p_dt_data);
            p_data->p_dt_data = NULL;
        }
        if(p_data)
        {
            wfree(p_data);
            p_data = NULL;
        }
    }

}

STATIC Wilddog_Payload_T *_wilddog_conn_payloadGet(
    Wilddog_Conn_Cmd_T cmd,
    Wilddog_Repo_T *p_repo,
    Wilddog_ConnCmd_Arg_T *p_arg)
{
    Wilddog_Payload_T *p_payload = NULL;
    if(cmd == WILDDOG_CONN_CMD_AUTH)
    {
        u8 *p_buf =NULL;
        
        p_repo->p_rp_store->p_se_callback(p_repo->p_rp_store, \
                                            WILDDOG_STORE_CMD_GETAUTH,&p_buf,0);
        if(p_buf == NULL)   
            return NULL;
        p_payload = wmalloc(sizeof(Wilddog_Payload_T));
        
        if(p_payload == NULL )
            return NULL;
        p_payload->d_dt_len = strlen((const char *)p_buf);  
        p_payload->p_dt_data = wmalloc(p_payload->d_dt_len);
        memcpy(p_payload->p_dt_data,p_buf,p_payload->d_dt_len);
        
    }
    else
        if(p_arg->p_data)
        {
            p_payload = _wilddog_node2Payload(p_arg->p_data);
        }
    
    return p_payload;
}
/* store path */
STATIC int _wilddog_conn_Observer_on
    (
    Wilddog_Conn_Node_T *p_conn_node,
    u8 *Argpath
    )
{

    int len = strlen((const char *)Argpath);
    p_conn_node->p_cn_path = wmalloc(len+1);
    if(p_conn_node->p_cn_path == NULL)
        return WILDDOG_ERR_NULL;
    memcpy(p_conn_node->p_cn_path,Argpath,len);
    return WILDDOG_ERR_NOERR;
}
/* find on node with path and delete it */
STATIC void _wilddog_conn_cmd_offEvent
    (
    Wilddog_Conn_T *p_conn,
    u8 *p_path
    )
{
    Wilddog_Conn_Node_T *p_cur,*p_tmp=NULL;
    LL_FOREACH_SAFE(p_conn->p_conn_node_hd,p_cur,p_tmp)
    {
        if(p_cur->d_cmd == WILDDOG_CONN_CMD_ON                         && \
            p_cur->p_cn_path != NULL                                    && \
            strlen((const char *)p_cur->p_cn_path) == strlen((const char *)p_path)  && \
            (memcmp(p_path,p_cur->p_cn_path, strlen((const char *)p_cur->p_cn_path)) == 0)
            )
        {
            if(p_cur->p_cn_pkt != NULL)
                _wilddog_conn_pkt_free(&p_cur->p_cn_pkt);
            _wilddog_conn_node_remove(p_conn,&p_cur);   
            return ;
        }
    }

}
STATIC int _wilddog_conn_Observer_handle(
    Wilddog_ConnCmd_Arg_T *p_arg,
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_conn_node)
{
    int res =0;
    
    switch(p_conn_node->d_cmd)
    {
        case WILDDOG_CONN_CMD_ON:
            res = _wilddog_conn_Observer_on(p_conn_node,p_arg->p_url->p_url_path);
            break;
        case WILDDOG_CONN_CMD_OFF:
            _wilddog_conn_cmd_offEvent(p_conn,p_arg->p_url->p_url_path);
            break;
    }
    
    return res;
}
/* query = auth*/
STATIC  int _wilddog_conn_sendWithAuth
    (
    Wilddog_Conn_Cmd_T cmd,
    void *p_pkt,
    Wilddog_Conn_T *p_conn
    )
{   
    if( cmd != WILDDOG_CONN_CMD_AUTH  &&
        _wilddog_conn_auth_get(p_conn) != WILDDOG_CONN_AUTH_AUTHED)
            return 0;
    else 
        return _wilddog_conn_pkt_send(p_conn->d_socketid, \
                        &p_conn->d_remoteAddr,(u8 *)&p_conn->d_wauth,p_pkt);
}
/* add cmd request to sending list */
STATIC int _wilddog_conn_send
    (
    Wilddog_Conn_Cmd_T cmd,
    Wilddog_Repo_T *p_repo,
    Wilddog_ConnCmd_Arg_T *p_arg
    )
{
    
    int res =0 ;
    Wilddog_Conn_PktSend_T d_conn_send;
    Wilddog_Conn_T *p_conn= NULL;
    Wilddog_Conn_Node_T *p_conn_node = NULL;
    Wilddog_Payload_T *p_payload = NULL;
    void *p_pkt = NULL;
    /*  illegality input */
    if( !p_arg || !p_repo || !p_repo->p_rp_conn)
        return WILDDOG_ERR_INVALID;
    p_conn = p_repo->p_rp_conn;
    /* get host  */
    if( p_conn->d_remoteAddr.len == 0 )
    {
        res = _wilddog_conn_getHost( &p_conn->d_remoteAddr,
                             p_arg->p_url->p_url_host,WILDDOG_PORT);
#ifdef WILDDOG_SELFTEST                            
		ramtest_skipLastmalloc();
#endif
     }
    /* cmd analyze */
    memset(&d_conn_send,0,sizeof(Wilddog_Conn_PktSend_T));
    d_conn_send.cmd = cmd;

    p_payload = _wilddog_conn_payloadGet(cmd,p_repo,p_arg);
    if(p_payload)
    {
        
        d_conn_send.d_payloadlen = p_payload->d_dt_len;
        d_conn_send.p_payload = p_payload->p_dt_data;
    }

    /*  malloc */
    res = _wilddog_conn_urlMalloc(cmd,p_repo->p_rp_conn,p_arg->p_url, \
                                    &d_conn_send.p_url);
    if(res < 0) 
        goto _CONN_SEND_FREE;
    
    /*  creat pkt */

    res = _wilddog_conn_pkt_creat(&d_conn_send,&p_pkt);
    wilddog_debug_level(WD_DEBUG_LOG,"get pkt node=%p\n",p_pkt);
    if(res < 0)
        goto _CONN_SEND_FREE;
    /*  send */
    res = _wilddog_conn_sendWithAuth(cmd,p_pkt,p_conn);
    if(res < 0)
    {
        if( res  == WILDDOG_ERR_SOCKETERR)
        {
            
            res = wilddog_openSocket((int*)&(p_conn->d_socketid));
            
            res = _wilddog_conn_sendWithAuth(cmd,p_conn,p_pkt);
            if(res == WILDDOG_ERR_SOCKETERR)
                goto _CONN_SEND_FREE;
        }
        else 
            goto _CONN_SEND_FREE;
    }
    /* malloc conn node  add list */
    res = _wilddog_conn_node_add(cmd,p_arg,p_pkt,p_conn,&p_conn_node);

    if(res < 0)
        goto _CONN_SEND_FREE;
    
    
    res = _wilddog_conn_Observer_handle(p_arg,p_conn,p_conn_node);

_CONN_SEND_FREE:

    _wilddog_conn_freePayload(&p_payload);
    _wilddog_conn_urlFree(&d_conn_send.p_url);
    return res;
}

STATIC void _wilddog_conn_cb_auth(Wilddog_Conn_Node_T* p_cn_node,u32 err)
{
    
    if(p_cn_node->f_cn_callback)
        p_cn_node->f_cn_callback(p_cn_node->p_cn_cb_arg,err);
}

STATIC void _wilddog_conn_cb_set(Wilddog_Conn_Node_T * p_cn_node,u32 err)
{
    if(p_cn_node->f_cn_callback)
        p_cn_node->f_cn_callback(p_cn_node->p_cn_cb_arg,err);

}

STATIC void _wilddog_conn_cb_push
    (
    Wilddog_Conn_Node_T * p_cn_node,
    unsigned char *p_respbuf,
    u32 d_respsize,
    u32 err
    )
{
    if(p_cn_node->f_cn_callback)
        p_cn_node->f_cn_callback(p_respbuf,p_cn_node->p_cn_cb_arg,err);

}

STATIC void _wilddog_conn_cb_get
    (
    Wilddog_Conn_Node_T * p_cn_node,
    unsigned char *p_respbuf,
    u32 d_respsize,
    u32 err
    )
{
    /* get */
    Wilddog_Payload_T tmpData;
    Wilddog_Node_T* p_snapshot = NULL;
    
    
    if(  d_respsize>0 && p_respbuf)
    {
        tmpData.d_dt_len = d_respsize;
        tmpData.d_dt_pos = 0;
        tmpData.p_dt_data = p_respbuf;
#ifdef WILDDOG_SELFTEST
		ramtest_skipLastmalloc();
#endif
        p_snapshot = _wilddog_payload2Node((Wilddog_Payload_T*)&tmpData);
#ifdef WILDDOG_SELFTEST        
        ramtest_caculate_nodeRam();
#endif

    }
    else 
        p_snapshot = NULL;
    
    if(p_cn_node->f_cn_callback)
        p_cn_node->f_cn_callback(p_snapshot,p_cn_node->p_cn_cb_arg,err);

    if(p_snapshot)
        wilddog_node_delete(p_snapshot);
    
    return;
}
STATIC int _wilddog_conn_cb
    (
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_cn_node,
    Wilddog_Conn_RecvData_T *p_cn_recvData
    )
{
    Wilddog_Conn_RecvData_T d_cn_recvData;

    wilddog_debug_level(WD_DEBUG_WARN,"!!!!ERROR=%lu\n",p_cn_recvData->d_RecvErr);

    d_cn_recvData.d_RecvErr = p_cn_recvData->d_RecvErr;
    /* handle error**/
    if(RESP_IS_ERR(p_cn_recvData->d_RecvErr))
    {
    	/* recv unauth err */
        if(p_cn_recvData->d_RecvErr == WILDDOG_HTTP_UNAUTHORIZED)
            _wilddog_conn_auth_set(p_conn,WILDDOG_CONN_AUTH_NOAUTH,NULL);
        d_cn_recvData.d_recvlen = 0;
        d_cn_recvData.p_Recvdata = NULL;
    }else
    {
        d_cn_recvData.d_recvlen = p_cn_recvData->d_recvlen;
        d_cn_recvData.p_Recvdata = p_cn_recvData->p_Recvdata;
    }
    
    switch(p_cn_node->d_cmd)
    {
        case WILDDOG_CONN_CMD_AUTH:
#ifdef WILDDOG_SELFTEST
        	performtest_tm_getAuthHandle();
#endif
            if(d_cn_recvData.d_RecvErr == WILDDOG_HTTP_OK )
                _wilddog_conn_auth_set(p_conn,WILDDOG_CONN_AUTH_AUTHED, \
                                        d_cn_recvData.p_Recvdata);
                
            _wilddog_conn_cb_auth(p_cn_node,d_cn_recvData.d_RecvErr);
            
            break;
        
        case WILDDOG_CONN_CMD_PUSH:
            _wilddog_conn_cb_push
                (
                p_cn_node,
                d_cn_recvData.p_Recvdata,
                d_cn_recvData.d_recvlen,
                d_cn_recvData.d_RecvErr
                );
            break;
        
        case WILDDOG_CONN_CMD_SET:
        case WILDDOG_CONN_CMD_REMOVE:
            _wilddog_conn_cb_set(p_cn_node,d_cn_recvData.d_RecvErr);
            break;
        
        case WILDDOG_CONN_CMD_GET:
        case WILDDOG_CONN_CMD_ON:
            _wilddog_conn_cb_get
                (
                p_cn_node,
                d_cn_recvData.p_Recvdata,
                d_cn_recvData.d_recvlen,
                d_cn_recvData.d_RecvErr
                );
            break;
        case WILDDOG_CONN_CMD_OFF:
            break;
        default:
            break;
        
    }
    return 0;
} 
STATIC int _wilddog_conn_timeoutCB
    (
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_cn_node
    )
{
    Wilddog_Conn_RecvData_T d_cn_recvData;
    d_cn_recvData.d_RecvErr = WILDDOG_ERR_RECVTIMEOUT;
    d_cn_recvData.d_recvlen = 0;
    d_cn_recvData.p_Recvdata = NULL;
    
    return _wilddog_conn_cb(p_conn,p_cn_node,&d_cn_recvData);
}
STATIC int _wilddog_conn_recv(Wilddog_Conn_T *p_conn)
{
    Wilddog_Conn_Node_T *p_cur=NULL,*p_tmp=NULL;
    int res =0;
    void *p_cn_pkt = NULL;
    Wilddog_Conn_RecvData_T d_cn_recvpkt;
    d_cn_recvpkt.d_recvlen= WILDDOG_PROTO_MAXSIZE;
    d_cn_recvpkt.d_RecvErr = 0;
	/*remember it is not thread safty!!!*/
    d_cn_recvpkt.p_Recvdata = l_recvData;
    memset(l_recvData, 0, WILDDOG_PROTO_MAXSIZE);
    if(d_cn_recvpkt.p_Recvdata  == NULL)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
        return WILDDOG_ERR_NULL;
    }
    res = _wilddog_conn_pkt_recv(p_conn->d_socketid,&p_conn->d_remoteAddr,
                                 &p_cn_pkt,&d_cn_recvpkt);
    if( res <0 || p_cn_pkt == NULL || d_cn_recvpkt.p_Recvdata == NULL )
        goto _RECV_END_ ;

    LL_FOREACH_SAFE(p_conn->p_conn_node_hd,p_cur,p_tmp)
    {
        if( p_cur->p_cn_pkt == p_cn_pkt )
        {
            res = _wilddog_conn_cb(p_conn,p_cur,&d_cn_recvpkt);
            p_conn->d_ralyRecv = _wilddog_getTime();
            if(_wilddog_conn_observeFlagSet(WILDDOG_Conn_Observe_Notif,p_cur)== 0)
                    _wilddog_conn_node_remove(p_conn,&p_cur);
        }
    }
_RECV_END_:

    d_cn_recvpkt.p_Recvdata = NULL;
    return res;
}
STATIC int _wilddog_conn_pingSend
    (
    u32 fd,
    Wilddog_Address_T *Addrin,
    Wilddog_Conn_T *p_conn
    )
{
    int res = NULL;
    void *p_pkt = NULL;
    Wilddog_Conn_PktSend_T d_conn_send;
    memset(&d_conn_send,0,sizeof(Wilddog_Conn_PktSend_T));
    /*  creat pkt */
    d_conn_send.cmd = WILDDOG_CONN_CMD_PING;
    res = _wilddog_conn_pkt_creat(&d_conn_send,&p_pkt);
    if(res < 0)
        return WILDDOG_ERR_NULL;
    /*  send */    
    res = _wilddog_conn_pkt_send(fd,Addrin,NULL,p_pkt);
    _wilddog_conn_pkt_free(&p_pkt);
	wilddog_debug("send ping %s!", res >= 0?("Success"):("Failed"));
    return res;
    
}

STATIC int _wilddog_conn_keepLink(Wilddog_Conn_T *p_conn)
{ 
    int res =0 ;
    if( DIFF(p_conn->d_ralySend, _wilddog_getTime()) > WILDDOG_PING_INTERVAL)
    {

        res = _wilddog_conn_pingSend(p_conn->d_socketid,&p_conn->d_remoteAddr, \
                                        p_conn);
        if(res >= 0)
            p_conn->d_ralySend =_wilddog_getTime();
    }
    
    return res;
}

STATIC int _wilddog_conn_retransTimeout
    (
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_cn_node
    )
{
    if( DIFF( p_cn_node->d_cn_regist_tm, \
                _wilddog_getTime()) > WILDDOG_RETRANSMITE_TIME
        )
    {   
        
        wilddog_debug_level(WD_DEBUG_LOG, \
                                "start time=%lu;curr time=%lu;max timout=%u", \
                                p_cn_node->d_cn_regist_tm,_wilddog_getTime(), \
                                WILDDOG_RETRANSMITE_TIME);
        _wilddog_conn_timeoutCB(p_conn,p_cn_node);
        _wilddog_conn_node_remove(p_conn,&p_cn_node);
        return 1;
    }
    return 0;
}
STATIC int _wilddog_conn_retransmit(Wilddog_Conn_T *p_conn)
{
    Wilddog_Conn_Node_T *cur,*tmp;
    int res ;
    u32 curtm = _wilddog_getTime();
    
    LL_FOREACH_SAFE(p_conn->p_conn_node_hd,cur,tmp)
    {
        if((_wilddog_conn_observerISNotify(cur)== 0)&& 
            ((curtm > cur->d_cn_nextsend_tm) || 
            (DIFF(curtm,cur->d_cn_nextsend_tm) > ( 0xffffffff)) ))
            {
                if(_wilddog_conn_retransTimeout(p_conn,cur))
                    continue;
                
                wilddog_debug_level(WD_DEBUG_WARN," <><><> Retransmit!!");
                res = _wilddog_conn_sendWithAuth(cur->d_cmd,cur->p_cn_pkt,p_conn);
                if(res >=0 )
                    cur->d_cn_nextsend_tm = curtm + \
                            (FIRSTRTRANSMIT_INV << (cur->d_cn_retansmit_cnt++));    
            }
    }
    return res ;
}

STATIC int _wilddog_conn_trySync(Wilddog_Repo_T *p_repo)
{
    int res = 0;
    
    res = _wilddog_conn_recv(p_repo->p_rp_conn);

    res = _wilddog_conn_keepLink(p_repo->p_rp_conn);
    res = _wilddog_conn_retransmit(p_repo->p_rp_conn);
    res = _wilddog_conn_auth_delete(p_repo);

    return res ;
    
}
/* do dtls hanshake */
Wilddog_Conn_T * _wilddog_conn_init(Wilddog_Repo_T* p_repo)
{
    if(!p_repo)
        return NULL;
    Wilddog_Conn_T* p_repo_conn   = (Wilddog_Conn_T*)wmalloc(sizeof(Wilddog_Conn_T));
    if(!p_repo_conn)
        return NULL;

    p_repo_conn->p_conn_repo = p_repo;
    p_repo_conn->f_conn_send = (Wilddog_Func_T)_wilddog_conn_send;
    p_repo_conn->f_conn_trysyc = (Wilddog_Func_T)_wilddog_conn_trySync;
    p_repo->p_rp_conn = p_repo_conn;
 #ifdef WILDDOG_SELFTEST                       
	ramtest_skipLastmalloc();
#endif   
    wilddog_openSocket((int*)&p_repo_conn->d_socketid);
    _wilddog_conn_getHost( &p_repo->p_rp_conn->d_remoteAddr,
                             p_repo->p_rp_url->p_url_host,WILDDOG_PORT);
#ifdef WILDDOG_SELFTEST                        
	ramtest_gethostbyname();
#endif
#ifdef WILDDOG_SELFTEST     
	performtest_star_tm();
#endif
    _wilddog_conn_pkt_init(p_repo_conn->d_socketid,&p_repo_conn->d_remoteAddr);
#ifdef WILDDOG_SELFTEST    
	performtest_tm_getDtlsHsk();	
 	performtest_star_tm();
#endif    
    _wilddog_conn_auth_send(p_repo);
#ifdef WILDDOG_SELFTEST                            
		ramtest_skipLastmalloc();
#endif 
#ifdef WILDDOG_SELFTEST
 	performtest_tm_getAuthSend();
 	performtest_star_tm();
#endif 

    return p_repo_conn;
}
/* destory conn list */
Wilddog_Conn_T* _wilddog_conn_deinit(Wilddog_Repo_T*p_repo)
{

    Wilddog_Conn_Node_T *p_hd,*cur,*tmp;

    if( !p_repo || !p_repo->p_rp_conn )
        return NULL;
        
    p_hd = p_repo->p_rp_conn->p_conn_node_hd;
    if(p_hd)
    {
        LL_FOREACH_SAFE(p_hd,cur,tmp)
        {
            _wilddog_conn_node_remove(p_repo->p_rp_conn,&cur);
        }
    }
    _wilddog_conn_pkt_deinit(p_repo->p_rp_conn->d_socketid, \
                                &p_repo->p_rp_conn->d_remoteAddr);
    p_repo->p_rp_conn->p_conn_node_hd = NULL;
    p_repo->p_rp_conn->d_socketid = 0;
    wfree( p_repo->p_rp_conn);
    p_repo->p_rp_conn = NULL;
    return NULL;
}

