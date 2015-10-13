####第一步 编译Demo,编译后的可执行文件在bin目录下
	$ cd wilddog-client-c
	$ make example

 生成可执行程序：

	$ ls ./bin/
	demo

####第二步 可执行程序的使用说明

#####demo

使用方法： 
	
	./bin/demo getValue|setValue|push|removeValue|addObserver -l coap://<yourappid>.wilddogio.com/ [--key=<key string> --value=[value string]]

举例：

	./bin/demo setValue -l coap://imba.wilddogio.com --key a --value 124
