
#include "wilddog.h"
#include "wilddog_config.h"
#include "wilddog_port.h"

/*
 * Function:    _wilddog_sec_send
 * Description: No security send function
 * Input:       fd: socket id    
 *		addr_in: The address which contains ip and port
 *		p_data: The buffer which store the sending data
 *		len: The length of the wanting send data
 * Output:      N/A
 * Return:      The length of the actual sending data
*/
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

/*
 * Function:    _wilddog_sec_recv
 * Description: No security recv function
 * Input:       fd: socket id    
 *		addr_in: The address which contains ip and port
 *		len: The length of the wanting receive data
 * Output:      p_data: The buffer which store the receiving data
 * Return:      The length of the actual receiving data
*/
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

/*
 * Function:    _wilddog_sec_init
 * Description: Initialize no security session
 * Input:       fd: socket id    
 		addr_in: The address which contains ip and port
 * Output:      N/A
 * Return:      Success: 0
*/
Wilddog_Return_T _wilddog_sec_init
    (
    int fd, 
    Wilddog_Address_T * addr_in
    )
{
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_sec_deinit
 * Description: Destroy no security session
 * Input:       fd: socket id    
 		addr_in: The address which contains ip and port
 * Output:      N/A
 * Return:      Success: 0
*/
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

