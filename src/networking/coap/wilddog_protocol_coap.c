/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_protocol_coap.c
 *
 * Description: coap special protocol functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 1.1.1        jimmy           2017-01-11  Create file.
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
#include "wilddog_protocol.h"
#include "wilddog_protocol_coap.h"
#include "wilddog_debug.h"
#include "wilddog_common.h"
#include "wilddog_sec.h"
#include "test_lib.h"

#include "wilddog_conn.h"

/*
 * Function:    _wilddog_conn_mallocRecvBuffer
 * Description: conn layer malloc the buffer which used for recv
 *   
 * Input:       N/A
 * Output:      N/A
 * Return:      the buffer's pointer
*/
STATIC u8* WD_SYSTEM _wilddog_coap_mallocRecvBuffer(Wilddog_Protocol_T *proto)
{   
    int i = 0;
    u8* buffer = NULL;  

    wilddog_assert(proto, NULL);
    
    for(i = 0; i < WILDDOG_PROTO_RECV_BUF_NUM; i++){
        if(proto->recv_buf[i].isused == FALSE)  
        {       
            buffer = proto->recv_buf[i].data;       
            proto->recv_buf[i].isused = TRUE;       
            memset(buffer, 0, WILDDOG_PROTO_MAXSIZE);   
        }
    }
     
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
STATIC void WD_SYSTEM _wilddog_coap_freeRecvBuffer(Wilddog_Protocol_T *proto,u8* ptr)
{
    int i;

    if(!ptr || !proto)
        return;

    for(i = 0; i < WILDDOG_PROTO_RECV_BUF_NUM; i++){   
        if(proto->recv_buf[i].data == ptr && TRUE == proto->recv_buf[i].isused){
            proto->recv_buf[i].isused = FALSE;
        }
    }

    return;
}


/*
 * Function:    _wilddog_coap_random
 * Description: Get a rand number
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC INLINE u32 WD_SYSTEM _wilddog_coap_random(void)
{
    srand(_wilddog_getTime()); 
    return (u32)rand();
}
//not thread safe
STATIC INLINE u16 WD_SYSTEM _wilddog_coap_getMid(void)
{
    STATIC u16 mid = 1;
    return mid++;
}
/*
 * Function:    _wilddog_coap_getToken
 * Description: get index, not thread safe
 * Input:       N/A    
 * Output:      N/A
 * Return:      A rand number 
*/
STATIC u32 WD_SYSTEM _wilddog_coap_getToken(u16 mid)
{
    u32 token = _wilddog_coap_random();
    return (u32)(((token & (~0xff)) | (mid & 0xff)) & 0xffffffff);
}
/*
 * Function:    _wilddog_coap_countChar
 * Description: count the number of  char 'c' exist in  the string buffer.
 * Input:       c: The char.   
 *              p_buf: The pointer of the string buffer.
 * Output:      N/A
 * Return:      The number.
*/
STATIC int WD_SYSTEM _wilddog_coap_countChar(const char c,const char *p_buf)
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
coap header.
 |       0       |       1       |       2       |       3       |
 |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |Ver| T |  TKL  |      Code     |          Message ID           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |   Token (if any, TKL bytes) ...
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |   Options (if any) ...
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |1 1 1 1 1 1 1 1|    Payload (if any) ...
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

option max header will be 5 bytes.
 7   6   5   4   3   2   1   0
+---------------+---------------+
|  Option Delta | Option Length |   1 byte
+---------------+---------------+
/         Option Delta          /   0-2 bytes
\          (extended)           \
+-------------------------------+
/         Option Length         /   0-2 bytes
\          (extended)           \
+-------------------------------+
/         Option Value          /   0 or more bytes
+-------------------------------+
*/
//Try to count this coap packet's size, so we can malloc less buffer.
STATIC int WD_SYSTEM _wilddog_coap_countSize(Wilddog_Coap_Pkt_T pkt){
    int size = 0;

    wilddog_assert(pkt.url, -1);

    // 4 bytes header and token
    size += 4 + pkt.token_length;
    
    //host option, option max head length is 5
    if(pkt.url->p_url_host){
        size += 5 + strlen((const char*)pkt.url->p_url_host) + 1;
    }
    //path option
    if(pkt.url->p_url_path){
        // path will be separated into some options by '/', it may be more than 1 option
        int num = 0;
        num = _wilddog_coap_countChar('/', (const char *)pkt.url->p_url_path);
        // max option number is "number of /" + 1
        num++;
        size += num * 5 + strlen((const char*)pkt.url->p_url_path) + 1;
    }
    else{
        //a null path, also will be in coap, but no data
        size += 5;
    }
    
    //query option
    if(pkt.url->p_url_query){
        //query will be separated into some options by '&', it may be more than 1 option
        int num = 0;
        num = _wilddog_coap_countChar('&', (const char *)pkt.url->p_url_query);
        // max option number is "number of &" + 1
        num++;
        size += num * 5 + strlen((const char*)pkt.url->p_url_query) + 1;
    }
    //may be observe option, observe 0
    if(WILDDOG_COAP_CMD_ON == pkt.command){
        size += 5 + 1;
    }
        
    //payload, with 0xff ahead.
    size += pkt.data_len + 1;
    return size;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_initSession(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Coap_Pkt_T pkt;
    u32 token;
    Wilddog_Str_T* oldPath = NULL;
    Wilddog_Str_T* tmp = NULL;
    int finalPathLen = 0;
    coap_pdu_t * pdu = NULL;
    
    wilddog_assert(data, WILDDOG_ERR_NULL);
    wilddog_assert(!arg->p_url->p_url_path, WILDDOG_ERR_NULL);

    //add information for coap header
    pkt.command = WILDDOG_COAP_CMD_INIT;
    pkt.type = COAP_MESSAGE_CON;
    pkt.code = COAP_REQUEST_POST;
    pkt.version = COAP_DEFAULT_VERSION;
    pkt.mid = _wilddog_coap_getMid();

    //get coap token, donot use hdr.token, use pkt.token instead
    pkt.token_length = WILDDOG_COAP_TOKEN_LEN;
    token = _wilddog_coap_getToken(pkt.mid);
    memcpy(pkt.token,(u8*)&token,WILDDOG_COAP_TOKEN_LEN);

    //in our coap, token is message id and more than mid, tell up layer the message id
    *(arg->p_message_id) = token;
    
    //get user auth token as data.
    pkt.data = arg->p_data;
    pkt.data_len = arg->d_data_len;

    pkt.url = arg->p_url;

    //merge session init path with path .cs, assert pkt.url->p_url_path is null
    finalPathLen = strlen(WILDDOG_COAP_SESSION_PATH) + 1;
    tmp = (Wilddog_Str_T*)wmalloc(finalPathLen);
    wilddog_assert(tmp, WILDDOG_ERR_NULL);
    if(NULL == tmp){
        wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed!");
        return WILDDOG_ERR_NULL;
    }
    //store old path
    oldPath = pkt.url->p_url_path;
    pkt.url->p_url_path = tmp;
    snprintf((char*)pkt.url->p_url_path, finalPathLen, "%s", WILDDOG_COAP_SESSION_PATH);
    //count pkt size.
    pkt.size = _wilddog_coap_countSize(pkt);

    //make a coap packet
    pdu = coap_pdu_init(pkt.type, pkt.code, pkt.mid, pkt.size);
    if(NULL == pdu){
        wfree(pkt.url->p_url_path);
        pkt.url->p_url_path = oldPath;
        wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed!");
        return WILDDOG_ERR_NULL;
    }
    //add token
    coap_add_token(pdu, pkt.token_length, pkt.token);
    //add host
    coap_add_option(pdu,COAP_OPTION_URI_HOST,strlen((const char*)pkt.url->p_url_host),pkt.url->p_url_host);

    //add path
    coap_add_option(pdu,COAP_OPTION_URI_PATH, strlen((const char*)pkt.url->p_url_path),pkt.url->p_url_path);

    //add data
    coap_add_data(pdu,pkt.data_len, pkt.data);

    if(_wilddog_sec_send(arg->protocol, pdu->hdr, pdu->length) < 0){
        coap_delete_pdu(pdu);
        wfree(pkt.url->p_url_path);
        return WILDDOG_ERR_SENDERR;
    }
    wfree(pkt.url->p_url_path);
    pkt.url->p_url_path = oldPath;
    *(u8**)(arg->p_out_data) = (u8*)pdu;
    *(arg->p_out_data_len) = (u32)pdu->length;
    
    return WILDDOG_ERR_NOERR;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_recv_freePkt(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;

    wilddog_assert(data, WILDDOG_ERR_NULL);

    if(arg->p_out_data){
        wfree(arg->p_out_data);
    }
    return WILDDOG_ERR_NOERR;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_recv_getPkt(void* data, int flag){
    u8* recv_data = NULL;
    int res = 0;
    coap_pdu_t * pdu;
    
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;

    wilddog_assert(data, WILDDOG_ERR_NULL);
    //1. recv from socket
    recv_data = _wilddog_coap_mallocRecvBuffer(arg->protocol);
    wilddog_assert(recv_data, WILDDOG_ERR_NULL);
    res = _wilddog_sec_recv(arg->protocol,(void*)recv_data,(s32)WILDDOG_PROTO_MAXSIZE);
    if(res < 0 || res > WILDDOG_PROTO_MAXSIZE){
        _wilddog_coap_freeRecvBuffer(arg->protocol,recv_data);
        wilddog_debug_level(WD_DEBUG_ERROR, "Receive failed, error = %d",res);
        return WILDDOG_ERR_INVALID;
    }
    
    //2. malloc coap buffer, handle coap, get token
    pdu = coap_new_pdu();
    if(NULL == pdu){
        _wilddog_coap_freeRecvBuffer(arg->protocol,recv_data);
        wilddog_debug_level(WD_DEBUG_ERROR, "Malloc pdu failed!");
        return WILDDOG_ERR_NULL;
    }
    if(0 == coap_pdu_parse(recv_data, res,pdu)){
        coap_delete_pdu(pdu);
        _wilddog_coap_freeRecvBuffer(arg->protocol,recv_data);
        wilddog_debug_level(WD_DEBUG_ERROR, "Parse pdu failed!");
        return WILDDOG_ERR_INVALID;
    }
    if(pdu->hdr->version != COAP_DEFAULT_VERSION){
        coap_delete_pdu(pdu);
        _wilddog_coap_freeRecvBuffer(arg->protocol,recv_data);
        wilddog_debug_level(WD_DEBUG_ERROR, "Parse pdu failed!");
        return WILDDOG_ERR_INVALID;
    }
    //3. send pkt to upper
    *(arg->p_message_id) = *(u32*)(pdu->hdr->token);
    *(arg->p_out_data) = (u8*)pdu;
    *(arg->p_out_data_len) = pdu->length;
    return WILDDOG_ERR_NOERR;
}

/* protocol :: coap  interface */
Wilddog_Func_T _wilddog_protocol_funcTable[WD_PROTO_CMD_MAX + 1] = 
{
    (Wilddog_Func_T) _wilddog_coap_initSession,//init   
    (Wilddog_Func_T)NULL,//ping
    (Wilddog_Func_T)NULL,//get
    (Wilddog_Func_T)NULL,//set
    (Wilddog_Func_T)NULL,//push
    (Wilddog_Func_T)NULL,//remove
    (Wilddog_Func_T)NULL,//on
    (Wilddog_Func_T)NULL,//off
    (Wilddog_Func_T)NULL,//dis set
    (Wilddog_Func_T)NULL,//dis push
    (Wilddog_Func_T)NULL,//dis remove
    (Wilddog_Func_T)NULL,//dis cancel
    (Wilddog_Func_T)NULL,//online
    (Wilddog_Func_T)NULL,//offline
    (Wilddog_Func_T)_wilddog_coap_recv_getPkt,//get pkt
    (Wilddog_Func_T)_wilddog_coap_recv_freePkt//free pkt
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
    Wilddog_Proto_Cmd_T cmd,
    void *p_args,
    int flags
    )
{
    if( cmd >= WD_PROTO_CMD_MAX ||
        cmd < 0)
        return WILDDOG_ERR_INVALID;

    return (size_t)(_wilddog_protocol_funcTable[cmd])(p_args,flags);
}

/*
 * Function:    _wilddog_protocol_init.
 * Description: init protocol.
 *   
 * Input:       p_conn : connect layer pointer.
 *              
 * Output:      N/A.
 * Return:      Wilddog_Protocol_T type.
*/
Wilddog_Protocol_T * WD_SYSTEM _wilddog_protocol_init(void *data)
{
    Wilddog_Protocol_T *protocol;
    Wilddog_Conn_T* p_conn = data;
    
    wilddog_assert(p_conn, NULL);

    protocol = (Wilddog_Protocol_T*)wmalloc(sizeof(Wilddog_Protocol_T));

    wilddog_assert(protocol, NULL);
    
    protocol->socketFd = -1;
    protocol->host = p_conn->p_conn_repo->p_rp_url->p_url_host;
    protocol->addr.port = WILDDOG_PORT;
    protocol->callback = _wilddog_protocol_ioctl;

    //init socket and ip
    if(WILDDOG_ERR_NOERR != _wilddog_sec_init(protocol)){
        wfree(protocol);
        wilddog_debug_level(WD_DEBUG_ERROR, "Init secure failed!");
        return NULL;
    }

    return protocol;
}

/*
 * Function:    _wilddog_protocol_deInit.
 * Description: DeInit protocol.
 *   
 * Input:       N/A.
 * Output:      N/A.
 * Return:      Wilddog_Return_T type .
*/
Wilddog_Return_T WD_SYSTEM _wilddog_protocol_deInit(void *data)
{
    Wilddog_Conn_T *p_conn = (Wilddog_Conn_T*)data;
    _wilddog_sec_deinit(p_conn->p_protocol);

    wfree(p_conn->p_protocol);
    return WILDDOG_ERR_NOERR;
}

