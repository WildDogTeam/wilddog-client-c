
#ifndef _PORT_H_
#define _PORT_H_


#include <stdio.h>

#include "wilddog_config.h"

typedef unsigned int UINT;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;

typedef struct {
	char len;//4 or 16
	char ip[16];
	unsigned short port;
} wilddog_address_t;

int wilddog_gethostbyname(wilddog_address_t* addr,char* host);
int wilddog_openSocket(int* socketId);
int wilddog_closeSocket(int socketId);
int wilddog_send(int socketId,wilddog_address_t*,void* tosend,size_t tosendLength);

/*
 * return <0 have not receive anything >=0 the length
 *
 *
 */

int wilddog_receive(int socketId,wilddog_address_t*,void* toreceive,size_t toreceiveLength);


/* Default to little endian, since this is what most ARM targets are.  */
#ifndef WILDDOG_LITTLE_ENDIAN
#define WILDDOG_LITTLE_ENDIAN    1
#endif

/* Define macros that swap the endian for little endian ports.  */
#ifdef WILDDOG_LITTLE_ENDIAN
#define NX_CHANGE_ULONG_ENDIAN(arg)                         \
    {                                                       \
        ULONG i;                                            \
        ULONG tmp;                                          \
        i = (UINT)arg;                                      \
        /* i = A, B, C, D */                                \
        tmp = i ^ (((i) >> 16) | (i << 16));                \
        /* tmp = i ^ (i ROR 16) = A^C, B^D, C^A, D^B */     \
        tmp &= 0xff00ffff;                                  \
        /* tmp = A^C, 0, C^A, D^B */                        \
        i = ((i) >> 8) | (i<<24);                           \
        /* i = D, A, B, C */                                \
        i = i ^ ((tmp) >> 8);                               \
        /* i = D, C, B, A */                                \
        arg = i;                                            \
    }
#define NX_CHANGE_USHORT_ENDIAN(a)      a = (((a >> 8) | (a << 8)) & 0xFFFF)


#define __SWAP32__(val) ( (ULONG) ((((val) & 0xFF000000) >> 24 ) | (((val) & 0x00FF0000) >> 8) \
             | (((val) & 0x0000FF00) << 8) | (((val) & 0x000000FF) << 24)) )

#define __SWAP16__(val) ( (USHORT) ((((val) & 0xFF00) >> 8) | (((val) & 0x00FF) << 8)))


#ifndef htonl
#define htonl(val)  __SWAP32__(val)
#endif /* htonl */
#ifndef ntohl
#define ntohl(val)  __SWAP32__(val)
#endif /* htonl */

#ifndef htons
#define htons(val)  __SWAP16__(val)
#endif /*htons */

#ifndef ntohs
#define ntohs(val)  __SWAP16__(val)
#endif /*htons */


#else
#define NX_CHANGE_ULONG_ENDIAN(a)
#define NX_CHANGE_USHORT_ENDIAN(a)

#ifndef htons
#define htons(val) (val)
#endif /* htons */

#ifndef ntohs
#define ntohs(val) (val)
#endif /* ntohs */

#ifndef ntohl
#define ntohl(val) (val)
#endif

#ifndef htonl
#define htonl(val) (val)
#endif /* htonl */
#endif

#endif
