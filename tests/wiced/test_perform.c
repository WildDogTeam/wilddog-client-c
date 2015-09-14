#include <string.h>
#include "wiced.h"
#include "wilddog.h"
#include "wiced_tcpip.h"
#include "wilddog_api.h"
#include "wifi_config_dct.h"

#include "test_lib.h"
#include "test_config.h"


static char *p_tree_url[TEST_TREE_ITEMS] = 
					{
						TEST_TREE_T_127,
						TEST_TREE_T_256,
						TEST_TREE_T_576,
						TEST_TREE_T_810,
						TEST_TREE_T_1044,
						TEST_TREE_T_1280,
					};

static  u32 d_tree_num[TEST_TREE_ITEMS] = {127,256,576,810,1044,1280};

int test_perform(void)
{

#ifdef WILDDOG_SELFTEST
	int res = 0;
	u8 url[sizeof(TEST_URL)+20];

	if( (res = test_buildtreeFunc(TEST_URL) ) < 0 )
			return res;
	{
			u8 tree_m=0,n=0,d=0;	
			u8 request_num[4] = {1,16,32,64};
			u32 delay_tm[5] = {0,50,100,250,500};
				
			performtest_titile_printf();
			
			for(d=0; d<2; d++)
			{
				for(n=0; n<4; n++)
				{
					for(tree_m=0; tree_m< TEST_TREE_ITEMS; tree_m++)
					{
						memset(url,0,sizeof(url));
						sprintf(url,"%s%s",TEST_URL,p_tree_url[tree_m]);
						performtest_handle(delay_tm[d],url,\
							d_tree_num[tree_m],request_num[n]);
#ifdef WILDDOG_PORT_TYPE_WICED
						wiced_rtos_delay_milliseconds(2000);
#endif

					}
				}
			}
			
			performtest_end_printf();
			return 0;
	}
#endif
	return 0;

}
