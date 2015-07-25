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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utlist.h"
#include "wilddog_api.h"
#include "wilddog_event.h"
#include "wilddog_conn.h"
#include "wilddog_common.h"

/*
 * Function:    _wilddog_event_nodeInit
 * Description: Init an event node.
 * Input:       N/A
 * Output:      N/A
 * Return:      Pointer to the node.
*/
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
    head->p_onData = NULL;
    head->p_dataArg = NULL;

    return head;
}

/*
 * Function:    _wilddog_event_nodeFree
 * Description: Free an event node.
 * Input:       node: The pointer to node which want free.
 * Output:      N/A
 * Return:      N/A
*/
STATIC void _wilddog_event_nodeFree(Wilddog_EventNode_T *node)
{
    if(node->path != NULL)
    {
        wfree(node->path);
    }
    wfree(node);
}

/*
 * Function:    _wilddog_event_nodeDeinit
 * Description: Free an event node tree.
 * Input:       node: The pointer to node head which want free.
 * Output:      N/A
 * Return:      N/A
*/
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
STATIC u8 _wilddog_event_pathContain( char *spath, char *dpath)
{
    int n;
    int slen=0, dlen = 0;
    
    wilddog_assert(spath, 2);
    wilddog_assert(dpath, 2);
    slen = strlen((const char *)spath);
    dlen = strlen((const char *)dpath);
    n= (slen < dlen ? slen : dlen);
    if(slen == dlen && 0 == strncmp(spath,dpath,n))
    {
        return  2;
    }
    if((0 == strncmp(spath,dpath,n)) && (n == dlen))
    {
        return 1;
    }
    if((0 == strncmp(spath,dpath,n)) && (n == slen))
    {
        return 0;
    }
    
    return 3;
}

/*
 * Function:    _wilddog_event_pathRelative
 * Description: find the relative path of dpath from spath.
 * Input:       spath: The pointer to source path.
 *              dpath: The pointer to destination path.
 * Output:      N/A
 * Return:      if spath is "/a/b/c"  dpath is "/a/b/c/d/e", return "/d/e.
*/
char * _wilddog_event_pathRelative( char *spath, char *dpath)
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
 * Return:      if spath is "/a/b/c"  dpath is "/a/b/c/d/e", return "/d/e.
*/
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
    
    /*TODO: use the type to check which event can be triggered */
    while(enode)
    {
        flag = 0;
        p_str = NULL;
        if(
            _wilddog_event_pathContain( enode->path, \
                            (char *)((Wilddog_Url_T *)arg)->p_url_path) == 1
            )
        {
            /*  if could not find the node, new a null node.*/
            
            p_str = (Wilddog_Str_T *)_wilddog_event_pathRelative( \
            (char*)((Wilddog_Url_T *)arg)->p_url_path, enode->path);
            
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
 * Description: The event handler, called by connectivity layer.
 * Input:       node: The pointer to data node tree.
 *              arg: The pointer to data url.
 *              err: error code.
 * Output:      N/A
 * Return:      if spath is "/a/b/c"  dpath is "/a/b/c/d/e", return "/d/e.
*/
Wilddog_Return_T _wilddog_event_nodeAdd
    ( 
    Wilddog_Event_T *event, 
    Wilddog_EventType_T type, 
    Wilddog_ConnCmd_Arg_T *arg
    )
{
    Wilddog_EventNode_T *node, *tmp_node = NULL, *prev_tmp_node = NULL;
    Wilddog_EventNode_T *head;
    Wilddog_Str_T *tmp;
    Wilddog_Conn_T *p_conn = event->p_ev_store->p_se_repo->p_rp_conn;

    head = event->p_head;

    {
        node = _wilddog_event_nodeInit(); 
        if(node == NULL)
            return WILDDOG_ERR_NULL;
 
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

        wilddog_debug_level(WD_DEBUG_LOG, "event node path:%s\n", node->path);

        node->p_onData = arg->p_complete;
        node->p_dataArg = arg->p_completeArg;
        node->flag = OFF_FLAG;

		tmp_node = head;
		prev_tmp_node = head;
		while(tmp_node)
		{
			int slen = 0, dlen = 0, len = 0;
			slen = strlen((const char *)tmp_node->path);
			dlen = strlen((const char *)node->path);

			len = (slen < dlen ? slen : dlen);
			if((strncmp(tmp_node->path, node->path, len) < 0 ) || ((strncmp(tmp_node->path, node->path, len) == 0) && (slen < dlen)))
			{
			    prev_tmp_node = tmp_node;
				tmp_node = tmp_node->next;	
			}
			else
			{
				break;
			}
		}
		if(prev_tmp_node == tmp_node)
		{
		    node->next = tmp_node;
		    event->p_head = node;
		}
		else
		{
		    prev_tmp_node->next = node;
		    node->next = tmp_node;
		}
		

    }
    arg->p_complete= (Wilddog_Func_T)_wilddog_event_trigger;
    arg->p_completeArg= arg->p_url;
		
    head= event->p_head;
    while(head)
    {
        if(
            _wilddog_event_pathContain(head->path, \
                (char*)arg->p_url->p_url_path) == 1
            )
        {
        	if(head->flag == ON_FLAG)
        	{
	            if(p_conn && p_conn->f_conn_send)
	            {
	                tmp = arg->p_url->p_url_path;
	                arg->p_url->p_url_path = (Wilddog_Str_T*)head->path;
	                p_conn->f_conn_send(WILDDOG_CONN_CMD_OFF, \
	                            event->p_ev_store->p_se_repo,arg);
	                wilddog_debug_level(WD_DEBUG_LOG, "off path:%s", arg->p_url->p_url_path);
	                head->flag == OFF_FLAG;
	                arg->p_url->p_url_path = tmp;
	            }
	        }
            head = head->next;
        }
        else if(
                _wilddog_event_pathContain(head->path, \
                    (char*)arg->p_url->p_url_path)==0
            )
        {
        	wilddog_debug_level(WD_DEBUG_LOG, "don't send");
            if(p_conn && p_conn->f_conn_send)
            {
                /*don't send oberve on*/
            }
            break;
                        
        }
        else if(
                _wilddog_event_pathContain(head->path, \
                    (char*)arg->p_url->p_url_path)==2
            )
        {
        	if(head->flag == OFF_FLAG)
        	{
        		wilddog_debug_level(WD_DEBUG_LOG, "send the on path:%s", arg->p_url->p_url_path);
        		head->flag = ON_FLAG;
                p_conn->f_conn_send(WILDDOG_CONN_CMD_ON, \
                            event->p_ev_store->p_se_repo,arg);
        	}
            head = head->next;
        }
        else
        {

        	head = head->next;
        }
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
STATIC Wilddog_EventNode_T * _wilddog_event_nodeFind
    ( 
    Wilddog_EventNode_T *head, 
    char *path
    )
{
    Wilddog_EventNode_T *node, *tmp;
    
    wilddog_assert(path, NULL);

    LL_FOREACH_SAFE(head,node,tmp) 
    {
        if(node->path)
        {
            if(!memcmp(node->path, path, strlen((const char *)path)))
            {
                return node;
            }
        }
    }
    
    return NULL;
}

/*
 * Function:    _wilddog_event_nodeDelete
 * Description: Delete an event.
 * Input:       event: The pointer to event structure.
 *              type: The event type.
 *              arg: the args.
 * Output:      N/A
 * Return:      If success, return 0.
*/
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
	
    if(node == NULL)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "node is NULL!");
        return WILDDOG_ERR_INVALID;
    }
    if(p_conn && p_conn->f_conn_send)
        err =  p_conn->f_conn_send(WILDDOG_CONN_CMD_OFF, \
                            event->p_ev_store->p_se_repo,arg);
    if(err == WILDDOG_ERR_NOERR)
    {
        LL_DELETE(head, node);
        if(event->p_head != head)
        	_wilddog_event_nodeFree(event->p_head);
        else
            _wilddog_event_nodeFree(node);
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
Wilddog_Event_T* _wilddog_event_deinit(Wilddog_Store_T *p_store)
{
    _wilddog_event_nodeDeinit(p_store->p_se_event->p_head);
    wfree(p_store->p_se_event);

    return (Wilddog_Event_T*)NULL;
}


