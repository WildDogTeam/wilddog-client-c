
#ifndef _WILDDOG_SEC_H_
#define _WILDDOG_SEC_H_

#ifdef __cplusplus
extern "C" {
#endif

extern Wilddog_Return_T _wilddog_sec_send
    (int fd, 
    Wilddog_Address_T * addr_in, 
    void* p_data, 
    s32 len
    );
extern int _wilddog_sec_recv
    (
    int fd, 
    Wilddog_Address_T * addr_in, 
    void* p_data, 
    s32 len
    );
extern Wilddog_Return_T _wilddog_sec_init
    (int fd, 
    Wilddog_Address_T * addr_in
    );
extern Wilddog_Return_T _wilddog_sec_deinit
    (
    int fd, 
    Wilddog_Address_T * addr_in
    );

#ifdef __cplusplus
}
#endif

#endif/*_WILDDOG_SEC_H_*/

