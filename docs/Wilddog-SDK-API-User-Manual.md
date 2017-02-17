# Wilddog Sync C/嵌入式 SDK API 文档

## Wilddog Sync 实例操作

### wilddog_initWithUrl

**定义**

```c
Wilddog_T wilddog_initWithUrl(Wilddog_Str_T *url)
```

**说明**

初始化应用 URL 对应的 Wilddog Sync 实例，和云端建立连接。

**参数**

| 参数名 | 说明 |
|---|---|
| url | `Wilddog_Str_T ` 指针类型。指向应用 URL 地址的指针。 |

**返回值**

成功返回 Wilddog Sync 实例，否则返回 0。

**示例**

```c
int main(){
    //初始化实例，<appId> 为你的应用 ID，路径为/user/jackxy/device/light/10abcde
    Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy/device/light/10abcde");
    //do something
    ...
    //销毁实例
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_destroy

**定义**

```c
Wilddog_Return_T wilddog_destroy(Wilddog_T *p_wilddog)
```

**说明**

销毁一个 Wilddog Sync 对象并回收内存。

**参数**

| 参数名 | 说明 |
|---|---|
| p_wilddog | `Wilddog_T ` 指针类型。指向当前路径对应 Wilddog Sync 实例的指针。 |

**返回值**

成功返回 0，否则返回对应的 [错误码](/api/sync/c/error-code.html)。

**示例**

```c
int main(){
    //初始化实例，<appId> 为你的应用 ID，路径为/user/jackxy/device/light/10abcde
    Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy/device/light/10abcde");
    //do something
    ...
    //销毁实例
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_getParent

**定义**

```c
Wilddog_T wilddog_getParent(Wilddog_T wilddog)
```

**说明**

获取当前路径的父路径的 Wilddog Sync 实例。如果当前路径是根路径，获取失败，返回0。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |

**返回值**

成功返回当前路径的父路径的 Wilddog Sync 实例，否则返回 0。

**示例**

```c
//获取 /user/jackxy 的 Wilddog Sync 实例
Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy");
//获取 /user 的 Wilddog Sync 实例
Wilddog_T parent = wilddog_getParent(wilddog);
```

</br>

---

### wilddog_getRoot

**定义**

```c
Wilddog_T wilddog_getRoot(Wilddog_T wilddog)
```

**说明**

获取当前路径对应的根路径的 Wilddog Sync 实例。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |

**返回值**

成功返回根路径的 Wilddog Sync 实例，否则返回 0。

**示例**

```c
//获取 /user/jackxy 的 Wilddog Sync 实例
Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy");
//获取根路径的 Wilddog Sync 实例
Wilddog_T root = wilddog_getRoot(wilddog);
```

</br>

---

### wilddog_getChild

**定义**

```c
Wilddog_T wilddog_getChild(Wilddog_T wilddog, Wilddog_Str_T *childName)
```

**说明**

创建当前路径下 `childName` 路径的 Wilddog Sync 实例。注意：`childName` 可能在云端并不存在。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |
| childName | `Wilddog_Str_T` 指针类型。相对子路径，多级需用'/'隔开，即使不存在也能创建。|

**返回值**

成功返回子路径的 Wilddog Sync 实例，否则返回 0。

**示例**

```c
//获取 /user/jackxy 的 Wilddog Sync 实例
Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy");
//获取 /user/jacxy/aaa 的 Wilddog Sync 实例
Wilddog_T child = wilddog_getChild(wilddog, "aaa");
```

</br>

---

### wilddog_getKey

**定义**

```c
Wilddog_Str_T *wilddog_getKey(Wilddog_T wilddog)
```

**说明**

获取当前路径的 key。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |

**返回值**

成功返回当前路径的 key，否则返回 NULL。

**示例**

```c
//获取 /user/jackxy 的 Wilddog Sync 实例
Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy");
//获取Key值（即jackxy）
Wilddog_Str_T *key = wilddog_getKey(wilddog);
```

</br>

---

### wilddog_getHost

**定义**

```c
Wilddog_Str_T *wilddog_getHost(Wilddog_T wilddog)
```

**说明**

获取当前路径的 host。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |

**返回值**

成功返回当前路径的 host，否则返回 NULL。

**示例**

```c
//获取 /user/jackxy 的 Wilddog Sync 实例
Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy");
//获取 host 值（即 "<appId>.wilddogio.com"）
Wilddog_Str_T *host = wilddog_getHost(wilddog);
```

</br>

---

### wilddog_getPath

**定义**

```c
Wilddog_Str_T *wilddog_getPath(Wilddog_T wilddog)
```

**说明**

获取当前路径的 path。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |

**返回值**

成功返回当前路径的 path，否则返回 NULL。

**示例**

```c
//获取 /user/jackxy 的 Wilddog Sync 实例
Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy");
//获取 path 值（即 "/user/jackxy" ）
Wilddog_Str_T *path = wilddog_getPath(wilddog);
```

</br>

---

## 数据修改和同步

### wilddog_getValue

**定义**

```c
Wilddog_Return_T wilddog_getValue(Wilddog_T wilddog, onQueryFunc callback, void* arg)
```

**说明**

获取当前路径的数据，数据格式为 `Wilddog_Node_T` (类似 JSON )。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |
| callback | `onQueryFunc` 类型。服务端回应数据或者回应超时触发的回调函数。|
| arg | `void` 指针类型。可为 NULL，用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)，同时会触发回调函数，错误码也能够在回调函数中查询。

**示例**

```c
STATIC void onQueryCallback(const Wilddog_Node_T* p_snapshot, void* arg, Wilddog_Return_T err){
    if(err != WILDDOG_HTTP_OK){
        wilddog_debug("query error!");
        return;
    }
    wilddog_debug("query success!");
    if(p_snapshot){
        *(Wilddog_Node_T**)arg = wilddog_node_clone(p_snapshot);
    }
    return;
}
int main(void){
    Wilddog_T wilddog = 0;

    //用户自定义参数，这里的用途为：将云端发回的数据clone到本地
    Wilddog_Node_T * p_node = NULL;

    //<url>即希望获取数据的url，如https://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    //注意，这里省略了对wilddog_getValue返回值的检查
    wilddog_getValue(wilddog, onQueryCallback, (void*)(&p_node));

    while(1){
        if(p_node){
            //打印得到的节点数据
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

</br>

---

### wilddog_setValue

**定义**

```c
Wilddog_Return_T wilddog_setValue(Wilddog_T wilddog, Wilddog_Node_T *p_node, onSetFunc callback, void *arg)
```

**说明**

设置当前路径的数据到云端，数据格式为`Wilddog_Node_T`(类似 JSON )。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |
| p_node | `Wilddog_Node_T` 指针类型。指向当前路径对应 `Wilddog_Node_T` 节点数据的指针，注意，头节点即为当前路径。 |
| callback | `onSetFunc` 类型。服务端回应数据或者回应超时触发的回调函数。|
| arg | `void` 指针类型。可为 NULL，用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)，同时会触发回调函数，错误码也能够在回调函数中查询。

**示例**

```c
STATIC void onSetCallback(void* arg, Wilddog_Return_T err){
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED){
        wilddog_debug("set error!");
        return;
    }
    wilddog_debug("set success!");
    return;
}
int main(void){
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL;

    //创建一个字符串类型节点，值为 123456，key 为 NULL （这个节点的 key 是当前路径的 key，因此无需设置） 
    p_node = wilddog_node_createUString(NULL,"123456");

    //<url>即希望设置数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    //注意，这里省略了对wilddog_setValue返回值的检查
    wilddog_setValue(wilddog, p_node, onSetCallback, NULL);

    //数据已经设置到云端，删除刚才建立的节点
    wilddog_node_delete(p_node);

    while(1){
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_push

**定义**

```c
Wilddog_Return_T wilddog_push( Wilddog_T wilddog, Wilddog_Node_T *p_node, onPushFunc callback, void *arg)
```

**说明**

在当前路径下追加一个子节点，并在回调中返回该子节点的完整路径 。子节点的 key 由服务端根据当前时间生成。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前节点对应 Wilddog Sync 实例。 |
| p_node | `Wilddog_Node_T` 指针类型。指向当前路径对应 `Wilddog_Node_T` 节点数据的指针，注意，头节点即为当前路径。 |
| callback | `onPushFunc` 类型。服务端回应数据或者回应超时触发的回调函数。|
| arg | `void` 指针类型。可为 NULL，用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)，同时会触发回调函数，错误码也能够在回调函数中查询。

**示例**

```c
STATIC void onPushCallback(u8 *p_path,void* arg, Wilddog_Return_T err){
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED){
        wilddog_debug("push failed");
        return;
    }
    wilddog_debug("new path is %s", p_path);
    return;
}

int main(void){
    //用户自定义参数，这里的用途为：初始化为FALSE，回调函数中设为TRUE
    //因此可以在main函数中得知是否成功
    BOOL isFinish = FALSE;
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL, *p_head = NULL;

    //建立一个object节点，即类似json中的{}
    p_head = wilddog_node_createObject(NULL);

    //建立一个key为2，value为数字1234的节点
    p_node = wilddog_node_createNum("2",1234);

    //将节点p_node添加到object中
    wilddog_node_addChild(p_head, p_node);
    
    //<url>即希望推送数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    //注意，这里省略了对wilddog_push返回值的检查
    wilddog_push(wilddog, p_head, onPushCallback, NULL);

    //数据已经推送，删除刚才建立的节点
    wilddog_node_delete(p_head);

    while(1){
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_removeValue

**定义**

```c
Wilddog_Return_T wilddog_removeValue(Wilddog_T wilddog, onRemoveFunc callback, void *arg)
```

**说明**

删除当前路径及其子路径下所有数据。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |
| callback | `onRemoveFunc` 类型。服务端回应数据或者回应超时触发的回调函数。|
| arg | `void` 指针类型。可为 NULL，用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)，同时会触发回调函数，错误码也能够在回调函数中查询。

**示例**

```c
STATIC void onDeleteCallback(void* arg, Wilddog_Return_T err){
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED){
        wilddog_debug("delete failed!");
        return;
    }
    wilddog_debug("delete success!");
    return;
}
int main(void){
    Wilddog_T wilddog;

    //<url>即希望删除数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    //注意，这里省略了对wilddog_removeValue返回值的检查
    wilddog_removeValue(wilddog, onDeleteCallback, NULL);

    while(1){
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_addObserver

**定义**

```c
Wilddog_Return_T wilddog_addObserver(Wilddog_T wilddog, Wilddog_EventType_T event, onEventFunc onDataChange, void *dataChangeArg)
```

**说明**

监听当前路径的数据变化。一旦该数据发生改变, `onDataChange` 函数将被调用。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |
| event | [Wilddog_EventType_T]() 类型。监听的事件类型。 |
| callback | `onRemoveFunc` 类型。服务端回应数据或者回应超时触发的回调函数。|
| arg | `void` 指针类型。可为 NULL，用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)，同时会触发回调函数，错误码也能够在回调函数中查询。

**示例**

```c
STATIC void onObserverCallback(const Wilddog_Node_T* p_snapshot, void* arg, Wilddog_Return_T err){
    if(err != WILDDOG_HTTP_OK){
        wilddog_debug("observe failed!");
        return;
    }
    wilddog_debug("observe data!");
    return;
}
int main(void){
    Wilddog_T wilddog = 0;
    STATIC int count = 0;

    //<url>即希望订阅数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    //注意，这里省略了对wilddog_addObserver返回值的检查
    wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, onObserverCallback, NULL);

    while(1){
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_removeObserver

**定义**

```c
Wilddog_Return_T wilddog_removeObserver(Wilddog_T wilddog, Wilddog_EventType_T event)
```

**说明**

取消对当前路径下某个事件的监听（对应于 `wilddog_addObserver` ）。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |
| event | [Wilddog_EventType_T]() 类型。 取消的事件类型。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)。

**示例**

```c
STATIC void onObserverCallback(const Wilddog_Node_T* p_snapshot, void* arg, Wilddog_Return_T err){
    if(err != WILDDOG_HTTP_OK){
        wilddog_debug("observe failed!");
        return;
    }
    *(BOOL*)arg = TRUE;
    wilddog_debug("observe data!");
    return;
}
int main(void){
    //用户自定义参数，这里的用途为：初始化为FALSE，回调函数中设为TRUE
    //因此可以在main函数中得知是否成功
    BOOL isFinished = FALSE;
    Wilddog_T wilddog = 0;
    STATIC int count = 0;

    //<url>即希望订阅数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    //注意，这里省略了对wilddog_addObserver返回值的检查
    wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, onObserverCallback, (void*)&isFinished);

    while(1){
        if(TRUE == isFinished){
            //每次接收到推送count + 1
            wilddog_debug("get new data %d times!", count++);

            //重新设置接收状态为FALSE
            isFinished = FALSE;

            //count 超过10时，调用wilddog_removeObserver取消订阅，并退出
            if(count > 10){
                wilddog_debug("off the data!");
                wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE);
                break;
            }
        }
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_auth

<blockquote class="warning">
  <p><strong>注意：</strong></p>
  <ul>
    <li>wilddog_auth 函数需在调用 wilddog_initWithUrl 初始化该 appId 并获取实例后调用，否则将不生效。</li>
  </ul>
</blockquote>

**定义**

```c
Wilddog_Return_T wilddog_auth(Wilddog_Str_T *p_host, u8 *p_auth, int len, onAuthFunc onAuth, void *args)
```

**说明**

发送 auth 数据到服务器进行认证，每个 host 只需要认证一次。

**参数**

| 参数名 | 说明 |
|---|---|
| p_host | `Wilddog_Str_T` 指针类型。进行 auth 认证的 host 字符串，如 `"<appId>.wilddogio.com"`。 |
| p_auth | `unsigned char` 指针类型。指向 auth 数据的指针，auth 数据可以使用其他端 SDK 的 token，或者[使用 Server SDK 生成](/guide/auth/server/server.html)。|
| len | `int` 类型。auth 数据的长度。 |
| onAuth | `onAuthFunc` 类型。服务端回应认证或者认证超时触发的回调函数。 |
| args | `void` 指针类型。用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)。

**示例**

```c
void myOnAuthFunc(void* arg, Wilddog_Return_T err){
    if(err < WILDDOG_ERR_NOERR || err >= WILDDOG_HTTP_BAD_REQUEST){
        printf("auth fail!\n");
        return;
    }
    printf("auth success! %d\n", *(int*)arg);
    return;
}

char* newToken="ABCD1234567890";

wilddog_auth("aaa.wilddogio.com", newToken, strlen(newToken), myOnAuthFunc, NULL);
```

</br>

---

### wilddog_unauth

<blockquote class="warning">
  <p><strong>注意：</strong></p>
  <ul>
    <li>wilddog_unauth 函数需在调用 wilddog_initWithUrl 初始化该 appId 并获取实例后调用，否则将不生效。</li>
  </ul>
</blockquote>

**定义**

```c
Wilddog_Return_T wilddog_unauth(Wilddog_Str_T *p_host, onAuthFunc onAuth, void *args)
```

**说明**

取消和服务器的 auth 认证，每个 host 只需要取消认证一次。

**参数**

| 参数名 | 说明 |
|---|---|
| p_host | `Wilddog_Str_T` 指针类型。取消 auth 认证的 host 字符串，如 `"<appId>.wilddogio.com"`。 |
| onAuth | `onAuthFunc` 类型。服务端回应认证或者认证超时触发的回调函数。 |
| args | `void` 指针类型。用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)。

**示例**

```c
void myOnAuthFunc(void* arg, Wilddog_Return_T err){
    if(err < WILDDOG_ERR_NOERR || err >= WILDDOG_HTTP_BAD_REQUEST){
        printf("auth fail!\n");
        return;
    }
    return;
}

wilddog_unauth("aaa.wilddogio.com", myOnAuthFunc, NULL);
```

</br>

---

### wilddog_trySync

**定义**

```c
void wilddog_trySync(void)
```

**说明**

和云端维持连接、接收云端数据、管理数据重传，应该在空闲时调用。

**示例**

```c
int main(){
    //初始化实例，<appId> 为你的应用ID，路径为/user/jackxy/device/light/10abcde
    Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy/device/light/10abcde");
    //do something
    ...
    while(1){
        wilddog_trySync();
    }
}
```

</br>

---

### wilddog_increaseTime

**定义**

```c
void wilddog_increaseTime(u32 ms)
```

**说明**

用于校准 Wilddog 的时钟(可以在定时器中调用)。`wilddog_trySync()` 被调用时会自动增加 Wilddog 时钟，但该时间的计算会有偏差，可以通过传入一个时间增量来校准 Wilddog 时钟。

**示例**

```c
int main(){
    //初始化实例，<appId> 为你的应用ID，路径为/user/jackxy/device/light/10abcde
    Wilddog_T wilddog=wilddog_initWithUrl("coaps://<appId>.wilddogio.com/user/jackxy/device/light/10abcde");
    //do something
    ...
    while(1){
        wilddog_trySync();
        //休眠1秒
        sleep(1);
        //给wilddog 时钟增加1秒增量
        wilddog_increaseTime(1000);
    }
}
```

</br>

---

## 离线事件

### wilddog_onDisconnectSetValue

**定义**

```c
Wilddog_Return_T wilddog_onDisconnectSetValue(Wilddog_T wilddog, Wilddog_Node_T *p_node, onDisConnectFunc callback, void* arg)
```

**说明**

当云端检测到客户端离线时，设置当前路径的数据，数据格式为 `Wilddog_Node_T`。回调函数用于判断该离线事件是否成功注册。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |
| p_node | `Wilddog_Node_T` 指针类型。指向节点数据的指针，注意，头节点即为当前路径。 |
| callback | `onDisConnectFunc` 类型。服务端回应数据或者回应超时触发的回调函数。|
| arg | `void` 指针类型。可为 NULL，用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)，同时会触发回调函数，错误码也能够在回调函数中查询。

**示例**

```c
STATIC void onSetCallback(void* arg, Wilddog_Return_T err){
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED){
        wilddog_debug("offline set error!");
        return;
    }
    wilddog_debug("offline set success!");
    return;
}
int main(void){
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL;

    /* create a node to "wilddog", value is "123456" */
    p_node = wilddog_node_createUString(NULL,"123456");

    //<url>即希望设置数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    //注意，这里省略了对 wilddog_onDisconnectSetValue 返回值的检查
    wilddog_onDisconnectSetValue(wilddog, p_node, onSetCallback, NULL);
    wilddog_node_delete(p_node);

    while(1){
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_onDisconnectPush

**定义**

```c
Wilddog_Return_T wilddog_onDisconnectPush( Wilddog_T wilddog, Wilddog_Node_T *p_node, onDisConnectFunc callback, void* arg)
```

**说明**

当云端检测到客户端离线时，在当前路径下生成一个子节点，数据格式为 `Wilddog_Node_T`。回调函数用于判断该离线事件是否成功注册。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |
| p_node | `Wilddog_Node_T` 指针类型。指向节点数据的指针，注意，头节点即为当前路径。 |
| callback | `onDisConnectFunc` 类型。服务端回应数据或者回应超时触发的回调函数。|
| arg | `void` 指针类型。可为 NULL，用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)，同时会触发回调函数，错误码也能够在回调函数中查询。

**示例**

```c
STATIC void onPushCallback(void* arg, Wilddog_Return_T err){
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED){
        wilddog_debug("offline push failed");
        return;
    }
    wilddog_debug("offline push success");
    return;
}
int main(void){
    Wilddog_T wilddog = 0;
    Wilddog_Node_T * p_node = NULL, *p_head = NULL;
    p_head = wilddog_node_createObject(NULL);
    p_node = wilddog_node_createNum("2",1234);
    wilddog_node_addChild(p_head, p_node);
    
    //<url>即希望推送数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    //注意，这里省略了对wilddog_onDisconnectPush返回值的检查
    wilddog_onDisconnectPush(wilddog, p_head, onPushCallback, NULL);
    wilddog_node_delete(p_head);

    while(1){
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_onDisconnectRemoveValue

**定义**

```c
Wilddog_Return_T wilddog_onDisconnectRemoveValue(Wilddog_T wilddog, onDisConnectFunc callback, void* arg)
```

**说明**

当云端检测到客户端离线时，删除当前路径及子路径下所有数据，数据格式为 `Wilddog_Node_T`。回调函数用于判断该离线事件是否成功注册。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |
| callback | `onDisConnectFunc` 类型。服务端回应数据或者回应超时触发的回调函数。|
| arg | `void` 指针类型。可为 NULL，用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)，同时会触发回调函数，错误码也能够在回调函数中查询。

**示例**

```c
STATIC void onDeleteCallback(void* arg, Wilddog_Return_T err){
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED){
        wilddog_debug("offline delete failed!");
        return;
    }
    wilddog_debug("offline delete success!");
    return;
}
int main(void){
    BOOL isFinished = FALSE;
    Wilddog_T wilddog = 0;

    //<url>即希望删除数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    //注意，这里省略了对wilddog_onDisconnectRemoveValue返回值的检查
    wilddog_onDisconnectRemoveValue(wilddog, onDeleteCallback, NULL);

    while(1){
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_cancelDisconnectOperations

**定义**

```c
Wilddog_Return_T wilddog_cancelDisconnectOperations(Wilddog_T wilddog, onDisConnectFunc callback, void* arg)
```

**说明**

取消该 Wilddog Sync 实例设置的所有离线事件。

**参数**

| 参数名 | 说明 |
|---|---|
| wilddog | `Wilddog_T ` 类型。当前路径对应 Wilddog Sync 实例。 |
| callback | `onDisConnectFunc` 类型。服务端回应数据或者回应超时触发的回调函数。|
| arg | `void` 指针类型。可为 NULL，用户给回调函数传入的参数。|

**返回值**

成功返回 0，否则返回对应 [错误码](/api/sync/c/error-code.html)。

**示例**

```c
STATIC void onCancelCallback(void* arg, Wilddog_Return_T err){
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED){
        wilddog_debug("offline operation cancel failed!");
        return;
    }
    wilddog_debug("offline operation cancel success!");
    return;
}
int main(void){
    Wilddog_T wilddog = 0;

    //<url>即希望删除数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    //注意，这里省略了对wilddog_cancelDisconnectOperations返回值的检查
    wilddog_cancelDisconnectOperations(wilddog, onCancelCallback, NULL);

    while(1){
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
}
```

</br>

---

### wilddog_goOffline

**定义**

```c
void wilddog_goOffline(void)
```

**说明**

断开客户端和云端的连接，之前若注册了离线事件则云端会触发离线事件。

**示例**

```c
int main(void){
    Wilddog_T wilddog = 0;

    //<url>即希望删除数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    wilddog_goOffline();
}
```

</br>

---

### wilddog_goOnline

**定义**

```c
void wilddog_goOnline(void)
```

**说明**

若客户端处于离线状态，则重新连接云端服务，之前若注册了监听事件，则 SDK 会重新发送监听请求。注意：重连后会触发监听回调，返回当前的数据。

**示例**

```c
int main(void){
    Wilddog_T wilddog = 0;

    //<url>即希望删除数据的url，如coaps://<appid>.wilddogio.com/a/b/c
    wilddog = wilddog_initWithUrl(<url>);

    wilddog_goOnline();
}
```

</br>

---

## 节点操作

### wilddog_node_createObject

**定义**

```c
Wilddog_Node_T * wilddog_node_createObject(Wilddog_Str_T *key)
```

**说明**

创建一个 Object 类型的节点。Object 类型的节点即非叶子节点（有子节点的节点）。

**参数**

| 参数名 | 说明 |
|---|---|
| key | `Wilddog_Str_T` 指针类型。指向节点的 key 的指针。 |

**返回值**

成功返回指向创建的节点的指针，否则返回 NULL。

**示例**

```c
Wilddog_Node_T *p_node = wilddog_node_createObject((Wilddog_Str_T *)"123");
```

</br>

---

### wilddog_node_createUString

**定义**

```c
Wilddog_Node_T * wilddog_node_createUString(Wilddog_Str_T *key, Wilddog_Str_T *value)
```

**说明**

创建一个字符串类型节点。

**参数**

| 参数名 | 说明 |
|---|---|
| key | `Wilddog_Str_T` 指针类型。指向节点的 key 的指针。 |
| value | `Wilddog_Str_T` 指针类型。指向节点的 value 的指针。 |

**返回值**

成功返回指向创建的节点的指针，否则返回 NULL。

**示例**

```c
Wilddog_Node_T *p_node = wilddog_node_createUString((Wilddog_Str_T *)"this is key",(Wilddog_Str_T *)"this is value");
```

</br>

---

### wilddog_node_createBString

**定义**

```c
Wilddog_Node_T * wilddog_node_createBString(Wilddog_Str_T *key, unsigned char *value, int len)
```

**说明**

创建一个二进制数组类型节点。

**参数**

| 参数名 | 说明 |
|---|---|
| key | `Wilddog_Str_T` 指针类型。指向节点的 key 的指针。 |
| value | `unsigned char` 指针类型。指向节点的 value 的指针。 |
| len | `int` 类型。value 的长度（字节数）。|

**返回值**

成功返回指向创建的节点的指针，否则返回 NULL。

**示例**

```c
u8 data[8] = {0};
Wilddog_Node_T *p_node = wilddog_node_createBString((Wilddog_Str_T *)"this is key", data, 8);
```

</br>

---

### wilddog_node_createFloat

**定义**

```c
Wilddog_Node_T * wilddog_node_createFloat(Wilddog_Str_T *key, wFloat num)
```

**说明**

创建一个浮点类型节点。8 位机器为 4 字节, 其他位机器为 8 字节。

**参数**

| 参数名 | 说明 |
|---|---|
| key | `Wilddog_Str_T` 指针类型。指向节点的 key 的指针。 |
| num | `wFloat` 类型浮点数据。 |

**返回值**

成功返回指向创建的节点的指针，否则返回 NULL。

**示例**

```c
wFloat data = 1.234;
Wilddog_Node_T *p_node = wilddog_node_createFloat((Wilddog_Str_T *)"this is key", data);
```

</br>

---

### wilddog_node_createNum

**定义**

```c
Wilddog_Node_T * wilddog_node_createNum(Wilddog_Str_T *key, s32 num)
```

**说明**

创建一个整数类型节点，只支持 32 位整型。

**参数**

| 参数名 | 说明 |
|---|---|
| key | `Wilddog_Str_T` 指针类型。指向节点的 key 的指针。 |
| num | `s32` 类型。 |

**返回值**

成功返回指向创建的节点的指针，否则返回 NULL。

**示例**

```c
s32 data = 1;
Wilddog_Node_T *p_node = wilddog_node_createNum((Wilddog_Str_T *)"this is key", data);
```

</br>

---

### wilddog_node_createNull

**定义**

```c
Wilddog_Node_T * wilddog_node_createNull(Wilddog_Str_T *key)
```

**说明**

创建一个 Null 类型节点，对应到 JSON 中即其值为 `null` 。

**参数**

| 参数名 | 说明 |
|---|---|
| key | `Wilddog_Str_T` 指针类型。指向节点的 key 的指针。 |

**返回值**

成功返回指向创建的节点的指针，否则返回 NULL。

**示例**

```c
Wilddog_Node_T *p_node = wilddog_node_createNull((Wilddog_Str_T *)"this is key");
```

</br>

---

### wilddog_node_createTrue

**定义**

```c
Wilddog_Node_T * wilddog_node_createTrue(Wilddog_Str_T *key)
```

**说明**

创建一个 True 类型节点，对应到 JSON 中即其值为 `true` 。

**参数**

| 参数名 | 说明 |
|---|---|
| key | `Wilddog_Str_T` 指针类型。指向节点的 key 的指针。 |

**返回值**

成功返回指向创建的节点的指针，否则返回 NULL。

**示例**

```c
Wilddog_Node_T *p_node = wilddog_node_createTrue((Wilddog_Str_T *)"this is key");
```

</br>

---

### wilddog_node_createFalse

**定义**

```c
Wilddog_Node_T * wilddog_node_createFalse(Wilddog_Str_T *key)
```

**说明**

创建一个 False 类型节点，对应到 JSON 中即其值为 `false` 。

**参数**

| 参数名 | 说明 |
|---|---|
| key | `Wilddog_Str_T` 指针类型。指向节点的 key 的指针。 |

**返回值**

成功返回指向创建的节点的指针，否则返回 NULL。

**示例**

```c
Wilddog_Node_T *p_node = wilddog_node_createFalse((Wilddog_Str_T *)"this is key");
```

</br>

---

###  wilddog_node_addChild

**定义**

```c
Wilddog_Return_T wilddog_node_addChild(Wilddog_Node_T *parent, Wilddog_Node_T *child)
```

**说明**

向一个节点添加子节点，成功后，child 节点成为父节点 parent 的子节点。parent 节点可以通过`parent->p_wn_child`或者`parent->p_wn_child->p_wn_next`（可能有多次`p_wn_next`，由 parent 节点的组成决定）的链表顺序查找方式找到 child 节点。

**参数**

| 参数名 | 说明 |
|---|---|
| parent | `Wilddog_Node_T` 指针类型。指向父节点的指针，如果父节点不是 Object 类型，会自动转换为 Object 类型，原有的值会丢失。 |
| child | `Wilddog_Node_T` 指针类型。指向要添加的子节点的指针。 |

**返回值**

成功返回指向创建的节点的指针，否则返回 NULL。

**示例**

```c
//构建节点 {"123":{"this is key": false}}
Wilddog_Node_T *p_father = wilddog_node_createObject((Wilddog_Str_T *)"123");
Wilddog_Node_T *p_child = wilddog_node_createFalse((Wilddog_Str_T *)"this is key");
wilddog_node_addChild(p_father, p_child);
```

</br>

---

###  wilddog_node_delete

**定义**

```c
Wilddog_Return_T wilddog_node_delete( Wilddog_Node_T *head)
```

**说明**

删除节点及其所有子节点。

**参数**

| 参数名 | 说明 |
|---|---|
| head | `Wilddog_Node_T` 指针类型。指向要删除节点的指针。 |

**返回值**

成功返回 0，否则返回[错误码](/api/sync/c/error-code.html)。

**示例**

```c
Wilddog_Node_T *p_father = wilddog_node_createObject((Wilddog_Str_T *)"123");
Wilddog_Node_T *p_child = wilddog_node_createFalse((Wilddog_Str_T *)"this is key");
wilddog_node_addChild(p_father, p_child);

wilddog_node_delete(p_father); //会将 p_father 和子节点 p_child 全部删除
```

</br>

---

###  wilddog_node_clone

**定义**

```c
Wilddog_Node_T * wilddog_node_clone( Wilddog_Node_T *head)
```

**说明**

拷贝当前节点及其下所有子节点。

**参数**

| 参数名 | 说明 |
|---|---|
| head | `Wilddog_Node_T` 指针类型。指向要拷贝节点的指针。 |

**返回值**

成功返回当前节点副本的指针，否则返回 NULL。

**示例**

```c
Wilddog_Node_T *p_father = wilddog_node_createObject((Wilddog_Str_T *)"123");
Wilddog_Node_T *p_child = wilddog_node_createFalse((Wilddog_Str_T *)"this is key");
wilddog_node_addChild(p_father, p_child);

//复制一个副本，需要单独调用 wilddog_node_delete 释放
Wilddog_Node_T *p_clone = wilddog_node_clone(p_father); 
```

</br>

---

###  wilddog_node_find

**定义**

```c
Wilddog_Node_T *wilddog_node_find( Wilddog_Node_T *root, char *path)
```

**说明**

从 `root` 节点中查找相对子路径下的节点。

**参数**

| 参数名 | 说明 |
|---|---|
| root | `Wilddog_Node_T` 指针类型。指向 `root` 节点的指针。 |
| path |  `char` 指针类型。指向相对子路径的指针。|

**返回值**

成功返回查找到的节点的指针，否则返回 NULL。

**示例**

```c
Wilddog_Node_T *p_father = wilddog_node_createObject((Wilddog_Str_T *)"123");
Wilddog_Node_T *p_child = wilddog_node_createFalse((Wilddog_Str_T *)"this is key");
wilddog_node_addChild(p_father, p_child);

//p_find和p_child是同一个节点
Wilddog_Node_T *p_find = wilddog_node_find(p_father, "this is key"); 
```

</br>

---

###  wilddog_node_getValue

**定义**

```c
Wilddog_Str_T* wilddog_node_getValue(Wilddog_Node_T *node, int *len)
```

**说明**

获取 `node` 节点的 value。其中，值为 true，false，null 的节点无法通过这种方式获取，而应该直接根据节点的 `d_wn_type` 得出其类型。

**参数**

| 参数名 | 说明 |
|---|---|
| root | `Wilddog_Node_T` 指针类型。指向 `root` 节点的指针。 |
| len |  `int` 指针类型。输出参数，指向该节点 value 长度指针。|

**返回值**

成功返回指向节点 value 的指针，否则返回 NULL。

**示例**

```c
int len = 0;
s32 value;
Wilddog_Node_T *p_child = wilddog_node_createNum((Wilddog_Str_T *)"this is key",234);
//len 应和sizeof(s32)相同
Wilddog_Str_T *p_data = wilddog_node_getValue(p_father, &len); 
value = (s32)(*p_data);
```

</br>

---

###  wilddog_node_setValue

**定义**

```c
Wilddog_Return_T (Wilddog_Node_T *node, unsigned char *value, int len)
```

**说明**

设置 `node` 节点的 value。其中，值为 true，false，null 的节点无法通过这种方式设置。整数类型和浮点类型的节点，len 应该为 `sizeof(s32)` 和 `sizeof(wFloat)`。

**参数**

| 参数名 | 说明 |
|---|---|
| root | `Wilddog_Node_T` 指针类型。指向 `root` 节点的指针。 |
| value | `unsigned char` 类型。指向新 value 值的指针。|
| len |  `int` 类型。新 value 长度。|

**返回值**

成功返回 0，否则返回 [错误码](/api/sync/c/error-code.html)。

**示例**

```c
int len = sizeof(s32);
s32 value = 456;
Wilddog_Node_T *p_child = wilddog_node_createNum((Wilddog_Str_T *)"this is key",234);
wilddog_node_setValue(p_father, &value, len);
```

</br>

---

## 错误码

| 错误码  | 错误信息                             | 描述                                       |
| ---- | -------------------------------- | ---------------------------------------- |
| 0    | WILDDOG_ERR_NOERR                | 无错误                                      |
| -1   | WILDDOG_ERR_NULL                 | 遇到空指针，往往是函数的指针入参传入了空值引起的。                |
| -2   | WILDDOG_ERR_INVALID              | 遇到非法值                                    |
| -3   | WILDDOG_ERR_SENDERR              | 发送出错                                     |
| -4   | WILDDOG_ERR_OBSERVEERR           | 监听错误                                     |
| -5   | WILDDOG_ERR_SOCKETERR            | socket 错误                                |
| -7   | WILDDOG_ERR_NOTAUTH              | 客户端未被认证，需要调用 wilddog_auth() 进行认证         |
| -8   | WILDDOG_ERR_QUEUEFULL            | 请求队列溢出，可以过一段时间等 sdk 处理完 queue 中的请求，再发起新的请求。也可以增大 wilddog_config.h 中 WILDDOG_REQ_QUEUE_NUM 的值 |
| -9   | WILDDOG_ERR_MAXRETRAN            | 重传错误                                     |
| -10   | WILDDOG_ERR_RECVTIMEOUT          | 传输超时，客户端未接收到云端的回应。有两方面引起该错误，一方面是客户端断网，请求没有发送出去，另一方面是网络环境差，传输中的数据包丢失。需要抓包确定 |
| -11  | WILDDOG_ERR_RECVNOMATCH          | 收到的数据不匹配。                           |
| -12  | WILDDOG_ERR_CLIENTOFFLINE        | 客户端离线                                    |
| -13  | WILDDOG_ERR_RECONNECT            | 重连提示，会话已经断线重连，并且重新获取监听数据，本次获取的数据可能是已经推送过的数据(重连后不能确定断线过程中监听的数据是否有过修改)，需要用户去甄别数据是已经推送过，还是新的数据。 |
| 200  | WILDDOG_HTTP_OK                  | 请求已成功                                    |
| 201  | WILDDOG_HTTP_CREATED             | 请求已经被实现，而且有一个新的资源已经依据请求的需要而创建            |
| 204  | WILDDOG_HTTP_NO_CONTENT          | 服务端成功处理了请求，但无需返回任何内容                     |
| 304  | WILDDOG_HTTP_NOT_MODIFIED        | 数据并没有修改                                  |
| 400  | WILDDOG_HTTP_BAD_REQUEST         | 服务端无法处理该请求                               |
| 401  | WILDDOG_HTTP_UNAUTHORIZED        | 客户端未认证，需要先调用 wilddog_auth() 发送 token，认证通过后，服务端才处理客户端的请求。 |
| 403  | WILDDOG_HTTP_FORBIDDEN           | 服务端已经收到请求，但是拒绝处理                         |
| 404  | WILDDOG_HTTP_NOT_FOUND           | 访问的 url 资源不存在                            |
| 405  | WILDDOG_HTTP_METHOD_NOT_ALLOWED  | url 对应的资源不支持该请求                          |
| 406  | WILDDOG_HTTP_NOT_ACCEPTABLE      | 服务端已经收到请求，但是拒绝处理                         |
| 412  | WILDDOG_HTTP_PRECONDITION_FAIL   | 资源(存储、流量或连接数)超出限制，请进入 Wilddog 的控制查看      |
| 413  | WILDDOG_HTTP_REQ_ENTITY_TOOLARGE | 请求的数据大小溢出                                |
| 415  | WILDDOG_HTTP_UNSUPPORT_MEDIA     | 请求的数据格式出错                                |
| 500  | WILDDOG_HTTP_INTERNAL_SERVER_ERR | 服务端出错，劳烦联系野狗工作人员                         |
| 501  | WILDDOG_HTTP_NOT_IMPLEMENTED     | 当服务器无法识别请求的方法，并且无法支持其对任何资源的请求            |
| 502  | WILDDOG_HTTP_BAD_GATEWAY         | 上游服务器接收到无效的响应                            |
| 503  | WILDDOG_HTTP_SERVICE_UNAVAILABLE | 当服务器无法识别请求的方法，并且无法支持其对任何资源的请求            |
| 504  | WILDDOG_HTTP_GATEWAY_TIMEOUT     | 中继超时                                     |
| 505  | WILDDOG_HTTP_PROXY_NOT_SUPPORT   | 服务器不支持，或者拒绝支持在请求中使用的协议版本                 |
