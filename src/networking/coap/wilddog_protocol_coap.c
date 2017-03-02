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

#define  COAP_CODE_GET(code)   ((code >> 5) * 100 + (code & 0x1F))
#define WILDDOG_RECONNECT_TIMES (3) //reconnect retry time

typedef struct _WILDDOG_COAP_OBSERVE_DATA{
    u32 last_index;
    u32 last_recv_time;
    u32 maxage;
}_Wilddog_Coap_Observe_Data_T;

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
 * Function:    _wilddog_coap_ntoh
 * Description: Convert the byte order
 * Input:       src: The pointer of the source byte    
 *              srcLen: src max bytes
 *              dstLen: The length of the source byte
 * Output:      dst: The pointer of the destination byte
 * Return:      N/A
*/
STATIC INLINE void WD_SYSTEM  _wilddog_coap_ntoh
    (
    u8 *dst,
    const u16 dstLen,
    const u8 *src,
    const u16 srcLen
    )
{
    u16 i;
    u16 len = srcLen > dstLen ? dstLen:srcLen;
    // if srcLen > dstLen, copy lsb
#if WILDDOG_LITTLE_ENDIAN == 1
    // lsb is from head
    for(i = 0; i < len; i++){
        dst[i] = src[srcLen - i];
    }
#else
    //lsb is from tail
    for(i = 0; i < len; i++){
        if(dstLen - srcLen + i < 0)
            continue;
        dst[dstLen - srcLen + i] = src[i];
    }
#endif
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
    //Do not need to care about endian, because we send what, will receive the same.
    //Only we need is to make sure it will not conflict in short time.
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
    //Do not need to care about endian, because we send what, will receive the same.
    //Only we need is to sure it is randomable.
    u32 token = _wilddog_coap_random();
    return (u32)(((token & (~0xff)) | (mid & 0xff)) & 0xffffffff);
}
STATIC coap_opt_t* WD_SYSTEM _wilddog_coap_getSendSessionOption(coap_pdu_t * pdu){
    //FIXME: We used a simple method, we assume .cs query is the head, 
    // so coap_check_option's return is .cs query option. 
    //But we have more than 1 query, such as disconnect , so fix it!!!
    coap_opt_iterator_t d_oi;
    
    wilddog_assert(pdu, NULL);

    return coap_check_option(pdu,COAP_OPTION_URI_QUERY,&d_oi);
}
/*
 * Function:    _wilddog_coap_getRecvCode
 * Description: Convert the coap code to http return code
 * Input:       pdu: The coap pkt   
 * Output:      N/A
 * Return:      The http return code
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_getRecvCode(coap_pdu_t * pdu)
{
    u32 rec_code;

    wilddog_assert(pdu, WILDDOG_ERR_NULL);

    rec_code = COAP_CODE_GET(pdu->hdr->code);

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
 * Function:    _wilddog_coap_addPath
 * Description: add path to coap packages.
 * Input:       pdu: coap pdu.
 *              path: path string.
 * Output:      N/A
 * Return:      WILDDOG_ERR_NOERR or WILDDOG_ERR_NULL.
 * Other:       path can be : 1. "/" 2. "/a" 3. "/a/b" 4. "/a/b/(maybe)" 
 *                            5. "a" (maybe) 6. "a/b" (maybe)
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_addPath(coap_pdu_t *pdu,char *path)
{
    char *p_subpath_start = NULL;
    char *p_subpath_end = NULL;
    unsigned int total_len = 0;
    
    wilddog_assert(pdu, WILDDOG_ERR_NULL);

    if(NULL == path){
        coap_add_option(pdu, COAP_OPTION_URI_PATH,0, NULL);
        return WILDDOG_ERR_NOERR;
    }
    total_len = strlen((const char*)path);
    if('/' == path[0] && 1 == total_len){
        // handle condition 1
        //coap_add_option(pdu,COAP_OPTION_URI_PATH,total_len, (u8*)path);
        coap_add_option(pdu, COAP_OPTION_URI_PATH,0, NULL);
        return WILDDOG_ERR_NOERR;
    }
    
    p_subpath_start = path;
    //change condition 2/3/4 to 5 or 6
    if('/' == p_subpath_start[0])
        p_subpath_start++;

    //now we only need handle condition 5/6
    while(p_subpath_start){
        p_subpath_end = _wilddog_strchar(p_subpath_start, '/');
        if(!p_subpath_end){
            // path has no / anymore, maybe condition 5 or condition 6's step 2
            coap_add_option(pdu,COAP_OPTION_URI_PATH,strlen(p_subpath_start),(u8*)p_subpath_start);
            return WILDDOG_ERR_NOERR;
        }
        // now we find /, such as "a" in "a/b", length is end - start.
        coap_add_option(pdu,COAP_OPTION_URI_PATH,p_subpath_end - p_subpath_start,(u8*)p_subpath_start);
        //jump "/"
        p_subpath_start = p_subpath_end + 1;
    }
    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_coap_addQuery
 * Description: add query to coap packages.
 * Input:       pdu: coap pdu.
 *              query: query string.
 * Output:      N/A
 * Return:      WILDDOG_ERR_NOERR or WILDDOG_ERR_NULL.
 * Other:       path can be : 1. "a" 2. "a&b"
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_addQuery(coap_pdu_t *pdu,char *query)
{
    char *p_subquery_start = NULL;
    char *p_subquery_end = NULL;
    
    wilddog_assert(pdu, WILDDOG_ERR_NULL);

    if(NULL == query){
        coap_add_option(pdu, COAP_OPTION_URI_QUERY,0, NULL);
        return WILDDOG_ERR_NOERR;
    }
    
    p_subquery_start = query;

    while(p_subquery_start){
        p_subquery_end = _wilddog_strchar(p_subquery_start, '&');
        if(!p_subquery_end){
            // query has no & anymore, maybe condition 1 or condition 2's step 2
            coap_add_option(pdu,COAP_OPTION_URI_QUERY,strlen(p_subquery_start),(u8*)p_subquery_start);
            return WILDDOG_ERR_NOERR;
        }
        // now we find &, such as "a" in "a&b", length is end - start.
        coap_add_option(pdu,COAP_OPTION_URI_QUERY,p_subquery_end - p_subquery_start,(u8*)p_subquery_start);
        //jump "&"
        p_subquery_start = p_subquery_end + 1;
    }
    return WILDDOG_ERR_NOERR;
}


/*notice, observe_index may not be continually, and may be fallback to 
  little index.
  from RFC 7641: https://tools.ietf.org/html/rfc7641#section-3.4
   V1 is last index, V2 is recv index, T1 is last recv time
   T2 is now recv time. Condition 3 we do not care , only care 1 and 2

  (V1 < V2 and V2 - V1 < 2^23) or
  (V1 > V2 and V1 - V2 > 2^23) or
  (T2 > T1 + 128 seconds)
*/
STATIC BOOL WD_SYSTEM _wilddog_coap_isObserveNew(u32 old_index, u32 new_index){
    if(((old_index < new_index) && (new_index - old_index < (1<<23))) || \
        ((old_index > new_index) && (old_index - new_index > (1<<23))))
        return TRUE;
    
    return FALSE;
}
/*
+-----+---+---+---+---+---------+--------+--------+---------+
| No. | C | U | N | R | Name    | Format | Length | Default |
+-----+---+---+---+---+---------+--------+--------+---------+
|   6 |   | x | - |   | Observe | uint   | 0-3 B  | (none)  |
+-----+---+---+---+---+---------+--------+--------+---------+
*/
STATIC u32 WD_SYSTEM _wilddog_coap_getRecvObserveIndex(coap_pdu_t * pdu)
{
    u32 observe = 0;
    u16 len;
    coap_opt_t *p_op =NULL;
    coap_opt_iterator_t d_oi;
    u8 *option_value = NULL;
    
    wilddog_assert(pdu, WILDDOG_ERR_NULL);

    p_op = coap_check_option(pdu,COAP_OPTION_OBSERVE,&d_oi);

    if(p_op){
        len = coap_opt_length(p_op);
        // max observe data is 3 bytes.
        if(len == 0 || len > 3){
            wilddog_debug_level(WD_DEBUG_ERROR, "Get an maxage option but length is %d!",len);
            return 0;
        }
        option_value = coap_opt_value(p_op);
        if(NULL == option_value){
            wilddog_debug_level(WD_DEBUG_ERROR, \
                     "Get an option length is %d but no value!",len);

            return 0;
        }
        //observe option is in big endian.
        _wilddog_coap_ntoh((u8*)&observe,sizeof(observe),option_value,len);
    }
    return observe;
}

/*
+-----+---+---+---+---+----------------+--------+--------+----------+
| No. | C | U | N | R |      Name      | Format | Length |  Default |
+-----+---+---+---+---+----------------+--------+--------+----------+
| 14  |   | x | - |   |     Max-Age    | uint   | 0-4    |  60      |
+-----+---+---+---+---+----------------+--------+--------+----------+
*/
STATIC u32 WD_SYSTEM _wilddog_coap_getRecvMaxage(coap_pdu_t * pdu)
{
    u32 maxage = 0;
    u16 len;
    coap_opt_t *p_op =NULL;
    coap_opt_iterator_t d_oi;
    u8 *option_value = NULL;
    
    wilddog_assert(pdu, WILDDOG_ERR_NULL);

    p_op = coap_check_option(pdu,COAP_OPTION_MAXAGE,&d_oi);

    if(p_op){
        len = coap_opt_length(p_op);
        // max maxage data is 4 bytes.
        if(len == 0 || len > 4){
            wilddog_debug_level(WD_DEBUG_ERROR, "Get an maxage option but length is %d!",len);
            return 0;
        }
        option_value = coap_opt_value(p_op);
        if(NULL == option_value){
            wilddog_debug_level(WD_DEBUG_ERROR, \
                     "Get an option length is %d but no value!",len);

            return 0;
        }
        //maxage option is in big endian.
        _wilddog_coap_ntoh((u8*)&maxage,sizeof(maxage),option_value,len);
    }
    return maxage;
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
    size += 5 + 1;
    
    //payload, with 0xff ahead.
    size += pkt.data_len + 1;
    return size;
}
//now we only care one packet, do not thinking about partition.
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_sendPkt(Wilddog_Coap_Sendpkt_Arg_T arg,BOOL isNeedCs, Wilddog_Coap_Observe_Stat_T observeStat)
{
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Pkt_T pkt;
    u32 token = 0;
    coap_pdu_t *pdu = NULL;
    Wilddog_Str_T *new_query = NULL;
    int query_len = 0;
    Wilddog_Str_T *tmp = NULL;
    
    wilddog_assert(arg.protocol&&arg.url&&arg.token&&arg.send_pkt, WILDDOG_ERR_NULL);

    //add information for coap header
    pkt.type = COAP_MESSAGE_CON;
    pkt.code = arg.code;
    pkt.version = COAP_DEFAULT_VERSION;
    pkt.mid = _wilddog_coap_getMid();

    //get coap token, donot use hdr.token, use pkt.token instead
    pkt.token_length = WILDDOG_COAP_TOKEN_LEN;
    token = _wilddog_coap_getToken(pkt.mid);
    memcpy(pkt.token,(u8*)&token,WILDDOG_COAP_TOKEN_LEN);

    *arg.token = token;

    //get user data.
    pkt.data = arg.data;
    pkt.data_len = arg.data_len;

    pkt.url = arg.url;

    if(TRUE == isNeedCs){
        //combine the short token with .cs query option, ".cs=<short token>", etc.
        if(NULL != arg.url->p_url_query){
            //query string length  = 
            //(pkt.url->p_url_query) + (&) + (.cs) + (=) + (short token length) + '\0'
            query_len = strlen((const char*)arg.url->p_url_query) + 1 + strlen(WILDDOG_COAP_SESSION_QUERY) + 1 + arg.d_session_len + 1;
        }else{
            //query string length  = (.cs) + (=) + (short token length) + '\0'
            query_len = strlen(WILDDOG_COAP_SESSION_QUERY) + 1 + arg.d_session_len + 1;
        }
        new_query = (Wilddog_Str_T*)wmalloc(query_len);
        wilddog_assert(new_query, WILDDOG_ERR_NULL);
        if(NULL != arg.url->p_url_query){
            sprintf((char*)new_query, "%s&%s=%s",(const char*)arg.url->p_url_query,WILDDOG_COAP_SESSION_PATH,(const char*)arg.p_session_info);
        }else{
            sprintf((char*)new_query, "%s=%s",WILDDOG_COAP_SESSION_PATH,(const char*)arg.p_session_info);
        }
        //store old query
        tmp = arg.url->p_url_query;
        arg.url->p_url_query = new_query;
    }

    //count pkt size.
    pkt.size = _wilddog_coap_countSize(pkt);

    //make a coap packet
    pdu = coap_pdu_init(pkt.type, pkt.code, pkt.mid, pkt.size);
    if(NULL == pdu){
        //restore query
        wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed!");
        ret = WILDDOG_ERR_NULL;
        goto END;
    }

    //add token
    coap_add_token(pdu, pkt.token_length, pkt.token);
    //add host
    coap_add_option(pdu,COAP_OPTION_URI_HOST,strlen((const char*)pkt.url->p_url_host),pkt.url->p_url_host);
    if(WILDDOG_COAP_OBSERVE_NOOBSERVE != observeStat){
        u8 observe_value = (WILDDOG_COAP_OBSERVE_ON == observeStat)?0:1;
        coap_add_option(pdu,COAP_OPTION_OBSERVE,sizeof(observe_value),&observe_value);
    }
    //add path
    _wilddog_coap_addPath(pdu,(char*)pkt.url->p_url_path);
    //add query
    if(pkt.url->p_url_query)
        _wilddog_coap_addQuery(pdu, (char*)pkt.url->p_url_query);
    //add data
    if(pkt.data)
        coap_add_data(pdu,pkt.data_len, pkt.data);
    
    ret = WILDDOG_ERR_NOERR;
    if(TRUE == arg.isSend){
        ret = _wilddog_sec_send(arg.protocol, pdu->hdr, pdu->length);
        if(ret < 0){
            coap_delete_pdu(pdu);
            pdu = NULL;
            wilddog_debug_level(WD_DEBUG_ERROR, "Send Packet %x failed.",(unsigned int)token);
            ret = WILDDOG_ERR_SENDERR;
        }else{
            ret = WILDDOG_ERR_NOERR;
        }
    }
    if(pdu){
        Wilddog_Conn_Pkt_Data_T *out_pkt = *arg.send_pkt;
        out_pkt->next = NULL;
        out_pkt->data = (u8*)pdu;
        out_pkt->len = (u32)(pdu->length&0xffff);
    }
END:
    if(isNeedCs){
        //resume old query
        arg.url->p_url_query = tmp;
        if(new_query)
            wfree(new_query);
    }
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_initSession(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Str_T* tmp = NULL;
    Wilddog_Str_T* new_path = NULL;
    int finalPathLen = 0;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
    //now we only care one packet, do not thinking about partition.
    wilddog_assert(data&&arg->protocol&&arg->p_url&&arg->p_out_data, WILDDOG_ERR_NULL);

    //merge session init path with path .cs, assert pkt.url->p_url_path is null
    finalPathLen = strlen(WILDDOG_COAP_SESSION_PATH) + 1;
    new_path = (Wilddog_Str_T*)wmalloc(finalPathLen);
    wilddog_assert(new_path, WILDDOG_ERR_NULL);
    if(NULL == new_path){
        wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed!");
        return WILDDOG_ERR_NULL;
    }
    sprintf((char*)new_path, "%s", WILDDOG_COAP_SESSION_PATH);
    //store old path
    tmp = arg->p_url->p_url_path;
    arg->p_url->p_url_path = new_path;
    
    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_POST;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = arg->p_data;
    send_arg.data_len = arg->d_data_len;
    send_arg.isSend = TRUE;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;

    ret = _wilddog_coap_send_sendPkt(send_arg, FALSE, WILDDOG_COAP_OBSERVE_NOOBSERVE);
    //resume old path
    arg->p_url->p_url_path = tmp;
    if(new_path)
        wfree(new_path);
    

    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_reconnect(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    wilddog_assert(arg->protocol, WILDDOG_ERR_NULL);
    //retry WILDDOG_RECONNECT_TIMES times
    return _wilddog_sec_reconnect(arg->protocol, WILDDOG_RECONNECT_TIMES);
}
/*
    Ping policy: 
    1. Short: Send GET coap://<appid>.wilddogio.com/.ping?.cs=<short token>
    2. Long:  Send POST coap://<appid>.wilddogio.com/.rst, payload is long token
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_ping(void* data, int flag){
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Str_T *new_query = NULL, *new_path = NULL;
    Wilddog_Str_T *tmp_query = NULL, *tmp_path = NULL;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;

    wilddog_assert(data&&arg->protocol&&arg->p_url&&arg->p_message_id&&arg->p_session_info&&arg->p_out_data, WILDDOG_ERR_NULL);

    //change url and data
    if(FALSE == flag){
        //coap://<appid>.wilddogio.com/.ping?.cs=<short token>
        int query_len = 0;
        //query = ".cs" + "=" + "<short token>"
        query_len = strlen(WILDDOG_COAP_SESSION_QUERY) + 1 + arg->d_session_len + 1;
        new_query = (Wilddog_Str_T*)wmalloc(query_len);
        if(NULL == new_query){
            wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed");
            return WILDDOG_ERR_NULL;
        }
        sprintf((char*)new_query, "%s=%s",WILDDOG_COAP_SESSION_QUERY,(const char*)arg->p_session_info);
        new_path = (Wilddog_Str_T*)wmalloc(strlen((const char*)WILDDOG_COAP_SESSION_PING_PATH) + 1);
        if(NULL == new_path){
            wfree(new_query);
            wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed");
            return WILDDOG_ERR_NULL;
        }
        sprintf((char*)new_path, "%s",WILDDOG_COAP_SESSION_PING_PATH);
        send_arg.data = NULL;
        send_arg.data_len = 0;
    }else{
        //coap://<appid>.wilddogio.com/.rst  payload = <long token>
        new_path = (Wilddog_Str_T*)wmalloc(strlen((const char*)WILDDOG_COAP_SESSION_RST_PATH) + 1);
        if(NULL == new_path){
            wfree(new_query);
            wilddog_debug_level(WD_DEBUG_ERROR, "Malloc failed");
            return WILDDOG_ERR_NULL;
        }
        sprintf((char*)new_path, "%s",WILDDOG_COAP_SESSION_RST_PATH);
        send_arg.data = arg->p_session_info;
        send_arg.data_len = arg->d_session_len;
    }
    
    //store old query
    tmp_query = arg->p_url->p_url_query;
    arg->p_url->p_url_query = new_query;
    tmp_path = arg->p_url->p_url_path;
    arg->p_url->p_url_path = new_path;

    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = (FALSE == flag) ? (COAP_REQUEST_GET):(COAP_REQUEST_POST);
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.isSend = TRUE;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;

    ret = _wilddog_coap_send_sendPkt(send_arg, FALSE, WILDDOG_COAP_OBSERVE_NOOBSERVE);

    //resume old query
    arg->p_url->p_url_query = tmp_query;
    if(new_query){
        wfree(new_query);
    }
    arg->p_url->p_url_path = tmp_path;
    if(new_path){
        wfree(new_path);
    }

    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_retransmit(void* data, int flag){
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    coap_pdu_t *pdu = NULL;
    coap_opt_t *opt = NULL;
    
    wilddog_assert(arg&&arg->p_data&&arg->protocol, WILDDOG_ERR_NULL);

    pdu = (coap_pdu_t *)arg->p_data;
    if(arg->d_session_len == WILDDOG_CONN_SESSION_SHORT_LEN - 1){
        //we must change short session key
        //find .cs query, change the value to newer
        opt  = _wilddog_coap_getSendSessionOption(pdu);
        if(opt){
            //reflash session data
            //data = ".cs" + "=" + "<short token>"
            int query_len;
            query_len = strlen(WILDDOG_COAP_SESSION_QUERY) + 1 + arg->d_session_len;
            if(query_len != coap_opt_length(opt)){
                //not match!
                wilddog_debug_level(WD_DEBUG_WARN,"Session not match!!!, real is %d, want %d",
                                    coap_opt_length(opt),query_len);
            }else{
                //matched, change old short token to new
                u8* value = coap_opt_value(opt);
                wilddog_assert(value, WILDDOG_ERR_NULL);
                //check again, because _wilddog_coap_getSendSessionOption maybe fail now.
                if(0 == strncmp((const char*)value, WILDDOG_COAP_SESSION_QUERY, strlen(WILDDOG_COAP_SESSION_QUERY))){
                    //ignore ".cs=", so value add 4, copy new sort token to query.
                    memcpy((char*)(value + 4),arg->p_session_info, arg->d_session_len);
                }
            }
        }
        //observe special operation
        if(arg->p_proto_data){
            _Wilddog_Coap_Observe_Data_T * observe_data = (_Wilddog_Coap_Observe_Data_T*)arg->p_proto_data;
            observe_data->last_index = 0;
            observe_data->last_recv_time = 0;
            observe_data->maxage = 0;
        }
    }else if(arg->d_session_len == WILDDOG_CONN_SESSION_LONG_LEN - 1){
        //change long session key
        //FIXME: now we do not need to change long token, let it go.
        //If send old token, we only need to reauth.
        wilddog_debug_level(WD_DEBUG_WARN, "Can not be here!");
    }
    ret = _wilddog_sec_send(arg->protocol, pdu->hdr, pdu->length);
    return (ret < 0) ? (WILDDOG_ERR_SENDERR):(WILDDOG_ERR_NOERR);
}

/*
    coap get packet like coap://  a.b.cn / path  ? query=1&query=2
                         [scheme] [host]  [path]   [     query   ]
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_getValue(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
    
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data, WILDDOG_ERR_NULL);

    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_GET;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = NULL;
    send_arg.data_len = 0;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_NOOBSERVE);

    return ret;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_setValue(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
   
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data && \
                   arg->p_data&&arg->d_data_len, WILDDOG_ERR_NULL);

    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_PUT;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = arg->p_data;
    send_arg.data_len = arg->d_data_len;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_NOOBSERVE);

    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_push(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
   
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data && \
                   arg->p_data&&arg->d_data_len, WILDDOG_ERR_NULL);

    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_POST;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = arg->p_data;
    send_arg.data_len = arg->d_data_len;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_NOOBSERVE);

    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_remove(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
   
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data, WILDDOG_ERR_NULL);

    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_DELETE;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = arg->p_data;
    send_arg.data_len = arg->d_data_len;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_NOOBSERVE);

    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_addObserver(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
    _Wilddog_Coap_Observe_Data_T *observe_data = NULL;
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data, WILDDOG_ERR_NULL);

    observe_data = (_Wilddog_Coap_Observe_Data_T*)wmalloc(sizeof(_Wilddog_Coap_Observe_Data_T));
    wilddog_assert(observe_data, WILDDOG_ERR_NULL);
    
    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_GET;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = NULL;
    send_arg.data_len = 0;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_ON);

    //add proto_data
    if(WILDDOG_ERR_NOERR == ret){
        *arg->p_proto_data = (u8*)observe_data;
    }else{
        wfree(observe_data);
    }
    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_removeObserver(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data, WILDDOG_ERR_NULL);
    
    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_GET;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = NULL;
    send_arg.data_len = 0;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_OFF);

    return ret;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_disSetValue(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
    Wilddog_Str_T *new_query = NULL;
    int query_len = 0;
    Wilddog_Str_T *tmp = NULL;
    
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data && \
                   arg->p_data&&arg->d_data_len, WILDDOG_ERR_NULL);

    //add disconnect function with query .dis=add
    if(NULL != arg->p_url->p_url_query){
        //query string length  = 
        //(pkt.url->p_url_query) + (&) + (.dis=add) + '\0'
        query_len = strlen((const char*)arg->p_url->p_url_query) + 1 + strlen(WILDDOG_COAP_ADD_DIS_QUERY) + 1;
    }else{
        //query string length  = (.dis=add) + '\0'
        query_len = strlen(WILDDOG_COAP_ADD_DIS_QUERY) + 1;
    }
    new_query = (Wilddog_Str_T*)wmalloc(query_len);
    wilddog_assert(new_query, WILDDOG_ERR_NULL);

    if(NULL != arg->p_url->p_url_query){
        sprintf((char*)new_query, "%s&%s",(const char*)arg->p_url->p_url_query,WILDDOG_COAP_ADD_DIS_QUERY);
    }else{
        sprintf((char*)new_query, "%s",WILDDOG_COAP_ADD_DIS_QUERY);
    }
    //store old query
    tmp = arg->p_url->p_url_query;
    arg->p_url->p_url_query = new_query;

    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_PUT;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = arg->p_data;
    send_arg.data_len = arg->d_data_len;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_NOOBSERVE);

    //resume old query
    arg->p_url->p_url_query = tmp;
    if(new_query)
        wfree(new_query);

    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_disPush(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
    Wilddog_Str_T *new_query = NULL;
    int query_len = 0;
    Wilddog_Str_T *tmp = NULL;
    
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data && \
                   arg->p_data&&arg->d_data_len, WILDDOG_ERR_NULL);

    //add disconnect function with query .dis=add
    if(NULL != arg->p_url->p_url_query){
        //query string length  = 
        //(pkt.url->p_url_query) + (&) + (.dis=add) + '\0'
        query_len = strlen((const char*)arg->p_url->p_url_query) + 1 + strlen(WILDDOG_COAP_ADD_DIS_QUERY) + 1;
    }else{
        //query string length  = (.dis=add) + '\0'
        query_len = strlen(WILDDOG_COAP_ADD_DIS_QUERY) + 1;
    }
    new_query = (Wilddog_Str_T*)wmalloc(query_len);
    wilddog_assert(new_query, WILDDOG_ERR_NULL);

    if(NULL != arg->p_url->p_url_query){
        sprintf((char*)new_query, "%s&%s",(const char*)arg->p_url->p_url_query,WILDDOG_COAP_ADD_DIS_QUERY);
    }else{
        sprintf((char*)new_query, "%s",WILDDOG_COAP_ADD_DIS_QUERY);
    }
    //store old query
    tmp = arg->p_url->p_url_query;
    arg->p_url->p_url_query = new_query;

    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_POST;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = arg->p_data;
    send_arg.data_len = arg->d_data_len;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_NOOBSERVE);
    
    //resume old query
    arg->p_url->p_url_query = tmp;
    if(new_query)
        wfree(new_query);

    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_disRemove(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
    Wilddog_Str_T *new_query = NULL;
    int query_len = 0;
    Wilddog_Str_T *tmp = NULL;
    
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data, WILDDOG_ERR_NULL);
    //add disconnect function with query .dis=add
    if(NULL != arg->p_url->p_url_query){
        //query string length  = 
        //(pkt.url->p_url_query) + (&) + (.dis=add) + '\0'
        query_len = strlen((const char*)arg->p_url->p_url_query) + 1 + strlen(WILDDOG_COAP_ADD_DIS_QUERY) + 1;
    }else{
        //query string length  = (.dis=add) + '\0'
        query_len = strlen(WILDDOG_COAP_ADD_DIS_QUERY) + 1;
    }
    new_query = (Wilddog_Str_T*)wmalloc(query_len);
    wilddog_assert(new_query, WILDDOG_ERR_NULL);

    if(NULL != arg->p_url->p_url_query){
        sprintf((char*)new_query, "%s&%s",(const char*)arg->p_url->p_url_query,WILDDOG_COAP_ADD_DIS_QUERY);
    }else{
        sprintf((char*)new_query, "%s",WILDDOG_COAP_ADD_DIS_QUERY);
    }
    //store old query
    tmp = arg->p_url->p_url_query;
    arg->p_url->p_url_query = new_query;

    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_DELETE;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = arg->p_data;
    send_arg.data_len = arg->d_data_len;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_NOOBSERVE);
    
    //resume old query
    arg->p_url->p_url_query = tmp;
    if(new_query)
        wfree(new_query);

    return ret;
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_disCancel(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
    Wilddog_Str_T *new_query = NULL;
    int query_len = 0;
    Wilddog_Str_T *tmp = NULL;
    
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data, WILDDOG_ERR_NULL);
    //add disconnect function with query .dis=add
    if(NULL != arg->p_url->p_url_query){
        //query string length  = 
        //(pkt.url->p_url_query) + (&) + (.dis=rm) + '\0'
        query_len = strlen((const char*)arg->p_url->p_url_query) + 1 + strlen(WILDDOG_COAP_CANCEL_DIS_QUERY) + 1;
    }else{
        //query string length  = (.dis=add) + '\0'
        query_len = strlen(WILDDOG_COAP_CANCEL_DIS_QUERY) + 1;
    }
    new_query = (Wilddog_Str_T*)wmalloc(query_len);
    wilddog_assert(new_query, WILDDOG_ERR_NULL);

    if(NULL != arg->p_url->p_url_query){
        sprintf((char*)new_query, "%s&%s",(const char*)arg->p_url->p_url_query,WILDDOG_COAP_CANCEL_DIS_QUERY);
    }else{
        sprintf((char*)new_query, "%s",WILDDOG_COAP_CANCEL_DIS_QUERY);
    }
    //store old query
    tmp = arg->p_url->p_url_query;
    arg->p_url->p_url_query = new_query;

    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_DELETE;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = arg->p_data;
    send_arg.data_len = arg->d_data_len;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_NOOBSERVE);
    
    //resume old query
    arg->p_url->p_url_query = tmp;
    if(new_query)
        wfree(new_query);

    return ret;
}
//send GET coap://<appId>.wilddogio.com/.off
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_send_offline(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    Wilddog_Coap_Sendpkt_Arg_T send_arg;
    Wilddog_Str_T *new_path = NULL;
    int path_len = 0;
    Wilddog_Str_T *tmp = NULL;
    
    wilddog_assert(data&&arg->protocol&& \
                   arg->p_url&&arg->p_session_info&& \
                   arg->d_session_len&&arg->p_out_data, WILDDOG_ERR_NULL);
    //add disconnect function with query .dis=add
    if(NULL != arg->p_url->p_url_path){
        //path string length  = 
        //(pkt.url->p_url_path) + (/) + (.off) + '\0'
        path_len = strlen((const char*)arg->p_url->p_url_path) + 1 + strlen(WILDDOG_COAP_OFFLINE_PATH) + 1;
    }else{
        //query string length  = (.off) + '\0'
        path_len = strlen(WILDDOG_COAP_OFFLINE_PATH) + 1;
    }
    new_path = (Wilddog_Str_T*)wmalloc(path_len);
    wilddog_assert(new_path, WILDDOG_ERR_NULL);

    if(NULL != arg->p_url->p_url_path){
        sprintf((char*)new_path, "%s&%s",(const char*)arg->p_url->p_url_path,WILDDOG_COAP_OFFLINE_PATH);
    }else{
        sprintf((char*)new_path, "%s",WILDDOG_COAP_OFFLINE_PATH);
    }
    //store old path
    tmp = arg->p_url->p_url_path;
    arg->p_url->p_url_path = new_path;

    send_arg.protocol = arg->protocol;
    send_arg.url = arg->p_url;
    send_arg.code = COAP_REQUEST_GET;
    send_arg.p_session_info = arg->p_session_info;
    send_arg.d_session_len = arg->d_session_len;
    send_arg.data = arg->p_data;
    send_arg.data_len = arg->d_data_len;
    send_arg.isSend = flag;
    send_arg.token = arg->p_message_id;
    send_arg.send_pkt = (Wilddog_Conn_Pkt_Data_T**)arg->p_out_data;
    ret = _wilddog_coap_send_sendPkt(send_arg, TRUE, WILDDOG_COAP_OBSERVE_NOOBSERVE);
    
    //resume old query
    arg->p_url->p_url_path = tmp;
    if(new_path)
        wfree(new_path);

    return ret;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_recv_handlePkt(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    u32 error_code = WILDDOG_ERR_NOTAUTH;
    u32 maxage = 0;
    u32 observe_index = 0;
    coap_pdu_t * pdu;
    u32 payload_len = 0;
    u8 *payload = NULL;
    _Wilddog_Coap_Observe_Data_T *observe_data;
    
    wilddog_assert(data, WILDDOG_ERR_NULL);

    /* we may get the following:
     * 1. error code, 
     * 2. token(already got in getPkt), 
     * 3. maxage 
     * 4. observe index(the 3&4 were used by observer), 
     * 5. path, 
     * 6. blockNum(used by block), 
     * 7. payload
    */

    //pdu stored in p_data
    pdu = (coap_pdu_t *)arg->p_data;

    wilddog_assert(pdu, WILDDOG_ERR_NULL);
    
    //1. get error code
    error_code = _wilddog_coap_getRecvCode(pdu);
    
    //2. get token, we already got it, pass.
    //3. get maxage
    maxage = _wilddog_coap_getRecvMaxage(pdu);
    //4. get observe index
    observe_index = _wilddog_coap_getRecvObserveIndex(pdu);
    //5. get path, only observer may be use path to find the root path, we assume
    //   when root firstly sended, do not send child, when child firstly sended,
    //   send root again, and remove child observe.
    //6. get block number, FIXME: we do not support block [rfc7959]
    //7. get payload
    coap_get_data(pdu,&payload_len,&payload);

    //from observe index, check if the pkt is new or not.
    if(observe_index){
        observe_data = *(_Wilddog_Coap_Observe_Data_T**)arg->p_proto_data;

        wilddog_assert(observe_data, WILDDOG_ERR_NULL);

        if(TRUE == _wilddog_coap_isObserveNew(observe_data->last_index,observe_index)){
            //get new observe, handle it.
            observe_data->last_index = observe_index;
            //FIXME: we do not use maxage to flash observe packet, only store.
            if(maxage)
                observe_data->maxage = maxage;
            observe_data->last_recv_time = _wilddog_getTime();
        }else{
            //get old observe, ignore it.
            error_code = WILDDOG_ERR_IGNORE;
        }
    }
    //error code and payload must tell connect layer
    //send payload to connect layer.
    *(arg->p_out_data) = (u8*)payload;
    *(arg->p_out_data_len) = payload_len;
    return error_code;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_recv_freePkt(void* data, int flag){
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;
    coap_pdu_t* pdu = (coap_pdu_t*)arg->p_data;
    coap_pdu_t  *toSend = NULL;
    u8 type = flag?(COAP_MESSAGE_ACK):(COAP_MESSAGE_RST);
    wilddog_assert(data && pdu, WILDDOG_ERR_NULL);

    //1. if need, send ack to the src of recvPkt
    //2. release recvPkt
    
    //if is con, match send ack ,other send rst
    if(COAP_MESSAGE_CON == pdu->hdr->type){
        toSend = coap_pdu_init(type,0,pdu->hdr->id,WILDDOG_PROTO_MAXSIZE);
        if(toSend){
            coap_add_token(toSend,pdu->hdr->token_length,pdu->hdr->token);
            //we don't care send success or not
            _wilddog_sec_send(arg->protocol,toSend->hdr,toSend->length);
            coap_delete_pdu(toSend);
        }
    }

    //we free recvPkt
    coap_delete_pdu(pdu);
    
    return WILDDOG_ERR_NOERR;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_coap_recv_getPkt(void* data, int flag){
    u8* recv_data = NULL;
    int res = 0;
    coap_pdu_t * pdu;
    int len;
    Wilddog_Proto_Cmd_Arg_T * arg = (Wilddog_Proto_Cmd_Arg_T*)data;

    wilddog_assert(data, WILDDOG_ERR_NULL);
    //1. recv from socket
    recv_data = _wilddog_coap_mallocRecvBuffer(arg->protocol);
    wilddog_assert(recv_data, WILDDOG_ERR_NULL);
    res = _wilddog_sec_recv(arg->protocol,(void*)recv_data,(s32)WILDDOG_PROTO_MAXSIZE);
    if(res <= 0 || res > WILDDOG_PROTO_MAXSIZE){
        _wilddog_coap_freeRecvBuffer(arg->protocol,recv_data);
        //wilddog_debug_level(WD_DEBUG_LOG, "Receive failed, error = %d",res);
        return WILDDOG_ERR_RECVTIMEOUT;
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
    //3. send pkt to connect layer.
    //!!!Notice, we assume token is WILDDOG_COAP_TOKEN_LEN bytes!
    //more than WILDDOG_COAP_TOKEN_LEN bytes will be ignored!
    len = pdu->hdr->token_length > WILDDOG_COAP_TOKEN_LEN? \
            (WILDDOG_COAP_TOKEN_LEN):(pdu->hdr->token_length);
    memcpy((u8*)arg->p_message_id,pdu->hdr->token, len);
    _wilddog_coap_freeRecvBuffer(arg->protocol,recv_data);
    //send pdu to connect layer to store.
    *(arg->p_out_data) = (u8*)pdu;
    *(arg->p_out_data_len) = pdu->length;
    return WILDDOG_ERR_NOERR;
}

/* protocol :: coap  interface */
Wilddog_Func_T _wilddog_protocol_funcTable[WD_PROTO_CMD_MAX + 1] = 
{
    (Wilddog_Func_T) _wilddog_coap_initSession,//init
    (Wilddog_Func_T)_wilddog_coap_reconnect,
    (Wilddog_Func_T)_wilddog_coap_send_ping,//ping
    (Wilddog_Func_T)_wilddog_coap_send_retransmit,//retransmit
    (Wilddog_Func_T)_wilddog_coap_send_getValue,//get
    (Wilddog_Func_T)_wilddog_coap_send_setValue,//set
    (Wilddog_Func_T)_wilddog_coap_send_push,//push
    (Wilddog_Func_T)_wilddog_coap_send_remove,//remove
    (Wilddog_Func_T)_wilddog_coap_send_addObserver,//on
    (Wilddog_Func_T)_wilddog_coap_send_removeObserver,//off
    (Wilddog_Func_T)_wilddog_coap_send_disSetValue,//dis set
    (Wilddog_Func_T)_wilddog_coap_send_disPush,//dis push
    (Wilddog_Func_T)_wilddog_coap_send_disRemove,//dis remove
    (Wilddog_Func_T)_wilddog_coap_send_disCancel,//dis cancel
    (Wilddog_Func_T)NULL,//online, the same as init
    (Wilddog_Func_T)_wilddog_coap_send_offline,//offline
    (Wilddog_Func_T)_wilddog_coap_recv_getPkt,//get pkt
    (Wilddog_Func_T)_wilddog_coap_recv_freePkt,//free pkt
    (Wilddog_Func_T)_wilddog_coap_recv_handlePkt,//handle pkt
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

    if(_wilddog_protocol_funcTable[cmd]){
        return (_wilddog_protocol_funcTable[cmd])(p_args,flags);
    }
    else{
        wilddog_debug_level(WD_DEBUG_ERROR, "Cannot find function %d!",cmd);
        return WILDDOG_ERR_NULL;
    }
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
    p_conn->p_protocol->host = NULL;
    p_conn->p_protocol->user_data = NULL;
    
    wfree(p_conn->p_protocol);
    return WILDDOG_ERR_NOERR;
}

