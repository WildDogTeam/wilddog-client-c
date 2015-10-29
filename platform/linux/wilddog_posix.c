/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_posix.c
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
#include <unistd.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include "wilddog_port.h"
#include "wilddog_config.h"
#include "wilddog_endian.h"
#include "test_lib.h"

/*
 * Function:    wilddog_gethostbyname
 * Description: wilddog gethostbyname function, it use the interface in posix.
 * Input:        host: The pointer of host string.
 * Output:      addr: The pointer of the Wilddog_Address_T.
 * Return:      If success, return 0; else return -1.
*/
int wilddog_gethostbyname(Wilddog_Address_T* addr,char* host)
{

    struct hostent* hp;
    if((hp=gethostbyname(host))==NULL){
        return -1;
    }
    
    memcpy(addr->ip, hp->h_addr_list[0], hp->h_length);

    addr->len = hp->h_length;
    
    return 0;
}

/*
 * Function:    wilddog_openSocket
 * Description: wilddog openSocket function, it use the interface in posix.
 * Input:        N/A
 * Output:      socketId: The pointer of socket id.
 * Return:      If success, return 0; else return -1.
*/
int wilddog_openSocket(int* socketId)
{
    int fd;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket");
        return -1;
    }
    *socketId = fd;
    return 0;
}

/*
 * Function:    wilddog_closeSocket
 * Description: wilddog closeSocket function, it use the interface in posix.
 * Input:        socketId: The socket id.
 * Output:      N/A
 * Return:      If success, return 0; else return -1.
*/
int wilddog_closeSocket(int socketId)
{
    
    return close(socketId);
}


/*
 * Function:    wilddog_send
 * Description: wilddog send function, it use the interface in posix.
 * Input:        socketId: The socket id.
 *                  addr_in:  The pointer of Wilddog_Address_T
 *                  tosend: The pointer of the send buffer
 *                  tosendLength: The length of the send buffer.
 * Output:      N/A
 * Return:      If success, return the number of characters sent.; else return -1.
*/
int wilddog_send(int socketId,Wilddog_Address_T* addr_in,void* tosend,s32 tosendLength)
{
    int ret;
    struct sockaddr_in servaddr;    /* server address */
    /* fill in the server's address and data */
    memset((char*)&servaddr, 0, sizeof(servaddr));
    if(addr_in->len==4){
        servaddr.sin_family = AF_INET;
    }
    else{
        wilddog_debug_level(WD_DEBUG_ERROR, "wilddog_send-unkown addr len!");
        return -1;
    }
    servaddr.sin_port = wilddog_htons(addr_in->port);
    memcpy(&servaddr.sin_addr.s_addr,addr_in->ip,addr_in->len);
#if WILDDOG_SELFTEST
		performtest_getDtlsSendTime();
#endif

    wilddog_debug_level(WD_DEBUG_LOG, "addr_in->port = %d, ip = %u.%u.%u.%u\n", addr_in->port, addr_in->ip[0], \
        addr_in->ip[1], addr_in->ip[2], addr_in->ip[3]);
    if((ret = sendto(socketId, tosend, tosendLength, 0, (struct sockaddr *)&servaddr,
             sizeof(servaddr)))<0){
        perror("sendto failed");
        return -1;
    }
    return ret;
}

/*
 * Function:    wilddog_receive
 * Description: wilddog receive function, it use the interface in posix.
 * Input:        socketId: The socket id.
 *                  addr:  The pointer of Wilddog_Address_T
 *                  buf: The pointer of the send buffer
 *                  bufLen: The length of the send buffer.
 *                  timeout: The max timeout in recv process.
 * Output:      N/A
 * Return:      If success, return the number of bytes received; else return -1.
*/
int wilddog_receive(int socketId,Wilddog_Address_T* addr,void* buf,s32 bufLen, s32 timeout){
    struct sockaddr_in remaddr;
    socklen_t addrlen = sizeof(remaddr);
    int recvlen;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout*1000;

    setsockopt(socketId, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

    recvlen = recvfrom(socketId, buf, bufLen, 0, (struct sockaddr *)&remaddr, &addrlen);
    if(recvlen < 0)
    {
        return -1;
    }
    if(memcmp(addr->ip, &remaddr.sin_addr.s_addr, addr->len) || \
        wilddog_ntohs(remaddr.sin_port) != addr->port)
    {
        wilddog_debug("ip or port not match!");
        return -1;
    }
#if WILDDOG_SELFTEST
	{
		performtest_getWaitSessionQueryTime();
		performtest_getWaitRecvTime();
	}
#endif
    return recvlen;
}
