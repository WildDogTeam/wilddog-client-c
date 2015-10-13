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


为esp\_iot\_sdk\_v1.2.0添加patch：

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

	$ cd app/
	$ cp ../wilddog-client-c/examples/espressif/* . -rf
	$ ./gen_misc.sh

生成的bin文件在bin目录下，烧录方法请参见esp\_iot\_sdk\_v1.2.0下的文档。

#### 建立自己的Demo程序

请参考wilddog\-client\-c/examples/espressif的目录结构和Makefile写法。如果有多个子目录，请在Makefile中修改SUBDIRS变量。另外，请注意Makefile中的INCLUDES变量，需要添加wilddog\-client\-c下的路径。
