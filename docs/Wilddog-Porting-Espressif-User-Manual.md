
SDK在esp\_iot\_sdk\_v1.2.0上测试通过，以esp\_iot\_sdk\_v1.2.0为例进行说明，其他版本可以参考该说明。

#### 下载esp\_iot\_sdk\_v1.2.0

下载地址：http://espressif.com/en/products/hardware/esp8266ex/resources

#### 为esp\_iot\_sdk\_v1.2.0打patch

首先将wilddog\-client\-c拷贝到esp\_iot\_sdk\_v1.2.0根目录下,此时的目录结构如下：

	.
	├── app
	├── bin
	├── document
	├── examples
	├── include
	├── ld
	├── lib
	├── Makefile
	├── tools
	└── wilddog-client-c

注意：从网页下载时，解压后sdk文件夹名字为wilddog\-client\-c\-master，将之修改为wilddog\-client\-c。


从esp sdk根目录下开始，为esp\_iot\_sdk\_v1.2.0添加patch：

	$pwd
	/home/espressif/esp_iot_sdk_v1.2.0
	$ cp wilddog-client-c/platform/espressif/esp.patch ./include/
	$ cd ./include/
	$ patch -p0 < esp.patch

为esp\_iot\_sdk\_v1.2.0添加静态库的更新：
(注：lib\_mem\_optimize\_150714.zip可以从http://bbs.espressif.com/viewtopic.php?f=5&t=734下载)

	$ cp wilddog-client-c/platform/espressif/lib_mem_optimize_150714.zip ./lib/
	$ cd ./lib/
	$ unzip lib_mem_optimize_150714.zip


#### 配置wifi和URL
打开`wilddog-client-c/examples/espressif/user/user_config.h`填写热点名称和密码：


	#define SSID            "your ssid"
	#define PASSWORD       "your password"


配置URL，用户在Wilddog云端申请的URL：

	#define TEST_URL "coaps://<appId>.wilddogio.com/"


#### 编译烧录运行
在`wilddog-client-c`根目录下编译静态库:

	$ cd wilddog-client-c/
	$ make PORT_TYPE=espressif
 	
它自动将生成的libwilddog.a拷贝到esp\_iot\_sdk\_v1.2.0下的lib目录中.

接着将Wilddog SDK的示例程序拷贝到esp平台下，在app目录下编译示例程序:

	$ cd ../app/
	$ cp ../wilddog-client-c/examples/espressif/* . -rf
	$ ./gen_misc.sh

生成的bin文件在bin目录下，烧录方法请参见esp\_iot\_sdk\_v1.2.0下的文档。

#### 建立自己的Demo程序

请参考wilddog\-client\-c/examples/espressif的目录结构和Makefile写法。如果有多个子目录，请在Makefile中修改SUBDIRS变量。另外，请注意Makefile中的INCLUDES变量，需要添加wilddog\-client\-c下的路径。

注意：

1. 在esp\_iot\_sdk\_v1.5.0版本，需要对wilddog\-client\-c/examples/espressif/Makefile进行修改，在`LINKFLAGS_eagle.app.v6`变量中增加`-lcrypto`，否则可能出现找不到函数的情况；

2. esp sdk不要放在共享文件夹下编译，否则可能出现循环编译`user_main.c`的问题。
 
