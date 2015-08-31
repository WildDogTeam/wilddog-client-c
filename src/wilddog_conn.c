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

#define DIFF(a,b)   	((a>b)?(a-b):(b-a))
#define RESP_IS_ERR(err)    	( err>= 400)

#define CB_NO_SERVERERR(err)	((err)< WILDDOG_HTTP_INTERNAL_SERVER_ERR )
#define CB_IS_NORESPONSE(err)	((err) == WILDDOG_ERR_RECVTIMEOUT)
#define _GETBYTE_H(b)   ((b>>4)&0x0f)		 
#define _GETBYTE_L(b)   (b&0x0f)

#define WILDDOG_OBSERVE_REQ (0X01)			/* observer sign*/
#define WILDDOG_OBSERVE_WAITNOTIF (0X11)

#define FIRSTRTRANSMIT_INV  (2000)		 /* retransmit cover (FIRSTRTRANSMIT_INV**n) */
#define WILDDOG_PING_INTERVAL 60000 

typedef enum _CONN_BOOT_T{
	WILDDOG_CONN_FALSE,
	WILDDOG_CONN_TRUE,
}_Conn_Boot_T;

typedef enum _CONN_OBSERVE_FLAG_T{
    WILDDOG_Conn_Observe_Req =0x01,
    WILDDOG_Conn_Observe_Notif =0x11,
}_Conn_Observe_Flag_T;

typedef enum _CONN_AUTH_STATE{
	
    WILDDOG_CONN_AUTH_NOAUTH,
    WILDDOG_CONN_AUTH_DOAUTH,
    WILDDOG_CONN_AUTH_AUTHING,
    WILDDOG_CONN_AUTH_AUTHED,
    
}Wilddog_Conn_AuthState_T;

typedef enum _CONN_PONG_STATE_T{
    WILDDOG_CONN_ONLINE,
	WILDDOG_CONN_OFFLINE
}_Conn_Pong_State_T;
typedef enum _CONN_REOBSERVER_STATE_T
{
	WILDDOG_CONN_NOREOBSERVER,
	WILDDOG_CONN_DOREOBSERVER
}_Conn_Reobserver_T;

typedef struct _WILDDOG_RECV_STRUCT
{	
	u8 data[WILDDOG_PROTO_MAXSIZE];	
	u8 isused;
}_wilddog_Recv_T;
STATIC _wilddog_Recv_T l_recvData;
STATIC int l_initCount = 0;
STATIC int _wilddog_conn_send(Wilddog_Conn_Cmd_T cmd,
                                Wilddog_Repo_T *p_repo,
                                Wilddog_ConnCmd_Arg_T *p_arg);
STATIC void _wilddog_conn_updataRegistertime
	( 
	Wilddog_Conn_T *p_conn
	);

STATIC int _wilddog_conn_cb
	(
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_cn_node,
    Wilddog_Conn_RecvData_T *p_cn_recvData
    );
STATIC void _wilddog_conn_setReObserverFlag
	(
	Wilddog_Conn_T *p_conn,
	BOOL state
	);



/*LOCK and UNLOCK used for multi thread*/
STATIC INLINE int _wilddog_conn_recvBufLock(int timeout)
{	
	return 0;
}
STATIC INLINE void _wilddog_conn_recvBufUnlock(void)
{	
	return;
}
STATIC void _wilddog_conn_initRecvBuffer(void)
{	
	_wilddog_conn_recvBufLock(0);	
	memset(&l_recvData, 0, sizeof(l_recvData));
	_wilddog_conn_recvBufUnlock();	
	return;
}
STATIC u8* _wilddog_conn_mallocRecvBuffer(void)
{	
	u8* buffer = NULL;	
	_wilddog_conn_recvBufLock(0);	
	
	/*TODO: use round-robin queue*/	
	if(l_recvData.isused == FALSE)	
	{		
		buffer = l_recvData.data;		
		l_recvData.isused = TRUE;		
		memset(buffer, 0, WILDDOG_PROTO_MAXSIZE);	
	}	
	_wilddog_conn_recvBufUnlock();	
	return buffer;
}
STATIC void _wilddog_conn_freeRecvBuffer(u8* ptr)
{	
	if(!ptr)		
		return;		
	_wilddog_conn_recvBufLock(0);	
	/*TODO: if use round-robin queue, find index by ptr*/	
	if(l_recvData.data == ptr && TRUE == l_recvData.isused)	
	{		
		l_recvData.isused = FALSE;	
	}	
	_wilddog_conn_recvBufUnlock();
}                                

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

	
    d_arg.p_url = &d_url;
    d_url.p_url_host = p_repo->p_rp_url->p_url_host;
	
    return _wilddog_conn_send(WILDDOG_CONN_CMD_AUTH,p_repo,&d_arg);

}
STATIC int _wilddog_conn_auth_detect(Wilddog_Repo_T *p_repo)
{
    if(p_repo->p_rp_conn->d_auth_st == WILDDOG_CONN_AUTH_DOAUTH )
        {
            
            return _wilddog_conn_auth_send(p_repo);
        }
    return 0;
}
/* check auth status and change it befor sending any request */
STATIC void _wilddog_conn_auth_reset
	( 
	Wilddog_Conn_Cmd_T cmd,
	Wilddog_Conn_T *p_conn
	)
{
	if( cmd != WILDDOG_CONN_CMD_AUTH )
	{
		if(WILDDOG_CONN_AUTH_NOAUTH == _wilddog_conn_auth_get(p_conn))
			_wilddog_conn_auth_set(p_conn,WILDDOG_CONN_AUTH_DOAUTH,NULL);
	}
	else
		_wilddog_conn_auth_set(p_conn,WILDDOG_CONN_AUTH_AUTHING,NULL);
}

STATIC INLINE void _wilddog_conn_pongstatus_set
	(
	Wilddog_Conn_T *p_conn,
	_Conn_Pong_State_T state
	)
{
	p_conn->d_pong_state = state;
}
STATIC INLINE u8 _wilddog_conn_pongstatus_get(Wilddog_Conn_T *p_conn)
{
	return p_conn->d_pong_state;
}
/* put off pong request until next cycle */
STATIC void _wilddog_conn_pong_resetNextSendTm
	(
		Wilddog_Conn_T *p_conn,
		u32 tiemIncrement
	)
{
	p_conn->d_pong_nextSendTm = _wilddog_getTime()+ tiemIncrement;
	
}

STATIC int _wilddog_conn_pong_send(Wilddog_Conn_T *p_conn)
{
	Wilddog_ConnCmd_Arg_T d_arg;
    Wilddog_Url_T d_url;

	memset(&d_arg,0,sizeof(Wilddog_ConnCmd_Arg_T));
	memset(&d_url,0,sizeof(Wilddog_Url_T));
	
	d_arg.p_url = &d_url;
	d_url.p_url_host = p_conn->p_conn_repo->p_rp_url->p_url_host;
	
	_wilddog_conn_pong_resetNextSendTm(p_conn,PONG_REQUESINTERVAL);
	return _wilddog_conn_send(WILDDOG_CONN_CMD_PONG,p_conn->p_conn_repo,&d_arg);
}

STATIC int _wilddog_conn_pong_trysync(Wilddog_Repo_T *p_repo)
{
	Wilddog_Conn_T *p_conn = NULL;

	p_conn = p_repo->p_rp_conn;
	if( _wilddog_getTime() >= p_conn->d_pong_nextSendTm &&
		DIFF(_wilddog_getTime(),p_conn->d_pong_nextSendTm) < (0xffff) )
	{
		return _wilddog_conn_pong_send(p_conn);

	}
    return WILDDOG_ERR_NOERR;
}
STATIC int _wilddog_conn_pong_cb
	(
	Wilddog_Conn_T *p_conn,	
    Wilddog_Conn_Node_T *p_cn_node,
	Wilddog_Conn_RecvData_T *p_cn_recvData
	)
{
	
    Wilddog_Payload_T tmpData;
    Wilddog_Node_T* p_snapshot = NULL;

	static int tempflag = 0; /* todo test */
    if(  p_cn_recvData->d_recvlen >0 && p_cn_recvData->p_Recvdata)
    {
    	u8 pongIdx;
		int len;
        tmpData.d_dt_len = p_cn_recvData->d_recvlen;
        tmpData.d_dt_pos = 0;
        tmpData.p_dt_data = p_cn_recvData->p_Recvdata;

        p_snapshot = _wilddog_payload2Node((Wilddog_Payload_T*)&tmpData);
        
		pongIdx  = (u8) *(wilddog_node_getValue(p_snapshot,&len));
		if( pongIdx != ( p_conn->d_pong_num+1))
		{
			/* todo server error*/
			_wilddog_conn_pongstatus_set(p_conn,WILDDOG_CONN_OFFLINE);
		}
		else
		{
			if(tempflag == 0)
			{
				tempflag= 1;
			}
			if(_wilddog_conn_pongstatus_get(p_conn) == WILDDOG_CONN_OFFLINE )
			{
				_wilddog_conn_setReObserverFlag(p_conn,TRUE);
				_wilddog_conn_pongstatus_set(p_conn,WILDDOG_CONN_ONLINE);
			}
			p_conn->d_pong_num = pongIdx;
			_wilddog_conn_pong_resetNextSendTm(p_conn,PONG_REQUESINTERVAL);
		}		
		if(p_snapshot)
        	wilddog_node_delete(p_snapshot);
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
STATIC int _wilddog_conn_isNotify
    (
    Wilddog_Conn_Node_T *p_cn_node
    )
{
    return (p_cn_node->d_observe_flag == WILDDOG_Conn_Observe_Notif);
}
STATIC int _wilddog_conn_isObserver
    (
    Wilddog_Conn_Node_T *p_cn_node
    )
{
    return ((p_cn_node->d_observe_flag & WILDDOG_Conn_Observe_Req) == WILDDOG_Conn_Observe_Req);
}

/*  alloc memory to memcpy store arg so u have to free while used it */
STATIC Wilddog_Payload_T *_wilddog_conn_allocStoreArg(
    Wilddog_Store_Cmd_T cmd,
	Wilddog_Conn_T *p_conn
    )
{
    Wilddog_Payload_T *p_payload = NULL;
    u8 *p_buf = NULL;
	
    p_conn->p_conn_repo->p_rp_store->p_se_callback(
							p_conn->p_conn_repo->p_rp_store,cmd,&p_buf,0);
    if(p_buf == NULL)   
        return NULL;
	
    p_payload = wmalloc(sizeof(Wilddog_Payload_T));
    if(p_payload == NULL )
        return NULL;

	p_payload->d_dt_len = strlen((const char *)p_buf);  
    p_payload->p_dt_data = wmalloc(p_payload->d_dt_len);
	if(p_payload->p_dt_data)
    	memcpy(p_payload->p_dt_data,p_buf,p_payload->d_dt_len);
	
	return p_payload;	
}
STATIC INLINE void _wilddog_conn_node_addlist
    (
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *nodeadd
    )
{
    LL_APPEND(p_conn->p_conn_node_hd,nodeadd);
}
STATIC void _wilddog_conn_regsterTime
	(
	Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_cn_node
    )
{
	if( _wilddog_conn_auth_get(p_conn) ==  WILDDOG_CONN_AUTH_AUTHING )
	{
		p_cn_node->d_cn_regist_tm = WILDDOG_RETRANSMITE_TIME \
			+ _wilddog_getTime();
		p_cn_node->d_cn_nextsend_tm = WILDDOG_RETRANSMITE_TIME \
			+FIRSTRTRANSMIT_INV + _wilddog_getTime();
	}
	else
	{
		p_cn_node->d_cn_regist_tm=  _wilddog_getTime();
		p_cn_node->d_cn_nextsend_tm = FIRSTRTRANSMIT_INV + _wilddog_getTime();
	}
	p_cn_node->d_cn_retansmit_cnt = 1;
}
STATIC void _wilddog_conn_updataRegistertime
	( 
	Wilddog_Conn_T *p_conn
	)
{
	
	Wilddog_Conn_Node_T *cur,*tmp;
   
   	LL_FOREACH_SAFE(p_conn->p_conn_node_hd,cur,tmp)
   	{
		_wilddog_conn_regsterTime(p_conn,cur);
	}

}

STATIC int _wilddog_conn_node_add
    (
    Wilddog_Conn_Cmd_T cmd ,
    Wilddog_ConnCmd_Arg_T *p_arg,
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
    (*pp_conn_node)->p_cn_pkt = NULL;
    
    (*pp_conn_node)->f_cn_callback = p_arg->p_complete;
    (*pp_conn_node)->p_cn_cb_arg = p_arg->p_completeArg;
	
    _wilddog_conn_regsterTime(p_conn,*pp_conn_node);
    wilddog_debug_level(WD_DEBUG_LOG,"conn AddNode=%p\n",(*pp_conn_node));
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
        
        wilddog_debug_level(WD_DEBUG_WARN,\
			"conn remove node =%p\n",*pp_conn_node);
#if 1
        if( (*pp_conn_node)->p_cn_pkt )
        {
         	_wilddog_conn_pkt_free(&((*pp_conn_node)->p_cn_pkt));
		}
#endif       	
        wfree(*pp_conn_node);
        *pp_conn_node = NULL;
    }
}
STATIC void _wilddog_conn_setReObserverFlag
	(
	Wilddog_Conn_T *p_conn,
	BOOL state
	)
{ 
	p_conn->d_reObserver_flag = state;
}
STATIC INLINE u8 _wilddog_conn_doReObserverFlag
	(
	Wilddog_Conn_T *p_conn
	)
{
	return p_conn->d_reObserver_flag;
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
            _wilddog_conn_node_remove(p_conn,&p_cur);   
            return ;
        }
    }

}

STATIC int _wilddog_conn_sendMixhandle(
    Wilddog_ConnCmd_Arg_T *p_arg,
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_conn_node)
{
    int res =0;
    
    switch(p_conn_node->d_cmd)
    {
        case WILDDOG_CONN_CMD_ON:
			_wilddog_conn_observeFlagSet(WILDDOG_Conn_Observe_Req,p_conn_node);
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
	if(cmd == WILDDOG_CONN_CMD_AUTH )	
		_wilddog_conn_auth_set(p_conn,WILDDOG_CONN_AUTH_AUTHING,NULL);
    else 
	if( _wilddog_conn_auth_get(p_conn) != WILDDOG_CONN_AUTH_AUTHED)
            return 0;
	
    return _wilddog_conn_pkt_send((u8 *)&p_conn->d_wauth,p_pkt);
}
STATIC int _wilddog_conn_allocAuthArg
	( 
		Wilddog_Conn_T *p_conn,
	    Wilddog_Conn_PktSend_T *p_sendArg
    )
{
	
	Wilddog_Payload_T *p_payload = NULL;
	p_payload = _wilddog_conn_allocStoreArg(WILDDOG_STORE_CMD_GETAUTH,p_conn);
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
/* get node data */
STATIC int _wilddog_conn_allocNodeData
	(
		Wilddog_Node_T * p_data,
		Wilddog_Conn_PktSend_T *p_sendArg
	)
{
	Wilddog_Payload_T *p_payload = NULL;
	p_payload = _wilddog_node2Payload(p_data);
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
STATIC int _wilddog_conn_creatSendPayload
	(  
		Wilddog_Conn_Cmd_T cmd,
	    Wilddog_Conn_T *p_conn,
		Wilddog_Node_T * p_nodeData,
	    Wilddog_Conn_PktSend_T *p_sendArg
    )
{
	int res = 0;
	
	switch(cmd)
	{
		case WILDDOG_CONN_CMD_AUTH:
			res = _wilddog_conn_allocAuthArg(p_conn,p_sendArg);
			break;			
		default:
			if(p_nodeData)
			{
				res = _wilddog_conn_allocNodeData(p_nodeData,p_sendArg);
			}
			break;
	}
	return res;
}
STATIC void _wilddog_conn_destroySendPayload(Wilddog_Conn_PktSend_T *p_sendArg)
{
	if(p_sendArg)
	{
		if(p_sendArg->p_payload)
		{
			wfree( p_sendArg->p_payload);
			p_sendArg->p_payload = NULL;
		}
	}
}
/* parsing cmd urln  u have to free while used it */
STATIC int _wilddog_conn_allocUrl
		(
			Wilddog_Conn_Cmd_T cmd,
			Wilddog_Conn_T *p_conn,
    		Wilddog_Url_T *p_srcUrl,
    		Wilddog_Url_T **pp_dstUrl
		)
{
	int len;
	
	if( !p_srcUrl )
		return WILDDOG_ERR_INVALID;
	(*pp_dstUrl) = (Wilddog_Url_T*) wmalloc(sizeof(Wilddog_Url_T));
	if( NULL == (*pp_dstUrl) )
		return WILDDOG_ERR_NULL;
	switch(cmd)
	{
		case WILDDOG_CONN_CMD_AUTH:
			(*pp_dstUrl)->p_url_path = (Wilddog_Str_T*)AUTHR_PATH;
			(*pp_dstUrl)->p_url_host =  p_srcUrl->p_url_host;
			(*pp_dstUrl)->p_url_query = NULL;
			break;
			
		case WILDDOG_CONN_CMD_PONG:
			(*pp_dstUrl)->p_url_path = (Wilddog_Str_T*)PONG_PATH;
			(*pp_dstUrl)->p_url_host =	p_srcUrl->p_url_host;
			len = strlen(PONG_QURES);
			(*pp_dstUrl)->p_url_query = wmalloc( len + PONG_NUMBERLEN +1 );
			if( NULL == (*pp_dstUrl)->p_url_query )
			{
				wfree((*pp_dstUrl));
				return WILDDOG_ERR_NULL;
			} 
			p_conn->d_pong_num = ( p_conn->d_pong_num >= PONG_NUMBERMAX)? \
									0:(p_conn->d_pong_num);
			memcpy( (*pp_dstUrl)->p_url_query,PONG_QURES,strlen(PONG_QURES)); 
			(*pp_dstUrl)->p_url_query[len] = 0x30 + p_conn->d_pong_num /10;	
			(*pp_dstUrl)->p_url_query[len+1] = 0x30 + p_conn->d_pong_num %10;
			break;

		default:
			(*pp_dstUrl)->p_url_path =  p_srcUrl->p_url_path;
			(*pp_dstUrl)->p_url_host =  p_srcUrl->p_url_host;
			len = strlen(AUTHR_QURES);
			(*pp_dstUrl)->p_url_query = wmalloc( len + AUTHR_LENINBYTE+ 1 );
			if( NULL == (*pp_dstUrl)->p_url_query )
			{
				wfree((*pp_dstUrl));
				return WILDDOG_ERR_NULL;
			}
			memcpy( (*pp_dstUrl)->p_url_query,AUTHR_QURES,len);   
			_byte2bytestr((u8*)&((*pp_dstUrl)->p_url_query[len]),\
				(u8*)& p_conn->d_wauth,AUTHR_LEN);
			break;
	}
	return WILDDOG_ERR_NOERR;
}
/* free url and query */
STATIC void _wilddog_conn_freeUrl(Wilddog_Url_T **p_url)
{
	if(*p_url)
	{
		if( (*p_url)->p_url_query )
			wfree( (*p_url)->p_url_query);
		(*p_url)->p_url_query = NULL;
		wfree(*p_url);
		*p_url = NULL;
	}
}

STATIC int _wilddog_conn_creatpkt
	(
	Wilddog_Conn_Cmd_T cmd,
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_conn_node,
    Wilddog_ConnCmd_Arg_T *p_arg
	)
{
    int res =0 ;
    Wilddog_Conn_PktSend_T d_conn_send;
	Wilddog_Url_T *p_conn_url;
    /*  illegality input */
    if( !p_arg || !p_arg->p_url || !p_conn )
        return WILDDOG_ERR_INVALID;
	
    /* init */
    memset(&d_conn_send,0,sizeof(Wilddog_Conn_PktSend_T));
    d_conn_send.cmd = cmd;
	res = _wilddog_conn_allocUrl(cmd,p_conn,p_arg->p_url,&p_conn_url);
	if(res < 0 )
		return res;
	/*get payload*/
	res = _wilddog_conn_creatSendPayload(cmd,p_conn,p_arg->p_data,&d_conn_send); 
    if(res < 0) 
    {
		_wilddog_conn_freeUrl( &p_conn_url);
		return res;
	}
	
    /*  creat pkt */
	d_conn_send.p_url = p_conn_url;	
	d_conn_send.p_cn_node = p_conn_node;
	d_conn_send.p_conn = p_conn;
	d_conn_send.f_cn_callback = (Wilddog_Func_T) _wilddog_conn_cb;
	
    res = _wilddog_conn_pkt_creat(&d_conn_send,&p_conn_node->p_cn_pkt);

	_wilddog_conn_freeUrl( &p_conn_url);
    _wilddog_conn_destroySendPayload(&d_conn_send);
	
	return res;

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
    Wilddog_Conn_T *p_conn= NULL;
    Wilddog_Conn_Node_T *p_conn_node = NULL;

    /*  illegality input */
    if( !p_arg || !p_arg->p_url || !p_repo || !p_repo->p_rp_conn )
        return WILDDOG_ERR_INVALID;
    p_conn = p_repo->p_rp_conn;

	/* check server status */
	if(	_wilddog_conn_pongstatus_get(p_conn) == WILDDOG_CONN_OFFLINE && \
		cmd != WILDDOG_CONN_CMD_PONG )
		return WILDDOG_ERR_CLIENTOFFLINE;
	/* malloc conn node  add list */
    res = _wilddog_conn_node_add(cmd,p_arg,p_conn,&p_conn_node);
	if(res < 0) 
        return res;
    /*  creat pkt */
	res  = _wilddog_conn_creatpkt(cmd,p_conn,p_conn_node,p_arg);
    wilddog_debug_level(WD_DEBUG_LOG,"conn get pkt node=%p\n",\
						p_conn_node->p_cn_pkt);
    if( res < 0 )
	{
		_wilddog_conn_node_remove(p_conn,&p_conn_node);  		
		return res;
    }
	/* check auth and reset auth status */
	_wilddog_conn_auth_reset(cmd,p_conn);
    /*  send */
	if(_wilddog_getTime() >= p_conn_node->d_cn_regist_tm )
	{
	    res = _wilddog_conn_sendWithAuth(cmd,p_conn_node->p_cn_pkt,p_conn);
	    if( res < 0 )
	    {	
			_wilddog_conn_node_remove(p_conn,&p_conn_node);  		
	       return res;
	    }
	}
    res = _wilddog_conn_sendMixhandle(p_arg,p_conn,p_conn_node);

    return res;
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

#ifdef WILDDOG_SELFTEST                        
  	ramtest_caculate_peakRam();
#endif
    if(p_cn_node->f_cn_callback)
        p_cn_node->f_cn_callback(p_snapshot,p_cn_node->p_cn_cb_arg,err);

    if(p_snapshot)
        wilddog_node_delete(p_snapshot);
    
    return;
}
STATIC void _wilddog_conn_cb_auth
		(
		Wilddog_Conn_T *p_conn,
    	Wilddog_Conn_Node_T *p_cn_node,
		Wilddog_Conn_RecvData_T *p_cn_recvData
		)
{
	if( p_cn_recvData->d_RecvErr == WILDDOG_HTTP_OK )
	{
		_wilddog_conn_auth_set(p_conn,WILDDOG_CONN_AUTH_AUTHED, \
								  p_cn_recvData->p_Recvdata);
		/*all  request in sending list must be reSend */
		_wilddog_conn_updataRegistertime(p_conn);
	    if(p_cn_node->f_cn_callback)
        	p_cn_node->f_cn_callback(	p_cn_node->p_cn_cb_arg, \
        								p_cn_recvData->d_RecvErr);
  }
  else
  {
	  	_wilddog_conn_auth_set(p_conn,WILDDOG_CONN_AUTH_NOAUTH,NULL);
	  	if(p_cn_recvData->d_RecvErr == WILDDOG_ERR_RECVTIMEOUT )
			_wilddog_conn_pong_resetNextSendTm(p_conn,\
										  PONG_REQUEST_IMMEDIATELY);  
  }

}
/* check server respon  > 500 and no respon */
STATIC void _wilddog_conn_cb_checkServerErr
	(
		Wilddog_Conn_T *p_conn,
		Wilddog_Conn_RecvData_T *p_src_recvData,
		Wilddog_Conn_RecvData_T *p_dst_recvData
	)
{
	 /* handle error**/
    if(RESP_IS_ERR(p_src_recvData->d_RecvErr))
    {
    	/* recv unauth err */
        if(p_src_recvData->d_RecvErr == WILDDOG_HTTP_UNAUTHORIZED)
        {
        	if(_wilddog_conn_auth_get(p_conn) == WILDDOG_CONN_AUTH_AUTHING)
            	_wilddog_conn_auth_set(p_conn,WILDDOG_CONN_AUTH_NOAUTH,NULL);
           	else
            	_wilddog_conn_auth_set(p_conn,WILDDOG_CONN_AUTH_DOAUTH,NULL);
		}
        p_dst_recvData->d_recvlen = 0;
        p_dst_recvData->p_Recvdata = NULL;
    }else
    {
        p_dst_recvData->d_recvlen = p_src_recvData->d_recvlen;
        p_dst_recvData->p_Recvdata = p_src_recvData->p_Recvdata;
    }
    
	p_dst_recvData->d_RecvErr = p_src_recvData->d_RecvErr;
	
	/* no server push off pong request include no response  */
	if( CB_NO_SERVERERR(p_src_recvData->d_RecvErr) )
		_wilddog_conn_pong_resetNextSendTm(p_conn,PONG_REQUESINTERVAL);
}

STATIC int _wilddog_conn_cbDispatch
    (
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_cn_node,
    Wilddog_Conn_RecvData_T *p_cn_recvData
    )
{
    Wilddog_Conn_RecvData_T d_cn_recvData;
	
    wilddog_debug_level( WD_DEBUG_WARN,"conn CB ERROR=%lu \n", \
						 p_cn_recvData->d_RecvErr);
	_wilddog_conn_cb_checkServerErr(p_conn,p_cn_recvData,&d_cn_recvData);
	
    switch(p_cn_node->d_cmd)
    {
        case WILDDOG_CONN_CMD_AUTH:
#ifdef WILDDOG_SELFTEST
        	performtest_tm_getAuthHandle();
#endif
          	_wilddog_conn_cb_auth(p_conn,p_cn_node,&d_cn_recvData);
            break;
        case WILDDOG_CONN_CMD_PONG:
			/* todo */
			_wilddog_conn_pong_cb(p_conn,p_cn_node,&d_cn_recvData);
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
/* coap call while get an respone */
STATIC int _wilddog_conn_cb
	    (
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_cn_node,
    Wilddog_Conn_RecvData_T *p_cn_recvData
    )
{
	int res ;
	
	res = _wilddog_conn_cbDispatch(p_conn,p_cn_node,p_cn_recvData);
	p_conn->d_ralyRecv = _wilddog_getTime();
	if(_wilddog_conn_observeFlagSet(WILDDOG_Conn_Observe_Notif,p_cn_node)== 0)
		_wilddog_conn_node_remove(p_conn,&p_cn_node);
	
	return res;
}
STATIC int _wilddog_conn_timeoutCB
    (
    Wilddog_Conn_T *p_conn,
    Wilddog_Conn_Node_T *p_cn_node
    )
{
    Wilddog_Conn_RecvData_T d_cn_recvData;
    if(_wilddog_conn_auth_get(p_conn) == WILDDOG_CONN_AUTH_AUTHED)
    	d_cn_recvData.d_RecvErr = WILDDOG_ERR_RECVTIMEOUT;
    else
    	d_cn_recvData.d_RecvErr = WILDDOG_ERR_NOTAUTH;
    	
    d_cn_recvData.d_recvlen = 0;
    d_cn_recvData.p_Recvdata = NULL;
    
    return _wilddog_conn_cbDispatch(p_conn,p_cn_node,&d_cn_recvData);
}
STATIC int _wilddog_conn_recv(Wilddog_Conn_T *p_conn)
{
	
    int res =0;
    Wilddog_Conn_RecvData_T d_cn_recvpkt;
    d_cn_recvpkt.d_recvlen= WILDDOG_PROTO_MAXSIZE;
    d_cn_recvpkt.d_RecvErr = 0;
	/*remember it is not thread safty!!!*/
    d_cn_recvpkt.p_Recvdata = _wilddog_conn_mallocRecvBuffer();
    
    if(d_cn_recvpkt.p_Recvdata  == NULL)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
        return WILDDOG_ERR_NULL;
    }
	
    res = _wilddog_conn_pkt_recv(&d_cn_recvpkt);
    _wilddog_conn_freeRecvBuffer(d_cn_recvpkt.p_Recvdata);
    
    return res;
}
STATIC int _wilddog_conn_pingSend
    (
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
    res = _wilddog_conn_pkt_send(NULL,p_pkt);
    _wilddog_conn_pkt_free(&p_pkt);
	wilddog_debug_level( WD_DEBUG_LOG,"send ping %s!",\
						 res >= 0?("Success"):("Failed"));
    return res;
    
}

STATIC int _wilddog_conn_keepLink(Wilddog_Conn_T *p_conn)
{ 
    int res =0 ;
    if( DIFF(p_conn->d_ralySend, _wilddog_getTime()) > WILDDOG_PING_INTERVAL)
    {

        res = _wilddog_conn_pingSend(p_conn);
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
        wilddog_debug_level(WD_DEBUG_LOG,"<><> Timeout\n");
        wilddog_debug_level(WD_DEBUG_LOG, \
                                "start time=%lu;curr time=%lu;max timout=%u", \
                                p_cn_node->d_cn_regist_tm,_wilddog_getTime(), \
                                WILDDOG_RETRANSMITE_TIME);
        _wilddog_conn_timeoutCB(p_conn,p_cn_node);
        /* todo */
        _wilddog_conn_node_remove(p_conn,&p_cn_node);
        return 1;
    }
    return 0;
}

STATIC int _wilddog_conn_reObserver
	(
	Wilddog_Conn_T *p_conn
	)
{
	int res = 0;
	Wilddog_Conn_Node_T *cur,*tmp;
		
	if( _wilddog_conn_doReObserverFlag(p_conn) == TRUE)
	{
		LL_FOREACH_SAFE(p_conn->p_conn_node_hd,cur,tmp)
		{
			if(_wilddog_conn_isObserver(cur))
			{
				/* check auth and reset auth status */
				_wilddog_conn_auth_reset(cur->d_cmd,p_conn);
				
			    /*  send */
				_wilddog_conn_observeFlagSet(WILDDOG_Conn_Observe_Req,cur);
				_wilddog_conn_regsterTime(p_conn,cur);
			    res = _wilddog_conn_sendWithAuth( cur->d_cmd,cur->p_cn_pkt,\
			    									p_conn);
			}
		}
		_wilddog_conn_setReObserverFlag(p_conn,FALSE);
	}
	return res;
}
STATIC int _wilddog_conn_retransmit(Wilddog_Conn_T *p_conn)
{
    Wilddog_Conn_Node_T *cur,*tmp;
    int res ;
    u32 curtm = _wilddog_getTime();
    
    LL_FOREACH_SAFE(p_conn->p_conn_node_hd,cur,tmp)
    {
    
        if((_wilddog_conn_isNotify(cur)== 0)&& 
            ((curtm > cur->d_cn_nextsend_tm) || 
            (DIFF(curtm,cur->d_cn_nextsend_tm) > ( 0xffffffff)) ))
            {
                if(_wilddog_conn_retransTimeout(p_conn,cur))
                    continue;
                wilddog_debug_level(WD_DEBUG_WARN,"@@ < ><>< > Retransmit!!");
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
    res = _wilddog_conn_auth_detect(p_repo);
 	res = _wilddog_conn_pong_trysync(p_repo);
	res = _wilddog_conn_reObserver(p_repo->p_rp_conn);
    return res ;
    
}
/* do dtls hanshake */
Wilddog_Conn_T * _wilddog_conn_init(Wilddog_Repo_T* p_repo)
{
    if(!p_repo)
        return NULL;
    Wilddog_Conn_T* p_repo_conn = (Wilddog_Conn_T*)wmalloc(sizeof(Wilddog_Conn_T));
    if(!p_repo_conn)
        return NULL;

    p_repo_conn->p_conn_repo = p_repo;
    p_repo_conn->f_conn_send = (Wilddog_Func_T)_wilddog_conn_send;
    p_repo_conn->f_conn_trysyc = (Wilddog_Func_T)_wilddog_conn_trySync;
    
    p_repo->p_rp_conn = p_repo_conn;

	if(0 == l_initCount)	
	{		 
		_wilddog_conn_initRecvBuffer();		
		_wilddog_conn_pkt_init(p_repo_conn->p_conn_repo->p_rp_url->p_url_host, \
			WILDDOG_PORT);
	}	
	l_initCount++;
#ifdef WILDDOG_SELFTEST    
	performtest_tm_getDtlsHsk();	
 	performtest_star_tm();
#endif    
    _wilddog_conn_auth_send(p_repo);
#ifdef WILDDOG_SELFTEST                            
	ramtest_skipLastmalloc();

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
    if(l_initCount > 0)
    {		
	    l_initCount--;		
	    if(0 == l_initCount)    		
		    _wilddog_conn_pkt_deinit();
    }
    
    p_repo->p_rp_conn->p_conn_node_hd = NULL;
    wfree( p_repo->p_rp_conn);
    p_repo->p_rp_conn = NULL;
    
    return NULL;
}

