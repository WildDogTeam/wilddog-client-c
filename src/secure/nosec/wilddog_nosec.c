
#include "wilddog.h"
#include "wilddog_config.h"
#include "wilddog_port.h"
#include "wilddog_sec.h"
#include "wilddog_protocol.h"

/*
 * Function:    _wilddog_sec_send
 * Description: No security send function
 * Input:       fd: socket id    
 *      addr_in: The address which contains ip and port
 *      p_data: The buffer which store the sending data
 *      len: The length of the wanting send data
 * Output:      N/A
 * Return:      The length of the actual sending data
*/
Wilddog_Return_T WD_SYSTEM _wilddog_sec_send
    (
    Wilddog_Protocol_T *protocol,
    void* p_data, 
    s32 len
    )
{
    int res;
    res = wilddog_send(protocol->socketFd,&protocol->addr, p_data, len);
    if(res < 0)
    {
        if(protocol->socketFd)
            wilddog_closeSocket(protocol->socketFd);
        wilddog_openSocket(&protocol->socketFd);        
    }
    return res;
}

/*
 * Function:    _wilddog_sec_recv
 * Description: No security recv function
 * Input:       fd: socket id    
 *      addr_in: The address which contains ip and port
 *      len: The length of the wanting receive data
 * Output:      p_data: The buffer which store the receiving data
 * Return:      The length of the actual receiving data
*/
int WD_SYSTEM _wilddog_sec_recv
    (
    Wilddog_Protocol_T *protocol,
    void* p_data, 
    s32 len
    )
{
    return wilddog_receive(protocol->socketFd, &protocol->addr, p_data, len, \
                           WILDDOG_RECEIVE_TIMEOUT);
}

/*
 * Function:    _wilddog_sec_init
 * Description: Initialize no security session
 * Input:       fd: socket id    
        addr_in: The address which contains ip and port
 * Output:      N/A
 * Return:      Success: 0
*/
Wilddog_Return_T WD_SYSTEM _wilddog_sec_init
    (
    Wilddog_Protocol_T *protocol
    )
{
    wilddog_assert(protocol, WILDDOG_ERR_NULL);
    
    if(0 != wilddog_openSocket(&protocol->socketFd)){
        wilddog_debug_level(WD_DEBUG_ERROR, "Can not open socket!");
        return WILDDOG_ERR_INVALID;
    }

    return _wilddog_sec_getHost(&protocol->addr, protocol->host);
}

/*
 * Function:    _wilddog_sec_deinit
 * Description: Destroy no security session
 * Input:       fd: socket id    
                addr_in: The address which contains ip and port
 * Output:      N/A
 * Return:      Success: 0
*/
Wilddog_Return_T WD_SYSTEM _wilddog_sec_deinit(Wilddog_Protocol_T *protocol)
{
    if(protocol->socketFd)
        wilddog_closeSocket(protocol->socketFd);
    protocol->socketFd = -1;
    return WILDDOG_ERR_NOERR;
}
/*
 * Function:    _wilddog_sec_init
 * Description: Initialize dtls security session
 * Input:       p_host: url host string  
 *              d_port: the port want to connect
 *              retryNum: Max retry time
 * Output:      N/A
 * Return:      Success: 0    Faied: < 0
*/
Wilddog_Return_T _wilddog_sec_reconnect
    (
    Wilddog_Protocol_T *protocol,
    int retryNum
    )
{
    int i;
    Wilddog_Return_T ret = WILDDOG_ERR_INVALID;
    for(i = 0; i < retryNum; i++)
    {
        _wilddog_sec_deinit(protocol);
        ret = _wilddog_sec_init(protocol);
        if(WILDDOG_ERR_NOERR == ret)
            return ret;
    }
    return ret;
}

