##Wilddog SDK 配置说明

###1. 编译选项

编译选项是指在编译阶段需要设置的参数，这里特指已经支持的平台。

linux和espressif平台的编译选项在make时指定，wiced平台的编译选项在project/wiced/wiced.mk中。

编译选项目前有以下几种：

	APP_SEC_TYPE ： 加密类型，目前支持DTLS加密类型dtls和tinydtls，以及无加密类型nosec；
	PORT_TYPE ： 平台类型，根据不同平台选择；

###2. 用户参数

用户参数位于include/wilddog_config.h中，包含以下几种：

	WILDDOG_LITTLE_ENDIAN ： 目标机字节序，如果为小端则该宏值为1；
	WILDDOG_MACHINE_BITS : 目标机位数，一般为8/16/32/64；
	WILDDOG_PROTO_MAXSIZE : 应用层协议数据包最大长度，其范围为0~1300；
	WILDDOG_REQ_QUEUE_NUM : 数据请求队列的元素个数；
	WILDDOG_RETRANSMITE_TIME : 单次请求超时时间，单位为ms，超过该值没有收到服务端回应则触发回调函数,并返回超时错误码。参见Wilddog_Return_T；
	WILDDOG_RECEIVE_TIMEOUT : 接收数据最大等待时间，单位为ms。

