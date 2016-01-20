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





extern Wilddog_Node_T *_wilddog_node_new();
static const char * WD_SYSTEM parse_value
    (
    Wilddog_Node_T *item,
    const char *value
    );
static const char * WD_SYSTEM skip(const char *in)
{
    while (in && *in && (unsigned char)*in<=32)
        in++; 
    return in;
}
static const unsigned char firstByteMark[7] = {
    0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

static unsigned WD_SYSTEM parse_hex4(const char *str)
{
    unsigned h=0;
    if (*str>='0' && *str<='9') 
        h+=(*str)-'0'; 
    else if (*str>='A' && *str<='F') 
        h+=10+(*str)-'A'; 
    else if (*str>='a' && *str<='f') 
        h+=10+(*str)-'a'; 
    else 
        return 0;
    
    h=h<<4;str++;
    
    if (*str>='0' && *str<='9') 
        h+=(*str)-'0'; 
    else if (*str>='A' && *str<='F') 
        h+=10+(*str)-'A'; 
    else if (*str>='a' && *str<='f') 
        h+=10+(*str)-'a'; 
    else 
        return 0;
    
    h=h<<4;str++;
    
    if (*str>='0' && *str<='9') 
        h+=(*str)-'0'; 
    else if (*str>='A' && *str<='F') 
        h+=10+(*str)-'A'; 
    else if (*str>='a' && *str<='f') 
        h+=10+(*str)-'a'; 
    else 
        return 0;
    
    h=h<<4;str++;
    
    if (*str>='0' && *str<='9') 
        h+=(*str)-'0'; 
    else if (*str>='A' && *str<='F') 
        h+=10+(*str)-'A'; 
    else if (*str>='a' && *str<='f') 
        h+=10+(*str)-'a'; 
    else 
        return 0;
    
    return h;
}

STATIC const char *WD_SYSTEM parse_string
    (
    Wilddog_Node_T *item,
    const char *str
    )
{
    const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc,uc2;
    
    if (*str!='\"') 
    {
        return 0;
    } /* not a string! */
    
    while (*ptr!='\"' && *ptr && ++len) 
        if (*ptr++ == '\\') 
            ptr++;/* Skip escaped quotes. */

    /* This is how long we need for the string, roughly. */
    out=(char*)wmalloc(len+1);  
    
    if (!out) 
        return 0;
    
    ptr=str+1;ptr2=out;
    while (*ptr!='\"' && *ptr)
    {
        if (*ptr!='\\') *ptr2++=*ptr++;
        else
        {
            ptr++;
            switch (*ptr)
            {
                case 'b': *ptr2++='\b'; break;
                case 'f': *ptr2++='\f'; break;
                case 'n': *ptr2++='\n'; break;
                case 'r': *ptr2++='\r'; break;
                case 't': *ptr2++='\t'; break;
                case 'u':    /* transcode utf16 to utf8. */
                    uc=parse_hex4(ptr+1);ptr+=4;    /* get the unicode char. */

                    if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)    
                        break;  /* check for invalid.   */

                    if (uc>=0xD800 && uc<=0xDBFF)   /* UTF16 surrogate pairs.*/
                    {
                        if (ptr[1]!='\\' || ptr[2]!='u')    
                            break;  /* missing second-half of surrogate.    */
                        uc2=parse_hex4(ptr+3);ptr+=6;
                        if (uc2<0xDC00 || uc2>0xDFFF)       
                            break;  /* invalid second-half of surrogate.    */
                        uc=0x10000 + (((uc&0x3FF)<<10) | (uc2&0x3FF));
                    }

                    len=4;

                    if (uc<0x80) 
                        len=1;
                    else if (uc<0x800) 
                        len=2;
                    else if (uc<0x10000) 
                        len=3; 

                    ptr2+=len;
                    
                    switch (len) 
                    {
                        case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                        case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                        case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
                        case 1: *--ptr2 =(uc | firstByteMark[len]);
                    }
                    ptr2+=len;
                    break;
                default:  
                    *ptr2++=*ptr; 
                    break;
            }
            ptr++;
        }
    }
    *ptr2=0;
    if (*ptr=='\"') ptr++;
    item->p_wn_value=(Wilddog_Str_T *)out;
    item->d_wn_type=WILDDOG_NODE_TYPE_UTF8STRING;
    item->d_wn_len = strlen(out);
    return ptr;
}

STATIC double WD_SYSTEM wd_pow(double x, double y)
{
    int i = 0;  
    double z = x;
    if(y>0)
    {
        for(i=0; i<y; i++)
            x*=z;
        return x;   
    }
    else if(y<0)
    {
        for(i=0; i<-y-1; i++)
            x*=z;
        return 1/x; 
    }
    else
        return 1;
}

STATIC const char * WD_SYSTEM parse_number
    (
    Wilddog_Node_T *item,
    const char *num
    )
{
    double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;

    if (*num=='-') sign=-1,num++;   /* Has sign? */
    
    if (*num=='0') num++;           /* is zero */
    
    if (*num>='1' && *num<='9') do  n=(n*10.0)+(*num++ -'0');   
    while (*num>='0' && *num<='9'); /* Number? */
    
    if (*num=='.' && num[1]>='0' && num[1]<='9') 
    {
        num++;
        do
        {
            n=(n*10.0)+(*num++ -'0'),scale--; 
        }
        while (*num>='0' && *num<='9');
    }  /* Fractional part? */
    if (*num=='e' || *num=='E')     /* Exponent? */
    {   
        num++;
        if (*num=='+') 
            num++;  
        else if (*num=='-') 
            signsubscale=-1,num++;      /* With sign? */
        
        while (*num>='0' && *num<='9') 
            subscale=(subscale*10)+(*num++ - '0');  /* Number? */
    }
    /* number = +/- number.fraction * 10^+/- exponent */
    n=sign*n*wd_pow(10.0,(scale+subscale*signsubscale));
    
    if((n-(int)n/1) == 0)
    {
        item->d_wn_len = sizeof(s32);
        item->p_wn_value = wmalloc(item->d_wn_len);

        *(s32*)item->p_wn_value = (s32)(n);
        
        item->d_wn_type=WILDDOG_NODE_TYPE_NUM;
    }
    else
    {
        item->d_wn_len = sizeof(wFloat);
        item->p_wn_value = wmalloc(item->d_wn_len);

        *(wFloat*)item->p_wn_value = (wFloat)(n);
        
        item->d_wn_type=WILDDOG_NODE_TYPE_FLOAT;
    }

    return num;
}



STATIC const char * WD_SYSTEM parse_array
    (
    Wilddog_Node_T *item,
    const char *value
    )
{
    Wilddog_Node_T *child;
    if (*value!='[')    {return 0;} /* not an array! */

    item->d_wn_type=WILDDOG_NODE_TYPE_OBJECT;
    value=skip(value+1);
    if (*value==']') return value+1;    /* empty array. */

    item->p_wn_child=child=_wilddog_node_new();
    if (!item->p_wn_child) 
        return 0;        /* memory fail */

    /* skip any spacing, get the value. */
    value=skip(parse_value(child,skip(value))); 
    if (!value) 
        return 0;

    while (*value==',')
    {
        Wilddog_Node_T *new_item;
        if (!(new_item=_wilddog_node_new())) return 0;  /* memory fail */
        child->p_wn_next=new_item;new_item->p_wn_prev=child;child=new_item;
        value=skip(parse_value(child,skip(value+1)));
        if (!value) return 0;   /* memory fail */
    }

    if (*value==']') return value+1;    /* end of array */
    return 0;   /* malformed. */
}


STATIC const char * WD_SYSTEM parse_object
    (
    Wilddog_Node_T *item,
    const char *value
    )
{
    Wilddog_Node_T *child;

    if (*value!='{')    {return 0;} /* not an object! */
    
    item->d_wn_type=WILDDOG_NODE_TYPE_OBJECT;
    item->d_wn_len = 0;
    value=skip(value+1);
    if (*value=='}') return value+1;    /* empty array. */
    
    item->p_wn_child=child=_wilddog_node_new();
    child->p_wn_parent = item;
    if (!item->p_wn_child) return 0;
    value=skip(parse_string(child,skip(value)));
    if (!value) 
        return 0;
    child->p_wn_key=child->p_wn_value;
    child->p_wn_value=0;
    if (*value!=':')
    {
        return 0;
    } /* fail! */

    
    /* skip any spacing, get the value. */
    value=skip(parse_value(child,skip(value+1)));   
    
    if (!value) return 0;
    
    while (*value==',')
    {
        Wilddog_Node_T *new_item;
        if (!(new_item=_wilddog_node_new()))    
            return 0; /* memory fail */
        child->p_wn_next=new_item;
        new_item->p_wn_prev=child;
        child=new_item;
        new_item->p_wn_parent = item;
        value=skip(parse_string(child,skip(value+1)));
        if (!value) 
            return 0;
        child->p_wn_key=child->p_wn_value;
        child->p_wn_value=0;
        if (*value!=':') 
        {
            return 0;
        } /* fail! */
        
        /* skip any spacing, get the value. */
        value=skip(parse_value(child,skip(value+1)));   
        if (!value) return 0;
    }
    
    if (*value=='}') return value+1;    /* end of array */
    return 0;   /* malformed. */
}

STATIC const char * WD_SYSTEM parse_value
    (
    Wilddog_Node_T *item,
    const char *value
    )
{
    if (!value) 
    {
        return 0;   /* Fail on null. */
    }
    if (!strncmp(value,"null",4))   
    { 
        item->d_wn_type=WILDDOG_NODE_TYPE_NULL;  
        item->d_wn_len=0;
        return value+4; 
    }
    if (!strncmp(value,"false",5))  
    { 
        item->d_wn_type=WILDDOG_NODE_TYPE_FALSE; 
        item->d_wn_len=0;
        return value+5; 
    }
    if (!strncmp(value,"true",4))   
    { 
        item->d_wn_type=WILDDOG_NODE_TYPE_TRUE;  
        item->d_wn_len=0;
        return value+4; 
    }
    if (*value=='\"')               
    { 
        return parse_string(item,value); 
    }
    if (*value=='-' || (*value>='0' && *value<='9'))    
    { 
        return parse_number(item,value); 
    }
    if (*value=='[')                
    { 
        return parse_array(item,value); 
    }
    if (*value=='{')                
    { 
        return parse_object(item,value); 
    }
    
    return 0;   /* failure. */
}

Wilddog_Node_T * WD_SYSTEM wilddog_jsonStr2node(const char *value) 
{
    const char *end=0;
    Wilddog_Node_T *c=wilddog_node_createObject((Wilddog_Str_T*)"/");
    if (!c) return 0;       /* memory fail */

    end=parse_value(c,skip(value));
    if (!end)   
    {
        wilddog_node_delete(c);
        return 0;
    }/* parse failure. ep is set. */
    
    return c;
}

