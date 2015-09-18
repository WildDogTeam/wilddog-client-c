
#ifndef _WILDDOG_COMMON_H_
#define _WILDDOG_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "wilddog.h"
#include "wilddog_config.h"

extern void* wmalloc(int size);
extern void wfree(void* ptr);
extern int _wilddog_atoi(char* str);
extern void *wrealloc(void *ptr, size_t oldSize, size_t newSize);

extern u8 _wilddog_isUrlValid(Wilddog_Str_T * url);
extern u8 _wilddog_isAuthValid(Wilddog_Str_T * auth, int len);
extern void INLINE _wilddog_setTimeIncrease(u32 ms);
extern void _wilddog_syncTime(void);
extern u32 _wilddog_getTime(void);

#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_COMMON_H_*/

