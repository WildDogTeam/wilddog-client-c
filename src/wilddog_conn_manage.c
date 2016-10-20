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
 * 0.8.0        lxs       2015-12-23  only manage list and system state.
 *
 */
 
#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <string.h>

#include "utlist.h"
#include "wilddog_config.h"
#include "wilddog_common.h"
#include "wilddog_url_parser.h"
#include "wilddog_ct.h"
#include "wilddog.h"
#include "wilddog_store.h"
#include "wilddog_debug.h"
#include "wilddog_payload.h"

#include "wilddog_conn.h"
#include "wilddog_conn_manage.h"
#include "wilddog_sec.h"

#include "test_lib.h"

/*link config */
#define TEST_LINK_LOG_EN    (0)    

#define _CM_TOKEN_SHORT 's'
#define _CM_TOKEN_LONG 'l'
#define _CM_TOKEN_SHORT_LEN ( strlen( _CM_AUTHR_QURES ) + 8)
#define _CM_TOKEN_LONG_LEN  ( 32 + 1)

#define _GETBYTE_H(b)   ((b>>4)&0x0f)        
#define _GETBYTE_L(b)   (b&0x0f)
#define DIFF(a,b)       ((a>b)?(a-b):(b-a))
#define _CM_MS  (1000)

#define PONG_QURES  "seq="
#define PONG_NUMBERMAX  (98)
#define PONG_NUMBERLEN  (2)
#define PONG_REQUESINTERVAL  (12*60*1000)

/* auth timeout need to pong immediately*/
#define PONG_REQUEST_IMMEDIATELY    (1000)
#define _CM_RECV_SERVER_ERROR(err)   ( err >= 400)

/* retransmit cover (FIRSTRTRANSMIT_INV**n) */
#define _PAKGE_RETRANSMIT_TIME   (2000)
#define WILDDOG_PING_INTERVAL 60000 
#define _CM_MAXAGE_TIME         ( 360*60)

#define _CM_NEXTSENDTIME_SET(ctime,cnt) (ctime+(_PAKGE_RETRANSMIT_TIME<<(cnt++)))

#define _CM_SYS_PING_SHORTTOKEN_PATH   "/.ping"
#define _CM_SYS_PING_LONGTOKEN_PATH   "/.rst"
#define _CM_SYS_RECONNECT_TIME  (2)

#define _CM_SYS_STEP_SEC    ( 9 )
#define _CM_SYS_INTERVALINIT_SEC (20 )   
#define _CM_SYS_KEEPOFFLINE     (3)
#define _CM_SYS_PINGRETRACETIME_SEC	(10)
#define _CM_SYS_OFFLINE_PINGTM_SEC (3*60)
#define _CM_SYS_SERVER_KEEPSESSION_SEC   (168)
#define _CM_SYS_PING_INTERVAL_MIN_SEC	(10)
#define _CM_SYS_PING_INTERVAL_MAX_SEC ((_CM_SYS_SERVER_KEEPSESSION_SEC) - \
                            (WILDDOG_RETRANSMITE_TIME/(_CM_MS)))


typedef enum CM_NODE_TYPE{
    CM_NODE_TYPE_OBSERVER = 1,
    CM_NODE_TYPE_MAX
}CM_Node_Type;
typedef enum _CM_EVENT_TYPE{
    _CM_EVENT_TYPE_NULL,
    _CM_EVENT_TYPE_REONLINE,
    _CM_EVENT_TYPE_MAX    
}_CM_Event_Type;

typedef enum _CM_SYS_PING_TYPE_T{
    _CM_SYS_PINGTYPE_SHORT,
    _CM_SYS_PINGTYPE_LONG
}_CM_SYS_PING_TYPE_T;

typedef struct _CM_SYS_NODE_T{
    struct _CM_SYS_NODE_T *next;
    void *p_ping_pkg;
    _CM_SYS_PING_TYPE_T d_pingType;

    u32 d_token;
    u32 d_ping_sendTm;
    u32 d_ping_registerTm;
    Wilddog_Cm_List_T *p_cm_l;

    u8 d_userOffLine;
    u8 d_sendCnt;
    u8 d_intervalTm;
    u8 d_stepTm;
    u8 d_offLineCnt;    
    u8 d_onlineState;
    BOOL d_disableLink;
#if TEST_LINK_LOG_EN 
    u8 d_long_pingCont;
#endif
}_CM_SYS_Node_T;

typedef struct CM_CONTROL_T
{
    Wilddog_Cm_List_T *p_cm_l_hd;
    _CM_SYS_Node_T *p_cmsys_n_hd;
    Wilddog_Func_T f_cn_callBackHandle;
    
    int d_messageId;
    u8 d_list_cnt;
    u8 d_cm_onlineEvent;
}CM_Control_T;

STATIC u16 d_user_node_num = 0;

CM_Control_T *p_l_cmControl = NULL;

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_node_destory
    (
    Wilddog_CM_Node_T **pp_dele
    );

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_session_maintian
    (
    Wilddog_Cm_List_T *p_cm_l
    );
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_session_saveToken
    (
    Wilddog_Payload_T *p_recvData,
    Wilddog_Cm_List_T *p_cm_l
    );

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_node_updataSendTime
    (
    Wilddog_Cm_List_T *p_cm_l,
    Wilddog_CM_Node_T *p_cm_n,
    u32 nextSendTm
    );
STATIC BOOL WD_SYSTEM _wilddog_cm_sys_findNode
    (
    Protocol_recvArg_T *p_recv
    );
STATIC int WD_SYSTEM _wilddog_cm_sys_keeplink(void);
STATIC void* WD_SYSTEM _wilddog_cm_sys_creatPing
    (
    Wilddog_Cm_List_T *p_cm_l,
    _CM_SYS_PING_TYPE_T pingType,
    u32 *p_pkg_token
    );
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_trafficRunOut
    ( 
    Wilddog_Cm_List_T *p_cm_l
    );
STATIC int WD_SYSTEM _wilddog_cm_sys_disablePingLink
    (
    Wilddog_Cm_List_T *p_cm_l,
    BOOL newstate
    );
STATIC _CM_SYS_Node_T *WD_SYSTEM _wilddog_cm_sys_findSysnodeBycml
    (
    Wilddog_Cm_List_T *p_cm_l
    );
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sys_timeInit
    (
    _CM_SYS_Node_T *p_cmsys_n
    );
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sys_setOnLineState
    (
    Wilddog_Cm_List_T *p_cm_l,
    u32 s
    );
STATIC u8 WD_SYSTEM _wilddog_cm_sys_getOnlineState
    (
    Wilddog_Cm_List_T *p_cm_l
    );
STATIC int WD_SYSTEM _wilddog_cm_cmd_getIndex(void *p_arg,int flag);
STATIC u32 WD_SYSTEM _wilddog_cm_cmd_getToken(void *p_arg,int flag);
STATIC Wilddog_CM_Node_T* WD_SYSTEM _wilddog_cm_findObserverNode_byPath
	(
		Wilddog_CM_Node_T *p_node_hd,
		u8 *p_s_path
	);

/*
 * Function:    _wilddog_cm_rand_get
 * Description: Get a rand number
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC INLINE int WD_SYSTEM _wilddog_cm_rand_get(void)
{
    srand(_wilddog_getTime()); 
    return rand();
}

/*
 * Function:    _wilddog_cm_node_creat.
 * Description: malloc node .
 * Input:       p_arg : arg  
 * Output:      N/A.
 * Return:      pointer to the allocation address.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_node_creat
    (
    Wilddog_CM_UserArg_T *p_arg,
    Wilddog_CM_Node_T    **p_cm_node
    )
{
    Wilddog_CM_Node_T *p_newNode = NULL;

	/* 20160712:skylli:: add max queue node judge*/
	if( d_user_node_num > WILDDOG_REQ_QUEUE_NUM )
		return WILDDOG_ERR_QUEUEFULL;
    p_newNode = (Wilddog_CM_Node_T*)wmalloc(sizeof(Wilddog_CM_Node_T));
    if(p_newNode == NULL)
        return WILDDOG_ERR_NULL;
    
    memset(p_newNode,0,sizeof(Wilddog_CM_Node_T));

    p_newNode->cmd = p_arg->cmd;
    p_newNode->d_token = (0xffffffff) & p_arg->d_token;
    p_newNode->p_pkg = p_arg->p_pkg;
    p_newNode->f_userCB = p_arg->f_userCB;
    p_newNode->p_userCB_arg = p_arg->f_userCB_arg;

    p_newNode->d_retransmit_cnt = 0;
    p_newNode->d_registerTm = _wilddog_getTime();
    p_newNode->d_sendTm = _wilddog_getTime();
    
    if( p_arg->p_path )    
    {
        int tmpLen = strlen((const char*)p_arg->p_path) +1;
        p_newNode->p_path = wmalloc(tmpLen);
        if(p_newNode->p_path == NULL)
            return WILDDOG_ERR_NULL;
        memset(p_newNode->p_path,0,tmpLen);
        memcpy(p_newNode->p_path,p_arg->p_path,(tmpLen-1));
    }
    /* node type subscription.*/
    if( p_arg->cmd == WILDDOG_CONN_CBCMD_ON)
    {
        p_newNode->d_nodeType = CM_NODE_TYPE_OBSERVER;
	
        p_newNode->reObserver_flg = FALSE;
        p_newNode->d_subscibe_index = 0;
    }

    wilddog_debug_level(WD_DEBUG_LOG,"conn_manage:: creat cm node : %p \n",p_newNode);
	/* add */
	d_user_node_num++;
	*p_cm_node = p_newNode;
    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_node_destory
 * Description: free node.
 * Input:       pp_dele:address of delete pointer.
 * Output:      N/A
 * Return:      Wilddog_Return_T type 
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_node_destory
    (
    Wilddog_CM_Node_T **pp_dele
    )
{
    Wilddog_CM_Node_T *p_dele = NULL;
    if( pp_dele == NULL || *pp_dele == NULL)
        return 0;

    p_dele = *pp_dele;
    wilddog_debug_level(WD_DEBUG_LOG,"conn_manage:: destory node :%p",p_dele);
   
    if(p_dele->p_pkg)
        _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_dele->p_pkg,0);
    p_dele->p_pkg = NULL;
    
    if(p_dele->p_path)
        wfree(p_dele->p_path);
    p_dele->p_path = NULL;
    
    wfree(p_dele);
    *pp_dele = NULL;
    d_user_node_num = ( d_user_node_num == 0 )?0:(d_user_node_num-1);
    return 0;    
}
/*
 * Function:    _wilddog_cm_ndoe_isNotify
 * Description: judge if node was an observer and have receive and ack.
 * Input:       p_cm_n: cm node
 * Output:      N/A
 * Return:      BOOL type .
*/
STATIC BOOL WD_SYSTEM _wilddog_cm_ndoe_isNotify
    (
    Wilddog_CM_Node_T *p_cm_n
    )
{
    if( p_cm_n->d_nodeType == CM_NODE_TYPE_OBSERVER && \
        p_cm_n->d_subscibe_index > 0)
        return TRUE;
    else
        return FALSE;
}
/*
 * Function:    _wilddog_cm_node_updataSendTime
 * Description: updata node send time.
 * Input:       p_cm_n: cm node.
 *                 p_cm_l : list pointer.
 *                 nextSendTm: send time in milisecond.
 * Output:      N/A
 * Return:      Wilddog_Return_T type .
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_node_updataSendTime
    (
    Wilddog_Cm_List_T *p_cm_l,
    Wilddog_CM_Node_T *p_cm_n,
    u32 nextSendTm
    )
{
    /* set time.*/
    p_cm_n->d_sendTm = nextSendTm;//
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_cm_list_destory
 * Description: free list that belong some repo.
 * Input:      
 *                 p_cm_l : list pointer.
 * Output:      N/A
 * Return:      Wilddog_Return_T type .
*/
STATIC int WD_SYSTEM   _wilddog_cm_list_destory
    (
    Wilddog_Cm_List_T *p_cm_l
    )
{
    if(p_cm_l == NULL)
        return WILDDOG_ERR_INVALID;
    /* destory cm node hang on this list.*/
    wilddog_debug_level(WD_DEBUG_LOG,"conn_manage:: destory repo linked list : %p",p_cm_l);
    if(p_cm_l->p_cm_n_hd)
    {
        Wilddog_CM_Node_T *curr = NULL,*tmp = NULL;
        LL_FOREACH_SAFE(p_cm_l->p_cm_n_hd,curr,tmp)
        {
            
            LL_DELETE(p_cm_l->p_cm_n_hd,curr);
            _wilddog_cm_node_destory(&curr);
        }
        p_cm_l->p_cm_n_hd = NULL;
    }
    
    wfree(p_cm_l->p_short_token);
    p_cm_l->p_short_token = NULL;
    wfree(p_cm_l->p_long_token);
    p_cm_l->p_long_token = NULL;

    wfree(p_cm_l);

    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_sendWithToken
 * Description: updata token then send out.
 * Input:       p_cm_l:list pointer
 *                 p_send :send out node.   
 * Output:      N/A
 * Return:      Wilddog_Return_T type 
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sendWithToken
    (
    Wilddog_Cm_List_T *p_cm_l,
    Wilddog_CM_Node_T *p_send
    )
{
    /*  updata token.*/
    int res =0;
    Protocol_Arg_Auth_T authArg = {NULL,NULL,0};
    authArg.p_pkg = p_send->p_pkg;
    authArg.p_newAuth = p_cm_l->p_short_token;
    authArg.d_newAuthLen = strlen((char*)p_cm_l->p_short_token);
    _wilddog_protocol_ioctl(_PROTOCOL_CMD_AUTHUPDATA,&authArg,0);
    res = _wilddog_protocol_ioctl(_PROTOCOL_CMD_SEND,p_send->p_pkg,0);
    _wilddog_cm_node_updataSendTime(p_cm_l,p_send,
            _CM_NEXTSENDTIME_SET(_wilddog_getTime(),p_send->d_retransmit_cnt));
    return res;
}
/*
 * Function:    _wilddog_cm_authSend
 * Description: put off that package while unauth,if authed,send out immediately.
 * Input:       p_cm_l:list pointer
 *                 p_send :send out node.   
 * Output:      N/A
 * Return:      Wilddog_Return_T type 
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_authSend
    (
    Wilddog_Cm_List_T *p_cm_l,
    Wilddog_CM_Node_T *p_send
    )
{
    int res = 0;
    switch( p_cm_l->d_authStatus )
    {
        case CM_SESSION_DOAUTH:
        case CM_SESSION_AUTHING:
            /*all application request put off*/
            _wilddog_cm_node_updataSendTime(p_cm_l, \
                                        p_send, \
                                        _CM_NEXTSENDTIME_SET(_wilddog_getTime(),
                                                    p_send->d_retransmit_cnt));
            break;
        case CM_SESSION_UNAUTH:
        case CM_SESSION_AUTHED:
            res = _wilddog_cm_sendWithToken(p_cm_l,p_send);
            _wilddog_cm_node_updataSendTime(p_cm_l,
                    p_send,
                    _CM_NEXTSENDTIME_SET(_wilddog_getTime(),
                    p_send->d_retransmit_cnt));
            break;
    }

    return res;
}
/*
 * Function:    _wilddog_cm_onlineSend.
 * Description:according to it's online statue to send out.
 * Input:       p_cm_l : list head.
 *                 p_cm_n : cm node.                 
 * Output:      N/A.
 * Return:      pointer to the allocation address.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_onlineSend
    (
    Wilddog_Cm_List_T *p_cm_l,
    Wilddog_CM_Node_T *p_cm_n
    )
{
    if( CM_OFFLINE == _wilddog_cm_sys_getOnlineState(p_cm_l))
    {
        /* on event hold in list until system reonline.*/
        if( p_cm_n->cmd == WILDDOG_CONN_CMD_ON)
        {
             _wilddog_cm_node_updataSendTime(p_cm_l, \
                                        p_cm_n, \
                                        (_wilddog_getTime()+_CM_MAXAGE_TIME));
        }
        else
        {
            _wilddog_cm_node_updataSendTime(p_cm_l, \
                    p_cm_n, \
              _CM_NEXTSENDTIME_SET(_wilddog_getTime(),p_cm_n->d_retransmit_cnt));
        }
        return WILDDOG_ERR_NOERR;
    }
    else /* online*/
    {
        return _wilddog_cm_authSend(p_cm_l,p_cm_n); 
    }
}
/*
 * Function:    _wilddog_cm_cmd_authe_delete.
 * Description:  clean all autho node .
 * Input:       Wilddog_Cm_List_T *p_cm_l node list.
 *                  flag : N/A.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_cmd_authe_delete
    (
    Wilddog_Cm_List_T *p_cm_l,
    int flag
    )
{ 
	if(p_cm_l == NULL)
        return WILDDOG_ERR_INVALID;
    /* destory cm node hang on this list.*/
    if(p_cm_l->p_cm_n_hd)
    {
        Wilddog_CM_Node_T *curr = NULL,*tmp = NULL;
        LL_FOREACH_SAFE(p_cm_l->p_cm_n_hd,curr,tmp)
        {
            if( curr->cmd != WILDDOG_CONN_CMD_AUTH )
				continue;
            LL_DELETE(p_cm_l->p_cm_n_hd,curr);
            _wilddog_cm_node_destory(&curr);
        }
    }
    
    return WILDDOG_ERR_NOERR;
}


/*
 * Function:    _wilddog_cm_cmd_userSend.
 * Description:  hang application pakage in to list and send out. 
 * Input:       p_arg: input arg.
 *                  flag : N/A.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_cmd_userSend
    (
    Wilddog_CM_UserArg_T *p_arg,
    int flag
    )
{
    int res = 0;
    /* creat node.*/
    Wilddog_CM_Node_T *p_newNode = NULL;
	res = _wilddog_cm_node_creat(p_arg,&p_newNode);

    if( res != WILDDOG_ERR_NOERR)
        return res;
    /* add to list's head.*/
    LL_PREPEND(p_arg->p_cm_l->p_cm_n_hd,p_newNode);   
    
    if( p_arg->cmd == WILDDOG_CONN_CMD_AUTH)
    {
        /*auth send.*/
         res = _wilddog_protocol_ioctl(_PROTOCOL_CMD_SEND,p_arg->p_pkg,0);
         _wilddog_cm_node_updataSendTime(p_arg->p_cm_l, \
          p_newNode, \
          _CM_NEXTSENDTIME_SET(_wilddog_getTime(),p_newNode->d_retransmit_cnt));
    }else/*application send.*/
        res =  _wilddog_cm_onlineSend(p_arg->p_cm_l,p_newNode);
#if 0   
/* 20160804 :  never return send_to failt .let's try retransmits,we will release it when timeout*/ 
    if( res < 0 )
    {
        LL_DELETE(p_arg->p_cm_l->p_cm_n_hd,p_newNode);
		wfree(p_newNode->p_path);
		p_newNode->p_path = NULL;
		wfree(p_newNode);
		p_newNode = NULL;
        //_wilddog_cm_node_destory(&p_newNode);
    }
#endif	
    /* set auth state*/
    if( p_arg->cmd == WILDDOG_CONN_CMD_AUTH && res >= 0)
        p_arg->p_cm_l->d_authStatus = CM_SESSION_AUTHING;

    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_recv_errorHandle
 * Description:  handle receive error code.
 *               error code = 401, unauthoried then request new session.
 *               error code = 412, flow run out then cancel all observer event.
 *               no error then put off ping send time and reOnline the system.
 * Input:       
 *              p_recv : receive package.
 *              p_cm_l : reponde list.
 * Output:      N/A
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_recv_errorHandle
    (
    Protocol_recvArg_T *p_recv,
    Wilddog_Cm_List_T *p_cm_l
    )
{   
    /* server error.*/
    if( _CM_RECV_SERVER_ERROR(p_recv->err))
    {
        wilddog_debug_level(WD_DEBUG_LOG,"conn_manage:: receive return code: %ld",p_recv->err);
        /* unauth need to send auth request.*/
        if(p_recv->err == WILDDOG_HTTP_UNAUTHORIZED)
        {
            /* session init in next execution cycle.*/
            p_cm_l->d_authStatus = CM_SESSION_DOAUTH;
        }
        else
        if(p_recv->err == WILDDOG_HTTP_PRECONDITION_FAIL)
        {
            /*traffic runout. */
            p_cm_l->d_serverEvent = CM_SERVER_EVENT_PRECONDITION_FAIL; 
        }
    }
    else
    {
        /* set online*/
        if(CM_OFFLINE == _wilddog_cm_sys_getOnlineState(p_cm_l))
        {
            /*set reonline flag.*/
            p_l_cmControl->d_cm_onlineEvent = _CM_EVENT_TYPE_REONLINE;
            /* set online.*/
            _wilddog_cm_sys_setOnLineState(p_cm_l,CM_ONLINE);
        }
        /* normal responds.*/
		/* 20160627 : disable,while notify frequently , server need ping package to keep session.*/
        //_wilddog_cm_sys_timeSkip(p_cm_l);
   }
    
   return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_findObserverNode_byPath.
 * Description: list head and return node who's path ==  p_s_path
 * Input:    p_node_hd: list head.
 *           p_recv_path :  socurce path.
 *           p_recv : receive notify.

 * Output:      N/A.
 * Return:      Wilddog_CM_Node_T node.
*/
STATIC Wilddog_CM_Node_T* WD_SYSTEM _wilddog_cm_findObserverNode_byPath
	(
		Wilddog_CM_Node_T *p_node_hd,
		u8 *p_s_path
	)
{
	u32 len = 0;
	Wilddog_CM_Node_T *curr_cm = NULL,*temp_cm = NULL;
	LL_FOREACH_SAFE(p_node_hd,curr_cm,temp_cm)
	{
		if(curr_cm->p_path == 0 )
			continue;
		len  = (strlen((const char*)curr_cm->p_path) >= strlen((const char*)p_s_path))? \
				strlen((const char*)p_s_path) : strlen((const char*)curr_cm->p_path);
		if(memcmp(p_s_path,curr_cm->p_path,len) == 0 )
			return curr_cm;
	}
	return NULL;
}
STATIC Wilddog_CM_Node_T* WD_SYSTEM _wilddog_cm_findObserverNode
	(	Wilddog_CM_FindNode_Arg_T *p_arg,int flag)
{
	return _wilddog_cm_findObserverNode_byPath(p_arg->p_node_hd,p_arg->path);
}
/*
 * Function:    _wilddog_cm_recv_handle_on.
 * Description: handle notify: 
 *                if notify index larger then update node index and notify user.
 *                if notify index less then ignore it.
 *                if error code > 300 then delete it.
 * Input:    p_cm_l: list pointer.
 *           p_cm_n :  cm node.
 *           p_recv : receive notify.
 *           p_cm_recvArg : receive arg.
 *                  flag : N/A.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_recv_handle_on
    (
    Wilddog_Cm_List_T *p_cm_l,
    Wilddog_CM_Node_T *p_cm_n,
    Protocol_recvArg_T *p_recv,
    Wilddog_CM_Recv_T *p_cm_recvArg
    )
{
    u32 nextSendTm = 0;

    if(_CM_RECV_SERVER_ERROR(p_recv->err))
    {
        /*have beed observer,ignore it*/
//20161020 node must be remove since receive error.
#if 0
		if( _wilddog_cm_ndoe_isNotify(p_cm_n) && \
            p_recv->err != WILDDOG_HTTP_PRECONDITION_FAIL)
        {
            return WILDDOG_ERR_NOERR;
        }
#endif		
        /* call user call back.*/
        if(p_l_cmControl->f_cn_callBackHandle)
        {
            p_l_cmControl->f_cn_callBackHandle(p_cm_recvArg->cmd, \
                                               p_cm_recvArg, \
                                               0);
        }
        /* delete it.*/   
        LL_DELETE(p_cm_l->p_cm_n_hd,p_cm_n);
        _wilddog_cm_node_destory(&p_cm_n);
    }
    else
    {
        if( p_cm_n->d_nodeType == CM_NODE_TYPE_OBSERVER && \
            p_recv->d_isObserver == TRUE && \
            p_recv->d_observerIndx > p_cm_n->d_subscibe_index)
        {
        	/*reobserver call back */
			if(p_cm_n->reObserver_flg == TRUE)
			{
				p_cm_n->reObserver_flg = FALSE;
				p_cm_recvArg->err = WILDDOG_ERR_RECONNECT;
			}
             /* call user call back.*/
            if(p_l_cmControl->f_cn_callBackHandle)
            {
                p_l_cmControl->f_cn_callBackHandle(p_cm_recvArg->cmd,\
                                                   p_cm_recvArg, \
                                                   0);
            }
            /* get observer index.*/
            p_cm_n->d_subscibe_index = p_recv->d_observerIndx;
            /* get maxage.*/
            p_cm_n->d_maxAge = p_recv->d_maxAge * 60 ;
            /* updata next send time.*/
            nextSendTm = _wilddog_getTime() + p_recv->d_maxAge;

            /*adjust sending list.*/
            _wilddog_cm_node_updataSendTime(p_cm_l,p_cm_n,nextSendTm);
       }
    }
   
    return  WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_recv_handle.
 * Description: handle all receive package.
 * Input:  p_cm_l : list pointer.
 *         p_cm_n : response node.
 *         p_recv : store parsed receive package.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC int WD_SYSTEM _wilddog_cm_recv_handle
    (
    Protocol_recvArg_T *p_recv,
    Wilddog_CM_Node_T *p_cm_n,
    Wilddog_Cm_List_T *p_cm_l
    )
{
   /* handle error.*/
   Wilddog_Payload_T payload;
   Wilddog_CM_Recv_T cm_recv;
   int res = 0 ;
   
   memset(&payload,0,sizeof(Wilddog_Payload_T));
   memset(&cm_recv,0,sizeof(Wilddog_CM_Recv_T));
   
   if(  p_recv == NULL ||
        p_cm_n == NULL ||
        p_cm_l == NULL
    )
        return WILDDOG_ERR_INVALID;
   
   payload.p_dt_data = p_recv->p_recvData;
   payload.d_dt_len = p_recv->d_recvDataLen;
   payload.d_dt_pos = 0;

   cm_recv.cmd = p_cm_n->cmd;
   cm_recv.err = p_recv->err;
   cm_recv.p_recvData = &payload;
   cm_recv.p_url_path = p_cm_n->p_path;
   cm_recv.f_user_callback = p_cm_n->f_userCB;
   cm_recv.p_user_cb_arg = p_cm_n->p_userCB_arg;
   /* handle error.*/
   _wilddog_cm_recv_errorHandle(p_recv,p_cm_l);
   switch(p_cm_n->cmd)
   {
        case WILDDOG_CONN_CBCMD_AUTH:
            
#ifdef WILDDOG_SELFTEST
            performtest_getHandleSessionResponTime();
#endif         
            _wilddog_cm_session_saveToken(&payload,p_cm_l);
            if(p_l_cmControl->f_cn_callBackHandle)
            {
                p_l_cmControl->f_cn_callBackHandle(cm_recv.cmd,&cm_recv,0);
            }
            LL_DELETE(p_cm_l->p_cm_n_hd,p_cm_n);
            _wilddog_cm_node_destory(&p_cm_n);
            break;
        case WILDDOG_CONN_CBCMD_ON:
            res = _wilddog_cm_recv_handle_on(p_cm_l,p_cm_n,p_recv,&cm_recv);
            break;
        default:
            if(p_l_cmControl->f_cn_callBackHandle)
            {
                p_l_cmControl->f_cn_callBackHandle(cm_recv.cmd,&cm_recv,0);
            }
            LL_DELETE(p_cm_l->p_cm_n_hd,p_cm_n);
            _wilddog_cm_node_destory(&p_cm_n);
            break;
   }
   return res;
}
/*
 * Function:    _wilddog_cm_recv_findContext.
 * Description:  find respond context according to it's token.
 * Input:  p_recv:  receive package.
 *           flag :  N/A.
 * Output:      N/A.
 * Return:      BOOL type.
*/
STATIC BOOL WD_SYSTEM _wilddog_cm_recv_findContext
    (
    Protocol_recvArg_T *p_recv,
    int flag
    )
{
    Wilddog_Repo_T *curr_repo = NULL,*temp_repo = NULL;
    Wilddog_CM_Node_T *p_find = NULL;
    Wilddog_Cm_List_T *p_cm_l = NULL;
    Wilddog_Repo_T **pp_repo = _wilddog_ct_getRepoHead();
    if( pp_repo == NULL )
        return WILDDOG_ERR_INVALID;
    LL_FOREACH_SAFE(*pp_repo,curr_repo,temp_repo)
    {
        if(curr_repo->p_rp_conn == NULL)
            continue ;
        if( curr_repo->p_rp_conn->p_cm_l)
        {
            Wilddog_CM_Node_T *curr_cm = NULL,*temp_cm = NULL;
            LL_FOREACH_SAFE(curr_repo->p_rp_conn->p_cm_l->p_cm_n_hd,\
                    curr_cm,temp_cm)
            {
                if( (curr_cm->d_token & 0xffffffff) == (p_recv->d_token & 0xffffffff))
                {
                   p_find = curr_cm;
                   p_cm_l = curr_repo->p_rp_conn->p_cm_l;
                   
                   break;
                }
            }
        }
    }
    
    if( p_find )
    {
		    /* Observer then find node by path.*/
	   if(	p_recv->p_r_path && 
	   		p_find->d_nodeType == CM_NODE_TYPE_OBSERVER)
	   		p_find = _wilddog_cm_findObserverNode_byPath(p_cm_l->p_cm_n_hd, p_recv->p_r_path);
	   
        _wilddog_cm_recv_handle(p_recv,p_find,p_cm_l);
        return TRUE;
    }
    else/* system node responds.*/
        return _wilddog_cm_sys_findNode(p_recv);
}
STATIC int WD_SYSTEM _wilddog_cm_recv(void)
{
    wilddog_assert(p_l_cmControl,WILDDOG_ERR_INVALID);
        
    return  _wilddog_protocol_ioctl(_PROTOCOL_CMD_RECV,NULL,0);
}
/*
 * Function:    _wilddog_cm_trafficRunOut.
 * Description:  trffic over runs and touch all observer user call back .
 * Input:  p_cm_l: list pointer.
 * Output:      N/A.
 * Return:      BOOL type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_trafficRunOut
    ( 
    Wilddog_Cm_List_T *p_cm_l
    )
{
    wilddog_assert(p_cm_l,WILDDOG_ERR_INVALID);
    
    if(p_cm_l->d_serverEvent == CM_SERVER_EVENT_PRECONDITION_FAIL)
    {
        Wilddog_CM_Node_T *curr = NULL,*tmp = NULL;
        Protocol_recvArg_T trfficRunOut;

        memset(&trfficRunOut,0,sizeof(Protocol_recvArg_T));
        trfficRunOut.err = WILDDOG_HTTP_PRECONDITION_FAIL;
        
        LL_FOREACH_SAFE(p_cm_l->p_cm_n_hd,curr,tmp)
        {
            /* all node touch call back,and destory.*/
           _wilddog_cm_recv_handle(&trfficRunOut,curr,p_cm_l);
        }
    }

    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_transmitTimeOut.
 * Description:  retransmit time out and return -10 to user.
 * Input:  p_cm_l:  list.
 *           p_cm_n :  cm node poniter.
 * Output:      N/A.
 * Return:      BOOL type.
*/
STATIC BOOL WD_SYSTEM _wilddog_cm_transmitTimeOut
    ( 
    Wilddog_CM_Node_T *p_cm_n,
    Wilddog_Cm_List_T *p_cm_l
    )
{
    if(DIFF(p_cm_n->d_registerTm,_wilddog_getTime())> WILDDOG_RETRANSMITE_TIME)
    {
        Protocol_recvArg_T timeOutArg;
        memset(&timeOutArg,0,sizeof(Protocol_recvArg_T));
        timeOutArg.err = WILDDOG_ERR_RECVTIMEOUT;
        _wilddog_cm_recv_handle(&timeOutArg,p_cm_n,p_cm_l); 
        return TRUE;
    }
    
    return FALSE;
}
/*
 * Function:    _wilddog_cm_retransmit.
 * Description:  retransmit user request.
 * Input:  p_cm_l: retransmit list.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_retransmit
    (
    Wilddog_Cm_List_T *p_cm_l
    )
{
    Wilddog_CM_Node_T *curr = NULL,*tmp = NULL;
    u32 currTm = _wilddog_getTime();
    
    if(p_cm_l == NULL)
        return WILDDOG_ERR_INVALID;
    
    LL_FOREACH_SAFE(p_cm_l->p_cm_n_hd,curr,tmp)
    {
        /*successfully observer node not need to retransmit.*/
        if(_wilddog_cm_ndoe_isNotify(curr) == TRUE)
            continue;

        /* send out while touch send time.*/
        if( currTm >curr->d_sendTm && \
            DIFF(currTm,curr->d_sendTm) < (0xffff))
        {
            /* time out node will be dele and not need to retransmit.*/
            if(_wilddog_cm_transmitTimeOut(curr,p_cm_l) == FALSE)
            {
                if(curr->cmd == WILDDOG_CONN_CMD_AUTH)
                {
                    /*auth send.*/

                    _wilddog_protocol_ioctl(_PROTOCOL_CMD_SEND,curr->p_pkg,0);
                    _wilddog_cm_node_updataSendTime(p_cm_l, \
                        curr, \
                        _CM_NEXTSENDTIME_SET(_wilddog_getTime(), \
                        curr->d_retransmit_cnt));
                }
                else
                {
                    _wilddog_cm_onlineSend(p_cm_l,curr);
                    /* put to queue tial.*/
                }
            }

        }
    }

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_cm_reonline_send.
 * Description: send observer.
 * Input:  N/A.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_reonline_send
    (
    Wilddog_Cm_List_T *p_cm_l,
    Wilddog_CM_Node_T *p_send
    )
{
	Wilddog_CM_Send_Ping_Arg_T ping_pkg;  
	ping_pkg.p_pkg = p_send->p_pkg;
	ping_pkg.d_mid = (u16)_wilddog_cm_cmd_getIndex(NULL, 0);
	ping_pkg.d_token = (u32) _wilddog_cm_cmd_getToken(NULL, 0);
	p_send->d_token = ping_pkg.d_token;


	_wilddog_protocol_ioctl( _PROTOCOL_CMD_MODIFY_MIDTOKEN, &ping_pkg, 0);

	return _wilddog_cm_authSend(p_cm_l,p_send);	
}
/*
 * Function:    _wilddog_cm_reOnline.
 * Description: system reonline, retransmit observer request.
 * Input:  N/A.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_reOnline(void)
{
    wilddog_assert(p_l_cmControl,WILDDOG_ERR_INVALID);
    
    if(p_l_cmControl->d_cm_onlineEvent == _CM_EVENT_TYPE_REONLINE)
    {
        /* resend all reobserver node.*/
        Wilddog_Repo_T *head_repo = NULL,*curr_repo = NULL,*tmp_repo = NULL;
        Wilddog_Repo_T **pp_head_repo = _wilddog_ct_getRepoHead();
        if( pp_head_repo == NULL || \
            *pp_head_repo == NULL)
        {
            return WILDDOG_ERR_NOERR;
        }
        head_repo = *pp_head_repo;
        LL_FOREACH_SAFE(head_repo,curr_repo,tmp_repo)
        {
            Wilddog_CM_Node_T *head_n=NULL,*curr_n = NULL,*tmp_n = NULL;
            if( curr_repo->p_rp_conn == NULL || \
                curr_repo->p_rp_conn->p_cm_l == NULL || \
                curr_repo->p_rp_conn->p_cm_l->p_cm_n_hd == NULL)
            {
                    continue ;
            }
            head_n = curr_repo->p_rp_conn->p_cm_l->p_cm_n_hd;
            /* reObserver.*/
            LL_FOREACH_SAFE(head_n,curr_n,tmp_n)
            {
                if(curr_n->d_nodeType == CM_NODE_TYPE_OBSERVER)
                {
                	/* have been notify..*/
					if(curr_n->d_subscibe_index)
						curr_n->reObserver_flg = TRUE;
					
                    /*reset observer index.*/
                    curr_n->d_subscibe_index = 0;
					/* 20160627 UPDATE REGISTER TIME */
					curr_n->d_registerTm = _wilddog_getTime();
					_wilddog_cm_reonline_send(curr_repo->p_rp_conn->p_cm_l,curr_n);
                    //_wilddog_cm_authSend(curr_repo->p_rp_conn->p_cm_l,curr_n);    
                }
            }
        }
        p_l_cmControl->d_cm_onlineEvent = _CM_EVENT_TYPE_NULL;
    }
    return  WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_sessionInit.
 * Description: auth request and set auth state = authing.
 * Input:       N/A.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sessionInit
    (
    Wilddog_Cm_List_T *p_cm_l
    )
{
    void* p_pkg_index = NULL;
    u8 *pbuf = NULL;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;
    Protocol_Arg_Payload_T authData;
    Wilddog_CM_UserArg_T sendArg;
    Protocol_Arg_CountSize_T pkg_sizeCount;

    wilddog_assert(p_cm_l,WILDDOG_ERR_INVALID);
    wilddog_assert(p_cm_l->p_repo,WILDDOG_ERR_INVALID);
    
    memset(&pkg_sizeCount,0,sizeof(Protocol_Arg_CountSize_T));
    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));
    memset(&authData,0,sizeof(Protocol_Arg_Payload_T));
    memset(&sendArg,0,sizeof(Wilddog_CM_UserArg_T));
    /* malloc long and short token .*/
    if( p_cm_l->p_short_token == NULL)
    {
        p_cm_l->p_short_token = (u8*)wmalloc(_CM_TOKEN_SHORT_LEN+1);
        if(p_cm_l->p_short_token == NULL)
            return WILDDOG_ERR_NULL;
    }
    if(p_cm_l->p_long_token == NULL)
    {
        p_cm_l->p_long_token = (u8*)wmalloc(_CM_TOKEN_LONG_LEN+1);
        if(p_cm_l->p_long_token == NULL)
        {
            wfree(p_cm_l->p_short_token);
            return WILDDOG_ERR_NULL;
         }
    }
    /* init session token.*/
    memset(p_cm_l->p_short_token,'0',_CM_TOKEN_SHORT_LEN);
    memset(p_cm_l->p_long_token,'0',_CM_TOKEN_LONG_LEN);
    p_cm_l->p_long_token[_CM_TOKEN_LONG_LEN] = 0;
    p_cm_l->p_short_token[_CM_TOKEN_SHORT_LEN] = 0;
    
    memcpy(p_cm_l->p_short_token,_CM_AUTHR_QURES,strlen(_CM_AUTHR_QURES));
    /* init protocol package.*/
    pkg_arg.cmd = WILDDOG_CONN_CMD_AUTH;

    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_cmd_getIndex(NULL,0);
    pkg_arg.d_token = (u32) _wilddog_cm_cmd_getToken(NULL,0);

    /* get local auth len */
    authData.d_payloadLen = p_cm_l->p_repo->p_rp_store->p_se_callback(
                             p_cm_l->p_repo->p_rp_store,
                             WILDDOG_STORE_CMD_GETAUTH,&pbuf,0);
    authData.p_payload = pbuf;

    /*count package size.*/
    pkg_sizeCount.p_host = p_cm_l->p_repo->p_rp_url->p_url_host;
    pkg_sizeCount.p_path = (u8*)_CM_AUTHR_PATH;
    pkg_sizeCount.d_payloadLen = authData.d_payloadLen;
    pkg_arg.d_packageLen = (int)_wilddog_protocol_ioctl(
        _PROTOCOL_CMD_COUNTSIZE,&pkg_sizeCount,0);
        
    /* creat coap package.*/
    p_pkg_index =(void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == NULL)
        return WILDDOG_ERR_NULL;

    /* add host.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = p_cm_l->p_repo->p_rp_url->p_url_host;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_HOST,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
        goto _CM_AUTH_ERR;

    /* add path.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = _CM_AUTHR_PATH;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_PATH,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
       goto _CM_AUTH_ERR;

    /*add payload.*/
    authData.p_pkg = (void*)p_pkg_index;

    res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_DATA,&authData,0);
    if(res < 0)
        goto _CM_AUTH_ERR;
    
    /*   add list and send out.*/ 
    sendArg.cmd = WILDDOG_CONN_CMD_AUTH;
    sendArg.p_cm_l =  p_cm_l;
    sendArg.d_token = pkg_arg.d_token;
    sendArg.p_pkg = (void*)p_pkg_index;
    if((res = _wilddog_cm_cmd_userSend(&sendArg,0))< 0 )
        goto _CM_AUTH_ERR;

    return res;

_CM_AUTH_ERR:
	wfree( p_cm_l->p_short_token);
	p_cm_l->p_short_token = NULL;
	wfree( p_cm_l->p_long_token);
	p_cm_l->p_long_token = NULL;
    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return res;
    
}
/*
 * Function:    _wilddog_cm_session_maintian.
 * Description: set session state == doauth then send auth request in nex cycle.
 * Input:  p_cm_l : cm list.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_session_maintian
    (
    Wilddog_Cm_List_T *p_cm_l
    )
{
    if( p_cm_l &&
        p_cm_l->d_authStatus == CM_SESSION_DOAUTH)
    	{
			p_l_cmControl->d_cm_onlineEvent = _CM_EVENT_TYPE_REONLINE;
	       	return _wilddog_cm_sessionInit(p_cm_l);
    	}
	else 
        return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_session_saveToken.
 * Description: updata token.
 * Input:       p_cm_l : cm list.
 *              p_recvData :  receive package ,new token in it's payload.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_session_saveToken
    (
    Wilddog_Payload_T *p_recvData,
    Wilddog_Cm_List_T *p_cm_l
    )
{
    Wilddog_Str_T *p_short_token = NULL, *p_long_token = NULL;
    Wilddog_Node_T *p_snapshot = NULL;
    
    if( p_recvData == NULL || \
        p_recvData->p_dt_data == NULL || \
        p_recvData->d_dt_len == 0)
            return WILDDOG_ERR_INVALID;

    p_snapshot = _wilddog_payload2Node(p_recvData);
#ifdef DEBUG_LEVEL
        if(DEBUG_LEVEL <= WD_DEBUG_LOG )
    wilddog_debug_printnode(p_snapshot);
#endif
    /* get  token. */
    if(p_snapshot)
    {
        Wilddog_Node_T *p_child_node = p_snapshot->p_wn_child ;
        Wilddog_Node_T *p_next_node = p_child_node->p_wn_next;

        if( p_child_node && \
            *(p_child_node->p_wn_key) == _CM_TOKEN_SHORT)
        {
            p_short_token = p_child_node->p_wn_value;
        }
        if( p_child_node && \
            *(p_child_node->p_wn_key) == _CM_TOKEN_LONG)
        {
            p_long_token = p_child_node->p_wn_value;
        }
        
        if( p_next_node && \
            *(p_next_node->p_wn_key) == _CM_TOKEN_SHORT)
        {
            p_short_token = p_next_node->p_wn_value;
        }
        if( p_next_node && \
            *(p_next_node->p_wn_key) == _CM_TOKEN_LONG)
        {
            p_long_token = p_next_node->p_wn_value;
        }

        if( p_long_token && p_short_token)
        {   
            memset( p_cm_l->p_long_token ,0,_CM_TOKEN_LONG_LEN);
            sprintf((char*)p_cm_l->p_long_token,"%s",p_long_token);

            memset( p_cm_l->p_short_token ,0,_CM_TOKEN_SHORT_LEN); 
            sprintf((char*)p_cm_l->p_short_token,"%s%s",_CM_AUTHR_QURES,p_short_token);

            wilddog_node_delete(p_snapshot);
        }
        else
        {
            wilddog_node_delete(p_snapshot);
            goto _SET_UNAUTH;
        }    
        p_cm_l->d_authStatus = CM_SESSION_AUTHED;
        return WILDDOG_ERR_NOERR;
    }

_SET_UNAUTH:

   p_cm_l->d_authStatus = CM_SESSION_UNAUTH;
   return WILDDOG_ERR_INVALID;
}
/*
 * Function:    _wilddog_cm_sys_setOnLineState.
 * Description: all repo was offline then system was offline,otherwise online.
 * Input:       p_cm_l : cm list.
 *              s : new state.
 * note:        in mutilple host,system was define online or offline while one 
 *              host was online  another offline ?? while all host was out of 
 *              line then the system was define offline otherwise online.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sys_setOnLineState
    (
    Wilddog_Cm_List_T *p_cm_l,
    u32 s
    )
{
    _CM_SYS_Node_T *p_cmsys_n = NULL;
    /*set host to new state.*/
    p_cmsys_n = _wilddog_cm_sys_findSysnodeBycml(p_cm_l);
    if( p_cmsys_n == NULL)
        return WILDDOG_ERR_NULL;
    /*set new state.*/
    p_cmsys_n->d_onlineState = s;
    /*deal with offline while system online.*/
    if( _wilddog_ct_getOnlineStatus() == CM_ONLINE )
    {
         if(s == CM_OFFLINE)
         {
            u8 changeFlag = 1;
            _CM_SYS_Node_T *curr = NULL,*tmp=NULL;
            LL_FOREACH_SAFE(p_l_cmControl->p_cmsys_n_hd,curr,tmp)
            {
                if(curr->d_onlineState == CM_ONLINE)
                {
                    changeFlag = 0;
                    break;
                }
            }
            if(changeFlag)
                _wilddog_ct_setOnlineStatus(CM_OFFLINE);
         }
    }
    else
        _wilddog_ct_setOnlineStatus(s);

    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_sys_getOnlineState.
 * Description: updata token.
 * Input:       p_cm_l : cm list.
 * Output:      N/A.
 * Return:      u8 type.
*/
STATIC u8 WD_SYSTEM _wilddog_cm_sys_getOnlineState
    (
    Wilddog_Cm_List_T *p_cm_l
    )
{
    _CM_SYS_Node_T *p_cmsys_n = NULL;
    /*set host to new state.*/
    p_cmsys_n = _wilddog_cm_sys_findSysnodeBycml(p_cm_l);
    if(p_cmsys_n == NULL)
        return CM_OFFLINE;
    return p_cmsys_n->d_onlineState;
}
/*
 * Function:    _wilddog_cm_sys_timeInit
 * Description: init sys the time of node.
 * Input:       p_cmsys_n: system node poniter.
 * Output:      N/A
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sys_timeInit
    (
    _CM_SYS_Node_T *p_cmsys_n
    )
{
    p_cmsys_n->d_intervalTm = _CM_SYS_INTERVALINIT_SEC;
    p_cmsys_n->d_stepTm = _CM_SYS_STEP_SEC;

    if( p_cmsys_n->d_pingType != _CM_SYS_PINGTYPE_SHORT)
    {
        if(p_cmsys_n->p_ping_pkg)
        {
            _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY, \
                                    (void*)p_cmsys_n->p_ping_pkg, \
                                    0);
        }
        p_cmsys_n->p_ping_pkg = NULL;
    }
    
    p_cmsys_n->d_pingType = _CM_SYS_PINGTYPE_SHORT;
    p_cmsys_n->d_disableLink = FALSE;

    p_cmsys_n->d_offLineCnt = 0;
    p_cmsys_n->d_ping_sendTm = _wilddog_getTime() + \
                               p_cmsys_n->d_intervalTm * _CM_MS;
    p_cmsys_n->d_ping_registerTm = p_cmsys_n->d_ping_sendTm;

    return WILDDOG_ERR_NOERR;
}
STATIC _CM_SYS_Node_T *WD_SYSTEM _wilddog_cm_sys_findSysnodeBycml
    (
    Wilddog_Cm_List_T *p_cm_l
    )
{
    _CM_SYS_Node_T *curr = NULL,*tmp=NULL;
    
    wilddog_assert(p_cm_l,NULL);
    
    LL_FOREACH_SAFE(p_l_cmControl->p_cmsys_n_hd,curr,tmp)
    {
        if(curr->p_cm_l == p_cm_l)
            return curr;
    }
    return NULL ;
}

/*
 * Function:    _wilddog_cm_sys_setPingControlFlag
 * Description: enable or disable ping controlFlag.
 * Input:       p_recv : receive data.
 * Output:      N/A
 * Return:      Wilddog_Return_T type.
*/
STATIC int WD_SYSTEM _wilddog_cm_sys_disablePingLink
    (
    Wilddog_Cm_List_T *p_cm_l,
    BOOL newstate
    )
{
    int res = WILDDOG_ERR_NULL;
    _CM_SYS_Node_T *p_find = NULL;
    
    wilddog_assert(p_cm_l,WILDDOG_ERR_INVALID);
    
    
    p_find = _wilddog_cm_sys_findSysnodeBycml( p_cm_l );
    if(p_find)
    {
        p_find->d_disableLink= newstate;
        res = WILDDOG_ERR_NOERR;
    }
    return res ;
}

/*
 * Function:    _wilddog_cm_sys_timeStepIncrease
 * Description: increase sys node step by step.
 * Input:       p_cmsys_n: system node poniter.
 * Output:      N/A
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sys_timeStepIncrease
    (
    _CM_SYS_Node_T *p_cmsys_n
    )
{
    if( p_cmsys_n->d_pingType != _CM_SYS_PINGTYPE_SHORT)
    {
        if(p_cmsys_n->p_ping_pkg)
        {
            _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY, \
                                    (void*)p_cmsys_n->p_ping_pkg, \
                                    0);
            
            p_cmsys_n->p_ping_pkg = NULL;

            if(p_cmsys_n->d_stepTm)
            {
                /*not convergence.*/
                p_cmsys_n->d_intervalTm -=  p_cmsys_n->d_stepTm;
                if(p_cmsys_n->d_stepTm == 1)
                {
                    p_cmsys_n->d_stepTm = 0;
					/*  internvaltime retrace retrace.*/
					if(p_cmsys_n->d_intervalTm > _CM_SYS_PINGRETRACETIME_SEC)
						p_cmsys_n->d_intervalTm -= _CM_SYS_PINGRETRACETIME_SEC;
					
				}
				else
                    p_cmsys_n->d_stepTm = p_cmsys_n->d_stepTm /2;
            }
            else
            {   
                /*convergence and nat change*/
                p_cmsys_n->d_stepTm = _CM_SYS_STEP_SEC;
                p_cmsys_n->d_intervalTm -=  p_cmsys_n->d_stepTm;
            }
        }
    }
    else
    {
        p_cmsys_n->d_intervalTm += p_cmsys_n->d_stepTm;
    }

    p_cmsys_n->d_pingType = _CM_SYS_PINGTYPE_SHORT;
    /* step increase.*/    
    if(p_cmsys_n->d_intervalTm >= _CM_SYS_PING_INTERVAL_MAX_SEC)
    {
        p_cmsys_n->d_stepTm = 0;
        p_cmsys_n->d_intervalTm = _CM_SYS_PING_INTERVAL_MAX_SEC;
    }
	else 
		/* mini interval time.*/
	if( p_cmsys_n->d_intervalTm < _CM_SYS_PING_INTERVAL_MIN_SEC)
			p_cmsys_n->d_intervalTm = _CM_SYS_PING_INTERVAL_MIN_SEC;
                                    
    p_cmsys_n->d_ping_registerTm = _wilddog_getTime() + \
                                   p_cmsys_n->d_intervalTm * _CM_MS;
    
    p_cmsys_n->d_ping_sendTm = p_cmsys_n->d_ping_registerTm;
    p_cmsys_n->d_sendCnt = 0;

    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_sys_timeSkip
 * Description: recv user respond and set sys node send time to next cycle.
 * Input:       p_cm_l:  responds list.
 * Output:      N/A
 * Return:      Wilddog_Return_T type.
*/
/* 20160627 : disable,while notify frequently , server need ping package to keep session.*/
#if 0
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sys_timeSkip
    (
    Wilddog_Cm_List_T *p_cm_l
    )
{
    _CM_SYS_Node_T *p_find = NULL;
    
    wilddog_assert(p_cm_l,WILDDOG_ERR_INVALID);

    p_find = _wilddog_cm_sys_findSysnodeBycml(p_cm_l);
    if(p_find)
    {
        p_find->d_ping_registerTm = _wilddog_getTime() + \
                                    p_find->d_intervalTm * _CM_MS;
        
        p_find->d_ping_sendTm = p_find->d_ping_registerTm;
        
        p_find->d_sendCnt = 0;
        return WILDDOG_ERR_NOERR;
    }
    else
        return WILDDOG_ERR_NULL;
}
#endif
/*
 * Function:    _wilddog_cm_sys_timeReset
 * Description: ping request lost or server haven't support.
 * Input:       p_cmsys_n:  ping node pointer.
 * Output:      N/A
 * Return:      Wilddog_Return_T type.
*/

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sys_timeReset
    (
    _CM_SYS_Node_T *p_cmsys_n
    )
{
    if( p_cmsys_n->d_pingType != _CM_SYS_PINGTYPE_LONG)
    {
        if(p_cmsys_n->p_ping_pkg)
        {
            _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY, \
                                    (void*)p_cmsys_n->p_ping_pkg, \
                                    0);
            
            p_cmsys_n->p_ping_pkg = NULL;
        }
    }
    p_cmsys_n->d_pingType = _CM_SYS_PINGTYPE_LONG;
#if TEST_LINK_LOG_EN
    ++(p_cmsys_n->d_long_pingCont);
#endif

    if( p_cmsys_n->d_offLineCnt < _CM_SYS_KEEPOFFLINE)
    {
        p_cmsys_n->d_ping_registerTm = _wilddog_getTime();
        p_cmsys_n->d_ping_sendTm = _wilddog_getTime();
        p_cmsys_n->d_sendCnt = 0;
    }
    else
    {
        p_cmsys_n->d_ping_registerTm = _CM_SYS_OFFLINE_PINGTM_SEC*_CM_MS + \
                                       _wilddog_getTime();
        
        p_cmsys_n->d_ping_sendTm = p_cmsys_n->d_ping_registerTm;
        
        p_cmsys_n->d_sendCnt = 0;
        p_cmsys_n->d_intervalTm = _CM_SYS_INTERVALINIT_SEC;
        p_cmsys_n->d_stepTm = _CM_SYS_STEP_SEC;
        /* set offline.*/
        
        _wilddog_cm_sys_setOnLineState(p_cmsys_n->p_cm_l,CM_OFFLINE);
    }
    
    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_cm_sys_recvHandle
 * Description: handle ping responds.
 * Input:       p_cmsys_n:  ping node pointer.
 *              p_recv : receive data.
 * Output:      N/A
 * Return:      Wilddog_Return_T type.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sys_recvHandle
    (
    Protocol_recvArg_T *p_recv,
    _CM_SYS_Node_T *p_cmsys_n
    )
{
    if( _CM_RECV_SERVER_ERROR(p_recv->err))
    {
        if( p_recv->err == WILDDOG_ERR_RECVTIMEOUT)
            p_cmsys_n->d_offLineCnt++;
        else if(p_recv->err == WILDDOG_HTTP_PRECONDITION_FAIL)
            p_cmsys_n->p_cm_l->d_serverEvent = CM_SERVER_EVENT_PRECONDITION_FAIL; 
        else if( p_recv->err == WILDDOG_HTTP_BAD_REQUEST &&
                 p_cmsys_n->d_pingType == _CM_SYS_PINGTYPE_LONG)
        {
            /* server have been release session resource.*/
            p_cmsys_n->p_cm_l->d_authStatus = CM_SESSION_DOAUTH;     
            /* reset ping time.*/
            _wilddog_cm_sys_timeInit(p_cmsys_n);
            return WILDDOG_ERR_NOERR;
        }
        /* package lost ,init session.*/
        if(p_recv->err == WILDDOG_ERR_RECVTIMEOUT)
        {
#if TEST_LINK_LOG_EN 
            
            wilddog_debug("\t re_connect session\n");
#endif
            _wilddog_sec_reconnect( p_cmsys_n->p_cm_l->p_host, \
                                    WILDDOG_PORT, \
                                    _CM_SYS_RECONNECT_TIME);
        }
        /* err responds.*/
        _wilddog_cm_sys_timeReset(p_cmsys_n);
    }
    else
    {
        /* if already off line then reOnline.*/
        if(CM_OFFLINE == _wilddog_cm_sys_getOnlineState(p_cmsys_n->p_cm_l))
        {
            p_l_cmControl->d_cm_onlineEvent = _CM_EVENT_TYPE_REONLINE;
            _wilddog_cm_sys_setOnLineState(p_cmsys_n->p_cm_l,CM_ONLINE);
        }
        /*reset offline flag.*/
        p_cmsys_n->d_offLineCnt = 0;
        /* normal responds.*/
        _wilddog_cm_sys_timeStepIncrease(p_cmsys_n); 
    }
#if TEST_LINK_LOG_EN 
    wilddog_debug("\t<><>\ttry long ping time : %d",p_cmsys_n->d_long_pingCont);
    wilddog_debug("\t<><>\tinterval time :%d ",p_cmsys_n->d_intervalTm);
    wilddog_debug("\t<><>\tstep time :%d ",p_cmsys_n->d_stepTm);
#endif

    return WILDDOG_ERR_NOERR;   
}
/*
 * Function:    _wilddog_cm_sys_findNode
 * Description: find ping node according to the token.
 * Input:        p_recv : receive data.
 * Output:      N/A
 * Return:      Wilddog_Return_T type.
*/
STATIC BOOL WD_SYSTEM _wilddog_cm_sys_findNode
    (
    Protocol_recvArg_T *p_recv
    )
{
    BOOL res = FALSE;
    _CM_SYS_Node_T *curr = NULL,*tmp=NULL;
    
    wilddog_assert(p_recv,WILDDOG_ERR_INVALID);
    
    LL_FOREACH_SAFE(p_l_cmControl->p_cmsys_n_hd,curr,tmp)
    {
        if((curr->d_token & 0xffffffff) == (p_recv->d_token & 0xffffffff))
        {
            _wilddog_cm_sys_recvHandle(p_recv,curr);
            res = TRUE;
            break;
        }
    }

    return res ;
}

STATIC int WD_SYSTEM _wilddog_cm_sys_pingSend
    (
    _CM_SYS_Node_T *p_cmsys_n
    )
{
    int res = 0;
    Wilddog_CM_Send_Ping_Arg_T ping_pkg;
    if(p_cmsys_n->p_ping_pkg == NULL)
    {
        p_cmsys_n->p_ping_pkg = _wilddog_cm_sys_creatPing(p_cmsys_n->p_cm_l, \
                                                        p_cmsys_n->d_pingType, \
                                                        &(p_cmsys_n->d_token));
        
        if(p_cmsys_n->p_ping_pkg == NULL)
            return  WILDDOG_ERR_NULL;
    }

    if(NULL == p_cmsys_n->p_ping_pkg)
        return -1;
    
    ping_pkg.p_pkg = p_cmsys_n->p_ping_pkg;
    ping_pkg.d_mid = (u16)_wilddog_cm_cmd_getIndex(NULL, 0);
    ping_pkg.d_token = (u32) _wilddog_cm_cmd_getToken(NULL, 0);
    p_cmsys_n->d_token = ping_pkg.d_token;
    
    _wilddog_protocol_ioctl(_PROTOCOL_CMD_MODIFY_MIDTOKEN, &ping_pkg, 0);
    res = _wilddog_protocol_ioctl(_PROTOCOL_CMD_SEND,p_cmsys_n->p_ping_pkg,0);
    p_cmsys_n->d_ping_sendTm = _CM_NEXTSENDTIME_SET(_wilddog_getTime(), \
                                                    p_cmsys_n->d_sendCnt);
    
    return res;
}

STATIC int WD_SYSTEM _wilddog_cm_sys_keeplink(void)
{
    _CM_SYS_Node_T *curr = NULL,*tmp = NULL;
    u32 currTm = _wilddog_getTime();
    
    wilddog_assert(p_l_cmControl,WILDDOG_ERR_INVALID);
    
    LL_FOREACH_SAFE(p_l_cmControl->p_cmsys_n_hd,curr,tmp)
    {
        if( currTm >curr->d_ping_sendTm&& \
            DIFF(currTm,curr->d_ping_sendTm) < (0xffff))
        {
            if(curr->d_disableLink == TRUE)
                continue;
            
            if( DIFF(curr->d_ping_registerTm,currTm) > WILDDOG_RETRANSMITE_TIME )
            {
                /* time out.*/
                Protocol_recvArg_T timeOutArg;
                memset(&timeOutArg,0,sizeof(Protocol_recvArg_T));
                timeOutArg.err = WILDDOG_ERR_RECVTIMEOUT;
                _wilddog_cm_sys_recvHandle(&timeOutArg,curr);
            }
            else /*send out.*/ 
                _wilddog_cm_sys_pingSend(curr);
        }
    }
    return WILDDOG_ERR_NOERR;
}
STATIC int WD_SYSTEM _wilddog_cm_sys_nodeAdd( Wilddog_Cm_List_T *p_cm_l)
{
    _CM_SYS_Node_T *p_cmSys_n = NULL;

    wilddog_assert(p_cm_l,WILDDOG_ERR_INVALID);

    p_cmSys_n = (_CM_SYS_Node_T*)wmalloc(sizeof(_CM_SYS_Node_T));
    if( p_cmSys_n  == NULL)
        return WILDDOG_ERR_NULL;
    
    memset(p_cmSys_n,0,sizeof(_CM_SYS_Node_T));
    
    p_cmSys_n->p_cm_l = p_cm_l;

    wilddog_debug_level(WD_DEBUG_LOG,"conn_manage:: Add system node : %p",p_cmSys_n);

    /*add to list.*/
    LL_APPEND(p_l_cmControl->p_cmsys_n_hd,p_cmSys_n);
    
    _wilddog_cm_sys_timeInit(p_cmSys_n);
    
    return WILDDOG_ERR_NOERR;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sys_nodeRemove
    (
    _CM_SYS_Node_T **pp_cmsys_n
    )
{
    _CM_SYS_Node_T *p_cmsys_n = NULL;

    wilddog_assert(pp_cmsys_n,WILDDOG_ERR_INVALID);
    wilddog_assert(*pp_cmsys_n,WILDDOG_ERR_INVALID);

    p_cmsys_n = *pp_cmsys_n;

    wilddog_debug_level(WD_DEBUG_LOG,"conn_manage::  remove system node : %p",p_cmsys_n);
    if(p_cmsys_n->p_ping_pkg)
        _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_cmsys_n->p_ping_pkg,0);

    /* remove from list*/
    LL_DELETE(p_l_cmControl->p_cmsys_n_hd,p_cmsys_n);
    /*free it.*/
    wfree(p_cmsys_n);

    *pp_cmsys_n = NULL;

    return  WILDDOG_ERR_NOERR;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_cm_sys_releaseRepoResource
    (
    Wilddog_Cm_List_T *p_cm_l
    )
{
    _CM_SYS_Node_T *p_find = NULL;

    wilddog_assert(p_cm_l,WILDDOG_ERR_INVALID);
    p_find = _wilddog_cm_sys_findSysnodeBycml(p_cm_l);
    if(p_find)
    {
        _wilddog_cm_sys_nodeRemove(&p_find);
    }
    return WILDDOG_ERR_NOERR;    
}

STATIC int WD_SYSTEM _wilddog_cm_sys_listDestory(void)
{
    _CM_SYS_Node_T *curr = NULL,*tmp=NULL;
    LL_FOREACH_SAFE(p_l_cmControl->p_cmsys_n_hd,curr,tmp)
    {
        _wilddog_cm_sys_nodeRemove(&curr);
    }
    
    return WILDDOG_ERR_NOERR;
}
/* system ping creat.*/
STATIC void* WD_SYSTEM _wilddog_cm_sys_creatPing
    (
    Wilddog_Cm_List_T *p_cm_l,
    _CM_SYS_PING_TYPE_T pingType,
    u32 *p_pkg_token
    )
{
    void* p_pkg_index = NULL;
    int res = 0;
    Protocol_Arg_Creat_T pkg_arg;
    Protocol_Arg_Option_T pkg_option;
    Protocol_Arg_CountSize_T pkg_payload;
    Wilddog_CM_UserArg_T sendArg;

    memset(&pkg_arg,0,sizeof(Protocol_Arg_Creat_T));
    memset(&pkg_option,0,sizeof(Protocol_Arg_Option_T));
    memset(&pkg_payload,0,sizeof(Protocol_Arg_CountSize_T));
    memset(&sendArg,0,sizeof(Wilddog_CM_UserArg_T));
   
    /* init protocol package.*/
    if( pingType == _CM_SYS_PINGTYPE_SHORT)
    {
        pkg_arg.cmd = WILDDOG_CM_SYS_CMD_SHORTPING;
        pkg_payload.p_path =(u8*)_CM_SYS_PING_SHORTTOKEN_PATH;
        pkg_payload.p_query = p_cm_l->p_short_token;
    }
    else
    {
        pkg_arg.cmd = WILDDOG_CM_SYS_CMD_LONGPING;
        pkg_payload.p_path =(u8*)_CM_SYS_PING_LONGTOKEN_PATH;
        pkg_payload.d_payloadLen = strlen((char*)p_cm_l->p_long_token)+1;
    }
    pkg_payload.p_host = p_cm_l->p_host;
    pkg_arg.d_packageLen=(int)_wilddog_protocol_ioctl(_PROTOCOL_CMD_COUNTSIZE, \
                                                      &pkg_payload,
                                                      0);
    
    /* get messageid */
    pkg_arg.d_index = (u16)_wilddog_cm_cmd_getIndex(NULL,0);
    pkg_arg.d_token = (u32) _wilddog_cm_cmd_getToken(NULL,0);
    

    /* creat coap package.*/
    p_pkg_index =(void*)_wilddog_protocol_ioctl(_PROTOCOL_CMD_CREAT,&pkg_arg,0);
    if( p_pkg_index == NULL)
        return NULL;

    /* add host.*/
    pkg_option.p_pkg = (void*)p_pkg_index;
    pkg_option.p_options = p_cm_l->p_repo->p_rp_url->p_url_host;

    res = _wilddog_protocol_ioctl(_PROTOCOL_CMD_ADD_HOST,&pkg_option,0);
    if( res != WILDDOG_ERR_NOERR )
       goto _CM_CREATPING_ERR;

    /* add token*/
    if( pingType == _CM_SYS_PINGTYPE_SHORT)
    {
        /* add ping path.*/
        pkg_option.p_pkg = (void*)p_pkg_index;
        //p_cm_l->p_repo->p_rp_url->p_url_path
        pkg_option.p_options = _CM_SYS_PING_SHORTTOKEN_PATH;
        res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_PATH,&pkg_option,0);
        if( res != WILDDOG_ERR_NOERR )
           goto _CM_CREATPING_ERR;
        /* add query.*/
        pkg_option.p_options = p_cm_l->p_short_token;
        res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_QUERY,&pkg_option,0);
        if( res != WILDDOG_ERR_NOERR )
            goto _CM_CREATPING_ERR;
    }
    else
    {
        /* add ping path.*/
        Protocol_Arg_Payload_T payloadArg;
        pkg_option.p_pkg = (void*)p_pkg_index;
        pkg_option.p_options = _CM_SYS_PING_LONGTOKEN_PATH;

        res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_PATH,&pkg_option,0);
        if( res != WILDDOG_ERR_NOERR )
            goto _CM_CREATPING_ERR;

        /* add ping token.*/
        payloadArg.p_pkg = (void*)p_pkg_index;
        payloadArg.p_payload = (void*)p_cm_l->p_long_token;
        payloadArg.d_payloadLen = strlen((char*)p_cm_l->p_long_token);
        
        res = _wilddog_protocol_ioctl( _PROTOCOL_CMD_ADD_DATA,&payloadArg,0);
        if(res < 0)
            goto _CM_CREATPING_ERR;

    }
    /* add to system list and send out.*/
    *p_pkg_token = 0xffffffff & pkg_arg.d_token;
    
    return p_pkg_index;

_CM_CREATPING_ERR:

    _wilddog_protocol_ioctl(_PROTOCOL_CMD_DESTORY,(void*)p_pkg_index,0);
    return NULL;
}

/*
 * Function:    _wilddog_get_index
 * Description: get index
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC int WD_SYSTEM _wilddog_cm_cmd_getIndex(void *p_arg,int flag)
{
    int res = 0xffff;
    p_l_cmControl->d_messageId++;
    
    res = res & p_l_cmControl->d_messageId;
    return res;
}
/*
 * Function:    _wilddog_get_index
 * Description: get index
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC u32 WD_SYSTEM _wilddog_cm_cmd_getToken(void *p_arg,int flag)
{
    u32 temtoken = 0;
    temtoken = _wilddog_cm_rand_get();
    return (u32)(0x0 |((temtoken<<8) | (p_l_cmControl->d_messageId & 0xff)));
}
/*
 * Function:    wilddog_cm_shortToken
 * Description: get 
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC INLINE void* WD_SYSTEM _wilddog_cm_cmd_shortToken
    (
    Wilddog_Cm_List_T *p_cm_l,
    int flag
    )
{
    return  (void*)p_cm_l->p_short_token;
}
/*
 * Function:    wilddog_cm_shortToken
 * Description: get 
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC INLINE void* WD_SYSTEM _wilddog_cm_cmd_authQuery
    (
    Wilddog_Cm_List_T *p_cm_l,
    int flag
    )
{
    return (void*)(p_cm_l->p_short_token);
}

/*
 * Function:    wilddog_cm_shortToken
 * Description: get 
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC int WD_SYSTEM _wilddog_cm_cmd_deleNodeByPath
    (
    Wilddog_CM_OffArg_T *p_arg,
    int flag
    )
{
    Wilddog_CM_Node_T *curr = NULL, *tmp = NULL;

    if( p_arg->p_cm_l == NULL)
        return WILDDOG_ERR_INVALID;
    
    LL_FOREACH_SAFE(p_arg->p_cm_l->p_cm_n_hd,curr,tmp)
    {
        if( curr->d_nodeType == CM_NODE_TYPE_OBSERVER &&
            strcmp((const char*)curr->p_path,(const char*)p_arg->p_path) == 0)
        {
            LL_DELETE(p_arg->p_cm_l->p_cm_n_hd,curr);
            _wilddog_cm_node_destory(&curr);
        }
    }  

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    wilddog_cm_offLine
 * Description: get 
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC int WD_SYSTEM _wilddog_cm_cmd_offLine
    (
    Wilddog_Cm_List_T *p_cm_l,
    int flag
    )
{
    wilddog_assert(p_cm_l,WILDDOG_ERR_INVALID);
    /*all host offLine cmd already sended out then we define system offline.*/
    _wilddog_cm_sys_setOnLineState(p_cm_l,CM_OFFLINE);
    return _wilddog_cm_sys_disablePingLink(p_cm_l,TRUE);
}
/*
 * Function:    wilddog_cm_onLine
 * Description: get 
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC int WD_SYSTEM _wilddog_cm_cmd_onLine
    (
    Wilddog_Cm_List_T *p_cm_l,
    int flag
    )
{
    _CM_SYS_Node_T *p_cmsys_n = NULL;
    wilddog_assert(p_cm_l,WILDDOG_ERR_INVALID);
    
    p_cmsys_n = _wilddog_cm_sys_findSysnodeBycml(p_cm_l);
    if(p_cmsys_n)
    {
        p_cmsys_n->p_cm_l->d_authStatus = CM_SESSION_DOAUTH;     
        /* reset ping time.*/
        _wilddog_cm_sys_timeInit(p_cmsys_n);

        return WILDDOG_ERR_NOERR;
    }
    else
        return WILDDOG_ERR_NULL;
}
/*
 * Function:    wilddog_cm_onLine
 * Description: get 
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC int WD_SYSTEM _wilddog_cm_cmd_trySync
    (
    Wilddog_Cm_List_T *p_cm_l,
    int flag
    )
{
    int res = 0;
    res  = _wilddog_cm_retransmit(p_cm_l);
    res = _wilddog_cm_session_maintian(p_cm_l);
    res = _wilddog_cm_recv();
    res = _wilddog_cm_reOnline();
    res = _wilddog_cm_trafficRunOut(p_cm_l);
    res = _wilddog_cm_sys_keeplink();
    return res;
}

/*
 * Function:    _wilddog_cm_cmd_init
 * Description: init session.
 *
 * Input:       p_host: The pointer of the host
 *              d_port: The port number.
 *              f_cn_callBack :  conn call back.
 * Output:      N/A
 * Return:      N/A
*/
Wilddog_Cm_List_T* WD_SYSTEM _wilddog_cm_cmd_init
    (
    Wilddog_CM_InitArg_T *p_arg,
    int flag
    )
{
    int res = 0;
    Protocol_Arg_Init_T d_pkginitArg;
    Wilddog_Cm_List_T *p_cm_l = NULL ;
    
    memset(&d_pkginitArg,0,sizeof(Protocol_Arg_Init_T));

    d_pkginitArg.d_port = WILDDOG_PORT;
    d_pkginitArg.p_host = p_arg->p_repo->p_rp_url->p_url_host;
    d_pkginitArg.f_handleRespond = (Wilddog_Func_T)_wilddog_cm_recv_findContext;

    /*creat list head.*/
    p_cm_l = wmalloc(sizeof(Wilddog_Cm_List_T));
    if( p_cm_l== NULL)
        return NULL;
    
    memset(p_cm_l,0,sizeof(Wilddog_Cm_List_T));
    p_cm_l->p_repo = p_arg->p_repo;
    /*  Auth request.*/
    /*  coap init.*/
    if(p_l_cmControl == NULL)
    {
        p_l_cmControl = (CM_Control_T*)wmalloc(sizeof(CM_Control_T));
        if( p_l_cmControl == NULL)
        {
            wfree(p_cm_l);
            return NULL;
        }
        memset(p_l_cmControl,0,sizeof(CM_Control_T));
        
        p_l_cmControl->p_cm_l_hd = p_cm_l;
        p_l_cmControl->d_messageId = _wilddog_cm_rand_get();
        p_l_cmControl->f_cn_callBackHandle = p_arg->f_conn_cb;
        _wilddog_protocol_ioctl(_PROTOCOL_CMD_INIT,&d_pkginitArg,0);  
    }

#ifdef WILDDOG_SELFTEST    
    performtest_getDtlsHskTime();   
    performtest_timeReset();
#endif 

    p_cm_l->p_host = p_arg->p_repo->p_rp_url->p_url_host;
    /* auth reques and send out.*/
    if((res = _wilddog_cm_sessionInit(p_cm_l))< 0 )
        goto CM_INIT_ERROR;

#ifdef WILDDOG_SELFTEST                            
    ramtest_skipLastmalloc();

    performtest_getSessionQueryTime();
    performtest_timeReset();
#endif 

    res =_wilddog_cm_sys_nodeAdd(p_cm_l);
    if( res < 0 )
        goto CM_INIT_ERROR;
    
    p_l_cmControl->d_list_cnt++;
    wilddog_debug_level(WD_DEBUG_LOG,"conn_manage:: creat repo linked list : %p",p_cm_l);
    return p_cm_l;
CM_INIT_ERROR:

    wfree(p_cm_l);
    if( p_l_cmControl->d_list_cnt == 0)
    {
        _wilddog_protocol_ioctl(_PROTOCOL_CMD_DEINIT,NULL,0); 
        wfree(p_l_cmControl);
    }
    return NULL;
}
/*
 * Function:    _wilddog_cm_cmd_init
 * Description: init session.
 *
 * Input:       p_host: The pointer of the host
 *                d_port: The port number.
 * Output:      N/A
 * Return:      N/A
*/
Wilddog_Return_T WD_SYSTEM _wilddog_cm_cmd_deInit
    (
    Wilddog_Cm_List_T *p_cm_l,
    int flags
    )
{
    if( p_l_cmControl == NULL)
        return WILDDOG_ERR_NULL; 
    
    wilddog_assert(p_cm_l,WILDDOG_ERR_INVALID);
    /* destory list.*/
	
    _wilddog_cm_sys_releaseRepoResource(p_cm_l);
    _wilddog_cm_list_destory(p_cm_l);
    p_l_cmControl->d_list_cnt--;
    
   /* pakg deinit.*/
   if(p_l_cmControl->d_list_cnt == 0)
   {
       _wilddog_cm_sys_listDestory();
       _wilddog_protocol_ioctl(_PROTOCOL_CMD_DEINIT,NULL,0); 
       wfree(p_l_cmControl);
       p_l_cmControl = NULL;
   }

   return   WILDDOG_ERR_NOERR;
}

/* protocol :: coap  interface */
Wilddog_Func_T _wilddog_cm_funcTable[CM_CMD_MAX + 1] = 
{
    (Wilddog_Func_T) _wilddog_cm_cmd_init,        
    (Wilddog_Func_T) _wilddog_cm_cmd_deInit,
    
    (Wilddog_Func_T) _wilddog_cm_cmd_getIndex,
    (Wilddog_Func_T) _wilddog_cm_cmd_getToken,

    (Wilddog_Func_T) _wilddog_cm_cmd_shortToken,
    (Wilddog_Func_T) _wilddog_cm_cmd_authQuery,

    (Wilddog_Func_T) _wilddog_cm_cmd_userSend,
    (Wilddog_Func_T) _wilddog_cm_cmd_deleNodeByPath,

    (Wilddog_Func_T) _wilddog_cm_cmd_offLine,
    (Wilddog_Func_T) _wilddog_cm_cmd_onLine,
    
    (Wilddog_Func_T) _wilddog_cm_cmd_trySync,
    (Wilddog_Func_T) _wilddog_cm_findObserverNode,
    (Wilddog_Func_T) _wilddog_cm_cmd_authe_delete,
    NULL
};
/*
 * Function:    _wilddog_protocol_ioctl
 * Description: the ioctl function  of protocol
 * Input:       cmd:  command
 *              arg: the arg
 *              flags: the flag, not used
 * Output:      N/A
 * Return:      if success, return WILDDOG_ERR_NOERR
*/
size_t WD_SYSTEM _wilddog_cm_ioctl
    (
    u8 cmd,
    void *p_args,
    int flags
    )
{
    if( cmd >= _PROTOCOL_CMD_MAX ||
        cmd < 0)
        return WILDDOG_ERR_INVALID;

    return (size_t)(_wilddog_cm_funcTable[cmd])(p_args,flags);
}

