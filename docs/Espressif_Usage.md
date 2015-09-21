#### 配置wifi和URL

打开`examples/espressif/user/user_main.c`填写热点名称和密码：


	#define SSID            "your ssid"
	#define PASSWORD       "your password"


配置URL，用户在Wilddog云端申请的URL：

	#define TEST_URL "coaps://<appId>.wilddogio.com/"


#### 编译烧录运行
首先将wilddog SDK拷贝到esp_sdk根目录下，在wilddog sdk根目录下编译静态库，执行make PORT_TYPE=espressif，并将生成的libwilddog.a拷贝到esp_sdk下的lib目录中

将`examples/espressif`目录下的代码拷贝到esp_sdk下的app目录并执行编译脚本。

烧录方法请参见esp_sdk下的文档。
