/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_cbor.c
 *
 * Description: CBOR functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Jimmy.Pan       2015-05-15  Create file.
 * 0.4.3        Baikal.Hu       2015-07-09  Fix BUG: Input more than 256 strings  
 *                                          will cause length parse error.
 * 0.4.6        Jimmy.Pan       2015-09-06  Fix BUG: If float is -1, parse float
 *                                          will cause error.
 */

#ifndef WILDDOG_PORT_TYPE_ESP
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include "wilddog.h"
#include "wilddog_config.h"
#include "wilddog_url_parser.h"
#include "wilddog_cbor.h"
#include "wilddog_endian.h"
#include "wilddog_common.h"
#include "wilddog_api.h"

/* The root node key is "/" */
#define WILDDOG_ROOT_KEY "/"

typedef enum _NODE_STRING_TYPE
{
    TYPE_KEY,
    TYPE_VALUE
}Node_String_T;

typedef Wilddog_Str_T *(*_cborParserFunc)(Wilddog_Payload_T *p_data, int* len);

extern Wilddog_Node_T *_wilddog_node_new();

STATIC Wilddog_Str_T * _wilddog_c2n_parseSimpleItem
    (
    Wilddog_Payload_T* p_data, 
    int* len
    );
STATIC s8 _wilddog_c2n_parseSpecial
    (
    Wilddog_Payload_T * p_data, 
    wFloat * p_num
    );

/*
 * Function:    _wilddog_c2n_typeTranslate
 * Description: Translate the cbor data type to the node type
 * Input:       CBOR data type
 * Output:      N/A
 * Return:      Node type
*/
STATIC u8 WD_SYSTEM _wilddog_c2n_typeTranslate(u8 type)
{
    if(WILDDOG_CBOR_UINT == type || \
        WILDDOG_CBOR_NEGINT == type
        )
        return WILDDOG_NODE_TYPE_NUM;
    else if(WILDDOG_CBOR_BYTE_STRING == type)
        return WILDDOG_NODE_TYPE_BYTESTRING;
    else if(WILDDOG_CBOR_TEXT_STRING == type)
        return WILDDOG_NODE_TYPE_UTF8STRING;
    else if(WILDDOG_CBOR_MAP == type)
        return WILDDOG_NODE_TYPE_OBJECT;
    return -1;
}

/*
 * Function:    _wilddog_c2n_getHeadType
 * Description: Get the CBOR data type
 * Input:       The payload
 * Output:      N/A
 * Return:      CBOR data type
*/
STATIC u8 WD_SYSTEM _wilddog_c2n_getHeadType(Wilddog_Payload_T* p_data)
{
    u8 head = *(u8*)(p_data->p_dt_data + p_data->d_dt_pos);
    return WILDDOG_CBOR_TYPE(head);
}

/*
 * Function:    _wilddog_c2n_getItemLen
 * Description: Get the length of CBOR item
 * Input:       data: The payload
 * Output:      p_num: The length
 * Return:      The length of the skip position
*/
STATIC s32 WD_SYSTEM _wilddog_c2n_getItemLen
    (
    Wilddog_Payload_T* data,
    s32 *p_num
    )
{
    s32 num = 0;
    u8 * p_data = (u8 *)(data->p_dt_data + data->d_dt_pos);
    
    num = (s32)WILDDOG_CBOR_INFO(*p_data);
    if(num < WILDDOG_CBOR_FOLLOW_1BYTE)
    {
        *p_num = num;
        return WILDDOG_CBOR_HEAD_LEN;
    }
    else if(WILDDOG_CBOR_FOLLOW_1BYTE == num)
    {
        *p_num = *(u8*)(p_data + 1);

        return WILDDOG_CBOR_FOLLOW_1BYTE_LEN + WILDDOG_CBOR_HEAD_LEN;
    }
    else if(WILDDOG_CBOR_FOLLOW_2BYTE == num)
    {
        /*2bytes after head are the value, and it is in network order*/
        *p_num = ((*(u8*)(p_data + 1)) << 8) | (*(u8*)(p_data + 2));
        return WILDDOG_CBOR_FOLLOW_2BYTE_LEN + WILDDOG_CBOR_HEAD_LEN;
    }
    else if(WILDDOG_CBOR_FOLLOW_4BYTE == num)
    {
        /*4bytes after head are the value, and it is in network order*/
        *p_num = ((*(u8*)(p_data + 1)) << 24)| ((*(u8*)(p_data + 2)) << 16)| \
                 ((*(u8*)(p_data + 3)) <<  8)| ((*(u8*)(p_data + 4)));
        return WILDDOG_CBOR_FOLLOW_4BYTE_LEN + WILDDOG_CBOR_HEAD_LEN;
    }
    else if(WILDDOG_CBOR_FOLLOW_VAR == num)
    {
        /*do not let it happen*/
        *p_num = -1;
        return WILDDOG_CBOR_HEAD_LEN;
    }
    else
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "cannot read header!\n");
        return 0;
    }
    return 0;
}


/*
 * Function:    _wilddog_c2n_numHandler
 * Description: Parse the number or length to real one
 * Input:       num: The original number
 *              dataLen: bytes the number owned
 *              type: the output number is nagtive or positive
 * Output:      p_num: pointer to the real number
 * Return:      N/A
*/
STATIC void WD_SYSTEM _wilddog_c2n_numHandler
    (
    s32 num, 
    u8 dataLen, 
    u8 type, 
    Wilddog_Str_T *p_num
    )
{
    if(dataLen == WILDDOG_CBOR_HEAD_LEN         || \
       dataLen == WILDDOG_CBOR_FOLLOW_1BYTE_LEN || \
       dataLen == WILDDOG_CBOR_FOLLOW_2BYTE_LEN || \
       dataLen == WILDDOG_CBOR_FOLLOW_4BYTE_LEN)
    {
        if (WILDDOG_CBOR_NEGINT == type)
            num = -1 - num;
    }
    else
        num = 0;

    memcpy(p_num, (u8*)&num, sizeof(s32));
    return ;
}

/*
 * Function:    _wilddog_c2n_parseInt
 * Description: Parse the CBOR integer to Node's value
 * Input:       p_data: The payload; 
 *              len: The length; 
 *              type: WILDDOG_CBOR_UINT or WILDDOG_CBOR_NEGINT;
 * Output:      N/A
 * Return:      The node's value string
*/
STATIC Wilddog_Str_T * WD_SYSTEM _wilddog_c2n_parseInt
    (
    Wilddog_Payload_T* p_data, 
    int* len,
    u8 type
    )
{
    /*size means how many bytes the data have*/
    int pos = 0, size = 0;
    s32 num  = 0;
    Wilddog_Str_T* p_str = NULL;

    pos = _wilddog_c2n_getItemLen(p_data, &num);
    if(0 == pos)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "parse head fail!");
        return NULL;
    }
    p_data->d_dt_pos += pos;

    /*
     * if pos == 1, size = 1, means data has 1 byte, and less than 24
     * if pos == 2, size = 1, means data has 1 byte, and bigger or equal to 24
     * if pos == 3, size = 2, means data has 2 bytes
     * if pos == 5, size = 4, means data has 4 bytes.
    */
    size = (pos > 1) ? (pos-1) : pos;
    if(size > sizeof(s32))
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
                "num need %d long, we only use %d!", size, (int)sizeof(s32));
        return NULL;
    }
    p_str = (Wilddog_Str_T *)wmalloc(sizeof(s32));
    if(NULL == p_str)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "cannot malloc node!");
        return NULL;
    }
    _wilddog_c2n_numHandler(num, size, type, p_str);
    *len = sizeof(s32);
    return p_str;   
}

/*
 * Function:    _wilddog_c2n_parseUint
 * Description: Parse the CBOR positive integer to Node's value
 * Input:       p_data: The payload; 
 *              len: The length; 
 * Output:      N/A
 * Return:      The node's value string
*/
STATIC Wilddog_Str_T * WD_SYSTEM _wilddog_c2n_parseUint
    (
    Wilddog_Payload_T* p_data, 
    int* len
    )
{
    return _wilddog_c2n_parseInt(p_data, len, WILDDOG_CBOR_UINT);
}

/*
 * Function:    _wilddog_c2n_parseNegint
 * Description: Parse the CBOR negative integer to Node's value
 * Input:       p_data: The payload; 
 *              len: The length; 
 * Output:      N/A
 * Return:      The node's value string
*/
STATIC Wilddog_Str_T * WD_SYSTEM _wilddog_c2n_parseNegint
    (
    Wilddog_Payload_T* p_data, 
    int* len
    )
{
    return _wilddog_c2n_parseInt(p_data, len, WILDDOG_CBOR_NEGINT);
}

/*
 * Function:    _wilddog_c2n_parseStr
 * Description: Parse the CBOR string to Node's value
 * Input:       p_data: The payload; 
 *              len: The length; 
 * Output:      N/A
 * Return:      The node's value string
*/
STATIC Wilddog_Str_T * WD_SYSTEM _wilddog_c2n_parseStr
    (
    Wilddog_Payload_T* p_data, 
    int* p_len
    )
{
    int pos = 0, var = FALSE;
    s32 num  = 0;
    Wilddog_Str_T* p_str = NULL;

    pos = _wilddog_c2n_getItemLen(p_data, &num);
    if(0 == pos)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "parse head fail!");
        return NULL;
    }
    if(num == WILDDOG_CBOR_FOLLOW_UNKNOW_LEN)
    {
        var = TRUE;
        num = WILDDOG_CBOR_FOLLOW_UNKNOW_DEFLEN;
    }

    p_data->d_dt_pos += pos;
    //todo test
    if(TRUE == var)
    {
        /*can not go here!*/
        int i, len ,count = 0, totalLen = 0;
        u8 strType = 0;
        Wilddog_Str_T *strP[WILDDOG_CBOR_FOLLOW_UNKNOW_DEFLEN];
        u8 strL[WILDDOG_CBOR_FOLLOW_UNKNOW_DEFLEN];
        Wilddog_Str_T * p_tmp= NULL;
        memset(strP, 0, WILDDOG_CBOR_FOLLOW_UNKNOW_DEFLEN);
        memset(strL, 0, WILDDOG_CBOR_FOLLOW_UNKNOW_DEFLEN);
        strType = (*(u8*)(p_data->p_dt_data + p_data->d_dt_pos)) & 0xff;
        /*
         * indefinite string, means several definite strings, totally end by a 
         * "break"
         */
        while(WILDDOG_CBOR_BREAK != strType)
        {
            strP[count] = _wilddog_c2n_parseStr(p_data, &len);
            strL[count] = len;
            count++;
            strType = *(u8*)(p_data->p_dt_data + p_data->d_dt_pos);
        }
        p_data->d_dt_pos++;
        
        /* combine strings*/
        for(i = 0; i < count; i++)
        {
            totalLen += strL[i];
        }

        p_str = (Wilddog_Str_T *)wmalloc(totalLen + 1);
        if(NULL == p_str)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "cannot malloc node!");
            return NULL;
        }
        p_tmp = p_str;
        for(i = 0; i < count; i++)
        {
            if(strP[i] == NULL)
                break;
            memcpy(p_tmp, strP[i], strL[i]);
            p_tmp += strL[i];
            wfree(strP[i]);
            strP[i] = NULL;
        }
        p_str[totalLen] = 0;
        *p_len = totalLen;
        return p_str;
    }
    p_str = (Wilddog_Str_T *)wmalloc(num + 1);
    if(NULL == p_str)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "cannot malloc node!");
        return NULL;
    }
    memcpy(p_str, (u8*)(p_data->p_dt_data + p_data->d_dt_pos), num);
    p_str[num] = 0;
    *p_len = num;
    p_data->d_dt_pos += num;

    return p_str;
}

/*
 * Function:    _wilddog_c2n_parseByteStr
 * Description: Parse the CBOR Byte string to Node's value
 * Input:       p_data: The payload;
 *              len: The length; 
 * Output:      N/A
 * Return:      The node's value string
*/
STATIC Wilddog_Str_T * WD_SYSTEM _wilddog_c2n_parseByteStr
    (
    Wilddog_Payload_T* p_data, 
    int* len
    )
{
    return _wilddog_c2n_parseStr(p_data, len);
}

/*
 * Function:    _wilddog_c2n_parseTextStr
 * Description: Parse the CBOR Text string to Node's value
 * Input:       p_data: The payload; 
 *              len: The length; 
 * Output:      N/A
 * Return:      The node's value string
*/
STATIC Wilddog_Str_T * WD_SYSTEM _wilddog_c2n_parseTextStr
    (
    Wilddog_Payload_T* p_data, 
    int* len
    )
{
    return _wilddog_c2n_parseStr(p_data, len);
}

/*
 * Function:    _wilddog_c2n_parseMap
 * Description: Parse the CBOR Map to Node
 * Input:       p_data: The payload;
 * Output:      N/A
 * Return:      The node
*/
STATIC Wilddog_Node_T * WD_SYSTEM _wilddog_c2n_parseMap
    (
    Wilddog_Payload_T* p_data
    )
{
    int i, len, pos = 0, var = FALSE;
    s32 currNum = 0;
    s16 type = WILDDOG_CBOR_UINT;
    Wilddog_Node_T * p_newNode = NULL, *p_head = NULL;
    Wilddog_Str_T *p_key = NULL, *p_value = NULL;

    /*parse header*/
    pos = _wilddog_c2n_getItemLen(p_data, &currNum);
    if(0 == pos)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "parse head fail!");
        return NULL;
    }
    
    p_data->d_dt_pos += pos;
    /* create head node*/
    p_head = wilddog_node_createObject((Wilddog_Str_T*)WILDDOG_ROOT_KEY);
    if(NULL == p_head)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "cannot malloc head!");
        return NULL;
    }
    /*parse key-value */
    if(currNum == WILDDOG_CBOR_FOLLOW_UNKNOW_LEN)
    {
        /* waiting break*/
        wilddog_debug_level(WD_DEBUG_WARN, "do not know the length");
        var = TRUE;
        currNum = WILDDOG_CBOR_FOLLOW_UNKNOW_DEFLEN;
    }
    for(i = 0; i < currNum; i++)
    {
        if(p_data->d_dt_len < p_data->d_dt_pos)
        {
            /* delete all alloced nodes in this function.*/
            wilddog_node_delete(p_head);
            return NULL;
        }   
        if(TRUE == var)
        {
            /* use break to end cycle*/
            if(WILDDOG_CBOR_BREAK==*(u8*)(p_data->p_dt_data + p_data->d_dt_pos))
            {
                p_data->d_dt_pos++;
                break;
            }
        }
        /*1. key, only can be simple item*/

        p_key = _wilddog_c2n_parseSimpleItem(p_data, &len);
        if(NULL == p_key)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "parse fail!");
            wilddog_node_delete(p_head);
            return NULL;
        }
        len = 0;
        /*2. value*/
        
        type = _wilddog_c2n_getHeadType(p_data) & 0xff;
        if( WILDDOG_CBOR_MAP == type )
        {
            p_newNode = _wilddog_c2n_parseMap(p_data);
            if(NULL == p_newNode)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "parse map failed!");
                wilddog_node_delete(p_head);
                wfree(p_key);
                return NULL;
            }
            wfree(p_newNode->p_wn_key);
            p_newNode->p_wn_key = p_key;
        }
        else if(WILDDOG_CBOR_ARRAY == type)
        {
            /* can not be array!*/
            wilddog_debug_level(WD_DEBUG_ERROR, "parse array err!");
            wilddog_node_delete(p_head);
            wfree(p_key);
            return NULL;
        }
        else
        {
            if(WILDDOG_CBOR_SPECIAL == type)
            {
                wFloat num;
            
                type = _wilddog_c2n_parseSpecial(p_data, &num);
                if(0 > type)
                {
                    wilddog_debug_level(WD_DEBUG_ERROR, "parse special err!");
                    wilddog_node_delete(p_head);
                    wfree(p_key);
                    return NULL;
                }
                if(WILDDOG_NODE_TYPE_FLOAT == type )
                {
                    p_value = (Wilddog_Str_T*)wmalloc(sizeof(wFloat));
                    if(NULL == p_value)
                    {
                        wilddog_debug_level(WD_DEBUG_ERROR, "malloc err!");
                        wilddog_node_delete(p_head);
                        wfree(p_key);
                        return NULL;
                    }
                    memcpy(p_value, &num, sizeof(wFloat));
                    len = sizeof(wFloat);
                }
                else
                {
                    p_value = NULL;
                    len = 0;
                }
            }
            else
            {
                p_value = _wilddog_c2n_parseSimpleItem(p_data, &len);
                type = _wilddog_c2n_typeTranslate(type);
                if(type < 0)
                {
                    wilddog_node_delete(p_head);
                    wfree(p_key);
                    wfree(p_value);
                    return NULL;
                }
            }
            /*add to head*/
            p_newNode = _wilddog_node_new();
            if(NULL == p_newNode)
            {
                wilddog_node_delete(p_head);
                wfree(p_key);
                wfree(p_value);
                return NULL;
            }
            p_newNode->d_wn_type = type;
            p_newNode->p_wn_key = p_key;
            p_newNode->p_wn_value = p_value;
            p_newNode->d_wn_len = len;
        }
        /*add to head*/
        wilddog_node_addChild(p_head, p_newNode);
    }
    return p_head;
}

/*
 * Function:    _wilddog_swap32
 * Description: swap 32 bit data
 * Input:       src
 * Output:      dst
 * Return:      N/A
*/
STATIC void WD_SYSTEM _wilddog_swap32(u8* src, u8* dst)
{
#if WILDDOG_LITTLE_ENDIAN == 1
    dst[0] = src[3];
    dst[1] = src[2];
    dst[2] = src[1];
    dst[3] = src[0];
#else
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
#endif
}

/*
 * Function:    _wilddog_swap64
 * Description: swap 64 bit data
 * Input:       src
 * Output:      dst
 * Return:      N/A
*/
STATIC void WD_SYSTEM _wilddog_swap64(u8* src, u8* dst)
{
#if WILDDOG_LITTLE_ENDIAN == 1
    dst[0] = src[7];
    dst[1] = src[6];
    dst[2] = src[5];
    dst[3] = src[4];
    dst[4] = src[3];
    dst[5] = src[2];
    dst[6] = src[1];
    dst[7] = src[0];
#else
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
    dst[5] = src[5];
    dst[6] = src[6];
    dst[7] = src[7];
#endif
}

/*
 * Function:    _wilddog_parseFloat
 * Description: Parse float
 * Input:       p_data: The payload
 * Output:      data: pointer to wFloat data
 * Return:      Return TRUE or FALSE 
*/
STATIC BOOL WD_SYSTEM _wilddog_parseFloat
    (
    Wilddog_Payload_T * p_data, 
    wFloat* data
    )
{
    wFloat num = 0;
    u8 head = *(u8*)(p_data->p_dt_data + p_data->d_dt_pos);
    
    if(WILDDOG_CBOR_FLOAT16 == head)
    {
        /*can not be half!*/
    }
    else if(WILDDOG_CBOR_FLOAT32 == head)
    {
        float tmp;
        _wilddog_swap32((u8*)(p_data->p_dt_data+p_data->d_dt_pos+ 1),(u8*)&tmp);
        num = tmp;
        p_data->d_dt_pos += 5;/* 1 byte head + 4 bytes data*/
    }
    else if(WILDDOG_CBOR_FLOAT64 == head)
    {
#if WILDDOG_MACHINE_BITS != 8
        /* only used in 32 bit machine*/
        _wilddog_swap64((u8*)(p_data->p_dt_data+p_data->d_dt_pos +1),(u8*)&num);
        p_data->d_dt_pos += 9;/* 1 byte head + 8bytes data*/
#else
        return FALSE;
#endif
    }
    *data = num;
    return TRUE;
}

/*
 * Function:    _wilddog_c2n_parseSpecial
 * Description: Parse CBOR special data type: FALSE/TRUE/NULL/FLOAT
 * Input:       p_data: The payload
 * Output:      p_num: Float data
 * Return:      Return Node type
*/
STATIC s8 WD_SYSTEM _wilddog_c2n_parseSpecial
    (
    Wilddog_Payload_T * p_data, 
    wFloat * p_num
    )
{
    u8 data = *(u8*)(p_data->p_dt_data + p_data->d_dt_pos);
    if(WILDDOG_CBOR_FALSE == data)
    {
        p_data->d_dt_pos++;
        return WILDDOG_NODE_TYPE_FALSE;
    }
    else if(WILDDOG_CBOR_TRUE == data)
    {
        p_data->d_dt_pos++;
        return WILDDOG_NODE_TYPE_TRUE;
    }
    else if(WILDDOG_CBOR_NULL == data)
    {
        p_data->d_dt_pos++;
        return WILDDOG_NODE_TYPE_NULL;
    }
    else if(
        WILDDOG_CBOR_FLOAT16 == data || \
        WILDDOG_CBOR_FLOAT32 == data || \
        WILDDOG_CBOR_FLOAT64 == data
        )
    {
        if(TRUE == _wilddog_parseFloat(p_data, p_num))
            return WILDDOG_NODE_TYPE_FLOAT;
        else
            return -1;
    }
    else
        return -1;
}

/*
 * Function:    _wilddog_cbor2Node
 * Description: Convert CBOR to Node
 * Input:       p_data: The payload
 * Output:      N/A
 * Return:      Return Node
*/
Wilddog_Node_T * WD_SYSTEM _wilddog_cbor2Node(Wilddog_Payload_T* p_data)
{
    int len;
    u8 type = _wilddog_c2n_getHeadType(p_data) & 0xff;

    if(WILDDOG_CBOR_MAP == type )
    {
        return _wilddog_c2n_parseMap(p_data);   
    }
    else
    {
        Wilddog_Node_T * p_node = NULL;
        wFloat d_float = 1.0;
        s8 d_type = 0;
        Wilddog_Str_T* p_value = NULL;
        if(WILDDOG_CBOR_SPECIAL == type)
        {
            d_type = _wilddog_c2n_parseSpecial(p_data , &d_float);
            if(0 > d_type || d_float < 0)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "parse special error!");
                return NULL;
            }
            p_node = _wilddog_node_new();
            if(NULL == p_node)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "create node error!");
                return NULL;
            }

            p_node->d_wn_type = d_type;
            if(WILDDOG_NODE_TYPE_FLOAT == d_type)
            {
                p_value = (Wilddog_Str_T*)wmalloc(sizeof(wFloat));
                if(NULL == p_value)
                {
                    wilddog_debug_level(WD_DEBUG_ERROR, "malloc error!");
                    wilddog_node_delete(p_node);
                    return NULL;
                }
                *(wFloat*)p_value = d_float;
                p_node->p_wn_value = p_value;
                p_node->d_wn_len = sizeof(wFloat);
            }
        }
        else
        {
            p_value = _wilddog_c2n_parseSimpleItem(p_data, &len);
            if(NULL == p_value)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "parse simple item error!");
                return NULL;
            }
            p_node = _wilddog_node_new();
            if(NULL == p_node)
            {
                wilddog_debug_level(WD_DEBUG_ERROR, "create node error!");
                wfree(p_value);
                return NULL;
            }
            p_node->p_wn_value = p_value;
            p_node->d_wn_type = _wilddog_c2n_typeTranslate(type);
            if(p_node->d_wn_type < 0)
            {
                wilddog_node_delete(p_node);
                wfree(p_value);
                return NULL;
            }
            p_node->d_wn_len = len;
        }
        return p_node;
    }
    
}

/*CBOR parse function table*/
STATIC _cborParserFunc _wilddog_c2n_func_table[WILDDOG_CBOR_MAJORTYPE_NUM]=
{
    _wilddog_c2n_parseUint,
    _wilddog_c2n_parseNegint,
    _wilddog_c2n_parseByteStr,
    _wilddog_c2n_parseTextStr,
    NULL,
    NULL,
    NULL,
    NULL
};

/*
 * Function:    _wilddog_c2n_parseSimpleItem
 * Description: Parse CBOR simple type: Uint/Negint/ByteStr/TextStr
 * Input:       p_data: The payload; 
 *              len: The length;
 * Output:      N/A
 * Return:      Return Node value string
*/
STATIC Wilddog_Str_T * WD_SYSTEM _wilddog_c2n_parseSimpleItem
    (
    Wilddog_Payload_T* p_data, 
    int* len
    )
{
    u8 data = *(u8*)(p_data->p_dt_data + p_data->d_dt_pos);
    
    u8 type = WILDDOG_CBOR_TYPE(data);

    wilddog_assert(p_data->d_dt_len >= p_data->d_dt_pos, NULL);
    
    /* call type special parse function*/
    if(WILDDOG_CBOR_ARRAY == type || WILDDOG_CBOR_MAP == type || \
        WILDDOG_CBOR_TAG == type || WILDDOG_CBOR_SPECIAL == type )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "cannot parse type %d, pos %d!", \
                            type, p_data->d_dt_pos);
        return NULL;
    }
        
    return (_wilddog_c2n_func_table[WILDDOG_CBOR_TYPE_GET(data)])(p_data, len);
}


/*The default buffer length used for n2c*/
#define  DEFAULT_LENGTH 100


/*
 * Function:    _wilddog_n2c_uintAdditionalInfo
 * Description: Return additional info field value for input value
 * Input:       val: An integer
 * Output:      N/A
 * Return:      Return Byte with the additional info bits set
*/
STATIC u8 WD_SYSTEM _wilddog_n2c_uintAdditionalInfo(int val)
{
    if (val < WILDDOG_CBOR_FOLLOW_1BYTE) 
    {
        return val;
    }
    else if (val <= 0xff) 
    {
        return WILDDOG_CBOR_FOLLOW_1BYTE;
    }
    else if (val <= 0xffff)
    {
        return WILDDOG_CBOR_FOLLOW_2BYTE;
    }

    return WILDDOG_CBOR_FOLLOW_4BYTE;
}

/*
 * Function:    _wilddog_n2c_encodeUint
 * Description: Encode the unsigned int
 * Input:       p_node: pointer to source node
 * Output:      p_data: output data type
 * Return:      Success:0 Faied:-1
*/
STATIC int WD_SYSTEM _wilddog_n2c_encodeUint
    (
    Wilddog_Node_T *p_node, 
    Wilddog_Payload_T *p_data
    )
{
    Wilddog_Str_T *ptr = NULL;
    Wilddog_Str_T *value = NULL;
    /*uint data max use length*/
    int maxExpectLen = WILDDOG_CBOR_HEAD_LEN + sizeof(s32);
    
    if(p_data->d_dt_pos + maxExpectLen + 2 > p_data->d_dt_len )
    {
        ptr = (Wilddog_Str_T*)wrealloc( p_data->p_dt_data, p_data->d_dt_len, \
                                        p_data->d_dt_pos + maxExpectLen + 2);
        p_data->d_dt_len = p_data->d_dt_pos + maxExpectLen + 2;
        if(ptr == NULL)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "n2c cannot realloc buf!");
            wfree(p_data->p_dt_data);
            return WILDDOG_ERR_NULL;
        }
        p_data->p_dt_data = ptr;
    }
    
    value = p_node->p_wn_value;
    *(p_data->p_dt_data + p_data->d_dt_pos) = \
                WILDDOG_CBOR_UINT | \
                _wilddog_n2c_uintAdditionalInfo(*(u32 *)(value)); 
    
    (p_data->d_dt_pos) += WILDDOG_CBOR_HEAD_LEN;
    if(_wilddog_n2c_uintAdditionalInfo(*(u32 *)(value)) == \
            WILDDOG_CBOR_FOLLOW_1BYTE
        )
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = (*(u32 *)(value));
        (p_data->d_dt_pos) += WILDDOG_CBOR_FOLLOW_1BYTE_LEN;
    }
    else if(_wilddog_n2c_uintAdditionalInfo(*(u32 *)(value)) == \
        WILDDOG_CBOR_FOLLOW_2BYTE
        )
    {
        *(u16 *)(p_data->p_dt_data + p_data->d_dt_pos) = \
                                            wilddog_htons(*(u16 *)(value));
        (p_data->d_dt_pos) += WILDDOG_CBOR_FOLLOW_2BYTE_LEN;
    }
    else if(_wilddog_n2c_uintAdditionalInfo(*(u32 *)(value)) == \
                    WILDDOG_CBOR_FOLLOW_4BYTE
        )
    {
        *(u32 *)(p_data->p_dt_data + p_data->d_dt_pos) = \
                                            wilddog_htonl(*(u32 *)(value));
        (p_data->d_dt_pos) += WILDDOG_CBOR_FOLLOW_4BYTE_LEN;
    }

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_n2c_encodeNegint
 * Description: Encode the negative integer
 * Input:       p_node: pointer to source node
 * Output:      p_data: output data type
 * Return:      Success:0 Faied:-1
*/
STATIC int WD_SYSTEM _wilddog_n2c_encodeNegint
    (
    Wilddog_Node_T *p_node, 
    Wilddog_Payload_T *p_data
    )
{
    Wilddog_Str_T *ptr = NULL;
    Wilddog_Str_T *value = NULL;
    /*negint data max use length*/
    int maxExpectLen = WILDDOG_CBOR_HEAD_LEN + sizeof(s32);

    if(p_data->d_dt_pos + maxExpectLen + 2 > p_data->d_dt_len )
    {
        ptr = (Wilddog_Str_T*)wrealloc( p_data->p_dt_data, p_data->d_dt_len, \
                                        p_data->d_dt_pos + maxExpectLen + 2);
        p_data->d_dt_len = p_data->d_dt_pos + maxExpectLen + 2;
        if(ptr == NULL)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "n2c cannot realloc buf!");
            wfree(p_data->p_dt_data);
            return WILDDOG_ERR_NULL;
        }
        p_data->p_dt_data = ptr;
    }

    value = p_node->p_wn_value;
    *(p_data->p_dt_data + p_data->d_dt_pos) = \
        WILDDOG_CBOR_NEGINT | \
        _wilddog_n2c_uintAdditionalInfo((-1 - *(s32 *)(value))); 
    (p_data->d_dt_pos) += WILDDOG_CBOR_HEAD_LEN;
    if(_wilddog_n2c_uintAdditionalInfo((-1 - *(s32 *)(value))) == \
        WILDDOG_CBOR_FOLLOW_1BYTE
        )
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = \
                                            -1 - (*(s32 *)(value));
        (p_data->d_dt_pos) += WILDDOG_CBOR_FOLLOW_1BYTE_LEN;
    }
    else if(
        _wilddog_n2c_uintAdditionalInfo(-1 - *(s32 *)(value)) == \
        WILDDOG_CBOR_FOLLOW_2BYTE
        )
    {
        *(s16 *)(p_data->p_dt_data + p_data->d_dt_pos) = \
                                    wilddog_htons(-1 - *(s16 *)(value));
        (p_data->d_dt_pos) += WILDDOG_CBOR_FOLLOW_2BYTE_LEN;
    }
    else if(
        _wilddog_n2c_uintAdditionalInfo(-1 - *(s32 *)(value)) == \
        WILDDOG_CBOR_FOLLOW_4BYTE
        )
    {
        *(s32 *)(p_data->p_dt_data + p_data->d_dt_pos) = \
                                    wilddog_htonl(-1 - (*(s32 *)(value)));
        (p_data->d_dt_pos) += WILDDOG_CBOR_FOLLOW_4BYTE_LEN;
    }

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_n2c_encodeSpecial
 * Description: Encode the True Flase and Null type
 * Input:       p_node: pointer to source node
 * Output:      p_data: output data type
 * Return:      Success:0 Faied:-1
*/
STATIC int WD_SYSTEM _wilddog_n2c_encodeSpecial
    (
    Wilddog_Node_T *p_node,
    Wilddog_Payload_T *p_data
    )
{
    Wilddog_Str_T *ptr;
    
    /*special data max use length*/
    int maxExpectLen = WILDDOG_CBOR_HEAD_LEN;

    if(p_data->d_dt_pos + maxExpectLen + 2 > p_data->d_dt_len )
    {
        ptr = (Wilddog_Str_T*)wrealloc( p_data->p_dt_data, p_data->d_dt_len, \
                                        p_data->d_dt_pos + maxExpectLen + 2);
        p_data->d_dt_len = p_data->d_dt_pos + maxExpectLen + 2;
        if(ptr == NULL)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "n2c cannot realloc buf!");
            wfree(p_data->p_dt_data);
            return WILDDOG_ERR_NULL;
        }
        p_data->p_dt_data = ptr;
    }

    if(WILDDOG_NODE_TYPE_TRUE == p_node->d_wn_type)
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = WILDDOG_CBOR_TRUE;
    }
    else if(WILDDOG_NODE_TYPE_FALSE == p_node->d_wn_type)
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = WILDDOG_CBOR_FALSE;
    }
    else if(WILDDOG_NODE_TYPE_NULL == p_node->d_wn_type)
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = WILDDOG_CBOR_NULL;
    }
    
    (p_data->d_dt_pos) += WILDDOG_CBOR_HEAD_LEN;
    
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_n2c_encodeFloat
 * Description: Encode the Float type
 * Input:       p_node: pointer to source node
 * Output:      p_data: output data type
 * Return:      Success:0 Faied:-1
*/
STATIC int WD_SYSTEM _wilddog_n2c_encodeFloat
    (
    Wilddog_Node_T *p_node,
    Wilddog_Payload_T *p_data
    )
{
    Wilddog_Str_T *ptr;
    /*float data max use length*/
    int maxExpectLen = WILDDOG_CBOR_HEAD_LEN + sizeof(wFloat);

    if(p_data->d_dt_pos + 2 + maxExpectLen > p_data->d_dt_len )
    {
        
        ptr = (Wilddog_Str_T*)wrealloc( p_data->p_dt_data, p_data->d_dt_len, \
                                        p_data->d_dt_pos + 2 + maxExpectLen);
        p_data->d_dt_len = p_data->d_dt_pos + 2 + maxExpectLen;
        if(ptr == NULL)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "n2c cannot realloc buf!");
            wfree(p_data->p_dt_data);
            return WILDDOG_ERR_NULL;
        }
        p_data->p_dt_data = ptr;
    }
    
#if WILDDOG_MACHINE_BITS != 8
    *(p_data->p_dt_data + p_data->d_dt_pos) = WILDDOG_CBOR_FLOAT64;
    (p_data->d_dt_pos) += WILDDOG_CBOR_HEAD_LEN;
    
    _wilddog_swap64((u8*)p_node->p_wn_value, \
                    (u8*)(p_data->p_dt_data + p_data->d_dt_pos));
#else
    *(p_data->p_dt_data + p_data->d_dt_pos) = WILDDOG_CBOR_FLOAT32;
    (p_data->d_dt_pos) += WILDDOG_CBOR_HEAD_LEN;
    _wilddog_swap32((u8*)p_node->p_wn_value, \
                    (u8*)(p_data->p_dt_data + p_data->d_dt_pos));
#endif

    (p_data->d_dt_pos) += sizeof(wFloat);

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_n2c_encodeString
 * Description: Encode the node  String 
 * Input:       p_node: pointer to source node
               type: node key or node value
 * Output:      p_data: output data type
 * Return:      Success:0 Faied:-1
*/
STATIC int WD_SYSTEM _wilddog_n2c_encodeString
    (
    Wilddog_Node_T *p_node, 
    Wilddog_Payload_T *p_data,
    Node_String_T type
    )
{
    Wilddog_Str_T *ptr;
    size_t len = 0;

    
    if(NULL == p_node->p_wn_key && TYPE_KEY == type)
        return WILDDOG_ERR_NOERR;

    /*string data max use length*/
    int maxExpectLen = WILDDOG_CBOR_HEAD_LEN + p_node->d_wn_len + \
                       WILDDOG_CBOR_FOLLOW_4BYTE_LEN;

    if(p_data->d_dt_pos + 2 + maxExpectLen > p_data->d_dt_len )
    {
        ptr = (Wilddog_Str_T*)wrealloc(p_data->p_dt_data, p_data->d_dt_len, \
                                       p_data->d_dt_pos + 2 + maxExpectLen);
        p_data->d_dt_len = p_data->d_dt_pos + 2 + maxExpectLen;
        if(ptr == NULL)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "n2c cannot realloc buf!");
            wfree(p_data->p_dt_data);
            return WILDDOG_ERR_NULL;
        }
        p_data->p_dt_data = ptr;
    }

    if(type == TYPE_KEY)
    {
        if(NULL != p_node->p_wn_key)
            len = strlen((const char *)p_node->p_wn_key);
    }
    else if(type == TYPE_VALUE)
    {
        if(WILDDOG_NODE_TYPE_UTF8STRING == p_node->d_wn_type)
            if(NULL != p_node->p_wn_value)
                len = strlen((const char *)p_node->p_wn_value);

        if(WILDDOG_NODE_TYPE_BYTESTRING == p_node->d_wn_type)
                len = p_node->d_wn_len;
    }

    if(WILDDOG_NODE_TYPE_UTF8STRING == p_node->d_wn_type)
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = \
            WILDDOG_CBOR_TEXT_STRING | \
            _wilddog_n2c_uintAdditionalInfo(len);
    }
    else if(WILDDOG_NODE_TYPE_BYTESTRING == p_node->d_wn_type)
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = \
            WILDDOG_CBOR_BYTE_STRING | \
            _wilddog_n2c_uintAdditionalInfo(len);
    }
    else
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = \
            WILDDOG_CBOR_TEXT_STRING | \
            _wilddog_n2c_uintAdditionalInfo(len);
    }
    
    (p_data->d_dt_pos) += WILDDOG_CBOR_HEAD_LEN;
    if(_wilddog_n2c_uintAdditionalInfo(len) == WILDDOG_CBOR_FOLLOW_1BYTE)
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = len;
        (p_data->d_dt_pos) += WILDDOG_CBOR_FOLLOW_1BYTE_LEN;
    }
    else if(_wilddog_n2c_uintAdditionalInfo(len) == WILDDOG_CBOR_FOLLOW_2BYTE)
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = (len & 0xff00 ) >> 8;
        (p_data->d_dt_pos)++;
        *(p_data->p_dt_data + p_data->d_dt_pos) = (len) & 0xff;
        (p_data->d_dt_pos)++;
    }
    else if(_wilddog_n2c_uintAdditionalInfo(len) == WILDDOG_CBOR_FOLLOW_4BYTE)
    {
        *(p_data->p_dt_data + p_data->d_dt_pos) = ((len) & 0xff000000 ) >> 24;
        (p_data->d_dt_pos)++;
        *(p_data->p_dt_data + p_data->d_dt_pos) = ((len) & 0xff0000) >> 16;
        (p_data->d_dt_pos)++;
        *(p_data->p_dt_data + p_data->d_dt_pos) = ((len) & 0xff00 ) >> 8;
        (p_data->d_dt_pos)++;
        *(p_data->p_dt_data + p_data->d_dt_pos) = (len) & 0xff;
        (p_data->d_dt_pos)++;
    }

    if(type == TYPE_KEY)
    {
        memcpy( (p_data->p_dt_data + p_data->d_dt_pos), p_node->p_wn_key, \
                    len);
    }
    else if(type == TYPE_VALUE)
    {
        memcpy( (p_data->p_dt_data + p_data->d_dt_pos), p_node->p_wn_value, \
                    len);
    }
    (p_data->d_dt_pos) += len;

    return WILDDOG_ERR_NOERR;

}

/*
 * Function:    _wilddog_n2c_encodeMap
 * Description: Encode the Map type 
 * Input:       p_node: pointer to source node
 * Output:      p_data: output data type
 * Return:      Success:0 Faied:-1
*/
STATIC int WD_SYSTEM _wilddog_n2c_encodeMap
    (
    Wilddog_Node_T *p_node,
    Wilddog_Payload_T *p_data
    )
{
    Wilddog_Str_T *ptr;
    /*map data max use length*/
    int maxExpectLen = WILDDOG_CBOR_HEAD_LEN;

    if((p_data->d_dt_pos + maxExpectLen + 2) > p_data->d_dt_len )
    {
        ptr = (Wilddog_Str_T*)wrealloc( p_data->p_dt_data, p_data->d_dt_len, \
                                        p_data->d_dt_pos + maxExpectLen + 2);
        p_data->d_dt_len = p_data->d_dt_pos + maxExpectLen + 2;
        if(ptr == NULL)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "n2c cannot realloc buf!");
            wfree(p_data->p_dt_data);
            return WILDDOG_ERR_NULL;
        }
        p_data->p_dt_data = ptr;
    }

    *(p_data->p_dt_data + p_data->d_dt_pos) = \
        WILDDOG_CBOR_MAP | WILDDOG_CBOR_FOLLOW_VAR;
    
    (p_data->d_dt_pos) += WILDDOG_CBOR_HEAD_LEN;

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_n2c_inner
 * Description: Encode the Node tree 
 * Input:       p_node: pointer to source node
 * Output:      p_data: output data type
 * Return:      Success:0 Faied:-1
*/
STATIC int WD_SYSTEM _wilddog_n2c_inner
    ( 
    Wilddog_Node_T *p_node, 
    Wilddog_Payload_T *p_data
    )      
{
    Wilddog_Node_T *child;
    
    if( p_node->p_wn_child == NULL)
    {
        _wilddog_n2c_encodeString(p_node, p_data, TYPE_KEY);
        if(WILDDOG_NODE_TYPE_NUM == p_node->d_wn_type)   /*number*/
        {
            
            if( *(s32 *)(p_node->p_wn_value) >= 0)
            {
                if(_wilddog_n2c_encodeUint(p_node, p_data))
                    return WILDDOG_ERR_NULL;
            }
            else
            {
                if(_wilddog_n2c_encodeNegint(p_node, p_data))
                    return WILDDOG_ERR_NULL;
            }
        }
        else if(WILDDOG_NODE_TYPE_BYTESTRING == p_node->d_wn_type \
            || WILDDOG_NODE_TYPE_UTF8STRING == p_node->d_wn_type)   
        {
            if(_wilddog_n2c_encodeString(p_node, p_data, TYPE_VALUE))
                return WILDDOG_ERR_NULL;
        }
        else if(WILDDOG_NODE_TYPE_NULL == p_node->d_wn_type \
            || WILDDOG_NODE_TYPE_FALSE == p_node->d_wn_type  \
            || WILDDOG_NODE_TYPE_TRUE == p_node->d_wn_type) 
        {
            if(_wilddog_n2c_encodeSpecial(p_node, p_data))
                return WILDDOG_ERR_NULL;
        }
        else if(WILDDOG_NODE_TYPE_FLOAT == p_node->d_wn_type)
        {
            if(_wilddog_n2c_encodeFloat(p_node, p_data))
                return WILDDOG_ERR_NULL;
        }
        else if(WILDDOG_NODE_TYPE_OBJECT == p_node->d_wn_type)
        {
            return WILDDOG_ERR_INVALID;
        }
        
    }
    else
    {
        _wilddog_n2c_encodeString(p_node, p_data, TYPE_KEY);

        if(_wilddog_n2c_encodeMap(p_node, p_data))
            return WILDDOG_ERR_NULL;
        
        child = p_node->p_wn_child;
        while(child != NULL)
        {
            _wilddog_n2c_inner( child, p_data);
            child = child->p_wn_next;
        }
        
        *(p_data->p_dt_data + p_data->d_dt_pos) = WILDDOG_CBOR_BREAK;
        (p_data->d_dt_pos)++;

    }
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_node2Cbor
 * Description: Encode the Node tree 
 * Input:       p_node: pointer to source node
 * Output:      NA
 * Return:      CBOR data
*/
Wilddog_Payload_T * WD_SYSTEM _wilddog_node2Cbor(Wilddog_Node_T * p_node)
{
    Wilddog_Str_T *ptr;
    Wilddog_Str_T *newptr;
    Wilddog_Str_T * p_tmp = NULL;
    Wilddog_Payload_T *p_data;
    
    p_data = (Wilddog_Payload_T*)wmalloc(sizeof(Wilddog_Payload_T));
    if(p_data == NULL)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
                                "n2c cannot malloc Wilddog_Payload_T!");
        return NULL;
    }
    p_data->p_dt_data = (u8 *)wmalloc(DEFAULT_LENGTH);
    if(p_data->p_dt_data == NULL)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
                                "n2c cannot wmalloc Wilddog_Payload_T buf!");
        return NULL;
    }
    
    p_data->d_dt_len = DEFAULT_LENGTH;
    p_data->d_dt_pos = 0;


    p_tmp = p_node->p_wn_key;
    p_node->p_wn_key = NULL;

    if(!_wilddog_n2c_inner(p_node, p_data)) 
    {
        ptr = (Wilddog_Str_T*)wrealloc( p_data->p_dt_data, p_data->d_dt_len, \
                                        p_data->d_dt_pos);
        p_data->d_dt_len = p_data->d_dt_pos;
        if(ptr == NULL)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "n2c cannot realloc buf!");
            wfree(p_data->p_dt_data);
            return NULL;
        }
        p_data->p_dt_data = ptr;
        p_data->d_dt_pos = 0;
        
        newptr = wmalloc(p_data->d_dt_len);
        if(newptr == NULL)
        {
            wilddog_debug_level(WD_DEBUG_ERROR, "n2c cannot realloc buf!");
            return NULL;
        }
        memcpy(newptr, p_data->p_dt_data+0, p_data->d_dt_len-0);
        wfree(ptr);
        
        p_data->p_dt_data = newptr;
        p_node->p_wn_key = p_tmp;

        return p_data;
    }
    else
    {
        p_node->p_wn_key = p_tmp;
        if(p_data->p_dt_data != NULL)
            wfree(p_data->p_dt_data);
        if(p_data != NULL)
            wfree(p_data);
        return NULL;
    }
}

/*
 * Function:    _wilddog_node2Payload
 * Description: Convert the node tree to the payload 
 * Input:       p_node: input Node tree
 * Output:      NA
 * Return:      CBOR data
*/
Wilddog_Payload_T * WD_SYSTEM _wilddog_node2Payload
    (
    Wilddog_Node_T * p_node
    )
{
    return _wilddog_node2Cbor(p_node);
}

/*
 * Function:    _wilddog_payload2Node
 * Description: Convert the payload to the node tree 
 * Input:       p_data: CBOR data
 * Output:      NA
 * Return:      Node tree
*/
Wilddog_Node_T * WD_SYSTEM _wilddog_payload2Node
    (
    Wilddog_Payload_T* p_data
    )
{
    return _wilddog_cbor2Node(p_data);
}

