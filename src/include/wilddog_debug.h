/*
 * debug.h
 *
 *  Created on: 2014-12-12
 *      Author: x
 */

#ifndef _WILDDOG_DEBUG_H_
#define _WILDDOG_DEBUG_H_

#include <stdint.h>
#include <stddef.h>
 
#include "wilddog_url_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

int _wilddog_debug_errcodeCheck(int err);
void _wilddog_debug_printUrl(Wilddog_Url_T* url);
void wilddog_debug_printnode(const Wilddog_Node_T* node);


#ifdef __cplusplus
}
#endif

#endif /* _WILDDOG_DEBUG_H_ */

