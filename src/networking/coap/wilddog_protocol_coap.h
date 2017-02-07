/*
 * Wilddog.h
 *
 *  Created on: 2015-12-12
 *      Author: x
 */

#ifndef __WILDDOG_CONN_COAP_H_
#define __WILDDOG_CONN_COAP_H_

#include "pdu.h"
#include "wilddog.h"
#include "wilddog_url_parser.h"
#include "wilddog_conn.h"

#define WILDDOG_COAP_TOKEN_LEN 4
#define WILDDOG_COAP_SESSION_PATH ".cs"
#define WILDDOG_COAP_SESSION_QUERY WILDDOG_COAP_SESSION_PATH
#define WILDDOG_COAP_SESSION_RST_PATH ".rst"
#define WILDDOG_COAP_SESSION_PING_PATH ".ping"
#define WILDDOG_COAP_ADD_DIS_QUERY ".dis=add"
#define WILDDOG_COAP_CANCEL_DIS_QUERY ".dis=rm"
#define WILDDOG_COAP_OFFLINE_PATH ".off"

typedef enum{
    WILDDOG_COAP_OBSERVE_NOOBSERVE = 0,
    WILDDOG_COAP_OBSERVE_ON,
    WILDDOG_COAP_OBSERVE_OFF
}Wilddog_Coap_Observe_Stat_T;

typedef struct WILDDOG_COAP_PKT_T{
    size_t size;
    u8 version;   /* protocol version */
    u8 type;      /* type flag */
    u8 code;          /* request method (value 1--10) or response code (value 40-255) */
    u16 token_length;  /* length of Token */
    u16 mid;        /* message id */
    u8 token[WILDDOG_COAP_TOKEN_LEN];
    Wilddog_Url_T * url;
    u8 *data;
    u16 data_len;
}Wilddog_Coap_Pkt_T;

typedef struct WILDDOG_COAP_SENDPKT_ARG_T{
    Wilddog_Protocol_T * protocol;
    Wilddog_Url_T* url;
    u8  code;
    u8* p_session_info;
    u32 d_session_len;
    u8* data;
    int data_len;
    int isSend;
    u32 *token;
    Wilddog_Conn_Pkt_Data_T** send_pkt;
}Wilddog_Coap_Sendpkt_Arg_T;
#endif /* __WILDDOG_CONN_COAP_H_ */

