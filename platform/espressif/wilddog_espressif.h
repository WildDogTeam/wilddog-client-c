#ifndef _WILDDOG_EPRESSIF_H_
#define _WILDDOG_EPRESSIF_H_
#include "mem.h"
#include "osapi.h"
#include "c_types.h"
#define memcmp os_memcmp 
#define memcpy os_memcpy 
#define memmove os_memmove 
#define memset os_memset 
#define strcat os_strcat 
#define strchr os_strchr 
#define strcmp os_strcmp 
#define strcpy os_strcpy 
#define strlen os_strlen 
#define strncmp os_strncmp 
#define strncpy os_strncpy 
#define strstr os_strstr 


#define malloc pvPortMalloc
#define free vPortFree
#define printf os_printf

#define stdout 2
#define fprintf(fd, format, ...)  os_printf(format, ##__VA_ARGS__)
#define fflush 

#define sprintf os_sprintf
#define snprintf(str,size,format,...)  sprintf(str, format, ##__VA_ARGS__)
#define sscanf

#endif
