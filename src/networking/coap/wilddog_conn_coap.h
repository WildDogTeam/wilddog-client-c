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

#define COAP_TOKENLEN   4
#define PROTOCOL_QUERY_HEADLEN  (8)
#define PROTOCOL_PATH_HEADLEN  (8)



typedef enum PROTOCOL_CMD_T{
    _PROTOCOL_CMD_INIT,
    _PROTOCOL_CMD_DEINIT,

    
    _PROTOCOL_CMD_COUNTSIZE,
    _PROTOCOL_CMD_CREAT,
    _PROTOCOL_CMD_DESTORY,
    _PROTOCOL_CMD_ADD_HOST,
    _PROTOCOL_CMD_ADD_PATH,
    _PROTOCOL_CMD_ADD_QUERY,
    _PROTOCOL_CMD_ADD_OBSERVER,
    _PROTOCOL_CMD_ADD_DATA,
    
    _PROTOCOL_CMD_AUTHUPDATA,
    _PROTOCOL_CMD_SEND,
    _PROTOCOL_CMD_RECV,
    _PROTOCOL_CMD_MAX
}Protocol_cmd_t;

typedef struct PROTOCOL_ARG_INIT_T{
    Wilddog_Str_T *p_host;
    Wilddog_Func_T f_handleRespond;
    u16 d_port;
}Protocol_Arg_Init_T;

typedef struct PROTOCOL_ARG_CREAT_T{
    u8 cmd;
    u16 d_index;
    u16 d_packageLen;
    u32 d_token;
    
}Protocol_Arg_Creat_T;
typedef struct PROTOCOL_ARG_OPTION_T{
   void *p_pkg;
   void *p_options;
}Protocol_Arg_Option_T;

typedef struct PROTOCOL_ARG_PAYLOADA_T{
   void *p_pkg;
   void *p_payload;
   u32 d_payloadLen;
   
}Protocol_Arg_Payload_T;


typedef struct PROTOCOL_ARG_SEND_T{
    
    u8 cmd;
    Wilddog_Url_T *p_url;

	u32 d_token;
    u32 d_payloadlen;
    u8 *p_payload;  
    void *p_user_arg;

    
    u16 d_messageid;
    
}Protocol_Arg_Send_T;

typedef struct PROTOCOL_ARG_AUTHARG_T{
    void *p_pkg;
    u8 *p_newAuth;
    int d_newAuthLen;
    
}Protocol_Arg_Auth_T;
typedef struct PROTOCOL_ARG_COUNTSIZE_T{
    u8 *p_host;
    u8 *p_path;
    u8 *p_query;
    
    u32 d_payloadLen;
    u32 d_extendLen;
}Protocol_Arg_CountSize_T;


/* protocol application function */
extern size_t WD_SYSTEM _wilddog_protocol_ioctl
    (
    Protocol_cmd_t cmd,
    void *p_args,
    int flags
    );


#endif /* __WILDDOG_CONN_COAP_H_ */

