/*
 * wilddog_common.c
 *
 *  Created on: 2015-05-15
 *      Author: jimmy.pan
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "wilddog_url_parser.h"
#include "wilddog_config.h"
#include "wilddog_ct.h"

STATIC u32 l_wilddog_currTime = 0;

void* wmalloc(int size)
{
    void* ptr = NULL;
    ptr = malloc(size);
    if(NULL != ptr)
    {
        memset(ptr, 0, size);
    }
    return ptr;
}

void wfree(void* ptr)
{
    if(ptr) 
        free(ptr);
    
}

void *wrealloc(void *ptr, size_t size)
{
    return realloc( ptr, size);
}


int _wilddog_atoi(char* str)
{
    return atoi(str);
}

u8 _wilddog_isUrlValid(Wilddog_Str_T * url)
{
    return TRUE;
}

u8 _wilddog_isAuthValid(Wilddog_Str_T * auth)
{
    return TRUE;
}

void _wilddog_setTimeIncrease(u32 ms)
{
    l_wilddog_currTime += ms;
    return;
}
STATIC inline void _wilddog_setTime(u32 ms)
{
    l_wilddog_currTime = ms;
}
u32 _wilddog_getTime(void)
{
    return l_wilddog_currTime;
}

//called once per cycle
void _wilddog_syncTime(void)
{
    static u32 recentTime = 0;
    u32 currTime = _wilddog_getTime();
    u32 calTime = recentTime + _wilddog_ct_getRepoNum() * WILDDOG_RECEIVE_TIMEOUT;
    //currTime should more than or equal calTime
    
    //calTime < recentTime means overflow
    if(calTime < recentTime)
    {
        //overflow
        if(currTime > recentTime)
        {
            //currTime not overflow, means time not increased enough, force to calTime
            currTime = calTime;
        }
        else
        {
            //if curr time <= recent time ,means user did not call the increase function,use default.
            currTime = currTime > recentTime ? (currTime):(calTime);
        }
    }
    else
    {
        //if curr time <= recent time ,means user did not call the increase function,use default.
        currTime = currTime > recentTime ? (currTime):(calTime);
    }
    
    _wilddog_setTime(currTime);
    recentTime = currTime;
    return;
}

