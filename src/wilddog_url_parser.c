/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_ur_parser.c
 *
 * Description: url functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Jimmy.Pan       2015-05-15  Create file.
 * 0.4.3        Jimmy.Pan       2015-07-04  Add annotation.
 *
 */

/*_
 * Copyright 2010-2011 Scyphus Solutions Co. Ltd.  All rights reserved.
 *
 * Authors:
 *      Hirochika Asai
 */

#ifndef WILDDOG_PORT_TYPE_ESP
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "wilddog.h"
#include "wilddog_url_parser.h"
#include "wilddog_common.h"

/*
 * URL storage
 */
typedef struct parsed_url {
    char *scheme;               /* mandatory */
    char *host;                 /* mandatory */
    char *port;                 /* optional */
    char *path;                 /* optional */
    char *query;                /* optional */
    char *fragment;             /* optional */
    char *username;             /* optional */
    char *password;             /* optional */
}ParsedUrl_T;

/*
 * Prototype declarations
 */
STATIC INLINE int _is_scheme_char(int);
struct parsed_url * parse_url(const char *);
void parsed_url_free(struct parsed_url *);

/*
 * Check whether the character is permitted in scheme string
 */
#if 0
#define isalpha(c) wd_isalpha(c)
STATIC int WD_SYSTEM wd_isalpha(int c)
{
    if(c >= 'a' && c <= 'z')
        return 2;
    if(c >= 'A' && c <= 'Z')
        return 1;
    return 0;
}
#endif
STATIC INLINE int WD_SYSTEM _is_scheme_char(int c)
{
    return (!isalpha(c) && '+' != c && '-' != c && '.' != c) ? 0 : 1;
}

/*
 * See RFC 1738, 3986
 */
struct parsed_url * WD_SYSTEM parse_url(const char *url)
{
    struct parsed_url *purl;
    const char *tmpstr;
    const char *curstr;
    int len;
    int i;
    int userpass_flag;
    int bracket_flag;

    /* Allocate the parsed url storage */
    purl = wmalloc(sizeof(struct parsed_url));
    if ( NULL == purl ) {
        return NULL;
    }
    purl->scheme = NULL;
    purl->host = NULL;
    purl->port = NULL;
    purl->path = NULL;
    purl->query = NULL;
    purl->fragment = NULL;
    purl->username = NULL;
    purl->password = NULL;

    curstr = url;

    /*
     * <scheme>:<scheme-specific-part>
     * <scheme> := [a-z\+\-\.]+
     *             upper case = lower case for resiliency
     */
    /* Read scheme */
    tmpstr = strchr(curstr, ':');
    if ( NULL == tmpstr ) {
        /* Not found the character */
        parsed_url_free(purl);
        return NULL;
    }
    /* Get the scheme length */
    len = tmpstr - curstr;
    /* Check restrictions */
    for ( i = 0; i < len; i++ ) {
        if ( !_is_scheme_char(curstr[i]) ) {
            /* Invalid format */
            parsed_url_free(purl);
            return NULL;
        }
    }
    /* Copy the scheme to the storage */
    purl->scheme = wmalloc(sizeof(char) * (len + 1));
    if ( NULL == purl->scheme ) {
        parsed_url_free(purl);
        return NULL;
    }
    (void)strncpy(purl->scheme, curstr, len);
    purl->scheme[len] = '\0';
    /* Make the character to lower if it is upper case. */
    for ( i = 0; i < len; i++ ) {
        purl->scheme[i] = (char)tolower((int)(purl->scheme[i]));
    }
    /* Skip ':' */
    tmpstr++;
    curstr = tmpstr;

    /*
     * //<user>:<password>@<host>:<port>/<url-path>
     * Any ":", "@" and "/" must be encoded.
     */
    /* Eat "//" */
    for ( i = 0; i < 2; i++ ) {
        if ( '/' != *curstr ) {
            parsed_url_free(purl);
            return NULL;
        }
        curstr++;
    }

    /* Check if the user (and password) are specified. */
    userpass_flag = 0;
    tmpstr = curstr;
    while ( '\0' != *tmpstr ) {
        if ( '@' == *tmpstr ) {
            /* Username and password are specified */
            userpass_flag = 1;
            break;
        } else if ( '/' == *tmpstr ) {
            /* End of <host>:<port> specification */
            userpass_flag = 0;
            break;
        }
        tmpstr++;
    }

    /* User and password specification */
    tmpstr = curstr;
    if ( userpass_flag ) {
        /* Read username */
        while ( '\0' != *tmpstr && ':' != *tmpstr && '@' != *tmpstr ) {
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->username = wmalloc(sizeof(char) * (len + 1));
        if ( NULL == purl->username ) {
            parsed_url_free(purl);
            return NULL;
        }
        (void)strncpy(purl->username, curstr, len);
        purl->username[len] = '\0';
        /* Proceed current pointer */
        curstr = tmpstr;
        if ( ':' == *curstr ) {
            /* Skip ':' */
            curstr++;
            /* Read password */
            tmpstr = curstr;
            while ( '\0' != *tmpstr && '@' != *tmpstr ) {
                tmpstr++;
            }
            len = tmpstr - curstr;
            purl->password = wmalloc(sizeof(char) * (len + 1));
            if ( NULL == purl->password ) {
                parsed_url_free(purl);
                return NULL;
            }
            (void)strncpy(purl->password, curstr, len);
            purl->password[len] = '\0';
            curstr = tmpstr;
        }
        /* Skip '@' */
        if ( '@' != *curstr ) {
            parsed_url_free(purl);
            return NULL;
        }
        curstr++;
    }

    if ( '[' == *curstr ) {
        bracket_flag = 1;
    } else {
        bracket_flag = 0;
    }
    /* Proceed on by delimiters with reading host */
    tmpstr = curstr;
    while ( '\0' != *tmpstr ) {
        if ( bracket_flag && ']' == *tmpstr ) {
            /* End of IPv6 address. */
            tmpstr++;
            break;
        } else if ( !bracket_flag && (':' == *tmpstr || '/' == *tmpstr) ) {
            /* Port number is specified. */
            break;
        }
        tmpstr++;
    }
    len = tmpstr - curstr;
    purl->host = wmalloc(sizeof(char) * (len + 1));
    if ( NULL == purl->host || len <= 0 ) {
        parsed_url_free(purl);
        return NULL;
    }
    (void)strncpy(purl->host, curstr, len);
    purl->host[len] = '\0';
    curstr = tmpstr;

    /* Is port number specified? */
    if ( ':' == *curstr ) {
        curstr++;
        /* Read port number */
        tmpstr = curstr;
        while ( '\0' != *tmpstr && '/' != *tmpstr ) {
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->port = wmalloc(sizeof(char) * (len + 1));
        if ( NULL == purl->port ) {
            parsed_url_free(purl);
            return NULL;
        }
        (void)strncpy(purl->port, curstr, len);
        purl->port[len] = '\0';
        curstr = tmpstr;
    }

    /* End of the string */
    if ( '\0' == *curstr ) {
        return purl;
    }

    /* Skip '/' */
    if ( '/' != *curstr ) {
        parsed_url_free(purl);
        return NULL;
    }
    curstr++;

    /* Parse path */
    tmpstr = curstr;
    while ( '\0' != *tmpstr && '#' != *tmpstr  && '?' != *tmpstr ) {
        tmpstr++;
    }
    len = tmpstr - curstr;
    purl->path = wmalloc(sizeof(char) * (len + 1));
    if ( NULL == purl->path ) {
        parsed_url_free(purl);
        return NULL;
    }
    (void)strncpy(purl->path, curstr, len);
    purl->path[len] = '\0';
    curstr = tmpstr;

    /* Is query specified? */
    if ( '?' == *curstr ) {
        /* Skip '?' */
        curstr++;
        /* Read query */
        tmpstr = curstr;
        while ( '\0' != *tmpstr && '#' != *tmpstr ) {
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->query = wmalloc(sizeof(char) * (len + 1));
        if ( NULL == purl->query ) {
            parsed_url_free(purl);
            return NULL;
        }
        (void)strncpy(purl->query, curstr, len);
        purl->query[len] = '\0';
        curstr = tmpstr;
    }

    /* Is fragment specified? */
    if ( '#' == *curstr ) {
        /* Skip '#' */
        curstr++;
        /* Read fragment */
        tmpstr = curstr;
        while ( '\0' != *tmpstr ) {
            tmpstr++;
        }
        len = tmpstr - curstr;
        purl->fragment = wmalloc(sizeof(char) * (len + 1));
        if ( NULL == purl->fragment ) {
            parsed_url_free(purl);
            return NULL;
        }
        (void)strncpy(purl->fragment, curstr, len);
        purl->fragment[len] = '\0';
        curstr = tmpstr;
    }

    return purl;
}

/*
 * Free memory of parsed url
 */
void WD_SYSTEM parsed_url_free(struct parsed_url *purl)
{
    if ( NULL != purl ) {
        if ( NULL != purl->scheme ) {
            wfree(purl->scheme);
        }
        if ( NULL != purl->host ) {
            wfree(purl->host);
        }
        if ( NULL != purl->port ) {
            wfree(purl->port);
        }
        if ( NULL != purl->path ) {
            wfree(purl->path);
        }
        if ( NULL != purl->query ) {
            wfree(purl->query);
        }
        if ( NULL != purl->fragment ) {
            wfree(purl->fragment);
        }
        if ( NULL != purl->username ) {
            wfree(purl->username);
        }
        if ( NULL != purl->password ) {
            wfree(purl->password);
        }
        wfree(purl);
    }
}

/*
 * Function:    _wilddogurl_checkPath
 * Description: check path is valid or not.
 * Input:       p_path: Input path.
 * Output:      N/A
 * Return:      success return 0, failed return nagative number.
*/

int WD_SYSTEM _wilddogurl_checkPath(Wilddog_Str_T *p_path)
{
    int len , i;
    u8 data;
    wilddog_assert(p_path, WILDDOG_ERR_NULL);

    len = strlen((const char*)p_path);

    if( (p_path[0] == '/') || ((len >= 1) && (p_path[len - 1] == '/')))
        return WILDDOG_ERR_INVALID;

    for(i = 0; i < len; i++)
    {
        data = p_path[i];
        if( data <  32  || \
            data == '.' || \
            data == '$' || \
            data == '#' || \
            data == '[' || \
            data == ']' || \
            data ==  127)
        {
            return WILDDOG_ERR_INVALID;
        }
        if(i < len - 1)
        {
            if(data == '/' && p_path[i + 1] == '/')
                return WILDDOG_ERR_INVALID;
        }
    }
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_url_parseUrl
 * Description: parse url using wilddog format.
 * Input:       url: Input url.
 * Output:      N/A
 * Return:      Pointer to the url structure.
*/
Wilddog_Url_T * WD_SYSTEM _wilddog_url_parseUrl(Wilddog_Str_T * url)
{
    struct parsed_url * p_paresd_url = NULL;
    Wilddog_Url_T * p_wd_url = NULL;
    int len  = 2;
    
    wilddog_assert(url, NULL);
    
    p_paresd_url = parse_url((char*)url);
    if(NULL == p_paresd_url)
        return NULL;
    
    p_wd_url = (Wilddog_Url_T *)wmalloc(sizeof(Wilddog_Url_T));
    if(NULL == p_wd_url)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "cannot malloc p_wd_url!\n");
        parsed_url_free(p_paresd_url);
        return NULL;
    }
    
    if(NULL != p_paresd_url->host)
    {
        p_wd_url->p_url_host = (Wilddog_Str_T *)wmalloc( \
            strlen(p_paresd_url->host) + 1);
        if(NULL == p_wd_url->p_url_host)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "cannot malloc p_url_host!\n");
            _wilddog_url_freeParsedUrl(p_wd_url);
            parsed_url_free(p_paresd_url);
            return NULL;
        }
        strncpy((char*)p_wd_url->p_url_host, (char*)p_paresd_url->host, \
            strlen((const char*)p_paresd_url->host));
    }
    else 
        return NULL;
    
    if(NULL == p_paresd_url->path)
    {
        p_wd_url->p_url_path = (Wilddog_Str_T *)wmalloc(len);
        if(NULL == p_wd_url->p_url_path)
        {
            _wilddog_url_freeParsedUrl(p_wd_url);
            parsed_url_free(p_paresd_url);
            return NULL;
        }
        p_wd_url->p_url_path[0] = '/';
    }
    else
    {
        len += strlen((const char*)p_paresd_url->path);

        if(WILDDOG_ERR_NOERR != \
           _wilddogurl_checkPath((Wilddog_Str_T*)p_paresd_url->path)
           )
        {
            _wilddog_url_freeParsedUrl(p_wd_url);
            parsed_url_free(p_paresd_url);
            return NULL;
        }
        p_wd_url->p_url_path = (Wilddog_Str_T *)wmalloc(len + 1);
        if(NULL == p_wd_url->p_url_path)
        {
            _wilddog_url_freeParsedUrl(p_wd_url);
            parsed_url_free(p_paresd_url);
            return NULL;
        }
        snprintf((char*)p_wd_url->p_url_path, len, "/%s", \
            p_paresd_url->path);
        len = strlen((const char*)p_wd_url->p_url_path);
        if(len > 1 && p_wd_url->p_url_path[len - 1] == '/')
        {
            p_wd_url->p_url_path[len - 1] = 0;
        }
    }
    
    if(NULL != p_paresd_url->query)
    {
        p_wd_url->p_url_query = (Wilddog_Str_T *)wmalloc( \
            strlen((const char*)p_paresd_url->query) + 1);
        if(NULL == p_wd_url->p_url_query)
        {
            _wilddog_url_freeParsedUrl(p_wd_url);
            parsed_url_free(p_paresd_url);
            return NULL;
        }
        strncpy((char*)p_wd_url->p_url_query, (char*)p_paresd_url->query, \
            strlen((const char*)p_paresd_url->query));
    }
    
    parsed_url_free(p_paresd_url);
    return p_wd_url;
}

/*
 * Function:    _wilddog_url_freeParsedUrl
 * Description: free wilddog format url.
 * Input:       url: Input url.
 * Output:      N/A
 * Return:      Pointer to the url structure.
*/
void WD_SYSTEM _wilddog_url_freeParsedUrl(Wilddog_Url_T * p_url)
{
    if(NULL == p_url)
        return;
    if(NULL != p_url->p_url_host)
        wfree(p_url->p_url_host);
    
    if(NULL != p_url->p_url_path)
        wfree(p_url->p_url_path);

    if(NULL != p_url->p_url_query)
        wfree(p_url->p_url_query);
    
    wfree(p_url);
    return;
}

/*
 * Function:    _wilddog_url_diff
 * Description: Compare two url.
 * Input:       p_src: Source url.
 *              p_dst: Destination url.
 * Output:      N/A
 * Return:      Same return TRUE, others return FALSE.
*/
BOOL WD_SYSTEM _wilddog_url_diff
    (
    Wilddog_Url_T* p_src,
    Wilddog_Url_T* p_dst
    )
{
    if(
        strcmp((const char*)p_src->p_url_host,(const char*)p_dst->p_url_host) || \
        strcmp((const char*)p_src->p_url_path, (const char*)p_dst->p_url_path)
        )
      {
          return TRUE;
      }
    return FALSE;
}

/*
 * Function:    _wilddog_url_getKey
 * Description: Get the Key of the path.
 * Input:       p_path: The path.
 * Output:      N/A
 * Return:      Pointer to the Key.
*/
Wilddog_Str_T * WD_SYSTEM _wilddog_url_getKey(Wilddog_Str_T * p_path)
{
    int i, len, pos = 0;
    Wilddog_Str_T* p_str = NULL;
    if(NULL == p_path)
        return NULL;
    
    len = strlen((const char*)p_path);
    if(len == 1 && p_path[0] == '/')
    {
        p_str = (Wilddog_Str_T*)wmalloc(len + 1);
        if(NULL == p_str)
            return NULL;
        p_str[0] = '/';
        return p_str;
    }
    for(i = len - 1; i >=0; i--)
    {
        if(p_path[len - 1] == '/')
            continue;
        if(p_path[i] == '/')
        {
            pos = i;
            break;
        }
    }
    p_str = (Wilddog_Str_T*)wmalloc(len - pos);
    if(NULL == p_str)
        return NULL;
    memcpy((char*)p_str, (char*)(p_path + pos + 1), len - pos);
    return p_str;
}

/*
 * Function:    _wilddog_url_getParentStr
 * Description: Get the Parent path of the source path.
 * Input:       p_src_path: The source path.
 * Output:      N/A
 * Return:      Pointer to the parent's path.
*/
STATIC Wilddog_Str_T * WD_SYSTEM _wilddog_url_getParentStr
    (
    Wilddog_Str_T* p_src_path
    )
{
    int i;
    int size = 0;
    int pos = 0;
    Wilddog_Str_T* p_path = NULL;

    if(!p_src_path)
        return NULL;
    
    size = strlen((const char*)p_src_path);
    if(size == 1 && p_src_path[0] == '/')
    {
        return NULL;
    }
    for(i = size - 1; i >= 0; i--)
    {
        if(p_src_path[size - 1] == '/')
            continue;
        if(p_src_path[i] == '/')
        {
            pos = i;
            break;
        }
    }
    /* path do not have '/', invalid */
    if((!pos) && (p_src_path[pos] != '/'))
        return p_path;

    if(pos == 0)
    {
        p_path = (Wilddog_Str_T *)wmalloc(2);
        if(NULL == p_path)
            return NULL;
        p_path[0] = '/';
        return p_path;
    }
    
    p_path = (Wilddog_Str_T *)wmalloc(pos + 1);
    if(NULL == p_path)
        return NULL;
    
    strncpy((char*)p_path, (char*)p_src_path, pos);
    return p_path;
}

/*
 * Function:    _wilddog_url_getRootStr
 * Description: Get the root path of the source path.
 * Input:       p_src_path: The source path.
 * Output:      N/A
 * Return:      Pointer to the root's path.
*/
STATIC Wilddog_Str_T * WD_SYSTEM _wilddog_url_getRootStr
    (
    Wilddog_Str_T *p_srcPath
    )
{
    Wilddog_Str_T* p_path = NULL;

    p_path = (Wilddog_Str_T *)wmalloc(2);
    if(NULL == p_path)
        return NULL;
    strcpy((char*)p_path, "/");
    p_path[1] = 0;

    return p_path;
}

/*
 * Function:    _wilddog_url_getChildStr
 * Description: Create the child path of the source path.
 * Input:       p_src_path: The source path.
 *              childName: child's name or path
 * Output:      N/A
 * Return:      Pointer to the child's path.
*/
STATIC Wilddog_Str_T * WD_SYSTEM _wilddog_url_getChildStr
    (
    Wilddog_Str_T*p_srcPath, 
    Wilddog_Str_T* childName
    )
{
    Wilddog_Str_T *p_path = NULL;
    int len = 0;
    int srcLen = 0, childLen = 0;
    int dstPathLen = 0;
    
    if(!childName || !p_srcPath)
        return NULL;
    srcLen = strlen((const char*)p_srcPath);
    childLen = strlen((const char*)childName);

    if(childLen == 0 || \
        (childLen == 1 && childName[0] == '/'))
    {
        return NULL;
    }
    
    if(WILDDOG_ERR_NOERR != _wilddogurl_checkPath(childName))
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "check path error!");
        return NULL;
    }
    len = srcLen + childLen + 3;
    p_path = (Wilddog_Str_T*)wmalloc(len + 1);
    if(NULL == p_path)
    {
        return NULL;
    }

    if(srcLen == 1 && p_srcPath[0] == '/')
    {
        snprintf((char*)p_path, len , "/%s", (char*)childName);
    }
    else
    {
        snprintf((char*)p_path, len , "%s/%s", (char*)p_srcPath, \
            (char*)childName);
    }
    dstPathLen= strlen((const char *)p_path);
    p_path[dstPathLen]= 0;

    return p_path;
}

/*
 * Function:    _wilddog_url_getPath
 * Description: Get the path from the source path.
 * Input:       p_src_path: The source path.
 *              type: type of the function.
 *              str: input str, now used for child's name.
 *              pp_dstPath: The return 
 * Output:      pp_dstPath: pointer to the new path.
 * Return:      If success return 0, else return negative number.
*/
Wilddog_Return_T WD_SYSTEM _wilddog_url_getPath
    (
    Wilddog_Str_T* p_srcPath, 
    Wilddog_RefChange_T type,
    Wilddog_Str_T* str, 
    Wilddog_Str_T** pp_dstPath
    )
{
    wilddog_assert(p_srcPath, WILDDOG_ERR_NULL);
    wilddog_assert(pp_dstPath, WILDDOG_ERR_NULL);

    *pp_dstPath = NULL;

    switch(type)
    {
        case WILDDOG_REFCHG_PARENT:
            *pp_dstPath = _wilddog_url_getParentStr(p_srcPath);
            break;
        case WILDDOG_REFCHG_ROOT:
            *pp_dstPath = _wilddog_url_getRootStr(p_srcPath);
            break;
        case WILDDOG_REFCHG_CHILD:
            *pp_dstPath = _wilddog_url_getChildStr(p_srcPath, str);
            break;
        default:
            return 0;
    }
    if(*pp_dstPath)
        return WILDDOG_ERR_NOERR;
    return WILDDOG_ERR_NULL;
}

