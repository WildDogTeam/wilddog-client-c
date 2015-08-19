
#ifndef _WILDDOG_M26_H_
#define _WILDDOG_M26_H_

#define APN_NAME "cmnet"

extern int wilddog_ql_init(void);
extern int wilddog_m26_gethostbyname(Wilddog_Address_T* addr,char* host);
extern int wilddog_m26_openSocket(int* socketId);
extern int wilddog_m26_closeSocket(int socketId);
extern int wilddog_m26_send
    (
    int socketId,
    Wilddog_Address_T* addr_in,
    void* tosend,
    s32 tosendLength
    );
extern int wilddog_m26_receive
    (
    int socketId,
    Wilddog_Address_T* addr,
    void* buf,
    s32 bufLen, 
    s32 timeout
    );

#endif/*_WILDDOG_M26_H_*/

