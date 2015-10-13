#Test
##1.文件结构和说明

    	├── stab_function.c
    	├── test_buildtree.c
    	├── user_config.h
    	└── user_main.c
    
*   `stab_function.c` : 进行稳定性测试的代码
*	`test_buildtree.c` : 向云端构建用于内存和性能测试的数据树
*	`user_config.h` : 配置运行测试的URL，需要用户自行配置
*	`user_main.c` ： 配置网络环境。


##2.配置说明
每个测试项均需要在云端建立树，修改并获取以测试其准确性和稳定性。用户可以修改`user_config.h`配置测试使用的URL：

- `TEST_TYPE` ： 测试时使用的测试类型。
- `TEST_URL` ： 测试时使用的URl。
- `TREE_SN`  ： 测试时使用的云端树序号
- `REQ_NUMBER` ： 测试时使用的请求数目
- `DELAY_TIME_MS` ： 测试时使用的延时

##3.使用步骤
1. 请阅读docs目录下的Espressif_Usage.md。
2. 配置`TEST_TYPE`,确定测试的类型，支持`TEST_RAM`、`TEST_TIME`和`TEST_STAB_CYCLE`;配置`user_config.h`，确定测试时使用的云端URL；配置`TREE_SN`，确定测试时使用的云端树序号；配置`REQ_NUMBER`，确定测试时使用的请求数目；配置`DELAY_TIME_MS`，确定测试时使用的延时。
3. 重新编译wilddog-client-c下的库文件：

		$ make PORT_TYPE=espressif WILDDOG_SELFTEST=yes

4. 接着在app目录下编译示例程序:

		$ cd app/
		$ cp ../wilddog-client-c/tests/espressif/* . -rf
		$ ./gen_misc.sh

生成的bin文件在bin目录下，烧录方法请参见esp\_iot\_sdk\_v1.2.0下的文档。

