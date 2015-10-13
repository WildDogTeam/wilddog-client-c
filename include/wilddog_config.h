#ifndef _WILDDOG_CONFIG_H_
#define _WILDDOG_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
* define the endian type
*/
#ifndef WILDDOG_LITTLE_ENDIAN
#define WILDDOG_LITTLE_ENDIAN 1
#endif
/*
* define the machine's address type
*/
#ifndef WILDDOG_MACHINE_BITS
#define WILDDOG_MACHINE_BITS 32
#endif
/*
* define the appliance layer's protocol maximum size.
*/
#ifndef WILDDOG_PROTO_MAXSIZE
#define WILDDOG_PROTO_MAXSIZE 1280
#endif
/*
* define the maximum request queue number, means how many packets can be send/observed .
*/
#ifndef WILDDOG_REQ_QUEUE_NUM
#define WILDDOG_REQ_QUEUE_NUM 32
#endif
/*
* define the maximum transmit time, in ms
*/
#ifndef WILDDOG_RETRANSMITE_TIME
#define WILDDOG_RETRANSMITE_TIME 10000
#endif
/*
* define the maximum receive time per host during one wilddog_trySync() period, in ms
*/
#ifndef WILDDOG_RECEIVE_TIMEOUT
#define WILDDOG_RECEIVE_TIMEOUT 100
#endif

#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_CONFIG_H_*/

