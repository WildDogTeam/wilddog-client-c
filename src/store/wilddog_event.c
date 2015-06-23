
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wilddog_api.h"
#include "wilddog_event.h"
#include "wilddog_conn.h"
#include "wilddog_common.h"

STATIC Wilddog_EventNode_T * _wilddog_event_nodeInit()
{
    Wilddog_EventNode_T *head;

    head = (Wilddog_EventNode_T *)wmalloc(sizeof(Wilddog_EventNode_T));
    if(head == NULL)
    {
        wilddog_debug_level( WD_DEBUG_ERROR, \
            "wmalloc event head error");

        return NULL;
    }

    head->path = NULL;
    head->next = NULL;
    head->prev = NULL;
    head->p_onData = NULL;
    head->p_dataArg = NULL;

    return head;
}

STATIC void _wilddog_event_nodeFree(Wilddog_EventNode_T *node)
{

    if(node->path != NULL)
    {
        wfree(node->path);
    }
    wfree(node);
}

STATIC void _wilddog_event_nodeDeinit(Wilddog_EventNode_T *head)
{
    Wilddog_EventNode_T *node, *next;
    node = head;
    
    while(node)
    {
        next = node->next;
        _wilddog_event_nodeFree(node);
        node = next;     
    }

}
/*
 * if spath contains dpath, return 1;
 * if dpath contains spath, return 0;
 * else return 2;
 */
STATIC u8 _wilddog_event_pathContain( char *spath, char *dpath)
{
    int n;
    int slen=0, dlen = 0;
    wilddog_assert(spath);
    wilddog_assert(dpath);
    slen = strlen((const char *)spath);
    dlen = strlen((const char *)dpath);
    n= (slen < dlen ? slen : dlen);
    if(slen == dlen && 0 == strncmp(spath,dpath,n))
    {
        return  1;
    }
    if((0 == strncmp(spath,dpath,n)) && (n == dlen))
    {
        return 1;
    }
    if((0 == strncmp(spath,dpath,n)) && (n == dlen))
    {
        return 0;
    }
    return 2;

}

/*
 * obtain the relative path
 * if spath is "/a/b/c"  dpath is "/a/b/c/d/e", return "/d/e" 
 */
char * _wilddog_event_pathRelative( char *spath, char *dpath)
{

    int slength,dlength;
    wilddog_assert(spath);
    wilddog_assert(dpath);
    
    slength = strlen((const char *)spath);
    
    dlength = strlen((const char *)dpath);

    if(slength == dlength)
        return "/";
    else
        return (dpath + slength );
}

void _wilddog_event_trigger
    ( 
    Wilddog_Node_T *node, 
    void *arg, 
    Wilddog_Return_T err
    )
{
    Wilddog_EventNode_T *enode = NULL;
    Wilddog_Repo_T *repo;
    Wilddog_Node_T *obj_node, *obj_node_prev, *obj_node_next;
    u8 flag;
    Wilddog_Str_T *p_str ;
    flag = 0;
    obj_node_prev = NULL;
    obj_node_next = NULL;
    repo = (Wilddog_Repo_T *)_wilddog_ct_findRepo( \
        ((Wilddog_Url_T *)arg)->p_url_host);
    if(repo == NULL)
    {
        wilddog_debug_level( WD_DEBUG_ERROR, "could not find repo!");

        return;
    }

    enode = repo->p_rp_store->p_se_event->p_head;

    while(enode)
    {
        flag = 0;
        p_str = NULL;
        if(
            _wilddog_event_pathContain( enode->path, \
                            (char *)((Wilddog_Url_T *)arg)->p_url_path) == 1
            )
        {
            //  if could not find the node, new a null node.
            
            p_str = (Wilddog_Str_T *)_wilddog_event_pathRelative( \
            (char*)((Wilddog_Url_T *)arg)->p_url_path, enode->path);
            
            obj_node = wilddog_node_find(node, (char*)p_str);
            if(obj_node == NULL)
            {
                wilddog_debug_level( WD_DEBUG_LOG, "new a null node!");

                flag = 1;
                obj_node = wilddog_node_createNull(NULL);
            }
            // store the prev and next, make the temp node as the head
            
            if(obj_node->p_wn_prev != NULL)
            {
                obj_node_prev = obj_node->p_wn_prev;
                obj_node->p_wn_prev = NULL;
            }
            if(obj_node->p_wn_next != NULL)
            {
                obj_node_next = obj_node->p_wn_next;
                obj_node->p_wn_next = NULL;
            }
            
            enode->p_onData( obj_node, enode->p_dataArg, err);

            if(obj_node_prev != NULL)
            {
                obj_node->p_wn_prev = obj_node_prev;
            }
            if(obj_node_next != NULL)
            {
                obj_node->p_wn_next = obj_node_next;
            }
            if(flag)
                wilddog_node_delete(obj_node);

        }

        enode = enode->next;
    }

}

Wilddog_Return_T _wilddog_event_nodeAdd
    ( 
    Wilddog_Event_T *event, 
    Wilddog_EventType_T type, 
    Wilddog_ConnCmd_Arg_T *arg
    )
{
    Wilddog_EventNode_T *node;
    Wilddog_EventNode_T *head;
    Wilddog_Str_T *tmp;
    Wilddog_Conn_T *p_conn = event->p_ev_store->p_se_repo->p_rp_conn;

    head = event->p_head;
    if(head->path == NULL)
    {
        head->path = (char *)wmalloc( \
                                strlen((const char *)arg->p_url->p_url_path)+1
                                );
        
        if(head->path == NULL)
        {
            wilddog_debug_level( WD_DEBUG_ERROR, \
                "cannot wmalloc event head path!");

            return WILDDOG_ERR_NULL;
        }
        memcpy( head->path, arg->p_url->p_url_path,\
            strlen((const char *)arg->p_url->p_url_path));
        
        
        head->p_onData = arg->p_complete;
        head->p_dataArg = arg->p_completeArg;
        event->p_head = head;

    }
    else
    {
        node = (Wilddog_EventNode_T *)wmalloc(sizeof(Wilddog_EventNode_T));
        if(node == NULL)
        {
            wilddog_debug_level( WD_DEBUG_ERROR, "cannot wmalloc event node!");

            return WILDDOG_ERR_NULL;
        }
        
        node->path = (char *)wmalloc(strlen( \
            (const char *)arg->p_url->p_url_path)+1);
        if(node->path == NULL)
        {
            wilddog_debug_level( WD_DEBUG_ERROR, \
                "cannot wmalloc event node path!");

            return WILDDOG_ERR_NULL;
        }
        
        memcpy( node->path, arg->p_url->p_url_path, \
            strlen((const char *)arg->p_url->p_url_path));

        node->p_onData = arg->p_complete;
        node->p_dataArg = arg->p_completeArg;
        node->next = head;
        node->prev = NULL;
        head->prev = node;

        event->p_head = node;

    }
    arg->p_complete= (Wilddog_Func_T)_wilddog_event_trigger;
    arg->p_completeArg= arg->p_url;

    head= event->p_head;
    head = head->next;
    while(head)
    {
    
        if(
            _wilddog_event_pathContain(head->path, \
                (char*)arg->p_url->p_url_path) == 1
            )
        {
            if(p_conn && p_conn->f_conn_send)
            {
                p_conn->f_conn_send(WILDDOG_CONN_CMD_ON, \
                            event->p_ev_store->p_se_repo,arg);
                
                tmp = arg->p_url->p_url_path;
                arg->p_url->p_url_path = (Wilddog_Str_T*)head->path;
                p_conn->f_conn_send(WILDDOG_CONN_CMD_OFF, \
                            event->p_ev_store->p_se_repo,arg);
                
                arg->p_url->p_url_path = tmp;
            }
            return WILDDOG_ERR_NOERR;
        }
        else if(
                _wilddog_event_pathContain(head->path, \
                    (char*)arg->p_url->p_url_path)==0
            )
        {
            if(p_conn && p_conn->f_conn_send)
            {
                //don't send oberve on
                
            }
            return WILDDOG_ERR_NOERR;
        }

        head = head->next;
    }

    if(p_conn && p_conn->f_conn_send)
        return p_conn->f_conn_send(WILDDOG_CONN_CMD_ON, \
                            event->p_ev_store->p_se_repo,arg);

    return WILDDOG_ERR_NOERR;
}



STATIC Wilddog_EventNode_T * _wilddog_event_nodeFind
    ( 
    Wilddog_EventNode_T *head, 
    char *path
    )
{
    Wilddog_EventNode_T *node;
    node = head;
    wilddog_assert(path);

    while(node->next != node)
    {
        if(!memcmp(node->path, path, strlen((const char *)path)))
        {
            return node;
        }
        node = node->next;
    }
    
    return NULL;
}


Wilddog_Return_T _wilddog_event_nodeDelete
    ( 
    Wilddog_Event_T *event,
    Wilddog_EventType_T type,
    Wilddog_ConnCmd_Arg_T *arg
    ) 
{
    Wilddog_Return_T err = WILDDOG_ERR_NOERR;
    Wilddog_EventNode_T *node;
    Wilddog_EventNode_T *head;
    Wilddog_Conn_T *p_conn = event->p_ev_store->p_se_repo->p_rp_conn;
    
    head = event->p_head;

    node = _wilddog_event_nodeFind( head, (char*)arg->p_url->p_url_path);

    if(p_conn && p_conn->f_conn_send)
        err =  p_conn->f_conn_send(WILDDOG_CONN_CMD_OFF, \
                            event->p_ev_store->p_se_repo,arg);

    if(err == WILDDOG_ERR_NOERR)
    {
        if(node == NULL)
            return WILDDOG_ERR_INVALID;
        else
        {
            if(node->prev == NULL)
            {
                if(node->next != NULL)
                {
                    node = node->next;
                    node->prev = NULL;
                    event->p_head = node;
                    _wilddog_event_nodeFree(head);
                }
                else
                {   
                    head->next = NULL;
                    head->prev = NULL;
                    head->p_onData = NULL;
                    _wilddog_event_nodeFree(head);
                    event->p_head = NULL;
                }
            }
            else if(node->next == NULL)
            {
                node->prev->next = NULL;
                _wilddog_event_nodeFree(node);
            }
            else
            {
                node->prev->next = node->next;
                node->next->prev = node->prev;
                _wilddog_event_nodeFree(node);
            }       
            
        }

    }
    else 
    {
        return WILDDOG_ERR_INVALID;
    }
    return WILDDOG_ERR_INVALID;
}

Wilddog_Event_T* _wilddog_event_init(Wilddog_Store_T *p_store)
{
    Wilddog_Event_T *p_event;
    p_event = (Wilddog_Event_T *)wmalloc(sizeof(Wilddog_Event_T));
    if(p_event == NULL)
    {
        wilddog_debug_level( WD_DEBUG_ERROR, "cannot wmalloc Wilddog_Event_T");

        return NULL;
    }

    p_event->p_ev_store= p_store;
    p_event->p_head = _wilddog_event_nodeInit(); 
    p_event->p_ev_cb_on = _wilddog_event_nodeAdd;
    p_event->p_ev_cb_off = _wilddog_event_nodeDelete;

    return p_event;
}

Wilddog_Event_T* _wilddog_event_deinit(Wilddog_Store_T *p_store)
{

    _wilddog_event_nodeDeinit(p_store->p_se_event->p_head);
    wfree(p_store->p_se_event);

    return (Wilddog_Event_T*)NULL;
}


