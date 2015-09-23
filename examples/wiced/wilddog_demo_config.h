/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* define url */
#define TEST_URL "coaps://<appId>.wilddogio.com/"
/* This is the default AP the device will connect to (as a client)*/
#define CLIENT_AP_SSID       "your ssid"
#define CLIENT_AP_PASSPHRASE "your passport"
/* Gpio that control led */
#define DEMO_LED1   (WICED_GPIO_80)

#ifdef __cplusplus
} /*extern "C" */
#endif

