#ifndef _WILDDOG_QUECTEL_H_
#define _WILDDOG_QUECTEL_H_
#include "ql_stdlib.h"
#include "ql_memory.h"
#include "ql_trace.h"
#define atoi Ql_atoi
#define memset Ql_memset
#define memcpy Ql_memcpy
#define memcmp Ql_memcmp
#define strcpy Ql_strcpy
#define strncpy Ql_strncpy
#define strcmp Ql_strcmp
#define strncmp Ql_strncmp
#define strchr Ql_strchr
#define strlen Ql_strlen
#define strstr Ql_strstr
#define sprintf Ql_sprintf
#define snprintf Ql_snprintf
#define sscanf Ql_sscanf
#define tolower Ql_tolower
//#define toupper Ql_toupper
//#define isdigit Ql_isdigit
#define fprintf(fd, format, ...)  Ql_Debug_Trace(format, ##__VA_ARGS__)
#define fflush 
#define malloc Ql_MEM_Alloc
#define free Ql_MEM_Free
#define printf Ql_Debug_Trace

#endif
