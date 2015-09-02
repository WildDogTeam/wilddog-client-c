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

#ifndef _WILDDOG_TEST_CONFIG_
#define _WILDDOG_TEST_CONFIG_

#ifdef WILDDOG_SELFTEST

#include "wilddog.h"

/* tree path */
#define TEST_TREE_T_127	"/tree_127"
#define TEST_TREE_T_256	"/tree_256"
#define TEST_TREE_T_576	"/tree_576"
#define TEST_TREE_T_810	"/tree_810"
#define TEST_TREE_T_1044	"/tree_1044"
#define TEST_TREE_T_1280	"/tree_1280"
#define TEST_TREE_ITEMS		5
#define TEST_PROTO_COVER		100

#define TEST_HOST_HEAD		"coap://"
#define TEST_HOST_END		".wilddogio.com"

#define TEST_URL_LEN		(127)	

#define TEST_RAM_URL			"coap://sky.wilddogio.com/ramtest"
#define TEST_PERFORM_URL		"coap://sky.wilddogio.com/performtest"
#define TEST_STAB_CYCLE_URL		"coap://sky.wilddogio.com/stab_cycle"
#define TEST_STAB_SETTEST_URL	"coap://sky.wilddogio.com/set_test"
#define TEST_STEP_URL			"coap://sky.wilddogio.com/step"
#define TEST_MTS_URL			"coap://sky.wilddogio.com/MTS"

#define TEST_MULTIHOST_URL1		"coap://c_test1.wilddogio.com/"
#define TEST_MULTIHOST_URL2		"coap://c_test2.wilddogio.com/"
#define TEST_MULTIHOST_URL3		"coap://c_test3.wilddogio.com/"

/*test subject */ 
#define TEST_STEP_AUTHS 	"yourauth"

extern int test_buildtreeFunc(const char *p_userUrl);

#endif /* #ifdef WILDDOG_SELFTEST */ 

#endif /*_WILDDOG_TEST_CONFIG_*/

