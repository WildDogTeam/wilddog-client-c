/*
 * Wilddog.h
 *
 *  Created on: 2015-12-12
 *      Author: x
 */

#ifndef __WILDDOG_CONN_COAP_H_
#define __WILDDOG_CONN_COAP_H_

#include "wilddog.h"
#include "pdu.h"


/*@ CONFIG
*   coap configure
*/

typedef struct WILDDOG_CONN_COAP_PACKET_NODE_T{
    
    struct WILDDOG_CONN_COAP_PACKET_NODE_T *next;

    u32 d_observer_cnt;
    u8 d_observer_flag;  /*observe 0x01*/
    u8 d_separate_flag; 
    u8 d_dismsgid_fig;
    u8 d_blockIdx;
    coap_pdu_t* p_CoapPkt;  

}Wilddog_Conn_Coap_PacketNode_T;

typedef struct WILDDOG_CONN_COAP_PCB{
    
    struct WILDDOG_CONN_COAP_PACKET_NODE_T *P_hd;
    u16 d_pkt_idx;
    u32 d_pkt_cnt;
}Wilddog_Conn_Coap_PCB_T;




#endif /* __WILDDOG_CONN_COAP_H_ */

