/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_ct.c
 *
 * Description: core, include container functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Jimmy.Pan       2015-05-15  Create file.
 * 0.4.3        Jimmy.Pan       2015-07-04  Add l_isStarted.
 * 0.8.0		Jimmy.Pan		2016-01-20	Add new API.
 *
 */
#ifndef WILDDOG_PORT_TYPE_ESP   
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "wilddog.h"
#include "wilddog_url_parser.h"
#include "wilddog_ct.h"
#include "wilddog_api.h"

#include "wilddog_common.h"
#include "wilddog_store.h"
#include "utlist.h"
#include "wilddog_conn.h"

#define WD_CMD_NORMAL 0
#define WD_CMD_ONDIS  1
#ifdef WILDDOG_ADD_ONLINESTAT
//#define WILDDOG_ONLINE_PATH ".info/connected"
#define WILDDOG_ONLINE_PATH_WITH_SPRIT "/.info/connected"
#endif

/*store the head of all repos.*/
STATIC Wilddog_Repo_Con_T l_wilddog_containTable = {NULL, 0};
/*store the wilddog init status.*/
STATIC BOOL l_isStarted = FALSE;

Wilddog_Repo_T** _wilddog_ct_getRepoHead(void);
STATIC Wilddog_T _wilddog_ct_createRef(void *args, int flag);
STATIC Wilddog_Return_T _wilddog_ct_destroyRef(void *args, int flag);
STATIC Wilddog_Ref_T *_wilddog_ct_findRef
    (
    Wilddog_Repo_T *p_repo, 
    Wilddog_Url_T * p_url
    );
STATIC Wilddog_Ref_T * _wilddog_ct_getRef(void* arg, int flag);
STATIC Wilddog_Repo_T * _wilddog_ct_createRepo(Wilddog_Url_T *p_url);
STATIC Wilddog_Return_T _wilddog_ct_destoryRepo
    (
    Wilddog_Repo_T *p_repo
    );

STATIC Wilddog_Return_T _wilddog_ct_store_setAuth
    (
    void *p_args, 
    int flag
    );
STATIC Wilddog_Return_T _wilddog_ct_store_query
    (
    void *p_args, 
    int flag
    );
STATIC Wilddog_Return_T _wilddog_ct_store_set(void *p_args, int flag);
STATIC Wilddog_Return_T _wilddog_ct_store_push
    (
    void* p_args, 
    int flag
    );
STATIC Wilddog_Return_T _wilddog_ct_store_remove
    (
    void* p_args, 
    int flag
    );
STATIC Wilddog_Return_T _wilddog_ct_store_on(void* p_args, int flag);
STATIC Wilddog_Return_T _wilddog_ct_store_off(void* p_args, int flag);
/*
 * Function:    _wilddog_ct_init
 * Description: Init the container.
 * Input:       not used.
 * Output:      N/A
 * Return:      WILDDOG_ERR_NOERR.
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_init(void* args, int flag)
{
    l_wilddog_containTable.p_rc_head = NULL;
    l_wilddog_containTable.d_rc_online = FALSE;
    l_wilddog_containTable.isForcedOffline = FALSE;
    return WILDDOG_ERR_NOERR;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_deinit(void)
{
    l_wilddog_containTable.d_rc_online = FALSE;
    return WILDDOG_ERR_NOERR;
}
#ifdef WILDDOG_ADD_ONLINESTAT

/*
 * Function:    _wilddog_ct_onlineCallbackTrigger
 * Description: trigger all online callbacks
 * Input:       N/A.
 * Output:      N/A
 * Return:      N/A
*/
void _wilddog_ct_onlineCallbackTrigger(void){
    Wilddog_Repo_T** p_head = _wilddog_ct_getRepoHead();
    Wilddog_Repo_T *p_curr, *p_tmp;
    Wilddog_Node_T *tmp_online_status = NULL;
    tmp_online_status = (_wilddog_ct_getOnlineStatus())?
        (wilddog_node_createTrue(NULL)) : (wilddog_node_createFalse(NULL));
    if(NULL == tmp_online_status){
        return ;
    }
    LL_FOREACH_SAFE(*p_head, p_curr, p_tmp)
    {
        if(p_curr->p_rp_onlineFunc){
            ((onEventFunc)(p_curr->p_rp_onlineFunc))(tmp_online_status, p_curr->p_rp_onlineArg, WILDDOG_HTTP_OK);
        }
    }
    wilddog_node_delete(tmp_online_status);
    return;
}
#endif
#ifdef WILDDOG_FORCE_OFFLINE
u32 WD_SYSTEM _wilddog_ct_getOfflineForced(void)
{
    return l_wilddog_containTable.isForcedOffline;
}

u32 WD_SYSTEM _wilddog_ct_setOfflineForced(BOOL flag)
{
    l_wilddog_containTable.isForcedOffline = flag;
    return WILDDOG_ERR_NOERR;
}
#endif

u32 WD_SYSTEM _wilddog_ct_getOnlineStatus(void)
{
    return l_wilddog_containTable.d_rc_online;
}

Wilddog_Return_T WD_SYSTEM _wilddog_ct_setOnlineStatus(u32 s)
{
#ifdef WILDDOG_FORCE_OFFLINE
    //add force flag, if set to online but is forced to offline ,return
    if(s && _wilddog_ct_getOfflineForced()){
        return WILDDOG_ERR_INVALID;
    }
#endif
    l_wilddog_containTable.d_rc_online = s;
#ifdef WILDDOG_ADD_ONLINESTAT
    //trigger all repo's online callbacks
    _wilddog_ct_onlineCallbackTrigger();
#endif
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_ct_getRepoHead
 * Description: Get the head of the repo.
 * Input:       N/A.
 * Output:      N/A
 * Return:      Pointer to the head.
*/
Wilddog_Repo_T** WD_SYSTEM _wilddog_ct_getRepoHead(void)
{
    return &(l_wilddog_containTable.p_rc_head);
}

/*
 * Function:    _wilddog_ct_getRepoNum
 * Description: Get the number of the repo.
 * Input:       N/A.
 * Output:      N/A
 * Return:      the number.
*/
u8 WD_SYSTEM _wilddog_ct_getRepoNum(void)
{
    int count = 0;
    Wilddog_Repo_T** p_head = _wilddog_ct_getRepoHead();
    Wilddog_Repo_T *p_curr, *p_tmp;
    LL_FOREACH_SAFE(*p_head, p_curr, p_tmp)
    {
        count++;
    }
    return count;
}

/*
 * Function:    _wilddog_ct_createRef
 * Description: create ref.
 * Input:       args: the url args
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      the Wilddog_T
*/
STATIC Wilddog_T WD_SYSTEM _wilddog_ct_createRef(void *args, int flag)
{
    Wilddog_Url_T *p_url = NULL;
    Wilddog_Ref_T *p_ref = NULL;
    Wilddog_Repo_T *p_repo = NULL;
    Wilddog_Str_T *url = (Wilddog_Str_T*)args;
    
    wilddog_assert(url, 0);

    if(FALSE == l_isStarted)
    {
        _wilddog_ct_init(NULL, 0);
        l_isStarted = TRUE;
    }

    //todo remove
    /*add valid check!*/
    if(!_wilddog_isUrlValid(url))
        return 0;

    p_url = _wilddog_url_parseUrl(url);
    if(NULL == p_url)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "parse url failed!");
        return 0;
    }

    /*find if can in a repo*/
    p_repo = _wilddog_ct_findRepo(p_url->p_url_host);
    if(NULL == p_repo)
    {
        /*cannot find repo, create a repo*/
        p_repo = _wilddog_ct_createRepo(p_url);
        if(NULL == p_repo)
        {
            _wilddog_url_freeParsedUrl(p_url);
            return 0;
        }
    }
    else
    {
        /*find whether it has a ref already*/
        p_ref = _wilddog_ct_findRef(p_repo, p_url);
        if(NULL != p_ref)
        {
            /*find, return ref*/
            _wilddog_url_freeParsedUrl(p_url);
            return (Wilddog_T)p_ref;
        }
    }

    /*create ref*/
    p_ref = (Wilddog_Ref_T*)wmalloc(sizeof(Wilddog_Ref_T));
    if(NULL == p_ref)
    {
        _wilddog_url_freeParsedUrl(p_url);
        return 0;
    }
    p_ref->p_ref_url = p_url;
    p_ref->p_ref_repo = p_repo;
    p_ref->next = NULL;
    LL_APPEND(p_repo->p_rp_head, p_ref);

    return (Wilddog_T)p_ref;
}


/*
 * Function:    _wilddog_ct_destroyRef
 * Description: destory ref.
 * Input:       args: the url args
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if success, return WILDDOG_ERR_NOERR; else return 
 *              WILDDOG_ERR_NULL
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_destroyRef
    (
    void *args, 
    int flag
    )
{
    Wilddog_Repo_T * p_repo = NULL;
    Wilddog_Ref_T **p_ref = (Wilddog_Ref_T**)args;

    if(NULL == (*p_ref))
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "ref is null!");
        return WILDDOG_ERR_NULL;
    }

    if(NULL == (*p_ref)->p_ref_url)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "ref already been deleted!");
        return WILDDOG_ERR_NULL;
    }

    p_repo = _wilddog_ct_findRepo((*p_ref)->p_ref_url->p_url_host);
    if(NULL == p_repo || p_repo != (*p_ref)->p_ref_repo)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "cannot find repo!");
        if(NULL != (*p_ref)->p_ref_url)
        {
            _wilddog_url_freeParsedUrl((*p_ref)->p_ref_url);
            (*p_ref)->p_ref_url = NULL;
        }
        (*p_ref) = NULL;
        return WILDDOG_ERR_NULL;
    }
    
    if(NULL == _wilddog_ct_findRef((*p_ref)->p_ref_repo, (*p_ref)->p_ref_url))
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "cannot find ref!");
        if(NULL != (*p_ref)->p_ref_url)
        {
            _wilddog_url_freeParsedUrl((*p_ref)->p_ref_url);
            (*p_ref)->p_ref_url = NULL;
        }
        (*p_ref) = NULL;
        return WILDDOG_ERR_NULL;
    }
    
    if(NULL != (*p_ref)->p_ref_url)
    {
        _wilddog_url_freeParsedUrl((*p_ref)->p_ref_url);
        (*p_ref)->p_ref_url = NULL;
    }

    LL_DELETE(p_repo->p_rp_head, (*p_ref));

    /*if repo only has this ref, delete repo.*/
    if(NULL == p_repo->p_rp_head)
    {
        _wilddog_ct_destoryRepo(p_repo);
        p_repo = NULL;
    }
    if(NULL != (*p_ref))
        wfree((*p_ref));
    (*p_ref) = NULL;
    return WILDDOG_ERR_NOERR;
}


/*
 * Function:    _wilddog_ct_findRef
 * Description: find ref from the repo by the url
 * Input:       p_repo: the pointer of the repo struct
 *              p_url: the url
 * Output:      N/A
 * Return:      the pointer of the ref struct
*/
STATIC Wilddog_Ref_T * WD_SYSTEM _wilddog_ct_findRef
    (
    Wilddog_Repo_T *p_repo, 
    Wilddog_Url_T * p_url
    )
{
    Wilddog_Ref_T *p_curr = NULL, *p_tmp = NULL;

    wilddog_assert(p_repo, NULL);
    wilddog_assert(p_url, NULL);
    if(NULL == p_repo->p_rp_head)
        return NULL;
    LL_FOREACH_SAFE(p_repo->p_rp_head, p_curr, p_tmp)
    {
        if(FALSE == _wilddog_url_diff(p_url, p_curr->p_ref_url))
        {
            /* find , return.*/
            return p_curr;
        }
    }
    return NULL;
}

/*
 * Function:    _wilddog_ct_getRef
 * Description: get the ref
 * Input:       arg: the pointer of the arg get ref struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      the pointer of the ref struct
*/
STATIC Wilddog_Ref_T * WD_SYSTEM _wilddog_ct_getRef(void* arg, int flag)
{
    Wilddog_Arg_GetRef_T * args = (Wilddog_Arg_GetRef_T* )arg;
    Wilddog_Ref_T *p_srcRef = (Wilddog_Ref_T *)(args->p_ref);
    Wilddog_RefChange_T type = args->d_cmd;
    Wilddog_Str_T * str = args->p_str;
    int len;
    Wilddog_Str_T *p_path = NULL;
    Wilddog_Url_T *p_url = NULL;
    Wilddog_Ref_T *p_ref = NULL;
    
    if(!p_srcRef->p_ref_url || !p_srcRef->p_ref_url->p_url_path)
        return NULL;

    /*when user want get parent or root, check whether it is root already*/
    if(WILDDOG_REFCHG_PARENT == type || WILDDOG_REFCHG_ROOT == type)
    {
        if(p_srcRef->p_ref_url->p_url_path)
        {
            if(strlen((const char*)p_srcRef->p_ref_url->p_url_path) == 1 && \
                *(char*)(p_srcRef->p_ref_url->p_url_path) == '/')
            {
                /*it is already root, get parent return NULL*/
                if(WILDDOG_REFCHG_PARENT == type)
                    return NULL;
                
                /*it is already root, get root return itself*/
                return p_srcRef;
            }
        }
    }
    
    if(
        WILDDOG_ERR_NOERR != \
       _wilddog_url_getPath(p_srcRef->p_ref_url->p_url_path,type, str, &p_path)
       )
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "can not find path!");
        return NULL;
    }

    /*
     * In child type, we should check whether the new path is valid, but it's 
     * hard, because user may call this func before get data from server, 
     * so let user check it.
    */
    
    p_url = (Wilddog_Url_T *)wmalloc(sizeof(Wilddog_Url_T));
    if(NULL == p_url)
    {
        return NULL;
    }

    /* build a url structure, to find existed ref*/

    /* Reuse p_srcRef's host to find ref.*/
    p_url->p_url_host = p_srcRef->p_ref_url->p_url_host;
    p_url->p_url_path = p_path;

    /*same host only build one repo, so we find it in current repo */
    p_ref = _wilddog_ct_findRef(p_srcRef->p_ref_repo, p_url);

    if(NULL == p_ref)
    {
        //todo merge with create ref
        /*cannot find ref, new one*/
        p_ref = (Wilddog_Ref_T*)wmalloc(sizeof(Wilddog_Ref_T));
        if(NULL == p_ref)
        {
            _wilddog_url_freeParsedUrl(p_url);
            return NULL;
        }
        len = strlen((const char *)(p_srcRef->p_ref_url->p_url_host));
        p_url->p_url_host = (Wilddog_Str_T*)wmalloc(len + 1);
        if(NULL == p_url->p_url_host)
        {
            _wilddog_url_freeParsedUrl(p_url);
            wfree(p_ref);
            return NULL;
        }

        memcpy(p_url->p_url_host, p_srcRef->p_ref_url->p_url_host, len);

        p_ref->p_ref_repo = p_srcRef->p_ref_repo;
        p_ref->p_ref_url = p_url;
        LL_APPEND(p_ref->p_ref_repo->p_rp_head, p_ref);
    }
    else
    {
        p_url->p_url_host = NULL;
        _wilddog_url_freeParsedUrl(p_url);
    }

    return p_ref;
}

/*
 * Function:    _wilddog_ct_createRepo
 * Description: create the repo
 * Input:       p_url: the pointer of url
 * Output:      N/A
 * Return:      the pointer of the repo struct
*/
STATIC Wilddog_Repo_T * WD_SYSTEM _wilddog_ct_createRepo
    (
    Wilddog_Url_T *p_url
    )
{
    Wilddog_Repo_T *p_repo =NULL;
    Wilddog_Repo_T **p_repoHead = NULL;
    int len;
    Wilddog_Url_T * p_rp_url = NULL;

    wilddog_assert(p_url, NULL);

    //todo costdown
    p_repo = _wilddog_ct_findRepo(p_url->p_url_host);
    
    if(NULL != p_repo)
    {
        return p_repo;
    }
    
    /*create repo*/
    p_repoHead = _wilddog_ct_getRepoHead();
    
    p_repo = (Wilddog_Repo_T*)wmalloc(sizeof(Wilddog_Repo_T));
    if(NULL == p_repo)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, "cannot malloc p_repo!\n");
        return NULL;
    }
    
    p_repo->p_rp_head = NULL;
    
    p_rp_url = (Wilddog_Url_T *)wmalloc(sizeof(Wilddog_Url_T));
    if(NULL == p_rp_url)
    {
        wfree(p_repo);
        return NULL;
    }

    len = strlen((const char *)(p_url->p_url_host));
    p_rp_url->p_url_host = (Wilddog_Str_T*)wmalloc(len + 1);
    if(NULL == p_rp_url->p_url_host)
    {
        wfree(p_repo);
        return NULL;
    }
    memcpy(p_rp_url->p_url_host, p_url->p_url_host, len);
    
    p_repo->p_rp_url = p_rp_url;
    p_repo->next = NULL;

    p_repo->p_rp_store= _wilddog_store_init(p_repo);
    p_repo->p_rp_conn = _wilddog_conn_init(p_repo);

    /*add to repo container*/
    LL_APPEND(*p_repoHead, p_repo);

    return p_repo;
}

/*
 * Function:    _wilddog_ct_destoryRepo
 * Description: destory the repo
 * Input:       p_repo: the pointer of repo
 * Output:      N/A
 * Return:      return WILDDOG_ERR_NOERR
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_destoryRepo
    (
    Wilddog_Repo_T *p_repo
    )
{
    Wilddog_Repo_T **p_repoHead = NULL;

    wilddog_assert(p_repo, WILDDOG_ERR_NULL);
    p_repoHead = _wilddog_ct_getRepoHead(); 
    
    _wilddog_store_deinit(p_repo);
    _wilddog_conn_deinit(p_repo);
    
    p_repo->p_rp_store = NULL;
    p_repo->p_rp_conn  = NULL;
    
    _wilddog_url_freeParsedUrl(p_repo->p_rp_url);
    p_repo->p_rp_url = NULL;
    LL_DELETE((*p_repoHead), p_repo);

    wfree(p_repo);
    p_repo = NULL;
    if(!(*p_repoHead))
    {
        l_isStarted = FALSE;
        _wilddog_ct_deinit();
    }
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_ct_findRepo
 * Description: find repo by the host
 * Input:       p_host: the pointer of host
 * Output:      N/A
 * Return:      the pointer of the repo
*/
Wilddog_Repo_T * WD_SYSTEM _wilddog_ct_findRepo(Wilddog_Str_T * p_host)
{
    Wilddog_Repo_T *p_curr = NULL, *p_tmp = NULL;
    Wilddog_Repo_T **p_repoHead = NULL;

    p_repoHead = _wilddog_ct_getRepoHead(); 
    if(NULL == *p_repoHead || NULL == p_host)
        return NULL;
    LL_FOREACH_SAFE(*p_repoHead, p_curr, p_tmp)
    {
        if(p_curr->p_rp_url->p_url_host && p_host)
        {
            if(0 == strcmp((const char *)(p_curr->p_rp_url->p_url_host), \
                (const char *)p_host))
                return p_curr;
        }
    }
    return NULL;
}

/*
 * Function:    _wilddog_ct_store_setAuth
 * Description: set auth
 * Input:       p_args: the pointer of the arg set auth struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if failed, return WILDDOG_ERR_INVALID
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_setAuth
    (
    void *p_args, 
    int flag
    )
{
    Wilddog_Arg_SetAuth_T *arg = (Wilddog_Arg_SetAuth_T*)p_args;
    Wilddog_Repo_T *p_repo =NULL;

    Wilddog_Store_AuthArg_T authArg;
    Wilddog_Store_T * p_rp_store = NULL;
    
    wilddog_assert(arg->p_host, WILDDOG_ERR_NULL);
#ifdef WILDDOG_FORCE_OFFLINE
    if(TRUE == _wilddog_ct_getOfflineForced()){
        return WILDDOG_ERR_CLIENTOFFLINE;
    }
#endif

    /*add valid check!*/
    if(!_wilddog_isAuthValid(arg->p_auth, arg->d_len))
        return WILDDOG_ERR_INVALID;

    p_repo = _wilddog_ct_findRepo(arg->p_host);
    if(NULL == p_repo)
        return WILDDOG_ERR_INVALID;

    authArg.p_data = arg->p_auth;
    authArg.d_len = arg->d_len;
    authArg.p_onAuth = arg->onAuth;
    authArg.p_onAuthArg = arg->arg;
    authArg.p_url = p_repo->p_rp_url;

    p_rp_store = p_repo->p_rp_store;
    if( p_rp_store && p_rp_store->p_se_callback)
        return (p_rp_store->p_se_callback)(p_rp_store, \
                                        WILDDOG_STORE_CMD_SETAUTH, &authArg, 0);
    else
        return WILDDOG_ERR_INVALID;
}

/*
 * Function:    _wilddog_ct_store_query
 * Description: query function
 * Input:       p_args: the pointer of the arg set auth struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if failed, return WILDDOG_ERR_INVALID
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_query
    (
    void *p_args, 
    int flag
    )
{
    Wilddog_Arg_Query_T *arg = (Wilddog_Arg_Query_T* )p_args;
    Wilddog_ConnCmd_Arg_T connCmd;
    Wilddog_Store_T * p_rp_store = NULL;
    Wilddog_Ref_T * p_ref = (Wilddog_Ref_T *)(arg->p_ref);
    Wilddog_Return_T ret = WILDDOG_ERR_NULL;
#ifdef WILDDOG_ADD_ONLINESTAT
    if(NULL != p_ref->p_ref_url && NULL != p_ref->p_ref_url->p_url_path){
        if(0 == strncmp((const char*)p_ref->p_ref_url->p_url_path, WILDDOG_ONLINE_PATH_WITH_SPRIT,strlen(WILDDOG_ONLINE_PATH_WITH_SPRIT))){
            //trigger callback for this repo
            Wilddog_Node_T *tmp_online_status = NULL;
            tmp_online_status = (_wilddog_ct_getOnlineStatus())?
                (wilddog_node_createTrue(NULL)) : (wilddog_node_createFalse(NULL));
            
            if(NULL == tmp_online_status){
                ret = WILDDOG_ERR_NULL;
                goto query_done;
            }
            if(arg->p_callback){
                ((onQueryFunc)(arg->p_callback))((const Wilddog_Node_T*)tmp_online_status,arg->arg, WILDDOG_HTTP_OK);
            }
            wilddog_node_delete(tmp_online_status);
            return WILDDOG_ERR_NOERR;
        }
    }
#endif
#ifdef WILDDOG_FORCE_OFFLINE
    if(TRUE == _wilddog_ct_getOfflineForced()){
        ret = WILDDOG_ERR_CLIENTOFFLINE;
        goto query_done;
    }
#endif

    connCmd.p_repo = p_ref->p_ref_repo;
    connCmd.p_url = p_ref->p_ref_url;
    connCmd.p_complete = arg->p_callback;
    connCmd.p_completeArg = arg->arg;
    connCmd.p_data = NULL;
    p_rp_store = p_ref->p_ref_repo->p_rp_store;
    if( p_rp_store && p_rp_store->p_se_callback){
        ret = (p_rp_store->p_se_callback)(p_rp_store, \
                                        WILDDOG_STORE_CMD_SENDGET, &connCmd, 0);
    }
    
query_done:
    if(WILDDOG_ERR_NOERR != ret){
        if(arg->p_callback){
            ((onQueryFunc)(arg->p_callback))(NULL,arg->arg, ret);
        }
    }
    return ret;
}

/*
 * Function:    _wilddog_ct_store_set
 * Description: set function
 * Input:       p_args: the pointer of the arg set auth struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if failed, return WILDDOG_ERR_INVALID
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_set
    (
    void *p_args, 
    int flag
    )
{
    Wilddog_Arg_Set_T *arg = (Wilddog_Arg_Set_T*)p_args;
    Wilddog_ConnCmd_Arg_T connCmd;
    Wilddog_Store_T * p_rp_store = NULL;
    Wilddog_Ref_T * p_ref = (Wilddog_Ref_T *)(arg->p_ref);
    Wilddog_Store_Cmd_T cmd = WILDDOG_STORE_CMD_SENDSET;
    Wilddog_Return_T ret = WILDDOG_ERR_NULL;
#ifdef WILDDOG_ADD_ONLINESTAT
    if(NULL != p_ref->p_ref_url && NULL != p_ref->p_ref_url->p_url_path){
        if(0 == strncmp((const char*)p_ref->p_ref_url->p_url_path, WILDDOG_ONLINE_PATH_WITH_SPRIT,strlen(WILDDOG_ONLINE_PATH_WITH_SPRIT))){
            //It is invalid!
            wilddog_debug_level(WD_DEBUG_ERROR, "Set %s is invalid!",WILDDOG_ONLINE_PATH_WITH_SPRIT);
            ret = WILDDOG_ERR_INVALID;
            goto set_done;
        }
    }
#endif

#ifdef WILDDOG_FORCE_OFFLINE
    if(TRUE == _wilddog_ct_getOfflineForced()){
        ret = WILDDOG_ERR_CLIENTOFFLINE;
        goto set_done;
    }
#endif

    cmd = (flag == WD_CMD_NORMAL)? \
                   (WILDDOG_STORE_CMD_SENDSET): (WILDDOG_STORE_CMD_ONDISSET);
    connCmd.p_repo = p_ref->p_ref_repo;
    connCmd.p_url = p_ref->p_ref_url;
    connCmd.p_data = arg->p_node;
    connCmd.p_complete = arg->p_callback;
    connCmd.p_completeArg = arg->arg;

    p_rp_store = p_ref->p_ref_repo->p_rp_store;
    
    if(p_rp_store && p_rp_store->p_se_callback){
        ret = (p_rp_store->p_se_callback)(p_rp_store, \
                                    cmd, &connCmd, 0);
    }

set_done:
    if(WILDDOG_ERR_NOERR != ret){
        if(arg->p_callback){
            ((onSetFunc)(arg->p_callback))(arg->arg, ret);
        }
    }
    return ret;
}

/*
 * Function:    _wilddog_ct_store_push
 * Description: push function
 * Input:       p_args: the pointer of the arg set auth struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if failed, return WILDDOG_ERR_INVALID
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_push
    (
    void* p_args, 
    int flag
    )
{
    Wilddog_Arg_Push_T *arg = (Wilddog_Arg_Push_T*)p_args;
    Wilddog_ConnCmd_Arg_T connCmd;
    Wilddog_Store_T * p_rp_store = NULL;
    Wilddog_Ref_T * p_ref = (Wilddog_Ref_T *)(arg->p_ref);
    Wilddog_Store_Cmd_T cmd = WILDDOG_STORE_CMD_SENDSET;
    Wilddog_Return_T ret = WILDDOG_ERR_NULL;
#ifdef WILDDOG_ADD_ONLINESTAT
    if(NULL != p_ref->p_ref_url && NULL != p_ref->p_ref_url->p_url_path){
        if(0 == strncmp((const char*)p_ref->p_ref_url->p_url_path, WILDDOG_ONLINE_PATH_WITH_SPRIT,strlen(WILDDOG_ONLINE_PATH_WITH_SPRIT))){
            //It is invalid!
            wilddog_debug_level(WD_DEBUG_ERROR, "Push %s is invalid!",WILDDOG_ONLINE_PATH_WITH_SPRIT);
            ret = WILDDOG_ERR_INVALID;
            goto push_done;
        }
    }
#endif
#ifdef WILDDOG_FORCE_OFFLINE
    if(TRUE == _wilddog_ct_getOfflineForced()){
        ret = WILDDOG_ERR_CLIENTOFFLINE;
        goto push_done;
    }
#endif

    cmd = (flag == WD_CMD_NORMAL)? \
                   (WILDDOG_STORE_CMD_SENDPUSH): (WILDDOG_STORE_CMD_ONDISPUSH);

    connCmd.p_repo = p_ref->p_ref_repo;
    connCmd.p_url = p_ref->p_ref_url;
    connCmd.p_data = arg->p_node;
    connCmd.p_complete = arg->p_callback;
    connCmd.p_completeArg = arg->arg;
    
    p_rp_store = p_ref->p_ref_repo->p_rp_store;
    if(p_rp_store && p_rp_store->p_se_callback){
        ret = (p_rp_store->p_se_callback)(p_rp_store, \
                                    cmd, &connCmd, 0);
    }
    
push_done:
    if(WILDDOG_ERR_NOERR != ret){
        if(arg->p_callback){
            ((onPushFunc)(arg->p_callback))(NULL, arg->arg, ret);
        }
    }
    return ret;
}

/*
 * Function:    _wilddog_ct_store_remove
 * Description: remove function
 * Input:       p_args: the pointer of the arg set auth struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if failed, return WILDDOG_ERR_INVALID
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_remove
    (
    void* p_args,
    int flag
    )
{
    Wilddog_Arg_Remove_T *arg = (Wilddog_Arg_Remove_T*)p_args;
    Wilddog_ConnCmd_Arg_T connCmd;
    Wilddog_Store_T * p_rp_store = NULL;
    Wilddog_Ref_T * p_ref = (Wilddog_Ref_T *)(arg->p_ref);
    Wilddog_Store_Cmd_T cmd = WILDDOG_STORE_CMD_SENDSET;
    Wilddog_Return_T ret = WILDDOG_ERR_NULL;
#ifdef WILDDOG_ADD_ONLINESTAT
    if(NULL != p_ref->p_ref_url && NULL != p_ref->p_ref_url->p_url_path){
        if(0 == strncmp((const char*)p_ref->p_ref_url->p_url_path, WILDDOG_ONLINE_PATH_WITH_SPRIT,strlen(WILDDOG_ONLINE_PATH_WITH_SPRIT))){
            //It is invalid!
            wilddog_debug_level(WD_DEBUG_ERROR, "Remove %s is invalid!",WILDDOG_ONLINE_PATH_WITH_SPRIT);
            ret = WILDDOG_ERR_INVALID;
            goto remove_done;
        }
    }
#endif
#ifdef WILDDOG_FORCE_OFFLINE
    if(TRUE == _wilddog_ct_getOfflineForced()){
        ret = WILDDOG_ERR_CLIENTOFFLINE;
        goto remove_done;
    }
#endif

    cmd = (flag == WD_CMD_NORMAL)? \
                   (WILDDOG_STORE_CMD_SENDREMOVE): (WILDDOG_STORE_CMD_ONDISRMV);

    connCmd.p_repo = p_ref->p_ref_repo;
    connCmd.p_url = p_ref->p_ref_url;
    connCmd.p_complete = arg->p_callback;
    connCmd.p_completeArg = arg->arg;
    connCmd.p_data = NULL;
    p_rp_store = p_ref->p_ref_repo->p_rp_store;
    if(p_rp_store && p_rp_store->p_se_callback){
        ret = (p_rp_store->p_se_callback)(p_rp_store, \
                                    cmd, &connCmd, 0);
    }
remove_done:
    if(WILDDOG_ERR_NOERR != ret){
        if(arg->p_callback){
            ((onRemoveFunc)(arg->p_callback))(arg->arg, ret);
        }
    }
    return ret;
}

/*
 * Function:    _wilddog_ct_store_on
 * Description: oberve on function
 * Input:       p_args: the pointer of the arg set auth struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if failed, return WILDDOG_ERR_INVALID
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_on
    (
    void* p_args, 
    int flag
    )
{
    Wilddog_Arg_On_T *arg = (Wilddog_Arg_On_T*)p_args;
    Wilddog_ConnCmd_Arg_T connCmd;
    Wilddog_Store_T * p_rp_store = NULL;
    Wilddo_Store_EventArg_T eventArg;
    Wilddog_Ref_T * p_ref = (Wilddog_Ref_T *)(arg->p_ref);
    Wilddog_Return_T ret = WILDDOG_ERR_NULL;
#ifdef WILDDOG_ADD_ONLINESTAT
    if(NULL != p_ref->p_ref_url && NULL != p_ref->p_ref_url->p_url_path){
        if(0 == strncmp((const char*)p_ref->p_ref_url->p_url_path, WILDDOG_ONLINE_PATH_WITH_SPRIT,strlen(WILDDOG_ONLINE_PATH_WITH_SPRIT))){
            //trigger callback for this repo
            Wilddog_Node_T *tmp_online_status = NULL;
            tmp_online_status = (_wilddog_ct_getOnlineStatus())?
                (wilddog_node_createTrue(NULL)) : (wilddog_node_createFalse(NULL));
            
            if(NULL == tmp_online_status){
                ret = WILDDOG_ERR_NULL;
                goto on_done;
            }
            if(arg->p_onData){
                ((onEventFunc)(arg->p_onData))((const Wilddog_Node_T*)tmp_online_status,arg->p_dataArg, WILDDOG_HTTP_OK);
            }
            wilddog_node_delete(tmp_online_status);
            
            //put this callback to repo
            wilddog_debug_level(WD_DEBUG_LOG, "Add observer %s to online callback!",WILDDOG_ONLINE_PATH_WITH_SPRIT);
            p_ref->p_ref_repo->p_rp_onlineFunc = arg->p_onData;
            p_ref->p_ref_repo->p_rp_onlineArg = arg->p_dataArg;

            return WILDDOG_ERR_NOERR;
        }
    }
#endif
#ifdef WILDDOG_FORCE_OFFLINE
    if(TRUE == _wilddog_ct_getOfflineForced()){
        ret = WILDDOG_ERR_CLIENTOFFLINE;
        goto on_done;
    }
#endif

    connCmd.p_repo = p_ref->p_ref_repo;
    connCmd.p_url = p_ref->p_ref_url;
    connCmd.p_complete = arg->p_onData;
    connCmd.p_completeArg = arg->p_dataArg;
    connCmd.p_data = NULL;
    eventArg.d_event = arg->d_event;
    eventArg.d_connCmd = connCmd;
    
    p_rp_store = p_ref->p_ref_repo->p_rp_store;
    if(p_rp_store && p_rp_store->p_se_callback){
        ret = (p_rp_store->p_se_callback)(p_rp_store, \
                                    WILDDOG_STORE_CMD_SENDON, &eventArg, 0);
    }
on_done:
    if(WILDDOG_ERR_NOERR != ret){
        if(arg->p_onData){
            ((onEventFunc)(arg->p_onData))(NULL,arg->p_dataArg, ret);
        }
    }
    return ret;
}

/*
 * Function:    _wilddog_ct_store_off
 * Description: oberve off function
 * Input:       p_args: the pointer of the arg set auth struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if failed, return WILDDOG_ERR_INVALID
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_off
    (
    void* p_args, 
    int flag
    )
{
    Wilddog_Arg_Off_T *arg = (Wilddog_Arg_Off_T*)p_args;
    Wilddog_ConnCmd_Arg_T connCmd;
    Wilddog_Store_T * p_rp_store = NULL;
    Wilddo_Store_EventArg_T eventArg;
    Wilddog_Ref_T * p_ref = (Wilddog_Ref_T *)(arg->p_ref);
#ifdef WILDDOG_ADD_ONLINESTAT
    if(NULL != p_ref->p_ref_url && NULL != p_ref->p_ref_url->p_url_path){
        if(0 == strncmp((const char*)p_ref->p_ref_url->p_url_path, WILDDOG_ONLINE_PATH_WITH_SPRIT,strlen(WILDDOG_ONLINE_PATH_WITH_SPRIT))){
            //put this callback to repo
            wilddog_debug_level(WD_DEBUG_LOG, "Remove observer %s to online callback!",WILDDOG_ONLINE_PATH_WITH_SPRIT);
            p_ref->p_ref_repo->p_rp_onlineFunc = NULL;
            p_ref->p_ref_repo->p_rp_onlineArg = NULL;

            return WILDDOG_ERR_NOERR;
        }
    }
#endif
#ifdef WILDDOG_FORCE_OFFLINE
    if(TRUE == _wilddog_ct_getOfflineForced()){
        return WILDDOG_ERR_CLIENTOFFLINE;
    }
#endif

    connCmd.p_repo = p_ref->p_ref_repo;
    connCmd.p_url = p_ref->p_ref_url;
    connCmd.p_data = NULL;
    connCmd.p_complete = NULL;
    connCmd.p_completeArg = NULL;
    eventArg.d_event = arg->d_event;
    eventArg.d_connCmd = connCmd;
    p_rp_store = p_ref->p_ref_repo->p_rp_store;
    if(p_rp_store && p_rp_store->p_se_callback)
        return (p_rp_store->p_se_callback)(p_rp_store, \
                                    WILDDOG_STORE_CMD_SENDOFF, &eventArg, 0);
    else
        return WILDDOG_ERR_INVALID;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_disConnSet
    (
    void *p_args, 
    int flag
    )
{
    return _wilddog_ct_store_set(p_args, WD_CMD_ONDIS);
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_disConnPush
    (
    void *p_args, 
    int flag
    )
{
    return _wilddog_ct_store_push(p_args, WD_CMD_ONDIS);;
}

STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_disConnRmv
    (
    void *p_args, 
    int flag
    )
{
    return _wilddog_ct_store_remove(p_args, WD_CMD_ONDIS);
}
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_store_disCancel
    (
    void *p_args, 
    int flag
    )
{
    Wilddog_Arg_Remove_T *arg = (Wilddog_Arg_Remove_T*)p_args;
    Wilddog_ConnCmd_Arg_T connCmd;
    Wilddog_Store_T * p_rp_store = NULL;
    Wilddog_Ref_T * p_ref = (Wilddog_Ref_T *)(arg->p_ref);
#ifdef WILDDOG_ADD_ONLINESTAT
    if(NULL != p_ref->p_ref_url && NULL != p_ref->p_ref_url->p_url_path){
        if(0 == strncmp((const char*)p_ref->p_ref_url->p_url_path, WILDDOG_ONLINE_PATH_WITH_SPRIT,strlen(WILDDOG_ONLINE_PATH_WITH_SPRIT))){
            //It is invalid!
            wilddog_debug_level(WD_DEBUG_ERROR, "Disconnect cancel %s is invalid!",WILDDOG_ONLINE_PATH_WITH_SPRIT);
            return WILDDOG_ERR_INVALID;
        }
    }
#endif
#ifdef WILDDOG_FORCE_OFFLINE
    if(TRUE == _wilddog_ct_getOfflineForced()){
        return WILDDOG_ERR_CLIENTOFFLINE;
    }
#endif

    connCmd.p_repo = p_ref->p_ref_repo;
    connCmd.p_url = p_ref->p_ref_url;
    connCmd.p_complete = arg->p_callback;
    connCmd.p_completeArg = arg->arg;
    connCmd.p_data = NULL;
    p_rp_store = p_ref->p_ref_repo->p_rp_store;
    if(p_rp_store && p_rp_store->p_se_callback)
        return (p_rp_store->p_se_callback)(p_rp_store, \
                                    WILDDOG_STORE_CMD_ONDISCANCEL, &connCmd, 0);
    else
        return WILDDOG_ERR_INVALID;

}
/*
 * Function:    _wilddog_ct_url_getKey
 * Description: get key 
 * Input:       p_args: the pointer of the ref struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if failed, return NULL
*/
STATIC Wilddog_Str_T* WD_SYSTEM _wilddog_ct_url_getKey
    (
    void* p_args, 
    int flag
    )
{
    Wilddog_Ref_T * p_ref = (Wilddog_Ref_T*)p_args;

    wilddog_assert(p_ref, NULL);

    return _wilddog_url_getKey(p_ref->p_ref_url->p_url_path);
}

/*
 * Function:    _wilddog_ct_getHost
 * Description: get host of the wilddog client 
 * Input:       p_args: the pointer of the ref struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if failed, return NULL
*/
STATIC Wilddog_Str_T* WD_SYSTEM _wilddog_ct_getHost
    (
    void* p_args, 
    int flag
    )
{
    Wilddog_Ref_T * p_ref = (Wilddog_Ref_T*)p_args;
/*    Wilddog_Str_T* p_host = NULL;*/
    wilddog_assert(p_ref, NULL);
    wilddog_assert(p_ref->p_ref_url, NULL);
    
/*    if(p_ref->p_ref_url->p_url_host)
    {
        int len = strlen((const char*)p_ref->p_ref_url->p_url_host);

        p_host = (Wilddog_Str_T*)wmalloc(len + 1);
        if(p_host)
            memcpy((char*)p_host, (char*)p_ref->p_ref_url->p_url_host, len);
    }
    return p_host;
    */
    return p_ref->p_ref_url->p_url_host;
}

/*
 * Function:    _wilddog_ct_getPath
 * Description: get path of the wilddog client 
 * Input:       p_args: the pointer of the ref struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if failed, return NULL
*/
STATIC Wilddog_Str_T* WD_SYSTEM _wilddog_ct_getPath
    (
    void* p_args, 
    int flag
    )
{
    Wilddog_Ref_T * p_ref = (Wilddog_Ref_T*)p_args;
/*    Wilddog_Str_T* p_path = NULL;*/
    
    wilddog_assert(p_ref, NULL);
    wilddog_assert(p_ref->p_ref_url, NULL);
/*    if(p_ref->p_ref_url->p_url_path)
    {
        int len = strlen((const char*)p_ref->p_ref_url->p_url_path);
        
        p_path = (Wilddog_Str_T*)wmalloc(len + 1);
        if(p_path)
            memcpy((char*)p_path, (char*)p_ref->p_ref_url->p_url_path, len);
    }
    return p_path;*/
    return p_ref->p_ref_url->p_url_path;
}

/*
 * Function:    _wilddog_ct_conn_goOffline
 * Description: let wilddog client offline 
 * Input:       N/A
 * Output:      N/A
 * Return:      return WILDDOG_ERR_NOERR
*/
Wilddog_Return_T _wilddog_ct_conn_goOffline()
{
    Wilddog_Repo_T** p_head = _wilddog_ct_getRepoHead();
    Wilddog_Repo_T* p_curr, *p_tmp;
    Wilddog_Conn_T * p_conn;

#ifdef WILDDOG_FORCE_OFFLINE
    _wilddog_ct_setOfflineForced(TRUE);
#endif
    LL_FOREACH_SAFE(*p_head, p_curr, p_tmp)
    {
      p_conn = p_curr->p_rp_conn;
      if(p_conn && p_conn->f_conn_ioctl)
      {
          Wilddog_ConnCmd_Arg_T cmd = {NULL, NULL, NULL, NULL, NULL};
          cmd.p_repo = p_curr;
          cmd.p_url = p_curr->p_rp_url;
          (p_conn->f_conn_ioctl)(WILDDOG_CONN_CMD_OFFLINE, &cmd, 0);
      }
    }
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_ct_conn_goOnline
 * Description: let wilddog client online 
 * Input:       N/A
 * Output:      N/A
 * Return:      return WILDDOG_ERR_NOERR
*/
Wilddog_Return_T _wilddog_ct_conn_goOnline()
{
    Wilddog_Repo_T** p_head = _wilddog_ct_getRepoHead();
    Wilddog_Repo_T* p_curr, *p_tmp;
    Wilddog_Conn_T * p_conn;

#ifdef WILDDOG_FORCE_OFFLINE
    _wilddog_ct_setOfflineForced(FALSE);
#endif
    LL_FOREACH_SAFE(*p_head, p_curr, p_tmp)
    {
      p_conn = p_curr->p_rp_conn;
      if(p_conn && p_conn->f_conn_ioctl)
      {
          Wilddog_ConnCmd_Arg_T cmd = {NULL, NULL, NULL, NULL, NULL};
          cmd.p_repo = p_curr;
          cmd.p_url = p_curr->p_rp_url;
          (p_conn->f_conn_ioctl)(WILDDOG_CONN_CMD_ONLINE, &cmd, 0);
      }
    }
    return WILDDOG_ERR_NOERR;
}

/*
 * Function:    _wilddog_ct_conn_sync
 * Description: sync function 
 * Input:       arg: the pointer of the repo struct
 *              flag: the flag, not used
 * Output:      N/A
 * Return:      if success, return WILDDOG_ERR_NOERR
*/
STATIC Wilddog_Return_T WD_SYSTEM _wilddog_ct_conn_sync
    (
    void *arg, 
    int flag
    )
{
    Wilddog_Repo_T** p_head = _wilddog_ct_getRepoHead();
    Wilddog_Repo_T* p_curr, *p_tmp;
    Wilddog_Conn_T * p_conn;
    int total=0,offline=0;
#ifdef WILDDOG_FORCE_OFFLINE
    if(TRUE == _wilddog_ct_getOfflineForced()){
        return WILDDOG_ERR_CLIENTOFFLINE;
    }
#endif

    /*1. time increase
     *
     *2. call syncs in all repo
    */
    LL_FOREACH_SAFE(*p_head, p_curr, p_tmp)
    {
        p_conn = p_curr->p_rp_conn;
        if(p_conn && p_conn->f_conn_ioctl)
        {
            Wilddog_ConnCmd_Arg_T cmd = {NULL, NULL, NULL, NULL, NULL};
            cmd.p_repo = p_curr;
            (p_conn->f_conn_ioctl)(WILDDOG_CONN_CMD_TRYSYNC, &cmd, 0);
			_wilddog_syncTime();
            total++;
            if(0 != p_conn->d_conn_sys.d_offline_time){
                offline++;
            }
        }
    }
    if(total == offline){
        //all repo offline, then we offline
        if(TRUE == _wilddog_ct_getOnlineStatus()){
            _wilddog_ct_setOnlineStatus(FALSE);
        }
    }else{
        //online
        if(FALSE == _wilddog_ct_getOnlineStatus()){
            _wilddog_ct_setOnlineStatus(TRUE);
        }
    }
    return WILDDOG_ERR_NOERR;
}

Wilddog_Func_T Wilddog_ApiCmd_FuncTable[WILDDOG_APICMD_MAXCMD + 1] = 
{
    (Wilddog_Func_T)_wilddog_ct_init,
    (Wilddog_Func_T)_wilddog_ct_createRef,
    (Wilddog_Func_T)_wilddog_ct_destroyRef,
    (Wilddog_Func_T)_wilddog_ct_getRef,
    (Wilddog_Func_T)_wilddog_ct_store_setAuth,
    (Wilddog_Func_T)_wilddog_ct_store_query,
    (Wilddog_Func_T)_wilddog_ct_store_set,
    (Wilddog_Func_T)_wilddog_ct_store_push,
    (Wilddog_Func_T)_wilddog_ct_store_remove,
    (Wilddog_Func_T)_wilddog_ct_store_on,
    (Wilddog_Func_T)_wilddog_ct_store_off,
    (Wilddog_Func_T)_wilddog_ct_url_getKey,
    (Wilddog_Func_T)_wilddog_ct_getHost,
    (Wilddog_Func_T)_wilddog_ct_getPath,
    (Wilddog_Func_T)_wilddog_ct_store_disConnSet,
    (Wilddog_Func_T)_wilddog_ct_store_disConnPush,
    (Wilddog_Func_T)_wilddog_ct_store_disConnRmv,
    (Wilddog_Func_T)_wilddog_ct_store_disCancel,
    (Wilddog_Func_T)_wilddog_ct_conn_goOffline,
    (Wilddog_Func_T)_wilddog_ct_conn_goOnline,
    (Wilddog_Func_T)_wilddog_ct_conn_sync,
    NULL
};

#define NOT_USED(_a)


/*
 * Function:    _wilddog_ct_ioctl
 * Description: the ioctl function 
 * Input:       cmd: api command
 *              arg: the arg
 *              flags: the flag, not used
 * Output:      N/A
 * Return:      if success, return WILDDOG_ERR_NOERR
*/
size_t WD_SYSTEM _wilddog_ct_ioctl
    (
    Wilddog_Api_Cmd_T cmd,
    void* arg, 
    int flags
    )
{
    NOT_USED(flags);
    
    if(cmd > WILDDOG_APICMD_MAXCMD || cmd <= 0)
        return 0;
    else
        return (size_t)(Wilddog_ApiCmd_FuncTable[cmd])(arg, flags);     
}

