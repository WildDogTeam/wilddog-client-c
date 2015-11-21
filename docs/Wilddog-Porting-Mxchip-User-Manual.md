#### 在IAR中为MiCOKit添加wilddog\-client\-c文件

首先将wilddog\-client\-c拷贝到SDK_MiCO_v2.3.0目录中.

	.
	├── Application
	├── Board
	├── include
	├── libraries
	├── libwilddog
	├── mico
	├── Platform
	└── Output


#### 配置wifi和URL
打开`wilddog\-client\-c/examples/mxchip/demo.c`填写热点名称和密码：

	#define CLIENT_AP_SSID       "your ssid"
	#define CLIENT_AP_PASSPHRASE "your passport"


配置URL，用户在Wilddog云端申请的URL：

	#define TEST_URL "coaps://<appId>.wilddogio.com/"


#### 编译烧录运行

使用IAR7.3打开`wilddog\-client\-c/project/mxchip`下的工程，点击IAR中的Make和Download按钮，即可进行编译烧录运行。

