
#ifndef _WILDDOG_STORE_H_
#define _WILDDOG_STORE_H_

#ifdef __cplusplus
extern "C"
{
#endif

 
#include "wilddog_conn.h"

#define WILDDOG_AUTH_LEN 256

typedef enum WILDDOG_STORE_CMD_T
{
    WILDDOG_STORE_CMD_GETAUTH = 0,
    WILDDOG_STORE_CMD_GETNODE,
    WILDDOG_STORE_CMD_SETAUTH,
    WILDDOG_STORE_CMD_SENDGET,
    WILDDOG_STORE_CMD_SENDSET,
    WILDDOG_STORE_CMD_SENDPUSH,
    WILDDOG_STORE_CMD_SENDREMOVE,
    WILDDOG_STORE_CMD_SENDON,
    WILDDOG_STORE_CMD_SENDOFF,
}Wilddog_Store_Cmd_T;

typedef struct WILDDOG_STORE_AUTH_T
{
    u8 p_auth[WILDDOG_AUTH_LEN];
    u16 d_len;
}Wilddog_Store_Auth_T;

typedef struct WILDDOG_STORE_AUTHARG_T
{
    u8 * p_data;
    u16 d_len;
    Wilddog_Url_T * p_url;
    onAuthFunc p_onAuth;
    void* p_onAuthArg;
}Wilddog_Store_AuthArg_T;

typedef struct WILDDOG_STORE_EVENTARG_T
{
    Wilddog_EventType_T d_event;
    Wilddog_ConnCmd_Arg_T d_connCmd;
}Wilddo_Store_EventArg_T;

typedef struct WILDDOG_STORE_T
{
    struct WILDDOG_STORE_AUTH_T *p_se_auth;
    struct WILDDOG_EVENT_T *p_se_event;
    Wilddog_Node_T * p_se_head;
    Wilddog_Repo_T *p_se_repo;
    Wilddog_Func_T p_se_callback;
}Wilddog_Store_T;


extern Wilddog_Store_T *_wilddog_store_init(Wilddog_Repo_T* p_repo);
extern Wilddog_Store_T *_wilddog_store_deinit(Wilddog_Repo_T* p_repo);

#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_STORE_H_*/

