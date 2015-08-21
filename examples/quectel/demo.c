
#include <stdio.h>
#include "custom_feature_def.h"
#include "ql_stdlib.h"
#include "ql_common.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_uart.h"
#include "ql_uart.h"
#include "ql_gprs.h"
#include "ql_socket.h"
//#include "ql_network.h"
#include "ql_timer.h"
#include "ril_network.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_telephony.h"

#include "wilddog.h"
/*****************************************************************
* UART Param
******************************************************************/
#define SERIAL_RX_BUFFER_LEN  2048
static u8 m_RxBuf_Uart[SERIAL_RX_BUFFER_LEN];
static u8 m_RxBuf_Uart3[SERIAL_RX_BUFFER_LEN];
static u8 l_start = FALSE;

static Wilddog_Node_T *l_head =NULL;
static Wilddog_Node_T * l_lng = NULL;
static Wilddog_Node_T * l_lat = NULL;
static int l_needset = FALSE;
volatile static int l_canset = TRUE;
STATIC int wilddog_gps_getValueByIndex(char* src, int index, char** dst, int *len)
{
    int count = 0;
    int start = -1;
    int end = -1;
    int i;
    char* data = NULL;
    wilddog_assert(src, -1);
    wilddog_assert(len, -1);
    for(i = 0; i < strlen((const char*)src); i++)
    {
        if(src[i] == '\r' && src[i+1] == '\n')
            break;

        if(count == index && start < 0)
        {
            start = i;
        }
        if(count == index + 1)
        {
            end = i - 2;
            break;
        }
        
        if(src[i] == ',')
            count++;
    }
    if(start >= 0)
    {
        *dst = &src[start];
    
        if(end < 0)
            *len = strlen((const char*)src) - start;
        else
            *len = end - start;
        return 0;
    }
    
    return -1;
}

STATIC char* wilddog_gps_findLineByKey(char* src, char *key)
{
    char* ptr = NULL;
    wilddog_assert(key, NULL);
    wilddog_assert(src, NULL);

    ptr = Ql_strstr(src, key);
    return ptr;
}

static void proc_handle(u8 *pData,s32 len)
{
	
}

static s32 ReadSerialPort(Enum_SerialPort port, /*[out]*/u8* pBuffer, /*[in]*/u32 bufLen)
{
    s32 rdLen = 0;
    s32 rdTotalLen = 0;
    if (NULL == pBuffer || 0 == bufLen)
    {
        return -1;
    }
    Ql_memset(pBuffer, 0x0, bufLen);
    while (1)
    {
        rdLen = Ql_UART_Read(port, pBuffer + rdTotalLen, bufLen - rdTotalLen);
        if (rdLen <= 0)  // All data is read out, or Serial Port Error!
        {
            break;
        }
        rdTotalLen += rdLen;
        // Continue to read...
    }
    if (rdLen < 0) // Serial Port Error!
    {
        Ql_Debug_Trace("<--Fail to read from port[%d]-->\r\n", port);
        return -99;
    }
    return rdTotalLen;
}

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
    char temp[24];
    Ql_memset(temp, 0, 24);
    //Ql_UART_Write(UART_PORT1,"in call",strlen("in call"));
    switch (msg)
    {
    case EVENT_UART_READY_TO_READ:
        {
           
           s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart, sizeof(m_RxBuf_Uart));
           Ql_sprintf(temp, "%d hello read\r\n", port);
           
           if (totalBytes > 0)
           {
               proc_handle(m_RxBuf_Uart,sizeof(m_RxBuf_Uart));
           }
           break;
        }
    case EVENT_UART_READY_TO_WRITE:
        Ql_sprintf(temp, "%d hello write\r\n", port);
        break;
    default:
        break;
    }
    //Ql_UART_Write(UART_PORT1,temp,strlen(temp));
}

STATIC wFloat wd_atof(char* data)
{
    wFloat val= 0;
    int len = strlen(data);
    int i = 0;
    int cal = 1;
    wFloat m,n;
    volatile int integer = 0;
    volatile int decimal = 0;
    int count = 0;
    while(data[i] < '0' || data[i] > '9')
    {
        i++;
    }
    while(data[i] != '.' && data[i] >= '0' && data[i] <= '9')
    {
        integer *= 10;
        integer += data[i] - '0';
        i++;
    }
    i++;
    while(data[i] >= '0' && data[i] <= '9' && i < len)
    {
        decimal *= 10;
        decimal += data[i] - '0';
        i++;
        count++;
    }
    for(i = 0; i < count; i++)
    {
        cal *= 10;
    }
    val = (wFloat)decimal / (wFloat)cal;
    val += integer;

    m = integer / 100;
    n = (val - m * 100)/60;
    val = m + n;
    
    return val;
    
}

static void CallBack_UART3_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
    switch (msg)
    {
    case EVENT_UART_READY_TO_READ:
        {

           s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart3, sizeof(m_RxBuf_Uart3));
           
           if (totalBytes > 0)
           {
               char* ptr = NULL;
               ptr = wilddog_gps_findLineByKey(m_RxBuf_Uart3, "$GPRMC");
               if(ptr)
               {
                    char* dst = NULL;
                    int len = 0;

                    l_canset = FALSE;
                    wilddog_gps_getValueByIndex(ptr, 3, &dst,&len);
                    if(dst && len > 0)
                    {
                        /*lat*/
                        wFloat data;
                        char tmp[20];
                        memset(tmp, 0, 20);
                        memcpy(tmp, dst, len);
                        tmp[len] = 0;
                        data = wd_atof(tmp);
                        //wilddog_debug("lat:%f", data);
                        //wilddog_node_setValue(l_lat, (u8*)&data,sizeof(data));
                    }
                    wilddog_gps_getValueByIndex(ptr, 5, &dst,&len);
                    if(dst && len > 0)
                    {
                        /*lat*/
                        wFloat data;
                        char tmp[20];
                        memset(tmp, 0, 20);
                        memcpy(tmp, dst, len);
                        tmp[len] = 0;
                        data = wd_atof(tmp);
                        //wilddog_debug("lng %f", data);
                        //wilddog_node_setValue(l_lng, (u8*)&data,sizeof(data));
                    }
                    l_canset = TRUE;
               }
               else
               {
                    wilddog_debug("cannot find");
               }
               //wilddog_debug("%s",m_RxBuf_Uart3);
           }
           break;
        }
    case EVENT_UART_READY_TO_WRITE:
        break;
    default:
        break;
    }
}

static void Callback_Timer(u32 timerId, void* param)
{
    //wilddog_debug("in timer!");
    l_needset = TRUE;
}

void proc_main_task(s32 taskId)
{
    ST_MSG msg;

	Ql_UART_Register(UART_PORT1, CallBack_UART_Hdlr, NULL);
	Ql_UART_Open(UART_PORT1, 115200, FC_NONE);
	Ql_UART_Register(UART_PORT2, CallBack_UART_Hdlr, NULL);
	Ql_UART_Open(UART_PORT2, 115200, FC_NONE);
	//Ql_UART_Register(UART_PORT3, CallBack_UART3_Hdlr, NULL);
	//Ql_UART_Open(UART_PORT3, 9600, FC_NONE);

    //Ql_Timer_Register(TIMER_ID_USER_START, Callback_Timer, NULL);
    //Ql_Timer_Start(TIMER_ID_USER_START, 1000, TRUE);
    
    Ql_Debug_Trace("<--OpenCPU: Wilddog Client.-->\r\n");
    while(TRUE)
    {
        Ql_OS_GetMessage(&msg);
        switch(msg.message)
        {
#ifdef __OCPU_RIL_SUPPORT__
        case MSG_ID_RIL_READY:
            Ql_Debug_Trace("<-- RIL is ready -->\r\n");
            Ql_RIL_Initialize();
            l_start = TRUE;
            break;
#endif
        default:
            break;
        }
    }
}
STATIC void test_getValueFunc
    (
    const Wilddog_Node_T* p_snapshot, 
    void* arg, 
    Wilddog_Return_T err
    )
{
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("getValue fail!");
        return;
    }
    if(p_snapshot)
        wilddog_debug_printnode(p_snapshot);
    printf("\r\ngetValue success!\r\n");

    return;
}

extern int wilddog_ql_init(void);

STATIC void test_setValueFunc(void* arg, Wilddog_Return_T err)
{

    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("setValue error!");
        return;
    }
    wilddog_debug("setValue success!");
    return;
}
extern int wilddog_m26_openSocket(int* socketId);
extern int wilddog_m26_send
    (
    int socketId,
    Wilddog_Address_T* addr_in,
    void* tosend,
    s32 tosendLength
    );

void wilddog_task(s32 TaskId)
{
    Wilddog_T wilddog;
    Wilddog_Node_T* head = NULL, *node = NULL;
    ST_MSG msg;
    int ret;
    int count = 0;
    while(l_start == FALSE)
        Ql_Sleep(10);
    /*
    head = wilddog_node_createObject(NULL);
    node = wilddog_node_createFloat("Lng", 113.566839);
    l_lng = node;
    wilddog_node_addChild(head,node);
    node = wilddog_node_createFloat("Lat", 22.324112);
    l_lat = node;
    wilddog_node_addChild(head,node);
    l_head = head;
*/
	wilddog_ql_init();
    wilddog_debug("wilddog_task success");
/*
    s32 socketid = 0;
    Wilddog_Address_T addr;
    u8 data[] = {0x44, 0x02, 0xf4, 0x2d, 0x2e, 0x00, 0x00, 0x00,
                 0x3d, 0x09, 0x65, 0x6d, 0x62, 0x65, 0x64, 0x64,
                 0x65, 0x64, 0x2e, 0x77, 0x69, 0x6c, 0x64, 0x64,
                 0x6f, 0x67, 0x69, 0x6f, 0x2e, 0x63, 0x6f, 0x6d,
                 0x83, 0x2e, 0x63, 0x73};
    wilddog_m26_openSocket(&socketid);
    addr.len = 4;
    addr.ip[0] = 211;
    addr.ip[1] = 151;
    addr.ip[2] = 208;
    addr.ip[3] = 196;
    addr.port = 5683;
    
    wilddog_m26_send(socketid, &addr, data, sizeof(data));*/
    wilddog = wilddog_initWithUrl((u8*)"coap://embedded.wilddogio.com/bus1/coordinate");
    //ret = wilddog_getValue(wilddog, test_getValueFunc, NULL);
    
    //wilddog_setValue(wilddog, head, test_setValueFunc, NULL);
    //wilddog_node_delete(head);
    while(1)
    {
        if(TRUE == l_needset && TRUE == l_canset)
        {
            l_needset = FALSE;
            //wilddog_setValue(wilddog, head, test_setValueFunc, NULL);
            //wilddog_getValue(wilddog, test_getValueFunc, NULL);
            //wilddog_unauth("embedded.wilddogio.com", test_setValueFunc, NULL);
            wilddog_debug("count = %d", count++);
        }
        //Ql_OS_GetMessage(&msg);
        wilddog_trySync();
        //wilddog_debug("hello");
    }
    wilddog_destroy(&wilddog);
    wilddog_node_delete(head);
    return;
}

