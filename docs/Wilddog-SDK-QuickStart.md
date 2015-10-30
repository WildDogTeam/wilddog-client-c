####第一步 创建账号和应用

首先[**注册**](https://www.wilddog.com/account/signup)并登录Wilddog账号，进入控制面板。在控制面板中，添加一个新的应用。

你会获得一个独一无二的应用URL https://<appId>.wilddogio.com/，在同步和存取数据的时候，我们将使用这个URL。

####第二步 使用Wilddog C/嵌入式SDK

获得 SDK，你可以从gitHub中下载。

我们的SDK已经在Linux平台和Wiced平台、espressif平台上成功移植，在此仅以Linux为例，其他平台请查看对应的Wilddog-Porting-xxx-User-Manual.md。

####第三步 编译SDK

编译SDK，编译后的库文件在lib目录下

	$ cd wilddog-client-c
	$ make 

编译示例，编译后的可执行文件在bin目录下

	$ make example

####第四步 运行示例

向应用URL存储一个key-value结构的数据

	$ ./bin/demo setValue -l <应用URL> --key a --value 1 

获取应用URL的数据

	$ ./bin/demo getValue -l <应用URL>
	
执行结果：

	"/":{"a":"1"}
