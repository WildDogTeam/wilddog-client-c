/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_conn.c
 *
 * Description: connection functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        lsx       2015-05-15  Create file.
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
     
#include "wilddog_config.h"
#include "wilddog.h"
#include "wilddog_debug.h"
#include "wilddog_port.h"
#include "wilddog_api.h"
#include "test_lib.h"

STATIC Wilddog_Address_T l_defaultAddr_t[2] = 
{
	{4, {211,151,208,196}, 5683},
	{4, {211,151,208,197}, 5683}
};

STATIC int _wilddog_sec_hashIndex(Wilddog_Str_T *p_host, int totalNum)
{
	int len, i, total = 0;

	if(!p_host || totalNum <= 1)
		return 0;
	len = strlen((const char*)p_host);
	for(i = 0; i < len; i++)
	{
		total += p_host[i];
	}
	return total & ((1 << (totalNum - 1)) - 1);
}

STATIC int _wilddog_sec_getDefaultIpIndex(Wilddog_Str_T *p_host)
{
	int index;
	STATIC int count = 0;
	int totalNum = sizeof(l_defaultAddr_t)/sizeof(Wilddog_Address_T);
	index = _wilddog_sec_hashIndex(p_host, totalNum);

	index = (index + count) % totalNum;
	count++;

	return index;
}

int _wilddog_sec_getHost
    (
    Wilddog_Address_T *p_remoteAddr,
    Wilddog_Str_T *p_host,
    u16 d_port
    )
{   
    int res = -1;  
	int i;
#define WILDDOG_COAP_LOCAL_HOST "s-dal5-coap-1.wilddogio.com"

 #ifdef WILDDOG_SELFTEST                       
		ramtest_skipLastmalloc();
#endif   

    res = wilddog_gethostbyname(p_remoteAddr,WILDDOG_COAP_LOCAL_HOST);
    
#ifdef WILDDOG_SELFTEST                        
		ramtest_gethostbyname();
#endif
#ifdef WILDDOG_SELFTEST     
		performtest_star_tm();
#endif

	if(-1 == res)
	{
		i = _wilddog_sec_getDefaultIpIndex(p_host);
		p_remoteAddr->len = l_defaultAddr_t[i].len;
		memcpy(p_remoteAddr->ip, l_defaultAddr_t[i].ip, l_defaultAddr_t[i].len);
	}
#if 1
		p_remoteAddr->len = 4;
		p_remoteAddr->ip[0] = 10;
		p_remoteAddr->ip[1] = 18;
		p_remoteAddr->ip[2] = 2;
		p_remoteAddr->ip[3] = 200;
#endif 

    p_remoteAddr->port = d_port;
	
#undef WILDDOG_COAP_LOCAL_HOST
    return res;
}

