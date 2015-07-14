#ifndef _WILDDOG_EVENT_H_
#define _WILDDOG_EVENT_H_

 
#include "wilddog_store.h"

#ifdef __cplusplus
extern "C"{
#endif
typedef struct WILDDOG_EVENT_T
{
    struct WILDDOG_STORE_T *p_ev_store;
    Wilddog_Func_T p_ev_cb_on;
    Wilddog_Func_T p_ev_cb_off;
    struct WILDDOG_EVENTNODE_T *p_head;
}Wilddog_Event_T;

typedef struct WILDDOG_EVENTNODE_T
{
    char *path;
    struct WILDDOG_EVENTNODE_T *next,*prev;
    Wilddog_Func_T p_onData;
    void* p_dataArg;

}Wilddog_EventNode_T;


Wilddog_Event_T* _wilddog_event_init(Wilddog_Store_T *p_store);

Wilddog_Event_T* _wilddog_event_deinit(Wilddog_Store_T *p_store);


#ifdef __cplusplus
}
#endif

#endif

