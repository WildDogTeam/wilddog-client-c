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

extern void stab_test_cycle(void);
extern void stab_test_fullLoad(void);

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
    {
        u8 m=0, n=0;
        u8 tree_num[3] = {1,2,3};
        u8 request_num[4] = {1,16,32,64};
        ramtest_titile_printf();
    //  ramtest(tree_num[0],1);

        for( m=0; m < 3; m++)
        {
            for( n=0; n < 4; n++)
            {
                ramtest_handle(tree_num[m],request_num[n]);
            }
        }
        ramtest_end_printf();
    }
#endif
#if TEST_TYPE == TEST_TIME
    performtest_all();
#endif
#if TEST_TYPE == TEST_STAB_CYCLE
    stab_test_cycle();
#endif
#if TEST_TYPE == TEST_STAB_FULLLOAD
    stab_test_fullLoad();
#endif
}
