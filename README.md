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

----
##2. 快速入门

编译SDK，编译后的库文件在lib目录下

	$ cd wilddog-client-c
	$ make 

编译示例，编译后的可执行文件在bin目录下

	$ make example

向应用URL存储一个key-value结构的数据

	$ ./bin/demo setValue -l <应用URL> --key a --value 1 
获取应用URL的数据

	$ ./bin/demo getValue -l <应用URL>
执行结果：

	"/":{"a":"1"}


----
##3. 移植说明

SDK已经在WICED、ESP8266、树莓派、arduino yun、openwrt中成功移植，可参考docs目录。

----
##4. 其他参考

SDK 文档: https://z.wilddog.com/device/quickstart

Wiced 文档和sdk获取:http://www.broadcom.com/products/wiced/wifi/

espressif sdk获取: http://espressif.com/zh-hans/%E6%9C%80%E6%96%B0sdk%E5%8F%91%E5%B8%83/
