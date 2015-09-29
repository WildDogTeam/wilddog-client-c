/*
 * wilddog_esp8266.c
 *
 *  Created on: 2015-8-25 -- baikal.hu
 *				
 */
#ifndef WILDDOG_PORT_TYPE_ESP	
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "wilddog_port.h"
#include "wilddog.h"
#include "wilddog_config.h"
#include "wilddog_endian.h"
#include "wilddog_common.h"
#include "utlist.h"
#include "test_lib.h"

#include "espconn.h"
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"


extern os_timer_t test_timer1;
extern int dns_flag;

STATIC struct espconn socket;
STATIC ip_addr_t address;


struct recv_buf_node
{
	struct recv_buf_node *next;
	char *buf;
	unsigned short len;
};

struct recv_buf_node *head;


STATIC void WD_SYSTEM
recv_list_init(void)
{
	head = wmalloc(sizeof(struct recv_buf_node));
	head->buf = NULL;
	head->next = NULL;
	head->len = 0;

}

STATIC void WD_SYSTEM
recv_list_deinit(void)
{
    struct recv_buf_node *tmp = head;

    while(head != NULL)
    {
        tmp = head->next;
        wfree(head->buf);
        wfree(head);
        head = tmp;
    }
}


STATIC void WD_SYSTEM
send_cb(void *arg)
{
    struct espconn *pespconn = arg;
}

STATIC void WD_SYSTEM
recv_cb(void *arg, char *buf, unsigned short len)
{
    struct espconn *pespconn = arg;
	struct recv_buf_node *node;

	int i;
    #if 0
	printf("recv_cb buf\n");
	for(i = 0; i < len; i++)
	{
		printf("0x%x  ", *(buf+i));
	}
	printf("\n\n");
    #endif
    
	if(head->next == NULL && head->len == 0)
	{
		head->buf = wmalloc(len);
		head->len = len;
		memcpy(head->buf, buf, len);
	}
	else
	{
		node = wmalloc(sizeof(struct recv_buf_node));
		node->len = len;
		node->buf = wmalloc(len);
        node->next = NULL;
		memcpy(node->buf, buf, len);
		LL_APPEND(head, node);
		
	}
}


void WD_SYSTEM dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
	struct espconn *pespconn = (struct espconn *)arg;
	if(ipaddr == NULL)
	{
		printf("user_esp_platform_dns_found NULL\n");
	}

	if (ipaddr != NULL)
	{
		memcpy(&(address.addr), &ipaddr->addr, 4);
		dns_flag = TRUE;
	}
	
    espconn_regist_recvcb(pespconn, recv_cb);
    espconn_regist_sentcb(pespconn, send_cb);
}


int WD_SYSTEM gethost()
{
    int ret;	

	ret = espconn_gethostbyname(&socket, "s-dal5-coap-1.wilddogio.com", &address, dns_found);
	os_timer_arm(&test_timer1, 1000, 0);
    return ret;
}

int WD_SYSTEM wilddog_gethostbyname( Wilddog_Address_T* addr, char* host )
{
	memcpy(addr->ip, &(address.addr), 4);
	if(addr->ip[0] != 0 && addr->ip[1] != 0 && addr->ip[2] != 0 && addr->ip[3] != 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int WD_SYSTEM wilddog_openSocket( int* socketId )
{
    socket.type = ESPCONN_UDP;
    socket.proto.udp = (esp_udp *)wmalloc(sizeof(esp_udp));
    socket.proto.udp->local_port = espconn_port();   
    socket.proto.udp->remote_port = WILDDOG_PORT;
	socket.type = ESPCONN_UDP;
	socket.state = ESPCONN_NONE;
	
	recv_list_init();
	if(espconn_create(&socket) != 0)
	{
	    wfree(socket.proto.udp);
        recv_list_deinit();
		return -1;
	}
	
     *socketId = (int) (&socket);
    return 0;
}

int WD_SYSTEM wilddog_closeSocket( int socketId )
{
    espconn_delete( (struct espconn*) socketId );	
    if ( socketId )
    {
        socketId = 0;
    }
    wfree(socket.proto.udp);
    recv_list_deinit();
    return 0;
}

int WD_SYSTEM wilddog_send
    ( 
    int socketId, 
    Wilddog_Address_T* addr_in, 
    void* tosend, 
    s32 tosendLength 
    )
{
	int ret;
	struct espconn *socket = (struct espconn*) socketId;

	memcpy(socket->proto.udp->remote_ip, addr_in->ip, 4);
	int i;
    #if 0
	for(i = 0; i < tosendLength; i++)
	{
		printf("0x%x  ", *((char *)tosend+i));
	}
	printf("\n\n");
	#endif
    
	ret = espconn_sent(socket, tosend, tosendLength);
    
    if(ret == 0)
        return tosendLength;
    else
        return -1;
}
int WD_SYSTEM wilddog_receive
    ( 
    int socketId, 
    Wilddog_Address_T* addr_in, 
    void* buf, 
    s32 bufLen, 
    s32 timeout
    )
{
	struct recv_buf_node *tmp;
	int len = 0;
	tmp = head;
	s32 count = timeout;

	while(count > 0)
	{
		if(head->len == 0)
		{
			os_delay_us(10000);
		}
		else
		{
			len = head->len;
			if(head->next != NULL)
			{
			 	memcpy(buf, head->buf, head->len);
				
				head = head->next;
				wfree(tmp->buf);
				wfree(tmp);
			}
			else
			{
				memcpy(buf, head->buf, head->len);
				wfree(head->buf);
				head->buf = NULL;
				head->next = NULL;
				head->len = 0;
			}
            break;
		}
		count -= 10;
	}

	return len;
}
