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
 
#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdio.h>
#endif
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

/*
 * Function:    _wilddog_sec_hashIndex
 * Description: sec layer  index hash
 *   
 * Input:        p_host: the pointer of the host
 *                  totalNum: the number
 * Output:      N/A
 * Return:      the index
*/
STATIC int WD_SYSTEM _wilddog_sec_hashIndex
    (
    Wilddog_Str_T *p_host,
    int totalNum
    )
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

/*
 * Function:    _wilddog_sec_getDefaultIpIndex
 * Description: sec layer  get default ip index
 *   
 * Input:        p_host: the pointer of the host
 * Output:      N/A
 * Return:      the index
*/
STATIC int WD_SYSTEM _wilddog_sec_getDefaultIpIndex
    (
    Wilddog_Str_T *p_host
    )
{
    int index;
    STATIC int count = 0;
    int totalNum = sizeof(l_defaultAddr_t)/sizeof(Wilddog_Address_T);
    index = _wilddog_sec_hashIndex(p_host, totalNum);

    index = (index + count) % totalNum;
    count++;

    return index;
}

/*
 * Function:    _wilddog_sec_getHost
 * Description: sec layer  get host ip by the name
 *   
 * Input:        p_host: the pointer of the host
 *                  d_port: the port
 * Output:      p_remoteAddr: the pointer of the ip address
 * Return:      the gethostbyname result
*/
int WD_SYSTEM _wilddog_sec_getHost
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
        performtest_timeReset();
#endif

    if(-1 == res)
    {
        i = _wilddog_sec_getDefaultIpIndex(p_host);
        p_remoteAddr->len = l_defaultAddr_t[i].len;
        memcpy(p_remoteAddr->ip, l_defaultAddr_t[i].ip, l_defaultAddr_t[i].len);
    } 
    p_remoteAddr->port = d_port;
    
#undef WILDDOG_COAP_LOCAL_HOST
    return res;
}

