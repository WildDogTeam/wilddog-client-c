
## wilddog\_initWithUrl()

###### 定义
Wilddog\_T wilddog\_initWithUrl(Wilddog\_Str\_T \*url);

###### 说明
 通过应用URL初始化 Wilddog 客户端。

###### 参数

* Wilddog\_Str\_T *`url` 应用URL。Wilddog 中任何数据都能够通过一个URL来进行访问，如`coap[s]://<appId>.wilddogio.com/<path>` 
其中<appId>为开发者在 Wilddog 平台申请的应用id。
<path>为客户端关注的路径。

###### 返回值
 `Wilddog_T` 类型的客户端ID，如果创建失败，返回0。

###### 示例
```c
int main()
{

	//init client

	Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy/device/light/10abcde");

	//do something

	...

	//recycle memeory

	wilddog_destroy(&wilddog);
}
```
----
## wilddog\_getParent()

###### 定义

Wilddog\_T wilddog\_getParent(Wilddog\_T wilddog)

###### 说明

获取父节点的ID。如果当前节点是root节点，创建失败，返回0。

###### 参数

* wilddog `Wilddog_T` 当前节点的ID。

###### 返回值

返回父节点的id，如果创建失败，返回0。

###### 示例
```c

//定位到user/jackxy

Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy");

//定位到user

Wilddog_T parent = wilddog_getParent(wilddog);

```
----
## wilddog\_getRoot()

###### 定义
Wilddog\_T wilddog\_getRoot(Wilddog\_T wilddog)

###### 说明

 获取根节点ID。

###### 参数

* wilddog `Wilddog_T`  当前节点的ID。

###### 返回值

Wilddog\_T `root` 根节点的ID，如果失败，返回0。 

###### 示例
```c

//定位到user/jackxy

Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy");

//定位到root("/")

Wilddog_T root = wilddog_getRoot(wilddog);

```
----

## wilddog\_getChild()

###### 定义
Wilddog\_T wilddog\_getChild(Wilddog\_T wilddog, Wilddog\_Str\_T \*childName)

###### 说明

 查询当前节点下名字为childName的子节点ID。

###### 参数

* wilddog `Wilddog_T` 当前节点ID。

* childName `Wilddog_Str_T*` 
子节点的相对路径，多级子节点需用'/'隔开，即使子节点不存在也能创建。

###### 返回值
`Wilddog_T` 
子节点的id，如果创建失败，返回0。


###### 示例
```c
//定位到user/jackxy

Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy");

//定位到user/jackxy/aaa

Wilddog_T child = wilddog_getChild(wilddog, "aaa");

```
----
## wilddog\_getKey()

###### 定义

Wilddog\_Str\_T \*wilddog\_getKey(Wilddog\_T wilddog)

###### 说明

 获取当前节点的key。

###### 参数
 wilddog `Wilddog_T`  当前节点ID。

###### 返回值

 `Wilddog_Str_T` 如果获取失败，返回NULL。

----
## wilddog\_destroy()

###### 定义
Wilddog\_Return\_T wilddog\_destroy(Wilddog\_T \*p\_wilddog)

###### 说明
 销毁一个客户端并回收内存。
 
###### 参数

* p_wilddog `Wilddog_T* `
 客户端的ID。

###### 返回值

 `Wilddog_Return_T` 返回 `0`:成功 `<0`:失败。

----

## wilddog\_auth()

###### 定义
Wilddog\_Return\_T wilddog\_auth(Wilddog\_Str\_T \*p\_host, u8 \*p\_auth, int len, onAuthFunc onAuth, void \*args)

###### 说明
 发送auth数据到服务器进行认证。

###### 参数

* p_host `Wilddog_Str_T`进行auth认证的host字符串，如 `"<appId>.wilddogio.com"`。

* p_auth `u8*` auth的数据指针。

* int `len` auth数据的长度。

* onAuthFunc `onAuth`  服务端回应认证或者认证超时触发的回调函数，类型是`void (*onAuthFunc)(void* arg, Wilddog_Return_T err)`，其中`arg`为用户传递的值，（即下面的`args`），`err`为状态码，具体见`Wilddog_Return_T`定义。

* args `void*` 用户给回调函数传入的参数。

###### 返回值
 `Wilddog_Str_T` 发送成功返回0，发送失败则返回负数。注意该返回值仅表明发送是否成功，认证是否成功需要在回调函数中判断。
###### 示例
```c
void myOnAuthFunc(void* arg, Wilddog_Return_T err)
{
	if(err < WILDDOG_ERR_NOERR || err >= WILDDOG_HTTP_BAD_REQUEST)
	{
		printf("auth fail!\n");

		return;
	}

	printf("hello world! %d\n", (int)arg);

	return;
}

//aquired a new auth token

char* newToken="ABCD1234567890";

int args = 0;
wilddog_auth("aaa.wilddogio.com", newToken, strlen(newToken), myOnAuthFunc, (void*)&args);
...
```
----

## wilddog\_unauth()

###### 定义
Wilddog\_Return\_T wilddog\_auth(Wilddog\_Str\_T \*p\_host, onAuthFunc onAuth, void \*args)

###### 说明
 取消和服务器的auth认证。

###### 参数

* p_host `Wilddog_Str_T`进行auth认证的host字符串，如 `"<appId>.wilddogio.com"`。

* onAuthFunc `onAuth`  服务端回应认证或者认证超时触发的回调函数，类型是`void (*onAuthFunc)(void* arg, Wilddog_Return_T err)`，其中`arg`为用户传递的值，（即下面的`args`），`err`为状态码，具体见`Wilddog_Return_T`定义。

* args `void*` 用户给回调函数传入的参数。

###### 返回值
 `Wilddog_Str_T` 发送成功返回0，发送失败则返回负数。注意该返回值仅表明发送是否成功，认证是否成功需要在回调函数中判断。
###### 示例
```c
void myOnAuthFunc(void* arg, Wilddog_Return_T err)
{
	if(err < WILDDOG_ERR_NOERR || err >= WILDDOG_HTTP_BAD_REQUEST)
	{
		printf("auth fail!\n");

		return;
	}

	printf("hello world! %d\n", (int)arg);

	return;
}

int args = 0;
wilddog_unauth("aaa.wilddogio.com", myOnAuthFunc, (void*)&args);
...
```
----


## wilddog\_getValue()

###### 定义
Wilddog\_Return\_T wilddog\_getValue(Wilddog\_T wilddog, onQueryFunc callback, void* arg)

###### 说明
 获取当前节点的数据,数据格式为`Wilddog_Node_T`(类似JSON)。
 
###### 参数

* wilddog `Wilddog_T` 客户端ID。

* callback `onQueryFunc` 服务端回应数据或者回应超时触发的回调函数,类型是`void (*onQueryFunc)(const Wilddog_Node_T* p_snapshot, void* arg, Wilddog_Return_T err)`,其中`p_snapshot`是取回的数据镜像（err为200时）或者NULL，**退出函数后即被销毁**, `arg`为用户传递的值, `err`为状态码。

* arg `void*` 用户给回调函数传入的参数。

###### 返回值

 `Wilddog_Return_T` 返回 `0`:成功 `<0`:失败。
 
###### 示例
```c
STATIC void test_onQueryFunc(
	const Wilddog_Node_T* p_snapshot, 
	void* arg, 
	Wilddog_Return_T err)
{
	if(err != WILDDOG_HTTP_OK)
	{
		wilddog_debug("query error!");

		return;
	}

	wilddog_debug("query success!");

	if(p_snapshot)
	{
		*(Wilddog_Node_T**)arg = wilddog_node_clone(p_snapshot);
	}

	return;
}

int main(void)
{
	Wilddog_T wilddog = 0;

	Wilddog_Node_T * p_node = NULL;
	
	wilddog = wilddog_initWithUrl(<url>);

	wilddog_getValue(wilddog, test_onQueryFunc, (void*)(&p_node));

	while(1)
	{
		if(p_node)
		{
			_wilddog_debug_printnode(p_node);
			...
			wilddog_node_delete(p_node);
		}

		wilddog_trySync();
	}

	...

	wilddog_destroy(&wilddog);
}
```
----
## wilddog\_setValue()

###### 定义
Wilddog\_Return\_T wilddog\_setValue
   (
   Wilddog\_T wilddog,
   Wilddog\_Node\_T \*p\_node,
   onSetFunc callback,
   void \*arg
   )

###### 说明
 设置当前节点的数据,数据格式为`Wilddog_Node_T`。
 
###### 参数
* wilddog `Wilddog_T` 当前节点的ID。
* p_node `Wilddog_Node_T` 节点存储格式。
* callback `onSetFunc` 服务端回应或者回应超时触发的回调函数 ,类型是`void (*onSetFunc)(void* arg, Wilddog_Return_T err)`,其中`arg`为用户传递的值,`err`为状态码。
* arg `void*` 用户给回调函数传入的参数。

###### 返回值

`Wilddog_Return_T` 返回 `0`:成功 `<0`:失败。

###### 示例
```c
STATIC void test_onSetFunc(void* arg, Wilddog_Return_T err)
{
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("set error!");

		return;
	}

	wilddog_debug("set success!");
	*(BOOL*)arg = TRUE;

	return;
}

int main(void)
{
	BOOL isFinish = FALSE;

	Wilddog_T wilddog = 0;

	Wilddog_Node_T * p_node = NULL;

	/* create a node to "wilddog", value is "123456" */

	p_node = wilddog_node_createUString(NULL,"123456");

	wilddog = wilddog_initWithUrl(<url>);

	wilddog_setValue(wilddog, p_node, test_onSetFunc, (void*)&isFinish);

	wilddog_node_delete(p_node);

	while(1)
	{
		if(TRUE == isFinish)
		{
			wilddog_debug("set success!");

			...

		}

		wilddog_trySync();
	}

	wilddog_destroy(&wilddog);
}

```
----
## wilddog\_push()

###### 定义
Wilddog\_Return\_T wilddog\_push
   (
   Wilddog\_T wilddog, 
   Wilddog\_Node\_T \*p\_node, 
   onPushFunc callback, 
   void \*arg
   )
###### 说明
 在当前节点下生成一个子节点，并返回子节点的引用。子节点的key利用服务端的当前时间生成。

###### 参数 

* wilddog `Wilddog_T` 当前节点的ID。
* p_node `Wilddog_Node_T*` 新增`node`的ID。
* callback `onPushFunc` 服务端回应或者回应超时触发的回调函数 ,类型是`(*onPushFunc)(Wilddog_Str_T * p_newPath, void* arg, Wilddog_Return_T err)`,其中 `p_newPath` 是新增节点的完整路径,`arg` 为用户传递的值,`err` 为状态码。 
* arg `void*` 用户给回调函数传入的参数。

  
###### 返回值
`Wilddog_Return_T` 返回 `0`:成功 `<0`:失败。

###### 示例
```c
STATIC void test_onPushFunc(u8 *p_path,void* arg, Wilddog_Return_T err)
{
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("push failed");

		return;
	}

	wilddog_debug("new path is %s", p_path);

	*(BOOL*)arg = TRUE;

	return;
}

int main(void)
{
	BOOL isFinish = FALSE;

	Wilddog_T wilddog;

	Wilddog_Node_T * p_node = NULL, *p_head = NULL;

	p_head = wilddog_node_createObject(NULL);

	p_node = wilddog_node_createNum("2",1234);

	wilddog_node_addChild(p_head, p_node);
	
	wilddog = wilddog_initWithUrl(<url>);

	wilddog_push(wilddog, p_head, test_onPushFunc, (void *)&isFinish);
	
	wilddog_node_delete(p_head);
	
	while(1)
	{
		if(isFinish)
		{
			wilddog_debug("push success!");

			break;
		}

		wilddog_trySync();
	}

	wilddog_destroy(&wilddog);
}
```
----

## wilddog\_removeValue()

###### 定义
Wilddog\_Return\_T wilddog\_removeValue
   (
   Wilddog\_T wilddog, 
   onRemoveFunc callback, 
   void \*arg
   )

###### 说明
 删除当前节点及节点下所有数据。

###### 参数

* wilddog `Wilddog_T`当前节点的ID
* callback `onRemoveFunc`服务器回应或者回应超时触发的回调函数，类型是`void (*onRemoveFunc)(void* arg, Wilddog_Return_T err)`,其中`arg` 为用户传递的值,`err` 为状态码。
* arg `void*` 用户传给回调函数的参数。

###### 返回值
`Wilddog_Return_T` 返回 `0`:成功 `<0`:失败。

###### 示例
```c
STATIC void test_onDeleteFunc(void* arg, Wilddog_Return_T err)
{
	if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
	{
		wilddog_debug("delete failed!");

		return;
	}

	wilddog_debug("delete success!");
	*(BOOL*)arg = TRUE;

	return;
}

int main(void)
{
	BOOL isFinished = FALSE;

	Wilddog_T wilddog;

	wilddog = wilddog_initWithUrl(<url>);

	wilddog_removeValue(wilddog, test_onDeleteFunc, (void*)&isFinished);

	while(1)
	{
		if(TRUE == isFinished)
		{
			wilddog_debug("remove success!");

			break;
		}

		wilddog_trySync();
	}

	wilddog_destroy(&wilddog);
}
```
----

## wilddog\_addObserver()
###### 定义
Wilddog\_Return\_T wilddog\_addObserver
   (
   Wilddog\_T wilddog, 
   Wilddog\_EventType\_T event, 
   onQueryFunc onDataChange, 
   void \*dataChangeArg
   )

###### 说明

 监听某节点的数据变化。一旦该数据发生改变, `onDataChange`函数将被调用。

###### 参数


* wilddog `Wilddog_T` 当前节点ID 。
* event `Wilddog\_EventType\_T ` 关注的事件类型，见`Wilddog_EventType_T`定义。
* onDataChange `onQueryFunc` 数据变化所触发的回调函数，类型是`(*onEventFunc)(const Wilddog_Node_T* p_snapshot, void* arg, Wilddog_Return_T err)`,其中`p_snapshot`是取回的数据镜像（err为200时）或者NULL，**退出函数后即被销毁**, `arg`为用户传递的值, `err`为状态码。 
* dataChangeArg `void*` 传给回调函数的`arg`。

###### 返回值
`Wilddog_Return_T` 返回 `0`:成功 `<0`:失败。

###### 示例
```c
STATIC void test_onObserveFunc
	(
	const Wilddog_Node_T* p_snapshot, 
	void* arg,
	Wilddog_Return_T err
	)
{
	if(err != WILDDOG_HTTP_OK)
	{
		wilddog_debug("observe failed!");

		return;
	}

	wilddog_debug("observe data!");
	
	return;
}

int main(void)
{
	BOOL isFinished = FALSE;

	Wilddog_T wilddog;

	STATIC int count = 0;	

	wilddog = wilddog_initWithUrl(TEST_ON_PATH);

	if(0 == wilddog)
	{
		wilddog_debug("new wilddog failed!");

		return 0;
	}
	wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, test_onObserveFunc, (void*)&isFinished);

	while(1)
	{
		if(TRUE == isFinished)
		{
			wilddog_debug("get new data %d times!", count++);

			isFinished = FALSE;

			if(count > 10)
			{
				wilddog_debug("off the data!");
				wilddog_off(wilddog, WD_ET_VALUECHANGE);

				break;
			}
		}
		wilddog_trySync();
	}

	wilddog_destroy(&wilddog);
}
```
----

## wilddog\_removeObserver()
###### 定义
Wilddog\_Return\_T wilddog\_removeObserver
   (
   Wilddog\_T \*p\_wilddog, 
   Wilddog\_EventType\_T event
   )

###### 说明

 取消对某节点的监听(对应于on)。

###### 参数

* p_wilddog `Wilddog_T*` 当前节点ID。
* event `Wilddog_EventType_T` 取消的事件类型。

###### 返回值

`Wilddog_Return_T`返回 `0`:成功 `<0`:失败。

----
## wilddog\_trySync()

###### 定义
 void wilddog\_trySync(void)

###### 说明
 通过调用`wilddog_trySync`来向Wilddog云端同步数据。每次调用都会处理来自云端的推送和请求超时的重发、长连接的维持 ，以及触发用户注册的回调函数。
###### 返回值
void

----

## wilddog\_increaseTime()

###### 定义
void wilddog\_increaseTime(u32 ms)。

###### 说明
 用于校准Wilddog的时钟(可以在定时器中调用)。一般情况下Wilddog会根据自己推算的时间执行业务操作，这个时间的推算会有偏差，我们可以通过传入一个时间增量来校准Wilddog时钟。

###### 参数

* ms `u32 ` 增加的时间(单位为毫秒)。

###### 返回值
void

###### 示例
```c
void timer_isr()
{
	//this isr is been called per ms。

	wilddog_increaseTime(1);
}
```
----

#Node

##wilddog\_node\_createObject()

###### 定义

Wilddog\_Node\_T \* wilddog\_node\_createObject(Wilddog_Str\_T \*key)

###### 说明
创建一个Object类型的节点。

###### 参数

* key `Wilddog_Str_T* `节点的key值。

###### 返回值

 创建成功则返回该节点的指针，失败返回NULL。

----

## wilddog\_node\_createUString()

###### 定义
Wilddog\_Node\_T \* wilddog\_node\_createUString(Wilddog\_Str\_T \*key, Wilddog\_Str\_T \*value)

###### 说明
创建一个字符串类型节点。

###### 参数

* key `Wilddog_Str_T*` 节点的key值。
* value `Wilddog_Str_T*` 指向utf-8字符串的指针。

###### 返回值
`Wilddog_Node_T` 创建成功则返回节点的指针，失败返回NULL。

----

##wilddog\_node\_createBString()

###### 定义 
Wilddog\_Node\_T \* wilddog\_node\_createBString(Wilddog\_Str\_T \*key, u8 \*value, int len)

###### 说明
创建一个二进制数组类型节点。

###### 参数
* key `Wilddog_Str_T*` 节点的key值。
* value `u8*` 二进制数组的指针。
* len `int` 数据的长度(字节)。

###### 返回值
`Wilddog_Node_T*`  创建成功则返回节点的指针，失败返回NULL。

----

## wilddog\_node\_createFloat()

###### 定义
Wilddog\_Node\_T \* wilddog\_node\_createFloat(Wilddog\_Str\_T \*key, wFloat num)

###### 说明
创建一个浮点类型的节点。

###### 参数
* key `Wilddog_Str_T *` 节点的key值。
* num `wFloat` 浮点数据(8位机器为32bits, 其他为64bits)。

###### 返回值
 `Wilddog_Node_T *`创建成功则返回节点的指针，失败返回NULL。

----
## wilddog\_node\_createNum()

###### 定义
Wilddog\_Node\_T \* wilddog\_node\_createNum(Wilddog\_Str\_T \*key, s32 num)
###### 说明
创建一个整数类型节点。

###### 参数
* key `Wilddog_Str_T*` 节点的key值。
* num `s32` 32位有符号整数。

###### 返回值
 `Wilddog_Node_T `  创建成功则返回节点的指针，失败返回NULL。

----
## wilddog\_node\_createNull()

###### 定义

Wilddog\_Node\_T \* wilddog\_node\_createNull(Wilddog\_Str\_T \*key)。

###### 说明
创建一个Null类型节点。

###### 参数
* key `Wilddog_Str_T*` 节点的key值。

###### 返回值
 `Wilddog_Node_T ` 创建成功则返回节点的指针，失败返回NULL。

----

## wilddog\_node\_createTrue()

###### 定义

Wilddog\_Node\_T \* wilddog\_node\_createTrue(Wilddog\_Str\_T \*key)。

###### 说明
创建一个TRUE类型节点。

###### 参数
* key ` Wilddog_Str_T*` 节点的key值。

###### 返回值
`Wilddog_Node_T ` 创建成功则返回节点的指针，失败返回NULL。

----

## wilddog\_node\_createFalse()

###### 定义
Wilddog\_Node\_T \* wilddog\_node\_createFalse(Wilddog\_Str\_T \*key)。



###### 说明
创建一个FALSE类型节点。

###### 参数

* key `Wilddog_Str_T*` 节点的key值。

###### 返回值
`Wilddog_Node_T ` 创建成功则返回节点的指针，失败返回NULL。

----

## wilddog\_node\_addChild()

###### 定义

Wilddog\_Return\_T wilddog\_node\_addChild(Wilddog\_Node\_T \*parent, Wilddog\_Node\_T \*child)。

###### 说明

向一个节点添加子节点。

###### 参数

* parent `Wilddog_Node_T*` 指向父节点的指针。
* child `Wilddog_Node_T*` 指向要添加的子节点的指针。

###### 返回值
 `Wilddog_Return_T` 成功返回 `0`, 失败返回 `<0`的值。

----

## wilddog\_node\_delete()

###### 定义 
Wilddog\_Return\_T wilddog\_node\_delete( Wilddog\_Node\_T \*head)

###### 说明
删除节点及其所有子节点。

###### 参数
* head `Wilddog_Node_T*` 要删除节点的指针。

###### 返回值
 `Wilddog_Return_T` 成功返回 `0`, 失败返回 `<0`的值。

----

## wilddog\_node\_clone()

###### 定义
Wilddog\_Node\_T \* wilddog\_node\_clone( Wilddog\_Node\_T \*head)

###### 说明
拷贝当前节点及其下所有子节点。

###### 参数
* head `Wilddog_Node_T*` 指向节点的指针。

###### 返回值
`Wilddog_Node_T` 成功返回当前节点副本的指针, 失败返回NULL。

----


## wilddog\_node\_find()

###### 定义
Wilddog\_Node\_T \*wilddog\_node\_find( Wilddog\_Node\_T \*root, char \*path)

###### 说明
root中查找相对路径下的节点。

###### 参数
* root `Wilddog_Node_T*` 指向根节点的指针。
* path `char*` 指向相对路径的指针。

###### 返回值
`Wilddog_Node_T` 成功返回节点指针, 失败返回NULL。




----
## wilddog\_node\_getValue()

###### 定义
Wilddog\_Str\_T* wilddog\_node\_getValue(Wilddog\_Node\_T \*node, int \*len)

###### 说明
获取当前节点的value值。

###### 参数
* node `Wilddog_Node_T*` 指向节点的指针。
* len `int*` 输出参数，将存储value值的长度(字节)。

###### 返回值
 `Wilddog_Node_T` 成功返回指向节点value的指针(可根据type和传出的len来转化)，失败返回NULL。

----


## wilddog\_node\_setValue()

###### 定义
Wilddog\_Return\_T (Wilddog\_Node\_T \*node, u8 \*value, int len)

###### 说明
设置当前节点的value值。

###### 参数
* node `Wilddog_Node_T*` 指向节点的指针。
* value `u8*` 指向新value值的指针。
* len `int` 新value的长度。

###### 返回值
 `Wilddog_Return_T` 成功返回 `0`, 失败返回 `<0` 的数。

----

