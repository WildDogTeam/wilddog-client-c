#WildDog SDK说明
WildDog C/嵌入式SDK是Wilddog云在C/嵌入式场景下的客户端，使用者调用api接口即可实现和云端的通信、消息订阅功能。如有什么意见、建议或合作意向，可访问`www.wilddog.com`，或联系`jimmy.pan@wilddog.com`。

WildDog C/嵌入式的SDK 使用的是CoAP UDP + DTLS + CBOR技术。

在物联网环境下，由IETF主导的CoAP协议要比MQTT等协议要更加适用和更有针对性。只使用UDP协议可以使得ROM大小变小(无需TCP协议栈)；由于无需保持TCP状态，所以比传统TCP更加省电，更加轻量；适合在Thread和Zigbee上传输；满足在未来受限网络和受限设备的网络场景。

CoAP是唯一的国际物联网应用层标准，我们对选择何种协议并没有太多兴趣，但是我们认为CoAP更能解决问题。

**NEWS**：

我们在Linux平台下采用C/嵌入式SDK实现了一个远程调用shell脚本的工具，欢迎大家使用，路径：`https://github.com/WildDogTeam/liveshell`

##1. 目录结构

	├── docs
	├── examples
	├── include
	├── platform
	├── project
	├── src
	├── tests
	└── tools

####docs
SDK文档。

####examples
各个平台下的demo例子。

####include
目录中各个文件内容如下：

*	wilddog.h : 基本数据结构和宏定义
*	wilddog_api.h : api函数声明
*	wilddog_config.h : 用户配置宏定义
*	wilddog_port.h : 平台相关接口函数
*	widdog_debug.h : debug相关函数声明

####platform

该目录根据不同平台分为不同子目录，分别为linux和wiced等。

####project

demo例子的工程目录。

####src

平台无关目录。

####tests

测试文件。

####tools

各个平台使用的一些工具。

##2. 移植说明

目前SDK已经成功移植到Wiced等平台上，我们以WICED为例，说明如何移植SDK，其他平台的可参见docs目录下的说明。

### 将SDK拷贝到目标位置

首先，我们将SDK解压，并拷贝到`WICED-SDK-3.1.2\WICED-SDK\apps`中，即SDK位于`WICED-SDK-3.1.2\WICED-SDK\apps\wilddog-client-c\`。

WICED平台中，`-`有特殊含义，因此我们将目录名字从`wilddog-client-c`改成`wilddog_client_c`。

Wiced平台采用WICED IDE，打开WICED IDE，能够在工程下的`apps`目录下找到我们的SDK。

----

### 移植条件编译选项

Wiced平台需要用户完成Makefile，格式有严格要求，Makefile文件名称的前缀必须与目录名相同.

在`project/wiced/wiced.mk`中添加编译选项，并补完Makefile，详见`wiced.mk`文件。

注意：如果你的平台不支持自定义Makefile，那么请根据条件编译选项，仅将你所需的文件拷贝到平台下，避免出现重定义。需要选择拷贝的路径有：

*	`APP_PROTO_TYPE` : src/networking目录下，根据编译选项拷贝文件夹；

*	`APP_SEC_TYPE` ： src/secure目录下，根据编译选项拷贝文件夹；

*	`PORT_TYPE` ： platform目录下，根据编译选项拷贝文件夹，如果你的平台不属于`linux`或`wiced`等已支持平台，那么你需要自己实现平台相关的函数接口。

----

### 实现平台相关代码

需要实现的平台相关函数接口位于include/wilddog_port.h，如下：

	int wilddog_gethostbyname(Wilddog_Address_T* addr,char* host);
	int wilddog_openSocket(int* socketId);
	int wilddog_closeSocket(int socketId);
	int wilddog_send
		(
		int socketId,
		Wilddog_Address_T*,
		void* tosend,
		s32 tosendLength
		);
	int wilddog_receive
		(
		int socketId,
		Wilddog_Address_T*,
		void* toreceive,
		s32 toreceiveLength, 
		s32 timeout
		);

----
### 运行示例

移植完成后，你可以运行示例确认是否移植成功，下面以Wiced平台为例：

#### 配置wifi和URL

打开`examples/wiced/wilddog_demo_config.h`填写热点名称和密码：

	/* This is the default AP the device will connect to (as a client)*/

	#define CLIENT_AP_SSID       "your ssid"

	#define CLIENT_AP_PASSPHRASE "your ap password"


配置URL，用户在Wilddog云端申请的URL：

	#define TEST_URL "coaps://<appId>.wilddogio.com/"

#### 建立Target

在Make Target 窗口新建编译目标

`wilddog_client_c.project.wiced-<yourboard> download run`

其中`<yourboard>`为你的开发板型号，我们测试使用的wiced开发板是BCM943362WCD4，因而Target name 是 

`wilddog_client_c.project.wiced-BCM943362WCD4 download run`

#### 编译烧录运行

将wiced开发板通过USB连接电脑，USB驱动在`WICED-SDK-3.1.2\WICED-SDK\tools\drivers`中。

双击Make Target窗口刚刚建立的Target：`wilddog_client_c.project.wiced-<yourboard> download run`，编译完成后会自动烧录到开发板中运行。

----
##其他参考

SDK 文档: https://z.wilddog.com/device/quickstart

Wiced 文档和sdk获取:http://www.broadcom.com/products/wiced/wifi/

espressif sdk获取: http://espressif.com/zh-hans/%E6%9C%80%E6%96%B0sdk%E5%8F%91%E5%B8%83/
