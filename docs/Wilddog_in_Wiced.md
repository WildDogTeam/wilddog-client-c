#### 配置wifi和URL

打开`examples/wiced/wifi_config_dct.h`填写热点名称和密码：

	/* This is the default AP the device will connect to (as a client)*/

	#define CLIENT_AP_SSID       "your ssid"

	#define CLIENT_AP_PASSPHRASE "your ap password"


配置URL，用户在Wilddog云端申请的URL：

	#define TEST_URL "coaps://<appId>.wilddogio.com/"

#### 建立Target

在Make Target 窗口新建编译目标

`wilddog_client_coap.project.wiced-<yourboard> download run`

其中`<yourboard>`为你的开发板型号，我们测试使用的wiced开发板是BCM943362WCD4，因而Target name 是 

`wilddog_client_coap.project.wiced-BCM943362WCD4 download run`

#### 编译烧录运行

让你的wiced开发板通过USB连接电脑，USB驱动在`WICED-SDK-3.1.2\WICED-SDK\tools\drivers`中。

双击Make Target窗口刚刚建立的Target：`wilddog_client_coap.project.wiced-<yourboard> download run`，编译完成后会自动烧录到开发板中运行。