/*
 * Wilddog.h
 *
 *  Created on: 2015-12-12
 *      Author: x
 */

#ifndef __WILDDOG_CONN_COAP_H_
#define __WILDDOG_CONN_COAP_H_

#include "wilddog.h"
#include "pdu.h"

#define COAP_TOKENLEN   4
#define PACKAGE_OPTION_HEADLEN  (20)
#define PACKAGE_OPTION_PATHLEN  (4)


typedef enum _CM_RESPONSESTATE_T{
    _CM_RESPONSES_ACK,
    _CM_RESPONSES_REST,
}_CM_ResponsesState_T;

typedef enum PROTOCOL_CMD_T{
    _PROTOCOL_CMD_INIT,
    _PROTOCOL_CMD_DEINIT,
    
    _PROTOCOL_CMD_CREAT,
    _PROTOCOL_CMD_DESTORY,
    _PROTOCOL_CMD_ADD_HOST,
    _PROTOCOL_CMD_ADD_PATH,
    _PROTOCOL_CMD_ADD_QUERY,
    _PROTOCOL_CMD_ADD_DATA,
    
    _PROTOCOL_CMD_AUTHUPDATA,
    _PROTOCOL_CMD_SEND,
    _PROTOCOL_CMD_RECV,
    _PROTOCOL_CMD_MAX
}Protocol_cmd_t;

typedef struct PROTOCOL_ARGINIT_T{
    Wilddog_Str_T *p_host;
    Wilddog_Func_T f_handleRespond;
    u16 d_port;
}Protocol_ArgInit_T;

typedef struct PROTOCOL_PKG_CREATARG{
    Wilddog_Conn_Cmd_T cmd;
    u16 d_index;
    u16 d_packageLen;
    u32 d_token;
}Protocol_Pkg_creatArg_T;
typedef struct PROTOCOL_PKG_OPTIONARG_T{
   void *p_pkg;
   void *p_options;
}Protocol_Pkg_OptionArg_T;

typedef struct PROTOCOL_PKG_PAYLOADARG_T{
   void *p_pkg;
   void *p_payload;
   u32 d_payloadLen;
   
}Protocol_Pkg_PayloadArg_T;
typedef struct PROTOCOL_PKG_AUTHARG_T{
    u8 *p_auth;
    void *p_pkg;
    
}Protocol_Pkg_AuthArg_T;

typedef struct PROTOCOL_ARG_SEND_T{
    
    Wilddog_Conn_Cmd_T cmd;
    Wilddog_Url_T *p_url;

	u32 d_token;
    u32 d_payloadlen;
    u8 *p_payload;  
    void *p_user_arg;

    
    u16 d_messageid;
    
}Protocol_Arg_Send_T;

typedef struct PROTOCOL_RECVARG_T{

    u32 d_observerIndx;
    u32 d_maxAge;   
	u32 d_token;
	
	u8 *p_recvData;
	u32 d_recvDataLen;
	
    u8 d_isObserver;   
    u8 d_blockIdx;
	u8 d_blockNum;
    u8 err;
	
}Protocol_SendArg_T;

/* protocol application function */
int WD_SYSTEM _wilddog_protocol_ioctl
    (
    Protocol_cmd_t cmd,
    void *p_args,
    int flags
    );


#endif /* __WILDDOG_CONN_COAP_H_ */

