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
 

/* test url*/
#define TEST_URL         "coap://<your appid>.wilddogio.com"

/* Used in test_step.c to test set auth  */
#define TEST_AUTH 	"yourauth"

#endif /* #ifdef WILDDOG_SELFTEST */ 

#endif /*_WILDDOG_TEST_CONFIG_*/

