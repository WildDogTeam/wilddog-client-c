/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog.h
 *
 * Description: Wilddog's main header files.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Jimmy.Pan       2015-05-15  Create file.
 * 0.4.6        Jimmy.Pan       2015-09-06  Add notes.
 *
 */

#ifndef _WILDDOG_H_
#define _WILDDOG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef WILDDOG_PORT_TYPE_ESP
#include <stdio.h>
#endif

#include "wilddog_config.h"

/* if do not need debug log, undefine it to cost down ROM space. */
#define WILDDOG_DEBUG

#define WD_DEBUG_ALL    0
#define WD_DEBUG_LOG    1
#define WD_DEBUG_WARN   2
#define WD_DEBUG_ERROR  3
#define WD_DEBUG_NODBG  4

#ifdef WILDDOG_PORT_TYPE_ESP    
#include "wilddog_espressif.h"
#define FAR ICACHE_FLASH_ATTR
#define WD_SYSTEM FAR
#elif defined WILDDOG_PORT_TYPE_QUCETEL
#include "wilddog_quectel.h"
#elif defined(WILDDOG_PORT_TYPE_MXCHIP)
#include "wilddog_mxchip.h"
#else
typedef unsigned char u8 ;
typedef unsigned short u16 ;
typedef unsigned long u32 ;
typedef signed char s8 ;
typedef signed short s16 ;
typedef signed long s32 ;
#endif

#ifndef STATIC
#define STATIC static
#endif

#ifndef INLINE
#define INLINE __inline__
#endif

#ifndef VOLATILE
#define VOLATILE volatile
#endif

#ifndef WD_SYSTEM
#define WD_SYSTEM
#endif
#ifndef TRUE
#define TRUE (1==1)
#endif
#ifndef FALSE
#define FALSE (1==0)
#endif

#ifdef WILDDOG_DEBUG
#define DEBUG_LEVEL WD_DEBUG_ERROR

#define wilddog_debug_level(level, format,...) do{if(level >= DEBUG_LEVEL){ \
    printf("func:%s LINE: %d: "format"\r\n", __func__, __LINE__, ##__VA_ARGS__); \
    }}while(0)

#define wilddog_debug(format,...) wilddog_debug_level(WD_DEBUG_NODBG, \
    format,##__VA_ARGS__)

#else
#define wilddog_debug_level(level, format,...) 
#define wilddog_debug(format,...) do{printf(format"\r\n", ##__VA_ARGS__);}while(0)

#endif

#define wilddog_assert(_arg, _return) do{if((_arg)==0) \
    {printf("%s %d, assert failed!\r\n",__func__, __LINE__); \
        return(_return);}}while(0)

#if WILDDOG_MACHINE_BITS == 8
typedef float wFloat;
#else 
typedef double wFloat;
#endif

#ifndef BOOL
#define BOOL int
#endif

typedef u8 Wilddog_Str_T;

typedef size_t (*Wilddog_Func_T)();

#define WILDDOG_NODE_TYPE_FALSE  0
#define WILDDOG_NODE_TYPE_TRUE   1
#define WILDDOG_NODE_TYPE_NULL   2
#define WILDDOG_NODE_TYPE_NUM    3
#define WILDDOG_NODE_TYPE_FLOAT  4
#define WILDDOG_NODE_TYPE_BYTESTRING 5
#define WILDDOG_NODE_TYPE_UTF8STRING 6
#define WILDDOG_NODE_TYPE_OBJECT 7


typedef enum WILDDOG_EVENTTYPE_T
{
    WD_ET_NULL        = 0x00,
    WD_ET_VALUECHANGE = 0x01,
    WD_ET_CHILDADD    = 0x02,
    WD_ET_CHILDCHANGE = 0x04,
    WD_ET_CHILDREMOVE = 0x08,
    WD_ET_CHILDMOVED  = 0x10,
}Wilddog_EventType_T;

typedef enum WILDDOG_RETURN_T
{
/*****************client inner error*******************/
    WILDDOG_ERR_NOERR = 0,
    WILDDOG_ERR_NULL = -1,
    WILDDOG_ERR_INVALID = -2,
    
    WILDDOG_ERR_SENDERR = -3,
    WILDDOG_ERR_OBSERVEERR =-4,
    WILDDOG_ERR_SOCKETERR = -5,
    WILDDOG_ERR_NOTAUTH = -7,
    WILDDOG_ERR_QUEUEFULL = -8,
    WILDDOG_ERR_MAXRETRAN = -9,
    WILDDOG_ERR_RECVTIMEOUT = -10,
    WILDDOG_ERR_RECVNOMATCH = -11,
    WILDDOG_ERR_CLIENTOFFLINE = -12,
    WILDDOG_ERR_RECONNECT = -13, 
    /*
     * Using auto detect udp session tech, sdk maybe trigger reconnect event in 
     * first serval minutes, and the snapshot in callback maybe newer than local
     * , or the same as local.
    */
/*****************HTTP return error******************/
    WILDDOG_HTTP_OK = 200,
    WILDDOG_HTTP_CREATED = 201,
    WILDDOG_HTTP_NO_CONTENT = 204,

    WILDDOG_HTTP_NOT_MODIFIED = 304,

    WILDDOG_HTTP_BAD_REQUEST = 400,
    WILDDOG_HTTP_UNAUTHORIZED = 401,
    WILDDOG_HTTP_FORBIDDEN = 403,
    WILDDOG_HTTP_NOT_FOUND = 404,
    WILDDOG_HTTP_METHOD_NOT_ALLOWED = 405,
    WILDDOG_HTTP_NOT_ACCEPTABLE = 406,
    WILDDOG_HTTP_PRECONDITION_FAIL = 412,
    WILDDOG_HTTP_REQ_ENTITY_TOOLARGE = 413,
    WILDDOG_HTTP_UNSUPPORT_MEDIA = 415,

    WILDDOG_HTTP_INTERNAL_SERVER_ERR = 500,
    WILDDOG_HTTP_NOT_IMPLEMENTED = 501,
    WILDDOG_HTTP_BAD_GATEWAY = 502,
    WILDDOG_HTTP_SERVICE_UNAVAILABLE = 503,
    WILDDOG_HTTP_GATEWAY_TIMEOUT = 504,
    WILDDOG_HTTP_PROXY_NOT_SUPPORT = 505
}Wilddog_Return_T;

/*****************************************************************************/
/*                             response code                                 */
/*****************************************************************************/

typedef struct
{
    u8 len;
    u8 ip[16];
    u16 port;
} Wilddog_Address_T;

typedef struct WILDDOG_NODE
{
    struct WILDDOG_NODE *p_wn_next, *p_wn_prev;
    struct WILDDOG_NODE *p_wn_child, *p_wn_parent;
    u8 d_wn_type;
    int d_wn_len;
    Wilddog_Str_T *p_wn_value;
    Wilddog_Str_T *p_wn_key;
}Wilddog_Node_T;

typedef struct WILDDOG_PAYLOAD_TYPE
{
    u8* p_dt_data;
    int d_dt_pos;
    int d_dt_len;
}Wilddog_Payload_T;

typedef void (*onQueryFunc)
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err
    );

typedef void (*onSetFunc)
    (
    void* arg, 
    Wilddog_Return_T err
    );

typedef void (*onPushFunc)
    (
    Wilddog_Str_T * p_newPath,
    void* arg, 
    Wilddog_Return_T err
    );

typedef onSetFunc onRemoveFunc;
typedef onSetFunc onAuthFunc;
typedef onQueryFunc onEventFunc;
typedef onSetFunc onDisConnectFunc;

typedef size_t Wilddog_T;

extern void* wmalloc(int size);
extern void wfree(void* ptr);
extern void *wrealloc(void *ptr, size_t oldSize, size_t newSize);

#include "wilddog_api.h"
#include "wilddog_debug.h"
//#include "wilddog_endian.h"
#ifdef __cplusplus
}
#endif

#endif/*_WILDDOG_H_*/

