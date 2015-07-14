#ifndef _WILDDOG_CONFIG_H_
#define _WILDDOG_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*
* define the endian type
*/
#define WILDDOG_LITTLE_ENDIAN 1
/*
* define the machine's address type
*/
#define WILDDOG_MACHINE_BITS 32
/*
* define the appliance layer's protocol maximum size.
*/
#define WILDDOG_PROTO_MAXSIZE 1280
/*
* define the maximum request queue number, means how many packets can be send/observed .
*/
#define WILDDOG_REQ_QUEUE_NUM 32
/*
* define the maximum transmit time, in ms
*/
#define WILDDOG_RETRANSMITE_TIME 10000
/*
* define the maximum receive time per host during one wilddog_trySync() period, in ms
*/
#define WILDDOG_RECEIVE_TIMEOUT 100

#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_CONFIG_H_*/

