#Test
##1.文件结构和说明

	
    	├── test_buildtree.c
    	├── test_config.h
    	└── user_main.c
    

*	`test_buildtree.c` : 向云端构建用于内存和性能测试的数据树
*	`test_config.h` : 配置运行测试的URL，需要用户自行配置
*	`user_main.c` ： 配置网络环境。


##2.配置说明
每个测试项均需要在云端建立树，修改并获取以测试其准确性和稳定性。用户可以修改`user_config.h`配置测试使用的URL：

- `TEST_TYPE` ： 测试时使用的测试类型。
- `TEST_URL` ： 测试时使用的URl。
- `TREE_SN`  ： 测试时使用的云端树序号
- `REQ_NUMBER` ： 测试时使用的请求数目
- `DELAY_TIME_MS` ： 测试时使用的延时

##3.使用步骤
1. 配置`TEST_TYPE`,确定测试的类型，支持`TEST_RAM`和`TEST_TIME`;配置`user_config.h`，确定测试时使用的云端URL；配置`TREE_SN`，确定测试时使用的云端树序号；配置`REQ_NUMBER`，确定测试时使用的请求数目；配置`DELAY_TIME_MS`，确定测试时使用的延时。
2. 测试程序的编译、链接和烧录请参考docs目录下的Espressif_Usage.md。
3. 执行测试程序。


