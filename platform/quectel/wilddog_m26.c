

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

typedef struct _WILDDOG_CYCLE_DATA_T
{
    int len;
#if 1
    u8 data[WILDDOG_PROTO_MAXSIZE];
#else
    u8 data[1];
#endif
}Wd_Cycle_Data_T;

typedef struct _WILDDOG_CYCLE_T
{
    u8 head;
    u8 end;
    u8 total;
    u8 reserved;
    int dataSize;
#if 1
    Wd_Cycle_Data_T value[2];
#else
    Wd_Cycle_Data_T value[1];
#endif
}Wilddog_Cycle_T;

int wilddog_ql_init(void);

STATIC Wilddog_Cycle_T l_cycle;
STATIC Wilddog_Cycle_T* l_pCycle = NULL;
STATIC Wilddog_Cycle_T* wd_cycle_init(u8 total, int dataSize)
{
#if 0
    int i;
    Wilddog_Cycle_T* p_cycle = (Wilddog_Cycle_T*)wmalloc(sizeof(Wilddog_Cycle_T) \
                               + total * (sizeof(Wd_Cycle_Data_T*)));
    if(NULL == p_cycle)
        return NULL;
    p_cycle->head = 0;
    p_cycle->end = 0;
    p_cycle->dataSize = dataSize;
    p_cycle->total = total;
    p_cycle->value = (Wd_Cycle_Data_T*)wmalloc((sizeof(Wd_Cycle_Data_T)+ dataSize) * total);
    return p_cycle;
#else
    l_cycle.head = 0;
    l_cycle.end = 0;
    l_cycle.dataSize = WILDDOG_PROTO_MAXSIZE;
    l_cycle.total = 2;
    return &l_cycle;
#endif
}
STATIC void wd_cycle_deinit(Wilddog_Cycle_T* p_cycle)
{
#if 0
    if(!cycle) 
        return;
    
    wfree(p_cycle->value);
    wfree(p_cycle);
#endif
    return;
}
STATIC BOOL wd_cycle_isFull(Wilddog_Cycle_T *cycle)
{
    wilddog_assert(cycle, TRUE);
    
    return (cycle->head == ((cycle->end + 1) % cycle->total)); 
}

STATIC BOOL wd_cycle_isEmpty(Wilddog_Cycle_T *cycle)
{
    wilddog_assert(cycle, TRUE);
    
    return cycle->head == cycle->end;
}

STATIC BOOL wd_cycle_enqueue(Wilddog_Cycle_T *cycle, u8* ptr, int size)
{
    int copySize;
    
    wilddog_debug("cycle = %p", cycle);
    if(wd_cycle_isFull(cycle))
        return FALSE;

    memset(cycle->value[cycle->head].data, 0, cycle->dataSize);
    copySize = size > cycle->dataSize? (cycle->dataSize):(size);
    memcpy(cycle->value[cycle->head].data, ptr, copySize);
    cycle->value[cycle->head].len = copySize;

    cycle->head = (cycle->head + 1) % (cycle->total);
    return TRUE;
}

STATIC int wd_cycle_dequeue(Wilddog_Cycle_T *cycle, u8* ptr, int size)
{
    int copySize;

    wilddog_assert(cycle, 0);
    wilddog_assert(ptr, 0);
    
    wilddog_debug("cycle = %p", cycle);
    if(wd_cycle_isEmpty(cycle))
        return 0;
    copySize = size > (cycle->value[cycle->end].len)? (cycle->value[cycle->end].len):(size);
    memcpy(ptr, cycle->value[cycle->end].data, cycle->value[cycle->end].len);

    cycle->end = (cycle->end + 1) % (cycle->total);
    return copySize;
}

void wd_GPRS_activeCb(u8 contexId, s32 errCode, void* customParam)
{
    if(errCode == SOC_SUCCESS)
    {
        Ql_Debug_Trace("<--CallBack: active GPRS successfully.-->\r\n");
    }  
}

void wd_GPRS_deactiveCb(u8 contextId, s32 errCode, void* customParam )
{
    if (errCode == SOC_SUCCESS)
    {
        Ql_Debug_Trace("<--CallBack: deactived GPRS successfully.-->\r\n"); 
        l_isInitialed = FALSE;
        wilddog_ql_init();
    }
}

ST_PDPContxt_Callback wd_gprs_func_cb = 
{
    wd_GPRS_activeCb,
    wd_GPRS_deactiveCb
};
#if 1
void wd_socket_connect(s32 socketId, s32 errCode, void* customParam )
{
}

void wd_socket_close(s32 socketId, s32 errCode, void* customParam )
{    
    if (errCode == SOC_SUCCESS)
    {
        Ql_Debug_Trace("<--CallBack: close socket successfully.-->\r\n"); 
    }else if(errCode == SOC_BEARER_FAIL)
    {   
        Ql_Debug_Trace("<--CallBack: close socket failure,(socketId=%d,error_cause=%d)-->\r\n",socketId,errCode); 
    }else
    {
        Ql_Debug_Trace("<--CallBack: close socket failure,(socketId=%d,error_cause=%d)-->\r\n",socketId,errCode); 
    }
}

void wd_socket_accept(s32 listenSocketId, s32 errCode, void* customParam )
{  
}

void wd_socket_read(s32 socketId, s32 errCode, void* customParam )
{
    s32 ret;
    u8 srv_address[5] ={0};
    u16 srv_port;
    u8 data[WILDDOG_PROTO_MAXSIZE] = {0};

    Ql_Debug_Trace("<--CallBack: socket read,(sock=%d,error=%d)-->\r\n",socketId,errCode);
    if(errCode)
    {
        Ql_Debug_Trace("<--CallBack: socket read failure,(sock=%d,error=%d)-->\r\n",socketId,errCode);
    }
    else
    {
        
        ret = Ql_SOC_RecvFrom(socketId, data, WILDDOG_PROTO_MAXSIZE, (u32*)srv_address, &srv_port);
        if(ret < 0 && ret != -2)
        {
            Ql_Debug_Trace("<-- Receive data failure,ret=%d.-->\r\n",ret);
            Ql_Debug_Trace("<-- Close socket.-->\r\n");
        }
        else if(ret == -2)
        {
            Ql_Debug_Trace("<-- Receive data failure,ret=%d.-->\r\n",ret);
        }
        else if(ret > 0)
        {
            Ql_Debug_Trace("<-- Receive data success,ret=%d.-->\r\n",ret);
            if(!wd_cycle_enqueue(l_pCycle,data, ret))
            {
                Ql_Debug_Trace("<-- enqueue failure,ret=%d.-->\r\n",ret);
            }
        }
    }
}


void wd_socket_write(s32 socketId, s32 errCode, void* customParam )
{
    Ql_Debug_Trace("<--CallBack: socket write,(sock=%d,error=%d)-->\r\n",socketId,errCode);
    if(errCode)
    {
        Ql_Debug_Trace("<--CallBack: socket write failure,(sock=%d,error=%d)-->\r\n",socketId,errCode);
        Ql_Debug_Trace("<-- Close socket.-->\r\n");
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
#endif
static ST_GprsConfig  gprsCfg;

int wilddog_ql_init(void)
{
    

    s32 simStat = SIM_STAT_NOT_INSERTED;
    s32 cgreg = 0;
    s32 ret = 0;

    u8 * ptr123;
    
    wilddog_debug("initialing!");
    if(TRUE == l_isInitialed)
    {
        wilddog_debug("initialed!");
        return 0;
    }
    l_isInitialed = TRUE;
    /* 1. get sim state */
    RIL_NW_GetSIMCardState(&simStat);
    while(simStat != SIM_STAT_READY)
    {
		wilddog_debug("<--SIM card status is unnormal! %d-->",simStat);
        Ql_Sleep(WD_M26_WAIT_TIME);
        RIL_NW_GetSIMCardState(&simStat);
    }
    
    /* 2. get gprs state */
    RIL_NW_GetGPRSState(&cgreg);
    while((cgreg != NW_STAT_REGISTERED) && (cgreg != NW_STAT_REGISTERED_ROAMING))
    {
        wilddog_debug("STATE_NW_QUERY_STATE CHECKING cgreg = %d", cgreg);
        Ql_Sleep(WD_M26_WAIT_TIME);
        RIL_NW_GetGPRSState(&cgreg);
    }

    /* 3. register gprs status callback */
    Ql_GPRS_Register(WD_CONTEXT_ID, &wd_gprs_func_cb, NULL);
    wilddog_debug("Ql_GPRS_Register");
    //ptr123 = wmalloc(34);
    //wilddog_debug("ptr123 = %p", ptr123);
    //wfree(ptr123);
    /* 4. configures GPRS parameters */
    Ql_strcpy((char*)gprsCfg.apnName, APN_NAME);
    Ql_memset(gprsCfg.apnUserId, 0x0, sizeof(gprsCfg.apnUserId));
    Ql_memset(gprsCfg.apnPasswd, 0x0, sizeof(gprsCfg.apnPasswd));
    gprsCfg.authtype  = 0;
    gprsCfg.Reserved1 = 0;
    gprsCfg.Reserved2 = 0;
    Ql_GPRS_Config(WD_CONTEXT_ID, &gprsCfg);
    wilddog_debug("Ql_GPRS_Config");

    /*5. active gprs, blocked, max cost time is 180s*/
    ret = Ql_GPRS_ActivateEx(WD_CONTEXT_ID, TRUE);
    wilddog_debug("Ql_GPRS_ActivateEx %d", ret);

    if(GPRS_PDP_BEARER_FAIL == ret || GPRS_PDP_INVAL == ret)
        return -1;
#if 0
    /* 6. get dns address */
    ret =Ql_GPRS_GetDNSAddress(0, (u32*)primaryAddr,  (u32*)bkAddr);

    if(GPRS_PDP_SUCCESS != ret)
        return -1;
#endif
    //l_pCycle= wd_cycle_init(2, WILDDOG_PROTO_MAXSIZE);
    //Ql_SOC_Register(wd_soc_func, NULL);
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
    wilddog_debug("ret = %d, ipcount = %d, ip = %x\r\n", ret, ipCount, ipAddr[0]);
    if(SOC_SUCCESS == ret && ipCount > 0)
    {
        Ql_memcpy(addr->ip, &(ipAddr[0]), 4);
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
    ret = Ql_SOC_Create(0, SOC_TYPE_UDP);
    if(ret < 0)
        return -1;
    else
        *socketId = ret + 1;
    wilddog_debug("socket id = %d", *socketId);
    return 0;
}
int wilddog_m26_closeSocket(int socketId)
{
    if(socketId > 0)
        return Ql_SOC_Close(socketId - 1);
    return 0;
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
    wilddog_debug("send: ip = %u.%u.%u.%u, port = %d", addr_in->ip[0], \
                  addr_in->ip[1],addr_in->ip[2],addr_in->ip[3],addr_in->port);
    if(socketId > 0)
    {
        ret = Ql_SOC_SendTo(socketId - 1, (u8*)tosend, tosendLength, \
                 (u32)addr_in->ip, addr_in->port);

        if(ret < 0)
        {
            Ql_GPRS_DeactivateEx(WD_CONTEXT_ID, TRUE);
            l_isInitialed = FALSE;
        }
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
    /*wilddog_debug("recv: ip = %u.%u.%u.%u, port = %d, buf = %p, bufLen = %d", \
                  addr->ip[0], addr->ip[1],addr->ip[2],addr->ip[3],addr->port, \
                  buf, bufLen);*/
    u8 srv_address[5] ={0};
    u16 srv_port;
    
    if(socketId <= 0)
        return -1;
    while(count)
/*    if(!wd_cycle_isEmpty(l_pCycle))*/
    {
    #if 1
        ret = Ql_SOC_RecvFrom(socketId - 1, (u8*)buf, bufLen, (u32*)srv_address, &srv_port);
        if(ret > 0)
        {
            int i;
            wilddog_debug("receive ok!ret = %d\r\n", ret);
            //memcpy((u8*)buf,l_buf, ret > bufLen?(bufLen):(ret));
            for(i = 0; i < ret; i++)
            {
                printf("%02x ", *((u8*)(buf) + i));
            }
            printf("\r\n");

            return ret;
        }
        Ql_Sleep(10);
        count -= 10;
        //wilddog_debug("count = %d, ret = %d", count, ret);
    #else
        return wd_cycle_dequeue(l_pCycle, (u8*)buf, bufLen);
    #endif
    }
    
    return -1;
}

