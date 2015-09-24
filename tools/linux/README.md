#tools/linux
##1.文件结构和说明

	
    	├── autotest.sh
    	├── cov.sh
    	└── uncov.sh
    

*	`autotest.sh` : 自动测试脚本，使用方式: `autotest.sh nosec|tinydtls|dtls`，nosec|tinydtls|dtls三选一
*	`cov.sh` : 覆盖率测试脚本，使用方式: `cov.sh nosec|tinydtls|dtls`，nosec|tinydtls|dtls三选一
*	`uncov.sh`：覆盖率测试清理脚本，使用方式: `cov.sh nosec|tinydtls|dtls`，nosec|tinydtls|dtls三选一

##2.配置说明
每个测试项均需要在云端建立树，修改并获取以测试其准确性和稳定性。用户可以修改`test_config.h`配置测试使用的URL：

- `TEST_URL` ： 测试时使用的URL。
- `TEST_URL2` ： 多云端测试其中一个URL。
- `TEST_URL3` ： 多云端测试其中一个URL。
- `TEST_AUTH` ： 与`TEST_URL`建立会话的Auth。

##3.使用步骤
1. 配置`test_config.h`，确定测试时使用的云端URL。
2. 进入SDK的顶层目录，修改`tests/linux/test_config.h`,执行测试脚本，例如：
    $ cd tools/linux
	$ ./xxxx.sh nosec

3. 等待，会在终端看到测试结果。
