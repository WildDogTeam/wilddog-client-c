#Test
##1.文件结构和说明
	
    	├── test_config.h
		├── test.c
    	├── test_perform.c
    	├── test_ram.c
		├── wifi_config_dct.h
    	└── stab_function.c
    
*	`test_config.h` : 测试配置
*	`wifi_config_dct.h` ： WICED板wifi相关配置
*	`test_perform.c` : 性能测试API
*	`test_ram.c` : 内存占用测试API
*	`stab_function.c` : 稳定性测试API
*	`test.c` : wiced测试工程的main

##2.配置说明
每个测试项均需要在一个URL建立树，修改并获取以测试其准确性和稳定性。用户可以修改`test_config.h`配置测试使用的URL：

- `TEST_URL` ： 测试时使用的URL。
- `TEST_URL2` ： 多云端测试其中一个URL。
- `TEST_URL3` ： 多云端测试其中一个URL。
- `TEST_AUTH` ： 与`TEST_URL`建立会话的Auth。

在本测试中`wiced`工作于`station`模式，用户需要在`wifi_config_dct.h`中配置以下信息：

- `CLIENT_AP_SSID` ： 连接的`SSID`名称
- `CLIENT_AP_PASSPHRASE` ： SSID的密码
- `TEST_TYPE` ： 本次烧录运行的测试项

`TEST_TYPE`的值可配置为以下值：

- `TEST_RAM` ： 内存测试
- `TEST_TIME` : 性能测试
- `TEST_STAB_CYCLE` ： API稳定性测试
- `TEST_STAB_FULLLOAD` ： 满负荷运行稳定性测试



##3.使用步骤
1. 配置`test_config.h`，确定测试时使用的云端URL。
2. 在WICED-IDE上建立Target-`wilddog_client_c.tests.wiced-<yourplatform> download run`。
3. 配置`wifi_config_dct.h`确定需要连接的SSID和密码，以及测试的项。
2. 双击Target，编译并下载运行，同时打开串口工具，波特率设置为115200-8-n-1，查看测试结果。
