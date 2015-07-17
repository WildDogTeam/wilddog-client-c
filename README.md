#Wilddog SDK说明

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

该目录根据不同平台分为不同子目录，分别为linux和wiced。

####project

demo例子的工程目录。

####src

平台无关目录。

####tests

测试文件。

####tools

各个平台使用的一些工具。

##2. 移植说明

目前SDK已经成功移植到Wiced平台上，我们以此为例，说明如何移植SDK。

### 将SDK拷贝到目标位置

首先，我们将SDK解压，并拷贝到`WICED-SDK-3.1.2\WICED-SDK\apps`中，即SDK位于`WICED-SDK-3.1.2\WICED-SDK\apps\wilddog_client_coap\`。

Wiced平台采用WICED IDE，打开WICED IDE，能够在工程下的`apps`目录下找到我们的SDK。

----

### 移植条件编译选项

Wiced平台需要用户完成Makefile，格式有严格要求，Makefile文件名称的前缀必须与目录名相同.

在`project/wiced/wiced.mk`中添加编译选项，并补完Makefile，详见`wiced.mk`文件。

注意：如果你的平台不支持自定义Makefile，那么请根据条件编译选项，仅将你所需的文件拷贝到平台下，避免出现重定义。需要选择拷贝的路径有：

*	`APP_PROTO_TYPE` : src/networking目录下，根据编译选项拷贝文件夹；

*	`APP_SEC_TYPE` ： src/secure目录下，根据编译选项拷贝文件夹；

*	`PORT_TYPE` ： platform目录下，根据编译选项拷贝文件夹，如果你的平台不属于`linux`或`wiced`，那么你需要自己实现平台相关的函数接口。

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
##其他参考

SDK 文档: https://z.wilddog.com/device/quickstart

Wiced 文档和sdk获取:http://www.broadcom.com/products/wiced/wifi/
