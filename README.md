#wilddog COAP client

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



## C WRAPPER API

### init
>####`wilddog_t* wilddog_init(char* appid,char* path,unsigned char* auth)`
>初始化一个wilddog 客户端
>* `appid`
> 在wilddog平台申请的APPID
>* `path`
> wilddog客户端需要同步的路径
>* `auth`
>  服务端下发的,auth token,这个token可以通过多种方式获取
>
>返回指向wildog_t 结构体的指针
>#### sample
> ```c

>int main(){
>char* appid;
> char* path;
> char* authtoken;
>//init appid
>//init path
>//init auth
>//...
>//init client
>wilddog_t* wd= wilddog_init(appid,path,authtoken);
>//do something
>//...
>//recycle memeory
>wilddog_destroy(wd)
>}
> ```



### destroy
>#### `int wilddog_destroy(wilddog_t* wilddog);`
> 删除一个客户端 回首内存
>* `wilddog`
> `wilddog_t` 类型的指针,指向要删除的client结构体
>
>返回 `0`:成功 `<0`:失败

### setAuth
> #### `void wilddog_setAuth(wilddog_t* wilddog,unsigned char* auth);`
>更新auth
>``` c
>...
>//aquired a new auth token
>char* newToken="ABCD1234567890"
>
>wilddog_setAuth(wd,newToken);
>...
>```

### query

> #### `int wilddog_query(wilddog_t* wilddog,onCompleteFunc callback)`
>* `wilddog`
>* `callback`
>#### sample
>```c
>void onQueryComplete(wilddog_t* wilddog,handle,int errCode){
>    if(errCode<0)
>        printf("query error:%d",errCode)
>    else{
>       cJSON* data= wilddog->data;
>       //do something with data via cJSON API
>    }
>}
>int main(){
>    wilddog_t* wd =wilddog_init(<someappid>,<somepath>,<someAuth>);
>    //...
>    int handle=wilddog_query(wd,onQueryComplete);
>    if(handle<0)
>       return 0;
>    while(1){
>        //使用事件循环的方式,需要循环接收网络事件并处理.
>        trySync(wd);
>    }
>}
>```

### set
>#### `int wilddog_set(wilddog_t* wilddog,cJSON* data,onCompleteFunc callback)`
>

### push
>#### `int wilddog_push(wilddog_t* wilddog,cJSON* data,onCompleteFunc callback)`

### delete
>#### `int wilddog_delete(wilddog_t* wilddog,onCompleteFunc callback);`

### on
>#### `int wilddog_on(wilddog_t* wilddog,onDataFunc onDataChange,onCompleteFunc callback)`

### off
>#### `int wilddog_off(wilddog_t* wilddog);`

### trySync
>#### `int wilddog_trySync(wilddog_t* wilddog);`

### wilddog_dump
>#### `int wilddog_dump(wilddog_t* wilddog,char * buffer,size_t len)`




----
## 场景分析
###场景1:通过手机控制电灯
手机,电灯,CLOUD,开关
电灯出厂自带ID 3e4a32
`COAP_URL:coap://shome.io.wilddog.com\n/devices/jackxy/light/3e4a32?token=<AuthToken>`
(此处token 不同于coap传输协议中的token,此处是用户授权token,写在uri-query)
```sequence



手机->CLOUD:通过Auth接口获取AuthToken
手机->CLOUD:通过android ios 客户端连到云端\n并关注/devices/jackxy/的变化
手机->电灯:使其联网并给其一个AuthToken
note right of 电灯:电灯被激活试图连接云端
电灯->CLOUD:CON PUT \n{COAP_URL}\npayload:{"bright":0,"state":"off"}
note left of CLOUD:云端sync所有关注\n相关节点的客户端
CLOUD->手机:同步/devices/jackxy/light/3e4a32/\n{"bright":0,"state":"off"}
note left of 手机:显示有一个\n电灯被添加
note right of 电灯:连接成功监听数据变化
电灯->CLOUD:CON GET\n{COAP_URL}\nObserve:0
note left of 手机:用户控制将亮度调到85
手机->CLOUD:同步/devices/jackxy/light/3e4a32/\n{"bright":85,"state":"on"}
note right of CLOUD:云端同步所有\n关注相关节点\n的客户端
CLOUD->电灯:ACK\nObserve:32\n{"bright":85,"state":"on"}
note right of 电灯:执行操作


```








## 重要流程

### observe 

```sequence
client->coap_broker: CON GET\n/abc/de\nObserve:0\nToken:1234
note left of coap_broker: save the socket to map
coap_broker-> core_server: reg
note right of core_server: query data
core_server->coap_broker: return data
note right of coap_broker: query map of sockets
coap_broker->client: ACK 2.05\nObserve:22 \nPayload:"hello world"\nToken:1234

note right of core_server: ...\ntime flies\n...\nthere is a data change
core_server->coap_broker: send notification
note right of coap_broker: if notification is not full
coap_broker->core_server: query full data
core_server->coap_broker: return full data
coap_broker->client: ACK 2.05\nObserve:43 \nPayload:"fuckGFW"\nToken:1234
note left of client: ...\nnow i dont\n interest in\n /abc/de
client->coap_broker: RST
coap_broker->core_server: unreg


```






