/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_quectel.c
 *
 * Description: Socket API for quectel platform.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.5        Jimmy.Pan       2015-08-11  Create file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wilddog_port.h"
#include "wilddog_config.h"
#include "wilddog_m26.h"
int wilddog_gethostbyname(Wilddog_Address_T* addr,char* host)
{
    return wilddog_m26_gethostbyname(addr, host);
}
int wilddog_openSocket(int* socketId)
{
    return wilddog_m26_openSocket(socketId);
}

int wilddog_closeSocket(int socketId)
{
    return wilddog_m26_closeSocket(socketId);
}

int wilddog_send(int socketId,Wilddog_Address_T* addr_in,void* tosend,s32 tosendLength)
{
    return wilddog_m26_send(socketId, addr_in, tosend, tosendLength);
}

int wilddog_receive(int socketId,Wilddog_Address_T* addr,void* buf,s32 bufLen, s32 timeout)
{
    return wilddog_m26_receive(socketId, addr, buf, bufLen, timeout);
}

