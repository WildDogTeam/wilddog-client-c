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

#define WILDDOG_COAP_CMD_INIT 0
#define WILDDOG_COAP_CMD_GET 1
#define WILDDOG_COAP_CMD_SET 2
#define WILDDOG_COAP_CMD_PSH 3
#define WILDDOG_COAP_CMD_RMV 4
#define WILDDOG_COAP_CMD_ON  5
#define WILDDOG_COAP_CMD_OFF 5

typedef struct WILDDOG_COAP_PKT_T{
    size_t size;
    u8 command;
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

#endif /* __WILDDOG_CONN_COAP_H_ */

