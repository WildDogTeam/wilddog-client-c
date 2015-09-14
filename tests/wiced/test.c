/*
 * test.c
 *
 *  Created on: 2015-06-13 lixiongsheng
 */

#include <string.h>
#include "wiced.h"
#include "wilddog.h"
#include "wiced_tcpip.h"
#include "wilddog_api.h"
#include "wifi_config_dct.h"

#include "test_lib.h"
//#include "test_config.h"

extern void stab_test_cycle(void);
extern void stab_test_fullLoad(void);

extern int test_perform(void);
extern int test_ram(void);

/**
 *  Application start
 */
void application_start( void )
{
    /* Initialise the device */
    wiced_init();
    /* Run the main application function */
    wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
#if TEST_TYPE == TEST_RAM
	test_ram();
#endif
#if TEST_TYPE == TEST_TIME
    test_perform();
#endif
#if TEST_TYPE == TEST_STAB_CYCLE
    stab_test_cycle();
#endif
#if TEST_TYPE == TEST_STAB_FULLLOAD
    stab_test_fullLoad();
#endif
}
