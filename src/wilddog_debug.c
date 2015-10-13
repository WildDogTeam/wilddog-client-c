/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_debug.c
 *
 * Description: Debug functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        lixiongsheng    2015-06-01  Create file.
 * 0.4.3        Jimmy.Pan       2015-07-04  Add annotation, snprintf-->sprintf,
 *                                          change debug functions.
 *
 */

#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdio.h>  
#include <stdint.h>
#endif
#include <stdarg.h>
#include <string.h>
#include <stdlib.h> 

#include "wilddog.h"
#include "wilddog_url_parser.h"
#include "wilddog_ct.h"
#include "wilddog_debug.h"
#include "wilddog_common.h"

/*
 * Function:    wilddog_debug_errcodeCheck
 * Description: Print error code 's mean.
 * Input:       err: The error code.
 * Output:      N/A
 * Return:      N/A
*/
int WD_SYSTEM wilddog_debug_errcodeCheck(int err){
    switch(err)
    {
        case WILDDOG_ERR_NULL:
            printf("\t!!ERROR \t WILDDOG_ERR_NULL\n");
            break;
        case WILDDOG_ERR_INVALID:
            printf("\t!!ERROR \t WILDDOG_ERR_INVALID\n");
            break;
        case WILDDOG_ERR_SENDERR:
            printf("\t!!ERROR \t WILDDOG_ERR_SENDERR\n");
            break;
        case WILDDOG_ERR_OBSERVEERR:
            printf("\t!!ERROR \t WILDDOG_ERR_OBSERVEERR\n");
            break;  
        case WILDDOG_ERR_SOCKETERR:
            printf("\t!!ERROR \t WILDDOG_ERR_SOCKETERR\n");
            break;
        case WILDDOG_ERR_NOTAUTH:
            printf("\t!!ERROR \t WILDDOG_ERR_NOTAUTH\n");
            break;
        case WILDDOG_ERR_QUEUEFULL:
            printf("\t!!ERROR \t WILDDOG_ERR_QUEUEFULL\n");
        case WILDDOG_ERR_MAXRETRAN:
            printf("\t!!ERROR \t WILDDOG_ERR_MAXRETRAN\n");
            break;
        default:
            break;
    }
    return err;
}

/*
 * Function:    wilddog_debug_printUrl
 * Description: Print the client 's url.
 * Input:       wilddog: The client id.
 * Output:      N/A
 * Return:      N/A
*/
void WD_SYSTEM wilddog_debug_printUrl(Wilddog_T wilddog)
{
    Wilddog_Url_T * url = ((Wilddog_Ref_T*)wilddog)->p_ref_url;
    
    if(!wilddog || !url)
    {
        printf("wilddog client is invalid!\n");
        return;
    }
    printf("p_url_host = %s\n", url->p_url_host);
    printf("p_url_path = %s\n", url->p_url_path);
    printf("p_url_query = %s\n", url->p_url_query);
    return;
}

/*
 * Function:    wilddog_debug_printnode
 * Description: Print the node in json format.
 * Input:       node: The head of node.
 * Output:      N/A
 * Return:      N/A
*/
void WD_SYSTEM wilddog_debug_printnode(const Wilddog_Node_T* node)
{
    int i = 0;
    if(NULL == node)
        return;
    if(node->d_wn_type == WILDDOG_NODE_TYPE_OBJECT)
    {
        printf("\"%s\":{", node->p_wn_key);
        wilddog_debug_printnode(node->p_wn_child);
        printf("}");
        if(NULL != node->p_wn_next)
        {
            printf(", ");
            wilddog_debug_printnode(node->p_wn_next);
        }
    }
    else
    {
        if(node->d_wn_type == WILDDOG_NODE_TYPE_FALSE)
        {
            printf("\"%s\":false", node->p_wn_key);
        }
        else if(node->d_wn_type == WILDDOG_NODE_TYPE_TRUE)
        {
            printf("\"%s\":true", node->p_wn_key);
        }
        else if(node->d_wn_type == WILDDOG_NODE_TYPE_NULL)
        {
            printf("\"%s\":null", node->p_wn_key);
        }
        else if(node->d_wn_type == WILDDOG_NODE_TYPE_NUM)
        {
            printf("\"%s\":%d", node->p_wn_key, *(int*)(node->p_wn_value));
        }
        else if(node->d_wn_type == WILDDOG_NODE_TYPE_FLOAT)
        {
            printf("\"%s\":%lf", node->p_wn_key, *(wFloat*)(node->p_wn_value));
        }

        else if(node->d_wn_type == WILDDOG_NODE_TYPE_BYTESTRING)
        {
            printf("\"%s\":\"", node->p_wn_key);
            for( i = 0; i < node->d_wn_len; i++)
                printf("%x ", node->p_wn_value[i]);
            printf("\"");
        }
        else if(node->d_wn_type == WILDDOG_NODE_TYPE_UTF8STRING)
        {
            printf("\"%s\":\"%s\"", node->p_wn_key, node->p_wn_value);
        }
        if(node->p_wn_next)
        {
            printf(", ");
            wilddog_debug_printnode(node->p_wn_next);
        }
    }
    fflush(stdout);
}

/*
 * Function:    wilddog_debug_n2jsonStringInner
 * Description: Inner function: Change node to json string.
 * Input:       p_head: The head of node.
 * Output:      N/A
 * Return:      Pointer to the json string, must free by caller.
*/
STATIC Wilddog_Str_T * WD_SYSTEM wilddog_debug_n2jsonStringInner
    (
    Wilddog_Node_T * node
    )
{   
    int len = 0;
    Wilddog_Str_T *p_str = NULL, *p_brother = NULL;
    Wilddog_Str_T *p_childStr = NULL;
    if(node->d_wn_type == WILDDOG_NODE_TYPE_OBJECT)
    {
        if(node->p_wn_key)
        {
            len = 7 + strlen((const char *)node->p_wn_key);
            p_str = (Wilddog_Str_T *)wmalloc(len);
            if(NULL == p_str)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                return NULL;
            }
            sprintf((char*)p_str, "\"%s\":", node->p_wn_key);
            len = strlen((const char *)p_str);
        }
        if(NULL != node->p_wn_child)
        {
            p_childStr = wilddog_debug_n2jsonStringInner(node->p_wn_child);
            p_str = (Wilddog_Str_T *)wrealloc(p_str, \
                            p_str == NULL?(0):(strlen((const char *)p_str)), \
                            len + 4 + strlen((const char *)p_childStr) + 1);

            if(NULL == p_str)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                return NULL;
            }
            sprintf((char*)(p_str + len), "{%s}",p_childStr);
            if(p_childStr)
                wfree(p_childStr);
            len = strlen((const char *)p_str);
        }
        if(NULL != node->p_wn_next)
        {
            p_brother = wilddog_debug_n2jsonStringInner(node->p_wn_next);
            p_str = (Wilddog_Str_T *)wrealloc(p_str, \
                            p_str == NULL?(0):(strlen((const char *)p_str)),\
                            len + 4 + strlen((const char *)p_brother) + 1);
            
            if(NULL == p_str)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                return NULL;
            }
            sprintf((char*)(p_str + len), ",%s",p_brother);
            if(p_brother)
                wfree(p_brother);
        }

        return p_str;
    }
    else
    {
        len = 0;
        p_str = NULL;
        if(node->p_wn_key)
        {
            p_str = (Wilddog_Str_T *)wmalloc( \
                                    strlen((const char *)node->p_wn_key) + 5);
            if(NULL == p_str)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                return NULL;
            }
            sprintf((char*)p_str, "\"%s\":", node->p_wn_key);
            len = strlen((const char *)p_str);
        }
        if(node->d_wn_type == WILDDOG_NODE_TYPE_FALSE)
        {
            p_str = (Wilddog_Str_T *)wrealloc(p_str, \
                            p_str == NULL?(0):(strlen((const char *)p_str)),\
                            len + 6);
            
            if(NULL == p_str)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                return NULL;
            }
            sprintf((char*)(p_str + len), "false");
        }
        else if(node->d_wn_type == WILDDOG_NODE_TYPE_TRUE)
        {
            p_str = (Wilddog_Str_T *)wrealloc(p_str, \
                            p_str == NULL?(0):(strlen((const char *)p_str)), \
                            len + 5);
            
            if(NULL == p_str)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                return NULL;
            }
            sprintf((char*)(p_str + len), "true");

        }
        else if(node->d_wn_type == WILDDOG_NODE_TYPE_NULL)
        {
            p_str = (Wilddog_Str_T *)wrealloc(p_str, \
                            p_str == NULL?(0):(strlen((const char *)p_str)),\
                            len + 5);
            
            if(NULL == p_str)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                return NULL;
            }
            sprintf((char*)(p_str + len), "null");

        }
        else if(node->d_wn_type == WILDDOG_NODE_TYPE_NUM)
        {
            Wilddog_Str_T tmp[12];
            memset(tmp, 0, 12);
            sprintf((char*)tmp, "%d", *(int*)(node->p_wn_value));
            p_str = (Wilddog_Str_T *)wrealloc(p_str, \
                            p_str == NULL?(0):(strlen((const char *)p_str)),\
                            len + 2 + strlen((const char *)tmp));
            
            if(NULL == p_str)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                return NULL;
            }
            sprintf((char*)(p_str + len), "%s", (char*)tmp);
        }
        else if(node->d_wn_type == WILDDOG_NODE_TYPE_FLOAT)
        {
            Wilddog_Str_T tmp[40];
            memset(tmp, 0, 40);
            sprintf((char*)tmp, "%lf", *(wFloat*)(node->p_wn_value));
            p_str = (Wilddog_Str_T *)wrealloc(p_str, \
                            p_str == NULL?(0):(strlen((const char *)p_str)),\
                            len + 2 + strlen((const char *)tmp));
            
            if(NULL == p_str)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                return NULL;
            }
            sprintf((char*)(p_str + len), "%s", (char*)tmp);
        }

        else if(
            node->d_wn_type == WILDDOG_NODE_TYPE_BYTESTRING || \
            node->d_wn_type == WILDDOG_NODE_TYPE_UTF8STRING
            )
        {
            if(!node->p_wn_value)
            {
                wfree(p_str);
                return NULL;
            }
            p_str = (Wilddog_Str_T *)wrealloc(p_str, \
                            p_str == NULL?(0):(strlen((const char *)p_str)),\
                            len + 4 + strlen((const char *)node->p_wn_value));
            
            if(NULL == p_str)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                return NULL;
            }
            sprintf((char*)(p_str + len), "\"%s\"", (char*)node->p_wn_value);
        }
        if(node->p_wn_next)
        {
            Wilddog_Str_T * p_tmp = NULL;
            p_brother = wilddog_debug_n2jsonStringInner(node->p_wn_next);
            if(NULL == p_brother)
            {
                wfree(p_str);
                return NULL;
            }
            p_tmp = (Wilddog_Str_T *)wmalloc(strlen((const char *)p_str) + \
                                        strlen((const char *)p_brother) + 4);
            if(NULL == p_tmp)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
                wfree(p_str);
                return NULL;
            }
            sprintf((char*)p_tmp, "%s,%s", (char*)p_str, (char*)p_brother);
            if(p_brother)
                wfree(p_brother);
            if(p_str)
                free(p_str);
            p_str = p_tmp;
        }
    }
    return p_str;
}

/*
 * Function:    wilddog_debug_n2jsonString
 * Description: Change node to json string.
 * Input:       p_head: The head of node.
 * Output:      N/A
 * Return:      Pointer to the json string, must free by caller.
*/
Wilddog_Str_T  * WD_SYSTEM wilddog_debug_n2jsonString
    (
    Wilddog_Node_T* p_head
    )
{
    Wilddog_Str_T *p_str = NULL;
    Wilddog_Str_T *p_childStr = NULL;
    
    wilddog_assert(p_head, NULL);
    if(NULL == p_head->p_wn_child)
    {
        p_str = wilddog_debug_n2jsonStringInner(p_head);
        return p_str;
    }
    p_childStr = wilddog_debug_n2jsonStringInner(p_head->p_wn_child);
    if(NULL == p_childStr)
        return NULL;
    p_str = (Wilddog_Str_T *)wmalloc(strlen((const char *)p_childStr) + 4);
    if(NULL == p_str)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "malloc failed!");
        return NULL;
    }

    sprintf((char*)p_str, "{%s}", (char*)p_childStr);
    wfree(p_childStr);

    return p_str;
}

