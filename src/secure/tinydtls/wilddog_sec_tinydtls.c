/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_conn_sec_dtls.c
 *
 * Description: dtls functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.4        lsx             2015-07-15  Create file.
 * 0.4.4        Jimmy.Pan       2015-07-24  fix bugs, porting to wiced.
 *
 */


#include "tinydtls.h" 
#include "dtls_config.h"
/* This is needed for apple */
#define __APPLE_USE_RFC_3542

#include <stdio.h>
#if defined(WILDDOG_PORT_TYPE_QUCETEL)
#include "wilddog.h"
#else
#include <string.h>
#endif

#ifndef WILDDOG_PORT_TYPE_MXCHIP
#include <unistd.h>
#endif
#include <ctype.h>
#if !defined(WILDDOG_PORT_TYPE_WICED) && \
    !defined(WILDDOG_PORT_TYPE_QUCETEL) && \
    !defined(WILDDOG_PORT_TYPE_MXCHIP)

#include <netinet/in.h>
#endif

#include "global.h" 
#include "debug.h" 
#include "dtls.h" 
#include "session.h" 

#include "wilddog.h"
#include "wilddog_port.h"
#include "wilddog_sec.h"

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else
#define UNUSED_PARAM
#endif /* __GNUC__ */

typedef enum  _TINYDTLS_STATE_S{
    _TINYDTLS_STATE_WARNING = 1,
    _TINYDTLS_STATE_ERROR = 2,
    _TINYDTLS_STATE_CONNECT = 3
}TinyDtls_State_S;
typedef struct WILDDOG_SEC_TINYDTLS_T{
    dtls_context_t *dtls_context;
    session_t dst;
    unsigned short d_dtlstState;
    u8 *p_recvbuf;
    u32 d_recvlen;
    u8 d_recvFig;
    u8 reserved1;
    u8 reserved2;
    u8 reserved3;
}Wilddog_Sec_Tinydtls_T;

/*publick key*/
STATIC const unsigned char server_pub_key_x[] = 
{
    0x20, 0x8F, 0xB9, 0xBB, 0x42, 0xE8, 0x71, 0xEE,
    0x2D, 0x69, 0x0B, 0x8D, 0x35, 0x0B, 0xEA, 0x9E,
    0x4D, 0xA7, 0x1E, 0x93, 0x97, 0x6E, 0x6B, 0x89,
    0x17, 0x89, 0xC7, 0x80, 0x31, 0x62, 0xA0, 0x4C
};

STATIC const unsigned char server_pub_key_y[] = 
{
    0x6B, 0x54, 0x5C, 0x4B, 0x3A, 0x95, 0x38, 0x92,
    0x96, 0x9E, 0x0C, 0x86, 0x9B, 0x42, 0x51, 0x18,
    0x8E, 0x9B, 0x19, 0xd7, 0x8B, 0xA8, 0xFA, 0x3E,
    0x0D, 0x52, 0x9C, 0xDD, 0xA6, 0x34, 0xE7, 0xEA,
};

STATIC const unsigned char ecdsa_priv_key[] = 
{
    0x41, 0xC1, 0xCB, 0x6B, 0x51, 0x24, 0x7A, 0x14,
    0x43, 0x21, 0x43, 0x5B, 0x7A, 0x80, 0xE7, 0x14,
    0x89, 0x6A, 0x33, 0xBB, 0xAD, 0x72, 0x94, 0xCA,
    0x40, 0x14, 0x55, 0xA1, 0x94, 0xA9, 0x49, 0xFA
};

STATIC const unsigned char ecdsa_pub_key_x[] = 
{
    0x36, 0xDF, 0xE2, 0xC6, 0xF9, 0xF2, 0xED, 0x29,
    0xDA, 0x0A, 0x9A, 0x8F, 0x62, 0x68, 0x4E, 0x91,
    0x63, 0x75, 0xBA, 0x10, 0x30, 0x0C, 0x28, 0xC5,
    0xE4, 0x7C, 0xFB, 0xF2, 0x5F, 0xA5, 0x8F, 0x52
};

STATIC const unsigned char ecdsa_pub_key_y[] = 
{
    0x71, 0xA0, 0xD4, 0xFC, 0xDE, 0x1A, 0xB8, 0x78,
    0x5A, 0x3C, 0x78, 0x69, 0x35, 0xA7, 0xCF, 0xAB,
    0xE9, 0x3F, 0x98, 0x72, 0x09, 0xDA, 0xED, 0x0B,
    0x4F, 0xAB, 0xC3, 0x6F, 0xC7, 0x72, 0xF8, 0x29
};

#ifdef DTLS_PSK
#if !defined(WILDDOG_PORT_TYPE_QUCETEL) && !defined(WILDDOG_PORT_TYPE_MXCHIP)
ssize_t read_from_file(char *arg, unsigned char *buf, size_t max_buf_len)
{
  FILE *f;
  ssize_t result = 0;

  f = fopen(arg, "r");
  if (f == NULL)
    return -1;

  while (!feof(f)) {
    size_t bytes_read;
    bytes_read = fread(buf, 1, max_buf_len, f);
    if (ferror(f)) {
      result = -1;
      break;
    }

    buf += bytes_read;
    result += bytes_read;
    max_buf_len -= bytes_read;
  }

  fclose(f);
  return result;
}
#endif

/* The PSK information for DTLS */
//jimmy: Useful or not??? Only get_psk_info used them, but their value never changed.
// so we cannot retrieve key.
#define PSK_ID_MAXLEN 256
#define PSK_MAXLEN 256
static unsigned char psk_id[PSK_ID_MAXLEN];
static size_t psk_id_length = 0;
static unsigned char psk_key[PSK_MAXLEN];
static size_t psk_key_length = 0;

/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
STATIC int get_psk_info
    (
    struct dtls_context_t *ctx UNUSED_PARAM,
    const session_t *session UNUSED_PARAM,
    dtls_credentials_type_t type,
    const unsigned char *id, 
    size_t id_len,
    unsigned char *result, 
    size_t result_length
    ) 
{
    switch (type)
    {
        case DTLS_PSK_IDENTITY:
            if (id_len) 
            {
                dtls_debug("got psk_identity_hint: '%.*s'\n", id_len, id);
            }

            if (result_length < psk_id_length)
            {
                dtls_warn("cannot set psk_identity -- buffer too small\n");
                return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
            }

            memcpy(result, psk_id, psk_id_length);
            return psk_id_length;
        case DTLS_PSK_KEY:
            if (id_len != psk_id_length || memcmp(psk_id, id, id_len) != 0)
            {
                dtls_warn("PSK for unknown id requested, exiting\n");
                return dtls_alert_fatal_create(DTLS_ALERT_ILLEGAL_PARAMETER);
            } 
            else if (result_length < psk_key_length) 
            {
                dtls_warn("cannot set psk -- buffer too small\n");
                return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
            }

            memcpy(result, psk_key, psk_key_length);
            return psk_key_length;
        default:
            dtls_warn("unsupported request type: %d\n", type);
    }

    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
}
#endif /* DTLS_PSK */

#ifdef DTLS_ECC
STATIC int get_ecdsa_key
    (
    struct dtls_context_t *ctx,
    const session_t *session,
    const dtls_ecdsa_key_t **result
    )
{
    static const dtls_ecdsa_key_t ecdsa_key = 
    {
        .curve = DTLS_ECDH_CURVE_SECP256R1,
        .priv_key = ecdsa_priv_key,
        .pub_key_x = ecdsa_pub_key_x,
        .pub_key_y = ecdsa_pub_key_y
    };

    *result = &ecdsa_key;
    return 0;
}
void os_printf_nchar(unsigned char *p,int n)
{
    int i=0;
    if(!p)
        return;
    for(i=0;i<n;i++){
        printf("[%0x]",p[i]);
    }
    printf("\n");
}
STATIC int verify_ecdsa_key
    (
    struct dtls_context_t *ctx,
    const session_t *session,
    const unsigned char *other_pub_x,
    const unsigned char *other_pub_y,
    size_t key_size
    )
{
#if 0
  printf("<><>other_pub_x\n");
  os_printf_nchar(other_pub_x,32);
    printf("<><>other_pub_y\n");
  os_printf_nchar(other_pub_y,32);
#endif  
  if(memcmp(server_pub_key_x,other_pub_x,32 ) != 0 || 
     memcmp(server_pub_key_y,other_pub_y,32 ) != 0
    )
        return -1;

  return 0;
}
#endif /* DTLS_ECC */
STATIC int read_from_peer
    (
    struct dtls_context_t *ctx, 
    session_t *session, 
    uint8 *data, size_t len
    )
{
    size_t i,readlen;
    Wilddog_Protocol_T *protocol = NULL;
    Wilddog_Sec_Tinydtls_T * sec_session = NULL;

    protocol = (Wilddog_Protocol_T *)dtls_get_app_data(ctx);
    wilddog_assert(protocol, -1);
    
    sec_session = (Wilddog_Sec_Tinydtls_T*)protocol->user_data;
    wilddog_assert(sec_session, -1);

    if(sec_session->p_recvbuf == NULL)
        return 0;
    sec_session->d_recvFig = 1;
    readlen = (sec_session->d_recvlen > len)?len:sec_session->d_recvlen;
    for (i = 0; i < len; i++)
    {
        sec_session->p_recvbuf[i] = data[i];
    }
    sec_session->d_recvlen = readlen;

    return 0;
}

STATIC int send_to_peer
    (
    struct dtls_context_t *ctx,
    session_t *session, 
    uint8 *data, 
    size_t len
    ) 
{
    Wilddog_Protocol_T *protocol;
    
    int res = 0;
    Wilddog_Address_T addr_inSend;
    protocol = (Wilddog_Protocol_T *)dtls_get_app_data(ctx);

    wilddog_assert(protocol, -1);
    
    addr_inSend.len = session->size;
#if defined(WILDDOG_PORT_TYPE_WICED) || \
    defined(WILDDOG_PORT_TYPE_QUCETEL) || \
    defined(WILDDOG_PORT_TYPE_MXCHIP)
    
    addr_inSend.len = session->addr.len;
    addr_inSend.port = session->addr.port;
    memcpy(addr_inSend.ip,&session->addr.ip,session->size);
#else
    addr_inSend.port = session->addr.sin.sin_port;
    memcpy(addr_inSend.ip,&session->addr.sin.sin_addr,session->size);
#endif
    res = wilddog_send(protocol->socketFd,&addr_inSend,data,len);

    return res;
}

STATIC int dtls_handle_read(struct dtls_context_t *ctx)
{
    Wilddog_Protocol_T *protocol = NULL;
    Wilddog_Sec_Tinydtls_T * session = NULL;
#define MAX_READ_BUF 2000
    static uint8 buf[MAX_READ_BUF];
    int len;

    protocol = (Wilddog_Protocol_T *)dtls_get_app_data(ctx);
    wilddog_assert(protocol, -1);
    
    session = (Wilddog_Sec_Tinydtls_T*)protocol->user_data;
    wilddog_assert(session, -1);
    memset(buf, 0, MAX_READ_BUF);
    len = wilddog_receive(protocol->socketFd,&protocol->addr,buf,MAX_READ_BUF,WILDDOG_RECEIVE_TIMEOUT);
    if (len < 0)
    {
        return 0;// return -1;
    }
    else if(len > 0)
    {
        dtls_dsrv_log_addr(DTLS_LOG_DEBUG, "peer", &session->dst);
        dtls_debug_dump("bytes from peer", buf, len);
        return dtls_handle_message(ctx, &session->dst, buf, len);
    }
    return 0;
}

int handle_dtls_event
    (
    dtls_context_t *ctx, 
    session_t *session, 
    dtls_alert_level_t level, 
    unsigned short code
    )
{
    Wilddog_Protocol_T *protocol = NULL;
    Wilddog_Sec_Tinydtls_T * sec_session = NULL;

    protocol = (Wilddog_Protocol_T *)dtls_get_app_data(ctx);
    wilddog_assert(protocol, -1);
    
    sec_session = (Wilddog_Sec_Tinydtls_T*)protocol->user_data;
    wilddog_assert(sec_session, -1);

    sec_session->d_dtlstState = level;
    if( code == DTLS_EVENT_CONNECTED)
        sec_session->d_dtlstState = _TINYDTLS_STATE_CONNECT;

    return 0;
}

STATIC dtls_handler_t cb =
{
    .write = send_to_peer,
    .read  = read_from_peer,
    .event = handle_dtls_event,
#ifdef DTLS_PSK
    .get_psk_info = get_psk_info,
#endif /* DTLS_PSK */
#ifdef DTLS_ECC
    .get_ecdsa_key = get_ecdsa_key,
    .verify_ecdsa_key = verify_ecdsa_key
#endif /* DTLS_ECC */
};
/*@ wilddog dtls api
*/
#define DTLS_CLIENT_CMD_CLOSE "client:close"
#define DTLS_CLIENT_CMD_RENEGOTIATE "client:renegotiate"
#define WD_DEBUG_DTLS DTLS_LOG_EMERG // DTLS_LOG_EMERG for non  DTLS_LOG_DEBUG for all

STATIC int _wilddog_sec_reconnectInner
    (
    Wilddog_Sec_Tinydtls_T *wd_sec, 
    int timeout,
    int reconnectNum
    )
{
    int i;
    int res = 0;

    wilddog_assert(wd_sec, -1);    
    wilddog_assert(wd_sec->dtls_context, -1);

    for(i = 0; i < reconnectNum; i++)
    {
        int tmp_cnt = 0;
        dtls_connect(wd_sec->dtls_context, &wd_sec->dst);
        
        while( tmp_cnt++ < 100 )
        {
            /*dtls error.*/
            if( wd_sec->d_dtlstState == _TINYDTLS_STATE_ERROR )
                break;
            else /* connect finish.*/
            if( wd_sec->d_dtlstState == _TINYDTLS_STATE_CONNECT)
                return 0;
            /* read receive.*/
            res = dtls_handle_read(wd_sec->dtls_context);
            if(res < 0)
                break;        
        }
    }

    return -1;
}

Wilddog_Return_T _wilddog_sec_reconnect(Wilddog_Protocol_T *protocol,int retryNum)
{
    int res = 0;
    Wilddog_Sec_Tinydtls_T *session = (Wilddog_Sec_Tinydtls_T*)protocol->user_data;
    
    wilddog_assert(protocol&&session, -1);

    res = _wilddog_sec_reconnectInner(session,WILDDOG_RETRANSMITE_TIME,retryNum);
    if(res >= 0)
        return res;
        
    _wilddog_sec_deinit(protocol);
    res = _wilddog_sec_init(protocol);
    return res;
}

/* do not malloc session*/
STATIC int _wilddog_sec_setSession
    (
    Wilddog_Protocol_T *protocol
    )
{
    Wilddog_Sec_Tinydtls_T *session = (Wilddog_Sec_Tinydtls_T*)protocol->user_data;
    
    wilddog_assert(protocol&&session, -1);

#if defined(WILDDOG_PORT_TYPE_WICED) || \
    defined(WILDDOG_PORT_TYPE_QUCETEL) || \
    defined(WILDDOG_PORT_TYPE_MXCHIP)

    memcpy(&session->dst.addr.ip,protocol->addr.ip,protocol->addr.len);
    session->dst.size = protocol->addr.len;
    session->dst.addr.len = protocol->addr.len;
    session->dst.addr.port = protocol->addr.port;
#else
    session->dst.addr.sin.sin_family = 2;/*AF_INET  ipv4*/
    memcpy(&session->dst.addr.sin.sin_addr,protocol->addr.ip,protocol->addr.len);
    session->dst.size = protocol->addr.len;
    session->dst.addr.sin.sin_port = protocol->addr.port;
#endif
    return 0;
}
/*
 * Function:    _wilddog_sec_send
 * Description: tinyDtls send function
 * Input:   p_data: The buffer which store the sending data
 *          len: The length of the wanting send data
 * Output:      N/A
 * Return:      The length of the actual sending data
*/
Wilddog_Return_T _wilddog_sec_send
    (
    Wilddog_Protocol_T *protocol,
    void* p_data, 
    s32 len
    )
{       
    uint8 *p_buf = p_data;
    s32 sendLen = 0;
    int res = 0;
    Wilddog_Sec_Tinydtls_T * session = (Wilddog_Sec_Tinydtls_T*)protocol->user_data;
    
    wilddog_assert(protocol&&session, -1);

    _wilddog_sec_setSession(protocol);
    
    if(session->d_dtlstState != _TINYDTLS_STATE_CONNECT)
        return WILDDOG_ERR_SENDERR;
    
    while(sendLen < len)
    {       
        res = dtls_write(session->dtls_context, &session->dst, \
                         (uint8 *)(p_buf + sendLen), len - sendLen);
        if (res >= 0 )
        {
            sendLen += res;
        }
        else 
        {
            break;
        }
    }
    return sendLen;
}

/*
 * Function:    _wilddog_sec_recv
 * Description: No security recv function
 * Input:       len: The length of the wanting receive data
 * Output:      p_data: The buffer which store the receiving data
 * Return:      The length of the actual receiving data
*/
#define WD_DEAULT_RETRY_TIME 3

int _wilddog_sec_recv
    (
    Wilddog_Protocol_T *protocol,
    void* p_data, 
    s32 len
    )
{
    int res = 0;
    Wilddog_Sec_Tinydtls_T * session = (Wilddog_Sec_Tinydtls_T*)protocol->user_data;

    wilddog_assert(protocol&&session, -1);
    
    _wilddog_sec_setSession(protocol);
    
    session->p_recvbuf = p_data;
    session->d_recvlen = len;
    session->d_recvFig = 0;
    /* decode recv */
    
    res = dtls_handle_read(session->dtls_context);
    if(session->d_recvFig)
        res = session->d_recvlen;

    return res;
}
#undef WD_DEAULT_RETRY_TIME

/*
 * Function:    _wilddog_sec_init
 * Description: Initialize no security session
 * Input:    p_host: Remote Host   
           d_port: Remote Server's port
 * Output:  N/A
 * Return:  Success: 0 else init failure
*/
Wilddog_Return_T _wilddog_sec_init(Wilddog_Protocol_T *protocol)
{
    int res = 0;
    int sec_int_cnt = 0;
    Wilddog_Sec_Tinydtls_T * session = NULL;
    
    wilddog_assert(protocol, WILDDOG_ERR_NULL);
    
    wilddog_openSocket(&protocol->socketFd);
    res = _wilddog_sec_getHost(&protocol->addr,protocol->host);
    if(res < 0)
        return res;
    if(protocol->user_data){
        wfree(protocol->user_data);
        protocol->user_data = NULL;
    }
    protocol->user_data = wmalloc(sizeof(Wilddog_Sec_Tinydtls_T));
    if(!protocol->user_data){
        wilddog_debug_level(WD_DEBUG_ERROR,"Malloc failed!");
        return WILDDOG_ERR_NULL;
    }

    session = (Wilddog_Sec_Tinydtls_T*)protocol->user_data;
    
    _wilddog_sec_setSession(protocol);
    dtls_init();
#ifndef WILDDOG_PORT_TYPE_WICED
    tiny_dtls_set_log_level(WD_DEBUG_DTLS);
#endif
    session->dtls_context = dtls_new_context(protocol);

    if(session->dtls_context == NULL){
        return -1;
    }

    /*@ register dtls cb*/
    dtls_set_handler(session->dtls_context, &cb);

    /*@ Establishes a DTLS channel with the specified remote peer dst.  
    **@ start client Hello
    */

    res = dtls_connect(session->dtls_context, &session->dst);
    
    while(sec_int_cnt++ < 100)
    {
        if(session->d_dtlstState == _TINYDTLS_STATE_CONNECT)
            return 0;

        /*dtls error */
        if(session->d_dtlstState ==  _TINYDTLS_STATE_ERROR)
                break;

        res = dtls_handle_read(session->dtls_context);
        if(res < 0)
            break;
    }

    return res;
}

/*
 * Function:    _wilddog_sec_deinit
 * Description: close soket.Destroy no security session
 * Input:       N/A
 * Output:      N/A
 * Return:      Success: 0
*/
Wilddog_Return_T _wilddog_sec_deinit(Wilddog_Protocol_T *protocol)
{
    Wilddog_Sec_Tinydtls_T * session = (Wilddog_Sec_Tinydtls_T *)protocol->user_data;

    wilddog_assert(protocol&&session, WILDDOG_ERR_NULL);
    
    /* send terminate alert*/
    dtls_free_context(session->dtls_context);
    if(protocol->socketFd != -1)
        wilddog_closeSocket(protocol->socketFd);
    
    protocol->socketFd = -1;
    memset(&protocol->addr,0,sizeof(protocol->addr));
    wfree(session);
    protocol->user_data = NULL;

    return WILDDOG_ERR_NOERR;
}

