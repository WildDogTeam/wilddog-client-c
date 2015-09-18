/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_event.c
 *
 * Description: event functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        baikal.Hu       2015-05-15  Create file.
 * 0.4.3        Jimmy.Pan       2015-07-04  Add annotation.
 *
 */
#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "utlist.h"
#include "wilddog_api.h"
#include "wilddog_event.h"
#include "wilddog_conn.h"
#include "wilddog_common.h"
#include "wilddog_url_parser.h"
/*
 * Function:    _wilddog_event_nodeInit
 * Description: Init an event node.
 * Input:       N/A
 * Output:      N/A
 * Return:      Pointer to the node.
*/
STATIC Wilddog_EventNode_T * WD_SYSTEM _wilddog_event_nodeInit(void)
{
    Wilddog_EventNode_T *head;

    head = (Wilddog_EventNode_T *)wmalloc(sizeof(Wilddog_EventNode_T));
    if(head == NULL)
    {
        wilddog_debug_level( WD_DEBUG_ERROR, \
            "wmalloc event head error");

        return NULL;
    }

    head->p_url = (Wilddog_Url_T *)wmalloc(sizeof(Wilddog_Url_T));
    if(head->p_url == NULL)
    {
        wilddog_debug_level( WD_DEBUG_ERROR, \
            "wmalloc event head p_url error");

        return NULL;
    }
    head->next = NULL;
    head->p_onData = NULL;
    head->p_dataArg = NULL;
    head->flag = OFF_FLAG;

    return head;
}

/*
 * Function:    _wilddog_event_nodeFree
 * Description: Free an event node.
 * Input:       node: The pointer to node which want free.
 * Output:      N/A
 * Return:      N/A
*/
STATIC void WD_SYSTEM _wilddog_event_nodeFree
    (
    Wilddog_EventNode_T *node
    )
{
    _wilddog_url_freeParsedUrl(node->p_url);
    wfree(node);
}

/*
 * Function:    _wilddog_event_nodeDeinit
 * Description: Free an event node tree.
 * Input:       node: The pointer to node head which want free.
 * Output:      N/A
 * Return:      N/A
*/
STATIC void WD_SYSTEM _wilddog_event_nodeDeinit
    (
    Wilddog_EventNode_T *head
    )
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
 * Function:    _wilddog_event_pathContain
 * Description: Check the two path's relationship.
 * Input:       spath: The pointer to source path.
 *              dpath: The pointer to destination path.
 * Output:      N/A
 * Return:      if dpath contains spath, return 0, 
 *              if spath contains dpath, return 1,
 *              if spath equal dpath, return 2
 *              else return 3.
*/
STATIC u8 WD_SYSTEM _wilddog_event_pathContain
    (
    char *spath, 
    char *dpath
    )
{
    int n;
    int slen=0, dlen = 0;
    
    wilddog_assert(spath, WD_EVENT_PATHCONTAIN_OTHER);
    wilddog_assert(dpath, WD_EVENT_PATHCONTAIN_OTHER);
    
    slen = strlen((const char *)spath);
    dlen = strlen((const char *)dpath);
    n= (slen < dlen ? slen : dlen);
    if(slen == dlen && 0 == strncmp(spath,dpath,n))
    {
        return  WD_EVENT_PATHCONTAIN_SED;
    }
    if((0 == strncmp(spath,dpath,n)) && (n == dlen))
    {
        return WD_EVENT_PATHCONTAIN_SCD;
    }
    if((0 == strncmp(spath,dpath,n)) && (n == slen))
    {
        return WD_EVENT_PATHCONTAIN_DCS;
    }
    
    return WD_EVENT_PATHCONTAIN_OTHER;
}

/*
 * Function:    _wilddog_event_pathRelative
 * Description: find the relative path of dpath from spath.
 * Input:       spath: The pointer to source path.
 *              dpath: The pointer to destination path.
 * Output:      N/A
 * Return:      if spath is "/a/b/c"  dpath is "/a/b/c/d/e", return "/d/e.
*/
char * WD_SYSTEM _wilddog_event_pathRelative
    ( 
    char *spath, 
    char *dpath
    )
{
    int slength,dlength;
    
    wilddog_assert(spath, NULL);
    wilddog_assert(dpath, NULL);
    
    slength = strlen((const char *)spath);
    
    dlength = strlen((const char *)dpath);

    if(slength == dlength)
        return "/";
    else
        return (dpath + slength );
}

/*
 * Function:    _wilddog_event_trigger
 * Description: The event handler, called by connectivity layer.
 * Input:       node: The pointer to data node tree.
 *              arg: The pointer to data url.
 *              err: error code.
 * Output:      N/A
 * Return:      N/A
*/
void WD_SYSTEM _wilddog_event_trigger
    ( 
    Wilddog_Node_T *node, 
    void *arg, 
    Wilddog_Return_T err
    )
{
    Wilddog_EventNode_T *enode = NULL;
    Wilddog_Repo_T *repo;
    Wilddog_Node_T *obj_node = NULL, *obj_node_prev = NULL;
    Wilddog_Node_T *obj_node_next = NULL;
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
    
    /*TODO: use the type to check which event can be triggered */
    while(enode)
    {
        u8 pathContainResult = 0;
        flag = 0;
        p_str = NULL;
        pathContainResult = _wilddog_event_pathContain( \
                                (char *)enode->p_url->p_url_path, \
                                (char *)((Wilddog_Url_T *)arg)->p_url_path);
        
        if( (pathContainResult == WD_EVENT_PATHCONTAIN_SCD)  || \
            (pathContainResult == WD_EVENT_PATHCONTAIN_SED))
        {
            /*  if could not find the node, new a null node.*/
            
            p_str = (Wilddog_Str_T *)_wilddog_event_pathRelative( \
                                    (char*)((Wilddog_Url_T *)arg)->p_url_path, \
                                    (char *)enode->p_url->p_url_path);
            
            obj_node = wilddog_node_find(node, (char*)p_str);
            if(obj_node == NULL)
            {
                wilddog_debug_level( WD_DEBUG_LOG, "new a null node!");

                flag = 1;
                obj_node = wilddog_node_createNull(NULL);
            }
            /* store the prev and next, make the temp node as the head*/
            
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

 
/*
 * Function:    _wilddog_event_nodeAdd
 * Description: The event add handler, called by connectivity layer.
 * Input:       event: The pointer to Wilddog_Event_T.
 *              type: The event type.
 *              arg: Wilddog_ConnCmd_Arg_T.
 * Output:      N/A
 * Return:      success: WILDDOG_ERR_NOERR ; fail:WILDDOG_ERR_NULL
*/
Wilddog_Return_T WD_SYSTEM _wilddog_event_nodeAdd
    ( 
    Wilddog_Event_T *event, 
    Wilddog_EventType_T type, 
    Wilddog_ConnCmd_Arg_T *arg
    )
{
    Wilddog_Return_T err = WILDDOG_ERR_NOERR;
    
    Wilddog_EventNode_T *node = NULL, *tmp_node = NULL, *prev_node = NULL;
    Wilddog_EventNode_T *head = NULL;
    Wilddog_Str_T *tmp;
    Wilddog_Conn_T *p_conn = event->p_ev_store->p_se_repo->p_rp_conn;
    u32 len = 0;

    head = event->p_head;
    
    if(arg->p_url)
    {
        if(arg->p_url->p_url_path && arg->p_url->p_url_host)
        {
            node = _wilddog_event_nodeInit(); 
            if(node == NULL)
                return WILDDOG_ERR_NULL;
        
            len = strlen((const char *)arg->p_url->p_url_path);
            node->p_url->p_url_path= (Wilddog_Str_T *)wmalloc(len + 1);
            
            if(node->p_url->p_url_path == NULL)
            {
                wilddog_debug_level( WD_DEBUG_ERROR, \
                    "cannot wmalloc event node path!");
                _wilddog_event_nodeFree(node);
                return WILDDOG_ERR_NULL;
            }

            memcpy( node->p_url->p_url_path, arg->p_url->p_url_path, len);
    
            len = strlen((const char *)arg->p_url->p_url_host);
            node->p_url->p_url_host= (Wilddog_Str_T *)wmalloc(len + 1);
            if(node->p_url->p_url_host == NULL)
            {
                wilddog_debug_level( WD_DEBUG_ERROR, \
                    "cannot wmalloc event node host!");
                _wilddog_event_nodeFree(node);
                return WILDDOG_ERR_NULL;
            }
            
            memcpy( node->p_url->p_url_host, arg->p_url->p_url_host, len);
        }
        else
        {
            return WILDDOG_ERR_NULL;
        }

    }
    else
    {
        return WILDDOG_ERR_NULL;
    }

    wilddog_debug_level(WD_DEBUG_LOG, "event node path:%s\n", \
                        node->p_url->p_url_path);

    node->p_onData = arg->p_complete;
    node->p_dataArg = arg->p_completeArg;
    node->flag = OFF_FLAG;

    tmp_node = head;
    prev_node = head;
    while(tmp_node)
    {
        int slen = 0, dlen = 0, len = 0;
        int cmpResult = 0;
        slen = strlen((const char *)tmp_node->p_url->p_url_path);
        dlen = strlen((const char *)node->p_url->p_url_path);

        len = (slen < dlen ? slen : dlen);
        cmpResult = strncmp((const char*)tmp_node->p_url->p_url_path, \
                            (const char*) node->p_url->p_url_path, len);
        if((cmpResult < 0 ) || ((cmpResult == 0) && (slen < dlen)))
        {
            prev_node = tmp_node;
            tmp_node = tmp_node->next;  
        }
        else if( ((cmpResult == 0) && (slen == dlen)) )
        {
            tmp_node->p_onData = arg->p_complete;
            tmp_node->p_dataArg = arg->p_completeArg;
            _wilddog_event_nodeFree(node);
            wilddog_debug_level(WD_DEBUG_WARN, "cover the old path %s%s", \
                                tmp_node->p_url->p_url_host, \
                                tmp_node->p_url->p_url_path);
            return WILDDOG_ERR_NOERR;
        }
        else
        {
            break;
        }
    }
    if(prev_node == tmp_node)
    {
        node->next = tmp_node;
        event->p_head = node;
    }
    else
    {
        prev_node->next = node;
        node->next = tmp_node;
    }

    arg->p_complete= (Wilddog_Func_T)_wilddog_event_trigger;
    arg->p_completeArg= arg->p_url;
        
    head= event->p_head;
    while(head)
    {
        u8 pathContainResult = _wilddog_event_pathContain(\
                                       (char*)head->p_url->p_url_path, \
                                       (char*)arg->p_url->p_url_path);

        if(pathContainResult == WD_EVENT_PATHCONTAIN_SCD)
        {
            if(head->flag == ON_FLAG)
            {
                if(p_conn && p_conn->f_conn_send)
                {
                    tmp = arg->p_url->p_url_path;
                    arg->p_url->p_url_path = (Wilddog_Str_T*)head->p_url->p_url_path;
                    
                    if(p_conn && p_conn->f_conn_send)
                    {
                        err = p_conn->f_conn_send(WILDDOG_CONN_CMD_OFF, \
                                            event->p_ev_store->p_se_repo,arg);
                    }
                    
                    wilddog_debug_level(WD_DEBUG_LOG, "off path:%s", \
                                        arg->p_url->p_url_path);
                    head->flag = OFF_FLAG;
                    arg->p_url->p_url_path = tmp;
                }
            }
            head = head->next;
        }
        else if(pathContainResult == WD_EVENT_PATHCONTAIN_DCS)
        {
            wilddog_debug_level(WD_DEBUG_LOG, "don't send");
            if(p_conn && p_conn->f_conn_send)
            {
                /*don't send oberve on*/
            }
            break;               
        }
        else if(pathContainResult == WD_EVENT_PATHCONTAIN_SED)
        {
            if(head->flag == OFF_FLAG)
            {
                wilddog_debug_level(WD_DEBUG_LOG, "send the on path:%s", \
                                    arg->p_url->p_url_path);
                
                head->flag = ON_FLAG;
                
                if(p_conn && p_conn->f_conn_send)
                {
                    err = p_conn->f_conn_send(WILDDOG_CONN_CMD_ON, \
                                        event->p_ev_store->p_se_repo,arg);
                }
            }
            head = head->next;
        }
        else
        {
            head = head->next;
        }
    }
    
    if(err != WILDDOG_ERR_NOERR)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "send on to server failed!");
        return WILDDOG_ERR_INVALID;
    }
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_event_nodeFind
 * Description: Find the event node by path.
 * Input:       head: The pointer to event node tree.
 *              path: The path.
 * Output:      N/A
 * Return:      If found, return the pointer of the node.
*/
STATIC Wilddog_EventNode_T * WD_SYSTEM _wilddog_event_nodeFind
    ( 
    Wilddog_EventNode_T *head, 
    char *path
    )
{
    Wilddog_EventNode_T *node, *tmp;
    
    wilddog_assert(path, NULL);

    LL_FOREACH_SAFE(head,node,tmp) 
    {
        if(node->p_url->p_url_path)
        {
            if(!memcmp(node->p_url->p_url_path, path, \
                        strlen((const char *)path))
               )
            {
                return node;
            }
        }
    }
    
    return NULL;
}

/*
 * Function:    _wilddog_event_nodeDelete
 * Description: The event delete handler, called by connectivity layer.
 * Input:       event: The pointer to Wilddog_Event_T.
 *              type: The event type.
 *              arg: Wilddog_ConnCmd_Arg_T.
 * Output:      N/A
 * Return:      success: WILDDOG_ERR_NOERR ; fail:WILDDOG_ERR_NULL
*/
Wilddog_Return_T WD_SYSTEM _wilddog_event_nodeDelete
    ( 
    Wilddog_Event_T *event,
    Wilddog_EventType_T type,
    Wilddog_ConnCmd_Arg_T *arg
    ) 
{
    Wilddog_Return_T err = WILDDOG_ERR_NOERR;
    Wilddog_EventNode_T *node = NULL, *prev_node = NULL, *tmp_node = NULL;
    Wilddog_EventNode_T *head = NULL;
    Wilddog_ConnCmd_Arg_T *tmp_arg = NULL;
    u32 len = 0;
    Wilddog_Conn_T *p_conn = event->p_ev_store->p_se_repo->p_rp_conn;
    u8 pathContain = WD_EVENT_PATHCONTAIN_OTHER;
    u8 prePathContain = WD_EVENT_PATHCONTAIN_OTHER;
    head = event->p_head;

    node = _wilddog_event_nodeFind(head, (char*)arg->p_url->p_url_path);
    tmp_node = node;
    
    if(node == NULL)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "node is NULL!");
        return WILDDOG_ERR_INVALID;
    }
    
    if(node->flag == ON_FLAG)
    {
        if(p_conn && p_conn->f_conn_send)
        {
            err =  p_conn->f_conn_send(WILDDOG_CONN_CMD_OFF, \
                                event->p_ev_store->p_se_repo,arg);
        }
        
        wilddog_debug_level(WD_DEBUG_LOG, "nodedelete off node path:%s\n", \
                            arg->p_url->p_url_path);

        prev_node = node;
        node = node->next;
        while(node)
        {
            pathContain = _wilddog_event_pathContain( \
                             (char*)arg->p_url->p_url_path, \
                             (char*)node->p_url->p_url_path);
            prePathContain = _wilddog_event_pathContain( \
                                 (char*)prev_node->p_url->p_url_path, \
                                 (char*)node->p_url->p_url_path);
            
            if(((pathContain == WD_EVENT_PATHCONTAIN_DCS) && \
                (prePathContain == WD_EVENT_PATHCONTAIN_OTHER) \
                ) \
                || \
                ((prev_node == tmp_node) && \
                 (pathContain == WD_EVENT_PATHCONTAIN_DCS))
               )
            {
                if(node->flag == OFF_FLAG)
                {
                    tmp_arg = (Wilddog_ConnCmd_Arg_T *) \
                              wmalloc(sizeof(Wilddog_ConnCmd_Arg_T));
                    if(tmp_arg == NULL)
                    {
                        wilddog_debug_level( WD_DEBUG_ERROR, \
                            "cannot wmalloc tmp arg!");
                    }
                    tmp_arg->p_url = (Wilddog_Url_T *) \
                                     wmalloc(sizeof(Wilddog_Url_T));
                    if(tmp_arg->p_url == NULL)
                    {
                        wilddog_debug_level( WD_DEBUG_ERROR, \
                            "cannot wmalloc tmp arg url!");
                    }
                    len = strlen((const char *)node->p_url->p_url_host);
                    tmp_arg->p_url->p_url_host= (Wilddog_Str_T *) \
                                                wmalloc( len + 1);
                    
                    if(tmp_arg->p_url->p_url_host == NULL)
                    {
                        wilddog_debug_level( WD_DEBUG_ERROR, \
                            "cannot wmalloc event node path!");

                        return WILDDOG_ERR_NULL;
                    }

                    memcpy(tmp_arg->p_url->p_url_host, \
                           node->p_url->p_url_host, len);

                    len = strlen((const char *)node->p_url->p_url_path);
                    tmp_arg->p_url->p_url_path = (Wilddog_Str_T *) \
                                                 wmalloc(len + 1);
                    
                    if(tmp_arg->p_url->p_url_path == NULL)
                    {
                        wilddog_debug_level( WD_DEBUG_ERROR, \
                            "cannot wmalloc event node path!");

                        return WILDDOG_ERR_NULL;
                    }

                    memcpy(tmp_arg->p_url->p_url_path, \
                           node->p_url->p_url_path, len);

                    tmp_arg->p_complete = node->p_onData;
                    tmp_arg->p_completeArg=node->p_dataArg;

                    if(p_conn && p_conn->f_conn_send)
                    {
                        err = p_conn->f_conn_send(WILDDOG_CONN_CMD_ON, \
                                          event->p_ev_store->p_se_repo,tmp_arg);
                    }
                    
                    node->flag = ON_FLAG;                        
                    wilddog_debug_level(WD_DEBUG_LOG, \
                                        "nodedelete on node path:%s\n", \
                                        tmp_arg->p_url->p_url_path);
                    
                    _wilddog_url_freeParsedUrl(tmp_arg->p_url);
                    wfree(tmp_arg);
                }
            }
            prev_node = node;
            node = node->next;
        }
    }
    
    if(err == WILDDOG_ERR_NOERR)
    {
        LL_DELETE(head, tmp_node);

        if(event->p_head != head)
        {
            _wilddog_event_nodeFree(event->p_head);
        }
        else
        {
            _wilddog_event_nodeFree(tmp_node);
        }

        event->p_head = head;
    }
    else 
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "send off to server failed!");
        return WILDDOG_ERR_INVALID;
    }
    
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_event_init
 * Description: Init the event structure.
 * Input:       p_store: The pointer to store structure.
 * Output:      N/A
 * Return:      If success, return the pointer of the structure.
*/
Wilddog_Event_T* WD_SYSTEM _wilddog_event_init(Wilddog_Store_T *p_store)
{
    Wilddog_Event_T *p_event;
    p_event = (Wilddog_Event_T *)wmalloc(sizeof(Wilddog_Event_T));
    if(p_event == NULL)
    {
        wilddog_debug_level( WD_DEBUG_ERROR, "cannot wmalloc Wilddog_Event_T");

        return NULL;
    }

    p_event->p_ev_store= p_store;
    p_event->p_head = NULL;
    p_event->p_ev_cb_on = (Wilddog_Func_T)_wilddog_event_nodeAdd;
    p_event->p_ev_cb_off = (Wilddog_Func_T)_wilddog_event_nodeDelete;

    return p_event;
}
/*
 * Function:    _wilddog_event_deinit
 * Description: Deinit the event structure.
 * Input:       p_store: The pointer to store structure.
 * Output:      N/A
 * Return:      NULL.
*/
Wilddog_Event_T* WD_SYSTEM _wilddog_event_deinit
    (
    Wilddog_Store_T *p_store
    )
{
    _wilddog_event_nodeDeinit(p_store->p_se_event->p_head);
    wfree(p_store->p_se_event);

    return (Wilddog_Event_T*)NULL;
}

