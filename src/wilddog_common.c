/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_common.c
 *
 * Description: common functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Jimmy.Pan       2015-05-15  Create file.
 * 0.4.3        Jimmy.Pan       2015-07-04  Add annotation.
 *
 */
 
#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "wilddog.h"
#include "wilddog_url_parser.h"
#include "wilddog_config.h"
#include "wilddog_ct.h"

/* store current time */
STATIC VOLATILE u32 l_wilddog_currTime = 0;
/*
 * Function:    wmalloc
 * Description: alloc a memory block.
 * Input:       size: Size you want to alloc.
 * Output:      N/A
 * Return:      Pointer to the memory.
*/
void* WD_SYSTEM wmalloc(int size)
{
    void* ptr = NULL;

    if(!size)
        return NULL;
    
    ptr = (void *)malloc(size);
    if(NULL != ptr)
    {
        memset(ptr, 0, size);
    }
    return ptr;
}
/*
 * Function:    wfree
 * Description: Free the pointer.
 * Input:       ptr: pointer to memory you want free.
 * Output:      N/A
 * Return:      N/A
*/
void WD_SYSTEM wfree(void* ptr)
{
    if(ptr) 
    {
        free(ptr);
    }
}
/*
 * Function:    wrealloc
 * Description: Realloc the souce pointer's length.
 * Input:       ptr: old pointer.
 *              size: new length you want.
 * Output:      N/A
 * Return:      New pointer.
*/
void * WD_SYSTEM wrealloc(void *ptr, size_t oldSize, size_t newSize)
{
#if defined(WILDDOG_PORT_TYPE_QUCETEL) || defined(WILDDOG_PORT_TYPE_ESP)
    void* tmpPtr = NULL;
#endif

    if(!ptr)
        return wmalloc(newSize);
#if defined(WILDDOG_PORT_TYPE_QUCETEL) || defined(WILDDOG_PORT_TYPE_ESP)

    tmpPtr = (void*)wmalloc(newSize);
    if(!tmpPtr)
    {
        wfree(ptr);
        return NULL;
    }
    memcpy(tmpPtr, ptr, oldSize > newSize? (newSize):(oldSize));
    wfree(ptr);
    return tmpPtr;
#else
    return realloc( ptr, newSize);
#endif
}
/*
 * Function:    _wilddog_atoi
 * Description: char string to int.
 * Input:       str: string.
 * Output:      N/A
 * Return:      N/A
*/
int WD_SYSTEM _wilddog_atoi(char* str)
{
    return atoi(str);
}
/*
 * Function:    _wilddog_isUrlValid
 * Description: url valid check.
 * Input:       url.
 * Output:      N/A
 * Return:      N/A
*/
u8 WD_SYSTEM _wilddog_isUrlValid(Wilddog_Str_T * url)
{
    return TRUE;
}
/*
 * Function:    _wilddog_isAuthValid
 * Description: auth data valid check.
 * Input:       auth: pointer to auth data.
 *              len: auth data length.
 * Output:      N/A
 * Return:      N/A
*/
u8 WD_SYSTEM _wilddog_isAuthValid(u8 * auth, int len)
{
    return TRUE;
}
/*
 * Function:    _wilddog_setTimeIncrease
 * Description: Set time increase, called by user.
 * Input:       ms: Time want to increase.
 * Output:      N/A
 * Return:      N/A
*/
void INLINE WD_SYSTEM _wilddog_setTimeIncrease(u32 ms)
{
    l_wilddog_currTime += ms;
    return;
}
/*
 * Function:    _wilddog_setTime
 * Description: Set current time.
 * Input:       ms: Time want to set.
 * Output:      N/A
 * Return:      N/A
*/
STATIC INLINE void WD_SYSTEM _wilddog_setTime(u32 ms)
{
    l_wilddog_currTime = ms;
}
/*
 * Function:    _wilddog_getTime
 * Description: Get current time.
 * Input:       N/A
 * Output:      N/A
 * Return:      Current time.
*/
u32 WD_SYSTEM _wilddog_getTime(void)
{
    return l_wilddog_currTime;
}

/*
 * Function:    _wilddog_syncTime
 * Description: Called every cycle, calculate time passed.
 * Input:       N/A
 * Output:      N/A
 * Return:      N/A
*/
void WD_SYSTEM _wilddog_syncTime(void)
{
    static u32 recentTime = 0;
    u32 currTime = _wilddog_getTime();

    /*every repo will receive WILDDOG_RECEIVE_TIMEOUT ms*/
    u32 calTime = recentTime + WILDDOG_RECEIVE_TIMEOUT;
    
    currTime = (currTime != recentTime ? (currTime):(calTime));
    
    _wilddog_setTime(currTime);
    recentTime = currTime;
    return;
}

