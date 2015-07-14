
#ifndef _WILDDOG_CT_H_
#define _WILDDOG_CT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "wilddog.h"
#include "wilddog_url_parser.h"

#ifndef WILDDOG_WEAK
#define WILDDOG_WEAK  __attribute__((weak))
#endif
typedef enum WILDDOG_API_CMDS
{
    WILDDOG_APICMD_INIT = 0,
    WILDDOG_APICMD_CREATEREF ,
    WILDDOG_APICMD_DESTROYREF,
    WILDDOG_APICMD_GETREF,
    WILDDOG_APICMD_SETAUTH,
    WILDDOG_APICMD_QUERY,
    WILDDOG_APICMD_SET,
    WILDDOG_APICMD_PUSH,
    WILDDOG_APICMD_REMOVE,
    WILDDOG_APICMD_ON,
    WILDDOG_APICMD_OFF,
    WILDDOG_APICMD_GETKEY,
    WILDDOG_APICMD_SYNC,
    WILDDOG_APICMD_MAXCMD
}Wilddog_Api_Cmd_T;

typedef struct WILDDOG_REF_T
{
    struct WILDDOG_REF_T * next;
    struct WILDDOG_REPO_T * p_ref_repo;
    struct WILDDOG_URL_T * p_ref_url;
}Wilddog_Ref_T;

typedef struct WILDDOG_ARG_SETAUTH
{
    Wilddog_Str_T * p_host;
    onAuthFunc onAuth;
    void* arg;
    u8 * p_auth;
    u16 d_len;
}Wilddog_Arg_SetAuth_T;

typedef struct WILDDOG_ARG_QUERY
{
    Wilddog_T p_ref;
    Wilddog_Func_T p_callback;
    void* arg;
}Wilddog_Arg_Query_T;

typedef struct WILDDOG_ARG_SET
{
    Wilddog_T p_ref;
    Wilddog_Node_T *p_node;
    Wilddog_Func_T p_callback;
    void* arg;
}Wilddog_Arg_Set_T;

typedef Wilddog_Arg_Set_T Wilddog_Arg_Push_T ;
typedef Wilddog_Arg_Query_T Wilddog_Arg_Remove_T ;

typedef struct WILDDOG_ARG_ON
{
    Wilddog_T p_ref;
    Wilddog_EventType_T d_event;
    Wilddog_Func_T p_onData;
    void* p_dataArg;
}Wilddog_Arg_On_T;

typedef struct WILDDOG_ARG_OFF
{
    Wilddog_T p_ref;
    Wilddog_EventType_T d_event;
}Wilddog_Arg_Off_T;

typedef struct WILDDOG_ARG_GETREF
{
    Wilddog_T p_ref;
    Wilddog_RefChange_T d_cmd;
    Wilddog_Str_T * p_str;
}Wilddog_Arg_GetRef_T;


typedef struct WILDDOG_REPO_T
{
    struct WILDDOG_REPO_T * next;
    struct WILDDOG_REF_T* p_rp_head;
    Wilddog_Url_T * p_rp_url;
    struct WILDDOG_STORE_T * p_rp_store;
    struct WILDDOG_CONN_T * p_rp_conn;
}Wilddog_Repo_T;

typedef struct WILDDOG_REPO_CONTAINER_T
{
    Wilddog_Repo_T *p_rc_head;
}Wilddog_Repo_Con_T;

extern size_t _wilddog_ct_ioctl
    (
    Wilddog_Api_Cmd_T cmd, 
    void* arg, 
    int flags
    );
extern Wilddog_Repo_T *_wilddog_ct_findRepo(Wilddog_Str_T * p_host);
extern u8 _wilddog_ct_getRepoNum(void);

#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_CT_H_*/

