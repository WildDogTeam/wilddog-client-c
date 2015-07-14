
#ifndef _WILDDOG_PAYLOAD_H_
#define _WILDDOG_PAYLOAD_H_
#ifdef __cplusplus
extern "C"
{
#endif

#include "wilddog.h"

extern Wilddog_Payload_T * _wilddog_node2Payload
    (
    Wilddog_Node_T * p_node
    );
extern Wilddog_Node_T *_wilddog_payload2Node
    (
    Wilddog_Payload_T* p_data
    );
#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_PAYLOAD_H_*/

