/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: user_main.c
 *
 * Description: This demo  show you how to control a led through the cloud.
 *                   We will creat a led node and push it to server,then subscribe 
 *                   the led node ,so you can change led's value in the server to 
 *                   control the led's status.
 *                      
 *                    
 *           
 * Usage: 
 *          1¡¢Change TEST_URL to your own url in user_config.h:
 *                  like coap://<your appid>.wilddogio.com/TEST_LED, <your appid>
 *                  is the appid of the app you created, and TEST_LED is the path(
 *                  node path) in the app. if the tree like this, <1> is your 
 *                  appid, led-demo is the path.
 *                  
 *                  after runing that demo your data tree in cloud would like that:
 *
 *                  1.wilddogio.com
 *                  |
 *                  + led-demo
 *                      |
 *                      +led:"1"
 *
 *          2¡¢Modification SSID and PASSWORD to the SSID you want to 
 *                  connect in user_config.h:
 *                      
 *          3¡¢If you define DEF_LED_HARDWARE to 1, then you could control
 *                  a real led(there is a example which use GPIO14 in the code, 
 *                  so you should configure  the led gpio function yourself); or you
 *                   could see the data and the print information.
 *          
 *
************************************************************/

 
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_config.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"
    
#include "wilddog.h"



BOOL dns_flag = FALSE;
os_timer_t test_timer1;
os_timer_t test_timer2;
os_timer_t client_timer;




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
    os_printf("recv udp data: %s\n", pusrdata);
}

 

STATIC void WD_SYSTEM 
test_setValueFunc(void* arg, Wilddog_Return_T err)
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
        test_buildtreeFunc(TEST_URL);
    }
}


 /******************************************************************************
 * FunctionName : user_check_ip
 * Description  : check whether get ip addr or not
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void WD_SYSTEM
user_check_ip(void)
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


void user_rf_pre_init(void)
{
}



/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    os_printf("SDK version:%s\n", system_get_sdk_version());
   
    //Set station mode 
    wifi_set_opmode(STATION_MODE); 

    //ESP8266 connect to router
    user_set_station_config();
}


