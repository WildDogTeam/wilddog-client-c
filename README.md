## 开发向导

wilddog coap客户端是专为嵌入式设备设计的.嵌入式设备有各种平台.wilddog coap客户端在设计时充分考虑了在不同平台上运行的需求.为了解决适配问题,整个程序分为两部分
* 不依赖于任何平台的代码(src)
* 平台相关的适配代码(ports)

代码移植只需要实现`#include "port.h"` 中定义的几个函数:


* `int wilddog_gethostbyname(wilddog_address_t* addr,char* host);`

* `int wilddog_openSocket(int* socketId);`

* `int wilddog_closeSocket(int socketId);`

* `int wilddog_send(int socketId,wilddog_address_t*,void* tosend,size_t tosendLength);`

* `int wilddog_receive(int socketId,wilddog_address_t*,void* toreceive,size_t toreceiveLength);`


每个函数的意义可以参见 `port.h` 定义.
目前已经移植到 wiced 平台.

###移植和编译

#### LINUX 
直接运行 `make` 可以编译 `libwilddog.a` 运行 `make sample` 可编译一个简单的例子.
这个例子可以实现 `set` `query` `push` `delete``observe` 操作

#### WICED

wiced 是第一个被移植的平台.编译例子程序需要将代码拷贝到 **wiced-sdk** 下,通过 **wiced-sdk** 编译工具进行编译

##### 编译方法 

ports/wiced/app/wilddog
**步骤:**

* 将 ports/wiced/app/wilddog 拷贝到 **wiced-sdk** 下的 app 目录下
* 在 **wiced-sdk** 执行编译命令 比如 `sudo ./make wilddog.skel-BCM943362WCD4-ThreadX-NetX_Duo-SDIO download` 
* 其他参考(文档和sdk获取:http://www.broadcom.com/products/wiced/wifi/).
