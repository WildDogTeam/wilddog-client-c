/*_
 * Copyright 2010 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai
 *    Jimmy.Pan
 */

#ifndef _URL_PARSER_H
#define _URL_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wilddog.h"

typedef struct WILDDOG_URL_T
{
    Wilddog_Str_T     * p_url_host;
    Wilddog_Str_T     * p_url_path;
    Wilddog_Str_T     * p_url_query;
}Wilddog_Url_T;


typedef enum WILDDOG_REFCHANGE_T
{
    WILDDPG_REFCHG_SELF = 0x00,
    WILDDOG_REFCHG_PARENT = 0x01,
    WILDDOG_REFCHG_ROOT = 0x02,
    WILDDOG_REFCHG_CHILD = 0x03
}Wilddog_RefChange_T;


extern Wilddog_Url_T * _wilddog_url_parseUrl(Wilddog_Str_T * url);
extern void _wilddog_url_freeParsedUrl(Wilddog_Url_T * p_url);
extern BOOL _wilddog_url_diff
    (
    Wilddog_Url_T* p_src, 
    Wilddog_Url_T* p_dst
    );
extern Wilddog_Return_T _wilddog_url_getPath
    (
    Wilddog_Str_T* p_srcPath, 
    Wilddog_RefChange_T type, 
    Wilddog_Str_T* str, 
    Wilddog_Str_T** pp_dstPath
    );
extern Wilddog_Str_T *_wilddog_url_getKey(Wilddog_Str_T * p_path);
#ifdef __cplusplus
}
#endif

#endif /* _URL_PARSER_H */


