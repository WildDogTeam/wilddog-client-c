#### 配置wifi和URL

为esp\_iot\_sdk\_v1.2.0添加patch：将`wilddog_client_coap/platform/espressif`目录下的esp.patch文件拷贝到esp_iot_sdk_v1.2.0根目录下的include目录，并执行`patch -p0 < esp.patch`

打开`wilddog_client_coap/examples/espressif/user/user_config.h`填写热点名称和密码：


	#define SSID            "your ssid"
	#define PASSWORD       "your password"


配置URL，用户在Wilddog云端申请的URL：

	#define TEST_URL "coaps://<appId>.wilddogio.com/"


#### 编译烧录运行
首先将wilddog SDK拷贝到esp\_iot\_sdk\_v1.2.0根目录下，在wilddog sdk根目录下编译静态库，执行`make PORT_TYPE=espressif`，它将生成的libwilddog.a拷贝到esp\_iot\_sdk\_v1.2.0下的lib目录中

将`wilddog_client_coap/examples/espressif`目录下的代码拷贝到esp_sdk下的app目录并执行编译脚本。

烧录方法请参见esp_sdk下的文档。
