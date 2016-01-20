
#ifndef _WILDDOG_PORT_H_
#define _WILDDOG_PORT_H_

#ifdef __cplusplus
extern "C"
{
#endif


#ifndef WILDDOG_PORT_TYPE_ESP
#include <stdio.h>
#endif

#include "wilddog_config.h"
#include "wilddog.h"

int wilddog_gethostbyname(Wilddog_Address_T* addr,char* host);
int wilddog_openSocket(int* socketId);
int wilddog_closeSocket(int socketId);
int wilddog_send
    (
    int socketId,
    Wilddog_Address_T*,
    void* tosend,
    s32 tosendLength
    );

/*
 * return <0 have not receive anything >=0 the length，it is a blocked function.
 */

int wilddog_receive
    (
    int socketId,
    Wilddog_Address_T*,
    void* toreceive,
    s32 toreceiveLength, 
    s32 timeout
    );

#ifdef __cplusplus
}
#endif


#endif/*_WILDDOG_PORT_H_*/

