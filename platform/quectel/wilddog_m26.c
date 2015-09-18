/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: wilddog_m26.c
 *
 * Description: Wilddog porting for quectel M26.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.6        Jimmy.Pan       2015-08-24  Create file.
 *
 */


#include <stdio.h>
#include "wilddog.h"
#include "custom_feature_def.h"
#include "ql_stdlib.h"
#include "ql_common.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_uart.h"
#include "ql_gprs.h"
#include "ql_socket.h"
//#include "ql_network.h"
#include "ql_timer.h"
#include "ril_network.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_telephony.h"
#include "wilddog_m26.h"

#define WD_M26_WAIT_TIME 500
#define WD_CONTEXT_ID 0
STATIC int l_isInitialed = FALSE;

int wilddog_ql_m26_init(void);

void wd_GPRS_activeCb(u8 contexId, s32 errCode, void* customParam)
{
    if(errCode == SOC_SUCCESS)
    {
        wilddog_debug_level(WD_DEBUG_LOG, \
            "<--CallBack: active GPRS successfully.-->");
    }  
}

void wd_GPRS_deactiveCb(u8 contextId, s32 errCode, void* customParam )
{
    if (errCode == SOC_SUCCESS)
    {
        wilddog_debug_level(WD_DEBUG_LOG, \
            "<--CallBack: deactived GPRS successfully.-->"); 
        l_isInitialed = FALSE;
       // wilddog_ql_m26_init();
    }
}

ST_PDPContxt_Callback wd_gprs_func_cb = 
{
    wd_GPRS_activeCb,
    wd_GPRS_deactiveCb
};
void wd_socket_connect(s32 socketId, s32 errCode, void* customParam )
{
}

void wd_socket_close(s32 socketId, s32 errCode, void* customParam )
{    
    if (errCode == SOC_SUCCESS)
    {
        wilddog_debug_level(WD_DEBUG_LOG, \
            "<--CallBack: close socket successfully.-->"); 
    }else if(errCode == SOC_BEARER_FAIL)
    {   
        wilddog_debug_level(WD_DEBUG_ERROR, \
          "<--CallBack: close socket failure,(socketId=%d,error_cause=%d)-->", \
          socketId,errCode); 
    }else
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
          "<--CallBack: close socket failure,(socketId=%d,error_cause=%d)-->", \
          socketId,errCode); 
    }
}

void wd_socket_accept(s32 listenSocketId, s32 errCode, void* customParam )
{  
}

void wd_socket_read(s32 socketId, s32 errCode, void* customParam )
{
    wilddog_debug_level(WD_DEBUG_LOG, \
        "<--CallBack: socket read,(sock=%d,error=%d)-->",socketId,errCode);
    if(errCode)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
            "<--CallBack: socket read failure,(sock=%d,error=%d)-->", \
            socketId,errCode);
    }
}


void wd_socket_write(s32 socketId, s32 errCode, void* customParam )
{
    wilddog_debug_level(WD_DEBUG_LOG, \
        "<--CallBack: socket write,(sock=%d,error=%d)-->",socketId,errCode);
    if(errCode)
    {
        wilddog_debug_level(WD_DEBUG_ERROR, \
            "<--CallBack: socket write failure,(sock=%d,error=%d)-->", \
            socketId,errCode);
        wilddog_debug_level(WD_DEBUG_ERROR, "<-- Close socket.-->");
        return;
    }
}


STATIC ST_SOC_Callback wd_soc_func=
{
    wd_socket_connect,
    wd_socket_close,
    wd_socket_accept,
    wd_socket_read,    
    wd_socket_write
};
static ST_GprsConfig  gprsCfg;

int wilddog_ql_m26_init(void)
{
    s32 simStat = SIM_STAT_NOT_INSERTED;
    s32 cgreg = 0;
    s32 ret = 0;

    if(TRUE == l_isInitialed)
    {
        wilddog_debug_level(WD_DEBUG_LOG, "Already initialed!");
        return 0;
    }
    l_isInitialed = TRUE;
    /* 1. get sim state */
    RIL_NW_GetSIMCardState(&simStat);
    while(simStat != SIM_STAT_READY)
    {
		wilddog_debug_level(WD_DEBUG_LOG, \
            "<--SIM card status is unnormal! %d-->",simStat);
        Ql_Sleep(WD_M26_WAIT_TIME);
        RIL_NW_GetSIMCardState(&simStat);
    }
    
    /* 2. get gprs state */
    RIL_NW_GetGPRSState(&cgreg);
    while((cgreg != NW_STAT_REGISTERED) &&(cgreg != NW_STAT_REGISTERED_ROAMING))
    {
        wilddog_debug_level(WD_DEBUG_LOG, \
            "STATE_NW_QUERY_STATE CHECKING cgreg = %d", cgreg);
        Ql_Sleep(WD_M26_WAIT_TIME);
        RIL_NW_GetGPRSState(&cgreg);
    }

    /* 3. register gprs status callback */
    Ql_GPRS_Register(WD_CONTEXT_ID, &wd_gprs_func_cb, NULL);

    /* 4. configures GPRS parameters */
    Ql_strcpy((char*)gprsCfg.apnName, APN_NAME);
    Ql_memset(gprsCfg.apnUserId, 0x0, sizeof(gprsCfg.apnUserId));
    Ql_memset(gprsCfg.apnPasswd, 0x0, sizeof(gprsCfg.apnPasswd));
    gprsCfg.authtype  = 0;
    gprsCfg.Reserved1 = 0;
    gprsCfg.Reserved2 = 0;
    Ql_GPRS_Config(WD_CONTEXT_ID, &gprsCfg);

    /*5. active gprs, blocked, max cost time is 180s*/
    ret = Ql_GPRS_ActivateEx(WD_CONTEXT_ID, TRUE);
    wilddog_debug_level(WD_DEBUG_LOG, "Ql_GPRS_ActivateEx %d", ret);

    if(GPRS_PDP_BEARER_FAIL == ret || GPRS_PDP_INVAL == ret)
        return -1;

    Ql_SOC_Register(wd_soc_func, NULL);
    wilddog_debug("success");
    return 0;
}

int wilddog_m26_gethostbyname(Wilddog_Address_T* addr,char* host)
{
    s32 ret;
    u32 ipCount = 0;
    u32 ipAddr[5];

    memset(ipAddr, 0, sizeof(u32)* 5);
    ret = Ql_IpHelper_GetIPByHostNameEx(WD_CONTEXT_ID, 0, (u8*)host, &ipCount, \
                                        ipAddr);
    wilddog_debug_level(WD_DEBUG_LOG, \
        "ret = %d, ipcount = %d, ip = %x\r\n", ret, ipCount, ipAddr[0]);
    if(SOC_SUCCESS == ret && ipCount > 0)
    {
        Ql_memcpy(addr->ip, (u8*)&(ipAddr[0]), 4);
        addr->len = 4;
    }
    else
    {
        return -1;
    }
    return 0;
}

int wilddog_m26_openSocket(int* socketId)
{
    s32 ret;
    if(l_isInitialed == FALSE)
        wilddog_ql_m26_init();
    
    ret = Ql_SOC_Create(0, SOC_TYPE_UDP);
    
    if(ret < 0)
        return -1;
    else
        *socketId = ret + 1;
    wilddog_debug_level(WD_DEBUG_LOG, "socket id = %d", *socketId);
    return 0;
}

int wilddog_m26_closeSocket(int socketId)
{
    if(socketId > 0)
        return Ql_SOC_Close(socketId - 1);
    return 0;
}
STATIC BOOL wilddog_isNeedReInit(s32 errcode)
{
    switch(errcode)
    {
        case SOC_LIMIT_RESOURCE:
        case SOC_INVALID_SOCKET:
        case SOC_INVALID_ACCOUNT:
        case SOC_NOTCONN:
            return TRUE;
    }
    return FALSE;
}
int wilddog_m26_send
    (
    int socketId,
    Wilddog_Address_T* addr_in,
    void* tosend,
    s32 tosendLength
    )
{
    s32 ret;
    wilddog_debug_level(WD_DEBUG_LOG, \
      "send: ip = %u.%u.%u.%u, port = %d, tosendlength = %d", addr_in->ip[0], \
      addr_in->ip[1],addr_in->ip[2],addr_in->ip[3],addr_in->port, tosendLength);
    if(socketId > 0)
    {

        ret = Ql_SOC_SendTo(socketId - 1, (u8*)tosend, tosendLength, \
                 (u32)addr_in->ip, addr_in->port);

        if(ret < 0)
        {
            if(TRUE == wilddog_isNeedReInit(ret))
                l_isInitialed = FALSE;
            wilddog_debug_level(WD_DEBUG_WARN, "send error, ret = %d", ret);
        }
        else
            wilddog_debug_level(WD_DEBUG_LOG, "send success!, ret = %d", ret);
        return ret;
    }
    return -1;
}

int wilddog_m26_receive
    (
    int socketId,
    Wilddog_Address_T* addr,
    void* buf,
    s32 bufLen, 
    s32 timeout
    )
{
    s32 count = timeout;
    s32 ret = 0;
    u8 srv_address[5] ={0};
    u16 srv_port;

    wilddog_debug_level(WD_DEBUG_LOG, \
        "recv: ip = %u.%u.%u.%u, port = %d, buf = %p, bufLen = %d", \
        addr->ip[0], addr->ip[1],addr->ip[2],addr->ip[3],addr->port, \
        buf, bufLen);

    if(socketId <= 0)
        return -1;
    while(count > 0)
    {
        ret = Ql_SOC_RecvFrom(socketId - 1, (u8*)buf, bufLen, \
            (u32*)srv_address, &srv_port);
        if(ret > 0)
        {
            if(memcmp((u8*)&srv_address, addr->ip, addr->len) ||\
                addr->port != srv_port)
            {
                wilddog_debug("receive packet from wrong ip address or port.");
                memset(buf, 0, bufLen);
            }
            else
                return ret;
        }


        Ql_Sleep(10);
        count -= 10;
        wilddog_debug_level(WD_DEBUG_LOG, "count = %d, ret = %d", count, ret);
    }
    
    return -1;
}

