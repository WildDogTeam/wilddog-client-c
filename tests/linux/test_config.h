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
#ifndef TEST_URL
#define TEST_URL         "coap://<your appid1>.wilddogio.com"
#endif
/* used in test_multipleHost.c to Test performance while accessing  multiple url the same time.*/
#ifndef TEST_URL2
#define TEST_URL2         "coap://<your appid2>.wilddogio.com"
#endif
#ifndef TEST_URL3
#define TEST_URL3         "coap://<your appid3>.wilddogio.com"
#endif
#ifndef TEST_URL4
#define TEST_URL4         "coap://<your appid4>.wilddogio.com"
#endif
/* Used in test_step.c to test set auth  */
#ifndef TEST_AUTH
#define TEST_AUTH   "yourauth"
#endif


#endif /* #ifdef WILDDOG_SELFTEST */ 

#endif /*_WILDDOG_TEST_CONFIG_*/

