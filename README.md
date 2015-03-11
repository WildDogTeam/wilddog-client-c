# client_coap


## REST API


### 改变一个节点
> 改变一个节点的值,如果这个节点当前不存在,将会被创建.
#### REQUEST
> 
>```
>CON PUT 
>coap://{appid}.io.wilddog.com/{path}
>Payload:<payload>
>Token: <token>
>
>```
> ##### payload
>> 数据对象,字符串用json格式
#### RESPONSE
>```
> ACK 
><response code>
> Payload:<payload>
> Token:<token>
>```
>> |state |response code|payload|
>> |---|---|---|
>> |操作成功|2.04||
>> |没有权限|4.03|`{"err":"authority required"}` |
>> |others| -| 参照错误码解释|
#### SAMPLE
>> ```
>//request
> put:   "coap://myapp.io.wilddog.com/devices/123eabd7654" 
> data:  {"deviceName":"timemachine","deviceType":"universe destroyer"}
> //response
> code:  2.04
>>```


### 新增一个节点
> 在当前节点下新增数据,数据的id由服务端生成,通过payload(data)返回
#### REQUEST
> ```
> CON 
> POST
> coap://{appid}.io.wilddog.com/{path}
> Payload:<payload>
> Token:<token>
> ```
> ##### payload
>> 数据对象,字符串用json格式
>
#### RESPONSE
>```
> ACK
> <response code>
> Payload:<payload>
> Token:<token>
>```
>> |state |response code|payload|
>> |---|---|---|
>> |操作成功|2.01|`{"id":<generated id>}` |
>> |没有权限|4.03|`{"err":"authority required"}` |
>> |others| -| 参照错误码解释|
>>
>>
#### SAMPLE
>> ```
>//request
>post:   "coap://myapp.io.wilddog.com/devices"
>data:   {"deviceName":"timemachine","deviceType":"universe destroyer"}
>//response
>code:   2.01
>data:   {"id":"123eabd7654"}
>// now we can aquire the data with request coap://myapp.io.wilddog.com/devices/123eabd7654
>
>>```




### 删除一个节点
> 删除某个节点,以及节点下的所有子节点
#### REQUEST
>```
CON
DELETE
coap://{appid}.io.wilddog.com/{path}
Token:<token>
>```
#### RESPONSE
>```
> ACK
> <response code>
>  Payload:<payload>
>  Token:<token>
>```
>> |state |code|payload|
>> |---|---|---|
>> |操作成功|2.02| |
>> |没有权限|4.03|`{"err":"authority required"}` |
#### SAMPLE
> ```
> //request
> delete: "coap://myapp.io.wilddog.com/devices/123eabd7654"
> //response
> code:   2.02
> ```





### 查询操作
#### REQUEST
>```
> CON
> GET coap://{appid}.io.wilddog.com/{path}
> Token:<token>
>```

#### RESPONSE

>```
> ACK
> <response code>
> Payload:<payload>
> Token:<token>
>```
>
>> |state |response code|payload|
>> |---|---|---|
>> |操作成功|2.05| data in JSON format |
>> |没有权限|4.03|`{"err":"authority required"}` |

#### SAMPLE
> ```
> //request
> get:    "coap://myapp.io.wilddog.com/devices/123eabd7654"
> //response
> code:   2.05
> data:   {"deviceName":"timemachine","deviceType":"universe destroyer"}
> ```









### 监听一个节点的变化
用户发送一个特殊的get 请求监听某个节点上数据的变化.当服务端有数据更新的时候会给客户端发送 notification.
#### REQUEST
>```
> CON
> 
> Payload:<payload>
> Observe:0
> Token:<token>
>```
#### RESPONSE
>```
> ACK
> <response code>
> Payload:<payload>
> Observe:<seq code>
> Token:<token>
>```
>> |state |code|payload|seq code|
>> |---|---|---|---|
>> |操作成功|2.05| data in JSON format |严格递增的序列 |
>> |没有权限|4.03|`{"err":"authority required"}` | |

#### NOTIFICATION
>```
> ACK
> <response code>
> Payload:<payload>
> Observe:<seqence number>
> Token:<token>
> ```
>> |state |code|payload|seq code|
>> |---|---|---|---|
>> |操作成功|2.05| data in JSON format |严格递增的序列 |

#### CANCEL REQUEST
>```
> RST
> Token:<token>
>```

#### SAMPLE
>```
> //register
>```

### authentication
> 在任何操作加一个参数 `token=<token>` TOKEN 的获取不在此文档之内
> 
#### sample
> ```
> put:   "coap://myapp.io.wilddog.com/devices/123eabd7654?token=322E32E32" 
> data:  {"deviceName":"timemachine","deviceType":"universe destroyer"}
> 
> ```


## coap code
```
	"2.01":"Created",
	"2.02":"Deleted",
	"2.03":"Valid",
	"2.04":"Changed",
	"2.05":"Content",
	"4.00":"Bad Request",
	"4.01":"Unauthorized",
	"4.02":"Bad Option",
	"4.03":"Forbidden",
	"4.04":"Not Found",
	"4.05":"Method Not Allowed",
	"4.06":"Not Acceptable",
	"4.12":"Precondition Failed",
	"4.13":"Request Entity Too Large",
	"4.15":"Unsupported Content-Format",
	"5.00":"Internal Server Error",
	"5.01":"Not Implemented",
	"5.02":"Bad Gateway",
	"5.03":"Service Unavailable",
	"5.04":"Gateway Timeout",
	"5.05":"Proxying Not Supported"
	
```



## C++ WRAPPER API

### Wilddog

#### `Wilddog* ref=new Wilddog(string& url)`
new 一个实例
##### param
>* url:表示资源的URL
> 
##### return
>*   Wilddog 实例的指针



#### `int Wilddog::destroy()`
析构
##### param

##### return





#### 	`int Wilddog::setAuth(string& authCode)`
##### params
>  authCode

##### return
#### `string* Wilddog::get()`
对应rest的get
#####param

##### return
> 指向返回结果缓存的指针.

#### `string* Wilddog::post(string& data)`
对应rest的 post
##### param

##### return

#### `string* Wilddog::put(string& data)`
对应 rest的put
##### param

##### return 

#### `string* Wilddog::remove()`

##### param

##### return 

#### string& Wilddog::observe()
> 需要实现函数 `int (Wilddog::*NotificationHandler)(CoapPackage& response)`

#### string& Wilddog::cancel()

#### 适配考虑
通过函数指针实现适配

#### `int32_t (Wilddog::*_createSocket)()`

#### `int (Wilddog::*_deleteSocket)(int32_t socketId)`

#### `int (Wilddog::*_send)(int32_t socketId,WdAddress& addr,int8_t* buffer,uint32_t length)`

#### `int (Wilddog::*_receive)(int32_t socketId,WdAddress& addr,int8_t* buffer,uint32_t length)`




----
## 场景分析
###场景1:通过手机控制电灯
手机,电灯,CLOUD,开关
电灯出厂自带ID 3e4a32
`COAP_URL:coap://shome.io.wilddog.com\n/devices/jackxy/light/3e4a32?token=<AuthToken>`
(此处token 不同于coap传输协议中的token,此处是用户授权token,写在uri-query)

![](http://jackxy.com:8080/content/images/2015/03/__SVG__4efd1e917fcc8bdccb7749a17ec5003d.png)




