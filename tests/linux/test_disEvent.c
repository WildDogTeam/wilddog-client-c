/*
File name : test_disOffLineOnline.c
测试接口:      
       1.wilddog_onDisconnectSetValue
       2.wilddog_onDisconnectPush
       3.wilddog_onDisconnectRemoveValue
       4.wilddog_cancelDisconnectOperations
       5.wilddog_goOffline
       6.wilddog_goOnline
                              
 其他项:        1.建立多个host，并分别请求。
 测试原理:
 1.)   测试wilddog_onDisconnectSetValue: 
        --> 调用wilddog_removeValue 清理节点，为测试清场.
        -->调用wilddog_onDisconnectSetValue 接口设置节点为{"set":"disset"}
        -->调用wilddog_goOffline 并等待10s.
        -->调用wilddog_goOnline ，client将发送auth请求重新上线.
        -->调用wilddog_getValue,获取接节点数据，为{"set":"disset"}则wilddog_onDisconnectSetValue 成功否则失败。
 2)  wilddog_onDisconnectPush 接口测试同上。
 3)  wilddog_onDisconnectRemoveValue 接口测试:
        -->调用wilddog_setValue 设置节点为{"rm":"disrm"}
        -->调用wilddog_onDisconnectRemoveValue
        -->调用wilddog_goOffline 并等待10s.
        -->调用wilddog_goOnline ，client将发送auth请求重新上线.
        -->调用wilddog_getValue,获取接节点数据，为空成功否则 失败。
 4) wilddog_cancelDisconnectOperations 接口测试 
        --> 调用wilddog_setValue 设置节点为{"cancle":"discancle"}
        --> 调用wilddog_onDisconnectRemoveValue
        --> 收取到wilddog_onDisconnectRemoveValue 回应后调用wilddog_cancelDisconnectOperations；
        --> 调用wilddog_goOffline 并等待10s.
        --> 调用wilddog_goOnline ，client将发送auth请求重新上线.
        --> 调用wilddog_getValue,获取接节点数据，为{"cancle":"discancle"}成功否则 失败。
 5) wilddog_goOffline 和wilddog_goOnline 测试:
        --> 上1)~4)均成功则wilddog_goOffline 和wilddog_goOnline 成功否则失败。

注意:
    该测试在20s内完成，ping机制对该测试没有任何影响。

操作步骤:

        1) 修改test_config.h 中的url 为测试使用的url
                TEST_URL   :  wilddog_onDisconnectSetValue 使用的url。
                TEST_URL2 :  wilddog_onDisconnectPush 使用的url
                TEST_URL3 :  wilddog_onDisconnectRemove 使用的url.
                TEST_URL4 :  wilddog_cancelDisconnectOperations 使用的url


      
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h> 
#include "wilddog.h"
#include "test_config.h"

#define TEST_URL_LEN    (128)
#define TEST_DIS_PATH       "/dis"
#define TEST_DIS_SET_KEY    "set"
#define TEST_DIS_SET_VAL    "disset"
#define TEST_DIS_PUSH_KEY   "push"
#define TEST_DIS_PUSH_VAL   "dispush"
#define TEST_DIS_RM_KEY     "rm"
#define TEST_DIS_RM_VAL     "disrm"
#define TEST_DIS_CNC_KEY    "cancle"
#define TEST_DIS_CNC_VAL    "discancle"

typedef enum{
    _DIS_CMD_SET,
    _DIS_CMD_PUSH,
    _DIS_CMD_RM,
    _DIS_CMD_CNC,
    _DIS_CMD_MAX
}_TEST_DIS_CMD_T;

int disSet_OK =FALSE,disPush_OK=FALSE,disRm_OK=FALSE,disCnc_OK=FALSE;
int goOfflineOk = FALSE,goOnlineOk = FALSE;
int dis_finish = 0;

STATIC void dis_callback(void* arg, Wilddog_Return_T err)
{
	*(BOOL*)arg = TRUE;
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("failed! error = %d",err);
        return ;
    }

    return;
}

STATIC BOOL dis_judgeNode(const Wilddog_Node_T* p_sNode, const u8 *p_key,const u8 *p_val)
{

    wilddog_assert(p_sNode,FALSE);
    if( p_sNode->d_wn_type == WILDDOG_NODE_TYPE_UTF8STRING && \
        strcmp((char*)p_key,(char*)p_sNode->p_wn_key) == 0   )
    {
        if( strcmp((char*)p_key,(char*)p_sNode->p_wn_key) == 0)
            return TRUE;
    }

    return FALSE;
}
STATIC BOOL dis_findNode(const Wilddog_Node_T* p_sNode, const u8 *p_key,const u8 *p_val)
{
    BOOL res = FALSE;
    wilddog_assert(p_sNode,FALSE);

    res = dis_judgeNode(p_sNode,p_key,p_val);
    if(res == TRUE)
        return res;

    res = dis_judgeNode(p_sNode->p_wn_child,p_key,p_val);
    if(res == TRUE)
        return res;

    res = dis_judgeNode(p_sNode->p_wn_next,p_key,p_val);
    if(res == TRUE)
            return res;

    res = dis_judgeNode(p_sNode->p_wn_prev,p_key,p_val);
    if(res == TRUE)
        return res;    

    res = dis_judgeNode(p_sNode->p_wn_parent,p_key,p_val);
    if(res == TRUE)
        return res;  

    return res;
}

STATIC void dis_getCallBack
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err
    )
{
    if(arg == NULL)
        return ;
    
    int cmd = *((int*)arg);
    dis_finish++;

    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("getValue fail error = %d!",err);
        return;
    }
#if 0    
    if(p_snapshot)
        wilddog_debug_printnode(p_snapshot);
#endif    
    switch(cmd)
    {
       case _DIS_CMD_SET:
           if(p_snapshot)
                disSet_OK = dis_findNode(p_snapshot,(u8*)TEST_DIS_SET_KEY,(u8*)TEST_DIS_SET_VAL);
            break;
       case _DIS_CMD_PUSH:
        
            if( p_snapshot && p_snapshot->p_wn_child && \
                p_snapshot->p_wn_child->p_wn_child )
                disPush_OK = dis_findNode(p_snapshot->p_wn_child->p_wn_child,(u8*)TEST_DIS_PUSH_KEY,(u8*)TEST_DIS_PUSH_VAL);
            break;
       case _DIS_CMD_RM:
        
            if( p_snapshot && p_snapshot->p_wn_child == NULL &&\
                p_snapshot->p_wn_next == NULL && p_snapshot->p_wn_prev == NULL)
                disRm_OK = TRUE;

            break;
      case _DIS_CMD_CNC:
            
            if(p_snapshot)
                disCnc_OK = dis_findNode(p_snapshot,(u8*)TEST_DIS_CNC_KEY,(u8*)TEST_DIS_CNC_VAL);
    }
            
    return;
}

int main(void)
{
    int offtime=0;
    int argSet =_DIS_CMD_SET,argPush=_DIS_CMD_PUSH,argRm=_DIS_CMD_RM,argCnc = _DIS_CMD_CNC;
    Wilddog_Return_T res_set = 0,res_push = 0,res_rm = 0,res_cnc=0;
    Wilddog_T wd_set =0,wd_push = 0,wd_rm = 0,wd_cnc=0;
    Wilddog_Node_T *p_set_hd = NULL,*p_push_hd = NULL,*p_rm_hd=NULL,*p_cnc_hd=NULL;
    Wilddog_Node_T *p_set_n=NULL,*p_push_n = NULL,*p_rm_n = NULL,*p_cnc_n=NULL;
    BOOL setFinish = FALSE,pushFinish = FALSE,rmFinish = FALSE,cncFinish=FALSE;
    u8 p_setUrl[TEST_URL_LEN],p_pushUrl[TEST_URL_LEN],p_rmUrl[TEST_URL_LEN],p_cncUrl[TEST_URL_LEN];
	char resultKey[2][10]= {{"Fail"}, {"Success"}};
	
    memset(p_setUrl,0,TEST_URL_LEN);
    memset(p_pushUrl,0,TEST_URL_LEN);
    memset(p_rmUrl,0,TEST_URL_LEN);
    memset(p_cncUrl,0,TEST_URL_LEN);


    sprintf((char*)p_setUrl,"%s%s",TEST_URL,TEST_DIS_PATH);    
    sprintf((char*)p_pushUrl,"%s%s",TEST_URL2,TEST_DIS_PATH);
    sprintf((char*)p_rmUrl,"%s%s",TEST_URL3,TEST_DIS_PATH);    
    sprintf((char*)p_cncUrl,"%s%s",TEST_URL4,TEST_DIS_PATH);
    
    printf("\n\tStart dis_event\\Offline\\Online test:\n\n");
    printf("\tdis set url %s \n",p_setUrl);
    printf("\tdis push url %s\n",p_pushUrl);    
    printf("\tdis rm url %s\n",p_rmUrl);    
    printf("\tcancel dis url %s\n",p_cncUrl);

    /* init.*/
    wd_set = wilddog_initWithUrl((Wilddog_Str_T *)p_setUrl);
    wd_push = wilddog_initWithUrl((Wilddog_Str_T *)p_pushUrl);
    wd_rm = wilddog_initWithUrl((Wilddog_Str_T *)p_rmUrl);
    wd_cnc = wilddog_initWithUrl((Wilddog_Str_T *)p_cncUrl);
    /*build setting node.*/
    
    /*Create an node which type is an object*/
    p_set_hd = wilddog_node_createObject(NULL);
    p_push_hd = wilddog_node_createObject(NULL);    
    p_rm_hd = wilddog_node_createObject(NULL);
    p_cnc_hd = wilddog_node_createObject(NULL);
    
    /*Create an node which type is UTF-8 Sring*/
    p_set_n = wilddog_node_createUString((Wilddog_Str_T *)TEST_DIS_SET_KEY, \
                                        (Wilddog_Str_T *)TEST_DIS_SET_VAL);
    
    p_push_n = wilddog_node_createUString((Wilddog_Str_T *)TEST_DIS_PUSH_KEY, \
                                          (Wilddog_Str_T *)TEST_DIS_PUSH_VAL);
    
    p_rm_n = wilddog_node_createUString((Wilddog_Str_T *)TEST_DIS_RM_KEY, \
                                          (Wilddog_Str_T *)TEST_DIS_RM_VAL);

    p_cnc_n = wilddog_node_createUString((Wilddog_Str_T *)TEST_DIS_CNC_KEY, \
                                            (Wilddog_Str_T *)TEST_DIS_CNC_VAL);

    /*Add p_node to p_head, then p_node is the p_head's child node*/
    wilddog_node_addChild(p_set_hd, p_set_n);
    wilddog_node_addChild(p_push_hd, p_push_n);
    wilddog_node_addChild(p_rm_hd, p_rm_n);
    wilddog_node_addChild(p_cnc_hd, p_cnc_n);

    /*set value for test.*/
      
    /*set disconnection event.*/
    res_set  = wilddog_removeValue(wd_set,dis_callback,(void*)&setFinish);
    res_push = wilddog_removeValue(wd_push,dis_callback,(void*)&pushFinish);
    res_rm  = wilddog_setValue(wd_rm,p_rm_hd,dis_callback,(void*)&rmFinish);
    res_cnc = wilddog_setValue(wd_cnc,p_cnc_hd,dis_callback,(void*)&cncFinish);
    while(1)
    {

        wilddog_trySync();
        if( setFinish == TRUE &&    \
            pushFinish == TRUE &&   \
            rmFinish == TRUE && \
            cncFinish == TRUE )
                break;
    }    
    /*set disconnection event.*/
    res_set  = wilddog_onDisconnectSetValue(wd_set,p_set_hd,dis_callback,(void*)&setFinish);
    res_push = wilddog_onDisconnectPush(wd_push,p_push_hd,dis_callback,(void*)&pushFinish);
    res_rm  = wilddog_onDisconnectRemoveValue(wd_rm,dis_callback,(void*)&rmFinish);
    res_cnc = wilddog_onDisconnectRemoveValue(wd_cnc,dis_callback,(void*)&cncFinish);

    if( res_set < 0 || res_push < 0 || res_rm < 0 || res_cnc)
    {
        printf("\twilddog_onDisconnectSetValue error\t%d\t%d\t%d\t%d\n",res_set,res_push,res_rm,res_cnc);
        return -1;
    }
    while(1)
    {
        
        wilddog_trySync();
        if( setFinish == TRUE &&    \
            pushFinish == TRUE &&   \
            rmFinish == TRUE && \
            cncFinish == TRUE )
                break;
    }
    /* cancel dis event.*/
    cncFinish = FALSE;
    res_cnc = wilddog_cancelDisconnectOperations(wd_cnc,dis_callback,(void*)&cncFinish);
    while(1)
    {
        wilddog_trySync();
        if(cncFinish == TRUE)
            break;
    }
    /* offline.*/
    wilddog_goOffline();
    /*wait 10 sec.*/
    while(1)
    {
        sleep(1);
        wilddog_trySync();
        if(++offtime>10)
            break;
    }
    /*online again*/
    wilddog_goOnline();
    /*load */
    res_set = wilddog_getValue(wd_set,dis_getCallBack,(void*)&argSet);
    res_push = wilddog_getValue(wd_push,dis_getCallBack,(void*)&argPush);    
    res_rm = wilddog_getValue(wd_rm,dis_getCallBack,(void*)&argRm);    
    res_cnc = wilddog_getValue(wd_cnc,dis_getCallBack,(void*)&argCnc);
    while(1)
    {
        wilddog_trySync();
        if(dis_finish == _DIS_CMD_MAX)
            break;
    }
    
    wilddog_node_delete(p_set_hd);
    wilddog_node_delete(p_push_hd);
    wilddog_node_delete(p_rm_hd);
    wilddog_node_delete(p_cnc_hd);

    wilddog_destroy(&wd_set);
    wilddog_destroy(&wd_push);
    wilddog_destroy(&wd_rm);
    wilddog_destroy(&wd_cnc);
    
    /*printf result.*/
    printf("\n\tdistest result:\n\n");
    printf("\tdis setValue on %s",p_setUrl);
	printf(" %s\n", resultKey[disSet_OK]);
    
    printf("\tdis push on %s",p_pushUrl);
	printf(" %s\n", resultKey[disPush_OK]);

    printf("\tdis removeValue on %s",p_rmUrl);
	printf(" %s\n", resultKey[disRm_OK]);

    printf("\tdis cancel on %s",p_cncUrl);
	printf(" %s\n", resultKey[disCnc_OK]);

    if( disCnc_OK == TRUE && \
        disSet_OK == TRUE && \
        disPush_OK == TRUE && \
        disRm_OK == TRUE )
            goOfflineOk = TRUE,goOnlineOk = TRUE;

    printf("\tgoOffline %s, goOnLine", resultKey[goOfflineOk]);
	printf(" %s\n\n", resultKey[goOfflineOk]);
    
    if( goOfflineOk == FALSE)
            return -1;
    else
        return 0;

}
