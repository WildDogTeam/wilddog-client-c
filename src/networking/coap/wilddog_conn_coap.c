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
 * 0.5.0        lxs       2015-12-22  modify interface.
 */
 
#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "option.h"
#include "wilddog_payload.h"
     
#include "utlist.h"
#include "wilddog_config.h"
#include "wilddog.h"
#include "wilddog_endian.h"
#include "wilddog_conn_coap.h"
#include "wilddog_debug.h"
#include "wilddog_common.h"
#include "wilddog_sec.h"
#include "test_lib.h"

#include "wilddog_conn.h"
#include "wilddog_conn_manage.h"
#include "wilddog_conn_coap.h"

#define AUTHR_LEN   (4)

#define  _GET_COAP_CODE(code)   ((code >> 5) * 100 + (code & 0x1F))

Wilddog_Func_T _wilddog_coap_findRespondNode = NULL ;

typedef struct _WILDDOG_RECV_STRUCT
{   
    u8 data[WILDDOG_PROTO_MAXSIZE]; 
    u8 isused;
}_wilddog_Recv_T;




STATIC _wilddog_Recv_T l_recvData;

extern   int  _byte2bytestr(u8 *p_dst,u8 *p_src,u8 len);


/*LOCK and UNLOCK used for multi thread*/
STATIC INLINE int WD_SYSTEM _wilddog_coap_recvBufLock(int timeout)
{   
    return 0;
}
STATIC INLINE void WD_SYSTEM _wilddog_coap_recvBufUnlock(void)
{   
    return;
}
#if 0
STATIC void WD_SYSTEM _wilddog_coap_initRecvBuffer(void)
{   
    _wilddog_coap_recvBufLock(0);   
    memset(&l_recvData, 0, sizeof(l_recvData));
    _wilddog_coap_recvBufUnlock();  
    return;
}
#endif
/*
 * Function:    _sys_coap_ntol
 * Description: Convert the byte order
 * Input:       src: The pointer of the source byte    
 *              len: The length of the source byte
 * Output:      dst: The pointer of the destination byte
 * Return:      N/A
*/
STATIC INLINE void WD_SYSTEM  _sys_coap_ntol
    (
    u8 *dst,
    const u8 *src,
    const u8 len
    )
{
    u8 i;
    for(i=0;i<len; i++)
    {
        dst[i] = src[len - i -1 ];
    }
}

/*
 * Function:    _wilddog_conn_mallocRecvBuffer
 * Description: conn layer malloc the buffer which used for recv
 *   
 * Input:       N/A
 * Output:      N/A
 * Return:      the buffer's pointer
*/
STATIC u8* WD_SYSTEM _wilddog_coap_mallocRecvBuffer(void)
{   
    u8* buffer = NULL;  
    _wilddog_coap_recvBufLock(0);   
    
    /*TODO: use round-robin queue*/ 
    if(l_recvData.isused == FALSE)  
    {       
        buffer = l_recvData.data;       
        l_recvData.isused = TRUE;       
        memset(buffer, 0, WILDDOG_PROTO_MAXSIZE);   
    }   
    _wilddog_coap_recvBufUnlock();  
    return buffer;
}

/*
 * Function:    _wilddog_conn_freeRecvBuffer
 * Description: conn layer free the buffer which used for recv
 *   
 * Input:       ptr: the buffer's pointer
 * Output:      N/A
 * Return:      N/A
*/
STATIC void WD_SYSTEM _wilddog_coap_freeRecvBuffer(u8* ptr)
{   
    if(!ptr)        
        return;     
    _wilddog_coap_recvBufLock(0);   
    /*TODO: if use round-robin queue, find index by ptr*/   
    if(l_recvData.data == ptr && TRUE == l_recvData.isused) 
    {       
        l_recvData.isused = FALSE;  
    }   
    _wilddog_coap_recvBufUnlock();
}  
/*
 * Function:    _wilddog_coap_code2Http
 * Description: Convert the coap code to http return code
 * Input:       rec_code: The coap code   
 * Output:      N/A
 * Return:      The http return code
*/
unsigned int WD_SYSTEM _wilddog_coap_code2Http(unsigned int rec_code)
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

/*
 * Function:    _wilddog_coap_cmd2Typecode
 * Description: Convert cmd to coap type and  code field
 * Input:       cmd: The conn command
 * Output:      p_type: The pointer of the coap type
 *              p_code: The pointer of the coap code field
 *              pp_observe: The pointer of the observe flag
 * Return:      If success, return 0; else return WILDDOG_ERR_INVALID
*/
STATIC int WD_SYSTEM _wilddog_coap_cmd2Typecode
    (
    u8 cmd,
    u8 *p_type,
    u8 *p_code
    )
{
    int res = 0;
    switch(cmd)
    {
        case WILDDOG_CONN_CMD_AUTH:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_POST;
            break;          
        case WILDDOG_CONN_CMD_GET:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_GET;
            break;

        case WILDDOG_CONN_CMD_SET:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_PUT;
            break;
        case WILDDOG_CONN_CMD_PUSH:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_POST;
            break;
        case WILDDOG_CONN_CMD_REMOVE:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_DELETE;
            break;
        case WILDDOG_CONN_CMD_ON:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_GET;
            break;
        case WILDDOG_CONN_CMD_OFF:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_GET;
            break;
#if 1
        case WILDDOG_CM_SYS_CMD_SHORTPING:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_GET;
            break;
        case WILDDOG_CM_SYS_CMD_LONGPING:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_POST;
            break;
#endif
        case WILDDOG_CONN_CMD_ONDISSET:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_PUT;
            break;
        case WILDDOG_CONN_CMD_ONDISPUSH:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_POST;
            break;
        case WILDDOG_CONN_CMD_ONDISREMOVE:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_DELETE;
            break;
        case WILDDOG_CONN_CMD_CANCELDIS:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_DELETE;
            break;
        case WILDDOG_CONN_CMD_OFFLINE:
            *p_type = COAP_MESSAGE_CON;
            *p_code = COAP_REQUEST_GET;
            break;        
        default:
            res = WILDDOG_ERR_INVALID;
            break;
    }
    return res;
}
/*
 * Function:    _wilddog_coap_findChar
 * Description: Find the number of  char 'c' exist in  the string buffer.
 * Input:       c: The char.   
 *              p_buf: The pointer of the string buffer.
 * Output:      N/A
 * Return:      The number.
*/
STATIC int WD_SYSTEM _wilddog_coap_findChar
    (
    const char c,
    const unsigned char *p_buf
    )
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
/*
 * Function:    _wilddog_coap_countPacktSize.
 * Description: Count the conn packet size.
 *              Count method:
 *                    host : 4+ host len
 *                    path : n*(4+ subpath)
 *                    query: 4+2+query len
 * Input:       p_cp_pkt: The pointer of conn packet
 * Output:      N/A
 * Return:      The number of the conn packet size
*/
STATIC int WD_SYSTEM _wilddog_coap_countPacktSize
    (
    Protocol_Arg_CountSize_T *p_arg
    )
{
    int len = 0,n=0;
    wilddog_assert(p_arg,0);
    
    /*option*/
    if(p_arg->p_host)
        len = 4+ strlen((const char *)p_arg->p_host);
    
    if(p_arg->p_query)
        len += 6+ strlen((const char *)p_arg->p_query);
    if(p_arg->p_path)
    {
        n = _wilddog_coap_findChar('/',p_arg->p_path);
        len += 4*(n+1)+strlen((const char *)p_arg->p_path);
        }
    else
        len += 5;
    
    /* payload add */
    len += (p_arg->d_payloadLen + 4 );
    
    /* + had + token + observer*/
    len += 8 + 14 + 8;
    
    return len;
}

/*
 * Function:    _wilddog_coap_creat
 * Description: creat an coap package with no option and payload.
 * Input:       p_arg: cmd/message/token/package len. 
 * Output:      N/A
 * Return:      coap pointer.
*/
STATIC size_t WD_SYSTEM _wilddog_coap_creat
    (
    Protocol_Arg_Creat_T *p_arg,
    int flag
    )
{
    coap_pdu_t *p_pdu = NULL;
    u8 type=0,code = 0;
    if(p_arg == NULL)
        return 0;
    /* get coap type and code.*/
    if(_wilddog_coap_cmd2Typecode(p_arg->cmd,&type,&code) < 0)
        return 0;
    /*creat an coap package.*/
    p_pdu = coap_pdu_init(type,code,p_arg->d_index,p_arg->d_packageLen);

    if(p_pdu == NULL)
        return 0;
    /* add token option.*/
    coap_add_token(p_pdu, COAP_TOKENLEN, (u8*)&(p_arg->d_token));

    wilddog_debug_level(WD_DEBUG_LOG,"\tcc\tcreat coap pakge :%p :",p_pdu);
    return ( size_t )p_pdu;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_destory
    (
    void *p_coap,
    int flag
    )
{
    if(p_coap == 0)
        return WILDDOG_ERR_INVALID;
    
    wilddog_debug_level(WD_DEBUG_LOG,"\tcc\tdestory coap pakge :%p :",p_coap);
    coap_delete_pdu((coap_pdu_t*)p_coap);

    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_coap_addHost
 * Description: add host to coap packages.
 * Input:       p_arg: p_coap/p_options. 
 * Output:      N/A
 * Return:      WILDDOG_ERR_NOERR or WILDDOG_ERR_NULL.
*/

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_addHost
    (
    Protocol_Arg_Option_T *p_arg,
    int flag
    )
{
    if( p_arg == NULL || 
        p_arg->p_pkg == NULL ||
        p_arg->p_options == NULL)
        return WILDDOG_ERR_NULL;

    coap_add_option((coap_pdu_t*)p_arg->p_pkg, \
                     COAP_OPTION_URI_HOST, \
                     strlen((const char*)p_arg->p_options), \
                     p_arg->p_options);

    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_coap_addPath
 * Description: add host to coap packages.
 * Input:       p_arg: p_coap/p_options. 
 * Output:      N/A
 * Return:      WILDDOG_ERR_NOERR or WILDDOG_ERR_NULL.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_addPath
    (
    Protocol_Arg_Option_T *p_arg,
    int flag
    )
{
    u8 *p_subpath_h = NULL;
    u8 *p_subpath_end = NULL;
    
    if( p_arg == NULL || 
        p_arg->p_pkg == NULL ||
        p_arg->p_options == NULL)
        return WILDDOG_ERR_NULL;
    
    /* @add path */
    for(p_subpath_h= p_arg->p_options;p_subpath_h;)
    {
        p_subpath_h = (u8*)_wilddog_strchar(( char *)p_subpath_h,'/');
        if(!p_subpath_h)
            break;
        p_subpath_h++;
        p_subpath_end = (u8*)_wilddog_strchar((char *)p_subpath_h,'/');
        if( !p_subpath_end )
        {
            coap_add_option((coap_pdu_t*) p_arg->p_pkg, \
                             COAP_OPTION_URI_PATH, \
                             strlen((const char *)p_subpath_h), \
                             p_subpath_h);
            break;
        }
        else
            coap_add_option((coap_pdu_t*) p_arg->p_pkg, \
                            COAP_OPTION_URI_PATH, \
                            p_subpath_end - p_subpath_h, \
                            p_subpath_h);
    
    }

    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_coap_addQuery
 * Description: add query.
 * Input:       p_arg: p_coap/p_options. 
 * Output:      N/A
 * Return:      WILDDOG_ERR_NOERR or WILDDOG_ERR_NULL.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_addQuery
    (
    Protocol_Arg_Option_T *p_arg,
    int flag
    )
{
    if( p_arg == NULL || 
        p_arg->p_pkg == NULL ||
        p_arg->p_options == NULL)
        return WILDDOG_ERR_NULL;
    
    coap_add_option((coap_pdu_t*)p_arg->p_pkg, \
                    COAP_OPTION_URI_QUERY, \
                    strlen((const char *)p_arg->p_options), \
                    p_arg->p_options);

    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_coap_addData
 * Description: add data.
 * Input:       p_arg: p_coap/p_payload/d_payloadLen. 
 * Output:      N/A
 * Return:      WILDDOG_ERR_NOERR or WILDDOG_ERR_NULL.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_addData
    (
    Protocol_Arg_Payload_T *p_arg,
    int flag
    )
{
    int ret;
    if( p_arg == NULL || 
        p_arg->p_pkg == NULL ||
        p_arg->p_payload == NULL)
        return WILDDOG_ERR_NULL;
    ret = coap_add_data((coap_pdu_t*)p_arg->p_pkg,\
                        p_arg->d_payloadLen, \
                        p_arg->p_payload);
   
    if(0 == ret)
        return WILDDOG_ERR_NULL;

    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_coap_addObserver
 * Description: add query.
 * Input:       p_arg: p_coap/p_options. 
 * Output:      N/A
 * Return:      WILDDOG_ERR_NOERR or WILDDOG_ERR_NULL.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_addObserver
    (
    Protocol_Arg_Option_T *p_arg,
    int flag
    )
{
    if( p_arg == NULL || 
        p_arg->p_pkg == NULL ||
        p_arg->p_options == NULL)
    {
        return WILDDOG_ERR_NULL;
    }
    
    coap_add_option((coap_pdu_t*)p_arg->p_pkg,\
                    COAP_OPTION_OBSERVE, \
                    1, \
                    p_arg->p_options);

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:   _wilddog_coap_authUpdate
 * Description:Update the coap auth
 * Input:      p_arg->p_auth: The pointer of the auth data 
 *             p_arg->p_coap: The pointer of the coap pdu
 * Output:     N/A
 * Return:     If success, return WILDDOG_ERR_NOERR
*/
STATIC int WD_SYSTEM _wilddog_conn_coap_auth_update
    (
    Protocol_Arg_Auth_T *p_arg,
    int flag    
    )
{
    coap_opt_iterator_t d_oi;
    coap_opt_t *p_op = NULL;
    u8 *p_opvalue = NULL;

    if( p_arg == NULL ||\
        p_arg->p_pkg == NULL || \
        p_arg->p_newAuth == NULL)
            return WILDDOG_ERR_INVALID;
    /*   seek option*/
    p_op = coap_check_option(p_arg->p_pkg,COAP_OPTION_URI_QUERY,&d_oi);
    if(p_op == NULL)
        return WILDDOG_ERR_NOERR;
    /* pointer to option value*/
    p_opvalue = coap_opt_value(p_op);
    if(p_opvalue == NULL)
        return WILDDOG_ERR_INVALID;
    
    if(memcmp(p_opvalue,_CM_AUTHR_QURES,strlen(_CM_AUTHR_QURES)) != 0)
        return WILDDOG_ERR_INVALID;

    memcpy(p_opvalue,p_arg->p_newAuth,p_arg->d_newAuthLen);

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_coap_send.
 * Description: send out and coap package.
 *   
 * Input:       p_coap : point to an coap package.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
Wilddog_Return_T WD_SYSTEM _wilddog_coap_send(void *p_arg,int flag)
{
    coap_pdu_t *p_coap = (coap_pdu_t*)p_arg;
    
    if( p_coap == NULL)
        return WILDDOG_ERR_INVALID;
    
#ifdef DEBUG_LEVEL
    if(DEBUG_LEVEL <= WD_DEBUG_LOG )
        coap_show_pdu(p_coap);
#endif
   return _wilddog_sec_send(p_coap->hdr, p_coap->length);
}

Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_ping(void *p_arg,int flag)
{
    Wilddog_CM_Send_Ping_Arg_T *ping_pkg = (Wilddog_CM_Send_Ping_Arg_T*)p_arg;
    coap_pdu_t *p_coap = (coap_pdu_t*)(ping_pkg->p_pkg);
    coap_hdr_t *p_hdr = p_coap->hdr;
    
    if(NULL == ping_pkg || NULL == p_coap || NULL == p_hdr)
        return WILDDOG_ERR_INVALID;

    p_hdr->id = ping_pkg->d_mid;
    
    memcpy(p_hdr->token, (u8*)(&(ping_pkg->d_token)), p_hdr->token_length);

    return _wilddog_sec_send(p_coap->hdr, p_coap->length);
}

/*
 * Function:    _wilddog_recvCoap
 * Description: Verify the receive the coap 
 * Input:       p_buf: The pointer of the buffer
 *              buflen: The length of the buffer 
 * Output:      N/A
 * Return:      If there's coap packet, return the pointer of the coap pdu, 
 *              else return NULL
*/
STATIC coap_pdu_t * WD_SYSTEM _wilddog_recvCoap
    (
    u8 *p_buf,
    u32 buflen
    )
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
}
/*
 * Function:    _wilddog_recv_getOptionValue.
 * Description: parse  coap option and get it's value.
 *    
 * Input:       p_recvPdu : point to recv coap packet.
 *              optionCode : option code.
 *              p_dst : buffer that will store the  option value.
 *              d_dstSize :  p_dst sizeof.
 * Output:      N/A.
 * Return:      N/A.
*/
BOOL WD_SYSTEM _wilddog_recv_getOptionValue
    (
    coap_pdu_t *p_recvPdu,
    u32 optionCode,
    u8 *p_dst,
    u8 d_dstSize
    )
{
    coap_opt_t *p_op =NULL;
    u8 *p_optionvalue = NULL;
    u8 d_optionlen = 0;
    coap_opt_iterator_t d_oi;

    p_op = coap_check_option(p_recvPdu,optionCode,&d_oi);
    if(p_op)
    {
        d_optionlen = coap_opt_length(p_op);
        if(d_optionlen && d_optionlen <= d_dstSize)
        {
            p_optionvalue = coap_opt_value(p_op);

#if WILDDOG_LITTLE_ENDIAN == 1 
            _sys_coap_ntol((u8*)p_dst,p_optionvalue,d_optionlen); 
#else
            memcpy((u8*)p_dst,p_optionvalue,d_optionlen);
#endif
#if 0
    wilddog_debug_level( WD_DEBUG_WARN, \
                         "option value address = %p ,option len = %d \n",\
                         p_dst,d_optionlen);
#endif
            return TRUE;
        }
 
    }
    
    return FALSE;
}

/*
 * Function:    _wilddog_recv_dispatch.
 * Description: recv coap packet and handle it.
 *   
 * Input:       p_recvPdu : point to the recv coap packet.
 *              p_cm_recvArg: recv arg that will transmit to cm port.
 * Output:      N/A.
 * Return:      N/A.
*/
Wilddog_Return_T WD_SYSTEM _wilddog_recv_dispatch
    (
    coap_pdu_t *p_recvPdu,
    Protocol_recvArg_T *p_cm_recvArg
    )
{
    
    size_t playloadLen =0;
    u8 *p_payload = NULL;
    /* get err*/
    wilddog_debug_level(WD_DEBUG_LOG," coap err : %d",p_recvPdu->hdr->code);
    p_cm_recvArg->err = _wilddog_coap_code2Http( \
                            (unsigned int)_GET_COAP_CODE(p_recvPdu->hdr->code));
    
    wilddog_debug_level(WD_DEBUG_LOG,"coap http error %ld",p_cm_recvArg->err);

    /* get token option.*/
    if(p_recvPdu->hdr->token_length != COAP_TOKENLEN)
        return WILDDOG_ERR_INVALID;
    
    memcpy(&p_cm_recvArg->d_token,p_recvPdu->hdr->token,COAP_TOKENLEN);
    /* get observer index.*/
    p_cm_recvArg->d_isObserver = _wilddog_recv_getOptionValue(p_recvPdu,
                                        COAP_OPTION_OBSERVE,
                                        (u8*)&p_cm_recvArg->d_observerIndx,
                                        sizeof(p_cm_recvArg->d_observerIndx));
    
    wilddog_debug_level(WD_DEBUG_LOG, \
                        "coap get observerIndex : %lu ", \
                        p_cm_recvArg->d_observerIndx);
    /* get maxage.*/
    _wilddog_recv_getOptionValue(p_recvPdu,
                                 COAP_OPTION_MAXAGE, \
                                 (u8*)&(p_cm_recvArg->d_maxAge), \
                                 sizeof( p_cm_recvArg->d_maxAge));
    
    wilddog_debug_level(WD_DEBUG_LOG, \
                        "coap get max-age : %lu ", \
                        p_cm_recvArg->d_maxAge);
    /* get blockNum.*/
    /*get payload data */
    coap_get_data(p_recvPdu,&playloadLen,&p_payload);
    /* clean buf .*/
    memset(p_cm_recvArg->p_recvData,0,(size_t)p_cm_recvArg->d_recvDataLen);
    
    if( playloadLen > p_cm_recvArg->d_recvDataLen )
        return WILDDOG_ERR_INVALID;

    memcpy( p_cm_recvArg->p_recvData,p_payload,playloadLen);
    p_cm_recvArg->d_recvDataLen = playloadLen;
    
    wilddog_debug_level(WD_DEBUG_LOG, \
                        "coap recv data :%s", \
                        p_cm_recvArg->p_recvData);
    
    return WILDDOG_ERR_NOERR;
    
}
/*
 * Function:    _wilddog_coap_ackSend.
 * Description: response an ack .
 * Input:       recv_type: recv type conn or non-con .
 *              ack_type : respond an ack or reset.
 *              mid : recv coap's message id.
 *              tokenLen: recv coap's token len.
 *              recv_token: recv coap's token.
 * Output:      N/A.
 * Return:      N/A.
*/
STATIC int WD_SYSTEM _wilddog_coap_ackSend
    (
    u32 recv_type,
    u32 ack_type,
    u32 mid,
    u32 tokenLen,
    u32 recv_token
    ) 
{
    int returnCode= 0;
    unsigned int id = mid;
    size_t tkl = tokenLen;
    unsigned char* p_tk = (unsigned char*)&recv_token;
    coap_pdu_t  *toSend = NULL;
    /* only con request need tobe respond.*/
    if(recv_type != COAP_MESSAGE_CON)
        return WILDDOG_ERR_NOERR;

    toSend = coap_pdu_init(ack_type, 0, id,WILDDOG_PROTO_MAXSIZE);
    coap_add_token(toSend,tkl,p_tk);
    if (toSend == NULL) 
    {
        wilddog_debug_level(WD_DEBUG_ERROR,"coap_addToken error");
        return WILDDOG_ERR_NULL;
    }
    returnCode = _wilddog_sec_send(toSend->hdr,toSend->length);
    
    coap_delete_pdu(toSend);
    return returnCode;

}

/*  to do 
 * Function:    _wilddog_coap_receive.
 * Description: recv coap packet and handle it.
 *   
 * Input:       N/A.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type
*/
Wilddog_Return_T WD_SYSTEM _wilddog_coap_receive(void *p_arg,int flag)
{
    int res =0;
    u32 recv_type = 0;
    u32 ack_type = COAP_MESSAGE_ACK;
    u32 tmp_tokenLen = 0;
    u32 tmp_token = 0;
    u32 tmp_mid = 0;
    Protocol_recvArg_T recvArg;
    coap_pdu_t *p_pdu = NULL;

    memset(&recvArg,0,sizeof(Protocol_recvArg_T));

    recvArg.p_recvData = _wilddog_coap_mallocRecvBuffer();
    if( recvArg.p_recvData  == NULL)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
        return WILDDOG_ERR_NULL;
    }
    
    res = _wilddog_sec_recv((void*)recvArg.p_recvData,(s32)WILDDOG_PROTO_MAXSIZE);
    /*@ NO enougth space */
    if( res <= 0 || res  > WILDDOG_PROTO_MAXSIZE ) 
        goto _COAPRECV_ERR;

    recvArg.d_recvDataLen = res;

    /* distinguish recv an coap packet. */
    p_pdu = _wilddog_recvCoap(recvArg.p_recvData,recvArg.d_recvDataLen);
    if(p_pdu == NULL)
        goto _COAPRECV_ERR; 
    
#ifdef WILDDOG_SELFTEST                        
    ramtest_caculate_peakRam();             
    performtest_getHandleRecvDtlsTime();
#endif  

#ifdef WILDDOG_DEBUG
#if DEBUG_LEVEL <= WD_DEBUG_LOG
    printf("recv:\n");
    coap_show_pdu(p_pdu);
#endif
#endif
    /* dispatch .*/
    recvArg.d_recvDataLen = WILDDOG_PROTO_MAXSIZE;
    if( _wilddog_recv_dispatch(p_pdu,&recvArg) < 0)
    {
        coap_delete_pdu(p_pdu);
        goto _COAPRECV_ERR;
            
    }
    tmp_mid = p_pdu->hdr->id;
    recv_type = p_pdu->hdr->type;
    tmp_tokenLen = p_pdu->hdr->token_length;
    memcpy(&tmp_token,p_pdu->hdr->token,p_pdu->hdr->token_length);

    coap_delete_pdu(p_pdu);
    /* call back.*/
    if( _wilddog_coap_findRespondNode(&recvArg) != TRUE)
        ack_type = COAP_MESSAGE_RST;
    /* ack */
    _wilddog_coap_ackSend(recv_type,ack_type,tmp_mid,tmp_tokenLen,tmp_token);
    _wilddog_coap_freeRecvBuffer( recvArg.p_recvData );
   
    return  WILDDOG_ERR_NOERR;
   
_COAPRECV_ERR:
    _wilddog_coap_freeRecvBuffer( recvArg.p_recvData );
    return WILDDOG_ERR_NULL;
}
/*
 * Function:    _wilddog_coap_init.
 * Description: init coap port.
 *   
 * Input:       p_arg->p_host : DNS server name .
 *              p_arg->d_port : connect udp port.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type.
*/
Wilddog_Return_T WD_SYSTEM _wilddog_coap_init
    (
    Protocol_Arg_Init_T *p_arg,
    int flag
    )
{
    if( p_arg == NULL ||
        p_arg->p_host == NULL)
        return WILDDOG_ERR_INVALID;

    _wilddog_coap_findRespondNode = p_arg->f_handleRespond;
    
    return  _wilddog_sec_init(p_arg->p_host,p_arg->d_port);
}
/*
 * Function:    _wilddog_coap_deInit.
 * Description: destory coap packet  port.
 *   
 * Input:       N/A.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type .
*/
Wilddog_Return_T _wilddog_coap_deInit(void *p_arg,int flag)
{
    _wilddog_coap_findRespondNode = NULL;
    return _wilddog_sec_deinit();
}
/* protocol :: coap  interface */
Wilddog_Func_T _wilddog_protocolPackage_funcTable[_PROTOCOL_CMD_MAX + 1] = 
{
    (Wilddog_Func_T) _wilddog_coap_init,        
    (Wilddog_Func_T) _wilddog_coap_deInit,

    (Wilddog_Func_T) _wilddog_coap_countPacktSize,
    (Wilddog_Func_T) _wilddog_coap_creat,
    (Wilddog_Func_T) _wilddog_coap_destory,
    (Wilddog_Func_T) _wilddog_coap_addHost,
    (Wilddog_Func_T) _wilddog_coap_addPath,
    (Wilddog_Func_T) _wilddog_coap_addQuery,
    (Wilddog_Func_T) _wilddog_coap_addObserver,
    (Wilddog_Func_T) _wilddog_coap_addData,

    (Wilddog_Func_T) _wilddog_conn_coap_auth_update,
    (Wilddog_Func_T) _wilddog_coap_send,
    (Wilddog_Func_T) _wilddog_coap_receive,
    (Wilddog_Func_T) _wilddog_coap_send_ping,
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
size_t WD_SYSTEM _wilddog_protocol_ioctl
    (
    Protocol_cmd_t cmd,
    void *p_args,
    int flags
    )
{
    if( cmd >= _PROTOCOL_CMD_MAX ||
        cmd < 0)
        return WILDDOG_ERR_INVALID;

    return (size_t)(_wilddog_protocolPackage_funcTable[cmd])(p_args,flags);
}
