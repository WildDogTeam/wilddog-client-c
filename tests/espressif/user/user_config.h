/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: list_config.h
 *
 * Description: connection functions.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.0        lxs       2015-08-24  Create file.
 *
 */

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#ifdef WILDDOG_SELFTEST
 
/* test type */
/*
 * TEST_RAM : ram test
 * TEST_TIME : time cost test
 * TEST_STAB : get/set/push/delete api cycle stability test
 * TEST_STAB_FULLLOAD : full load stablitiy test
*/
#define TEST_RAM           1
#define TEST_TIME          2
#define TEST_STAB_CYCLE    3
#define TEST_STAB_FULLLOAD 4

#define TEST_TYPE   TEST_RAM

/* test url*/
#define TEST_URL         "coap://<AppId>.wilddogio.com"
#define SSID            "your ssid"
#define PASSWORD       "your password"

/* Used in test_step.c to test set auth  */
#define TEST_AUTH 	"yourauth"

#define TEST_TREE_T_127	"/tree_127"
#define TEST_TREE_T_256	"/tree_256"
#define TEST_TREE_T_576	"/tree_576"
#define TEST_TREE_T_810	"/tree_810"
#define TEST_TREE_T_1044	"/tree_1044"
#define TEST_TREE_T_1280	"/tree_1280"
//#define TEST_TREE_ITEMS		4
#define TEST_PROTO_COVER         100
#define TEST_HOST_HEAD		"coap://"
#define TEST_HOST_END		".wilddogio.com"
#define TEST_URL_LEN		(127)	

#define TREE_SN       0
#define REQ_NUMBER    1
#define DELAY_TIME_MS  50



#endif /* #ifdef WILDDOG_SELFTEST */ 

#endif /*_WILDDOG_TEST_CONFIG_*/

