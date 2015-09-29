/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_node.c
 *
 * Description: Node functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Baikal.Hu       2015-05-15  Create file.
 * 0.4.3        Jimmy.Pan       2015-07-04  Add annotation, fix key check error.
 *
 */
 
#ifndef WILDDOG_PORT_TYPE_ESP
#include <stdio.h>
#endif
#include <string.h>
#include <stdlib.h>
 
#include "wilddog_api.h"
#include "wilddog_debug.h"
#include "wilddog_common.h"

#define WILDDOG_KEY_MAX_LEN 768

Wilddog_Return_T wilddog_node_deleteChildren(Wilddog_Node_T *p_node);

/*
 * Function:    _wilddog_node_new
 * Description: Create a new node.
 * Input:       N/A
 * Output:      N/A
 * Return:      Pointer to the new node.
*/
Wilddog_Node_T * WD_SYSTEM _wilddog_node_new(void)
{
    Wilddog_Node_T *node;

    node = (Wilddog_Node_T*)wmalloc(sizeof(Wilddog_Node_T));
    if(NULL == node)
    {
        wilddog_debug_level( WD_DEBUG_ERROR, "malloc node error");
        return NULL;
    }

    node->p_wn_next = NULL;
    node->p_wn_prev = NULL;
    node->p_wn_child = NULL;
    node->p_wn_parent = NULL;
    node->d_wn_type = 0;
    node->p_wn_value = NULL;
    node->p_wn_key = NULL;
    node->d_wn_len = 0;
    return node;
}

/*
 * Function:    _isKeyValid
 * Description: Check the key is valid or not.
 * Input:       key: the key to be checked.
 *              isSpritValid : if TRUE , '/' is not valid
 * Output:      N/A
 * Return:      valid returns TRUE, others return FALSE.
*/
STATIC BOOL WD_SYSTEM _isKeyValid(Wilddog_Str_T * key, BOOL isSpritValid)
{
    int len, i;
    volatile u8 data;

    if(NULL == key)
        return TRUE;
    len = strlen((const char*)key);

    if(len > WILDDOG_KEY_MAX_LEN)
        return FALSE;
    if(len == 1 && key[0] == '/')
        return TRUE;
    for(i = 0; i < len; i++)
    {
        data = key[i];
        if( data <  32  || \
            data == '.' || \
            data == '$' || \
            data == '#' || \
            data == '[' || \
            data == ']' || \
            data ==  127
            )
            return FALSE;
        if(FALSE == isSpritValid)
        {
            if(data == '/')
                return FALSE;
        }
    }
    return TRUE;
}

/*
 * Function:    _wilddog_node_newWithStr
 * Description: Create a node with key, if key is a path, create a node tree.
 * Input:       key: the key which used in the node
 * Output:      p_node: pointer of the last node
 * Return:      Success returns the head of the node tree, or NULL.
*/
STATIC Wilddog_Node_T * WD_SYSTEM _wilddog_node_newWithStr
    (
    Wilddog_Str_T* key, 
    Wilddog_Node_T** p_node
    )
{
    int i, pos, length = 0;
    Wilddog_Node_T * p_head = NULL, *p_tmp = NULL, *p_parent = NULL;
    Wilddog_Str_T * p_tmpStr = NULL;

    if(NULL == key)
    {
        p_head = _wilddog_node_new();
        if(NULL == p_head)
        {
            *p_node = NULL;
            return NULL;
        }
        *p_node = p_head;
        return p_head;
    }
    /* do not check '/', it may be valid. */
    if(_isKeyValid(key, TRUE) == FALSE)
    {
        *p_node = NULL;
        return NULL;
    }
    
    length = strlen((const char*)key);
    if(length == 1 && key[0] == '/')
    {
        p_head = _wilddog_node_new();
        if(NULL == p_head)
        {
            *p_node = NULL;
            return NULL;
        }
        p_head->p_wn_key = wmalloc(length + 1);
        if(NULL == p_head)
        {
            wilddog_node_delete(p_head);
            *p_node = NULL;
            return NULL;
        }
        p_head->p_wn_key[0] = '/';
        *p_node = p_head;
        return p_head;
    }
    p_tmp = p_head;

    p_tmpStr = (Wilddog_Str_T*)wmalloc(length + 1);
    if(NULL == p_tmpStr)
    {
        *p_node = NULL;
        wilddog_node_delete(p_head);
        return NULL;
    }
    strcpy((char*)p_tmpStr, (char*)key);
    /*
     * key may be a path like /a/b/c, so create a node tree.
     */
    for(i = 0; i < length; i++)
    {
        if(p_tmpStr[i] == '/')
            p_tmpStr[i] = 0;
    }

    for(i = 0; i < length; )
    {
        if(p_tmpStr[i] == 0)
        {
            i++;
            continue;
        }
        pos = strlen((const char*)(p_tmpStr + i));
        if(pos > 0)
        {
            if(NULL == p_head)
            {
                p_head = _wilddog_node_new();
                if(NULL == p_head)
                {
                    wfree(p_tmpStr);
                    *p_node = NULL;
                    return NULL;
                }
                p_tmp = p_head;
                p_tmp->p_wn_key = (Wilddog_Str_T*)wmalloc(pos + 1);
                strcpy((char*)p_tmp->p_wn_key, (char*)(p_tmpStr + i));
                i += pos;
                continue;
            }
            p_parent = p_tmp;
            p_tmp = _wilddog_node_new();
            if(NULL == p_tmp)
            {
                wfree(p_tmpStr);
                wilddog_node_delete(p_head);
                return NULL;
            }
            p_tmp->p_wn_key = (Wilddog_Str_T*)wmalloc(pos + 1);
            strcpy((char*)p_tmp->p_wn_key, (char*)(p_tmpStr + i));
            wilddog_node_addChild(p_parent, p_tmp);
            i += pos;
        }
    }
    wfree(p_tmpStr);
    *p_node = p_tmp;
    return p_head;
}

/*
 * Function:    wilddog_node_createFalse
 * Description: Create a node, its type is FALSE.
 * Input:       key:    The pointer to the node's key (can be NULL).
 * Output:      N/A
 * Return:      if success, return pointer points to the node, else return NULL.
 * Others:      N/A
*/
Wilddog_Node_T * WD_SYSTEM wilddog_node_createFalse(Wilddog_Str_T* key)
{
    Wilddog_Node_T *p_node = NULL;
    Wilddog_Node_T * p_head = _wilddog_node_newWithStr(key, &p_node);
    if(p_node)
    {
        p_node->d_wn_type = WILDDOG_NODE_TYPE_FALSE;
        p_node->d_wn_len = 0;
    }
    return p_head;
}
/*
 * Function:    wilddog_node_createTrue
 * Description: Create a node, its type is TRUE.
 * Input:       key:    The pointer to the node's key (can be NULL).
 * Output:      N/A
 * Return:      if success, return pointer points to the node, else return NULL.
 * Others:      N/A
*/
Wilddog_Node_T * WD_SYSTEM wilddog_node_createTrue(Wilddog_Str_T* key)
{
    Wilddog_Node_T *p_node = NULL;
    Wilddog_Node_T * p_head = _wilddog_node_newWithStr(key, &p_node);
    if(p_node)
    {
        p_node->d_wn_type = WILDDOG_NODE_TYPE_TRUE;
        p_node->d_wn_len = 0;
    }
    return p_head;
}
/*
 * Function:    wilddog_node_createNull
 * Description: Create a node, its type is NULL.
 * Input:       key:    The pointer to the node's key (can be NULL).
 * Output:      N/A
 * Return:      if success, return pointer points to the node, else return NULL.
 * Others:      N/A
*/
Wilddog_Node_T * WD_SYSTEM wilddog_node_createNull(Wilddog_Str_T* key)
{
    Wilddog_Node_T *p_node = NULL;
    Wilddog_Node_T * p_head = _wilddog_node_newWithStr(key, &p_node);
    if(p_node)
    {
        p_node->d_wn_type = WILDDOG_NODE_TYPE_NULL;
        p_node->d_wn_len = 0;
    }
    return p_head;
}
/*
 * Function:    wilddog_node_createNum
 * Description: Create a node, its type is integer(32 bits).
 * Input:       key:    The pointer to the node's key (can be NULL).
 *              num:    integer value.
 * Output:      N/A
 * Return:      if success, return pointer points to the node, else return NULL.
 * Others:      N/A
*/
Wilddog_Node_T * WD_SYSTEM wilddog_node_createNum
    (
    Wilddog_Str_T* key, 
    s32 num
    )
{
    Wilddog_Node_T *p_node = NULL;
    Wilddog_Node_T * p_head = _wilddog_node_newWithStr(key, &p_node);

    if(p_node)
    {
        p_node->d_wn_type = WILDDOG_NODE_TYPE_NUM;
        p_node->d_wn_len = sizeof(num);
        p_node->p_wn_value = wmalloc(p_node->d_wn_len);
        if(NULL == p_node->p_wn_value)
        {
            wilddog_node_delete(p_node);
            return NULL;
        }
        *(s32*)p_node->p_wn_value = num;
    }
    return p_head;
}
/*
 * Function:    wilddog_node_createFloat
 * Description: Create a node, its type is float(on 8-bit machine is 32 bits 
 *              else is 64 bits).
 * Input:       key:    The pointer to the node's key (can be NULL).
 *              num:    float value.
 * Output:      N/A
 * Return:      if success, return pointer points to the node, else return NULL.
 * Others:      N/A
*/
Wilddog_Node_T * WD_SYSTEM wilddog_node_createFloat
    (
    Wilddog_Str_T* key, 
    wFloat num
    )
{
    Wilddog_Node_T *p_node = NULL;
    Wilddog_Node_T * p_head = _wilddog_node_newWithStr(key, &p_node);

    if(p_node)
    {
        p_node->d_wn_type = WILDDOG_NODE_TYPE_FLOAT;
        p_node->d_wn_len = sizeof(wFloat);
        p_node->p_wn_value = wmalloc(p_node->d_wn_len);
        
        if(NULL == p_node->p_wn_value)
        {
            wilddog_node_delete(p_node);
            return NULL;
        }
        *(wFloat*)p_node->p_wn_value = num;
    }
    return p_head;
}
/*
 * Function:    wilddog_node_createBString
 * Description: Create a node, its type is byte string(binary buffer).
 * Input:       key:    The pointer to the node's key (can be NULL).
 *              value:  The pointer to the string.
 *              len:    The length of the string.
 * Output:      N/A
 * Return:      if success, return pointer points to the node, else return NULL.
 * Others:      N/A
*/
Wilddog_Node_T * WD_SYSTEM wilddog_node_createBString
    (
    Wilddog_Str_T* key, 
    u8 *value, 
    int len
    )
{
    Wilddog_Node_T * p_node= NULL, *p_head;
    
    if(NULL == value || len < 0)
        return NULL;
    
    p_head = _wilddog_node_newWithStr(key, &p_node);
    if(p_node)
    {
        p_node->d_wn_type = WILDDOG_NODE_TYPE_BYTESTRING;
        p_node->p_wn_value = wmalloc(len);
        p_node->d_wn_len = len;
        if(NULL == p_node->p_wn_value)
        {
            wilddog_node_delete(p_node);
            return NULL;
        }
        memcpy(p_node->p_wn_value, value, len);
    }
    return p_head;
}
/*
 * Function:    wilddog_node_createUString
 * Description: Create a node, its type is UTF-8 string.
 * Input:       key:    The pointer to the node's key (can be NULL).
 *              value:  The pointer to the string.
 * Output:      N/A
 * Return:      if success, return pointer points to the node, else return NULL.
 * Others:      N/A
*/
Wilddog_Node_T * WD_SYSTEM wilddog_node_createUString
    (
    Wilddog_Str_T* key, 
    Wilddog_Str_T *value
    )
{
    Wilddog_Node_T * p_node = NULL, *p_head;
    
    if(NULL == value)
        return NULL;
    
     p_head = _wilddog_node_newWithStr(key, &p_node);

    if(p_node)
    {
        p_node->d_wn_type = WILDDOG_NODE_TYPE_UTF8STRING;
        p_node->d_wn_len = strlen((const char *)value);
        p_node->p_wn_value = wmalloc(p_node->d_wn_len + 1);
        if(NULL == p_node->p_wn_value)
        {
            wilddog_node_delete(p_node);
            return NULL;
        }
        memcpy(p_node->p_wn_value, value, p_node->d_wn_len);
    }
    return p_head;
}
/*
 * Function:    wilddog_node_createObject
 * Description: Create a node, its type is OBJECT.
 * Input:       key:    The pointer to the node's key (can be NULL).
 * Output:      N/A
 * Return:      if success, return pointer points to the node, else return NULL.
 * Others:      N/A
*/
Wilddog_Node_T * WD_SYSTEM wilddog_node_createObject
    (
    Wilddog_Str_T* key
    )
{
    Wilddog_Node_T *p_node = NULL;
    Wilddog_Node_T * p_head = _wilddog_node_newWithStr(key, &p_node);

    if(p_node)
    {
        p_node->d_wn_type = WILDDOG_NODE_TYPE_OBJECT;
        p_node->d_wn_len = 0;
        p_node->p_wn_value = NULL;
    }
    return p_head;
}
/*
 * Function:    _wilddog_node_setKey
 * Description: Set a node's key.
 * Input:       node:   The pointer to the node.
 *              key:    The pointer to the node's new key.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
 * Others:      N/A
*/
Wilddog_Return_T WD_SYSTEM _wilddog_node_setKey
    (
    Wilddog_Node_T *node, 
    Wilddog_Str_T *key
    )
{
    int len;
    Wilddog_Node_T *first_child;

    if(NULL == node)
        return WILDDOG_ERR_INVALID;
    
    if(FALSE == _isKeyValid(key, FALSE))
        return WILDDOG_ERR_INVALID;
    
    if(node->p_wn_parent != NULL)
    {
        first_child = node->p_wn_parent->p_wn_child;
        while(first_child != NULL)
        {
            if(first_child->p_wn_key != NULL )
            {
                if(strcmp((const char*)first_child->p_wn_key, \
                           (const char *)key ) == 0
                   )  
                {
                    return WILDDOG_ERR_INVALID;
                }
            }
            first_child = first_child->p_wn_next;
        }
    }


    if(node->p_wn_key != NULL)
    {
        wfree(node->p_wn_key);
        node->p_wn_key = NULL;
    }
    if(!key)
    {
        return WILDDOG_ERR_NOERR;
    }
    len = strlen((const char *)key);
    node->p_wn_key = wmalloc(len + 1);
    if(node->p_wn_key == NULL)
    {
        wilddog_debug_level( WD_DEBUG_ERROR, "setKey malloc error");

        return WILDDOG_ERR_NULL;
    }
    else
    {
        memcpy(node->p_wn_key, key, len);
    }
    return WILDDOG_ERR_NOERR;
}
#if 0
/*
 * Function:    wilddog_node_getKey
 * Description: Get a node's key.
 * Input:       node:   The pointer to the node.
 * Output:      N/A
 * Return:      if success, returns pointer points to the key, else return NULL.
 * Others:      N/A
*/
STATIC const Wilddog_Str_T * WD_SYSTEM wilddog_node_getKey(Wilddog_Node_T *node)
{
    if(NULL == node)
        return NULL;

    if(node->p_wn_key == NULL)
    {
        return NULL;
    }
    else
    {
        return node->p_wn_key;
    }
}
#endif
/*
 * Function:    wilddog_node_setType
 * Description: Set a node's type.
 * Input:       node:   The pointer to the node.
 *              type:   The new type of the node.
 * Output:      N/A
 * Return:      if success, returns 0, else return negative number.
 * Others:      N/A
*/
STATIC Wilddog_Return_T WD_SYSTEM wilddog_node_setType
    (
    Wilddog_Node_T *node,
    u8 type
    )
{
    if(NULL == node)
        return WILDDOG_ERR_NULL;

    if(WILDDOG_NODE_TYPE_OBJECT < type)
        return WILDDOG_ERR_INVALID;

    /*node change from object to normal, so delete it's children*/
    if(WILDDOG_NODE_TYPE_OBJECT == node->d_wn_type && \
        WILDDOG_NODE_TYPE_OBJECT > type)
    {
        wilddog_node_deleteChildren(node);
    }
    node->d_wn_type = type;
    return WILDDOG_ERR_NOERR;
}
#if 0
/*
 * Function:    wilddog_node_getType
 * Description: Get a node's type.
 * Input:       node:   The pointer to the node.
 * Output:      N/A
 * Return:      if success, return type of the node, else return negative number.
 * Others:      N/A
*/
STATIC u8 WD_SYSTEM wilddog_node_getType(Wilddog_Node_T *node)
{
    if(NULL == node)
        return WILDDOG_ERR_NULL;
        
    return node->d_wn_type;
}
#endif
/*
 * Function:    wilddog_node_setValue
 * Description: Set a node's value.
 * Input:       node:   The pointer to the node.
 *              value:  The pointer to the new value.
 *              len:    The length of the new value.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
 * Others:      N/A
*/
Wilddog_Return_T WD_SYSTEM wilddog_node_setValue
    (
    Wilddog_Node_T *node, 
    u8 *value, 
    int len
    )
{
    if(NULL == node)
        return WILDDOG_ERR_INVALID;
    
    if(node->p_wn_value!= NULL)
    {
        wfree(node->p_wn_value);
        node->p_wn_value = NULL;
    }
    if(!value || !len)
    {
        node->p_wn_value = NULL;
        node->d_wn_len = 0;
        return WILDDOG_ERR_NOERR;
    }
    if(WILDDOG_NODE_TYPE_NUM == node->d_wn_type)
    {
        len = sizeof(s32);
    }
    else if(WILDDOG_NODE_TYPE_FLOAT == node->d_wn_type)
    {
        len = sizeof(wFloat);
    }
    else if(
            WILDDOG_NODE_TYPE_NUM > node->d_wn_type     || \
            WILDDOG_NODE_TYPE_OBJECT == node->d_wn_type
            )
    {
        node->p_wn_value = NULL;
        node->d_wn_len = 0;
        return WILDDOG_ERR_NOERR;
    }
    node->p_wn_value = wmalloc(len + 1);
    if(node->p_wn_value== NULL)
    {
        wilddog_debug_level( WD_DEBUG_ERROR, "setValue malloc error");

        return WILDDOG_ERR_NULL;
    }
    else
    {
        memcpy(node->p_wn_value, value, len);
        node->d_wn_len = len;
    }
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    wilddog_node_getValue
 * Description: Get a node's value.
 * Input:       node:   The pointer to the node.
 * Output:      len:    The length of the value.
 * Return:      if success, returns point of the value, else return NULL.
 * Others:      N/A
*/
Wilddog_Str_T* WD_SYSTEM wilddog_node_getValue
    (
    Wilddog_Node_T *node, 
    int * len
    )
{
    if(NULL == node || NULL == len)
        return NULL;
        
    if(node->p_wn_value== NULL)
    {
        *len = 0;
        return NULL;
    }
    else
    {
        *len = node->d_wn_len;
        return node->p_wn_value;
    }
}

/*
 * Function:    _wilddog_node_free
 * Description: Free a node and it's children.
 * Input:       node:   The pointer to the node.
 * Output:      N/A
 * Return:      0
 * Others:      N/A
*/
int WD_SYSTEM _wilddog_node_free(Wilddog_Node_T *node)
{
    wilddog_assert(node , -1);
    
    if(node->p_wn_child != NULL)
    {
        _wilddog_node_free(node->p_wn_child);
    }   
    if(node->p_wn_next != NULL)
    {
        _wilddog_node_free(node->p_wn_next);
    }
    node->p_wn_next = NULL;
    node->p_wn_prev = NULL;
    node->p_wn_child = NULL;
    node->p_wn_parent = NULL;
    node->d_wn_type = 0;
    node->d_wn_len = 0;
    if(node->p_wn_value != NULL)
        wfree(node->p_wn_value);
    node->p_wn_value = NULL;
    if(node->p_wn_key != NULL)
        wfree(node->p_wn_key);
    node->p_wn_key = NULL;
    if(node != NULL)
        wfree(node);
    node = NULL;
    return 0;
}
/*
 * Function:    _wilddog_node_findInner
 * Description: Find a node from the path.
 * Input:       node:   The pointer to the head node.
 *              path:   The relative path.
 * Output:      N/A
 * Return:      If find, return the pointer of the node.
 * Others:      N/A
*/
STATIC Wilddog_Node_T * WD_SYSTEM _wilddog_node_findInner
    (
    Wilddog_Node_T *node, 
    char *path
    )
{
    int len ;
    wilddog_assert(path, NULL);
    wilddog_assert(node, NULL);
    
    if(!node)
        return NULL;
    
    len = strlen((const char *)node->p_wn_key);
    if(strncmp((const char*)node->p_wn_key, (const char*)path, len) == 0)
    {
        if(*(path+len) == '/')
        {
            if(node->p_wn_child != NULL)
                return _wilddog_node_findInner(node->p_wn_child, \
                path+len+1);
            else
                return NULL;
        }
        else
        {
            return node;
        }
    }
    else
    {
        while(node->p_wn_next != NULL)
        {
            node = node->p_wn_next;
            return _wilddog_node_findInner(node, path);
        }
        return NULL;
    }
}

/*
 * Function:    wilddog_node_find
 * Description: Find a node from the path. It do some checks and 
 *                    then call _wilddog_node_findInner function.
 * Input:       node:   The pointer to the head node.
 *              path:   The relative path.
 * Output:      N/A
 * Return:      If find, return the pointer of the node.
 * Others:      N/A
*/
Wilddog_Node_T * WD_SYSTEM wilddog_node_find
    ( 
    Wilddog_Node_T *root, 
    char *path 
    )
{
    Wilddog_Node_T *node = NULL;

    if(!root || !path)
    {
        return NULL;
    }
    if((*path) == '/' && (strlen((const char*)path) == 1))
        return root;

    if((*path) == '/')
    {
        path++;/* remove the first '/' */
    }
    
    if(root->p_wn_child != NULL)
        node = root->p_wn_child;
        
    return _wilddog_node_findInner(node, path);

}

/*
 * Function:    wilddog_node_addChild
 * Description: add newnode as node's child.
 * Input:       node:   The pointer to the head.
 *              newnode: The newnode.
 * Output:      N/A
 * Return:      If add success, return WILDDOG_ERR_NOERR.
 * Others:      N/A
*/
Wilddog_Return_T WD_SYSTEM wilddog_node_addChild
    (
    Wilddog_Node_T *node, 
    Wilddog_Node_T *newnode
    )
{
    Wilddog_Node_T *first_child;

    if(!node || !newnode)
        return WILDDOG_ERR_NULL;
    if(node->p_wn_child != NULL)
    {
        first_child = node->p_wn_child;
        while(first_child != NULL)
        {
            if(first_child->p_wn_key != NULL && newnode->p_wn_key != NULL)
            {
                if(strcmp((const char *)first_child->p_wn_key, \
                           (const char *)newnode->p_wn_key ) == 0
                   )    
                {
                    break;
                }
            }
            first_child = first_child->p_wn_next;
        }
        if(first_child == NULL)
        {
            first_child = node->p_wn_child;
            first_child->p_wn_prev = newnode;
            newnode->p_wn_next = first_child;
            newnode->p_wn_parent = node;
            node->p_wn_child = newnode;
        }
        else
        {
            if(first_child->p_wn_prev == NULL)
            {
                newnode->p_wn_prev = NULL;
                newnode->p_wn_next = first_child->p_wn_next;
                newnode->p_wn_parent = node;
                node->p_wn_child = newnode;
            }
            else
            {
                first_child->p_wn_prev->p_wn_next = newnode;
                newnode->p_wn_prev = first_child->p_wn_prev;
                if(first_child->p_wn_next != NULL)
                {
                    first_child->p_wn_next->p_wn_prev = newnode;
                    newnode->p_wn_next = first_child->p_wn_next;
                }
                newnode->p_wn_parent = node;
            }
            first_child->p_wn_prev = NULL;
            first_child->p_wn_next = NULL;
            first_child->p_wn_parent = NULL;
            wilddog_node_delete(first_child);
        }
    }
    else
    {
        node->d_wn_type = WILDDOG_NODE_TYPE_OBJECT;
        if(node->p_wn_value)
            wfree(node->p_wn_value);
        node->p_wn_value = NULL;
        node->d_wn_len= 0;
        newnode->p_wn_parent = node;
        node->p_wn_child = newnode;
    }
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    wilddog_node_deleteChildren
 * Description: delete node's all child node.
 * Input:       node:   The pointer to the head.
 * Output:      N/A
 * Return:      If delete children node success, return WILDDOG_ERR_NOERR.
 * Others:      N/A
*/
Wilddog_Return_T WD_SYSTEM wilddog_node_deleteChildren
    (
    Wilddog_Node_T *p_node
    )
{
    Wilddog_Node_T *p_child = NULL;
    
    wilddog_assert(p_node, WILDDOG_ERR_NULL);

    p_child = p_node->p_wn_child;
    if(p_child)
    {
        Wilddog_Node_T *p_brother = p_child->p_wn_next;
        Wilddog_Node_T *p_nextBrother = p_brother;

        while(p_nextBrother)
        {
            p_brother = p_nextBrother;
            p_nextBrother = p_brother->p_wn_next;

            wilddog_node_delete(p_brother);
        }
        wilddog_node_delete(p_child);
    }
    p_node->p_wn_child = NULL;

    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    wilddog_node_delete
 * Description: delete the p_node and it's child node and free them.
 * Input:       p_node:   The pointer to the head.
 * Output:      N/A
 * Return:      If delete node success, return WILDDOG_ERR_NOERR.
 * Others:      N/A
*/
Wilddog_Return_T WD_SYSTEM wilddog_node_delete( Wilddog_Node_T *p_node)
{
    Wilddog_Node_T * p_head = NULL;

    if(NULL == p_node)
        return WILDDOG_ERR_NULL;
    p_head = p_node;

    /*remove the real node from the tree*/
    if(NULL == p_head->p_wn_parent)
    {
        goto DEL_FREE;
    }
    else
    {
        if(NULL != p_head->p_wn_next && NULL != p_head->p_wn_prev)
        {
            /*have both left and right brother*/
            p_head->p_wn_next->p_wn_prev = p_head->p_wn_prev;
            p_head->p_wn_prev->p_wn_next = p_head->p_wn_next;

        }
        else if(NULL == p_head->p_wn_next && NULL != p_head->p_wn_prev)
        {
            /*have left brother, no right brother*/
            p_head->p_wn_prev->p_wn_next = NULL;
            goto DEL_FREE;
        }
        else if(NULL != p_head->p_wn_next && NULL == p_head->p_wn_prev)
        {
            /*
             * have right brother, no left brother it must be the head 
             * of parent's son
             */
            p_head->p_wn_parent->p_wn_child = p_head->p_wn_next;
            goto DEL_FREE;
        }
        else if(NULL  == p_head->p_wn_next && NULL == p_head->p_wn_prev)
        {
            /*set it's parent as null node*/
            wilddog_node_setType(p_head->p_wn_parent, WILDDOG_NODE_TYPE_NULL);
            _wilddog_node_setKey(p_head->p_wn_parent ,NULL);
            wilddog_node_setValue(p_head->p_wn_parent,NULL, 0);
            p_head->p_wn_parent->p_wn_child = NULL;
        }
    }
DEL_FREE:
    p_head->p_wn_prev = NULL;
    p_head->p_wn_next = NULL;
    p_head->p_wn_parent = NULL;
    _wilddog_node_free(p_head);
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    wilddog_node_clone
 * Description: clone the node and it's all child node.
 * Input:       node:   The pointer to the head.
 * Output:      N/A
 * Return:      If clone success, return the pointer of the node copy.
 * Others:      N/A
*/
Wilddog_Node_T * WD_SYSTEM wilddog_node_clone(const Wilddog_Node_T *node)
{
    Wilddog_Node_T *p_snapshot;
    Wilddog_Node_T *tmp;
    int len;
    if(!node)
        return NULL;
    p_snapshot = _wilddog_node_new();
    {
        
        if(p_snapshot == NULL)
        {
            wilddog_debug_level( WD_DEBUG_ERROR, \
                "could not clone the node");

            return NULL;
        }

        if(node->p_wn_key != NULL)
        {
            len = strlen((const char *)node->p_wn_key);
            p_snapshot->p_wn_key = wmalloc(len + 1);
            if(p_snapshot->p_wn_key == NULL)
                return NULL;
            memcpy(p_snapshot->p_wn_key, node->p_wn_key, len);
            p_snapshot->d_wn_len = node->d_wn_len;
        }

        if(node->p_wn_value != NULL)
        {
            len = node->d_wn_len; 
            p_snapshot->p_wn_value = wmalloc(len + 1);
            if(p_snapshot->p_wn_value == NULL)
                return NULL;
            memcpy(p_snapshot->p_wn_value, node->p_wn_value, len);
            p_snapshot->d_wn_len = node->d_wn_len;
        }

        p_snapshot->d_wn_type = node->d_wn_type;
        if(node->p_wn_child)
        {
            tmp =  wilddog_node_clone(node->p_wn_child);
            p_snapshot->p_wn_child = tmp;
            tmp->p_wn_parent = p_snapshot;
        }
        if(node->p_wn_next)
        {
            tmp = wilddog_node_clone(node->p_wn_next);
            p_snapshot->p_wn_next = tmp;
            tmp->p_wn_prev = p_snapshot;
        }
        return p_snapshot;  
    }
}

