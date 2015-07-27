#测试使用指南

测试的源文件位于tests文件夹中，目前有linux和wiced两种。

##linux端
1. 设置project/linux目录下的client.config 中的环境变量以及wilddog\_config.h中的宏定义

2. 在tools/linux目录下执行autotest.sh，检查基本API。

		$ ./autotest.sh

3. 运行完后会打印出测试结果，如果有fail项，可以在SDK的bin目录下运行对应的测试用例。
4. 进入bin目录运行`test_step`，按提示操作，检查交互API。

		$ test_step -l <appId>

##wiced端
将SDK拷贝到WICED IDE的apps目录下，修改tests/wiced目录下的`wifi_config_dct.h`，设置好`CLIENT_AP_SSID`、`CLIENT_AP_PASSPHRASE`以及`TEST_TYPE`。

在WICED IDE中添加`wilddog_client_coap.tests.wiced-<your board type> download run`，编译代码并下载到开发板中自动运行。

