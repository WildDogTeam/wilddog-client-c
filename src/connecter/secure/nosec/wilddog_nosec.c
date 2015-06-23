
#include "wilddog.h"
#include "wilddog_config.h"
#include "wilddog_port.h"

Wilddog_Return_T _wilddog_sec_send
    (
    int fd, 
    Wilddog_Address_T * addr_in, 
    void* p_data, 
    s32 len
    )
{
    return wilddog_send(fd, addr_in, p_data, len);
}

int _wilddog_sec_recv
    (
    int fd, 
    Wilddog_Address_T * addr_in, 
    void* p_data, 
    s32 len
    )
{
    return wilddog_receive(fd, addr_in, p_data, len, WILDDOG_RECEIVE_TIMEOUT);
}

Wilddog_Return_T _wilddog_sec_init
    (
    int fd, 
    Wilddog_Address_T * addr_in
    )
{
    return WILDDOG_ERR_NOERR;
}

Wilddog_Return_T _wilddog_sec_deinit
    (
    int fd, 
    Wilddog_Address_T * addr_in
    )
{
    if(fd)
        wilddog_closeSocket(fd);
    return WILDDOG_ERR_NOERR;
}

