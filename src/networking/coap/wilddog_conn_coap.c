/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_conn_coap.c
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
#include "wilddog_config.h"
#include "wilddog_conn_coap.h"
#include "wilddog_debug.h"
#include "wilddog_endian.h"
#include "wilddog.h"
#include "wilddog_conn.h"
#include "option.h"
#include "pdu.h"
#include "utlist.h"
#include "wilddog_port.h"
#include "wilddog_sec.h"
#include "wilddog_common.h"

#include "test_lib.h"

#define COAP_TOKENLEN   4
#define DIFF(a,b)   ((a>b)?(a-b):(b-a))
#define WILDDOG_CONN_COAP_RESPON_IGNORE 10	/* Recv repeated respond*/

typedef enum{
    WILDDOG_CONN_COAP_RESP_MATCH,
    WILDDOG_CONN_COAP_RESP_NOMATCH,
}WILDDOG_CONN_COAP_RESP_T;

typedef enum{
    WILDDOG_CONN_COAPPKT_NO_OBSERVER = 0,
    WILDDOG_CONN_COAPPKT_IS_OBSERVER =0x01,
    WILDDOG_CONN_COAPPKT_IS_NOTIFY =0x11,
}WILDDOG_CONN_COAP_OBSERVERFLAG_T;

typedef enum{
    WILDDOG_CONN_COAPPKT_NO_SEPARATE = 0,
    WILDDOG_CONN_COAPPKT_IS_SEPARATE ,
}WILDDOG_CONN_COAP_SEPARATEFLAG_T;

STATIC Wilddog_Conn_Coap_PCB_T *p_coap_pcb = NULL;

STATIC INLINE int _sys_rand_get()
{
    srand(_wilddog_getTime()); 
    return rand();
}

STATIC INLINE unsigned int _wilddog_conn_coap_code2int(unsigned int code) 
{
    unsigned int readable = (code >> 5) * 100 + (code & 0x1F);
    return readable;
}


int _wilddog_conn_coap_code2Http(int rec_code)
{
    switch(rec_code)
    {
        case 201:   return WILDDOG_HTTP_CREATED;
        case 202:   return WILDDOG_HTTP_NO_CONTENT;
        case 203:   return WILDDOG_HTTP_NOT_MODIFIED;
        case 204:   return WILDDOG_HTTP_NO_CONTENT;
        case 205:   return WILDDOG_HTTP_OK;
     
    }
    return rec_code;
}
STATIC int _wilddog_conn_coap_findChar(const char c,const unsigned char *p_buf)
{
	int res = 0 ;
	u32 i;
	for(i=0;p_buf[i] != 0;i++)
	{
		if(p_buf[i] == c)
			res++;
	}
	return res;
}
/*convert cmd to coap type and  code field */
STATIC int _wilddog_conn_coap_cmd2Typecode(Wilddog_Conn_Cmd_T cmd,
															u8 *p_type,u8 *p_code,u32 **pp_observe)
{
    int res = 0;
    switch(cmd){
        case WILDDOG_CONN_CMD_AUTH:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_POST;
            *pp_observe = NULL;
            break;
        case WILDDOG_CONN_CMD_GET:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_GET;
            *pp_observe = NULL;
            break;

        case WILDDOG_CONN_CMD_SET:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_PUT;
            *pp_observe = NULL;
            break;
        case WILDDOG_CONN_CMD_PUSH:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_POST;
            *pp_observe = NULL;
            break;
        case WILDDOG_CONN_CMD_REMOVE:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_DELETE;
            *pp_observe = NULL;
            break;
        case WILDDOG_CONN_CMD_ON:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_GET;
            **pp_observe = 0;
            break;
        case WILDDOG_CONN_CMD_OFF:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_GET;
            **pp_observe = 1;
            break;
        case WILDDOG_CONN_CMD_PING:
            *p_type = COAP_MESSAGE_RST;
            *p_code = 0;
            **pp_observe = 0;
            break;
        default:
            res = WILDDOG_ERR_INVALID;
            break;
    }
    return res;
}
STATIC INLINE int _wilddog_conn_coap_noSeparate(Wilddog_Conn_Coap_PacketNode_T *p_node)
{
    return (p_node->d_separate_flag == WILDDOG_CONN_COAPPKT_NO_SEPARATE);
}
STATIC INLINE int _wilddog_conn_coap_noObserve(Wilddog_Conn_Coap_PacketNode_T *p_node)
{
    return ((p_node->d_observer_flag & WILDDOG_CONN_COAPPKT_IS_OBSERVER) == 0);
}
STATIC INLINE int _wilddog_conn_coap_isNotify(Wilddog_Conn_Coap_PacketNode_T *p_node)
{
    return (p_node->d_observer_flag ==  WILDDOG_CONN_COAPPKT_IS_NOTIFY);
}
/* message equ */
STATIC INLINE int _wilddog_conn_coap_midEqu(coap_pdu_t* p_node,coap_pdu_t* p_resp)
{
    return (p_node->hdr->id != p_resp->hdr->id);
}

/*@  there no Termination err , option  illegality just ignore it . 
 * coap_add_option() destroys the PDU's data, so
 * coap_add_data() must be called after all options have been added.
 * As coap_add_token() destroys the options following the token,
 * the token must be added before coap_add_option() is called.
 * This function returns the number of bytes written or @c 0 on error.
*/
STATIC void _wilddog_conn_coap_pduOptionAdd( coap_pdu_t* p_coap, u8 * host,
                                                            u8* path,u8* uri_query,u32* observe)
{
    /*add host*/
    u8 *p_world_h=NULL;
    u8 *p_world_end=NULL;
    u8 d_obs;
    
    if(host)
        coap_add_option(p_coap, COAP_OPTION_URI_HOST, strlen((const char*)host), host);
    /*add observe*/
    if (observe)
    {
        d_obs = (*observe)?1:0;
        /* todo observer len */
        coap_add_option(p_coap, COAP_OPTION_OBSERVE, 1, &d_obs);
    }
    /* @add path */
    for(p_world_h= path;p_world_h;)
    {
        p_world_h = (u8*)strstr(( char *)p_world_h,"/");
        if(!p_world_h)
            break;
        p_world_h++;
        p_world_end = (u8*)strstr((char *)p_world_h,"/");
        if( !p_world_end )
        {
            coap_add_option(p_coap,COAP_OPTION_URI_PATH,strlen((const char *)p_world_h),p_world_h);
            break;
        }
        else
            coap_add_option(p_coap,COAP_OPTION_URI_PATH,p_world_end-p_world_h,p_world_h);
    
    }
    /*@ add uri query */
    if(uri_query)
        coap_add_option(p_coap, COAP_OPTION_URI_QUERY, strlen((const char *)uri_query), uri_query);
        return;
}

/* host : 4+ host len
* path : n*(4+ subpath)
* query: 4+2+query len
**/
STATIC int _wilddog_conn_coap_countPacktSize(Wilddog_Conn_PktSend_T *p_cp_pkt)
{
	int len = 0,n=0;
	/*option*/
	if(p_cp_pkt->p_url)
	{
		Wilddog_Url_T *p_url = p_cp_pkt->p_url;
		if(p_url->p_url_host)
			len = 4+strlen((const char *)p_url->p_url_host);
		if(p_url->p_url_query)
			len += 6+ strlen((const char *)p_url->p_url_query);
		if(p_url->p_url_path)
		{
			n = _wilddog_conn_coap_findChar('/',p_url->p_url_path);
			len += 4*(n+1)+strlen((const char *)p_url->p_url_path);
			}
		else
			len += 5;
		}
	/* payload add */
	if(p_cp_pkt->d_payloadlen && p_cp_pkt->p_payload)
		len += p_cp_pkt->d_payloadlen;
	/* add had and observer*/
	len += 8+8;
	return len;
}

coap_pdu_t
*_wilddog_conn_coap_pduCreat(   u8 types,u8 codes,u32 *p_observe,
                                            Wilddog_Conn_PktSend_T *p_cp_pkt)
{
    
    u16 index;
    u32 temtoken= 0;
    int d_packetsize = 0;
    coap_pdu_t* p_coap = NULL;
    index = p_coap_pcb->d_pkt_idx++;
    index = htons(index);
    
    d_packetsize = _wilddog_conn_coap_countPacktSize(p_cp_pkt);
    /*@ todo */
    p_coap = coap_pdu_init(types,codes, index,d_packetsize);

    if(!p_coap)
        return NULL;
    
    /*@ add token */
    
    temtoken = _sys_rand_get();
    temtoken = (temtoken<<8) | (p_coap_pcb->d_pkt_idx & 0xff);
    coap_add_token(p_coap, COAP_TOKENLEN, (u8*)&temtoken);
    
    /*@ add option*/
    if(p_cp_pkt->p_url)
    {
        Wilddog_Url_T *p_url = p_cp_pkt->p_url;
        wilddog_debug_level(WD_DEBUG_LOG, \
                    "p_url_host=%s;p_url_path:%s;p_url_query:%s;p_observe=%p\n",
                    p_url->p_url_host,p_url->p_url_path,p_url->p_url_query, \
                    p_observe);
        
        _wilddog_conn_coap_pduOptionAdd(p_coap, p_url->p_url_host, \
                                p_url->p_url_path,p_url->p_url_query,p_observe);    
    }
    /*@ add data*/
    if(p_cp_pkt->d_payloadlen && p_cp_pkt->p_payload)
    {
        wilddog_debug_level(WD_DEBUG_LOG, \
                            "p_coap=%p;d_payloadlen=%lu;p_payload=%p\n",p_coap, \
                                p_cp_pkt->d_payloadlen,p_cp_pkt->p_payload);
        if(coap_add_data( p_coap, p_cp_pkt->d_payloadlen,p_cp_pkt->p_payload) == 0)
        {
            coap_delete_pdu(p_coap);
            return  NULL;
        }
                
    }
    return p_coap;
}
INLINE int _wilddog_conn_coap_node_add
    (
    Wilddog_Conn_Coap_PCB_T *p_pcb,
    Wilddog_Conn_Coap_PacketNode_T *p_node
    )
{
    /* COAP_MESSAGE_CON not need to retransmit */
    if(p_node->p_CoapPkt->hdr->type != COAP_MESSAGE_CON)
        return WILDDOG_ERR_NOERR;
    wilddog_debug_level(WD_DEBUG_LOG,"coap add node=%p\n",p_node);

    if( p_pcb->d_pkt_cnt < WILDDOG_REQ_QUEUE_NUM)
    {
        LL_APPEND(p_pcb->P_hd,p_node);
        p_pcb->d_pkt_cnt++;
    }
    else 
        return WILDDOG_ERR_QUEUEFULL;
        
    return WILDDOG_ERR_NOERR;
}
Wilddog_Conn_Coap_PacketNode_T *_wilddog_conn_coap_node_creat
    (
    Wilddog_Conn_PktSend_T *p_pkt
    )
{
    
    Wilddog_Conn_Coap_PacketNode_T *p_node = NULL;
    coap_pdu_t* p_coap = NULL;
    u8 types = 0xff,codes =0xff;
    u32 d_observe = 0;
    u32 *p_observe = &d_observe;
    
    if(_wilddog_conn_coap_cmd2Typecode(p_pkt->cmd,&types,&codes,&p_observe) < 0)
        return NULL;
        
    p_coap = _wilddog_conn_coap_pduCreat(types,codes,p_observe,p_pkt);
    if(p_coap == NULL)
        return NULL;
    wilddog_debug_level(WD_DEBUG_LOG,"creat node=%p\n",p_coap); 

    /*@ malloc coap pkt node*/
    p_node = wmalloc(sizeof(Wilddog_Conn_Coap_PacketNode_T));
    if( p_node==NULL )
    {
        
        coap_delete_pdu(p_coap);
        return  NULL;
        }
    if( p_observe !=0 && *p_observe == 0)
        p_node->d_observer_flag = WILDDOG_CONN_COAPPKT_IS_OBSERVER;
    p_node->p_CoapPkt = p_coap; 
    
    return p_node;
}
void _wilddog_conn_coap_node_destory
    (
    Wilddog_Conn_Coap_PacketNode_T **p_node
    )
{
    
    wilddog_debug_level(WD_DEBUG_LOG,"coap free node:%p\n",p_node);
    if( *p_node)
    {
        coap_delete_pdu((*p_node)->p_CoapPkt);
        (*p_node)->p_CoapPkt = NULL;
    }
    if(*p_node){
        wfree(*p_node);
        *p_node = NULL;
        }
}
INLINE void _wilddog_conn_coap_node_remove
    (
    Wilddog_Conn_Coap_PCB_T *p_pcb,
    Wilddog_Conn_Coap_PacketNode_T *p_dele
    )
{

    wilddog_debug_level(WD_DEBUG_LOG,"coap remove node:%p\n",p_dele);
    if(p_dele->p_CoapPkt->hdr->type != COAP_MESSAGE_CON)
        return;
        
    p_pcb->d_pkt_cnt = (p_pcb->d_pkt_cnt)?(p_pcb->d_pkt_cnt - 1):0;
    LL_DELETE(p_pcb->P_hd, p_dele);

    return;
}

void _wilddog_conn_pkt_free(void **pp_pkt)
{
    _wilddog_conn_coap_node_remove(p_coap_pcb, \
                                    (Wilddog_Conn_Coap_PacketNode_T*)*pp_pkt);
    if(pp_pkt)
    {
        _wilddog_conn_coap_node_destory((Wilddog_Conn_Coap_PacketNode_T **)pp_pkt);
    }
}

int _wilddog_conn_pkt_creat
    (
    Wilddog_Conn_PktSend_T *p_pktSend,
    void **pp_pkt_creat
    )
{
    Wilddog_Conn_Coap_PacketNode_T *p_ndoe;
    int res =0;
    if(p_pktSend == NULL )
        return WILDDOG_ERR_INVALID;

    p_ndoe = _wilddog_conn_coap_node_creat(p_pktSend);
    if(p_ndoe == NULL)
        return WILDDOG_ERR_NULL;
        
    res = _wilddog_conn_coap_node_add(p_coap_pcb,p_ndoe);
    if(res < 0)
        _wilddog_conn_coap_node_destory(&p_ndoe);
    *pp_pkt_creat = p_ndoe;
    return res;
}
STATIC int _wilddog_conn_coap_auth_updata(u8 *p_auth,coap_pdu_t *p_pdu)
{
    coap_opt_iterator_t d_oi;
    coap_opt_t *p_op = NULL;
    u8 *p_opvalue = NULL;

    /* seek option*/
    p_op = coap_check_option(p_pdu,COAP_OPTION_URI_QUERY,&d_oi);
    if(p_op == NULL)
        return WILDDOG_ERR_NOERR;
    /* pointer to option value*/
    p_opvalue = coap_opt_value(p_op);
    if(p_opvalue == NULL)
        return WILDDOG_ERR_NOERR;
    if(memcmp(p_opvalue,AUTHR_QURES,strlen(AUTHR_QURES)) != 0)
        return WILDDOG_ERR_NOERR;

    _byte2bytestr(&p_opvalue[strlen(AUTHR_QURES)],p_auth,AUTHR_LEN);

    return WILDDOG_ERR_NOERR;
        
}
int _wilddog_conn_coap_send
    (
    u32 d_socketid,
    Wilddog_Address_T *p_remoteAdd,
    u8 *p_auth,
    coap_pdu_t *p_coap
    )
{
    int res =-1;
    
    
    if( p_coap == NULL)
        return WILDDOG_ERR_INVALID;
#ifdef DEBUG_LEVEL
    if(WD_DEBUG_LOG == DEBUG_LEVEL)
        coap_show_pdu(p_coap);
#endif
    if ( d_socketid == 0)
            return WILDDOG_ERR_SOCKETERR;
    wilddog_debug_level(WD_DEBUG_WARN,"ip = %u.%u.%u.%u\n", \
                                        0xff & p_remoteAdd->ip[0], \
                                        0xff & p_remoteAdd->ip[1], \
                                        0xff & p_remoteAdd->ip[2], \
                                        0xff & p_remoteAdd->ip[3]);

    _wilddog_conn_coap_auth_updata(p_auth,p_coap);
    res = _wilddog_sec_send(d_socketid, p_remoteAdd,p_coap->hdr, p_coap->length);
    if (res < 0) 
    {
        coap_delete_pdu(p_coap);
        return WILDDOG_ERR_SENDERR;
    }
    return res;
}
/* do not care url
*/
Wilddog_Return_T _wilddog_conn_pkt_send
    (
    int fd,
    Wilddog_Address_T * addr_in,
    u8 *p_auth,void *p_cn_pkt
    )
{
    /*@ todo  Fragmentation send */
    int res =0;
    Wilddog_Conn_Coap_PacketNode_T *p_node = NULL;
    
    if( p_cn_pkt == NULL )
        return WILDDOG_ERR_NULL;
        
    p_node = (Wilddog_Conn_Coap_PacketNode_T *)p_cn_pkt;
    wilddog_debug_level(WD_DEBUG_LOG,"coap send node =%p pkt = %p \n", \
                                                p_cn_pkt,p_node->p_CoapPkt);
#ifdef WILDDOG_DEBUG
#if DEBUG_LEVEL <= WD_DEBUG_LOG
	coap_show_pdu(p_node->p_CoapPkt);
#endif
#endif

    if(_wilddog_conn_coap_noSeparate(p_node))
        res = _wilddog_conn_coap_send(fd,addr_in,p_auth,p_node->p_CoapPkt);
    
    return res ;
}

STATIC int _wilddog_conn_coap_rstSend
    (
    u32 d_socketid,
    Wilddog_Address_T *p_remoteAdd,
    coap_pdu_t* p_resp
    )
{

    int res;
    unsigned int id = p_resp->hdr->id;
    size_t tkn = p_resp->hdr->token_length;
    unsigned char* tk = p_resp->hdr->token;
    
    coap_pdu_t* toSend = coap_pdu_init(COAP_MESSAGE_RST, 0, id, \
                                            WILDDOG_PROTO_MAXSIZE);
    if (toSend == NULL) 
    {
        wilddog_debug_level(WD_DEBUG_ERROR,"coap_addToken error");
        return WILDDOG_ERR_NULL;
    }
    
    if (!coap_add_token(toSend, tkn, tk))
    {
        wilddog_debug_level(WD_DEBUG_ERROR,"coap_addToken error");
        coap_delete_pdu(toSend);
        return WILDDOG_ERR_OBSERVEERR;

    } 
    res = _wilddog_sec_send(d_socketid, p_remoteAdd,toSend->hdr, toSend->length);   
    coap_delete_pdu(toSend);
    return res;
}

STATIC int _wilddog_conn_coap_ackSend
    (
    u32 d_socketid,
    Wilddog_Address_T *p_remoteAdd,
    coap_pdu_t* resp
    ) 
{
    int returnCode= 0;
    unsigned int id = resp->hdr->id;
    size_t tkl = resp->hdr->token_length;
    unsigned char* tk = resp->hdr->token;

    coap_pdu_t* toSend = coap_pdu_init(COAP_MESSAGE_ACK, 0, id, \
                                                        WILDDOG_PROTO_MAXSIZE);
    coap_add_token(toSend,tkl,tk);
    if (toSend == NULL) 
    {
        wilddog_debug_level(WD_DEBUG_ERROR,"coap_addToken error");
        return WILDDOG_ERR_NULL;
    }
    returnCode = _wilddog_sec_send(d_socketid, p_remoteAdd,toSend->hdr, \
                                    toSend->length);
    
    coap_delete_pdu(toSend);
    return returnCode;

}

STATIC int _wilddog_conn_coap_ack
    (
    u32 d_socketid,
    Wilddog_Address_T *p_remoteAdd,
    WILDDOG_CONN_COAP_RESP_T cmd,
    coap_pdu_t *p_pdu
    )
{
    int res = 0;
    
    wilddog_assert(p_pdu, -1);
    if(p_pdu->hdr->type == COAP_MESSAGE_CON){
        if(cmd == WILDDOG_CONN_COAP_RESP_MATCH)
            res = _wilddog_conn_coap_ackSend(d_socketid,p_remoteAdd,p_pdu);
        else
            res = _wilddog_conn_coap_rstSend(d_socketid,p_remoteAdd,p_pdu);
    }
    return res;
}

STATIC coap_pdu_t *_wilddog_conn_coap_recVerify(u8 *p_buf,u32 buflen)
{
        
    coap_pdu_t* p_resp = NULL;

    p_resp = coap_new_pdu();
    if(!p_resp)
        return NULL;

    /*  is coap packet */
    if( coap_pdu_parse(p_buf,buflen, p_resp) && 
        (p_resp->hdr->version == COAP_DEFAULT_VERSION)) 
            return p_resp;
    else
    {
        coap_delete_pdu(p_resp);
        return NULL;
    }
    return NULL;
}

STATIC int _wilddog_conn_coap_recv_respCheck
    (
    Wilddog_Conn_Coap_PacketNode_T *p_node,
    coap_pdu_t* p_resp
    )
{
    u8* p_rcvtoken = p_resp->hdr->token;
    size_t recvtkl = p_resp->hdr->token_length;
	
    /*  no rst and tokenleng !=0 respon token must be match */
    if(  p_resp->hdr->type != COAP_MESSAGE_RST && \
            p_node->p_CoapPkt->hdr->token_length  )
    {
        if(recvtkl == 0 || \
            (memcmp(p_node->p_CoapPkt->hdr->token,p_rcvtoken,recvtkl) != 0)
            )
            return WILDDOG_ERR_RECVNOMATCH;
    }
    /*  message id check*/
    if( _wilddog_conn_coap_noSeparate(p_node)   && 
        _wilddog_conn_coap_noObserve(p_node)    &&
        _wilddog_conn_coap_midEqu(p_node->p_CoapPkt,p_resp))
            return WILDDOG_ERR_RECVNOMATCH;

    return 0;
}
STATIC int _wilddog_conn_coap_recv_separationRespCheck
    (
    Wilddog_Conn_Coap_PacketNode_T *p_node,
    coap_pdu_t *p_resp
    )
{
    if(p_node->p_CoapPkt->hdr->type == COAP_MESSAGE_CON){
        /* empty message */
        if(p_resp->hdr->code == 0 )
        {
            return WILDDOG_CONN_COAP_RESPON_IGNORE;
            p_node->d_separate_flag = WILDDOG_CONN_COAPPKT_IS_SEPARATE;
        }
    }

    return WILDDOG_ERR_NOERR;
}

STATIC int _wilddog_conn_coap_recv_observCheck
    (
    Wilddog_Conn_Coap_PacketNode_T *p_node,
    coap_pdu_t *p_resp
    )
{
    coap_opt_t *p_op =NULL;
    u8 *p_optionvalue = NULL;
    u8 d_optionlen = 0;
    u32 d_obs_cnt =0;
    coap_opt_iterator_t d_oi;

    if(_wilddog_conn_coap_noObserve(p_node))
        return WILDDOG_ERR_NOERR;

    p_op = coap_check_option(p_resp,COAP_OPTION_OBSERVE,&d_oi);
    if(p_op)
    {
        d_optionlen = coap_opt_length(p_op);
        if(d_optionlen)
        {
            p_optionvalue = coap_opt_value(p_op);
            memcpy((u8*)&d_obs_cnt,p_optionvalue,d_optionlen);
            wilddog_debug_level(WD_DEBUG_WARN, \
                                    "d_obs_cnt = %ld ,node_cnt = %ld \n",\
                                    d_obs_cnt,p_node->d_observer_cnt);
            
            if( d_obs_cnt > p_node->d_observer_cnt ||
                DIFF(d_obs_cnt,p_node->d_observer_cnt)> 250)
            {
                p_node->d_observer_flag = WILDDOG_CONN_COAPPKT_IS_NOTIFY;
                p_node->d_observer_cnt = d_obs_cnt;
                /* remove  notify call back */
                return WILDDOG_ERR_NOERR;
            }
            else
                return WILDDOG_CONN_COAP_RESPON_IGNORE;    
        }
    }
    else 
        return WILDDOG_ERR_OBSERVEERR;
    return WILDDOG_ERR_NOERR;
}


/* check coap packer   if match call user callback function  */
STATIC int _wilddog_conn_coap_recvDispatch
    (
    Wilddog_Conn_Coap_PCB_T *p_pcb,
    coap_pdu_t* p_resp,
    Wilddog_Conn_RecvData_T *p_cpk_recv,
    void **pp_cn_pkt
    ) 
{
    int res =-1;
    Wilddog_Conn_Coap_PacketNode_T *tmp = NULL;
    Wilddog_Conn_Coap_PacketNode_T *curr = NULL;

    u8 *p_buftemp = NULL;
    size_t tmplen =0;
    if(!p_pcb->P_hd)
        return WILDDOG_ERR_INVALID;

    LL_FOREACH_SAFE((p_pcb->P_hd),curr,tmp)
    {
        /* mesage id and token check*/
        res = _wilddog_conn_coap_recv_respCheck(curr,p_resp);
        if(res < 0)
            continue ; 
            
        /*@ get observer indx */
        res = _wilddog_conn_coap_recv_observCheck(curr,p_resp);
        
        if(res == WILDDOG_CONN_COAP_RESPON_IGNORE)
            return WILDDOG_ERR_NOERR;
        else if(res == WILDDOG_ERR_OBSERVEERR)
            return WILDDOG_ERR_OBSERVEERR;
            
        /*@ Separation of reply  do not remove */
        if( _wilddog_conn_coap_recv_separationRespCheck(curr,p_resp) \
            == WILDDOG_CONN_COAP_RESPON_IGNORE )
            return WILDDOG_ERR_NOERR;
        /*
        **@ todo Fragmentation
        **@ get payload
        */
        *pp_cn_pkt = curr;
        coap_get_data(p_resp,&tmplen,&p_buftemp);
        p_cpk_recv->d_recvlen = tmplen;
        if(p_buftemp != NULL)
        {
            memcpy(p_cpk_recv->p_Recvdata,p_buftemp,p_cpk_recv->d_recvlen);
        }
            
        /* get error code */
        p_cpk_recv->d_RecvErr = _wilddog_conn_coap_code2Http( \
                                _wilddog_conn_coap_code2int(p_resp->hdr->code));
        
        /* todo handle err*/
        /*@ dele pkt node */
        if(_wilddog_conn_coap_noObserve(curr))
            _wilddog_conn_pkt_free((void**)&curr);
        return WILDDOG_ERR_NOERR;
    }
    return WILDDOG_ERR_NULL;
}

Wilddog_Return_T _wilddog_conn_pkt_recv
    (
    u32 fd,
    Wilddog_Address_T * addr_in,
    void **pp_cn_pkt,
    Wilddog_Conn_RecvData_T *p_cpk_recv
    )
{
    int recv_size,res =-1;

    coap_pdu_t *p_pdu = NULL;
    u8 p_buf[WILDDOG_PROTO_MAXSIZE];

	memset(p_buf, 0, WILDDOG_PROTO_MAXSIZE);

    if( fd == 0)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "no fd!");
        return WILDDOG_ERR_SOCKETERR;
    }
    recv_size = _wilddog_sec_recv(fd, addr_in,p_buf, WILDDOG_PROTO_MAXSIZE);
    
    /*@ NO enougth space */

    if( recv_size <= 0 || recv_size  > p_cpk_recv->d_recvlen ) 
        return WILDDOG_ERR_NOERR; 

    /*@  coap verify  malloc */
    p_pdu = _wilddog_conn_coap_recVerify(p_buf,recv_size);
#ifdef WILDDOG_SELFTEST                        
  	ramtest_caculate_peakRam();
#endif
#ifdef WILDDOG_SELFTEST               
		performtest_tm_getRecvDtls();
#endif

    if(p_pdu == NULL)
    {
        res = WILDDOG_ERR_NOERR;
        goto    PKT_RECV_RETURN_;
    }
#ifdef WILDDOG_DEBUG
#if DEBUG_LEVEL <= WD_DEBUG_LOG
	coap_show_pdu(p_pdu);
#endif
#endif

    /* copy payload call cbfunction */
    res = _wilddog_conn_coap_recvDispatch(p_coap_pcb,p_pdu,p_cpk_recv,pp_cn_pkt);
    if(res < 0)
        res = _wilddog_conn_coap_ack(fd, addr_in, \
                                        WILDDOG_CONN_COAP_RESP_NOMATCH,p_pdu);
    else    
        res = _wilddog_conn_coap_ack(fd, addr_in, \
                                        WILDDOG_CONN_COAP_RESP_MATCH,p_pdu);

PKT_RECV_RETURN_:   

    coap_delete_pdu(p_pdu);
    return res;
}
Wilddog_Return_T _wilddog_conn_pkt_init
    (
    int fd, 
    Wilddog_Address_T * addr_in
    )
{
    if(p_coap_pcb == NULL)
    {
        p_coap_pcb = wmalloc(sizeof(Wilddog_Conn_Coap_PCB_T));
        if(p_coap_pcb ==  NULL)
            return WILDDOG_ERR_NULL;
        p_coap_pcb->d_pkt_idx = rand();
        
    }
    
    return  _wilddog_sec_init(fd,addr_in);
}
Wilddog_Return_T _wilddog_conn_pkt_deinit
    (
    int fd, 
    Wilddog_Address_T * addr_in
    )
{
    Wilddog_Conn_Coap_PacketNode_T *curr,*tmp;

    if(p_coap_pcb)
    {
        LL_FOREACH_SAFE( p_coap_pcb->P_hd  ,curr,tmp)
        {
            if(curr)
                _wilddog_conn_pkt_free((void**)&curr);
        }
    }

    if(p_coap_pcb)
    {
        wfree(p_coap_pcb);
        p_coap_pcb = NULL;
    }
    return _wilddog_sec_deinit(fd,addr_in);
}

