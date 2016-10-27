#Test
##1.文件结构和说明
    
	├── test_config.h
	├── test_disEvent.c
	├── test_limit.c
	├── test_multipleHost.c
	├── test_perform.c
	├── test_ram.c
	├── test_stab_cycle.c
	├── test_stab_fullload.c
	└── test_step.c

*   `test_config.h` : 配置运行测试的URL，需要用户自行配置
*   `test_disEvent.c` : 离线事件API测试
*   `test_limit.c` : API边界条件测试
*   `test_multipleHost.c` : 连接多个云端URL（不同host）的测试
*   `test_perform.c` : 性能测试，sdk内各个部分code执行时间
*   `test_ram.c` : 内存占用测试
*   `test_stab_cycle.c` : API稳定性测试
*   `test_stab_fullload.c` : 满负荷运行稳定性测试
*   `test_step.c` : API可用性测试
*	`test_reobserver.c` : 重复 observer测试

##2.配置说明

每个测试项均需要在云端建立树，修改并获取以测试其准确性和稳定性。用户可以修改`test_config.h`配置测试使用的URL：

- `TEST_URL` ： 测试时使用的URL。
- `TEST_URL2` ： 多云端测试其中一个URL。
- `TEST_URL3` ： 多云端测试其中一个URL。
- `TEST_URL4` ： 多云端测试其中一个URL。
- `TEST_AUTH` ： 与`TEST_URL`建立会话的Auth。

##3.使用步骤

1. 配置`test_config.h`，确定测试时使用的云端URL。
2. 进入SDK的顶层目录，修改`tests/linux/test_config.h`,执行 `make test`编译并在`bin`目录下生成测试的可执行文件：

    
        $ make test
        $ ls bin/
          test_disEvent  test_multipleHost  test_ram         test_stab_fullload
          test_limit     test_perform       test_stab_cycle  test_step

3. 直接执行对应的可执行文件，会在终端看到测试结果。

##4.自动化测试

1. 进入tools/linux下，运行autotest.sh，例如：

		$ cd tools/linux
		$ ./autotest.sh -s nosec

2. 等待console返回测试结果。

###autotest.sh参数说明

	-s <arg> : 必选项，指定测试的APP_SEC_TYPE，可选项：nosec|tinydtls|mbedtls
	-1 <arg> : 可选项，指定TEST_URL的Appid（也可在tests/linux/test_config.h中修改）
	-2 <arg> : 可选项，指定TEST_URL1的Appid（也可在tests/linux/test_config.h中修改）
	-3 <arg> : 可选项，指定TEST_URL2的Appid（也可在tests/linux/test_config.h中修改）
	-4 <arg> : 可选项，指定TEST_URL3的Appid（也可在tests/linux/test_config.h中修改）
	-a <arg> : 可选项，指定TEST_AUTH（也可在tests/linux/test_config.h中修改）