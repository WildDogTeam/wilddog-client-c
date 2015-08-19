
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

STATIC int wilddog_gps_getValueByIndex(char* src, int index, char* dst, int *len)
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
    char tmp[24];
    Ql_memset(tmp, 0, 24);
    Ql_sprintf(tmp,"start %d,end %d index %d\r\n", start, end, index);
    Ql_UART_Write(UART_PORT1, (u8*)tmp, strlen((const char*)tmp));
    if(start >= 0)
    {
        dst = &src[start];
    
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

static void CallBack_UART3_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
    //Ql_UART_Write(UART_PORT1,"hello3\r\n",strlen("hello3\r\n"));
    switch (msg)
    {
    case EVENT_UART_READY_TO_READ:
        {

           s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart3, sizeof(m_RxBuf_Uart3));
           
           if (totalBytes > 0)
           {
               char* ptr = NULL;
               ptr = wilddog_gps_findLineByKey(m_RxBuf_Uart3, "$GPGGA");
               if(ptr)
               {
                    char* dst = NULL;
                    int len = 0;
                    wilddog_gps_getValueByIndex(ptr, 1, dst,&len);
                    Ql_UART_Write(UART_PORT1, (u8*)ptr, strlen(ptr));
                    Ql_UART_Write(UART_PORT1, "\r\nptr = ok\r\n", strlen("\r\nptr = ok\r\n"));
                    if(dst && len)
                    {
                        Ql_UART_Write(UART_PORT1, "value:\r\n", strlen("value:\r\n"));
                        Ql_UART_Write(UART_PORT1, (u8*)dst, len);
                        Ql_UART_Write(UART_PORT1, "\r\n", strlen("\r\n"));
                    }
               }
               else
               {
                    Ql_UART_Write(UART_PORT1, "cannot find\r\n", strlen("cannot find\r\n"));
               }
               //Ql_UART_Write(UART_PORT1,m_RxBuf_Uart3,sizeof(m_RxBuf_Uart3));
           }
           break;
        }
    case EVENT_UART_READY_TO_WRITE:
        break;
    default:
        break;
    }
}


void proc_main_task(s32 taskId)
{
    ST_MSG msg;

    // Register & open UART port
	//Ql_UART_Register(UART_PORT1, CallBack_UART_Hdlr, NULL);
	//Ql_UART_Open(UART_PORT1, 115200, FC_NONE);
	//Ql_UART_Register(UART_PORT2, CallBack_UART_Hdlr, NULL);
	//Ql_UART_Open(UART_PORT2, 115200, FC_NONE);
	//Ql_UART_Register(UART_PORT3, CallBack_UART3_Hdlr, NULL);
	//Ql_UART_Open(UART_PORT3, 9600, FC_NONE);
    //Ql_UART_Write(UART_PORT1,"hello www\r\n",strlen("hello www\r\n"));
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
    *(BOOL*)arg = TRUE;

    if(p_snapshot)
        wilddog_debug_printnode(p_snapshot);
    printf("\ngetValue success!\n");

    return;
}

extern int wilddog_ql_init(void);
static void Callback_Timer(u32 timerId, void* param)
{

}
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

void wilddog_task(s32 TaskId)
{
    Wilddog_T wilddog;
    Wilddog_Node_T* head = NULL;
    ST_MSG msg;
    int ret;
	Ql_UART_Register(UART_PORT1, CallBack_UART_Hdlr, NULL);
	Ql_UART_Open(UART_PORT1, 115200, FC_NONE);
	Ql_UART_Register(UART_PORT2, CallBack_UART_Hdlr, NULL);
	Ql_UART_Open(UART_PORT2, 115200, FC_NONE);
	Ql_UART_Register(UART_PORT3, CallBack_UART3_Hdlr, NULL);
	//Ql_UART_Open(UART_PORT3, 9600, FC_NONE);
    //Ql_UART_Write(UART_PORT1,"hello www\r\n",strlen("hello www\r\n"));
    //Ql_Timer_Register(TIMER_ID_USER_START, Callback_Timer, NULL);
    //Ql_Timer_Start(TIMER_ID_USER_START, 100, TRUE);

	wilddog_ql_init();
    wilddog_debug("wilddog_task success\r\n");
#if 1
    wilddog = wilddog_initWithUrl((u8*)"coap://embedded.wilddogio.com");
    //ret = wilddog_getValue(wilddog, test_getValueFunc, NULL);
    head = wilddog_node_createTrue(NULL);
    wilddog_setValue(wilddog, head, test_setValueFunc, NULL);
    wilddog_node_delete(head);
    while(1)
    {
        //Ql_OS_GetMessage(&msg);
        wilddog_trySync();
        //wilddog_debug("hello");
    }
    wilddog_destroy(&wilddog);
#endif
    return;
}

