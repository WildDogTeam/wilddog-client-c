##Wilddog Demo 使用说明


####第一步 编译Demo,编译后的可执行文件在bin目录下
	$ cd wilddog_client_coap
	$ make example

 总共生成六个可执行程序：

	$ ls ./bin/
	addObserver  demo  getValue  push  removeValue  setValue

####第二步 各个可执行程序的使用说明

#####setValue

使用方法： ./bin/setValue -l <url>

举例：

	$ ./bin/setValue -l http://imba.wilddogio.com
	func:test_setValueFunc LINE: 20: setValue success!

此时，查看imba.wilddogio.com下的数据，其下有一个key为1，value为“123456”的数据。


#####getValue

使用方法： ./bin/getValue -l <url>

举例：

	./bin/getValue -l http://imba.wilddogio.com
	"/":{"1":"123456"}
	getValue success!

此时，返回imba.wilddogio.com下的数据。

#####push

使用方法： ./bin/push -l <url>

举例：

	./bin/push -l http://imba.wilddogio.com
	func:test_pushFunc LINE: 20: new path is /1437114359336

此时，imba.wilddogio.com下新增一个节点，子节点的key利用服务端的当前时间生成。


#####removeValue

使用方法： ./bin/removeValue -l <url>

举例：

	./bin/removeValue -l http://imba.wilddogio.com
	func:test_removeValueFunc LINE: 19: removeValue success!

此时，imba.wilddogio.com下的数据全部被清空。

#####addObserver

使用方法： ./bin/addObserver -l <url>

举例：

	./bin/addObserver -l http://imba.wilddogio.com
	"/":{"1":"123456"}func:test_addObserverFunc LINE: 27: addObserver data!
	"/":{"1":"1234567"}func:test_addObserverFunc LINE: 27: addObserver data!
	"/":{"1":"12345678"}func:test_addObserverFunc LINE: 27: addObserver data!

此时，关注imba.wilddogio.com下的数据变化，一旦有数据的改动，addObserver方法的回调函数就打印出新的数据。


#####demo

使用方法： ./bin/demo getValue|setValue|push|removeValue|addObserver -l coap://yourappid.wilddogio.com/ --key a --value 124

举例：

	./bin/demo setValue -l coap://imba.wilddogio.com --key a --value 124


