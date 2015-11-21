/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: demo.c
 *
 * Description: This demo show you how to set value through server. We will
 *              creat a led node and push it to server.
 *                  
 *
 *
 * Usage: 
 *          1. Copy SDK to MICO SDK's root folder, probable path may be 
 *                  <SDK_MiCO_v2.3.0\wilddog-client-c>.
 *          2. In <wilddog-client-c\examples\mxchip\demo.c>:
 *
 *                  default TEST_URL is coap://<your appid>.wilddogio.com/[path]
 *                  , change <your appid> to your <appid>, which is the appid 
 *                  of the app you created, and path is the path(
 *                  node path) in the app. if the tree like followed , <1> is 
 *                  your appid, <led1> is path.
 *                  
 *                  after runing demo, your data tree in cloud would like that:
 *
 *                  1.wilddogio.com
 *                  |
 *                  +led1:1
 *
 *          3. Modify CLIENT_AP_SSID and CLIENT_AP_PASSPHRASE to the SSID you 
 *             want to connect in demo.c.
 *
 *          
 *
 * History:
 * Version      Author              Date        Description
 *
 * 0.7.0        baikal             2015-11-20   Create file.
 *
 */
#include "MICO.h"
#include "wilddog.h"
#include "wilddog_endian.h"


#define TEST_URL "coaps://<appId>.wilddogio.com/"

#define CLIENT_AP_SSID       "your ssid"
#define CLIENT_AP_PASSPHRASE "your passport"

static mico_semaphore_t wait_sem = NULL;

#define udp_unicast_log(M, ...) custom_log("UDP", M, ##__VA_ARGS__)

#define wifi_station_log(M, ...) custom_log("WIFI", M, ##__VA_ARGS__)

static void micoNotify_ConnectFailedHandler(OSStatus err, void* inContext)
{
    wifi_station_log("join Wlan failed Err: %d", err);
}

static void micoNotify_WifiStatusHandler(WiFiEvent event,  void* inContext)
{
    switch (event) 
    {
        case NOTIFY_STATION_UP:
            wifi_station_log("Station up");
            mico_rtos_set_semaphore(&wait_sem);
            break;
        case NOTIFY_STATION_DOWN:
            wifi_station_log("Station down");
            break;
        default:
            break;
    }
}

STATIC void setValue_callback(void* arg, Wilddog_Return_T err)
{
                        
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("setValue error!");
        return;
    }
    wilddog_debug("setValue success!");
    *(BOOL*)arg = TRUE;
    return;
}

void wilddog_thread(void *arg)
{
    char url[] = TEST_URL;
    BOOL isFinish = FALSE;

    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL, *p_head = NULL;
    
    p_head = wilddog_node_createObject(NULL);
    p_node = wilddog_node_createUString((Wilddog_Str_T*)"led1",(Wilddog_Str_T*)"1");
    wilddog_node_addChild(p_head, p_node);
     
    wilddog = wilddog_initWithUrl((Wilddog_Str_T*)url);
     
    if(0 == wilddog)
    {
        wilddog_debug("new wilddog error");
        return ;
    }

    wilddog_setValue(wilddog,p_head,setValue_callback, \
                     (void*)&isFinish);
    
    while(1)
    {
        if(isFinish == TRUE)
            break;
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);

    mico_rtos_delete_thread(NULL);
}



int application_start( void )
{
    OSStatus err = kNoErr;
    network_InitTypeDef_adv_st  wNetConfigAdv={0};
    IPStatusTypedef outNetpara;
    char ipstr[16];
    MicoInit( );
  
    mico_rtos_init_semaphore( &wait_sem,1 );
  
    /* Register user function when wlan connection status is changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler, NULL );
    require_noerr( err, exit ); 
  
    /* Register user function when wlan connection is faile in one attempt */
    err = mico_system_notify_register( mico_notify_WIFI_CONNECT_FAILED, (void *)micoNotify_ConnectFailedHandler, NULL );
    require_noerr( err, exit );
  
    /* Initialize wlan parameters */
    strcpy((char*)wNetConfigAdv.ap_info.ssid, CLIENT_AP_SSID);   /* wlan ssid string */
    strcpy((char*)wNetConfigAdv.key, CLIENT_AP_PASSPHRASE);                /* wlan key string or hex data in WEP mode */
    wNetConfigAdv.key_len = strlen(CLIENT_AP_PASSPHRASE);                       /* wlan key length */
    wNetConfigAdv.ap_info.security = SECURITY_TYPE_AUTO;          /* wlan security mode */
    wNetConfigAdv.ap_info.channel = 0;                            /* Select channel automatically */
    wNetConfigAdv.dhcpMode = DHCP_Client;                         /* Fetch Ip address from DHCP server */
    wNetConfigAdv.wifi_retry_interval = 100;                      /* Retry interval after a failure connection */
  
    /* Connect Now! */
    wifi_station_log("connecting to %s...", wNetConfigAdv.ap_info.ssid);
    micoWlanStartAdv(&wNetConfigAdv);
 
    mico_rtos_get_semaphore( &wait_sem, MICO_WAIT_FOREVER );
    wifi_station_log( "wifi connected successful" );

    micoWlanGetIPStatus(&outNetpara, Station);
    wifi_station_log("ip is %s\n", outNetpara.ip);
  
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "wilddog", wilddog_thread, 0x1000, NULL );
    require_noerr_string( err, exit, "ERROR: Unable to start the wilddog thread." );
  
exit:
    mico_rtos_delete_thread(NULL);
    return err;
}

