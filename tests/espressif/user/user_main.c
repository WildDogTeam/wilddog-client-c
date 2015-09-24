/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
    

#include "wilddog.h"
#include "user_config.h"

BOOL dns_flag = FALSE;
os_timer_t test_timer1;
os_timer_t test_timer2;
os_timer_t client_timer;
extern void stab_test_cycle(void);





 /******************************************************************************
  * FunctionName : user_udp_recv_cb
  * Description  : Processing the received udp packet
  * Parameters   : arg -- Additional argument to pass to the callback function
  *                pusrdata -- The received data (or NULL when the connection has been closed!)
  *                length -- The length of received data
  * Returns      : none
 *******************************************************************************/
STATIC void WD_SYSTEM 
user_udp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{   
}

 

STATIC void WD_SYSTEM  test_setValueFunc(void* arg, Wilddog_Return_T err)
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



void WD_SYSTEM 
fake_main(void)
{
    if(!dns_flag)
    {
        gethost();		
    }
    else
    {
#if TEST_TYPE == TEST_STAB_CYCLE
    	ramtest_init(1,1);
    	stab_titlePrint();
    	printf("%s\n",TEST_URL);
        os_timer_disarm(&test_timer1);
        os_timer_setfn(&test_timer1, (os_timer_func_t *)stab_test_cycle, NULL);
        os_timer_arm(&test_timer1, 1000, 0); 
#else
        test_buildtreeFunc(TEST_URL);
#endif
    }
}


 /******************************************************************************
 * FunctionName : user_check_ip
 * Description  : check whether get ip addr or not
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void WD_SYSTEM user_check_ip(void)
{
    struct ip_info ipconfig;

    //disarm timer first
    os_timer_disarm(&client_timer);

    //get ip info of ESP8266 station
    wifi_get_ip_info(STATION_IF, &ipconfig);

    if (wifi_station_get_connect_status() == STATION_GOT_IP &&  \
            ipconfig.ip.addr != 0) 
    {
        os_printf("got ip !!! \r\n");

        os_timer_disarm(&test_timer1);
        os_timer_setfn(&test_timer1, (os_timer_func_t *)fake_main, NULL);
        os_timer_arm(&test_timer1, 1000, 0);
    } 
    else 
    {
        if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
            wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
            wifi_station_get_connect_status() == STATION_CONNECT_FAIL)) 
        {
            os_printf("connect fail !!! \r\n");
        } 
        else 
        {
            //re-arm timer to check ip
            os_timer_setfn(&client_timer, \
                (os_timer_func_t *)user_check_ip, NULL);
            os_timer_arm(&client_timer, 100, 0);
        }
    }
}


/******************************************************************************
 * FunctionName : user_set_station_config
 * Description  : set the router info which ESP8266 station will connect to 
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void WD_SYSTEM 
user_set_station_config(void)
{
    // Wifi configuration 
    char ssid[32] = SSID;
    char password[64] = PASSWORD;

    struct station_config stationConf; 

    //need not mac address
    stationConf.bssid_set = 0; 
   
    //Set ap settings 
    os_memcpy(&stationConf.ssid, ssid, 32); 
    os_memcpy(&stationConf.password, password, 64); 
    wifi_station_set_config(&stationConf); 

    //set a timer to check whether got ip from router succeed or not.
    os_timer_disarm(&client_timer);
    os_timer_setfn(&client_timer, (os_timer_func_t *)user_check_ip, NULL);
    os_timer_arm(&client_timer, 100, 0);

}


void WD_SYSTEM user_rf_pre_init(void)
{
}



/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void WD_SYSTEM user_init(void)
{
    os_printf("SDK version:%s\n", system_get_sdk_version());
   
    //Set station mode 
    wifi_set_opmode(STATION_MODE); 

    //ESP8266 connect to router
    user_set_station_config();
}


