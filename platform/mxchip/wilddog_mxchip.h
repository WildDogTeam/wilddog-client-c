#ifndef _WILDDOG_MXCHIP_H_
#define _WILDDOG_MXCHIP_H_

#ifndef WILDDOG_PORT
#define WILDDOG_PORT 5683
#endif

typedef unsigned char u8 ;
typedef unsigned short u16 ;
typedef unsigned int u32 ;

typedef signed char s8 ;
typedef signed short s16 ;
typedef signed int s32 ;

#define fprintf(fd, format, ...)  printf(format, ##__VA_ARGS__)
#define fflush 

#define INLINE

#endif
