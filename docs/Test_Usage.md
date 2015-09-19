#测试使用指南

测试的源文件位于tests文件夹中，目前有linux和wiced两种。

##linux端

1. 在tools/linux目录下参照README，检查基本API。

2. 运行完后会打印出测试结果，如果有fail项，可以在SDK根目录下`make test`，在SDK的bin目录下运行对应的测试用例。

		$ make test APP_SEC_TYPE=nosec

4. 进入bin目录运行`test_step`，按提示操作，检查交互API。

		$ test_step -l <url>

##wiced端
参考tests/wiced下面的README。

