/*
 * debug.h
 *
 *  Created on: 2014年11月6日
 *      Author: x
 */

#ifndef _WILDDOG_DEBUG_H_
#define _WILDDOG_DEBUG_H_

#include <stdint.h>
#include <stddef.h>
#include "Wilddog.h"

#if !defined(arraySize)
#   define arraySize(a)            (sizeof((a))/sizeof((a[0])))
#endif

#define LOG_LEVEL_LOG		1
#define LOG_LEVEL_DEBUG		2
#define LOG_LEVEL_WARN      3
#define LOG_LEVEL_ERROR     4


#define MAX_DEBUG_MESSAGE_LENGTH 120
#define _FILE_PATH          __FILE__
#define LOG_LEVEL LOG_LEVEL_DEBUG         // Set to allow all LOG_LEVEL and above messages to be displayed conditionally by levels.
#define __LOG_LEVEL_TEST(level) (level >= LOG_LEVEL)

#ifdef __cplusplus
extern "C" {
#endif
// Must be provided by main if wanted as extern C definitions
extern uint32_t log_level_at_run_time __attribute__ ((weak));;
void log_print_(int level, int line, const char *func, const char *file, const char *msg, ...);
void debug_output_(const char *) __attribute__ ((weak));

#ifdef __cplusplus
}
#endif

#define WD_LOG(fmt, ...)    do { if ( __LOG_LEVEL_TEST(LOG_LEVEL_LOG)  )  {log_print_(LOG_LEVEL_LOG,__LINE__,__PRETTY_FUNCTION__,_FILE_PATH,fmt, ##__VA_ARGS__);}}while(0)
#define WD_DEBUG(fmt, ...)  do { if ( __LOG_LEVEL_TEST(LOG_LEVEL_DEBUG))  {log_print_(LOG_LEVEL_DEBUG,__LINE__,__PRETTY_FUNCTION__,_FILE_PATH,fmt,##__VA_ARGS__);}}while(0)
#define WD_WARN(fmt, ...)   do { if ( __LOG_LEVEL_TEST(LOG_LEVEL_WARN) )  {log_print_(LOG_LEVEL_WARN,__LINE__,__PRETTY_FUNCTION__,_FILE_PATH,fmt,##__VA_ARGS__);}}while(0)
#define WD_ERROR(fmt, ...)  do { if ( __LOG_LEVEL_TEST(LOG_LEVEL_ERROR) ) {log_print_(LOG_LEVEL_ERROR,__LINE__,__PRETTY_FUNCTION__,_FILE_PATH,fmt,##__VA_ARGS__);}}while(0)
int wilddog_dump(wilddog_t* wilddog,char * buffer,size_t len);
#endif /* DEBUG_H_ */
