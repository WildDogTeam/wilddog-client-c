##WildDog SDK 配置说明

SDK包含条件编译选项和用户参数，可对SDK进行配置。

#### 配置条件编译选项

Linux和Espressif平台的编译选项在make时指定，WICED平台的编译选项在project/wiced/wiced.mk中，MICO平台则在工程的配置中。

	APP_SEC_TYPE : 加密方式，目前支持轻量级加密tinydtls、ARM官方加密库mbedtls和无加密nosec；

	PORT_TYPE : 运行的平台，目前支持Linux和Espressif；

Linux和Espressif平台在make时指定选项，进行不同的编译，如：

	make APP_SEC_TYPE=nosec PORT_TYPE=linux

在其他平台中，上面的宏在Makefile中指定。如WICED平台中，WildDog SDK被嵌入WICED编译框架。因此条件编译选项在SDK目录下的`project/wiced/wiced.mk`中，配置项和Linux平台中相似，`PORT_TYPE`设置为`wiced`。

----

#### 配置用户参数

用户参数在SDK include目录下的wilddog_config.h中，包含如下参数：

`WILDDOG_LITTLE_ENDIAN` : 目标机字节序，如果为小端则该宏定义的值为1；

`WILDDOG_MACHINE_BITS` : 目标机位数，可为8/16/32/64；

`WILDDOG_PROTO_MAXSIZE` : 应用层协议数据包最大长度，其范围为0~1300；

`WILDDOG_REQ_QUEUE_NUM` : 请求队列的长度；

`WILDDOG_RETRANSMITE_TIME` : 单次请求超时时间，单位为ms，超过该值没有收到服务端回应则触发回调函数,并返回超时。返回码参见`Wilddog_Return_T`；

`WILDDOG_RECEIVE_TIMEOUT` : 接收数据最大等待时间，单位为ms。
