/*
 * wilddog_wiced.c
 *
 *  Created on: 2015-6-15 -- jimmy.pan
 *				
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "wilddog_port.h"
#include "wiced.h"
#include "wilddog.h"
#include "wilddog_config.h"
#include "wilddog_endian.h"
#include "wilddog_common.h"
#include "test_lib.h"

/*
 * Function:    _packet_get_next_fragment
 * Description: Get next fragment packet, it use the interface in wiced platform.
 * Input:       packet: The pointer of packet.
 * Output:      next_packet_fragment: The second rank pointer of the next packet.
 * Return:      If success, return 0
*/
wiced_result_t _packet_get_next_fragment
    (
    wiced_packet_t* packet, 
    wiced_packet_t** next_packet_fragment 
    )
{
    wiced_packet_t* p_tmp = NULL;
    if ( packet->nx_packet_fragment_next )
    {
        p_tmp = packet->nx_packet_fragment_next;
    }
    wiced_packet_delete( packet );
    *next_packet_fragment = p_tmp;
    return 0;
}
#define WILDDOG_MAKE_IPV4(a,b,c,d) (((uint8_t)(a) << 24) | \
    ((uint8_t)(b) << 16) | ((uint8_t)(c) << 8) | ((uint8_t)(d)))

extern wiced_result_t dns_client_add_server_address
( 
    wiced_ip_address_t address 
    );

/*
 * Function:    wilddog_gethostbyname
 * Description: wilddog gethostbyname function, it use the interface in wiced platform.
 * Input:        host: The pointer of host string.
 * Output:      addr: The pointer of the Wilddog_Address_T.
 * Return:      If success, return 0; else return -1.
*/
int wilddog_gethostbyname( Wilddog_Address_T* addr, char* host )
{
    wiced_ip_address_t address;
    uint32_t tmpIp = 0;
    uint32_t dns_ip;

    addr->len = 4;
    /* Add free DNS server (114.114.114.114) */
    dns_ip = WILDDOG_MAKE_IPV4(114,114,114,114);
    SET_IPV4_ADDRESS( address, &dns_ip );
    dns_client_add_server_address( address );
    if ( wiced_hostname_lookup( host, &address, 4000 ) != WICED_SUCCESS )
    {
        return -1;
    }
    tmpIp = address.ip.v4;
    tmpIp = wilddog_htonl(tmpIp);
    memcpy( addr->ip, &tmpIp, 4 );

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
    wiced_udp_socket_t* socket = NULL;

    socket = wmalloc( sizeof(wiced_udp_socket_t) );
    if ( NULL == socket )
    {
        printf( "%s malloc error!\n", __func__ );
        return -1;
    }
    if ( wiced_udp_create_socket( socket, WICED_ANY_PORT, WICED_STA_INTERFACE ) != WICED_SUCCESS )
    {
        return -1;
    }
    *socketId = (int) socket;

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
    wiced_udp_delete_socket( (wiced_udp_socket_t*) socketId );
    if ( socketId )
    {
        wfree( (void*)socketId );
        socketId = 0;
    }
    return 0;
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
    wiced_udp_socket_t* socket = (wiced_udp_socket_t*) socketId;
    wiced_ip_address_t ipaddr;
    ipaddr.version = WICED_IPV4;
    ipaddr.ip.v4 = WILDDOG_MAKE_IPV4(addr_in->ip[0], addr_in->ip[1], \
        addr_in->ip[2], addr_in->ip[3]);

    wiced_packet_t* packet;
    uint8_t* data;
    uint16_t aval;
    if ( wiced_packet_create_udp( socket, 0, &packet, &data, &aval ) != \
        WICED_SUCCESS )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "error create packet ...\r\n");
        return -1;
    }
    if ( aval < tosendLength )
    {
        /* Delete packet, since the send failed */
        wiced_packet_delete( packet ); 
        wilddog_debug_level(WD_DEBUG_ERROR, \
            "too large length to translate! should be %d, want send %ld\n", \
            aval, tosendLength);
        return -1;
    }

    memcpy( data, tosend, tosendLength );
#if WILDDOG_SELFTEST
    performtest_getDtlsSendTime();
#endif
    wiced_packet_set_data_end( packet, (uint8_t*) ( data + tosendLength ) );
    wilddog_debug_level(WD_DEBUG_LOG, "socketId = %d, port = %d\n", \
        socketId, addr_in->port);

    if ( wiced_udp_send( socket, &ipaddr, addr_in->port, packet ) != \
        WICED_SUCCESS )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "UDP packet send failed\r\n");
        wiced_packet_delete( packet );
    }
    else
    {
        wilddog_debug_level(WD_DEBUG_LOG, "send packet success!\n");
        return tosendLength;
    }
    return -1;

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
int wilddog_receive
    ( 
    int socketId, 
    Wilddog_Address_T* addr_in, 
    void* buf, 
    s32 bufLen, 
    s32 timeout
    )
{
    wiced_udp_socket_t* socket = (wiced_udp_socket_t*) socketId;

    wiced_packet_t* receive = NULL;
    u16 totalLeft = 0;
    u16 total = 0;
    uint16_t pos = 0;
    uint8_t* rxData;
    uint16_t fragLeft = 0;
    wiced_ip_address_t recieve_ip_addr;
    uint16_t receive_port;
    wiced_result_t result;
    result = wiced_udp_receive( socket, &receive, timeout );
    if ( result == WICED_SUCCESS )
    {
        if(NULL == receive)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "receive is NULL!");
            return -1;
        }
        wiced_udp_packet_get_info( receive, &recieve_ip_addr, &receive_port );

        if ( !recieve_ip_addr.ip.v4 == MAKE_IPV4_ADDRESS(addr_in->ip[0], \
            addr_in->ip[1], addr_in->ip[2], addr_in->ip[3]) )
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "addr error!\n" );
            wiced_packet_delete( receive );
            return -1;
        }
        do
        {
            if ( wiced_packet_get_data( receive, 0, (uint8_t**) &rxData, \
                &fragLeft, &totalLeft ) != WICED_SUCCESS )
            {
                wilddog_debug_level(WD_DEBUG_ERROR, \
                    "get data from packet error \r\n" );
                wiced_packet_delete( receive );
                return -1;
            }
            else
            {
                if ( pos + fragLeft > bufLen )
                {
                    /* too large, drop*/
                    wiced_packet_delete( receive );
                    wilddog_debug_level(WD_DEBUG_ERROR, \
                        "too large receive end , get %d bytes\n", pos );
                    return 0;
                }
                if ( total == 0 )
                {
                    /*only get first data, next fragment's total is wrong*/
                    total = totalLeft;
                }

                memcpy( (uint8_t *) ( buf + pos ), rxData, fragLeft );
                pos += fragLeft;

                fragLeft = 0;
            }
            if ( pos == total )
            {
                /*end*/
#if WILDDOG_SELFTEST
                {
                    performtest_getWaitSessionQueryTime();
                    performtest_getWaitRecvTime();
                }
#endif
                wilddog_debug_level(WD_DEBUG_LOG, "received %d data!", pos);
                wiced_packet_delete( receive );
                return pos;
            }
            else if ( pos < total )
            {
                /*get next fragment*/
                wilddog_debug_level(WD_DEBUG_LOG, "more than one packet!");
                if ( WICED_SUCCESS != _packet_get_next_fragment( receive, \
                    &receive ) )
                {
                    wilddog_debug_level(WD_DEBUG_ERROR, \
                        "get next fragment err! %d lost!", total);
					if(receive)
	                    wiced_packet_delete( receive );
                    return -1;
                }
                if ( NULL == receive )
                {
                    wilddog_debug_level(WD_DEBUG_ERROR, \
                        "get next fragment err! %d lost!", total);
                    return -1;
                }

            }
            else
            {
                wiced_packet_delete( receive );
                return -1;
            }
        } while ( pos < total );
    }
    else
    {
        wilddog_debug_level(WD_DEBUG_LOG, "result = %d",result);
    }
    /*if(receive)
        wiced_packet_delete( receive );*/
    return 0;
}

