
#ifndef _WILDDOG_SEC_H_
#define _WILDDOG_SEC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wilddog_protocol.h"
extern int WD_SYSTEM _wilddog_sec_getHost
    (
    Wilddog_Address_T *p_remoteAddr,
    Wilddog_Str_T *p_host
    );
extern Wilddog_Return_T _wilddog_sec_send
    (
    Wilddog_Protocol_T *protocol,
    void* p_data, 
    s32 len
    );
extern int _wilddog_sec_recv
    (
    Wilddog_Protocol_T *protocol,
    void* p_data, 
    s32 len
    );
extern Wilddog_Return_T _wilddog_sec_init(Wilddog_Protocol_T *protocol);
extern Wilddog_Return_T _wilddog_sec_deinit(Wilddog_Protocol_T *protocol);
extern Wilddog_Return_T _wilddog_sec_reconnect
    (
    Wilddog_Protocol_T *protocol,
    int retryNum
    );
#ifdef __cplusplus
}
#endif

#endif/*_WILDDOG_SEC_H_*/

