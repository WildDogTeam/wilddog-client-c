
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 
#include "wilddog_api.h"
#include "wilddog_debug.h"
#include "wilddog_common.h"

#define WILDDOG_KEY_MAX_LEN 768

Wilddog_Node_T *_wilddog_node_new()
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

STATIC BOOL _isKeyValid(Wilddog_Str_T * key)
{
    int len, i;

    if(NULL == key)
        return TRUE;
    len = strlen((const char*)key);

    if(len > WILDDOG_KEY_MAX_LEN)
        return FALSE;
    if(len == 1 && key[0] == '/')
        return TRUE;
    for(i = 0; i < len; i++)
    {
        if(key[i] < 32 || \
            key[i] == '.' || \
            key[i] == '$' || \
            key[i] == '#' || \
            key[i] == '[' || \
            key[i] == ']' || \
            key[i] == '/' || \
            key[i] ==  127)
            return FALSE;
    }
    return TRUE;
}

STATIC Wilddog_Node_T * _wilddog_node_newWithStr
    (
    Wilddog_Str_T* key, 
    Wilddog_Node_T** p_node
    )
{
    int i, pos;
    Wilddog_Node_T * p_head = NULL, *p_tmp = NULL, *p_parent = NULL;
    Wilddog_Str_T * p_tmpStr = NULL;
    int length = 0;
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
    if(_isKeyValid(key) == FALSE)
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
            wilddog_node_add(p_parent, p_tmp);
            i += pos;
        }
    }
    wfree(p_tmpStr);
    *p_node = p_tmp;
    return p_head;
}

Wilddog_Node_T * wilddog_node_createFalse(Wilddog_Str_T* key)
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

Wilddog_Node_T * wilddog_node_createTrue(Wilddog_Str_T* key)
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

Wilddog_Node_T * wilddog_node_createNull(Wilddog_Str_T* key)
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

Wilddog_Node_T * wilddog_node_createNum
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

Wilddog_Node_T * wilddog_node_createFloat
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

Wilddog_Node_T * wilddog_node_createBString
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

Wilddog_Node_T * wilddog_node_createUString
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

Wilddog_Node_T * wilddog_node_createObject
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

Wilddog_Return_T wilddog_node_setKey
    (
    Wilddog_Node_T *node, 
    Wilddog_Str_T *key
    )
{
    int len;

    if(NULL == node)
        return WILDDOG_ERR_INVALID;
    
    if(FALSE == _isKeyValid(key))
        return WILDDOG_ERR_INVALID;
    
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

const Wilddog_Str_T *wilddog_node_getKey(Wilddog_Node_T *node)
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

void wilddog_node_setType(Wilddog_Node_T *node, u8 type)
{
    if(NULL == node)
        return;
        
    node->d_wn_type = type;
    return;
}

u8 wilddog_node_getType(Wilddog_Node_T *node)
{
    if(NULL == node)
        return WILDDOG_ERR_INVALID;
        
    return node->d_wn_type;
}

Wilddog_Return_T wilddog_node_setValue
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
			WILDDOG_NODE_TYPE_NUM > node->d_wn_type 	|| \
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

Wilddog_Str_T* wilddog_node_getValue
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

void wilddog_node_free(Wilddog_Node_T *node)
{
    wilddog_assert(node);
    
    if(node->p_wn_child != NULL)
    {
        wilddog_node_free(node->p_wn_child);
    }   
    if(node->p_wn_next != NULL)
    {
        wilddog_node_free(node->p_wn_next);
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
}

STATIC Wilddog_Node_T *_wilddog_node_findInner
    (
    Wilddog_Node_T *node, 
    char *path
    )
{
    int len ;
    wilddog_assert(path);
    wilddog_assert(node);
    
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
 *  The path must in "/a/b" format
 *  The root node has a path key '/'
 */
Wilddog_Node_T *wilddog_node_find
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


    if((*path) != '/')
    {
        return NULL;
    }
    
    if(root->p_wn_child != NULL)
        node = root->p_wn_child;

    path++;     /* remove the first '/' */
        
    return _wilddog_node_findInner(node, path);

}

/*
 * add newnode as node's child
 * if you want to add newnode as node's brother, use the parent node
 *
 */
Wilddog_Return_T wilddog_node_add
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
        while(first_child->p_wn_next != NULL)
            first_child = first_child->p_wn_next;

        first_child->p_wn_next = newnode;
        newnode->p_wn_prev = first_child;
        newnode->p_wn_parent = node;
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

Wilddog_Return_T wilddog_node_delete( Wilddog_Node_T *p_node)
{
    Wilddog_Node_T * p_head = NULL;

    if(NULL == p_node)
        return WILDDOG_ERR_NULL;
    p_head = p_node;

    //remove the real node from the tree
    if(NULL == p_head->p_wn_parent)
    {
        goto DEL_FREE;
    }
    else
    {
        if(NULL != p_head->p_wn_next && NULL != p_head->p_wn_prev)
        {
            //have both left and right brother
            p_head->p_wn_next->p_wn_prev = p_head->p_wn_prev;
            p_head->p_wn_prev->p_wn_next = p_head->p_wn_next;

        }
        else if(NULL == p_head->p_wn_next && NULL != p_head->p_wn_prev)
        {
            // have left brother, no right brother
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
            //set it's parent as null node
            wilddog_node_setType(p_head->p_wn_parent, WILDDOG_NODE_TYPE_NULL);
            wilddog_node_setKey(p_head->p_wn_parent ,NULL);
            wilddog_node_setValue(p_head->p_wn_parent,NULL, 0);
            p_head->p_wn_parent->p_wn_child = NULL;
        }
    }
DEL_FREE:
    p_head->p_wn_prev = NULL;
    p_head->p_wn_next = NULL;
    p_head->p_wn_parent = NULL;
    wilddog_node_free(p_head);
    return WILDDOG_ERR_NOERR;
}

Wilddog_Node_T * wilddog_node_clone(const Wilddog_Node_T *node)
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


