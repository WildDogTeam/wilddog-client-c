#ifndef _WILDDOG_EVENT_H_
#define _WILDDOG_EVENT_H_

 
#include "wilddog_store.h"

#ifdef __cplusplus
extern "C"{
#endif

/* 
* if dpath contains spath,  0, 
* if spath contains dpath,  1,
* if spath equal dpath,  2
* else 3.
*/
#define WD_EVENT_PATHCONTAIN_DCS   0
#define WD_EVENT_PATHCONTAIN_SCD   1
#define WD_EVENT_PATHCONTAIN_SED   2
#define WD_EVENT_PATHCONTAIN_OTHER 3


typedef struct WILDDOG_EVENT_T
{
    struct WILDDOG_STORE_T *p_ev_store;
    Wilddog_Func_T p_ev_cb_on;
    Wilddog_Func_T p_ev_cb_off;
    struct WILDDOG_EVENTNODE_T *p_head;
}Wilddog_Event_T;

typedef enum ON_OFF_FLAG
{
    OFF_FLAG = 0,
    ON_FLAG
}ON_OFF_FLAG_T;


typedef struct WILDDOG_EVENTNODE_T
{
    struct WILDDOG_EVENTNODE_T *next;
    Wilddog_Url_T * p_url;
    Wilddog_Func_T p_onData;
    void* p_dataArg;
    ON_OFF_FLAG_T flag;
}Wilddog_EventNode_T;


Wilddog_Event_T* _wilddog_event_init(Wilddog_Store_T *p_store);

Wilddog_Event_T* _wilddog_event_deinit(Wilddog_Store_T *p_store);


#ifdef __cplusplus
}
#endif

#endif

