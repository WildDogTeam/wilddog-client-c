##Wilddog Demo 使用说明


####第一步 编译Demo,编译后的可执行文件在bin目录下
	$ cd wilddog_client_coap
	$ make example

 总共生成六个可执行程序：

	$ ls ./bin/
	addObserver  demo  getValue  push  removeValue  setValue

####第二步 各个可执行程序的使用说明

#####setValue

使用方法： ./bin/setValue -l <appId>

举例：
	$ ./bin/setValue -l imba
	func:test_setValueFunc LINE: 23: setValue success!

此时，查看imba.wilddogio.com下的数据，其下有一个key为1，value为“123456”的数据。


#####getValue

使用方法： ./bin/getValue -l <appId>

举例：
	./bin/getValue -l imba
	"/":{"1":"123456"}
	getValue success!

此时，返回imba.wilddogio.com下的数据。

#####push

使用方法： ./bin/push -l <appId>

举例：
	./bin/push -l imba
	func:test_pushFunc LINE: 23: new path is /1437112450316

此时，imba.wilddogio.com下


#####removeValue

#####addObserver

#####demo




