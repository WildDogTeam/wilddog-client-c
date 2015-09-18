
#include "wilddog.h"
#include "wilddog_config.h"
#include "wilddog_port.h"
#include "wilddog_sec_host.h"

STATIC Wilddog_Address_T l_addr_in;
STATIC int l_fd;

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
    void* p_data, 
    s32 len
    )
{
    int res;
    res = wilddog_send(l_fd,&l_addr_in, p_data, len);
    if(res < 0)
    {
        if(l_fd)
            wilddog_closeSocket(l_fd);
        wilddog_openSocket(&l_fd);           
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
    void* p_data, 
    s32 len
    )
{
    return wilddog_receive(l_fd, &l_addr_in, p_data, len, \
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
    Wilddog_Str_T *p_host,
    u16 d_port
    )
{   
    int res;
    wilddog_openSocket(&l_fd);
    res = _wilddog_sec_getHost(&l_addr_in,p_host,d_port);
    
    return res;
}

/*
 * Function:    _wilddog_sec_deinit
 * Description: Destroy no security session
 * Input:       fd: socket id    
                addr_in: The address which contains ip and port
 * Output:      N/A
 * Return:      Success: 0
*/
Wilddog_Return_T WD_SYSTEM _wilddog_sec_deinit(void)
{
    if(l_fd)
        wilddog_closeSocket(l_fd);
    return WILDDOG_ERR_NOERR;
}

