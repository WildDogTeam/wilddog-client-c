
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
    void *user_data;
    Wilddog_Func_T callback;
    _wilddog_Proto_Recv_T recv_buf[WILDDOG_PROTO_RECV_BUF_NUM];
}Wilddog_Protocol_T;
typedef struct WILDDOG_PROTO_CMD_ARG_T{
    Wilddog_Protocol_T* protocol;
    u8 *p_data;//input 
    u32 d_data_len;
    u8* p_session_info;
    u32 d_session_len;
    Wilddog_Url_T * p_url;
    u32 * p_message_id;//we assume message id in u32
    u8 **p_out_data;//output
    u32 *p_out_data_len;
    u8 **p_proto_data;//proto special, in/out
}Wilddog_Proto_Cmd_Arg_T;
typedef enum WILDDOG_PROTO_CMD_T{
    WD_PROTO_CMD_SEND_SESSION_INIT = 0,
    WD_PROTO_CMD_SEND_RECONNECT,// 1
    WD_PROTO_CMD_SEND_PING,// 2
    WD_PROTO_CMD_SEND_RETRANSMIT,// 3
    WD_PROTO_CMD_SEND_GET, // 4
    WD_PROTO_CMD_SEND_SET,// 5
    WD_PROTO_CMD_SEND_PUSH,// 6
    WD_PROTO_CMD_SEND_REMOVE,// 7
    WD_PROTO_CMD_SEND_ON,// 8
    WD_PROTO_CMD_SEND_OFF,// 9
    WD_PROTO_CMD_SEND_DIS_SET, // 10
    WD_PROTO_CMD_SEND_DIS_PUSH,// 11
    WD_PROTO_CMD_SEND_DIS_REMOVE,// 12
    WD_PROTO_CMD_SEND_DIS_CANCEL,// 13
    WD_PROTO_CMD_SEND_ONLINE,// 14
    WD_PROTO_CMD_SEND_OFFLINE,// 15
    WD_PROTO_CMD_RECV_GETPKT,// 16
    WD_PROTO_CMD_RECV_FREEPKT,// 17
    WD_PROTO_CMD_RECV_HANDLEPKT,// 18
    WD_PROTO_CMD_MAX
}Wilddog_Proto_Cmd_T;
extern Wilddog_Protocol_T * _wilddog_protocol_init(void *p_conn);
extern Wilddog_Return_T _wilddog_protocol_deInit(void *p_conn);
#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_PROTOCOL_H_*/

