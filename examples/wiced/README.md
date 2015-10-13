#Demo
##1.用例说明
该用例展示如何通过云端操控wiced板子上的一个GPIO.
##1.文件结构和说明
	
    	├── demo.c
		├── demo_function.c
    	├── test_perform.c
    	└── wilddog_demo_config.h
    
*	`wilddog_demo_config.h` ： wifi相关配置
*	`demo_function.c` : 用例实现函数
*	`demo.c` : wiced工程的main

##2.配置说明
用例需要连接路由并在云端URL建立gpio节点和订阅。用户可以修改`wilddog_demo_config.h`配置：

- `TEST_URL` ： Demo使用的URL。
- `CLIENT_AP_SSID` ： 连接的`SSID`名称。
- `CLIENT_AP_PASSPHRASE` ： SSID的密码。
- `DEMO_LED1` ： 用例操控的Gpio


##3.使用步骤
1. 配置`wilddog_demo_config.h`，确定用例wifi热点和云端URL。
2. 在WICED-IDE上建立Target-`wilddog_client_c.tests.wiced-<yourplatform> download run`。
3. 双击Target，编译并下载运行，同时打开串口工具，波特率设置为115200-8-n-1，查看运行的log输出。
4. 在浏览器端输入你的URL，登录云端，把节点`led1:1`修改为`led1:0`，留意GPIO口输出电平和串口输出。
