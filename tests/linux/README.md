#Test
##1.文件结构和说明

	
    	├── test_buildtree.c
    	├── test_config.h
    	├── test_limit.c
    	├── test_mts.c
    	├── test_multipleHost.c
    	├── test_node.c
    	├── test_perform.c
    	├── test_ram.c
    	├── test_stab_cycle.c
    	├── test_stab_fullload.c
    	├── test_step.c
    	└── test_wdProperty.c
    

*	`test_buildtree.c` : 向云端构建数据树
*	`test_config.h` : 配置运行测试的URL，需要用户自行配置
*	`test_mts.c`：	多线程测试
*	`test_multipleHost.c` : 连接多个云端URL（不同host）的测试
*	`test_limit.c` : API边界条件测试
*	`test_node.c` : node API操作测试
*	`test_perform.c` : 性能测试
*	`test_ram.c` : 内存占用测试
*	`test_stab_cycle.c` : API稳定性测试
*	`test_stab_fullload.c` : 满负荷运行稳定性测试
*	`test_step.c` : API可用性测试
*	`test_wdProperty.c` : wilddog的properties API测试

##2.配置说明
每个测试项均需要在云端建立树，修改并获取以测试其准确性和稳定性。用户可以修改`test_config.h`配置测试使用的URL：

- `TEST_URL` ： 测试时使用的URL。
- `TEST_URL2` ： 多云端测试其中一个URL。
- `TEST_URL3` ： 多云端测试其中一个URL。
- `TEST_AUTH` ： 与`TEST_URL`建立会话的Auth。

##3.使用步骤
1. 配置`test_config.h`，确定测试时使用的云端URL。
2. 进入SDK的顶层目录，修改`tests/linux/test_config.h`,执行 `make test`编译并在`bin`目录下生成测试的可执行文件：

	
	    $ make test
		$ ls bin/
	    test_buildtree  test_limit  test_mts  test_multipleHost  test_node  test_perform  test_ram  test_stab_cycle  test_stab_fullload  test_step  test_wdProperty

3. 直接执行对应的可执行文件，会在终端看到测试结果。
