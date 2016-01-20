/*
 * debug.h
 *
 *  Created on: 2014-12-12
 *      Author: x
 */

#ifndef _WILDDOG_DEBUG_H_
#define _WILDDOG_DEBUG_H_


#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdint.h>
#endif

#include <stddef.h>

#include "wilddog.h"

#ifdef __cplusplus
extern "C" {
#endif

int wilddog_debug_errcodeCheck(int err);
void wilddog_debug_printUrl(Wilddog_T wilddog);
void wilddog_debug_printnode(const Wilddog_Node_T* node);


#ifdef __cplusplus
}
#endif

#endif /* _WILDDOG_DEBUG_H_ */

