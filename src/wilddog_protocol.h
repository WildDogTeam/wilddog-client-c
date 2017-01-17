
#ifndef _WILDDOG_PROTOCOL_H_
#define _WILDDOG_PROTOCOL_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "wilddog.h"
#include "wilddog_ct.h"
#include "wilddog_url_parser.h"

#define WILDDOG_PROTO_RECV_BUF_NUM 1

typedef struct _WILDDOG_PROTO_RECV_STRUCT
{   
    u8 data[WILDDOG_PROTO_MAXSIZE]; 
    u8 isused;
}_wilddog_Proto_Recv_T;

typedef struct WILDDOG_PROTOCOL_T{
    Wilddog_Str_T *host;
    int socketFd;
    Wilddog_Address_T addr;
    Wilddog_Func_T callback;
    _wilddog_Proto_Recv_T recv_buf[WILDDOG_PROTO_RECV_BUF_NUM];
}Wilddog_Protocol_T;
typedef struct WILDDOG_PROTO_CMD_ARG_T{
    Wilddog_Protocol_T* protocol;
    u8 *p_data;//input 
    u32 d_data_len;
    Wilddog_Url_T * p_url;
    u32 * p_message_id;//we assume message id in u32
    u8 **p_out_data;//output
    u32 *p_out_data_len;
    u8 **p_proto_data;//proto special, in/out
}Wilddog_Proto_Cmd_Arg_T;
typedef enum WILDDOG_PROTO_CMD_T{
    WD_PROTO_CMD_SEND_SESSION_INIT = 0,
    WD_PROTO_CMD_SEND_SESSION_PING,
    WD_PROTO_CMD_SEND_GET,
    WD_PROTO_CMD_SEND_SET,
    WD_PROTO_CMD_SEND_PUSH,
    WD_PROTO_CMD_SEND_REMOVE,
    WD_PROTO_CMD_SEND_ON,
    WD_PROTO_CMD_SEND_OFF,
    WD_PROTO_CMD_SEND_DIS_SET,
    WD_PROTO_CMD_SEND_DIS_PUSH,
    WD_PROTO_CMD_SEND_DIS_REMOVE,
    WD_PROTO_CMD_SEND_DIS_CANCEL,
    WD_PROTO_CMD_SEND_ONLINE,
    WD_PROTO_CMD_SEND_OFFLINE,
    WD_PROTO_CMD_RECV_GETPKT,
    WD_PROTO_CMD_RECV_FREEPKT,
    WD_PROTO_CMD_RECV_HANDLEPKT,
    WD_PROTO_CMD_MAX
}Wilddog_Proto_Cmd_T;
extern Wilddog_Protocol_T * _wilddog_protocol_init(void *p_conn);
extern Wilddog_Return_T _wilddog_protocol_deInit(void *p_conn);
#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_PROTOCOL_H_*/

