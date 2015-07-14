
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
 * Function:    wilddog_timeIncrease
 * Description: Optional, user defined time increase. Because sdk need a 
 *              time to ageing request packets, if you have a requirement
 *              in accuracy,  you should call this func, in a timer or other 
 *              methods.
 * Input:       ms: time want to increase, in mili second
 * Output:      N/A
 * Return:      N/A
*/
extern void wilddog_timeIncrease(u32 ms);
/*
 * Function:    wilddog_init
 * Description: Init sdk.
 * Input:       N/A
 * Output:      N/A
 * Return:      N/A
*/
extern void wilddog_init(void);

/*****************************************************************************\
|*********************************APP API**************************************|
\*****************************************************************************/
/*
 * Function:    wilddog_new
 * Description: Init a wilddog client. A client is the path in the HOST tree.
 * Input:       url: A url such as coaps://<appid>.wilddogio.com/<path>
 * Output:      N/A
 * Return:      Id of the client.
*/
extern Wilddog_T wilddog_new(Wilddog_Str_T *url);
/*
 * Function:    wilddog_destroy
 * Description: Destory a wilddog client.
 * Input:       p_wilddog: a pointer which point to the client id.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_destroy(Wilddog_T *p_wilddog);
/*
 * Function:    wilddog_setAuth
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
extern Wilddog_Return_T wilddog_setAuth(Wilddog_Str_T * p_host, 
                                            u8 *p_auth, 
                                            int len, 
                                            onAuthFunc onAuth, 
                                            void* args);
/*
 * Function:    wilddog_query
 * Description: Get the data of the client from server.
 * Input:       wilddog: the id of wilddog client.
 *              callback: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_query(Wilddog_T wilddog, 
                                            onQueryFunc callback, 
                                            void* arg);
/*
 * Function:    wilddog_set
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
extern Wilddog_Return_T wilddog_set(Wilddog_T wilddog, 
                                        Wilddog_Node_T *p_node, 
                                        onSetFunc callback, 
                                        void* arg);
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
extern Wilddog_Return_T wilddog_push(Wilddog_T wilddog, 
                                            Wilddog_Node_T *p_node, 
                                            onPushFunc callback, 
                                            void* arg);
/*
 * Function:    wilddog_remove
 * Description: Remove the data of the client from server.
 * Input:       wilddog: Id of the client.
 *              callback: the callback function called when the server returns 
 *                      a response or send fail.
 *              args: the arg defined by user, if you do not need, can be NULL.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_remove(Wilddog_T wilddog, 
                                            onRemoveFunc callback, 
                                            void* arg);
/*
 * Function:    wilddog_on
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
extern Wilddog_Return_T wilddog_on(Wilddog_T wilddog, 
                                        Wilddog_EventType_T event, 
                                        onEventFunc onDataChange, 
                                        void* dataChangeArg);
/*
 * Function:    wilddog_off
 * Description: Unsubscibe the client's data change.
 * Input:       wilddog: Id of the client.
 *              event: Event type, see the struct.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
*/
extern Wilddog_Return_T wilddog_off(Wilddog_T wilddog, 
                                        Wilddog_EventType_T event);
/*
 * Function:    wilddog_trySync
 * Description: When called, try to sync.
 * Input:       N/A
 * Output:      N/A
 * Return:      N/A
*/
extern void wilddog_trySync(void);
/*
 * Function:    wilddog_getParent
 * Description: Get the client's parent.
 * Input:       p_wilddog: a pointer to the client
 * Output:      N/A
 * Return:      a pointer point to your client's parent.
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
extern Wilddog_T wilddog_getChild(Wilddog_T wilddog, 
                                            Wilddog_Str_T * childName);
/*
 * Function:    wilddog_getKey
 * Description: Get the client's key(the node's name).
 * Input:       wilddog: Id of the client.
 * Output:      N/A
 * Return:      a pointer point to a name string(should be freed by user).
 * Others:      N/A
*/
extern Wilddog_Str_T *wilddog_getKey(Wilddog_T wilddog);

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
/*utf-8 string, end with \0*/
/*
 * Function:    wilddog_node_createUString
 * Description: Create a node, type is UTF-8 string.
 * Input:       key:    The pointer to the node's key (can be NULL).
 *              value:  pointer to the string.
 * Output:      N/A
 * Return:      if success, returns pointer points to the node, else return NULL.
 * Others:      N/A
*/
extern Wilddog_Node_T * wilddog_node_createUString
    (
    Wilddog_Str_T* key,
    Wilddog_Str_T *value
    );
/*byte string, means a byte buffer, may not end with \0*/
/*
 * Function:    wilddog_node_createBString
 * Description: Create a node, type is byte string(binary buffer).
 * Input:       key:    The pointer to the node's key (can be NULL).
 *              value:  pointer to the string.
 *              len:    The length of the string.
 * Output:      N/A
 * Return:      if success, returns pointer points to the node, else return NULL.
 * Others:      N/A
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
 * Function:    wilddog_node_setKey
 * Description: Set a node's key.
 * Input:       node:   The pointer to the node.
 *              key:    The pointer to the node's key.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
 * Others:      N/A
*/
extern Wilddog_Return_T wilddog_node_setKey
    (
    Wilddog_Node_T *node, 
    Wilddog_Str_T *key
    );
/*
 * Function:    wilddog_node_getKey
 * Description: Get a node's key.
 * Input:       node:   The pointer to the node.
 * Output:      N/A
 * Return:      if success, returns pointer points to the key, else return NULL.
 * Others:      N/A
*/
extern const Wilddog_Str_T *wilddog_node_getKey(Wilddog_Node_T *node);
/*
 * Function:    wilddog_node_setType
 * Description: Set a node's type.
 * Input:       node:   The pointer to the node.
 *              type:   The new type of the node.
 * Output:      N/A
 * Return:      if success, returns 0, else return negative number.
 * Others:      N/A
*/
extern Wilddog_Return_T wilddog_node_setType(Wilddog_Node_T *node, u8 type);
/*
 * Function:    wilddog_node_getType
 * Description: Get a node's type.
 * Input:       node:   The pointer to the node.
 * Output:      N/A
 * Return:      if success, returns type of the node, else return negative number.
 * Others:      N/A
*/
extern u8 wilddog_node_getType(Wilddog_Node_T *node);
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
extern Wilddog_Return_T wilddog_node_setValue
    (
    Wilddog_Node_T *node, 
    u8 *value, 
    int len
    );
/*
 * Function:    wilddog_node_getValue
 * Description: Set a node's value.
 * Input:       node:   The pointer to the node.
 * Output:      len:    The length of the value.
 * Return:      if success, returns point of the value, else return NULL.
 * Others:      N/A
*/
extern Wilddog_Str_T* wilddog_node_getValue
    (
    Wilddog_Node_T *node, 
    int * len
    );

/*
 * Function:    wilddog_node_add
 * Description: add a node(and it's children) to a parent.
 * Input:       parent: the pointer to the parent which want to insert.
 *              child:  the pointer to the node.
 * Output:      N/A
 * Return:      0 means succeed, negative number means failed.
 * Others:      N/A
*/
extern Wilddog_Return_T wilddog_node_add
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

