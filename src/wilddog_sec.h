
#ifndef _WILDDOG_SEC_H_
#define _WILDDOG_SEC_H_

#ifdef __cplusplus
extern "C" {
#endif

extern Wilddog_Return_T _wilddog_sec_send
    (
    void* p_data, 
    s32 len
    );
extern int _wilddog_sec_recv
    (
    void* p_data, 
    s32 len
    );
extern Wilddog_Return_T _wilddog_sec_init
    (
    Wilddog_Str_T *p_host,
    u16 d_port
    );
extern Wilddog_Return_T _wilddog_sec_deinit(void);

#ifdef __cplusplus
}
#endif

#endif/*_WILDDOG_SEC_H_*/

