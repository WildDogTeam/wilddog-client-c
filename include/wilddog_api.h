/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_api.h
 *
 * Description: Wilddog's API header files.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        Jimmy.Pan       2015-05-15  Create file.
 * 0.4.6        Jimmy.Pan       2015-09-06  Add notes.
 *
 */

#ifndef _WILDDOG_API_H_
#define _WILDDOG_API_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "wilddog_config.h"
#include "wilddog.h"

/*****************************************************************************\
|*******************************Common  API************************************|
\*****************************************************************************/

/*
 * Function:    wilddog_increaseTime
 * Description: Optional, user defined time increase. Because SDK need a 
 *              time to ageing request packets, if you have a requirement
 *              in accuracy,  you should call this func, in a timer or other 
 *              methods.
 * Input:       ms: time want to increase, in mili second
 * Output:      N/A
 * Return:      N/A
*/
extern void wilddog_increaseTime(u32 ms);


/*****************************************************************************\
|*********************************APP API**************************************|
\*****************************************************************************/
/*
 * Function:    wilddog_initWithUrl
 * Description: Init a wilddog client. A client is the path in the HOST tree.
 * Input:       url: A url such as coaps://<appid>.wilddogio.com/<path>
 * Output:      N/A
 * Return:      Id of the client.
*/
extern Wilddog_T wilddog_initWithUrl(Wilddog_Str_T *url);
/*
 * Function:    wilddog_destroy
 * Description: Destory a wilddog client.
 * Input:       p_wilddog: a pointer which point to the client id.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_destroy(Wilddog_T *p_wilddog);
/*
 * Function:    wilddog_getParent
 * Description: Get the client's parent.
 * Input:       p_wilddog: a pointer to the client
 * Output:      N/A
 * Return:      A pointer point to the client's parent, if the client is root ,
 *              return NULL.
*/
extern Wilddog_T wilddog_getParent(Wilddog_T wilddog);
/*
 * Function:    wilddog_getRoot
 * Description: Get the client's Root.
 * Input:       wilddog: Id of the client.
 * Output:      N/A
 * Return:      an id of your client's root.
*/
extern Wilddog_T wilddog_getRoot(Wilddog_T wilddog);
/*
 * Function:    wilddog_getChild
 * Description: Get the client's child.
 * Input:       wilddog: Id of the client.
 * Output:      N/A
 * Return:      an id of your client's child.
 * Others:      The sdk do not check wether the child is really in the server 
 *              or not, only create it.
*/
extern Wilddog_T wilddog_getChild
    (
    Wilddog_T wilddog,
    Wilddog_Str_T * childName
    );
/*
 * Function:    wilddog_getKey
 * Description: Get the client's key(the node's name).
 * Input:       wilddog: Id of the client.
 * Output:      N/A
 * Return:      a pointer point to a name string.
 * Others:      N/A
*/
extern Wilddog_Str_T *wilddog_getKey(Wilddog_T wilddog);
/*
 * Function:    wilddog_getHost
 * Description: Get the client's host.
 * Input:       wilddog: Id of the client.
 * Output:      N/A
 * Return:      a pointer point to a host string like "aaa.wilddogio.com" .
 * Others:      N/A
*/
extern Wilddog_Str_T *wilddog_getHost(Wilddog_T wilddog);
/*
 * Function:    wilddog_getPath
 * Description: Get the client's path.
 * Input:       wilddog: Id of the client.
 * Output:      N/A
 * Return:      a pointer point to a path string like "/a/b/c" .
 * Others:      N/A
*/
extern Wilddog_Str_T *wilddog_getPath(Wilddog_T wilddog);

/*
 * Function:    wilddog_getValue
 * Description: Get the data of the client from server.
 * Input:       wilddog: the id of wilddog client.
 *              callback: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_getValue
    (
    Wilddog_T wilddog,
    onQueryFunc callback, 
    void* arg
    );
/*
 * Function:    wilddog_setValue
 * Description: Post the data of the client to server.
 * Input:       wilddog: Id of the client.
 *              p_node: a point to node(Wilddog_Node_T structure), you can
 *                      create a node tree by call node APIs.
 *              callback: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_setValue
    (
    Wilddog_T wilddog,
    Wilddog_Node_T *p_node,
    onSetFunc callback,
    void* arg
    );
/*
 * Function:    wilddog_push
 * Description: Push the data of the client to server.
 * Input:       wilddog: Id of the client.
 *              p_node: a point to node(Wilddog_Node_T structure), you can 
 *                      create a node tree by call node APIs.
 *              callback: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_push
    (
    Wilddog_T wilddog,
    Wilddog_Node_T *p_node,
    onPushFunc callback, 
    void* arg
    );
/*
 * Function:    wilddog_removeValue
 * Description: Remove the data of the client from server.
 * Input:       wilddog: Id of the client.
 *              callback: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_removeValue
    (
    Wilddog_T wilddog,
    onRemoveFunc callback,
    void* arg
    );
/*
 * Function:    wilddog_addObserver
 * Description: Subscibe the client's data change, if data changed, server 
 *              will notify the client.
 * Input:       wilddog: Id of the client.
 *              event: Event type, see the struct.
 *              onDataChange: The callback function called when the server 
 *                          sends a data change packet.
 *              dataChangeArg: The arg defined by user, if you do not need, 
 *                          can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_addObserver
    (
    Wilddog_T wilddog,
    Wilddog_EventType_T event,
    onEventFunc onDataChange,
    void* dataChangeArg
    );
/*
 * Function:    wilddog_removeObserver
 * Description: Unsubscibe the client's data change.
 * Input:       wilddog: Id of the client.
 *              event: Event type, see the struct.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_removeObserver
    (
    Wilddog_T wilddog,
    Wilddog_EventType_T event
    );
/*
 * Function:    wilddog_onDisconnectSetValue
 * Description: Set the device's disconnect action to cloud, when the device is 
 *              offline, the value will be set to the cloud.
 * Input:       wilddog: the client id.
 *              p_node: a point to node(Wilddog_Node_T structure), you can
 *                      create a node tree by call node APIs, can free after
 *                      this function.
 *              callback: the callback function called when the server returns 
 *                      or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means success , others fail.
 * Others:      N/A
*/

extern Wilddog_Return_T wilddog_onDisconnectSetValue
    (
    Wilddog_T wilddog, 
    Wilddog_Node_T *p_node, 
    onDisConnectFunc callback, 
    void* arg
    );
/*
 * Function:    wilddog_onDisconnectPush
 * Description: Set the device's disconnect action to cloud, when the device is 
 *              offline, the value will be push to the cloud.
 * Input:       wilddog: the client id.
 *              p_node: a point to node(Wilddog_Node_T structure), you can
 *                      create a node tree by call node APIs, can free after
 *                      this function.
 *              callback: the callback function called when the server returns 
 *                        or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means success , others fail.
 * Others:      N/A
*/
extern Wilddog_Return_T wilddog_onDisconnectPush
    (
    Wilddog_T wilddog, 
    Wilddog_Node_T *p_node, 
    onDisConnectFunc callback, 
    void* arg
    );
/*
 * Function:    wilddog_onDisconnectRemoveValue
 * Description: Set the device's disconnect action to cloud, when the device is 
 *              offline, the value will be push to the cloud.
 * Input:       wilddog:  Id of the client.
 *              callback: the callback function called when the server returns 
 *                        or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means success , others fail.
 * Others:      N/A
*/
extern Wilddog_Return_T wilddog_onDisconnectRemoveValue
    (
    Wilddog_T wilddog, 
    onDisConnectFunc callback, 
    void* arg
    );
/*
 * Function:    wilddog_cancelDisconnectOperations
 * Description: Cancel the wilddog client's disconnect actions.
 * Input:       wilddog:  Id of the client.
 *              callback: the callback function called when the server returns 
 *                        or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means success , others fail.
 * Others:      N/A
*/
extern Wilddog_Return_T wilddog_cancelDisconnectOperations
    (
    Wilddog_T wilddog, 
    onDisConnectFunc callback, 
    void* arg
    );
/*
 * Function:    wilddog_goOffline
 * Description: let the device offline.
 * Input:       N/A
 * Output:      N/A
 * Return:      0 means success , others fail.
 * Others:      N/A
*/
extern Wilddog_Return_T wilddog_goOffline(void);
/*
 * Function:    wilddog_goOnline
 * Description: let the device online.
 * Input:       N/A
 * Output:      N/A
 * Return:      0 means success , others fail.
 * Others:      N/A
*/
extern Wilddog_Return_T wilddog_goOnline(void);

/*
 * Function:    wilddog_auth
 * Description: Set the auth data to a host(such as aaa.wilddogio.com).
 * Input:       p_host: a pointer to host .
 *              p_auth: the auth data
 *              len: the auth data length
 *              onAuth: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_auth
    (
    Wilddog_Str_T * p_host,
    u8 *p_auth, 
    int len, 
    onAuthFunc onAuth, 
    void* args
    );
/*
 * Function:    wilddog_unauth
 * Description: Unauth to a host(such as aaa.wilddogio.com).
 * Input:       p_host: a pointer to host .
 *              onAuth: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_unauth
    (
    Wilddog_Str_T * p_host, 
    onAuthFunc onAuth, 
    void* args
    );

/*
 * Function:    wilddog_trySync
 * Description: When called, try to sync interal time and receive data from 
 *              internet.
 * Input:       N/A
 * Output:      N/A
 * Return:      N/A
*/
extern void wilddog_trySync(void);



/*****************************************************************************\
|*********************************Node  API*************************************|
\*****************************************************************************/
/*
 * Function:    wilddog_node_createObject
 * Description: Create a node, type is OBJECT.
 * Input:       key:    the pointer to the node's key (can be NULL).
 * Output:      N/A
 * Return:      if success, returns pointer points to the node, else return NULL.
 * Others:      N/A
*/

extern Wilddog_Node_T * wilddog_node_createObject(Wilddog_Str_T* key);

/*
 * Function:    wilddog_node_createUString
 * Description: Create a node, type is UTF-8 string.
 * Input:       key:    The pointer to the node's key (can be NULL).
 *              value:  pointer to the string.
 * Output:      N/A
 * Return:      if success, returns pointer points to the node, else return NULL.
 * Others:      utf-8 string, end with \0
*/
extern Wilddog_Node_T * wilddog_node_createUString
    (
    Wilddog_Str_T* key,
    Wilddog_Str_T *value
    );

/*
 * Function:    wilddog_node_createBString
 * Description: Create a node, type is byte string(binary buffer).
 * Input:       key:    The pointer to the node's key (can be NULL).
 *              value:  pointer to the string.
 *              len:    The length of the string.
 * Output:      N/A
 * Return:      if success, returns pointer points to the node, else return NULL.
 * Others:      byte string, means a byte buffer, may not end with \0
*/
extern Wilddog_Node_T * wilddog_node_createBString
    (
    Wilddog_Str_T* key, 
    u8 *value, 
    int len
    );
/*
 * Function:    wilddog_node_createFloat
 * Description: Create a node, type is float(8-bit machine is 32 bits else 64 bits).
 * Input:       key:    The pointer to the node's key (can be NULL).
 *              num:    float value.
 * Output:      N/A
 * Return:      if success, returns pointer points to the node, else return NULL.
 * Others:      N/A
*/
extern Wilddog_Node_T * wilddog_node_createFloat
    (
    Wilddog_Str_T* key, 
    wFloat num
    );
/*
 * Function:    wilddog_node_createNum
 * Description: Create a node, type is integer(32 bits).
 * Input:       key:    The pointer to the node's key (can be NULL).
 *              num:    integer value.
 * Output:      N/A
 * Return:      if success, returns pointer points to the node, else return NULL.
 * Others:      N/A
*/
extern Wilddog_Node_T * wilddog_node_createNum
    (
    Wilddog_Str_T* key, 
    s32 num
    );
/*
 * Function:    wilddog_node_createNull
 * Description: Create a node, type is NULL.
 * Input:       key:    The pointer to the node's key (can be NULL).
 * Output:      N/A
 * Return:      if success, returns pointer points to the node, else return NULL.
 * Others:      N/A
*/
extern Wilddog_Node_T * wilddog_node_createNull(Wilddog_Str_T* key);
/*
 * Function:    wilddog_node_createTrue
 * Description: Create a node, type is TRUE.
 * Input:       key:    The pointer to the node's key (can be NULL).
 * Output:      N/A
 * Return:      if success, returns pointer points to the node, else return NULL.
 * Others:      N/A
*/
extern Wilddog_Node_T * wilddog_node_createTrue(Wilddog_Str_T* key);
/*
 * Function:    wilddog_node_createFalse
 * Description: Create a node, type is FALSE.
 * Input:       key:    The pointer to the node's key (can be NULL).
 * Output:      N/A
 * Return:      if success, returns pointer points to the node, else return NULL.
 * Others:      N/A
*/
extern Wilddog_Node_T * wilddog_node_createFalse(Wilddog_Str_T* key);
/*
 * Function:    wilddog_node_getValue
 * Description: Set a node's value.
 * Input:       node:   The pointer to the node.
 * Output:      len:    The length of the value.
 * Return:      if success, returns point of the value, else return NULL.
 * Others:      N/A
*/
Wilddog_Str_T* wilddog_node_getValue
    (
    Wilddog_Node_T *node, 
    int * len
    );

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
Wilddog_Return_T wilddog_node_setValue
    (
    Wilddog_Node_T *node, 
    u8 *value, 
    int len
    );

/*
 * Function:    wilddog_node_addChild
 * Description: add a node(and it's children) to a parent.
 * Input:       parent: the pointer to the parent which want to insert.
 *              child:  the pointer to the node.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
 * Others:      head insert, so now parent->p_wn_child is child.
*/
extern Wilddog_Return_T wilddog_node_addChild
    (
    Wilddog_Node_T *parent, 
    Wilddog_Node_T *child
    );
/*
 * Function:    wilddog_node_delete
 * Description: delete a node(and it's children).
 * Input:       head:   the pointer to the node.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
 * Others:      N/A
*/
extern Wilddog_Return_T wilddog_node_delete( Wilddog_Node_T *head);
/*
 * Function:    wilddog_node_clone
 * Description: copy a node tree.
 * Input:       head:   the pointer to the node.
 * Output:      N/A
 * Return:      pointer points to the cloned node tree.
 * Others:      N/A
*/
extern Wilddog_Node_T * wilddog_node_clone(const Wilddog_Node_T *head);
/*
 * Function:    wilddog_node_find
 * Description: find a node by <path>.
 * Input:       root:   the pointer to the root.
 *              path:   the path(start from the <root>).
 * Output:      N/A
 * Return:      if found, returns pointer points to the node, else return NULL.
 * Others:      N/A
*/
extern Wilddog_Node_T *wilddog_node_find
    (
    Wilddog_Node_T *root, 
    char *path
    );

#ifdef __cplusplus
}
#endif

#endif /*_WILDDOG_API_H_*/

