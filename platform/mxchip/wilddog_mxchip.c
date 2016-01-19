/*
 * wilddog_wiced.c
 *
 *  Created on: 2015-11-11 -- Baikal.Hu
 *				
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "wilddog_port.h"
#include "wilddog.h"
#include "wilddog_config.h"
#include "wilddog_endian.h"
#include "wilddog_common.h"
#include "test_lib.h"
#include "common.h"
#include "mico_socket.h"

#define SOCKET_NUMBER 3

/*
 * Function:    wilddog_gethostbyname
 * Description: wilddog gethostbyname function, it use the interface in wiced platform.
 * Input:        host: The pointer of host string.
 * Output:      addr: The pointer of the Wilddog_Address_T.
 * Return:      If success, return 0; else return -1.
*/
int wilddog_gethostbyname( Wilddog_Address_T* addr, char* host )
{
    char ipstr[16];
    uint32_t addr_in;

    if(kNoErr != gethostbyname( host, (uint8_t *)ipstr, 16 ))
        return -1;

    addr_in = inet_addr(ipstr);

    addr->ip[0] = (addr_in >> 24 & 0xff);
    addr->ip[1] = (addr_in >> 16 & 0xff);
    addr->ip[2] = (addr_in >> 8 & 0xff);
    addr->ip[3] = (addr_in & 0xff);
    addr->len = sizeof(addr_in);

    return 0;
}

/*
 * Function:    wilddog_openSocket
 * Description: wilddog openSocket function, it use the interface in wiced platform.
 * Input:        N/A
 * Output:      socketId: The pointer of socket id.
 * Return:      If success, return 0; else return -1.
*/
int wilddog_openSocket( int* socketId )
{
    int fd = -1;

    if ((fd = socket(AF_INET,  SOCK_DGRM, IPPROTO_UDP)) < 0) {
        printf("cannot create socket");
        return -1;
    }

    *socketId = fd + SOCKET_NUMBER;
    return 0;
}

/*
 * Function:    wilddog_closeSocket
 * Description: wilddog closeSocket function, it use the interface in wiced platform.
 * Input:        socketId: The socket id.
 * Output:      N/A
 * Return:      If success, return 0; else return -1.
*/
int wilddog_closeSocket( int socketId )
{
    return close(socketId - SOCKET_NUMBER);
}

/*
 * Function:    wilddog_send
 * Description: wilddog send function, it use the interface in wiced platform.
 * Input:        socketId: The socket id.
 *                  addr_in:  The pointer of Wilddog_Address_T
 *                  tosend: The pointer of the send buffer
 *                  tosendLength: The length of the send buffer.
 * Output:      N/A
 * Return:      If success, return the number of characters sent.; else return -1.
*/
int wilddog_send
    ( 
    int socketId, 
    Wilddog_Address_T* addr_in, 
    void* tosend, 
    s32 tosendLength 
    )
{
    int ret;
    struct sockaddr_t servaddr;    

    memset((char*)&servaddr, 0, sizeof(servaddr));
    
    servaddr.s_port = addr_in->port;

    servaddr.s_ip = (addr_in->ip[0] << 24)| (addr_in->ip[1] << 16) | (addr_in->ip[2] << 8) | (addr_in->ip[3]) ;

    socketId -= SOCKET_NUMBER;
#if WILDDOG_SELFTEST
		performtest_getDtlsSendTime();
#endif

    wilddog_debug_level(WD_DEBUG_LOG, "addr_in->port = %d, ip = %u.%u.%u.%u\n", addr_in->port, addr_in->ip[0], \
        addr_in->ip[1], addr_in->ip[2], addr_in->ip[3]);
                
    if((ret = sendto(socketId, tosend, tosendLength, 0, &servaddr,
             (socklen_t)sizeof(servaddr)))<0){
        perror("sendto failed");
        return -1;
    }
                

    return ret;

}

/*
 * Function:    wilddog_receive
 * Description: wilddog receive function, it use the interface in wiced platform.
 * Input:        socketId: The socket id.
 *                  addr:  The pointer of Wilddog_Address_T
 *                  buf: The pointer of the send buffer
 *                  bufLen: The length of the send buffer.
 *                  timeout: The max timeout in recv process.
 * Output:      N/A
 * Return:      If success, return the number of bytes received; else return -1.
*/
int wilddog_receive(int socketId,Wilddog_Address_T* addr,void* buf,s32 bufLen, s32 timeout)
{
        int fd;
        struct sockaddr_t remaddr;
        memset((char*)&remaddr, 0, sizeof(remaddr));
        socklen_t addrlen = sizeof(remaddr);

        char ip_address[16];
        int recvlen=0;
            
        int time;
        time = timeout;

        fd=socketId - SOCKET_NUMBER;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time,sizeof(struct timeval_t));
        recvlen = recvfrom(fd, buf, bufLen, 0, &remaddr, &addrlen);
        inet_ntoa( ip_address, remaddr.s_ip );

        if(recvlen < 0)
        {
            return -1;
        }
        
        if( (remaddr.s_ip & 0xff == addr->ip[3]) || \
          ((remaddr.s_ip >> 8) & 0xff == addr->ip[2] ) || \
          ((remaddr.s_ip >> 16) & 0xff == addr->ip[1]) || \
          ((remaddr.s_ip >> 24) & 0xff == addr->ip[0])|| \
          (remaddr.s_port) != addr->port)
       {
           wilddog_debug("ip or port not match!");
            return -1;
       }
        
        return recvlen;
}

