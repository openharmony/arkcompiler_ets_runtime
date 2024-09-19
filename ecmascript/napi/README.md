# JSNApi接口说明

## ArrayBufferRef

ArrayBufferRef 是一种通用的、固定长度的原始二进制数据缓冲区。它不能直接读取或操作，需要通过类型数组或 DataView 对象来操作其中的数据。

### New

Local<ArrayBufferRef> ArrayBufferRef::New(const EcmaVM *vm, int32_t length)；

Local<ArrayBufferRef> ArrayBufferRef::New(const EcmaVM *vm, void *buffer, int32_t length, const NativePointerCallback &deleter, void *data)；

创建一个ArrayBuffer对象。

**参数：**

| 参数名  | 类型            | 必填 | 说明                        |
| ------- | --------------- | ---- | --------------------------- |
| vm      | const EcmaVM *  | 是   | 虚拟机对象。                |
| length  | int32_t         | 是   | 指定的长度。                |
| buffer  | void *          | 是   | 指定缓冲区。                |
| deleter | const NativePointerCallback & | 是   | 删除ArrayBuffer时所作的操作 |
| data    | void *          | 是   | 指定数据。                  |

**返回值：**

| 类型                  | 说明                             |
| --------------------- | -------------------------------- |
| Local<ArrayBufferRef> | 返回新创建的ArrayBufferRef对象。 |

**示例：**

```C++
Local<ArrayBufferRef> arrayBuffer1 = ArrayBufferRef::New(vm, 10);
uint8_t *buffer = new uint8_t[10]();
int *data = new int;
*data = 10;
NativePointerCallback deleter = [](void *env, void *buffer, void *data) -> void {
    delete[] reinterpret_cast<uint8_t *>(buffer);
    int *currentData = reinterpret_cast<int *>(data);
    delete currentData;
};
Local<ArrayBufferRef> arrayBuffer2 = ArrayBufferRef::New(vm, buffer, 10, deleter, data);
```

### GetBuffer

void *ArrayBufferRef::GetBuffer()；

获取ArrayBufferRef的原始缓冲区。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型   | 说明                                             |
| ------ | ------------------------------------------------ |
| void * | 返回为void *，使用时需强转为缓冲区创建时的类型。 |

**示例：**

```c++
uint8_t *buffer = new uint8_t[10]();
int *data = new int;
*data = 10;
NativePointerCallback deleter = [](void *env, void *buffer, void *data) -> void {
delete[] reinterpret_cast<uint8_t *>(buffer);
    int *currentData = reinterpret_cast<int *>(data);
    delete currentData;
};
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, buffer, 10, deleter, data);
uint8_t *getBuffer = reinterpret_cast<uint8_t *>(arrayBuffer->GetBuffer());
```

### ByteLength

int32_t ArrayBufferRef::ByteLength([[maybe_unused]] const EcmaVM *vm)；

此函数返回此ArrayBufferRef缓冲区的长度（以字节为单位）。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型    | 说明                            |
| ------- | ------------------------------- |
| int32_t | 以int32_t类型返回buffer的长度。 |

**示例：**

```c++
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, 10);
int lenth = arrayBuffer->ByteLength(vm);
```

### IsDetach

bool ArrayBufferRef::IsDetach()；

判断ArrayBufferRef与其缓冲区是否已经分离。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型 | 说明                                      |
| ---- | ----------------------------------------- |
| bool | 缓冲区已经分离返回true，未分离返回false。 |

**示例：**

```C++
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, 10);
bool b = arrayBuffer->IsDetach();
```

### Detach

void ArrayBufferRef::Detach(const EcmaVM *vm)；

将ArrayBufferRef与其缓冲区分离，并将缓冲区长度置为0。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型 | 说明                             |
| ---- | -------------------------------- |
| void | 将ArrayBufferRef与其缓冲区分离。 |

**示例：**

```C++
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, 10);
arrayBuffer->Detach(vm);
```



## BooleanRef

Boolean是一个基本数据类型，用于表示真或假。

### New

Local<BooleanRef> BooleanRef::New(const EcmaVM *vm, bool input)；

创建一个BooleanRef对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明                         |
| ------ | -------------- | ---- | ---------------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象。虚拟机对象。     |
| input  | bool           | 是   | 指定BooleanRef对象的bool值。 |

**返回值：**

| 类型              | 说明                         |
| ----------------- | ---------------------------- |
| Local<BooleanRef> | 返回新创建的BooleanRef对象。 |

**示例：**

```c++
Local<BooleanRef> boolRef = BooleanRef::New(vm, true);
```

## BufferRef

用于在数据从一个位置传输到另一个位置时临时存储数据。

### New

Local<BufferRef> BufferRef::New(const EcmaVM *vm, int32_t length);

Local<BufferRef> BufferRef::New(const EcmaVM *vm, void *buffer, int32_t length, const NativePointerCallback &deleter, void *data)

创建一个BufferRef对象。

**参数：**

| 参数名  | 类型            | 必填 | 说明                                               |
| ------- | --------------- | ---- | -------------------------------------------------- |
| vm      | const EcmaVM *  | 是   | 虚拟机对象。                                       |
| length  | int32_t         | 是   | 指定的长度。                                       |
| buffer  | void *          | 是   | 指定缓冲区                                         |
| deleter | const NativePointerCallback & | 是   | 一个删除器对象，用于在不再需要缓冲区时释放其内存。 |
| data    | void *          | 是   | 传递给删除器的额外数据。                           |

**返回值：**

| 类型             | 说明                        |
| ---------------- | --------------------------- |
| Local<BufferRef> | 返回新创建的BufferRef对象。 |

**示例：**

```c++
Local<BufferRef> bufferRef1 = BufferRef::New(vm, 10);
uint8_t *buffer = new uint8_t[10]();
int *data = new int;
*data = 10;
NativePointerCallback deleter = [](void *env, void *buffer, void *data) -> void {
    delete[] reinterpret_cast<uint8_t *>(buffer);
    int *currentData = reinterpret_cast<int *>(data);
    delete currentData;
};
Local<BufferRef> bufferRef2 = BufferRef::New(vm, buffer, 10, deleter, data);
```

### ByteLength

int32_t BufferRef::ByteLength(const EcmaVM *vm)；

此函数返回此ArrayBufferRef缓冲区的长度（以字节为单位）。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| :----: | -------------- | ---- | ------------ |
|   vm   | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型    | 说明                            |
| ------- | ------------------------------- |
| int32_t | 以int32_t类型返回buffer的长度。 |

**示例：**

```c++
Local<BufferRef> buffer = BufferRef::New(vm, 10);
int32_t lenth = buffer->ByteLength(vm);
```

### GetBuffer

void *BufferRef::GetBuffer()；

获取BufferRef的原始缓冲区。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型   | 说明                                             |
| ------ | ------------------------------------------------ |
| void * | 返回为void *，使用时需强转为缓冲区创建时的类型。 |

**示例：**

```c++
uint8_t *buffer = new uint8_t[10]();
int *data = new int;
*data = 10;
NativePointerCallback deleter = [](void *env, void *buffer, void *data) -> void {
    delete[] reinterpret_cast<uint8_t *>(buffer);
    int *currentData = reinterpret_cast<int *>(data);
    delete currentData;
};
Local<BufferRef> bufferRef = BufferRef::New(vm, buffer, 10, deleter, data);
uint8_t *getBuffer = reinterpret_cast<uint8_t *>(bufferRef->GetBuffer());
```
### BufferToStringCallback

static ecmascript::JSTaggedValue BufferToStringCallback(ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo);

调用ToString函数时会触发的回调函数。

**参数：**

| 参数名              | 类型                   | 必填 | 说明                                   |
| ------------------- | ---------------------- | ---- | -------------------------------------- |
| ecmaRuntimeCallInfo | EcmaRuntimeCallInfo  * | 是   | 设置指定的回调函数，以及所需要的参数。 |

**返回值：**

| 类型          | 说明                                                         |
| ------------- | ------------------------------------------------------------ |
| JSTaggedValue | 将设置回调函数的结果转换为JSTaggedValue类型，并作为此函数的返回值。 |

**示例：**

```C++
Local<BufferRef> bufferRef = BufferRef::New(vm, 10);
Local<StringRef> bufferStr = bufferRef->ToString(vm);
```

## DataViewRef

一种用于操作二进制数据的视图对象，它提供了一种方式来访问和修改 ArrayBuffer 中的字节。

### New

static Local<DataViewRef> New(const EcmaVM *vm, Local<ArrayBufferRef> arrayBuffer, uint32_t byteOffset,uint32_t byteLength)；

创建一个新的DataView对象。

**参数：**

| 参数名      | 类型                  | 必填 | 说明                       |
| ----------- | --------------------- | ---- | -------------------------- |
| vm          | const EcmaVM *        | 是   | 虚拟机对象。               |
| arrayBuffer | Local<ArrayBufferRef> | 是   | 指定的缓冲区。             |
| byteOffset  | uint32_t              | 是   | 从第几个字节开始写入数据。 |
| byteLength  | uint32_t              | 是   | 要操作的缓冲区的长度。     |

**返回值：**

| 类型               | 说明                   |
| ------------------ | ---------------------- |
| Local<DataViewRef> | 一个新的DataView对象。 |

**示例：**

```c++
const int32_t length = 15;
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, length);
Local<DataViewRef> dataObj = DataViewRef::New(vm, arrayBuffer, 5, 7);
```

### ByteOffset

uint32_t DataViewRef::ByteOffset()；

获取DataViewRef缓冲区的偏移量。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型     | 说明                               |
| -------- | ---------------------------------- |
| uint32_t | 该缓冲区从那个字节开始写入或读取。 |

**示例：**

```C++
const int32_t length = 15;
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, length);
Local<DataViewRef> dataView = DataViewRef::New(vm, arrayBuffer, 5, 7);
uint32_t offSet = dataView->ByteOffset();
```

### ByteLength

uint32_t DataViewRef::ByteLength()；

获取DataViewRef缓冲区可操作的长度。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型     | 说明                            |
| -------- | ------------------------------- |
| uint32_t | DataViewRef缓冲区可操作的长度。 |

**示例：**

```C++
const int32_t length = 15;
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, length);
Local<DataViewRef> dataView = DataViewRef::New(vm, arrayBuffer, 5, 7);
uint32_t offSet = dataView->ByteLength();
```

### GetArrayBuffer

Local<ArrayBufferRef> DataViewRef::GetArrayBuffer(const EcmaVM *vm)；

获取DataViewRef对象的缓冲区。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| :----: | -------------- | ---- | ------------ |
|   vm   | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型                  | 说明                                                         |
| --------------------- | ------------------------------------------------------------ |
| Local<ArrayBufferRef> | 获取DataViewRef的缓冲区，将其转换为Local<ArrayBufferRef>类型，并作为函数的返回值。 |

**示例：**

```c++
const int32_t length = 15;
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, length);
Local<DataViewRef> dataView = DataViewRef::New(vm, arrayBuffer, 5, 7);
Local<ArrayBufferRef> getBuffer = dataView->GetArrayBuffer(vm);
```

## DateRef

Date对象用于表示日期和时间。它提供了许多方法和属性来处理日期和时间。

### GetTime

double DateRef::GetTime()；

该方法返回自纪元以来该日期的毫秒数，该纪元定义为 1970 年 1 月 1 日，UTC 开始时的午夜。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型   | 说明                                             |
| ------ | ------------------------------------------------ |
| double | 一个double数字，表示此日期的时间戳以毫秒为单位。 |

**示例：**

```c++
double time = 1690854800000; // 2023-07-04T00:00:00.000Z
Local<DateRef> object = DateRef::New(vm, time);
double getTime = object->GetTime();
```

## JSNApi

JSNApi提供了一些通用的接口用于查询或获取一些对象属性。

### IsBundle

bool JSNApi::IsBundle(EcmaVM *vm)；

判断将文件或者模块打包时，是不是JSBundle模式。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| :----: | -------------- | ---- | ------------ |
|   vm   | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型 | 说明                                          |
| ---- | --------------------------------------------- |
| bool | 当为JSBundle模式时时返回true，否则返回false。 |

**示例：**

```c++
bool b = JSNApi::IsBundle(vm);
```

### addWorker

void JSNApi::addWorker(EcmaVM *hostVm, EcmaVM *workerVm)

创建一个新的虚拟机添加到指定虚拟机的工作列表中。

**参数：**

|  参数名  | 类型           | 必填 | 说明                 |
| :------: | -------------- | ---- | -------------------- |
|  hostVm  | const EcmaVM * | 是   | 指定虚拟机对象。     |
| workerVm | const EcmaVM * | 是   | 创建新的工作虚拟机。 |

**返回值：**

| 类型 | 说明       |
| ---- | ---------- |
| void | 无返回值。 |

**示例：**

```c++
JSRuntimeOptions option;
EcmaVM *workerVm = JSNApi::CreateEcmaVM(option);
JSNApi::addWorker(hostVm, workerVm);
```

### SerializeValue

void *JSNApi::SerializeValue(const EcmaVM *vm, Local<JSValueRef> value, Local<JSValueRef> transfer)

将value序列化为transfer类型。

**参数：**

|  参数名  | 类型              | 必填 | 说明               |
| :------: | ----------------- | ---- | ------------------ |
|    vm    | const EcmaVM *    | 是   | 指定虚拟机对象。   |
|  value   | Local<JSValueRef> | 是   | 需要序列化的数据。 |
| transfer | Local<JSValueRef> | 是   | 序列化的类型。     |

**返回值：**

| 类型   | 说明                                                         |
| ------ | ------------------------------------------------------------ |
| void * | 转化为SerializationData *可调用GetData，GetSize获取序列化的数据以及大小。 |

**示例：**

```c++
Local<JSValueRef> value = StringRef::NewFromUtf8(vm, "abcdefbb");
Local<JSValueRef> transfer = StringRef::NewFromUtf8(vm, "abcdefbb");
SerializationData *ptr = JSNApi::SerializeValue(vm, value, transfer);
```

### DisposeGlobalHandleAddr

static void DisposeGlobalHandleAddr(const EcmaVM *vm, uintptr_t addr);

释放全局句柄

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| :----: | -------------- | ---- | ---------------- |
|   vm   | const EcmaVM * | 是   | 指定虚拟机对象。 |
|  addr  | uintptr_t      | 是   | 全局句柄的地址。 |

**返回值：**

| 类型 | 说明       |
| ---- | ---------- |
| void | 无返回值。 |

**示例：**

```C++
Local<JSValueRef> value = StringRef::NewFromUtf8(vm, "abc");
uintptr_t address = JSNApi::GetGlobalHandleAddr(vm, reinterpret_cast<uintptr_t>(*value));
JSNApi::DisposeGlobalHandleAddr(vm, address);
```

### CheckSecureMem

static bool CheckSecureMem(uintptr_t mem);

检查给定的内存地址是否安全。

**参数：**

| 参数名 | 类型      | 必填 | 说明                 |
| :----: | --------- | ---- | -------------------- |
|  mem   | uintptr_t | 是   | 需要检查的内存地址。 |

**返回值：**

| 类型 | 说明                                |
| ---- | ----------------------------------- |
| bool | 内存地址安全返回true否则返回false。 |

**示例：**

```c++
Local<JSValueRef> value = StringRef::NewFromUtf8(vm, "abc");
uintptr_t address = JSNApi::GetGlobalHandleAddr(vm, reinterpret_cast<uintptr_t>(*value));
bool b = CheckSecureMem(address);
```

### GetGlobalObject

Local<ObjectRef> JSNApi::GetGlobalObject(const EcmaVM *vm)

用于能否成功获取JavaScript虚拟机的env全局对象，以及该全局对象是否为非空对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| :----: | -------------- | ---- | ---------------- |
|   vm   | const EcmaVM * | 是   | 指定虚拟机对象。 |

**返回值：**

| 类型             | 说明                                                      |
| ---------------- | --------------------------------------------------------- |
| Local<ObjectRef> | 可调用ObjectRef以及父类JSValueRef的函数来判断其是否有效。 |

**示例：**

```C++
Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm);
bool b = globalObject.IsEmpty();
```

### GetUncaughtException

Local<ObjectRef> JSNApi::GetUncaughtException(const EcmaVM *vm)；

获取虚拟机未捕获的异常

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| :----: | -------------- | ---- | ---------------- |
|   vm   | const EcmaVM * | 是   | 指定虚拟机对象。 |

**返回值：**

| 类型             | 说明                                                      |
| ---------------- | --------------------------------------------------------- |
| Local<ObjectRef> | 可调用ObjectRef以及父类JSValueRef的函数来判断其是否有效。 |

**示例：**

```c++
Local<ObjectRef> excep = JSNApi::GetUncaughtException(vm);
if (!excep.IsNull()) {
    // Error Message ...
}
```

### GetAndClearUncaughtException

Local<ObjectRef> JSNApi::GetAndClearUncaughtException(const EcmaVM *vm)；

用于获取并清除未捕获的虚拟机异常。

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| :----: | -------------- | ---- | ---------------- |
|   vm   | const EcmaVM * | 是   | 指定虚拟机对象。 |

**返回值：**

| 类型             | 说明                                                      |
| ---------------- | --------------------------------------------------------- |
| Local<ObjectRef> | 可调用ObjectRef以及父类JSValueRef的函数来判断其是否有效。 |

**示例：**

```C++
Local<ObjectRef> excep = JSNApi::GetUncaughtException(vm);
if (!excep.IsNull()) {
    // Error Message ...
    JSNApi::GetAndClearUncaughtException(vm);
}
```

### HasPendingException

bool JSNApi::HasPendingException(const EcmaVM *vm)；

用于检查虚拟机是否有未处理的异常。

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| :----: | -------------- | ---- | ---------------- |
|   vm   | const EcmaVM * | 是   | 指定虚拟机对象。 |

**返回值：**

| 类型 | 说明                                          |
| ---- | --------------------------------------------- |
| bool | 如果此虚拟机有异常产生返回true否则返回false。 |

**示例：**

```c++
if (JSNApi::HasPendingException(vm)) {
    JSNApi::PrintExceptionInfo(const EcmaVM *vm);
}
```

### PrintExceptionInfo

void JSNApi::PrintExceptionInfo(const EcmaVM *vm)；

用于打印未捕获的异常，并清除此虚拟机的异常。

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| :----: | -------------- | ---- | ---------------- |
|   vm   | const EcmaVM * | 是   | 指定虚拟机对象。 |

**返回值：**

| 类型 | 说明       |
| ---- | ---------- |
| void | 无返回值。 |

**示例：**

```C++
if (JSNApi::HasPendingException(vm)) {
    JSNApi::PrintExceptionInfo(vm);
}
```

### SetWeakCallback

uintptr_t JSNApi::SetWeakCallback(const EcmaVM *vm, uintptr_t localAddress, void *ref, WeakRefClearCallBack freeGlobalCallBack, WeakRefClearCallBack nativeFinalizeCallback)；

此函数用于设置一个弱回调函数。弱回调函数是一种特殊类型的回调函数，它可以在不需要时自动释放内存，以避免内存泄漏问题。

**参数：**

|         参数名         | 类型                 | 必填 | 说明                                                       |
| :--------------------: | -------------------- | ---- | ---------------------------------------------------------- |
|           vm           | const EcmaVM *       | 是   | 指定虚拟机对象。                                           |
|      localAddress      | uintptr_t            | 是   | 本地地址，指设置的弱引用在内存地址                         |
|          ref           | void *               | 是   | 需要引用对象的内存地址。                                   |
|   freeGlobalCallBack   | WeakRefClearCallBack | 是   | 弱引用清除回调函数，当弱引用被清除时，该回调函数将被调用。 |
| nativeFinalizeCallback | WeakRefClearCallBack | 是   | C++原生对象的析构函数。                                    |

**返回值：**

| 类型      | 说明                       |
| --------- | -------------------------- |
| uintptr_t | 弱引用在内存空间中的地址。 |

**示例：**

```C++
template <typename T> void FreeGlobalCallBack(void *ptr)
{
    T *i = static_cast<T *>(ptr);
}
template <typename T> void NativeFinalizeCallback(void *ptr)
{
    T *i = static_cast<T *>(ptr);
    delete i;
}
Global<JSValueRef> global(vm, BooleanRef::New(vm, true));
void *ref = new char('a');
WeakRefClearCallBack freeGlobalCallBack = FreeGlobalCallBack<char>;
WeakRefClearCallBack nativeFinalizeCallback = NativeFinalizeCallback<char>;
global.SetWeakCallback(ref, freeGlobalCallBack, nativeFinalizeCallback);
```

### ThrowException

void JSNApi::ThrowException(const EcmaVM *vm, Local<JSValueRef> error)；

VM虚拟机抛出指定异常。

**参数：**

| 参数名 | 类型              | 必填 | 说明             |
| :----: | ----------------- | ---- | ---------------- |
|   vm   | const EcmaVM *    | 是   | 指定虚拟机对象。 |
| error  | Local<JSValueRef> | 是   | 指定error信息。  |

**返回值：**

| 类型 | 说明       |
| ---- | ---------- |
| void | 无返回值。 |

**示例：**

```C++
Local<JSValueRef> error = Exception::Error(vm, StringRef::NewFromUtf8(vm, "Error Message"));
JSNApi::ThrowException(vm, error);
```

### NotifyNativeCalling

static void NotifyNativeCalling(const EcmaVM *vm, const void *nativeAddress);

void JSNApi::NotifyNativeCalling([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] const void *nativeAddress)；

在特定条件下通知原生调用事件。

**参数：**

| 参数名        | 类型                            | 必填 | 说明                |
| ------------- | ------------------------------- | ---- | ------------------- |
| vm            | [[maybe_unused]] const EcmaVM * | 是   | 虚拟机对象。        |
| nativeAddress | [[maybe_unused]] const void *   | 是   | C++原生对象的地址。 |

**返回值：**

无

**示例：**

```c++
void *par = new int(0);
JSNApi::NotifyNativeCalling(vm_, par);
```

### function

std::function<bool(std::string dirPath, uint8_t **buff, size_t *buffSize)> cb);

一个函数对象，可以用于存储、传递和调用一个可调用对象（如函数、lambda表达式或函数对象），以便在需要时执行相应的操作。

**参数：**

| 参数名   | 类型        | 必填 | 说明                                            |
| -------- | ----------- | ---- | ----------------------------------------------- |
| dirPath  | std::string | 是   | 字符串类型的参数，表示目录路径。                |
| buff     | uint8_t **  | 是   | 指向uint8_t类型指针的指针，用于存储缓冲区数据。 |
| buffSize | size_t *    | 是   | 指向size_t类型的指针，用于存储缓冲区大小。      |

**返回值：**

| 类型 | 说明                                                  |
| ---- | ----------------------------------------------------- |
| bool | 布尔值：true 或 false。表示该函数执行的操作是否成功。 |

**示例：**

```c++
static *void *SetHostResolveBufferTracker(EcmaVM **vm*, std::function<*bool*(std::string dirPath, *uint8_t* **buff, *size_t* *buffSize)> *cb*);
```

### SetProfilerState

static void JSNApi::SetProfilerState(const EcmaVM *vm, bool value);

用于设置虚拟机（EcmaVM）的CPUProfiler采样状态。

**参数：**

| 参数名 | 类型           | 必填 | 说明                    |
| ------ | -------------- | ---- | ----------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象。            |
| value  | bool           | 是   | 布尔值：true 或 false。 |

**返回值：**

无

**示例：**

```c++
bool ret = true;
JSNApi::SetProfilerState(vm_, ret);
```

### IsProfiling

static bool JSNApi::IsProfiling(EcmaVM *vm);

用于检查虚拟机（EcmaVM）是否处于CPUProfiling采样状态。

**参数：**

| 参数名 | 类型     | 必填 | 说明         |
| ------ | -------- | ---- | ------------ |
| vm     | EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 如果虚拟机（EcmaVM）处于CPUProfiling采样状态，返回true。否则，返回false。 |

**示例：**

```c++
bool b = JSNApi::IsProfiling(vm_);
```

### SynchronizVMInfo

static void JSNApi::SynchronizVMInfo(EcmaVM *vm, const EcmaVM *hostVM);

将一个EcmaVM对象的信息复制到另一个EcmaVM对象中，使它们具有相同的信息和属性。

**参数：**

| 参数名 | 类型     | 必填 | 说明                       |
| ------ | -------- | ---- | -------------------------- |
| vm     | EcmaVM * | 是   | 虚拟机对象（复制源）。     |
| hostVM | EcmaVM * | 是   | 虚拟机对象（复制目的地）。 |

**返回值：**

无

**示例：**

```c++
JSNApi::SynchronizVMInfo(vm_, hostVM_);
```

### SetLoop

static void JSNApi::SetLoop(EcmaVM *vm, void *loop);

将loop指针传递给EcmaVM对象vm的SetLoop方法。

**参数：**

| 参数名 | 类型     | 必填 | 说明           |
| ------ | -------- | ---- | -------------- |
| vm     | EcmaVM * | 是   | 虚拟机对象。   |
| loop   | void *   | 是   | 循环回调函数。 |

**返回值：**

无

**示例：**

```c++
JSNApi::SetLoop(vm_, loop);
```

### SetAssetPath

static void JSNApi::SetAssetPath(EcmaVM *vm, const std::string &assetPath);

将assetPath字符串转换为C风格的字符串，并将其传递给EcmaVM对象vm的SetAssetPath方法。

**参数：**

| 参数名    | 类型                | 必填 | 说明             |
| --------- | ------------------- | ---- | ---------------- |
| vm        | EcmaVM *            | 是   | 虚拟机对象。     |
| assetPath | const std::string & | 是   | 要设置的资源路径 |

**返回值：**

无

**示例：**

```c++
JSNApi::SetAssetPath(vm_, assetPath);
```

### SetBundle

static void JSNApi::SetBundle(EcmaVM *vm, bool value);

将value传递给EcmaVM对象vm的SetIsBundlePack方法。

**参数：**

| 参数名 | 类型     | 必填 | 说明                    |
| ------ | -------- | ---- | ----------------------- |
| vm     | EcmaVM * | 是   | 虚拟机对象。            |
| value  | bool     | 是   | 布尔值：true 或 false。 |

**返回值：**

无

**示例：**

```c++
JSNApi::SetBundle(vm_, value);
```

### SwitchCurrentContext

static void JSNApi::SwitchCurrentContext(EcmaVM *vm, EcmaContext *context);

将当前上下文切换到指定的context。

**参数：**

| 参数名  | 类型          | 必填 | 说明               |
| ------- | ------------- | ---- | ------------------ |
| vm      | EcmaVM *      | 是   | 虚拟机对象。       |
| context | EcmaContext * | 是   | 要切换到的上下文。 |

**返回值：**

无

**示例：**

```c++
JSNApi::SwitchCurrentContext(vm_, context);
```

### LoadAotFile

static void JSNApi::LoadAotFile(EcmaVM *vm, const std::string &moduleName);

加载一个Ahead-of-Time（AOT）文件。

**参数：**

| 参数名     | 类型                | 必填 | 说明                      |
| ---------- | ------------------- | ---- | ------------------------- |
| vm         | EcmaVM *            | 是   | 虚拟机对象。              |
| moduleName | const std::string & | 是   | 要加载的 AOT 文件的名称。 |

**返回值：**

无

**示例：**

```c++
JSNApi::LoadAotFile(vm_, moduleName);
```

### DestroyJSVM

static void JSNApi::DestroyJSVM(EcmaVM *ecmaVm);

销毁一个指定的ECMA虚拟机（EcmaVM）。

**参数：**

| 参数名 | 类型     | 必填 | 说明                 |
| ------ | -------- | ---- | -------------------- |
| ecmaVm | EcmaVM * | 是   | 要销毁的虚拟机对象。 |

**返回值：**

无

**示例：**

```c++
 JSNApi::DestroyJSVM(ecmaVm);
```

### DeleteSerializationData

void JSNApi::DeleteSerializationData(void *data);

删除一个序列化数据对象。

**参数：**

| 参数名 | 类型   | 必填 | 说明                 |
| ------ | ------ | ---- | -------------------- |
| data   | void * | 是   | 要删除的序列化数据。 |

**返回值：**

无

**示例：**

```c++
JSNApi::DeleteSerializationData(data);
```

### DeserializeValue

Local<JSValueRef> JSNApi::DeserializeValue(const EcmaVM *vm, void *recoder, void *hint);

将序列化数据反序列化为JavaScript值。

**参数：**

| 参数名  | 类型           | 必填 | 说明                     |
| ------- | -------------- | ---- | ------------------------ |
| vm      | const EcmaVM * | 是   | 虚拟机对象。             |
| recoder | void *         | 是   | 用于反序列化的编解码器。 |
| hint    | void *         | 是   | 可选的提示对象。         |

**返回值;**

| 类型              | 说明                                       |
| ----------------- | ------------------------------------------ |
| Local<JSValueRef> | 返回转换成 JSValueRef 类型的反序列化数据。 |

**示例：**

```c++
 Local<JSValueRef> value = JSNApi::DeserializeValue(vm_, recoder, hint);
```

### Execute

bool JSNApi::Execute(EcmaVM *vm, const uint8_t *data, int32_t size, const std::string &entry,const std::string &filename, bool needUpdate);

执行一个Ark缓冲区文件。

**参数：**

| 参数名     | 类型                | 必填 | 说明                                                        |
| ---------- | ------------------- | ---- | ----------------------------------------------------------- |
| vm         | EcmaVM *            | 是   | 虚拟机对象。                                                |
| data       | const uint8_t *     | 是   | JavaScript代码的字节数组。                                  |
| size       | int32_t             | 是   | JavaScript代码的大小（以字节为单位）。                      |
| entry      | const std::string & | 是   | JavaScript代码入口点的名称。                                |
| filename   | const std::string & | 是   | JavaScript代码文件名。                                      |
| needUpdate | bool                | 是   | 布尔值：true 或 false。（表示是否需要更新虚拟机对象的状态） |

**返回值：**

| 类型 | 说明                                               |
| ---- | -------------------------------------------------- |
| bool | 执行Ark缓冲区文件成功，返回true。否则，返回false。 |

**示例：**

```c++
bool b = JSNApi::Execute(vm_, data, size, entry,filename, needUpdate);
```

### Execute

bool JSNApi::Execute(EcmaVM *vm, const std::string &fileName, const std::string &entry, bool needUpdate);

执行一个Ark文件。

**参数：**

| 参数名     | 类型                | 必填 | 说明                                                        |
| ---------- | ------------------- | ---- | ----------------------------------------------------------- |
| vm         | EcmaVM *            | 是   | 虚拟机对象。                                                |
| fileName   | const std::string & | 是   | JavaScript代码文件名。                                      |
| entry      | const std::string & | 是   | JavaScript代码入口点的名称。                                |
| needUpdate | bool                | 是   | 布尔值：true 或 false。（表示是否需要更新虚拟机对象的状态） |

**返回值：**

| 类型 | 说明                                         |
| ---- | -------------------------------------------- |
| bool | 执行Ark文件成功，返回true。否则，返回false。 |

**示例：**

```c++
bool b = JSNApi::Execute(vm_, fileName, entry, needUpdate);
```

### SetHostPromiseRejectionTracker

void JSNApi::SetHostPromiseRejectionTracker(EcmaVM *vm, void *cb, void* data);

用于设置与JavaScript虚拟机（EcmaVM）相关联的宿主Promise拒绝跟踪器（Host Promise Rejection Tracker）。

**参数：**

| 参数名 | 类型     | 必填 | 说明                                          |
| ------ | -------- | ---- | --------------------------------------------- |
| vm     | EcmaVM * | 是   | 虚拟机对象。                                  |
| cb     | void     | 是   | 要设置为当前ECMA上下文的Promise拒绝回调函数。 |
| data   | void     | 是   | 要设置为当前ECMA上下文的数据。                |

**返回值：**

无

**示例：**

```c++
JSNApi::SetHostPromiseRejectionTracker(vm_, cb, data);
```

### ExecuteModuleFromBuffer

static bool ExecuteModuleFromBuffer(EcmaVM *vm, const void *data, int32_t size, const std::string &file);

从给定的数据缓冲区中执行一个模块。

**参数：**

| 参数名 | 类型              | 必填 | 说明                   |
| ------ | ----------------- | ---- | ---------------------- |
| vm     | EcmaVM *          | 是   | 虚拟机对象             |
| data   | const void *      | 是   | 要执行的模块数据       |
| size   | int32_t           | 是   | 要执行的模块数据的大小 |
| file   | const std::string | 是   | 要执行的模块文件的名称 |

**返回值：**

| 类型    | 说明                                          |
| :------ | :-------------------------------------------- |
| boolean | 如果执行模块成功，则返回true，否则返回false。 |

**示例：**

```c++
const void *data = "TestData";
int32_t size = 8;
const std::string file = "TestFile";
bool result = JSNApi::ExecuteModuleFromBuffer(vm_, data, size, file);
```

### GetExportObject

static Local<ObjectRef> GetExportObject(EcmaVM *vm, const std::string &file, const std::string &key);

根据指定的文件和key值获取并导出对象。

**参数：**

| 参数名 | 类型              | 必填 | 说明           |
| ------ | ----------------- | ---- | -------------- |
| vm     | EcmaVM *          | 是   | 虚拟机对象     |
| file   | const std::string | 是   | 指定的文件名称 |
| key    | const std::string | 是   | 指定的key值    |

**返回值：**

| 类型             | 说明                   |
| :--------------- | :--------------------- |
| Local<ObjectRef> | 返回获取并导出的对象。 |

**示例：**

```c++
const std::string file = "TestFile";
const std::string key = "TestKey";
Local<ObjectRef> result = GetExportObject(vm_, file, key);
```

### GetExportObjectFromBuffer

static Local<ObjectRef> GetExportObjectFromBuffer(EcmaVM *vm, const std::string &file, const std::string &key);

从给定的缓冲区中根据指定的文件和key值获取并导出对象。

**参数：**

| 参数名 | 类型              | 必填 | 说明           |
| ------ | ----------------- | ---- | -------------- |
| vm     | EcmaVM *          | 是   | 虚拟机对象     |
| file   | const std::string | 是   | 指定的文件名称 |
| key    | const std::string | 是   | 指定的key值    |

**返回值：**

| 类型             | 说明                                   |
| :--------------- | :------------------------------------- |
| Local<ObjectRef> | 返回从给定的缓冲区中获取并导出的对象。 |

**示例：**

```c++
const std::string file = "TestFile";
const std::string key = "TestKey";
Local<ObjectRef> result = GetExportObjectFromBuffer(vm_, file, key);
```

### GetAndClearUncaughtException

static Local<ObjectRef> GetAndClearUncaughtException(const EcmaVM *vm);

获取并清理未捕获的异常。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型             | 说明                   |
| :--------------- | :--------------------- |
| Local<ObjectRef> | 返回未捕获的异常对象。 |

**示例：**

```c++
Local<ObjectRef> result = JSNApi::GetAndClearUncaughtException(vm_);
```

### GetUncaughtException

static Local<ObjectRef> GetUncaughtException(const EcmaVM *vm);

获取未捕获的异常。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型             | 说明                   |
| :--------------- | :--------------------- |
| Local<ObjectRef> | 返回未捕获的异常对象。 |

**示例：**

```c++
Local<ObjectRef> result = JSNApi::GetUncaughtException(vm_);
```

### HasPendingException

static bool HasPendingException(const EcmaVM *vm);

检查是否有待处理的异常。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型    | 说明                                            |
| :------ | :---------------------------------------------- |
| boolean | 如果有待处理的异常，则返回true，否则返回false。 |

**示例：**

```c++
bool result = JSNApi::HasPendingException(vm_);
```

### HasPendingJob

static bool HasPendingJob(const EcmaVM *vm);

检查是否有待处理的任务。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型    | 说明                                            |
| :------ | :---------------------------------------------- |
| boolean | 如果有待处理的任务，则返回true，否则返回false。 |

**示例：**

```c++
bool result = JSNApi::HasPendingJob(vm_);
```

###  DestroyJSContext

static void DestroyJSContext(EcmaVM *vm, EcmaContext *context);

该函数的作用是销毁与给定虚拟机和上下文相关联的资源，通过 `EcmaContext::CheckAndDestroy` 方法实现检查和销毁过程。

**参数：**

| 参数名  | 类型          | 必填 | 说明                   |
| ------- | ------------- | ---- | ---------------------- |
| vm      | EcmaVM *      | 是   | 虚拟机对象。           |
| context | EcmaContext * | 是   | 表示Ecma上下文的指针。 |

**返回值：**

无

**示例：**

```c++
EcmaContext *context1 = JSNApi::CreateJSContext(vm);
JSNApi::DestroyJSContext(vm, context1);
```

###  SetMockModuleList

static void SetMockModuleList(EcmaVM *vm, const std::map<std::string, std::string> &list);

将给定的模拟模块列表设置到指定的Ecma虚拟机中。

**参数：**

| 参数名 | 类型                                       | 必填 | 说明                                                         |
| ------ | ------------------------------------------ | ---- | ------------------------------------------------------------ |
| vm     | EcmaVM *                                   | 是   | 虚拟机对象。                                                 |
| list   | const std::map<std::string, std::string> & | 是   | 表示一个从字符串到字符串的映射，<br/>其中第一个字符串是模块的名称，第二个字符串是模块的内容。 |

**返回值：**

无

**示例：**

```c++
std::map<std::string, std::string> mockModuleList;
mockModuleList["Module1"] = "Module1Context";
mockModuleList["Module2"] = "Module2Context";
JSNApi::SetMockModuleList(vm, mockModuleList);
```

###  SetHostPromiseRejectionTracker

static void SetHostPromiseRejectionTracker(EcmaVM *vm, void *cb, void *data);

该函数用于设置 Ecma 上下文的主机 Promise 拒绝跟踪器，以及相关的回调函数和数据。

`SetHostPromiseRejectionTracker` 方法设置主机 Promise 拒绝跟踪器的回调函数。

`SetPromiseRejectCallback` 方法设置 Promise 拒绝的回调函数。

`SetData` 方法设置特定的数据。

**参数：**

| 参数名 | 类型     | 必填 | 说明                                    |
| ------ | -------- | ---- | --------------------------------------- |
| vm     | EcmaVM * | 是   | 虚拟机对象。                            |
| cb     | Void *   | 是   | 表示主机 Promise 拒绝跟踪器的回调函数。 |
| data   | Void *   | 是   | 表示将要设置的数据。                    |

**返回值：**

无

**示例：**

```c++
void *data = reinterpret_cast<void *>(builtins::BuiltinsFunction::FunctionPrototypeInvokeSelf);
JSNApi::SetHostPromiseRejectionTracker(vm, data, data);
```

###  SetHostResolveBufferTracker

static void SetHostResolveBufferTracker(EcmaVM vm,std::function<bool(std::string dirPath, uint8_t*buff, size_t buffSize)> cb);

该函数用于设置 Ecma 虚拟机的主机解析缓冲区跟踪器回调函数。

回调函数通过 `std::function` 传递，接受目录路径（`dirPath`）和缓冲区指针（`uint8_t** buff`）及其大小指针（`size_t* buffSize`）作为参数，并返回一个布尔值，表示是否成功解析缓冲区。

**参数：**

| 参数名 | 类型                                                         | 必填 | 说明                                |
| ------ | ------------------------------------------------------------ | ---- | ----------------------------------- |
| vm     | EcmaVM *                                                     | 是   | 虚拟机对象。                        |
| cb     | std::function<bool(std::string dirPath, uint8_t **buff, size_t *buffSize)> | 是   | 主机解析缓冲区跟踪器<br/>的回调函数 |

**返回值：**

无

**示例：**

```c++
std::function<bool(std::string dirPath, uint8_t * *buff, size_t * buffSize)> cb = [](const std::string &inputPath,
    uint8_t **buff, size_t *buffSize) -> bool {
    if (inputPath.empty() || buff == nullptr || buffSize == nullptr) {
        return false;
    }
    return false;
};
JSNApi::SetHostResolveBufferTracker(vm, cb);
```

###  SetUnloadNativeModuleCallback

static void SetUnloadNativeModuleCallback(EcmaVM *vm, const std::function<bool(const std::string &moduleKey)> &cb);

自定义卸载本体模块时的行为。

**参数：**

| 参数名 | 类型                                              | 必填 | 说明                                                         |
| ------ | ------------------------------------------------- | ---- | ------------------------------------------------------------ |
| vm     | EcmaVM *                                          | 是   | 虚拟机对象。                                                 |
| cb     | std::function<bool(const std::string &moduleKey)> | 是   | 回调函数接受一个 `moduleKey` 参数（字符串类型）<br/>并返回一个布尔值，表示是否成功卸载本地模块 |

**返回值：**

无

**示例：**

```c++
bool UnloadNativeModuleCallback(const std::string &moduleKey) {
    return true;
}
JSNApi::SetUnloadNativeModuleCallback(vm, UnloadNativeModuleCallback);
```

###  SetNativePtrGetter

static void SetNativePtrGetter(EcmaVM *vm, void *cb);

该函数用于设置 Ecma 虚拟机的本机指针获取器的回调函数。

**参数：**

| 参数名 | 类型     | 必填 | 说明           |
| ------ | -------- | ---- | -------------- |
| vm     | EcmaVM * | 是   | 虚拟机对象。   |
| cb     | Void *   | 是   | 回调函数指针。 |

**返回值：**

无

**示例：**

```c++
void *cb = reinterpret_cast<void *>(NativePtrGetterCallback);
JSNApi::SetNativePtrGetter(vm, cb);
```

###  SetSourceMapTranslateCallback

static void SetSourceMapTranslateCallback(EcmaVM *vm, SourceMapTranslateCallback cb);

该函数用于设置 Ecma 虚拟机的源映射翻译回调函数。

**参数：**

| 参数名   | 类型                       | 必填 | 说明                               |
| -------- | -------------------------- | ---- | ---------------------------------- |
| vm       | EcmaVM *                   | 是   | 虚拟机对象。                       |
| callback | SourceMapTranslateCallback | 是   | 回调函数用于设置源映射翻译的回调。 |

**返回值：**

无

**示例：**

```c++
bool SourceMapTranslateCallback(const std::string& sourceLocation, std::string& translatedLocation) {
    return true; 
}
JSNApi::SetSourceMapTranslateCallback(vm, SourceMapTranslateCallback);
```

###  DestroyMemMapAllocator

static void DestroyMemMapAllocator();

该函数用于销毁内存映射分配器（MemMapAllocator）的实例。

**参数：**

无

**返回值：**

无

**示例：**

```c++
JSNApi::DestroyMemMapAllocator();
```

###  DestroyPGOProfiler

static void DestroyPGOProfiler();

该函数用于销毁性能分析（PGO，Profile-Guided Optimization）的分析器管理器实例。

通过调用 `ecmascript::pgo::PGOProfilerManager::GetInstance()`获取 PGOProfilerManager 的单例实例，并调用 `Destroy` 方法进行销毁。

**参数：**

无

**返回值：**

无

**示例：**

```c++
JSNApi::DestroyPGOProfiler();
```

###  ExecutePendingJob

static void ExecutePendingJob(const EcmaVM *vm);

该函数用于执行当前 Ecma 上下文中挂起的 Promise 任务。

在执行前，通过 `CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN` 宏检查是否存在潜在的异常。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

无

**示例：**

```c++
JSNApi::ExecutePendingJob(vm);
```

###  PreFork

static void PreFork(EcmaVM *vm);

函数用于在执行 fork 操作之前，执行与 Ecma 虚拟机相关的预处理步骤。

**参数：**

| 参数名 | 类型     | 必填 | 说明         |
| ------ | -------- | ---- | ------------ |
| vm     | EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

无

**示例：**

```c++
JSRuntimeOptions option;
EcmaVM *workerVm = JSNApi::CreateEcmaVM(option);
JSNApi::PreFork(workerVm);
```

###  PostFork

static void PostFork(EcmaVM *vm, const RuntimeOption &option);

该函数用于在执行 fork 操作之后，执行与 Ecma 虚拟机相关的后处理步骤。

**参数：**

| 参数名 | 类型                  | 必填 | 说明                 |
| ------ | --------------------- | ---- | -------------------- |
| vm     | EcmaVM *              | 是   | 虚拟机对象。         |
| option | const RuntimeOption & | 是   | 运行时选项和配置的类 |

**返回值：**

无

**示例：**

```c++
RuntimeOption option;
JSNApi::PostFork(vm, option);
```

###  ExecuteModuleBuffer

static bool ExecuteModuleBuffer(EcmaVM *vm, const uint8_t *data, int32_t size, const std::string &filename = "",bool needUpdate = false);

该函数用于执行传入的模块数据。

在执行前，函数会检查是否存在潜在的异常（`CHECK_HAS_PENDING_EXCEPTION` 宏）。

如果模块执行失败，会输出错误日志并返回 `false`，否则返回 `true`。

**参数：**

| 参数名     | 类型               | 必填 | 说明                   |
| ---------- | ------------------ | ---- | ---------------------- |
| vm         | EcmaVM *           | 是   | 虚拟机对象。           |
| data       | const uint8_t*     | 是   | 表示模块数据的指针。   |
| size       | int32_t            | 是   | 表示模块数据的大小。   |
| filename   | const std::string& | 否   | 表示模块的文件名。     |
| needUpdate | bool               | 否   | 表示是否需要更新模块。 |

**返回值：**

| 类型 | 说明                   |
| ---- | ---------------------- |
| bool | 表示模块执行是否成功。 |

**示例：**

```c++
const char *fileName = "__JSNApiTests_ExecuteModuleBuffer.abc";
const char *data = R"(
    .language ECMAScript
    .function any func_main_0(any a0, any a1, any a2) {
        ldai 1
        return
    }
)";
bool res =
    JSNApi::ExecuteModuleBuffer(vm, reinterpret_cast<const uint8_t *>(data), sizeof(data), fileName);
```

###  TriggerGC

static void TriggerGC(const EcmaVM *vm, TRIGGER_GC_TYPE gcType = TRIGGER_GC_TYPE::SEMI_GC);

该函数用于触发垃圾回收（GC）操作，可以选择不同的垃圾回收类型。

通过调用虚拟机的 CollectGarbage 方法执行垃圾回收操作。

在触发垃圾回收之前，通过 CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN 宏检查是否存在潜在的异常。

**参数：**

| 参数名 | 类型            | 必填 | 说明                                           |
| ------ | --------------- | ---- | ---------------------------------------------- |
| vm     | const EcmaVM *  | 是   | 虚拟机对象。                                   |
| gcType | TRIGGER_GC_TYPE | 否   | 是一个枚举类型，表示触发GC（垃圾回收）的类型。 |

**返回值：**

无

**示例：**

```c++
JSNApi::TriggerGC(vm);
```

###  ThrowException

static void ThrowException(const EcmaVM *vm, Local<JSValueRef> error);

该函数用于在 Ecma 虚拟机中抛出一个异常。

在抛出之前，首先检查是否已经存在挂起的异常，如果存在则记录日志并返回，以保留先前的异常信息。

通过 SetException 方法将传入的异常对象设置为当前线程的异常。

**参数：**

| 参数名 | 类型              | 必填 | 说明         |
| ------ | ----------------- | ---- | ------------ |
| vm     | const EcmaVM *    | 是   | 虚拟机对象。 |
| error  | Local<JSValueRef> | 是   | 异常对象     |

**返回值：**

无

**示例：**

```c++
Local<StringRef> message = StringRef::NewFromUtf8(vm, "ErrorTest");
Local<JSValueRef> error = Exception::Error(vm,message);
JSNApi::ThrowException(vm, error);
```

###  PrintExceptionInfo

static void PrintExceptionInfo(const EcmaVM *vm);

该函数用于打印当前潜在的异常信息，获取当前潜在的异常，如果不存在异常则直接返回。

如果异常是 `JSError` 类型，会调用虚拟机的 `PrintJSErrorInfo` 方法打印详细错误信息。

否则，将异常转化为字符串并输出到日志中。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

无

**示例：**

```c++
 JSNApi::PrintExceptionInfo(vm);
```

## JSValueRef

JSValueRef是一个用于表示JS值的类。它提供了一些方式来操作和访问JS中的各种数据类型，如字符串、数字、布尔值、对象、数组等。通过使用JSValueRef，您可以获取和设置JS值的属性和方法，执行函数调用，转换数据类型等。

### Undefined

Local<PrimitiveRef> JSValueRef::Undefined(const EcmaVM *vm)；

用于获取一个表示未定义值的`Value`对象。这个函数通常在处理JavaScript和C++之间的数据转换时使用。

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| :----: | -------------- | ---- | ---------------- |
|   vm   | const EcmaVM * | 是   | 指定虚拟机对象。 |

**返回值：**

| 类型                | 说明             |
| ------------------- | ---------------- |
| Local<PrimitiveRef> | 返回为原生对象。 |

**示例：**

```C++
Local<PrimitiveRef> primitive =JSValueRef::Undefined(vm);
```

### Null

Local<PrimitiveRef> JSValueRef::Null(const EcmaVM *vm)；

用于获取一个表示为Null的`Value`对象。这个函数通常在处理JavaScript和C++之间的数据转换时使用。

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| :----: | -------------- | ---- | ---------------- |
|   vm   | const EcmaVM * | 是   | 指定虚拟机对象。 |

**返回值：**

| 类型                | 说明             |
| ------------------- | ---------------- |
| Local<PrimitiveRef> | 返回为原生对象。 |

**示例：**

```C++
Local<PrimitiveRef> primitive = JSValueRef::Null(vm);
```

### IsGeneratorObject

bool JSValueRef::IsGeneratorObject()；

判断是否为生成器对象。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型 | 说明                                  |
| ---- | ------------------------------------- |
| bool | 是生成器对象返回true。否则返回false。 |

**示例：**

```C++
ObjectFactory *factory = vm->GetFactory();
auto env = vm->GetGlobalEnv();
JSHandle<JSTaggedValue> genFunc = env->GetGeneratorFunctionFunction();
JSHandle<JSGeneratorObject> genObjHandleVal = factory->NewJSGeneratorObject(genFunc);
JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
JSHandle<JSFunction> generatorFunc = JSHandle<JSFunction>::Cast(factory->NewJSObject(hclass));
JSFunction::InitializeJSFunction(vm->GetJSThread(), generatorFunc, FunctionKind::GENERATOR_FUNCTION);
JSHandle<GeneratorContext> generatorContext = factory->NewGeneratorContext();
generatorContext->SetMethod(vm->GetJSThread(), generatorFunc.GetTaggedValue());
JSHandle<JSTaggedValue> generatorContextVal = JSHandle<JSTaggedValue>::Cast(generatorContext);
genObjHandleVal->SetGeneratorContext(vm->GetJSThread(), generatorContextVal.GetTaggedValue());
JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(genObjHandleVal);
Local<GeneratorObjectRef> genObjectRef = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
bool b = genObjectRef->IsGeneratorObject();
```

### IsUint8Array

bool JSValueRef::IsUint8Array();

判断一个JSValueRef对象是否为Uint8Array类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 如果JSValueRef对象为Uint8Array类型，返回true。否则，返回false。 |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
Local<Uint8ArrayRef> object = Uint8ArrayRef::New(vm_, buffer, 4, 5);
bool b = object->IsUint8Array();
```

### IsInt8Array

bool JSValueRef::IsInt8Array();

判断一个JSValueRef对象是否为Int8Array类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 如果JSValueRef对象为Int8Array类型，返回true。否则，返回false。 |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
Local<ObjectRef> object = Int8ArrayRef::New(vm_, buffer, 4, 5);
bool b = object->IsInt8Array();
```

### IsError

bool JSValueRef::IsError();

判断一个JSValueRef对象是否为错误类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                      |
| ---- | --------------------------------------------------------- |
| bool | 如果JSValueRef对象为错误类型，返回true。否则，返回false。 |

**示例：**

```c++
Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
bool b = message->IsError();
```

### InstanceOf

bool JSValueRef::InstanceOf(const EcmaVM *vm, Local<JSValueRef> value);

检查一个特定的JSValueRef是否属于一个特定的EcmaVM对象。

**参数：**

| 参数名 | 类型              | 必填 | 说明          |
| ------ | ----------------- | ---- | ------------- |
| vm     | const EcmaVM *    | 是   | 虚拟机对象。  |
| value  | Local<JSValueRef> | 是   | 指定的value。 |

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 如果特定的JSValueRef属于特定的EcmaVM对象，返回true。否则，返回false。 |

**示例：**

```c++
Local<ObjectRef> origin = ObjectRef::New(vm_);
Local<JSValueRef> ConstructObjectJSValueRef(const EcmaVM *vm){
    // 构造一个JSValueRef类型的对象value
    // ...
    return value; // 返回参数value
}
bool b = origin->InstanceOf(vm_, ConstructObjectJSValueRef(vm_));
```

### IsPromise

bool JSValueRef::IsPromise();

判断一个JSValueRef对象是否为Promise类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 如果JSValueRef对象为PromiseRef类型，返回true。否则，返回false。 |

**示例：**

```c++
Local<JSValueRef> tag = PromiseCapabilityRef::New(vm_)->GetPromise(vm_);
bool b = tag->IsPromise();
```

### IsDate

bool JSValueRef::IsDate();

判断一个JSValueRef对象是否为日期类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                      |
| ---- | --------------------------------------------------------- |
| bool | 如果JSValueRef对象为日期类型，返回true。否则，返回false。 |

**示例：**

```c++
double timeRef = 1.1;
Local<DateRef> dateRef = DateRef::New(vm_, timeRef);
bool b = dateRef->IsDate();
```

### IsTypedArray

bool JSValueRef::IsTypedArray();

检查一个JSValueRef对象是否为类型化数组（TypedArray）。

**参数：**

无

**返回值：**

| 类型 | 说明                                                        |
| ---- | ----------------------------------------------------------- |
| bool | 如果JSValueRef对象为类型化数组，返回true。否则，返回false。 |

**示例：**

```c++
int input = 123;
Local<JSValueRef> res = IntegerRef::New(vm_, input);
bool b = res->IsTypedArray();
```

### IsDataView

bool JSValueRef::IsDataView();

判断一个JSValueRef对象是否为DataView类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 如果JSValueRef对象为DataViewRef类型，返回true。否则，返回false。 |

**示例：**

```c++
Local<JSValueRef> tag = DataViewRef::New(vm_,ArrayBufferRef::New(vm_,0),0,0);
bool b = tag->IsDataView();
```

### IsBuffer

bool JSValueRef::IsBuffer();

判断一个JSValueRef对象是否为Buffer类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                        |
| ---- | ----------------------------------------------------------- |
| bool | 如果JSValueRef对象为Buffer类型，返回true。否则，返回false。 |

**示例：**

```c++
int32_t length = 15;
Local<BufferRef> bufferRef = BufferRef::New(vm_, length);
bool b = bufferRef->IsBuffer();
```

### IsArrayBuffer

bool JSValueRef::IsArrayBuffer();

判断一个JSValueRef对象是否为ArrayBuffer类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 如果JSValueRef对象为ArrayBuffer类型，返回true。否则，返回false。 |

**示例：**

```c++
Local<ArrayBufferRef> ConstructObjectArrBufRef(const EcmaVM *vm, const int32_t length_){
    // 构造一个ArrayBufferRef类型的对象arrayBuffer
    // ...
    return arrayBuffer; // 返回arrayBuffer
}
Local<ArrayBufferRef> arrayBuf = ConstructObjectArrBufRef(vm_, 15);
bool b = arrayBuf->IsArrayBuffer();
```

### IsArray

bool JSValueRef::IsArray(const EcmaVM *vm);

判断一个JSValueRef对象是否为数组类型。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型 | 说明                                                      |
| ---- | --------------------------------------------------------- |
| bool | 如果JSValueRef对象是数组类型，返回true。否则，返回false。 |

**示例：**

```c++
uint32_t length = 3;
Local<ArrayRef> arrayObject = ArrayRef::New(vm_, length);
bool b = arrayObject->IsArray(vm_);
```

### ToObject

Local<ObjectRef> JSValueRef::ToObject(const EcmaVM *vm);

将一个JSValueRef对象转换为ObjectRef对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型             | 说明                                |
| ---------------- | ----------------------------------- |
| Local<ObjectRef> | 返回转换后的 ObjectRef 类型的对象。 |

**示例：**

```c++
int input = 0;
Local<IntegerRef> intValue = IntegerRef::New(vm_, input);
Local<ObjectRef> object = intValue->ToObject(vm_);
```

### ToNumber

Local<NumberRef> JSValueRef::ToNumber(const EcmaVM *vm);

将一个JSValueRef对象转换为NumberRef对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型             | 说明                                |
| ---------------- | ----------------------------------- |
| Local<NumberRef> | 返回转换后的 NumberRef 类型的对象。 |

**示例：**

```c++
Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
Local<JSValueRef> toValue(toString);
Local<NumberRef> number = toString->ToNumber(vm_);
```

### ToBoolean

Local<BooleanRef> JSValueRef::ToBoolean(const EcmaVM *vm);

将一个JSValueRef对象转换为布尔值（BooleanRef）。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型              | 说明                                 |
| ----------------- | ------------------------------------ |
| Local<BooleanRef> | 返回转换后的 BooleanRef 类型的对象。 |

**示例：**

```c++
Local<IntegerRef> intValue = IntegerRef::New(vm_, 0);
Local<BooleanRef> boo = intValue->ToBoolean(vm_);
```

### IsObject

bool JSValueRef::IsObject();

判断一个JSValueRef对象是否为Object类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                        |
| ---- | ----------------------------------------------------------- |
| bool | 如果JSValueRef对象是Object类型，返回true。否则，返回false。 |

**示例：**

```c++
Local<JSValueRef> res = ObjectRef::New(vm_);
bool b = res->IsObject();
```

### IsBigInt

bool JSValueRef::IsBigInt();

判断一个JSValueRef对象是否为BigInt类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                        |
| ---- | ----------------------------------------------------------- |
| bool | 如果JSValueRef对象为BigInt类型，返回true。否则，返回false。 |

**示例：**

```c++
int input = 2147483646;
Local<IntegerRef> intValue = IntegerRef::New(vm_, input);
bool b = intValue->IsBigInt();
```

### IsSymbol

bool JSValueRef::IsSymbol();

判断一个JSValueRef对象是否为Symbol类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                        |
| ---- | ----------------------------------------------------------- |
| bool | 如果JSValueRef对象是Symbol类型，返回true。否则，返回false。 |

**示例：**

```c++
int input = 123;
Local<JSValueRef> res = IntegerRef::New(vm_,input);
bool b = res->IsSymbol();
```

### IsUndefined

bool JSValueRef::IsUndefined();

判断一个JSValueRef对象是否为未定义类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                        |
| ---- | ----------------------------------------------------------- |
| bool | 如果JSValueRef对象为未定义类型，返回true。否则，返回false。 |

**示例：**

```c++
Local<JSValueRef> tag = JSValueRef::Undefined(vm_);
bool b = tag->IsUndefined();
```



### IsBoolean

bool JSValueRef::IsBoolean();

判断一个JSValueRef对象是否为布尔值。

**参数：**

无

**返回值：**

| 类型 | 说明                                                    |
| ---- | ------------------------------------------------------- |
| bool | 如果JSValueRef对象为布尔值，返回true。否则，返回false。 |

**示例：**

```c++
Local<JSValueRef> tag = BooleanRef::New(vm_,false);
bool b = tag->IsBoolean();
```

### IsNull

bool JSValueRef::IsNull();

判断一个JSValueRef对象是否为空。

**参数：**

无

**返回值：**

| 类型 | 说明                                                |
| ---- | --------------------------------------------------- |
| bool | 如果JSValueRef对象为空，返回true。否则，返回false。 |

**示例：**

```c++
Local<JSValueRef> tag = JSValueRef::Null(vm_);
bool b = tag->IsNull();
```



### IsNativePointer

bool JSValueRef::IsNativePointer();

判断一个JSValueRef对象是否为本地指针。

**参数：**

无

**返回值：**

| 类型 | 说明                                                      |
| ---- | --------------------------------------------------------- |
| bool | 如果JSValueRef对象为本地指针，返回true。否则，返回false。 |

**示例：**

```c++
void *vps = static_cast<void *>(new std::string("test"));
void *vps1 = static_cast<void *>(new std::string("test"));
Local<NativePointerRef> res_vps = 
    NativePointerRef::New(vm_, vps, NativeAreaAllocator::FreeBufferFunc, vps1, 0);
bool b = res_vps->IsNativePointer();
```

### IsFunction

bool JSValueRef::IsFunction();

判断一个JSValueRef对象是否为函数。

**参数：**

无

**返回值：**

| 类型 | 说明                                                  |
| ---- | ----------------------------------------------------- |
| bool | 如果JSValueRef对象为函数，返回true。否则，返回false。 |

**示例：**

```c++
Local<FunctionRef> ConstructObjectFunctionRef(EcmaVM *vm, size_t nativeBindingsize){
    // 构造一个FunctionRef类型对象obj
    // ...
    return obj; // 返回obj
}
Local<FunctionRef> object = ConstructObjectFunctionRef(vm_, 15);
bool b = object->IsFunction();
```

### IsString

bool JSValueRef::IsString();

判断一个JSValueRef对象是否为字符串。

**参数：**

无

**返回值：**

| 类型 | 说明                                                    |
| ---- | ------------------------------------------------------- |
| bool | 如果JSValueRef对象为字符串，返回true。否则，返回false。 |

**示例：**

```c++
Local<JSValueRef> tag = StringRef::NewFromUtf8(vm_,"abc");
bool b = tag->IsString();
```

### IsNumber

bool JSValueRef::IsNumber();

判断一个JSValueRef对象是否为数字。

**参数：**

无

**返回值：**

| 类型 | 说明                                                  |
| ---- | ----------------------------------------------------- |
| bool | 如果JSValueRef对象为数字，返回true。否则，返回false。 |

**示例：**

```c++
Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
bool b = resUnit32->IsNumber();
```

### True

static Local<PrimitiveRef> True(const EcmaVM *vm);

检查一个值是否为True对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型                | 说明                                                         |
| :------------------ | :----------------------------------------------------------- |
| Local<PrimitiveRef> | 创建一个表示布尔值true的JSValueRef对象，转换为PrimitiveRef类型的本地引用，并返回。 |

**示例：**

```c++
Local<BooleanRef> boolValue = BooleanRef::New(vm_, true);
Local<PrimitiveRef> result = boolValue->True(vm_);
```

### False

static Local<PrimitiveRef> False(const EcmaVM *vm);

检查一个值是否为False对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型                | 说明                                                         |
| :------------------ | :----------------------------------------------------------- |
| Local<PrimitiveRef> | 创建一个表示布尔值false的JSValueRef对象，转换为PrimitiveRef类型的本地引用，并返回。 |

**示例：**

```c++
Local<BooleanRef> boolValue = BooleanRef::New(vm_, false);
Local<PrimitiveRef> result = boolValue->True(vm_);
```

### IsFalse

bool IsFalse();

用于判断该对象是否为False对象。

**参数：**

无

**返回值：**

| 类型    | 说明                                           |
| :------ | :--------------------------------------------- |
| boolean | 如果该对象为false，则返回true，否则返回false。 |

**示例：**

```c++
Local<BooleanRef> boolValue = BooleanRef::New(vm_, false);
bool result = boolValue->IsFalse();
```

### IsJSArray

bool IsJSArray(const EcmaVM *vm);

用于判断是否为JS的数组类型。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型    | 说明                                                  |
| :------ | :---------------------------------------------------- |
| boolean | 如果该对象是JS的数组类型，则返回true，否则返回false。 |

**示例：**

```c++
JSArray *arr = JSArray::ArrayCreate(thread_, JSTaggedNumber(number)).GetObject<JSArray>();
JSHandle<JSTaggedValue> obj(thread_, arr);
Local<JSValueRef> JSArrayObject = JSNApiHelper::ToLocal<JSValueRef>(obj);
bool result = JSArrayObject->IsJSArray(vm_);
```

### IsConstructor

bool IsConstructor();

用于判断是否为构造函数类型。

**参数：**

无

**返回值：**

| 类型    | 说明                                                  |
| :------ | :---------------------------------------------------- |
| boolean | 如果该对象是构造函数类型，则返回true，否则返回false。 |

**示例：**

```c++
ObjectFactory *factory = vm_->GetFactory();
JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
JSHandle<JSFunction> func = factory->NewJSFunction(env, static_cast<void *>(nullptr), FunctionKind::BASE_CONSTRUCTOR);
JSHandle<JSObject> nullHandle(thread_, JSTaggedValue::Null());
JSHandle<JSObject> obj = JSObject::ObjectCreate(thread_, nullHandle);
JSHandle<JSTaggedValue> objValue(obj);
[[maybe_unused]] bool makeConstructor = JSFunction::MakeConstructor(thread_, func, objValue);
JSHandle<JSTaggedValue> funcHandle(func);
Local<JSValueRef> funConstructor = JSNApiHelper::ToLocal<JSValueRef>(funcHandle);
bool result = funConstructor->IsConstructor();
```

### IsWeakRef

bool IsWeakRef();

用于判断是否为WeakRef对象。

**参数：**

无

**返回值：**

| 类型    | 说明                                             |
| :------ | :----------------------------------------------- |
| boolean | 如果该对象是WeakRef，则返回true，否则返回false。 |

**示例：**

```c++
JSFunction *jsFunc = vm_->GetGlobalEnv()->GetObjectFunction().GetObject<JSFunction>();
JSHandle<JSTaggedValue> jsFuncTagValue(thread, jsFunc);
JSHandle<JSObject> newObj = vm_->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFuncTagValue);
JSTaggedValue value(newObj);
value.CreateWeakRef();
bool result = JSNApiHelper::ToLocal<JSValueRef>(value);->IsWeakRef();
```

### IsArrayIterator

bool IsArrayIterator();

用于判断该对象是否为数组迭代器。

**参数：**

无

**返回值：**

| 类型    | 说明                                                |
| :------ | :-------------------------------------------------- |
| boolean | 如果该对象是数组迭代器，则返回true，否则返回false。 |

**示例：**

```c++
ObjectFactory *factory = vm_->GetFactory();
JSHandle<TaggedArray> handleTaggedArrayFrom = vm_->GetFactory()->NewTaggedArray(arrayLength);
JSHandle<JSObject> handleJSObjectTaggedArrayFrom = JSArray::CreateArrayFromList(thread, handleTaggedArrayFrom); 
JSHandle<JSArrayIterator> handleJSArrayIter = 
    factory->NewJSArrayIterator(handleJSObjectTaggedArrayFrom,IterationKind::KEY);
JSHandle<JSTaggedValue> jsTagValue = JSHandle<JSTaggedValue>::Cast(handleJSArrayIter);
bool result = JSNApiHelper::ToLocal<JSValueRef>(jsTagValue)->IsStringIterator();
```

### IsStringIterator

bool IsStringIterator();

用于判断该对象是否为字符串迭代器。

**参数：**

无

**返回值：**

| 类型    | 说明                                                    |
| :------ | :------------------------------------------------------ |
| boolean | 如果该对象是字符串的迭代器，则返回true，否则返回false。 |

**示例：**

```c++
JSHandle<EcmaString> recordName = vm_->GetFactory()->NewFromUtf8("646458");
JSHandle<JSStringIterator> jsStringIter = JSStringIterator::CreateStringIterator(vm_->GetJSThread(), recordName);
JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(jsStringIter);
bool result = JSNApiHelper::ToLocal<StringRef>(setTag)->IsStringIterator();
```

### IsJSPrimitiveInt

bool IsJSPrimitiveInt();

用于判断是否为JS的原始整数类型。

**参数：**

无

**返回值：**

| 类型    | 说明                                                      |
| :------ | :-------------------------------------------------------- |
| boolean | 如果该对象是JS的原始整数类型，则返回true，否则返回false。 |

**示例：**

```c++
JSHandle<JSTaggedValue> jsTagValue;
JSHandle<JSPrimitiveRef> jsPrimitive = 
    vm_->GetFactory()->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BIGINT, jsTagValue);
JSHandle<JSTaggedValue> jsPriTagValue = JSHandle<JSTaggedValue>::Cast(jsPrimitive);
bool result = JSNApiHelper::ToLocal<JSValueRef>(jsPriTagValue)->IsJSPrimitiveInt();
```

### IsTreeMap

bool IsTreeMap();

用于判断该对象是否为TreeMap类型。

**参数：**

无

**返回值：**

| 类型    | 说明                                                 |
| :------ | :--------------------------------------------------- |
| boolean | 如果该对象是TreeMap类型，则返回true，否则返回false。 |

**示例：**

```c++
JSHandle<JSTaggedValue> proto = vm_->GetGlobalEnv()->GetObjectFunctionPrototype();
JSHandle<JSHClass> mapClass = vm_->GetFactory()->NewEcmaHClass(JSAPITreeMap::SIZE, JSType::JS_API_TREE_MAP, proto);
JSHandle<JSAPITreeMap> jsTreeMap = JSHandle<JSAPITreeMap>::Cast(factory->NewJSObjectWithInit(mapClass));
JSHandle<TaggedTreeMap> treeMap(thread, TaggedTreeMap::Create(thread));
jsTreeMap->SetTreeMap(thread, treeMap);
JSHandle<JSTaggedValue> treeMapTagValue = JSHandle<JSTaggedValue>::Cast(jsTreeMap);
bool result = JSNApiHelper::ToLocal<ArrayRef>(treeMapTagValue)->IsTreeMap();
```

### IsTreeSet

bool IsTreeSet();

用于判断该对象是否为树集。

**参数：**

无

**返回值：**

| 类型    | 说明                                          |
| :------ | :-------------------------------------------- |
| boolean | 如果该对象是树集，则返回true，否则返回false。 |

**示例：**

```c++
JSHandle<JSTaggedValue> proto = vm_->GetGlobalEnv()->GetObjectFunctionPrototype();
JSHandle<JSHClass> setClass = vm_->GetFactory()->NewEcmaHClass(JSAPITreeSet::SIZE, JSType::JS_API_TREE_SET, proto);
JSHandle<JSAPITreeSet> jsTreeSet = JSHandle<JSAPITreeSet>::Cast(factory->NewJSObjectWithInit(setClass));
JSHandle<TaggedTreeSet> treeSet(thread, TaggedTreeSet::Create(thread));
jsTreeSet->SetTreeSet(thread, treeSet);
JSHandle<JSTaggedValue> treeSetTagValue = JSHandle<JSTaggedValue>::Cast(jsTreeSet);
bool result = JSNApiHelper::ToLocal<ArrayRef>(treeSetTagValue)->IsTreeSet();
```

### IsVector

bool IsVector();

用于判断该对象是否为Vector容器。

**参数：**

无

**返回值：**

| 类型    | 说明                                                |
| :------ | :-------------------------------------------------- |
| boolean | 如果该对象是Vector容器，则返回true，否则返回false。 |

**示例：**

```c++
JSHandle<JSTaggedValue> proto = vm_->GetGlobalEnv()->GetFunctionPrototype();
JSHandle<JSHClass> vectorClass = vm_->GetFactory()->NewEcmaHClass(JSAPIVector::SIZE, JSType::JS_API_VECTOR, proto);
JSHandle<JSAPIVector> jsVector = JSHandle<JSAPIVector>::Cast(factory->NewJSObjectWithInit(vectorClass));
jsVector->SetLength(length);
JSHandle<JSTaggedValue> vectorTagValue = JSHandle<JSTaggedValue>::Cast(jsVector);
bool result = JSNApiHelper::ToLocal<ArrayRef>(vectorTagValue)->IsVector();
```

### IsArgumentsObject

bool IsArgumentsObject();

用于判断该对象是否为参数对象。

**参数：**

无

**返回值：**

| 类型    | 说明                                              |
| :------ | :------------------------------------------------ |
| boolean | 如果该对象是参数对象，则返回true，否则返回false。 |

**示例：**

```c++
JSHandle<JSArguments> obj = vm_->GetFactory()->NewJSArguments();
JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(obj);
bool result = JSNApiHelper::ToLocal<ObjectRef>(argumentTag)->IsArgumentsObject();
```

### IsAsyncFunction

bool IsAsyncFunction();

用于判断是否为异步函数。

**参数：**

无

**返回值：**

| 类型    | 说明                                        |
| :------ | :------------------------------------------ |
| boolean | 如果是异步函数，则返回true，否则返回false。 |

**示例：**

```c++
JSHandle<JSAsyncFuncObject> asyncFuncObj = vm_->GetFactory()->NewJSAsyncFuncObject();
JSHandle<JSTaggedValue> asyncFuncTag = JSHandle<JSTaggedValue>::Cast(asyncFuncObj);
bool result = JSNApiHelper::ToLocal<ObjectRef>(asyncFuncTag)->IsAsyncFunction();
```

### IsGeneratorFunction

bool IsGeneratorFunction();

用于判断是否为生成器函数。

**参数：**

无

**返回值：**

| 类型    | 说明                                                |
| :------ | :-------------------------------------------------- |
| boolean | 如果该对象是生成器函数，则返回true，否则返回false。 |

**示例：**

```c++
JSHandle<JSTaggedValue> genFunc = vm_->GetGlobalEnv()->GetGeneratorFunctionFunction();
JSHandle<JSGeneratorObject> genObjHandleVal = vm_->GetFactory()->NewJSGeneratorObject(genFunc);
JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(genObjHandleVal);
Local<GeneratorObjectRef> genObjectRef = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
Local<JSValueRef> object = genObjectRef->GetGeneratorFunction(vm_);
bool result = res->IsGeneratorFunction();
```

### IsBigInt64Array

bool IsBigInt64Array();

用于判断该对象是否为用于存储任意长度的有符号64位整数数组。

**参数：**

无

**返回值：**

| 类型    | 说明                                                         |
| :------ | :----------------------------------------------------------- |
| boolean | 如果该对象是任意长度的有符号64位整数数组，则返回true，否则返回false。 |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, bufferLength);
Local<ObjectRef> object = BigInt64ArrayRef::New(vm_, buffer, bufferOffset, offsetLength);
bool result = object->IsBigInt64Array();
```

### IsBigUint64Array

bool IsBigUint64Array();

用于判断该对象是否为用于存储任意长度的无符号64位整数数组。

**参数：**

无

**返回值：**

| 类型    | 说明                                                         |
| :------ | :----------------------------------------------------------- |
| boolean | 如果该对象是任意长度的无符号64位整数数组，则返回true，否则返回false。 |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
Local<ObjectRef> object = BigUint64ArrayRef::New(vm_, buffer, bufferOffset, offsetLength);
bool result = object->IsBigUint64Array();
```

### IsSharedArrayBuffer

bool IsSharedArrayBuffer();

用于判断该对象是否为共享的ArrayBuffer对象。

**参数：**

无

**返回值：**

| 类型    | 说明                                                       |
| :------ | :--------------------------------------------------------- |
| boolean | 如果该对象是共享的ArrayBuffer，则返回true，否则返回false。 |

**示例：**

```c++
JSHandle<JSArrayBuffer> jsArrayBuffer = vm_->GetFactory()->NewJSSharedArrayBuffer(bufferLength);
JSHandle<JSTaggedValue> jsTagValueBuffer = JSHandle<JSTaggedValue>::Cast(jsArrayBuffer);
bool result = JSNApiHelper::ToLocal<ArrayRef>(jsTagValueBuffer)->IsSharedArrayBuffer();
```

### IsUint8ClampedArray

bool JSValueRef::IsUint8ClampedArray();

判断对象是否为Uint8ClampedArray类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                       |
| ---- | ---------------------------------------------------------- |
| bool | 如果调用对象的类型为Uint8ClampedArray返回True否则返回False |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm, arrayLenth);
Local<JSValueRef> val = Uint8ClampedArrayRef::New(vm, buffer, Offset, OffsetLenth);
bool res = val->IsUint8ClampedArray();
```

### IsInt16Array

bool JSValueRef::IsInt16Array();

判断对象是否为Int16Array类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                |
| ---- | --------------------------------------------------- |
| bool | 如果调用对象的类型为Int16Array返回True否则返回False |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm, arrayLenth);
Local<JSValueRef> val = Int16ArrayRef::New(vm, buffer, Offset, OffsetLenth);
bool res = val->IsInt16Array();
```

### IsUint16Array

bool JSValueRef::IsUint16Array();

判断对象是否为Uint16Array类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                 |
| ---- | ---------------------------------------------------- |
| bool | 如果调用对象的类型为Uint16Array返回True否则返回False |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm, arrayLenth);
Local<JSValueRef> val = Uint16ArrayRef::New(vm, buffer, Offset, OffsetLenth);
bool res = val->IsUint16Array();
```

### IsInt32Array

bool JSValueRef::IsInt32Array();

判断对象是否为Int32Array类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                |
| ---- | --------------------------------------------------- |
| bool | 如果调用对象的类型为Int32Array返回True否则返回False |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm, 30);
Local<JSValueRef> val = Int32ArrayRef::New(vm, buffer, 4, 6);
bool res = val->IsInt32Array();
```

### IsUint32Array

bool JSValueRef::IsUint32Array();

判断对象是否为Uint32Array类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                 |
| ---- | ---------------------------------------------------- |
| bool | 如果调用对象的类型为Uint32Array返回True否则返回False |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm, 30);
Local<JSValueRef> val = Uint32ArrayRef::New(vm, buffer, 4, 6);
bool res = val->IsUint32Array();
```

### IsFloat32Array

bool JSValueRef::IsFloat32Array();

判断对象是否为Float32Array类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                  |
| ---- | ----------------------------------------------------- |
| bool | 如果调用对象的类型为Float32Array返回True否则返回False |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm, 30);
Local<JSValueRef> val = Float32ArrayRef::New(vm, buffer, 4, 6);
bool res = val->IsFloat32Array();
```

### IsFloat64Array

bool JSValueRef::IsFloat64Array();

判断对象是否为Float64Array类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                  |
| ---- | ----------------------------------------------------- |
| bool | 如果调用对象的类型为Float64Array返回True否则返回False |

**示例：**

```c++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm, arrayLength);
Local<JSValueRef> val = Float64ArrayRef::New(vm, buffer, eOffset, eOffsetLength);
bool res = val->IsFloat64Array();
```

### IsJSPrimitiveBoolean

bool JSValueRef::IsJSPrimitiveBoolean();

判断对象是否为JSPrimitiveBoolean类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                        |
| ---- | ----------------------------------------------------------- |
| bool | 如果调用对象的类型为JSPrimitiveBoolean返回True否则返回False |

**示例：**

```c++
ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> jstagvalue;
    JSHandle<JSPrimitiveRef> jsprimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BOOLEAN, jstagvalue);
    JSHandle<JSTaggedValue> jspri = JSHandle<JSTaggedValue>::Cast(jsprimitive);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jspri);
bool res = object->IsJSPrimitiveBoolean();
```

### IsMapIterator

bool JSValueRef::IsMapIterator();

判断对象是否为MapIterator类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                 |
| ---- | ---------------------------------------------------- |
| bool | 如果调用对象的类型为MapIterator返回True否则返回False |

**示例：**

```c++
JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
JSHandle<JSTaggedValue> builtinsMapFunc = env->GetBuiltinsMapFunction();
JSHandle<JSMap> jsMap(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(builtinsMapFunc), builtinsMapFunc));
JSHandle<JSTaggedValue> linkedHashMap(LinkedHashMap::Create(thread_));
jsMap->SetLinkedMap(thread_, linkedHashMap);
JSHandle<JSTaggedValue> mapIteratorVal =
    JSMapIterator::CreateMapIterator(thread_, JSHandle<JSTaggedValue>::Cast(jsMap), IterationKind::KEY);
Local<MapIteratorRef> object = JSNApiHelper::ToLocal<MapIteratorRef>(mapIteratorVal);
bool res = object->IsMapIterator();
```

### IsSetIterator

bool JSValueRef::IsSetIterator();

判断对象是否为SetIterator类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                 |
| ---- | ---------------------------------------------------- |
| bool | 如果调用对象的类型为SetIterator返回True否则返回False |

**示例：**

```c++
ObjectFactory *factory = vm->GetFactory();
JSHandle<JSTaggedValue> proto = thread_->GetEcmaVM()->GetGlobalEnv()->GetFunctionPrototype();
JSHandle<JSHClass> setClass = factory->NewEcmaHClass(JSSet::SIZE, JSType::JS_SET, proto);
JSHandle<JSSet> jsSet = JSHandle<JSSet>::Cast(factory->NewJSObjectWithInit(setClass));
JSHandle<LinkedHashSet> linkedSet(LinkedHashSet::Create(thread_));
jsSet->SetLinkedSet(thread_, linkedSet);
JSHandle<JSSetIterator> jsSetIter = factory->NewJSSetIterator(jsSet, IterationKind::KEY);
JSHandle<JSTaggedValue> setIter = JSHandle<JSTaggedValue>::Cast(jsSetIter);
bool res = JSNApiHelper::ToLocal<JSValueRef>(setiter)->IsSetIterator();
```

### IsModuleNamespaceObject

bool JSValueRef::IsModuleNamespaceObject();

判断对象是否为ModuleNamespaceObject类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 如果调用对象的类型为ModuleNamespaceObject返回True否则返回False |

**示例：**

```c++
ObjectFactory *factory = vm->GetFactory();
JSHandle<ModuleNamespace> moduleNamespace = factory->NewModuleNamespace();
JSHandle<JSTaggedValue> modname = JSHandle<JSTaggedValue>::Cast(moduleNamespace);
JSNApiHelper::ToLocal<ObjectRef>(modname)->IsModuleNamespaceObject();
bool res =object->IsModuleNamespaceObject()
```

### IsProxy

bool JSValueRef::IsProxy();

判断对象是否为Proxy类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                           |
| ---- | ---------------------------------------------- |
| bool | 如果调用对象的类型为Proxy返回True否则返回False |

**示例：**

```c++
Local<ProxyRef> tag = ProxyRef::New(vm);
bool res = tag->IsProxy();
```

### IsRegExp

bool JSValueRef::IsRegExp();

判断对象是否为RegExp类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                            |
| ---- | ----------------------------------------------- |
| bool | 如果调用对象的类型为RegExp返回True否则返回False |

**示例：**

```c++
Local<RegExp> val = RegExp::New(vm);
bool res = val->IsRegExp();
```

### IsJSPrimitiveNumber

bool JSValueRef::IsJSPrimitiveNumber();

判断对象是否为JSPrimitiveNumber类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                       |
| ---- | ---------------------------------------------------------- |
| bool | 如果调用对象的类型为JSPrimitiveNumber返回True否则返回False |

**示例：**

```c++
ObjectFactory *factory = vm->GetFactory();
JSHandle<JSTaggedValue> jstagvalue;
JSHandle<JSPrimitiveRef> jsprimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_NUMBER, jstagvalue);
JSHandle<JSTaggedValue> jspri = JSHandle<JSTaggedValue>::Cast(jsprimitive);
Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jspri);
bool res = object->IsJSPrimitiveNumber();
```

### IsMap

bool JSValueRef::IsMap();

判断对象是否为Map类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                         |
| ---- | -------------------------------------------- |
| bool | 如果调用对象的类型为Map返回True否则返回False |

**示例：**

```c++
Local<MapRef> mapRef = MapRef::New(vm);
bool res = mapRef->IsMap();
```

### IsSet

bool JSValueRef::IsSet();

判断对象是否为Set类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                         |
| ---- | -------------------------------------------- |
| bool | 如果调用对象的类型为Set返回True否则返回False |

**示例：**

```c++
Local<SetRef> setRef = SetRef::New(vm);
bool res = setRef->IsSet();
```

### IsJSPrimitiveString

bool JSValueRef::IsJSPrimitiveString();

判断对象是否为JSPrimitiveString类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                       |
| ---- | ---------------------------------------------------------- |
| bool | 如果调用对象的类型为JSPrimitiveString返回True否则返回False |

**示例：**

```c++
ObjectFactory *factory = vm->GetFactory();
JSHandle<JSTaggedValue> jstagValue;
JSHandle<JSPrimitiveRef> jsprimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_STRING, jstagvalue);
JSHandle<JSTaggedValue> jspString = JSHandle<JSTaggedValue>::Cast(jsprimitive);
Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jspString);
bool res = object->IsJSPrimitiveNumber();
```

### IsJSPrimitiveSymbol

bool JSValueRef::IsJSPrimitiveSymbol();

判断对象是否为JSPrimitiveSymbol类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                                       |
| ---- | ---------------------------------------------------------- |
| bool | 如果调用对象的类型为JSPrimitiveSymbol返回True否则返回False |

**示例：**

```c++
ObjectFactory *factory = vm->GetFactory();
JSHandle<JSTaggedValue> jstagValue;
JSHandle<JSPrimitiveRef> jsprimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_SYMBOL, jstagvalue);
JSHandle<JSTaggedValue> jspSymbol = JSHandle<JSTaggedValue>::Cast(jsprimitive);
Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jspSymbol);
bool res = object->IsJSPrimitiveNumber();
```

### IsWeakMap

bool JSValueRef::IsWeakMap();

判断对象是否为WeakMap类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                             |
| ---- | ------------------------------------------------ |
| bool | 如果调用对象的类型为WeakMap返回True否则返回False |

**示例：**

```c++
Local<JSValueRef> tag = WeakMapRef::New(vm);
bool res = tag->IsWeakMap();
```

### IsWeakSet

bool JSValueRef::IsWeakSet();

判断对象是否为WeakSet类型。

**参数：**

无

**返回值：**

| 类型 | 说明                                             |
| ---- | ------------------------------------------------ |
| bool | 如果调用对象的类型为WeakSet返回True否则返回False |

**示例：**

```c++
Local<JSValueRef> tag = WeakSetRef::New(vm);
bool res = tag->IsWeakSet();
```



## ObjectRef

继承于JSValueRef，提供了一些方法，用于获取对象对象的一些JSValueRef类型的值。

### New

static Local<ObjectRef> New(const EcmaVM *vm);

用于构造一个ObjectRef类的对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型             | 说明                              |
| :--------------- | :-------------------------------- |
| Local<ObjectRef> | 返回构造成功的ObjectRef类的对象。 |

**示例：**

```c++
 Local<ObjectRef> result = ObjectRef::New(vm_);
```

### GetPrototype

Local<JSValueRef> GetPrototype(const EcmaVM *vm);

验证是否正确返回函数或对象的原型，并验证返回的原型是否为对象类型。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型              | 说明                                           |
| :---------------- | :--------------------------------------------- |
| Local<JSValueRef> | 将获取到的对象原型转换为JSValueRef类型并返回。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
Local<JSValueRef> result = object->GetPrototype(vm_);
```

### GetOwnPropertyNames

Local<ArrayRef> GetOwnPropertyNames(const EcmaVM *vm);

用于获取该对象所有的自有属性名称。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型            | 说明                                   |
| :-------------- | :------------------------------------- |
| Local<ArrayRef> | 返回存储该对象所有自由属性名称的数组。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
Local<JSValueRef> value = ObjectRef::New(vm_);
PropertyAttribute attribute(value, true, true, true);
Local<ArrayRef> result = object->GetOwnPropertyNames(vm_);
```

### Set

bool Set(const EcmaVM *vm, uint32_t key, Local<JSValueRef> value);

用于设置ObjectRef对象的属性值。

**参数：**

| 参数名 | 类型              | 必填 | 说明               |
| ------ | ----------------- | ---- | ------------------ |
| vm     | const EcmaVM *    | 是   | 虚拟机对象         |
| key    | uint32_t          | 是   | 指定的key值        |
| value  | Local<JSValueRef> | 是   | key值对应的value值 |

**返回值：**

| 类型    | 说明                                                       |
| :------ | :--------------------------------------------------------- |
| boolean | ObjectRef对象的属性值设置成功，则返回true，否则返回false。 |

**示例：**

```c++
Local<FunctionRef> object = ObjectRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = ObjectRef::New(vm_);
bool result = object->Set(vm_, key, value);
```

### Has

bool Has(const EcmaVM *vm, Local<JSValueRef> key);

用于检查ObjectRef对象是否具有指定的键。

**参数：**

| 参数名 | 类型              | 必填 | 说明        |
| ------ | ----------------- | ---- | ----------- |
| vm     | const EcmaVM *    | 是   | 虚拟机对象  |
| key    | Local<JSValueRef> | 是   | 指定的key值 |

**返回值：**

| 类型    | 说明                                                         |
| :------ | :----------------------------------------------------------- |
| boolean | 如果该ObjectRef对象具有指定的键，则返回true，否则返回false。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
bool result = object->Has(vm_, key);
```

### Delete

bool Delete(const EcmaVM *vm, Local<JSValueRef> key);

用于根据指定的键值删除ObjectRef对象的属性值。

**参数：**

| 参数名 | 类型              | 必填 | 说明        |
| ------ | ----------------- | ---- | ----------- |
| vm     | const EcmaVM *    | 是   | 虚拟机对象  |
| key    | Local<JSValueRef> | 是   | 指定的key值 |

**返回值：**

| 类型    | 说明                                                         |
| :------ | :----------------------------------------------------------- |
| boolean | ObjectRef对象的属性值成功被删除，则返回true，否则返回false。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_, reinterpret_cast<void *>(detach1), reinterpret_cast<void *>(attach1));
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
bool result = object->Delete(vm_, key);
```

### Get

Local<JSValueRef> Get(const EcmaVM *vm, int32_t key);

根据指定的key值获取对象的value值。

**参数：**

| 参数名 | 类型           | 必填 | 说明        |
| ------ | -------------- | ---- | ----------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象  |
| key    | int32_t        | 是   | 指定的key值 |

**返回值：**

| 类型              | 说明                                   |
| :---------------- | :------------------------------------- |
| Local<JSValueRef> | 返回值为根据指定的key值获取的value值。 |

**示例：**

```c++
Local<FunctionRef> object = ObjectRef::New(vm_);
int32_t key = 123;
Local<JSValueRef> result = object->Get(vm_, key);
```

### Delete

bool Delete(const EcmaVM *vm, uint32_t key);

用于根据指定的键值删除ObjectRef对象的属性值。

**参数：**

| 参数名 | 类型           | 必填 | 说明        |
| ------ | -------------- | ---- | ----------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象  |
| key    | uint32_t       | 是   | 指定的key值 |

**返回值：**

| 类型    | 说明                                                         |
| :------ | :----------------------------------------------------------- |
| boolean | ObjectRef对象的属性值成功被删除，则返回true，否则返回false。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_, reinterpret_cast<void *>(detach1), reinterpret_cast<void *>(attach1));
uint32_t key = 123;
bool result = object->Delete(vm_, key);
```

### DefineProperty

bool DefineProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute attribute);

用于设置Key值及对应的属性值。

**参数：**

| 参数名    | 类型              | 必填 | 说明           |
| --------- | ----------------- | ---- | -------------- |
| vm        | const EcmaVM *    | 是   | 虚拟机对象     |
| key       | Local<JSValueRef> | 是   | 指定的key值    |
| attribute | PropertyAttribute | 是   | 要设置的属性值 |

**返回值：**

| 类型    | 说明                                                       |
| :------ | :--------------------------------------------------------- |
| boolean | ObjectRef对象的属性值设置成功，则返回true，否则返回false。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_, reinterpret_cast<void *>(detach1), reinterpret_cast<void *>(attach1));
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = ObjectRef::New(vm_);
PropertyAttribute attribute(value, true, true, true);
bool result = object->DefineProperty(vm_, key, attribute);
```

### GetAllPropertyNames

Local<ArrayRef> GetAllPropertyNames(const EcmaVM *vm, uint32_t filter);

用于获取对象的所有属性名，并返回一个Local<ArrayRef>类型的结果。

**参数：**

| 参数名 | 类型           | 必填 | 说明           |
| ------ | -------------- | ---- | -------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象     |
| filter | uint32_t       | 是   | 指定的过滤条件 |

**返回值：**

| 类型            | 说明                         |
| :-------------- | :--------------------------- |
| Local<ArrayRef> | 返回值为获取到的所有属性名。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
uint32_t filter = 123;
Local<ArrayRef> result = object->GetAllPropertyNames(vm_, filter);
```

### GetOwnEnumerablePropertyNames

Local<ArrayRef> GetOwnEnumerablePropertyNames(const EcmaVM *vm);

获取对象自身的所有可枚举属性名，并返回一个Local<ArrayRef>类型的结果。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型            | 说明                               |
| :-------------- | :--------------------------------- |
| Local<ArrayRef> | 返回值为获取到的所有可枚举属性名。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
Local<ArrayRef> result = object->GetOwnEnumerablePropertyNames(vm_);
```

### Get

Local<JSValueRef> ObjectRef::Get(const EcmaVM *vm, Local<JSValueRef> key);

从一个JavaScript对象中获取一个属性的值。

**参数：**

| 参数名 | 类型              | 必填 | 说明                                                       |
| ------ | ----------------- | ---- | ---------------------------------------------------------- |
| vm     | const EcmaVM *    | 是   | 虚拟机对象。                                               |
| key    | Local<JSValueRef> | 是   | 要获取的属性的键。这个键可以是字符串、数字或其他类型的值。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 将获取到的属性值转换为JSValueRef类型，并通过scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(ret.GetValue()))将其转回局部变量，并返回该值。 |

**示例：**

```c++
Local<FunctionRef> object = ObjectRef::New(vm_);
int32_t key = 123;
Local<JSValueRef> value = object->Get(vm_, key);
```

### GetOwnProperty

获取对象的属性。

bool ObjectRef::GetOwnProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute &property);

**参数：**

| 参数名   | 类型                | 必填 | 说明                         |
| -------- | ------------------- | ---- | ---------------------------- |
| vm       | const EcmaVM *      | 是   | 虚拟机对象。                 |
| key      | Local<JSValueRef>   | 是   | 要获取的属性的键。           |
| property | PropertyAttribute & | 是   | 用于存储获取到的属性的信息。 |

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 如果成功获取到属性描述符，函数会将属性值、getter、setter、可写性、可枚举性和可配置性等信息设置到property对象中。最后，返回`true`表示成功获取属性。否则，返回false。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = ObjectRef::New(vm_);
PropertyAttribute attribute(value, true, true, true);
bool b = object->GetOwnProperty(vm_, key, attribute);
```

### SetAccessorProperty

bool ObjectRef::SetAccessorProperty(const EcmaVM *vm, Local<JSValueRef> key, Local<FunctionRef> getter, Local<FunctionRef> setter, PropertyAttribute attribute);

设置对象的属性。

**参数：**

| 参数名    | 类型               | 必填 | 说明                                             |
| --------- | ------------------ | ---- | ------------------------------------------------ |
| vm        | const EcmaVM *     | 是   | 虚拟机对象。                                     |
| key       | Local<JSValueRef>  | 是   | 要设置的属性的键。                               |
| getter    | Local<FunctionRef> | 是   | 表示属性的getter函数。                           |
| setter    | Local<FunctionRef> | 是   | 表示属性的setter函数。                           |
| attribute | PropertyAttribute  | 是   | 表示属性的特性（如可写性、可枚举性和可配置性）。 |

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 表示是否成功设置了属性。如果设置成功，返回true。否则，返回false。 |

**示例：**

```c++
Local<FunctionRef> object = ObjectRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<FunctionRef> target1 = FunctionRef::New(vm_, nullptr);
Local<FunctionRef> target2 = FunctionRef::New(vm_, nullptr);
bool b = object->SetAccessorProperty(vm_, key, target1, target2);
```

### GetNativePointerField

void *ObjectRef::GetNativePointerField(int32_t index);

获取一个对象的指定索引处的原生指针字段。

**参数：**

| 参数名 | 类型    | 必填 | 说明                                 |
| ------ | ------- | ---- | ------------------------------------ |
| index  | int32_t | 是   | 用于指定要获取的原生指针字段的索引。 |

**返回值：**

| 类型   | 说明                             |
| ------ | -------------------------------- |
| void * | 表示返回一个指向任意类型的指针。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
NativePointerCallback callBack = nullptr;
void *vp1 = static_cast<void *>(new std::string("test"));
void *vp2 = static_cast<void *>(new std::string("test"));
object->SetNativePointerField(33, vp1, callBack, vp2);
void *ptr = object.GetNativePointerField(33);
```

### SetNativePointerFieldCount

void ObjectRef::SetNativePointerFieldCount(int32_t count);

设置一个对象的本地指针字段的数量。

**参数：**

| 参数名 | 类型    | 必填 | 说明                             |
| ------ | ------- | ---- | -------------------------------- |
| count  | int32_t | 是   | 指定要设置的本地指针字段的数量。 |

**返回值：**

无

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
int32_t input = 34;
object->SetNativePointerFieldCount(input);
```

### set

bool Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value);

bool Set(const EcmaVM *vm, uint32_t key, Local<JSValueRef> value);

在当前 `ObjectRef` 对象中设置键值对。

在设置之前，通过 `CHECK_HAS_PENDING_EXCEPTION` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `ObjectRef` 对象转换为 JavaScript 中的 `JSTaggedValue` 对象。

使用 `JSNApiHelper::ToJSHandle(key)` 和 `JSNApiHelper::ToJSHandle(value)` 将键和值转换为 JavaScript 中的 `JSTaggedValue` 对象。

调用 `JSTaggedValue::SetProperty` 方法在对象中设置键值对。

**参数：**

| 参数名  | 类型              | 必填 | 说明             |
| ------- | ----------------- | ---- | ---------------- |
| vm      | const EcmaVM *    | 是   | 虚拟机对象。     |
| key   | Local<JSValueRef> | 是   | 表示要设置的键。 |
| key   | uint32_t        | 是   | 表示要设置的键。 |
| value | Local<JSValueRef> | 是   | 表示要设置的值。 |

**返回值：**

| 类型 | 说明                        |
| ---- | --------------------------- |
| bool | 设置成功返回True反之False。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
Local<JSValueRef> toValue(toString);
bool res = object->Set(vm_, key, toValue);
bool res = object->Set(vm_, toValue, toValue);
```

###  Has

bool Has(const EcmaVM *vm, uint32_t key);

 检查当前 `ObjectRef` 对象是否包含指定键的属性。

 在检查之前，通过 `CHECK_HAS_PENDING_EXCEPTION` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `ObjectRef` 对象转换为 JavaScript 中的 `JSTaggedValue` 对象。

调用 `JSTaggedValue::HasProperty` 方法检查对象是否包含指定键的属性。

**参数：**

| 参数名 | 类型           | 必填 | 说明                 |
| ------ | -------------- | ---- | -------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象。         |
| key    | uint32_t       | 是   | 表示要检查的属性键。 |

**返回值：**

| 类型 | 说明                      |
| ---- | ------------------------- |
| bool | 如果有返回True反之False。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
bool res = object->Has(vm_, key);
```

###  SetNativePointerField

void SetNativePointerField(int32_t index,void *nativePointer = nullptr,NativePointerCallback callBack = nullptr,void *data = nullptr, size_t nativeBindingsize = 0);

在当前 `ObjectRef` 对象中设置指定索引的本机指针字段。

在设置之前，通过 `LOG_IF_SPECIAL` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `ObjectRef` 对象转换为 JavaScript 中的 `JSObject` 对象。

调用 `JSObject::SetNativePointerField` 方法设置本机指针字段。

**参数：**

| 参数名            | 类型                  | 必填 | 说明                             |
| ----------------- | --------------------- | ---- | -------------------------------- |
| index             | int32_t               | 是   | 表示要设置的本机指针字段的索引。 |
| nativePointer     | void*                 | 否   | 表示要设置的本机指针。           |
| callBack          | NativePointerCallback | 否   | 表示本机指针的回调函数。         |
| data              | void*                 | 否   | 表示与本机指针相关联的数据。     |
| nativeBindingsize | size_t                | 是   | 表示本机绑定的大小。             |

**返回值：**

无

**示例：**

```c++
NativePointerCallback callBack = nullptr;
void *vp1 = static_cast<void*>(new std::string("test"));
void *vp2 = static_cast<void*>(new std::string("test"));
object->SetNativePointerField(index, vp1, callBack, vp2);
```

###  Freeze

static void TriggerGC(const EcmaVM *vm, TRIGGER_GC_TYPE gcType = TRIGGER_GC_TYPE::SEMI_GC);

该函数用于触发垃圾回收（GC）操作，可以选择不同的垃圾回收类型。

通过调用虚拟机的 CollectGarbage 方法执行垃圾回收操作。

在触发垃圾回收之前，通过 CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN 宏检查是否存在潜在的异常。

**参数：**

| 参数名   | 类型            | 必填 | 说明                                           |
| -------- | --------------- | ---- | ---------------------------------------------- |
| vm       | const EcmaVM *  | 是   | 虚拟机对象。                                   |
| gcType | TRIGGER_GC_TYPE | 否   | 是一个枚举类型，表示触发GC（垃圾回收）的类型。 |

**返回值：**

无

**示例：**

```c++
JSNApi::TriggerGC(vm);
```

###  Seal

static void TriggerGC(const EcmaVM *vm, TRIGGER_GC_TYPE gcType = TRIGGER_GC_TYPE::SEMI_GC);

该函数用于触发垃圾回收（GC）操作，可以选择不同的垃圾回收类型。

通过调用虚拟机的 CollectGarbage 方法执行垃圾回收操作。

在触发垃圾回收之前，通过 CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN 宏检查是否存在潜在的异常。

**参数：**

| 参数名   | 类型            | 必填 | 说明                                           |
| -------- | --------------- | ---- | ---------------------------------------------- |
| vm       | const EcmaVM *  | 是   | 虚拟机对象。                                   |
| gcType | TRIGGER_GC_TYPE | 否   | 是一个枚举类型，表示触发GC（垃圾回收）的类型。 |

**返回值：**

无

**示例：**

```c++
JSNApi::TriggerGC(vm);
```

###  New

Local<ObjectRef> ObjectRef::New(const EcmaVM *vm)；

创建一个新的 JavaScript 对象，并返回对该对象的本地引用。

在创建之前，通过 `CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED` 宏检查是否存在潜在的异常。

获取虚拟机的工厂对象和全局环境。

使用全局环境中的 `GetObjectFunction` 函数获取对象构造函数。

调用工厂的 `NewJSObjectByConstructor` 方法创建新的 JavaScript 对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型             | 说明                       |
| ---------------- | -------------------------- |
| Local<ObjectRef> | 返回新的 JavaScript 对象。 |

**示例：**

```c++
Local<ObjectRef> myObject = ObjectRef::New(vm);
```

###   SetPrototype

bool ObjectRef::SetPrototype(const EcmaVM *vm, Local<ObjectRef> prototype)

设置当前 `ObjectRef` 对象的原型为指定的 `prototype` 对象。

在设置之前，通过 `LOG_IF_SPECIAL` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `ObjectRef` 对象转换为 JavaScript 中的 `JSObject` 对象。

使用 `JSNApiHelper::ToJSHandle(prototype)` 将传入的 `prototype` 对象转换为 JavaScript 中的 `JSObject` 对象。

调用 `JSTaggedValue::SetPrototype` 方法设置对象的原型。

**参数：**

| 参数名    | 类型             | 必填 | 说明                   |
| --------- | ---------------- | ---- | ---------------------- |
| vm        | const EcmaVM *   | 是   | 虚拟机对象。           |
| prototype | Local<ObjectRef> | 是   | 表示要设置的原型对象。 |

**返回值：**

| 类型 | 说明                        |
| ---- | --------------------------- |
| bool | 设置成功返回True反之False。 |

**示例：**

```c++
Local<ObjectRef> object = ObjectRef::New(vm_);
Local<ObjectRef> prototype = object->GetPrototype(vm_);
object->SetPrototype(vm_, prototype);
```

 static Local<PromiseCapabilityRef> New(const EcmaVM *vm);



## FunctionRef

提供方法将函数封装为一个对象，以及对封装函数的调用。

### New

Local<FunctionRef> FunctionRef::New(EcmaVM *vm, FunctionCallback nativeFunc, NativePointerCallback deleter, void *data, bool callNapi, size_t nativeBindingsize)；

创建一个新的函数对象。

**参数：**

|      参数名       | 类型             | 必填 | 说明                                                         |
| :---------------: | ---------------- | ---- | ------------------------------------------------------------ |
|        vm         | const EcmaVM *   | 是   | 指定虚拟机对象。                                             |
|    nativeFunc     | FunctionCallback | 是   | 一个回调函数，当JS调用这个本地函数时，将调用这个回调函。     |
|      deleter      | NativePointerCallback          | 否   | 一个删除器函数，用于在不再需要`FunctionRef`对象时释放其资源。 |
|       data        | void *           | 否   | 一个可选的指针，可以传递给回调函数或删除器函数。             |
|     callNapi      | bool             | 否   | 一个布尔值，表示是否在创建`FunctionRef`对象时立即调用回调函数。如果为`true`，则在创建对象时立即调用回调函数；如果为`false`，则需要手动调用回调函数。 |
| nativeBindingsize | size_t           | 否   | 表示nativeFunc函数的大小，0表示未知大小。                    |

**返回值：**

| 类型               | 说明                     |
| ------------------ | ------------------------ |
| Local<FunctionRef> | 返回为一个新的函数对象。 |

**示例：**

```C++
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}
Local<FunctionRef> callback = FunctionRef::New(vm, FunCallback);
```

### NewClassFunction

Local<FunctionRef> FunctionRef::NewClassFunction(EcmaVM *vm, FunctionCallback nativeFunc, NativePointerCallback deleter, void *data, bool callNapi, size_t nativeBindingsize)；

创建一个新的类函数对象。

**参数：**

|      参数名       | 类型             | 必填 | 说明                                                         |
| :---------------: | ---------------- | ---- | ------------------------------------------------------------ |
|        vm         | const EcmaVM *   | 是   | 指定虚拟机对象。                                             |
|    nativeFunc     | FunctionCallback | 是   | 一个回调函数，当JS调用这个本地函数时，将调用这个回调函。     |
|      deleter      | NativePointerCallback          | 否   | 一个删除器函数，用于在不再需要`FunctionRef`对象时释放其资源。 |
|       data        | void *           | 否   | 一个可选的指针，可以传递给回调函数或删除器函数。             |
|     callNapi      | bool             | 否   | 一个布尔值，表示是否在创建`FunctionRef`对象时立即调用回调函数。如果为`true`，则在创建对象时立即调用回调函数；如果为`false`，则需要手动调用回调函数。 |
| nativeBindingsize | size_t           | 否   | 表示nativeFunc函数的大小，0表示未知大小。                    |

**返回值：**

| 类型               | 说明                     |
| ------------------ | ------------------------ |
| Local<FunctionRef> | 返回为一个新的函数对象。 |

**示例：**

```C++
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}
NativePointerCallback deleter = nullptr;
void *cb = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
bool callNative = true;
size_t nativeBindingSize = 15;
Local<FunctionRef> obj(FunctionRef::NewClassFunction(vm, FunCallback, deleter, cb, callNative, nativeBindingSize));
```

### Call

Local<JSValueRef> FunctionRef::Call(const EcmaVM *vm, Local<JSValueRef> thisObj, const Local<JSValueRef> argv[], int32_t length)；

设置指定对象调用FunctionRef对象设置的回调函数。

**参数：**

| 参数名  | 类型                    | 必填 | 说明                            |
| :-----: | ----------------------- | ---- | ------------------------------- |
|   vm    | const EcmaVM *          | 是   | 指定虚拟机对象。                |
| thisObj | FunctionCallback        | 是   | 指定调用回调函数的对象。        |
| argv[]  | const Local<JSValueRef> | 否   | Local<JSValueRef>对象数组。     |
| length  | int32_t                 | 否   | Local<JSValueRef>对象数组长度。 |

**返回值：**

| 类型              | 说明                     |
| ----------------- | ------------------------ |
| Local<JSValueRef> | 用于返回函数执行的结果。 |

**示例：**

```C++
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}
Local<IntegerRef> intValue = IntegerRef::New(vm, 0);
std::vector<Local<JSValueRef>> argumentsInt;
argumentsInt.emplace_back(intValue);
Local<FunctionRef> callback = FunctionRef::New(vm, FunCallback);
callback->Call(vm, JSValueRef::Undefined(vm), argumentsInt.data(), argumentsInt.size());
```

### GetSourceCode

Local<StringRef> GetSourceCode(const EcmaVM *vm, int lineNumber)；

获取调用此函数的CPP文件内，指定行号的源代码。

**参数：**

|   参数名   | 类型           | 必填 | 说明             |
| :--------: | -------------- | ---- | ---------------- |
|     vm     | const EcmaVM * | 是   | 指定虚拟机对象。 |
| lineNumber | int            | 是   | 指定行号。       |

**返回值：**

| 类型             | 说明                  |
| ---------------- | --------------------- |
| Local<StringRef> | 返回为StringRef对象。 |

**示例：**

```C++
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}
Local<FunctionRef> callback = FunctionRef::New(vm, FunCallback);
Local<StringRef> src = callback->GetSourceCode(vm, 10);
```

### Constructor

Local<JSValueRef> FunctionRef::Constructor(const EcmaVM *vm, const Local<JSValueRef> argv[], int32_t length)；

用于一个函数对象的构造。

**参数：**

| 参数名 | 类型                    | 必填 | 说明                 |
| :----: | ----------------------- | ---- | -------------------- |
|   vm   | const EcmaVM *          | 是   | 指定虚拟机对象。     |
|  argv  | const Local<JSValueRef> | 是   | 参数数组。           |
| length | int32_t                 | 是   | argv参数的数组大小。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 生成一个FunctionRef，并将其转为Local<JSValueRef>类型，作为函数返回值。 |

**示例：**

```C++
Local<FunctionRef> cls = FunctionRef::NewClassFunction(vm, FunCallback, nullptr, nullptr);
Local<JSValueRef> argv[1];          
int num = 3;                      
argv[0] = NumberRef::New(vm, num);
Local<JSValueRef>functionObj = cls->Constructor(vm, argv, 1); 
```

### GetFunctionPrototype

Local<JSValueRef> FunctionRef::GetFunctionPrototype(const EcmaVM *vm)；

获取prototype对象，它包含了所有函数的原型方法，这些方法可以被所有的函数实例共享和重写。

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| :----: | -------------- | ---- | ---------------- |
|   vm   | const EcmaVM * | 是   | 指定虚拟机对象。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 将prototype对象转为Local<JSValueRef>类型，并作为此函数的返回值。 |

**示例：**

```C++
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}
Local<FunctionRef> res = FunctionRef::New(vm, FunCallback);
Local<JSValueRef> prototype = res->GetFunctionPrototype(vm);
```

## TypedArrayRef

一种用于处理二进制数据的内置对象。它类似于普通数组，但只能存储和操作特定类型的数据。

### ByteLength

uint32_t TypedArrayRef::ByteLength([[maybe_unused]] const EcmaVM *vm)；

此函数返回此ArrayBufferRef缓冲区的长度（以字节为单位）。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| :----: | -------------- | ---- | ------------ |
|   vm   | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型     | 说明                              |
| -------- | --------------------------------- |
| uint32_t | 以uint32_t 类型返回buffer的长度。 |

**示例：**

```c++
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, 10);
Local<DataViewRef> dataView = DataViewRef::New(vm, arrayBuffer, 5, 7);
uint32_t len = dataView->ByteLength();
```

### ByteOffset

uint32_t TypedArrayRef::ByteOffset([[maybe_unused]] const EcmaVM *vm);

获取当前 `TypedArrayRef` 对象在关联的底层数组缓冲区中的字节偏移位置。

在获取字节偏移之前，通过 `CHECK_HAS_PENDING_EXCEPTION` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `TypedArrayRef` 对象转换为 JavaScript 中的 `JSTypedArray` 对象。

调用 `GetByteOffset` 方法获取实际的 TypedArray 在 ArrayBuffer 中的字节偏移位置。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型    | 说明                                                      |
| ------- | --------------------------------------------------------- |
| int32_t | 表示 TypedArray 对象在关联 ArrayBuffer 中的字节偏移位置。 |

**示例：**

```c++
Local<ArrayBufferRef> arrayOffsetBuffer = ArrayBufferRef::New(vm, length);
Local<Int8ArrayRef> objOffset = Int8ArrayRef::New(vm, arrayOffsetBuffer, Offset, length);
uint32_t byteOffset = obj->ByteOffset(vm);
```

### ArrayLength

uint32_t TypedArrayRef::ArrayLength(const EcmaVM *vm);

获取当前 `TypedArrayRef` 对象的数组长度，即其中存储的元素的数量。

在获取数组长度之前，通过 `CHECK_HAS_PENDING_EXCEPTION` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `TypedArrayRef` 对象转换为 JavaScript 中的 `JSTypedArray` 对象。

调用 `GetArrayLength` 方法获取实际的数组长度。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型    | 说明                                         |
| ------- | -------------------------------------------- |
| int32_t | 表示 TypedArray 对象的数组长度（元素数量）。 |

**示例：**

```c++
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, length);
Local<Int8ArrayRef> obj = Int8ArrayRef::New(vm, arrayBuffer, 5, 6);
uint32_t byteOffset = obj->ArrayLength(vm);
```

### GetArrayBuffer

Local<ArrayBufferRef> TypedArrayRef::GetArrayBuffer(const EcmaVM *vm);

获取当前 `TypedArrayRef` 对象关联的 ArrayBufferRef 对象。

在获取 ArrayBufferRef 之前，通过 `CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `TypedArrayRef` 对象转换为 JavaScript 中的 `JSTypedArray` 对象。

调用 `JSTypedArray::GetOffHeapBuffer` 方法获取实际的 ArrayBuffer 对象。

使用 `JSNApiHelper::ToLocal<ArrayBufferRef>(arrayBuffer)` 
将 JavaScript 中的 ArrayBuffer 对象转换为本地的 `ArrayBufferRef` 对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型                  | 说明                          |
| --------------------- | ----------------------------- |
| Local<ArrayBufferRef> | `ArrayBufferRef` 类型的对象。 |

**示例：**

```c++
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, length);
Local<Int8ArrayRef> obj = Int8ArrayRef::New(vm, arrayBuffer, 5, 6);
Local<ArrayBufferRef> byteOffset = obj->GetArrayBuffer(vm);
```



## Exception

提供了一些静态方法，用于根据不同的错误类型创建一个对应的JS异常对象，并返回一个指向该对象的引用。

### AggregateError

static Local<JSValueRef> AggregateError(const EcmaVM *vm, Local<StringRef> message)；

当需要将多个错误包装在一个错误中时，AggregateError对象表示一个错误。

**参数：**

| 参数名  | 类型             | 必填 | 说明         |
| :-----: | ---------------- | ---- | ------------ |
|   vm    | const EcmaVM *   | 是   | 虚拟机对象。 |
| message | Local<StringRef> | 是   | 错误信息。   |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 将多个错误包装为AggregateError对象，并将其转为Local<JSValueRef>类型，作为函数的返回值。 |

**示例：**

```C++
Local<JSValueRef> error = Exception::AggregateError(vm, StringRef::NewFromUtf8(vm, "test aggregate error"));
```

### EvalError

static Local<JSValueRef> EvalError(const EcmaVM *vm, Local<StringRef> message)；

用于表示在执行 `eval()` 函数时发生的错误。当 `eval()` 函数无法解析或执行传入的字符串代码时，会抛出一个 `EvalError` 异常。

**参数：**

| 参数名  | 类型             | 必填 | 说明         |
| :-----: | ---------------- | ---- | ------------ |
|   vm    | const EcmaVM *   | 是   | 虚拟机对象。 |
| message | Local<StringRef> | 是   | 错误信息。   |

**返回值：**

| 类型              | 说明 |
| ----------------- | ---- |
| Local<JSValueRef> |      |

**示例：**

```C++
Local<JSValueRef> error = Exception::EvalError(vm, StringRef::NewFromUtf8(vm, "test eval error"));
```



### OOMError

static Local<JSValueRef> OOMError(const EcmaVM *vm, Local<StringRef> message);

在内存不足的情况下抛出一个错误对象。（由于这是一个静态成员函数，因此可以直接通过类名调用，而无需创建类的实例。）

**参数：**

| 参数名  | 类型             | 必填 | 说明                           |
| ------- | ---------------- | ---- | ------------------------------ |
| vm      | const EcmaVM *   | 是   | 虚拟机对象。                   |
| message | Local<StringRef> | 是   | 传递给OOMError函数的错误消息。 |

**返回值：**

| 类型              | 说明                                                       |
| ----------------- | ---------------------------------------------------------- |
| Local<JSValueRef> | 返回一个 JSValueRef 对象，表示JavaScript中的值或错误信息。 |

**示例：**

```c++
Local<JSValueRef> value = Exception::OOMError(vm_, message);
```

### TypeError

static Local<JSValueRef> TypeError(const EcmaVM *vm, Local<StringRef> message);

在给定的虚拟机（EcmaVM）中创建一个表示类型错误的 JavaScript 值引用（JSValueRef）。

**参数：**

| 参数名  | 类型             | 必填 | 说明                            |
| ------- | ---------------- | ---- | ------------------------------- |
| vm      | const EcmaVM *   | 是   | 虚拟机对象。                    |
| message | Local<StringRef> | 是   | 传递给TypeError函数的错误消息。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 返回一个 JSValueRef 对象，表示JavaScript中的错误信息。可以用于进一步处理或传递该错误对象。 |

**示例：**

```c++
Local<JSValueRef> value = Exception::TypeError(vm_, message);
```

### Error

static Local<JSValueRef> Error(const EcmaVM *vm, Local<StringRef> message);

用于检测error并创建一个新的错误对象。

**参数：**

| 参数名  | 类型             | 必填 | 说明           |
| ------- | ---------------- | ---- | -------------- |
| vm      | const EcmaVM *   | 是   | 虚拟机对象     |
| message | Local<StringRef> | 是   | 传入的错误消息 |

**返回值：**

| 类型              | 说明                 |
| :---------------- | :------------------- |
| Local<JSValueRef> | 返回创建的错误对象。 |

**示例：**

```c++
Local<StringRef> message = StringRef::NewFromUtf8(vm_, "test error");
Local<JSValueRef> result = Exception::Error(vm_, message);
```

### RangeError

static Local<JSValueRef> RangeError(const EcmaVM *vm, Local<StringRef> message);

用于检测范围错误并创建一个新的错误对象。

**参数：**

| 参数名  | 类型             | 必填 | 说明           |
| ------- | ---------------- | ---- | -------------- |
| vm      | const EcmaVM *   | 是   | 虚拟机对象     |
| message | Local<StringRef> | 是   | 传入的错误消息 |

**返回值：**

| 类型              | 说明                 |
| :---------------- | :------------------- |
| Local<JSValueRef> | 返回创建的错误对象。 |

**示例：**

```c++
Local<StringRef> message = StringRef::NewFromUtf8(vm_, "test error");
Local<JSValueRef> result = Exception::RangeError(vm_, message);
```

### ReferenceError

static Local<JSValueRef> ReferenceError(const EcmaVM *vm, Local<StringRef> message);

用于检测引用错误并创建一个新的错误对象。

**参数：**

| 参数名  | 类型             | 必填 | 说明           |
| ------- | ---------------- | ---- | -------------- |
| vm      | const EcmaVM *   | 是   | 虚拟机对象     |
| message | Local<StringRef> | 是   | 传入的错误消息 |

**返回值：**

| 类型              | 说明                 |
| :---------------- | :------------------- |
| Local<JSValueRef> | 返回创建的错误对象。 |

**示例：**

```c++
Local<StringRef> message = StringRef::NewFromUtf8(vm_, "test error");
Local<JSValueRef> result = Exception::ReferenceError(vm_, message);
```

### SyntaxError

static Local<JSValueRef> SyntaxError(const EcmaVM *vm, Local<StringRef> message);

创建一个 SyntaxError 类型的 JavaScript 异常对象。

如果存在挂起的异常，则清除之。

获取 `message` 的 `EcmaString` 对象，用于表示异常错误的消息。

用 `ObjectFactory` 获取工厂实例，创建一个新的 JavaScript SyntaxError 对象。

返回结果作为本地引用。

**参数：**

| 参数名  | 类型             | 必填 | 说明                 |
| ------- | ---------------- | ---- | -------------------- |
| vm      | const EcmaVM *   | 是   | 虚拟机对象。         |
| message | Local<StringRef> | 是   | 表示异常错误的消息。 |

**返回值：**

| 类型                     | 说明                  |
| ------------------------ | --------------------- |
| static Local<JSValueRef> | ArrayRef 类型的对象。 |

**示例：**

```c++
Local<StringRef> errorMessage = StringRef::NewFromUtf8(vm, "Invalid syntax");
Local<JSValueRef> syntaxError = SyntaxError(vm, errorMessage);
```



## MapIteratorRef

用于表示和操作JS Map对象的迭代器引用的类，它继承自ObjectRef类，并提供了一些操作JS Map迭代器方法。

### GetKind

Local<JSValueRef> GetKind(const EcmaVM *vm)；

获取MapIterator迭代元素的类型，分别为key，value，keyAndValue。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| :----: | -------------- | ---- | ------------ |
|   vm   | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 获取迭代器的类型并将其转为Local<JSValueRef>，作为函数的返回值。 |

**示例：**

```c++
JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
ObjectFactory *factory = vm->GetFactory();
JSHandle<JSTaggedValue> builtinsMapFunc = env->GetBuiltinsMapFunction();
JSHandle<JSMap> jsMap(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(builtinsMapFunc), builtinsMapFunc));
JSHandle<JSTaggedValue> linkedHashMap(LinkedHashMap::Create(vm->GetJSThread()));
jsMap->SetLinkedMap(vm->GetJSThread(), linkedHashMap);
JSHandle<JSTaggedValue> mapValue(jsMap);
JSHandle<JSTaggedValue> mapIteratorVal = JSMapIterator::CreateMapIterator(vm->GetJSThread(), mapValue, IterationKind::KEY);
JSHandle<JSMapIterator> mapIterator = JSHandle<JSMapIterator>::Cast(mapIteratorVal);
mapIterator->SetIterationKind(IterationKind::VALUE);
mapIterator->SetIterationKind(IterationKind::KEY_AND_VALUE);
Local<MapIteratorRef> object = JSNApiHelper::ToLocal<MapIteratorRef>(mapIteratorVal);
Local<JSValueRef> type = object->GetKind(vm);
```

### GetIndex

int32_t GetIndex();

用于获取Map迭代器的索引值，并使用断言验证结果是否为0，即初始索引值。

**参数：**

无

**返回值：**

| 类型    | 说明                          |
| :------ | :---------------------------- |
| int32_t | 返回获取的Map迭代器的索引值。 |

**示例：**

```c++
ObjectFactory *factory = vm_->GetFactory();
JSHandle<JSTaggedValue> builtinsMapFunc = vm_->GetGlobalEnv()->GetBuiltinsMapFunction();
JSHandle<JSMap> jsMap(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(builtinsMapFunc), builtinsMapFunc));
JSHandle<JSTaggedValue> linkedHashMap(LinkedHashMap::Create(thread_));
jsMap->SetLinkedMap(thread_, linkedHashMap);
JSHandle<JSTaggedValue> mapValue(jsMap);
JSHandle<JSTaggedValue> mapIteratorVal = JSMapIterator::CreateMapIterator(thread_, mapValue, IterationKind::KEY);
JSHandle<JSMapIterator> mapIterator = JSHandle<JSMapIterator>::Cast(mapIteratorVal);
mapIterator->SetNextIndex(index);
Local<MapIteratorRef> object = JSNApiHelper::ToLocal<MapIteratorRef>(mapIteratorVal);
int32_t result = object->GetIndex();
```



## PrimitiveRef

表述为原始对象，包括Undefined，Null，Boolean，Number，String，Symbol，BigInt 这些Primitive类型的值是不可变的，即一旦创建就不能修改。

### GetValue

Local<JSValueRef> GetValue(const EcmaVM *vm)；

获取原始对象的值。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| :----: | -------------- | ---- | ------------ |
|   vm   | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 获取值将其转换为 Local<JSValueRef>类型对象，并作为函数的返回值。 |

**示例：**

```C++
Local<IntegerRef> intValue = IntegerRef::New(vm, 10);
Local<JSValueRef> jsValue = intValue->GetValue(vm);
```

## IntegerRef

用于表示一个整数，它通常用于处理整数运算，IntegerRef可以存储更大的整数，最多可以存储16个整数。

### New

static Local<IntegerRef> New(const EcmaVM *vm, int input)；

创建一个新的IntegerRef对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明                 |
| :----: | -------------- | ---- | -------------------- |
|   vm   | const EcmaVM * | 是   | 虚拟机对象。         |
| input  | int            | 是   | IntegerRef对象的值。 |

**返回值：**

| 类型              | 说明                         |
| ----------------- | ---------------------------- |
| Local<IntegerRef> | 返回一个新的IntegerRef对象。 |

**示例：**

```C++
Local<IntegerRef> intValue = IntegerRef::New(vm, 0);
```

### NewFromUnsigned

static Local<IntegerRef> NewFromUnsigned(const EcmaVM *vm, unsigned int input)；

创建无符号的IntegerRef对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明                 |
| :----: | -------------- | ---- | -------------------- |
|   vm   | const EcmaVM * | 是   | 虚拟机对象。         |
| input  | int            | 是   | IntegerRef对象的值。 |

**返回值：**

| 类型              | 说明                         |
| ----------------- | ---------------------------- |
| Local<IntegerRef> | 返回一个新的IntegerRef对象。 |

**示例：**

```C++
Local<IntegerRef> intValue = IntegerRef::NewFromUnsigned(vm, 0);
```

### Value

int IntegerRef::Value();

获取一个IntegerRef对象的整数值。

**参数：**

无

**返回值：**

| 类型 | 说明                    |
| ---- | ----------------------- |
| int  | 返回一个int类型的数值。 |

**示例：**

```c++
int num = 0;
Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
int i = intValue->Value();
```





## PromiseRef

用于处理异步操作，它表示一个尚未完成但预计在未来会完成的操作，并且返回一个值。Promise有三种状态：pending（进行中）、fulfilled（已成功）和rejected（已失败）。

### Catch

Local<PromiseRef> Catch(const EcmaVM *vm, Local<FunctionRef> handler)；

用于捕获异步操作中的错误，当一个Promise被rejected时，可以使用catch方法来处理错误。

**参数：**

| 参数名  | 类型               | 必填 | 说明                                                         |
| :-----: | ------------------ | ---- | ------------------------------------------------------------ |
|   vm    | const EcmaVM *     | 是   | 虚拟机对象。                                                 |
| handler | Local<FunctionRef> | 是   | 指向FunctionRef类型的局部变量，表示处理异常的回调函数。将在Promise对象中发生异常时被调用。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<PromiseRef> | 如果在调用过程中发生中断，则返回未定义（undefined）。否则，将结果转换为PromiseRef类型并返回。 |

**示例：**

```C++
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}
Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm);
Local<PromiseRef> promise = capability->GetPromise(vm);
Local<FunctionRef> reject = FunctionRef::New(vm, FunCallback);
Local<PromiseRef> result = promise->Catch(vm, reject);
```

### Then

Local<PromiseRef> Then(const EcmaVM *vm, Local<FunctionRef> handler)；

对Promise设置一个回调函数，Promise对象敲定时执行的函数。

Local<PromiseRef> Then(const EcmaVM *vm, Local<FunctionRef> onFulfilled, Local<FunctionRef> onRejected)；

对Promise设置一个回调函数，Promise对象敲定执行onFulfilled，Promise对象拒绝执行onRejected。

**参数：**

|   参数名    | 类型               | 必填 | 说明                        |
| :---------: | ------------------ | ---- | --------------------------- |
|     vm      | const EcmaVM *     | 是   | 虚拟机对象。                |
| onFulfilled | Local<FunctionRef> | 是   | Promise对象敲定执行的函数。 |
| onRejected  | Local<FunctionRef> | 是   | Promise对象拒绝执行的函数。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<PromiseRef> | 将其结果为 Local<JSValueRef>类型对象，并作为函数的返回值，用于判断异步函数是否设置成功。 |

**示例：**

```C++
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}
Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm);
Local<PromiseRef> promise = capability->GetPromise(vm);
Local<FunctionRef> callback = FunctionRef::New(vm, FunCallback);
Local<PromiseRef> result = promise->Then(vm, callback, callback);
```

### Finally

Local<PromiseRef> Finally(const EcmaVM *vm, Local<FunctionRef> handler)；

无论Promise对象敲定还是拒绝都会执行的函数。

**参数：**

| 参数名  | 类型               | 必填 | 说明             |
| :-----: | ------------------ | ---- | ---------------- |
|   vm    | const EcmaVM *     | 是   | 虚拟机对象。     |
| handler | Local<FunctionRef> | 是   | 需要执行的函数。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<PromiseRef> | 将其结果为 Local<JSValueRef>类型对象，并作为函数的返回值，用于判断异步函数是否设置成功。 |

**示例：**

```C++
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}
Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm);
Local<PromiseRef> promise = capability->GetPromise(vm);
Local<FunctionRef> callback = FunctionRef::New(vm, FunCallback);
Local<PromiseRef> result = promise->Finally(vm, callback);
```

## TryCatch

异常处理类，用于在JS中捕获和处理一些异常。

### GetAndClearException

Local<ObjectRef> GetAndClearException()；

获取和清除捕获到的异常对象。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型             | 说明                                                         |
| ---------------- | ------------------------------------------------------------ |
| Local<ObjectRef> | 获取捕获到的异常，并将其转换为 Local<ObjectRef>类型对象，并将作为函数的返回值。 |

**示例：**

```C++
Local<StringRef> message = StringRef::NewFromUtf8(vm, "ErrorTest");
JSNApi::ThrowException(vm, Exception::Error(vm, message););
TryCatch tryCatch(vm);
Local<ObjectRef> error = tryCatch.GetAndClearException();
```

### TryCatch

explicit TryCatch(const EcmaVM *ecmaVm) : ecmaVm_(ecmaVm) {};

用于构造TryCatch类的对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| ecmaVm | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

无

**示例：**

```c++
TryCatch(vm_);
```

### HasCaught

bool HasCaught() const;

用于检查是否有异常对象被捕获。

**参数：**

无

**返回值：**

| 类型    | 说明                                              |
| :------ | :------------------------------------------------ |
| boolean | 如果有异常对象被捕获，则返回true，否则返回false。 |

**示例：**

```c++
Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
Local<JSValueRef> error = Exception::Error(vm_, message);
JSNApi::ThrowException(vm_, error);
TryCatch tryCatch(vm_);
bool result = tryCatch.HasCaught();
```

### Rethrow

void Rethrow();

如果有异常被捕获并处理，通过检查rethrow_的值来确定是否需要重新抛出异常。

**参数：**

无

**返回值：**

无。

**示例：**

```c++
TryCatch tryCatch(vm_);
tryCatch.Rethrow();
```



## Uint32ArrayRef

用于表示一个无符号32位整数数组的引用，继承自TypedArrayRef，并提供了创建一个新的Uint32Array对象的方法。

### New

static Local<Uint32ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);

用于构造一个使用ArrayBuffer对象指定偏移量和长度转换的Uint32Array对象。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                 |
| ---------- | --------------------- | ---- | ------------------------------------ |
| vm         | const EcmaVM *        | 是   | 虚拟机对象                           |
| buffer     | Local<ArrayBufferRef> | 是   | 要转换为Uint32Array的ArrayBuffer对象 |
| byteOffset | int32_t               | 是   | ArrayBuffer对象的指定位置偏移量      |
| length     | int32_t               | 是   | ArrayBuffer对象的指定长度            |

**返回值：**

| 类型                  | 说明                        |
| :-------------------- | :-------------------------- |
| Local<Uint32ArrayRef> | 返回构造的Uint32Array对象。 |

**示例：**

```c++
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, bufferLength);
Local<Uint32ArrayRef> result = Uint32ArrayRef::New(vm_,arrayBuffer,bufferOffset,offsetLength);
```



## Uint8ArrayRef

用于表示一个无符号8位整数数组的引用，继承自TypedArrayRef，并提供了创建一个新的Uint8Array对象的方法。

### New

static Local<Uint8ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);

用于构造一个使用ArrayBuffer对象指定偏移量和长度转换的Uint8Array对象。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                |
| ---------- | --------------------- | ---- | ----------------------------------- |
| vm         | const EcmaVM *        | 是   | 虚拟机对象                          |
| buffer     | Local<ArrayBufferRef> | 是   | 要转换为Uint8Array的ArrayBuffer对象 |
| byteOffset | int32_t               | 是   | ArrayBuffer对象的指定位置偏移量     |
| length     | int32_t               | 是   | ArrayBuffer对象的指定长度           |

**返回值：**

| 类型                  | 说明                        |
| :-------------------- | :-------------------------- |
| Local<Uint32ArrayRef> | 返回构造的Uint32Array对象。 |

**示例：**

```c++
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, 30);
Local<Uint8ArrayRef> result = Uint8ArrayRef::New(vm_,arrayBuffer,10,10);
```



## MapRef

用于表示和操作JS Map对象的引用，它继承自ObjectRef类，并提供了一些操作JSMap对象的方法。

### New

static Local<MapRef> New(const EcmaVM *vm);

用于创建一个Map对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型          | 说明                |
| :------------ | :------------------ |
| Local<MapRef> | 返回创建的Map对象。 |

**示例：**

```c++
Local<MapRef> result = MapRef::New(vm_);
```

### GetSize

int32_t GetSize();

 返回 `MapRef` 对象中键值对的数量，即 `Map` 对象的大小。

通过 `JSNApiHelper::ToJSHandle(this)` 将当前的 `MapRef` 对象转换为 JavaScript 中的 `JSMap` 对象。

调用 `JSMap::GetSize` 方法获取 `Map` 对象的大小。

**参数：**

无

**返回值：**

| 类型    | 说明                   |
| ------- | ---------------------- |
| int32_t | 返回`Map` 对象的大小。 |

**示例：**

```c++
Local<MapRef> mapRef = MapRef::New(vm_);
int32_t size = mapRef->GetSize();
```

### GetTotalElements

int32_t GetTotalElements();

返回 `MapRef` 对象中所有元素的数量，包括实际存在的元素和已删除的元素。

通过 `JSNApiHelper::ToJSHandle(this)` 将当前的 `MapRef` 对象转换为 JavaScript 中的 `JSMap` 对象。

调用 `JSMap::GetSize` 方法获取 `Map` 对象的大小，加上已删除元素的数量。

**参数：**

无

**返回值：**

| 类型    | 说明                   |
| ------- | ---------------------- |
| int32_t | 对象中所有元素的数量。 |

**示例：**

```c++
Local<MapRef> mapRef = MapRef::New(vm_);
int32_t totalElements = mapRef->GetTotalElements();
```

### Get

Local<JSValueRef> Get(const EcmaVM *vm, Local<JSValueRef> key);

返回 `MapRef` 对象中指定键的值。

在获取之前，通过 `CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `MapRef` 对象转换为 JavaScript 中的 `JSMap` 对象。

调用 `map->Get` 方法获取与给定键对应的值。

**参数：**

| 参数名 | 类型              | 必填 | 说明             |
| ------ | ----------------- | ---- | ---------------- |
| vm     | const EcmaVM *    | 是   | 虚拟机对象。     |
| key    | Local<JSValueRef> | 是   | 表示要获取的键。 |

**返回值：**

| 类型              | 说明                         |
| ----------------- | ---------------------------- |
| Local<JSValueRef> | 返回表示迭代类型的字符串值。 |

**示例：**

```c++
Local<MapRef> mapRef = MapRef::New(vm_);
Local<JSValueRef> myValue = MapRef->Get(myEcmaVM, myKey);
```

### GetKey

Local<JSValueRef> GetKey(const EcmaVM *vm, int entry);

获取 Map 对象中指定索引处的键。

在获取之前，通过 `CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `MapRef` 对象转换为 JavaScript 中的 `JSMap` 对象。

 调用 `map->GetKey(entry)` 方法获取 Map 对象中指定索引处的键，并将结果转换为 `Local<JSValueRef>`。

**参数：**

| 参数名 | 类型           | 必填 | 说明                                          |
| ------ | -------------- | ---- | --------------------------------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象。                                  |
| entry  | int            | 是   | 表示 Map 对象中的条目索引，用于获取相应的键。 |

**返回值：**

| 类型              | 说明                              |
| ----------------- | --------------------------------- |
| Local<JSValueRef> | 返回了 Map 对象中指定索引处的键。 |

**示例：**

```c++
Local<MapRef> mapRef = MapRef::New(vm_);
Local<JSValueRef> myKey = MapRef->GetKey(myEcmaVM, myEntry);
```

### GetValue

Local<JSValueRef> GetValue(const EcmaVM *vm, int entry);

获取 Map 对象中指定索引处的值。

在获取之前，通过 `CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `MapRef` 对象转换为 JavaScript 中的 `JSMap` 对象。

调用 `map->GetValue(entry)` 方法获取 Map 对象中指定索引处的值，并将结果转换为 `Local<JSValueRef>`。

**参数：**

| 参数名 | 类型           | 必填 | 说明               |
| ------ | -------------- | ---- | ------------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。       |
| entry  | int            | 是   | 用于获取相应的值。 |

**返回值：**

| 类型              | 说明                              |
| ----------------- | --------------------------------- |
| Local<JSValueRef> | 返回了 Map 对象中指定索引处的值。 |

**示例：**

```c++
Local<MapRef> mapRef = MapRef::New(vm_);
Local<JSValueRef> myValue = MapRef->Get(myEcmaVM, myEntry);
```

### Set

void Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value);

 向当前 `MapRef` 对象中设置键值对。

在设置之前，通过 `CHECK_HAS_PENDING_EXCEPTION_WITHOUT_RETURN` 宏检查是否存在潜在的异常。

使用 `JSNApiHelper::ToJSHandle(this)` 将当前的 `MapRef` 对象转换为 JavaScript 中的 `JSMap` 对象。

调用 `JSMap::Set` 方法在 Map 对象中设置键值对。

**参数：**

| 参数名 | 类型              | 必填 | 说明             |
| ------ | ----------------- | ---- | ---------------- |
| vm     | const EcmaVM *    | 是   | 虚拟机对象。     |
| key    | Local<JSValueRef> | 是   | 表示要设置的键。 |
| value  | Local<JSValueRef> | 是   | 表示要设置的值。 |

**返回值：**

无

**示例：**

```c++
myMap.Set(myEcmaVM, myKey, myValue);
```



## WeakMapRef

用于表示和操作JS WeakMap对象的类，它继承自ObjectRef类，并提供了一些操作JS WeakMap对象的方法。

### GetSize

int32_t GetSize();

用于获取WeakMap的大小。

**参数：**

无

**返回值：**

| 类型    | 说明                        |
| :------ | :-------------------------- |
| int32_t | 返回获取到的WeakMap的大小。 |

**示例：**

```c++
Local<WeakMapRef> object = WeakMapRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
object->Set(vm_, key, value);
int32_t result = object->GetSize();
```

### GetTotalElements

int32_t GetTotalElements();

用于获取WeakMap的元素个数。

**参数：**

无

**返回值：**

| 类型    | 说明                            |
| :------ | :------------------------------ |
| int32_t | 返回获取到的WeakMap的元素个数。 |

**示例：**

```c++
Local<WeakMapRef> object = WeakMapRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
object->Set(vm_, key, value);
int32_t result = object->GetTotalElements();
```

### GetKey

Local<JSValueRef> GetKey(const EcmaVM *vm, int entry);

通过指定的入口位置获取WeakMap中指定的键。

**参数：**

| 参数名 | 类型           | 必填 | 说明                 |
| ------ | -------------- | ---- | -------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象           |
| entry  | int            | 是   | 要获取的键的入口位置 |

**返回值：**

| 类型              | 说明                   |
| :---------------- | :--------------------- |
| Local<JSValueRef> | 返回获取到的指定的键。 |

**示例：**

```c++
Local<WeakMapRef> object = WeakMapRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
object->Set(vm_, key, value);
Local<JSValueRef> result = object->GetKey(vm_, entry);
```

### GetValue

Local<JSValueRef> GetValue(const EcmaVM *vm, int entry);

通过指定的入口位置获取WeakMap中指定的Value值。

**参数：**

| 参数名 | 类型           | 必填 | 说明                      |
| ------ | -------------- | ---- | ------------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象                |
| entry  | int            | 是   | 要获取的Value值的入口位置 |

**返回值：**

| 类型              | 说明                        |
| :---------------- | :-------------------------- |
| Local<JSValueRef> | 返回获取到的指定的Value值。 |

**示例：**

```c++
Local<WeakMapRef> object = WeakMapRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
object->Set(vm_, key, value);
Local<JSValueRef> result = object->GetValue(vm_, entry);
```



## WeakSetRef

用于表示和操作JS WeakSet对象的类，它继承自ObjectRef类，并提供了一些操作JS WeakSet对象的方法。

### GetSize

int32_t GetSize();

用于获取WeakSet的大小。

**参数：**

无

**返回值：**

| 类型    | 说明                        |
| :------ | :-------------------------- |
| int32_t | 返回获取到的WeakSet的大小。 |

**示例：**

```c++
Local<WeakSetRef> object = WeakSetRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
object->Set(vm_, key, value);
int32_t result = object->GetSize();
```

### GetTotalElements

int32_t GetTotalElements();

用于获取WeakSet的元素个数。

**参数：**

无

**返回值：**

| 类型    | 说明                            |
| :------ | :------------------------------ |
| int32_t | 返回获取到的WeakSet的元素个数。 |

**示例：**

```c++
Local<WeakSetRef> object = WeakSetRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
object->Set(vm_, key, value);
int32_t result = object->GetTotalElements();
```

### GetValue

Local<JSValueRef> GetValue(const EcmaVM *vm, int entry);

通过指定的入口位置获取WeakSet中指定的Value值。

**参数：**

| 参数名 | 类型           | 必填 | 说明                      |
| ------ | -------------- | ---- | ------------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象                |
| entry  | int            | 是   | 要获取的Value值的入口位置 |

**返回值：**

| 类型              | 说明                        |
| :---------------- | :-------------------------- |
| Local<JSValueRef> | 返回获取到的指定的Value值。 |

**示例：**

```c++
Local<WeakSetRef> object = WeakSetRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
object->Set(vm_, key, value);
Local<JSValueRef> result = object->GetValue(vm_, 0);
```



## JSExecutionScope

用于表示JS执行作用域的类，管理JS代码在特定执行环境中的上下文。

### JSExecutionScope

explicit JSExecutionScope(const EcmaVM *vm);

用于构造一个 JSExecutionScope类型的对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

无

**示例：**

```c++
JSExecutionScope(vm_);
```



## NativePointerRef

继承于JSValueRef，提供了构造原生指针的方法。

### New

static Local<NativePointerRef> New(const EcmaVM *vm, void *nativePointer, size_t nativeBindingsize = 0);

static Local<NativePointerRef> New(const EcmaVM *vm, void *nativePointer, NativePointerCallback callBack, void *data, size_t nativeBindingsize = 0);

用于构造一个原生指针类型的对象。

**参数：**

| 参数名            | 类型                  | 必填 | 说明                             |
| ----------------- | --------------------- | ---- | -------------------------------- |
| vm                | const EcmaVM *        | 是   | 虚拟机对象                       |
| nativePointer     | void *                | 是   | 原生指针                         |
| nativeBindingsize | size_t                | 否   | 原生绑定的大小                   |
| callBack          | NativePointerCallback | 是   | 原生指针的回调函数               |
| data              | void *                | 是   | 任意数据指针，作为回调函数的参数 |

**返回值：**

| 类型                    | 说明                                       |
| :---------------------- | :----------------------------------------- |
| Local<NativePointerRef> | 返回构造成功的NativePointerRef类型的对象。 |

**示例：**

```c++
void  *vps = static_cast<void *>(new std::string("test"));
Local<NativePointerRef> result = NativePointerRef::New(vm_, vps, 0);
NativePointerCallback callBack = nullptr;
void *vps = static_cast<void *>(new std::string("test"));
void *vpsdata = static_cast<void *>(new std::string("test"));
Local<NativePointerRef> result =  NativePointerRef::New(vm_, vps, callBack, vpsdata, 0);
```

### Value

void *Value();

获取一个外部指针，并将它指向一个本地指针并返回。

**参数：**

无

**返回值：**

| 类型   | 说明                         |
| :----- | :--------------------------- |
| void * | 返回值为获取原生对象的指针。 |

**示例：**

```c++
void *vps = static_cast<void *>(new std::string("test"));
void *vps1 = static_cast<void *>(new std::string("test"));
Local<NativePointerRef> res_vps = NativePointerRef::New(vm_, vps, NativeAreaAllocator::FreeBufferFunc, vps1, 0);
void *result = res_vps->Value();
```



## BigInt64ArrayRef

用于表示一个64位整数数组，它通常用于处理大整数运算，因为普通的Number类型在JavaScript中只能精确表示到53位整数

### New

static Local<BigInt64ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length)；

创建一个BigInt64ArrayRef对象。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                  |
| ---------- | --------------------- | ---- | ------------------------------------- |
| vm         | const EcmaVM *        | 是   | 虚拟机对象。                          |
| buffer     | Local<ArrayBufferRef> | 是   | 一个 ArrayBuffer 对象，用于存储数据。 |
| byteOffset | int32_t               | 是   | 表示从缓冲区的哪个字节开始读取数据。  |
| length     | int32_t               | 是   | 表示要读取的元素数量。                |

**返回值：**

| 类型                    | 说明                           |
| ----------------------- | ------------------------------ |
| Local<BigInt64ArrayRef> | 一个新的BigInt64ArrayRef对象。 |

**示例：**

```C++
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm, 5);
Local<ObjectRef> object = BigInt64ArrayRef::New(vm, buffer, 0, 5);
```

## BigIntRef

用于表示任意大的整数。它提供了一种方法来处理超过Number类型能表示的整数范围的数字。

### New

static Local<BigIntRef> New(const EcmaVM *vm, int64_t input)；

创建一个新的BigIntRef对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明                          |
| ------ | -------------- | ---- | ----------------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象。                  |
| input  | int64_t        | 是   | 需要转为BigIntRef对象的数值。 |

**返回值：**

| 类型             | 说明                    |
| ---------------- | ----------------------- |
| Local<BigIntRef> | 一个新的BigIntRef对象。 |

**示例：**

```C++
int64_t maxInt64 = std::numeric_limits<int64_t>::max();
Local<BigIntRef> valie = BigIntRef::New(vm, maxInt64);
```

### BigIntToInt64

void BigIntRef::BigIntToInt64(const EcmaVM *vm, int64_t *cValue, bool *lossless)；

将BigInt对象转换为64位有符号整数，是否能够正确处理无损转换。

**参数：**

| 参数名   | 类型           | 必填 | 说明                                    |
| -------- | -------------- | ---- | --------------------------------------- |
| vm       | const EcmaVM * | 是   | 虚拟机对象。                            |
| cValue   | int64_t *      | 是   | 用于存储转换为Int64数值的变量。         |
| lossless | bool *         | 是   | 用于判断超大数是否能够转换为Int64类型。 |

**返回值：**

| 类型 | 说明       |
| ---- | ---------- |
| void | 无返回值。 |

**示例：**

```C++
uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm, maxUint64);
int64_t toNum;
bool lossless = true;
maxBigintUint64->BigIntToInt64(vm, &toNum, &lossless);
```

### BigIntToUint64

void BigIntRef::BigIntToUint64(const EcmaVM *vm, uint64_t *cValue, bool *lossless)；

将BigInt对象转换为64位无符号整数，无损转换是否可以正确处理。

**参数：**

| 参数名   | 类型           | 必填 | 说明                                    |
| -------- | -------------- | ---- | --------------------------------------- |
| vm       | const EcmaVM * | 是   | 虚拟机对象。                            |
| cValue   | uint64_t *     | 是   | 用于存储转换为uint64_t数值的变量。      |
| lossless | bool *         | 是   | 用于判断超大数是否能够转换为Int64类型。 |

**返回值：**

| 类型 | 说明       |
| ---- | ---------- |
| void | 无返回值。 |

**示例：**

```C++
uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm, maxUint64);
int64_t toNum;
bool lossless = true;
maxBigintUint64->BigIntToInt64(vm, &toNum, &lossless);
```

### CreateBigWords

Local<JSValueRef> BigIntRef::CreateBigWords(const EcmaVM *vm, bool sign, uint32_t size, const uint64_t* words)；

将一个uint64_t数组包装为一个BigIntRef对象。

**参数：**

| 参数名 | 类型            | 必填 | 说明                                 |
| ------ | --------------- | ---- | ------------------------------------ |
| vm     | const EcmaVM *  | 是   | 虚拟机对象。                         |
| sign   | bool            | 是   | 确定生成的 `BigInt` 是正数还是负数。 |
| size   | uint32_t        | 是   | uint32_t 数组大小。                  |
| words  | const uint64_t* | 是   | uint32_t 数组。                      |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 将uint32_t 转换为BigIntRef对象，并将其转换为Local<JSValueRef>类型，作为函数的返回值。 |

**示例：**

```C++
bool sign = false;
uint32_t size = 3;
const uint64_t words[3] = {
    std::numeric_limits<uint64_t>::min() - 1,
    std::numeric_limits<uint64_t>::min(),
    std::numeric_limits<uint64_t>::max(),
};
Local<JSValueRef> bigWords = BigIntRef::CreateBigWords(vm, sign, size, words);
```

### GetWordsArraySize

uint32_t GetWordsArraySize()；

获取BigIntRef对象包装uint64_t数组的大小。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型     | 说明                                      |
| -------- | ----------------------------------------- |
| uint32_t | 返回BigIntRef对象包装uint64_t数组的大小。 |

**示例：**

```C++
bool sign = false;
uint32_t size = 3;
const uint64_t words[3] = {
    std::numeric_limits<uint64_t>::min() - 1,
    std::numeric_limits<uint64_t>::min(),
    std::numeric_limits<uint64_t>::max(),
};
Local<JSValueRef> bigWords = BigIntRef::CreateBigWords(vm, sign, size, words);
Local<BigIntRef> bigWordsRef(bigWords);
uint32_t cnt = bigWordsRef->GetWordsArraySize();
```

### GetWordsArray

void BigIntRef::GetWordsArray(bool *signBit, size_t wordCount, uint64_t *words); 

获取一个BigIntRef对象的值，并将其表示为一个64位无符号整数数组。同时，它还会设置一个布尔值signBit，表示该BigIntRef对象的符号（正数或负数）。

**参数：**

| 参数名    | 类型       | 必填 | 说明                                                         |
| --------- | ---------- | ---- | ------------------------------------------------------------ |
| signBit   | bool *     | 是   | 指向布尔值的指针，用于存储BigIntRef对象的符号（正数或负数）。 |
| wordCount | size_t     | 是   | 无符号整数，表示要获取的64位无符号整数数组的长度。           |
| words     | uint64_t * | 是   | 指向64位无符号整数的指针，用于存储BigIntRef对象的值。        |

**返回值：**

无

**示例：**

```c++
bool sign = false;
uint32_t size = 3;
const uint64_t words[3] = {
    std::numeric_limits<uint64_t>::min() - 1,
    std::numeric_limits<uint64_t>::min(),
    std::numeric_limits<uint64_t>::max(),
};
Local<JSValueRef> bigWords = BigIntRef::CreateBigWords(vm_, sign, size, words);
Local<BigIntRef> bigWordsRef(bigWords);
bool resultSignBit = true;
uint64_t *resultWords = new uint64_t[3]();
bigWordsRef->GetWordsArray(&resultSignBit, size, resultWords);
```



## StringRef

继承于PrimitiveRef，用于表示字符串类型数据的引用，提供了一些对字符串的操作方法。

### NewFromUtf8

Local<StringRef> StringRef::NewFromUtf8(const EcmaVM *vm, const char *utf8, int length)。

创建utf8类型的StringRef对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| ------ | -------------- | ---- | ---------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象。     |
| utf8   | char *         | 是   | char类型字符串。 |
| int    | length         | 是   | 字符串长度。     |

**返回值：**

| 类型             | 说明                    |
| ---------------- | ----------------------- |
| Local<StringRef> | 一个新的StringRef对象。 |

**示例：**

```C++
std::string testUtf8 = "Hello world";
Local<StringRef> description = StringRef::NewFromUtf8(vm, testUtf8.c_str());
```

### NewFromUtf16

Local<StringRef> StringRef::NewFromUtf16(const EcmaVM *vm, const char16_t *utf16, int length)；

创建utf16类型的StringRef对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明                  |
| ------ | -------------- | ---- | --------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象。          |
| utf16  | char16_t *     | 是   | char16_t 类型字符串。 |
| int    | length         | 是   | 字符串长度。          |

**返回值：**

| 类型             | 说明                    |
| ---------------- | ----------------------- |
| Local<StringRef> | 一个新的StringRef对象。 |

**示例：**

```C++
char16_t data = 0Xdf06;
Local<StringRef> obj = StringRef::NewFromUtf16(vm, &data);
```

### Utf8Length

int32_t StringRef::Utf8Length(const EcmaVM *vm)；

按utf8类型读取StringRef的值长度。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型    | 说明                   |
| ------- | ---------------------- |
| int32_t | utf8类型字符串的长度。 |

**示例：**

```C++
std::string testUtf8 = "Hello world";
Local<StringRef> stringObj = StringRef::NewFromUtf8(vm, testUtf8.c_str());
int32_t lenght = stringObj->Utf8Length(vm);
```

### WriteUtf8

int StringRef::WriteUtf8(char *buffer, int length, bool isWriteBuffer)；

将StringRef的值写入char数组缓冲区。

**参数：**

| 参数名        | 类型   | 必填 | 说明                                  |
| ------------- | ------ | ---- | ------------------------------------- |
| buffer        | char * | 是   | 需要写入的缓冲区。                    |
| length        | int    | 是   | 需要写入缓冲区的长度。                |
| isWriteBuffer | bool   | 否   | 是否需要将StringRef的值写入到缓冲区。 |

**返回值：**

| 类型 | 说明                              |
| ---- | --------------------------------- |
| int  | 将StringRef的值转为Utf8后的长度。 |

**示例：**

```C++
Local<StringRef> local = StringRef::NewFromUtf8(vm, "abcdefbb");
char cs[16] = {0};
int length = local->WriteUtf8(cs, 6);
```

### WriteUtf16

int StringRef::WriteUtf16(char16_t *buffer, int length)；

将StringRef的值写入char数组缓冲区。

**参数：**

| 参数名 | 类型   | 必填 | 说明                   |
| ------ | ------ | ---- | ---------------------- |
| buffer | char * | 是   | 需要写入的缓冲区。     |
| length | int    | 是   | 需要写入缓冲区的长度。 |

**返回值：**

| 类型 | 说明                              |
| ---- | --------------------------------- |
| int  | 将StringRef的值转为Utf8后的长度。 |

**示例：**

```c++
Local<StringRef> local = StringRef::NewFromUtf16(vm, u"您好，华为！");
char cs[16] = {0};
int length = local->WriteUtf16(cs, 3);
```

### Length

uint32_t StringRef::Length()；

获取StringRef的值的长度。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型 | 说明                  |
| ---- | --------------------- |
| int  | StringRef的值的长度。 |

**示例：**

```c++
Local<StringRef> local = StringRef::NewFromUtf8(vm, "abcdefbb");
int len = local->Length()
```

### ToString

std::string StringRef::ToString()；

将StringRef的值转换为std::string。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型        | 说明                                  |
| ----------- | ------------------------------------- |
| std::string | 将StringRef的value转为C++string类型。 |

**示例：**

```c++
Local<StringRef> stringObj = StringRef::NewFromUtf8(vm, "abc");
std::string str = stringObj->ToString();
```

### GetNapiWrapperString

Local<StringRef> StringRef::GetNapiWrapperString(const EcmaVM *vm);

获取一个表示NAPI包装字符串的StringRef对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型             | 说明                                                         |
| ---------------- | ------------------------------------------------------------ |
| Local<StringRef> | 将表示NAPI包装字符串的对象转换为StringRef对象，并将其作为函数的返回值。 |

**示例：**

```c++
Local<StringRef> local = StringRef::GetNapiWrapperString(vm_);
```

### WriteLatin1

int WriteLatin1(char *buffer, int length);

将字符串写入到指定的缓冲区中。

**参数：**

| 参数名 | 类型   | 必填 | 说明             |
| ------ | ------ | ---- | ---------------- |
| buffer | char * | 是   | 指定的缓冲区     |
| length | int    | 是   | 要写入的数据长度 |

**返回值：**

| 类型 | 说明                   |
| :--- | :--------------------- |
| int  | 返回值为写入的字节数。 |

**示例：**

```c++
Local<StringRef> object = StringRef::NewFromUtf8(vm_, "abcdefbb");
char cs[16] = {0};
int result = object->WriteLatin1(cs, length);
```



## NumberRef

继承于PrimitiveRef，用于表示Number类型数据的引用，并提供了构造NumberRef对象的方法，以及对Number类型数据的一些操作。

### New

static Local<NumberRef> New(const EcmaVM *vm, double input);

static Local<NumberRef> New(const EcmaVM *vm, int32_t input);

static Local<NumberRef> New(const EcmaVM *vm, uint32_t input);

static Local<NumberRef> New(const EcmaVM *vm, int64_t input);

该接口为重载函数，用于构造不同数据类型的NumberRef对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明             |
| ------ | -------------- | ---- | ---------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象       |
| input  | double         | 是   | double类型数据   |
| input  | int32_t        | 是   | int32_t类型数据  |
| input  | uint32_t       | 是   | uint32_t类型数据 |
| input  | int64_t        | 是   | int64_t类型数据  |

**返回值：**

| 类型             | 说明                                      |
| :--------------- | :---------------------------------------- |
| Local<NumberRef> | 返回构造成功的不同数据类型的NumberRef对象 |

**示例：**

```c++
double doubleValue = 3.14;
Local<NumberRef> result = NumberRef::New(vm_, doubleValue);
int32_t int32Value = 10;
Local<NumberRef> result = NumberRef::New(vm_, int32Value);
uint32_t uint32Value = 10;
Local<NumberRef> result = NumberRef::New(vm_, uint32Value);
int64_t int64Value = 10;
Local<NumberRef> result = NumberRef::New(vm_, int64Value);
```



## ArrayRef

继承于ObjectRef，用于构造一个数组对象，并提供了一些操作数组的方法。

### Length

uint32_t Length(const EcmaVM *vm);

获取数组对象的长度。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型     | 说明                   |
| :------- | :--------------------- |
| uint32_t | 返回获取到的数组长度。 |

**示例：**

```c++
Local<ArrayRef> arrayObj = ArrayRef::New(vm_, 3);
uint32_t result = arrayObj->Length(vm_);
```

### SetValueAt

static bool SetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index, Local<JSValueRef> value);

用于在给定的数组对象中设置指定索引位置的值。

**参数：**

| 参数名 | 类型              | 必填 | 说明                 |
| ------ | ----------------- | ---- | -------------------- |
| vm     | const EcmaVM *    | 是   | 虚拟机对象           |
| obj    | Local<JSValueRef> | 是   | 指定的数组对象       |
| index  | uint32_t          | 是   | 要设置的指定索引位置 |
| value  | Local<JSValueRef> | 是   | 要设置的值           |

**返回值：**

| 类型    | 说明                                                  |
| :------ | :---------------------------------------------------- |
| boolean | 指定索引位置的值设置成功，则返回true，否则返回false。 |

**示例：**

```c++
Local<ArrayRef> arrayObj = ArrayRef::New(vm_, 1);
Local<IntegerRef> intValue = IntegerRef::New(vm_, 0);
bool result = ArrayRef::SetValueAt(vm_, arrayObj, index, intValue);
```

### GetValueAt

static Local<JSValueRef> GetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index);

用于在给定的数组对象中获取指定索引位置的值。

**参数：**

| 参数名 | 类型              | 必填 | 说明                 |
| ------ | ----------------- | ---- | -------------------- |
| vm     | const EcmaVM *    | 是   | 虚拟机对象           |
| obj    | Local<JSValueRef> | 是   | 指定的数组对象       |
| index  | uint32_t          | 是   | 要获取的指定索引位置 |

**返回值：**

| 类型              | 说明                           |
| :---------------- | :----------------------------- |
| Local<JSValueRef> | 返回获取到的指定索引位置的值。 |

**示例：**

```c++
Local<ArrayRef> arrayObj = ArrayRef::New(vm_, 1);
Local<IntegerRef> intValue = IntegerRef::New(vm_, 0);
ArrayRef::SetValueAt(vm_, arrayObj, index, intValue);
Local<JSValueRef> result = ArrayRef::GetValueAt(vm_, arrayObj, 0);
```

### New

Local<ArrayRef> ArrayRef::New(const EcmaVM *vm, uint32_t length = 0);

创建一个具有指定长度的 JavaScript 数组对象。

在创建数组对象之前，通过 `CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED` 宏检查是否存在潜在的异常。

使用 `JSArray::ArrayCreate` 方法创建 JavaScript 中的数组对象，指定数组的长度为 `length`。

使用 `JSNApiHelper::ToLocal<ArrayRef>(array)` 将 JavaScript 中的数组对象转换为本地的 `ArrayRef` 对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明                     |
| ------ | -------------- | ---- | ------------------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。             |
| length | uint32_t       | 否   | 表示要创建的数组的长度。 |

**返回值：**

| 类型            | 说明                   |
| --------------- | ---------------------- |
| Local<ArrayRef> | ArrayRef  类型的对象。 |

**示例：**

```c++
 Local<ArrayRef> myArray = ArrayRef::New(VM, length);
```



## JsiRuntimeCallInfo

主要用于处理JS运行时调用的信息，并提供了一些方法。

### GetData

void* GetData();

获取JsiRuntimeCallInfo的数据。

**参数：**

无

**返回值：**

| 类型   | 说明                                         |
| :----- | :------------------------------------------- |
| void * | 返回值为从JsiRuntimeCallInfo中获取到的数据。 |

**示例：**

```c++
JsiRuntimeCallInfo object;
void *result = object.GetData();
```

### GetVM

EcmaVM *GetVM() const;

获取与当前 `JsiRuntimeCallInfo` 对象相关联的 `EcmaVM` 指针。

**参数：**

无

**返回值：**

| 类型    | 说明             |
| ------- | ---------------- |
| EcmaVM* | 返回EcmaVM指针。 |

**示例：**

```c++
JsiRuntimeCallInfo callInfo;
EcmaVM *vm = callInfo.GetVM();
```



## PromiseRejectInfo

`PromiseRejectInfo` 类用于存储有关 Promise 被拒绝事件的信息，包括被拒绝的 Promise 对象、拒绝的原因、事件类型和与事件相关的数据。提供了相应的访问方法用于获取这些信息。

### GetPromise

Local<JSValueRef> GetPromise() const；

获取一个Promise对象。

**参数：**

| 参数名 | 类型 | 必填 | 说明 |
| ------ | ---- | ---- | ---- |
| 无参   |      |      |      |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 获取Promise对象，并将其转换为Local<JSValueRef>类型，作为函数的返回值。 |

**示例：**

```C++
Local<JSValueRef> promise(PromiseCapabilityRef::New(vm)->GetPromise(vm));
Local<StringRef> toStringReason = StringRef::NewFromUtf8(vm, "3.14");
Local<JSValueRef> reason(toStringReason);
void *data = static_cast<void *>(new std::string("promisereject"));
PromiseRejectInfo promisereject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data);
Local<JSValueRef> obj = promisereject.GetPromise();
```

### GetData

void* PromiseRejectInfo::GetData() const

返回存储在类的私有成员变量data_中的额外数据。

**参数：**

无

**返回值：**

| 类型   | 说明                                                         |
| ------ | ------------------------------------------------------------ |
| void * | 返回值是一个通用指针（void *），它指向存储在类的私有成员变量data_中的额外数据。 |

**示例：**

```c++
Local<StringRef> toStringPromise = StringRef::NewFromUtf8(vm_, "-3.14");
Local<JSValueRef> promise(toStringPromise);
Local<StringRef> toStringReason = StringRef::NewFromUtf8(vm_, "3.14");
Local<JSValueRef> reason(toStringReason);
void *data = static_cast<void *>(new std::string("promisereject"));
PromiseRejectInfo promisereject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data);
void *ptr = promisereject.GetData();
```

### PromiseRejectInfo

PromiseRejectInfo(Local<JSValueRef> promise, Local<JSValueRef> reason,PromiseRejectInfo::PROMISE_REJECTION_EVENT operation, void* data);

创建一个 `PromiseRejectInfo` 对象，用于保存与 Promise 被拒绝事件相关的信息。

将传入的 `promise`、`reason`、`operation` 和 `data` 参数存储在对象的成员变量中。

**参数：**

| 参数名    | 类型                                       | 必填 | 说明                                  |
| --------- | ------------------------------------------ | ---- | ------------------------------------- |
| promise   | Local<JSValueRef>                          | 是   | 表示被拒绝的 Promise 对象。           |
| reason    | Local<JSValueRef>                          | 是   | 表示 Promise 被拒绝的原因。           |
| operation | PromiseRejectInfo::PROMISE_REJECTION_EVENT | 是   | 表示 Promise 被拒绝的事件类型。       |
| data      | void*                                      | 是   | 表示与 Promise 被拒绝事件相关的数据。 |

**返回值：**

| 类型                                       | 说明                                       |
| ------------------------------------------ | ------------------------------------------ |
| PromiseRejectInfo::PROMISE_REJECTION_EVENT | `PromiseRejectInfo` 对象的操作类型枚举值。 |

**示例：**

```c++
PromiseRejectInfo::PROMISE_REJECTION_EVENT operationType = myRejectInfo.GetOperation();
```

### GetReason

  Local<JSValueRef> GetReason() const;

获取存储在 `PromiseRejectInfo` 对象中的被拒绝的 Promise 的原因。

**参数：**

无

**返回值：**

| 类型              | 说明                                       |
| ----------------- | ------------------------------------------ |
| Local<JSValueRef> | `PromiseRejectInfo` 对象的操作类型枚举值。 |

**示例：**

```c++
Local<JSValueRef> rejectionReason = myRejectInfo.GetReason();
```

### GetOperation

PromiseRejectInfo::PROMISE_REJECTION_EVENT GetOperation() const;

获取当前 `PromiseRejectInfo` 对象的操作类型。

返回值是一个枚举值，表示 Promise 拒绝事件的具体操作类型。

**参数：**

无

**返回值：**

| 类型                                       | 说明                                       |
| ------------------------------------------ | ------------------------------------------ |
| PromiseRejectInfo::PROMISE_REJECTION_EVENT | `PromiseRejectInfo` 对象的操作类型枚举值。 |

**示例：**

```c++
PromiseRejectInfo::PROMISE_REJECTION_EVENT operationType = myRejectInfo.GetOperation();
```



## PromiseCapabilityRef

`PromiseCapabilityRef` 类是 `ObjectRef` 类的子类，专门用于处理 Promise 对象的功能。它提供了创建新的 PromiseCapability 对象、解决 Promise、拒绝 Promise 以及获取 Promise 的方法。

### Resolve

bool Resolve(const EcmaVM *vm, Local<JSValueRef> value)；

用于敲定Promise对象。

**参数：**

| 参数名 | 类型              | 必填 | 说明                         |
| ------ | ----------------- | ---- | ---------------------------- |
| vm     | const EcmaVM *    | 是   | 虚拟机对象。                 |
| value  | Local<JSValueRef> | 是   | 执行回调函数是所需要的参数。 |

**返回值：**

| 类型 | 说明                                |
| ---- | ----------------------------------- |
| bool | Promise对象的回调函数是否成功调用。 |

**示例：**

```c++
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}
Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm);
Local<PromiseRef> promise = capability->GetPromise(vm);
promise->Then(vm, FunctionRef::New(vm, FunCallback), FunctionRef::New(vm, FunCallback));
bool b = capability->Resolve(vm, NumberRef::New(vm, 300.3));
```

### Reject

bool Reject(const EcmaVM *vm, Local<JSValueRef> reason)；

用于拒绝Promise对象。

**参数：**

| 参数名 | 类型              | 必填 | 说明                         |
| ------ | ----------------- | ---- | ---------------------------- |
| vm     | const EcmaVM *    | 是   | 虚拟机对象。                 |
| value  | Local<JSValueRef> | 是   | 执行回调函数是所需要的参数。 |

**返回值：**

| 类型 | 说明                                |
| ---- | ----------------------------------- |
| bool | Promise对象的回调函数是否成功调用。 |

**示例：**

```C++
Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}
Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm);
Local<PromiseRef> promise = capability->GetPromise(vm);
promise->Then(vm, FunctionRef::New(vm, FunCallback), FunctionRef::New(vm, FunCallback));
bool b = capability->Reject(vm, NumberRef::New(vm, 300.3));
```

### GetPromise

Local<PromiseRef> GetPromise(const EcmaVM *vm);

获取与当前对象关联的Promise对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<PromiseRef> | 获取Promise对象，并将其转换为本地引用（PromiseRef类型）返回。 |

**示例：**

```c++
Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
Local<PromiseRef> promise = capability->GetPromise(vm_);
```

### new

static Local<PromiseCapabilityRef> New(const EcmaVM *vm);

创建一个新的 `PromiseCapability` 对象，并返回对该对象的本地引用。

在创建之前，通过 `CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED` 宏检查是否存在潜在的异常。

获取虚拟机的线程、工厂对象和全局环境。

使用全局环境中的 `GetPromiseFunction` 函数获取 `Promise` 构造函数。

调用 `JSPromise::NewPromiseCapability` 方法创建新的 `PromiseCapability` 对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型                        | 说明                                       |
| --------------------------- | ------------------------------------------ |
| Local<PromiseCapabilityRef> | `PromiseRejectInfo` 对象的操作类型枚举值。 |

**示例：**

```c++
Local<PromiseCapabilityRef> myPromiseCapability = PromiseCapabilityRef::New(vm);
```



## SymbolRef : public PrimitiveRef 

这个类继承自PrimitiveRef类，主要用于定义一个名为`SymbolRef`的公共API类。

### New

static Local<SymbolRef> New(const EcmaVM *vm, Local<StringRef> description = Local<StringRef>());

用于创建一个新的`SymbolRef`对象。

**参数：**

| 参数名      | 类型             | 必填 | 说明                                                         |
| ----------- | ---------------- | ---- | ------------------------------------------------------------ |
| vm          | const EcmaVM *   | 是   | 虚拟机对象。                                                 |
| description | Local<StringRef> | 否   | 可选的`Local<StringRef>`类型的描述。如果没有提供描述，则默认为空。 |

**返回值：**

| 类型             | 说明                                      |
| ---------------- | ----------------------------------------- |
| Local<SymbolRef> | 创建一个新的SymbolRef类型对象并将其返回。 |

**示例：**

```c++
SymbolRef::New(vm_);
```

### GetDescription

Local<StringRef> GetDescription(const EcmaVM *vm);

获取符号引用的描述。

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型             | 说明                                   |
| ---------------- | -------------------------------------- |
| Local<StringRef> | 将符号描述的句柄转换为本地引用并返回。 |

**示例：**

```c++
SymbolRef::GetDescription(vm_);
```



## FunctionCallScope

这个类用于管理函数调用的作用域。

### FunctionCallScope

FunctionCallScope::FunctionCallScope(EcmaVM *vm) : vm_(vm);

FunctionCallScope类的带参构造函数。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

无

**示例：**

```c++
FunctionCallScope::FunctionCallScope FC(vm_); 
```



## LocalScope

该类的作用是管理局部作用域，包括跟踪前一个和后一个局部作用域、前一个局部作用域的结束位置、前一个局部作用域的句柄存储索引以及线程信息。

### LocalScope

explicit LocalScope(const EcmaVM *vm);

初始化 LocalScope 对象，并设置其关联的 EcmaVM实例。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

无

**示例：**

```c++
LocalScope::LocalScope Scope(vm_);
```

### LocalScope

inline LocalScope(const EcmaVM *vm, JSTaggedType value);

构造函数：用于初始化 `LocalScope` 类的对象。

获取当前线程的 `JSThread` 对象。

通过 `reinterpret_cast` 将线程对象转换为 `JSThread*` 类型。

使用 `EcmaHandleScope::NewHandle` 方法在句柄作用域中创建一个新的本地句柄。

**参数：**

| 参数名 | 类型           | 必填 | 说明                         |
| ------ | -------------- | ---- | ---------------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象。                 |
| value  | JSTaggedType   | 是   | 表示要存储在本地句柄中的值。 |

**返回值：**

无

**示例：**

```c++
 LocalScope myLocalScope(myEcmaVM, someJSTaggedValue);
```



## SetRef : public ObjectRef

该类是继承自ObjectRef类的子类。它的作用是表示一个JavaScript的Set对象，并提供了获取其大小、总元素数量以及获取指定索引位置的元素值的方法。

### GetSize

int32_t SetRef::GetSize();

获取一个SetRef对象的大小。

**参数：**

无

**返回值：**

| 类型    | 说明                                              |
| ------- | ------------------------------------------------- |
| int32_t | 返回一个int32_t类型的数值，表示SetRef对象的大小。 |

**示例：**

```c++
Local<SetRef> object = SetRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
object->Set(vm_, key, value);
int32_t size = object->GetSize();
```

### GetTotalElements

int32_t SetRef::GetTotalElements();

获取一个SetRef对象中所有元素的总数，包括已删除的元素。

**参数：**

无

**返回值：**

| 类型    | 说明                                                         |
| ------- | ------------------------------------------------------------ |
| int32_t | 返回一个int32_t类型的数值，表示SetRef对象中所有元素的总数，包括已删除的元素。 |

**示例：**

```c++
Local<SetRef> object = SetRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
object->Set(vm_, key, value);
int32_t element = object->GetTotalElements();
```

### GetValue

Local<JSValueRef> SetRef::GetValue(const EcmaVM *vm, int entry);

获取SetRef对象中指定索引位置的元素值。

**参数：**

| 参数名 | 类型           | 必填 | 说明                                                 |
| ------ | -------------- | ---- | ---------------------------------------------------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象。                                         |
| entry  | int            | 是   | 一个整数，表示要获取的元素在SetRef对象中的索引位置。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 获取指定索引位置的元素值后，将其转换为Local<JSValueRef>类型的对象，并作为函数的返回值。 |

**示例：**

```c++
Local<SetRef> object = SetRef::New(vm_);
Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
object->Set(vm_, key, value);
Local<JSValueRef> value = object->GetValue(vm_, 0);
```



## BigUint64ArrayRef : public TypedArrayRef

该类继承自TypedArrayRef类，用于表示一个64位无符号整数数组。

### New

static Local<BigUint64ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);

静态成员函数，用于创建一个新的BigUint64ArrayRef对象。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                                   |
| ---------- | --------------------- | ---- | ------------------------------------------------------ |
| vm         | const EcmaVM *        | 是   | 虚拟机对象。                                           |
| buffer     | Local<ArrayBufferRef> | 是   | 一个ArrayBufferRef类型的局部变量，要存储数据的缓冲区。 |
| byteOffset | int32_t               | 是   | 一个32位整数，表示在缓冲区中的字节偏移量。             |
| length     | int32_t               | 是   | 一个32位整数，表示数组的长度。                         |

**返回值：**

| 类型                     | 说明                                          |
| ------------------------ | --------------------------------------------- |
| Local<BigUint64ArrayRef> | 返回一个创建好的BigUint64ArrayRef类型的对象。 |

**示例：**

```c++
Local<ArrayBufferRef> array = ArrayBufferRef::New(/*....所需参数....*/);
int32_t byteOffset = 3;
int32_t length = 40;
Local<BigUint64ArrayRef> bu64array = BigUint64ArrayRef::New(vm_, array, byteOffset, length);
```



## Float32ArrayRef : public TypedArrayRef

该类继承自TypedArrayRef类，作用是创建一个浮点数类型的数组引用，可以用于操作和访问浮点数类型的数据。

### New

static Local<Float32ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);

静态成员函数，用于创建一个新的Float32ArrayRef对象。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                                   |
| ---------- | --------------------- | ---- | ------------------------------------------------------ |
| vm         | const EcmaVM *        | 是   | 虚拟机对象。                                           |
| buffer     | Local<ArrayBufferRef> | 是   | 一个ArrayBufferRef类型的局部变量，要存储数据的缓冲区。 |
| byteOffset | int32_t               | 是   | 一个32位整数，表示在缓冲区中的字节偏移量。             |
| length     | int32_t               | 是   | 一个32位整数，表示数组的长度。                         |

**返回值：**

| 类型                   | 说明                                                         |
| ---------------------- | ------------------------------------------------------------ |
| Local<Float32ArrayRef> | 返回一个Float32ArrayRef类型的对象，表示创建的浮点数类型数组引用。 |

**示例：**

```c++
Local<ArrayBufferRef> array = ArrayBufferRef::New(/*....所需参数....*/);
int32_t byteOffset = 3;
int32_t length = 40;
Local<Float32ArrayRef> fl32array = Float32ArrayRef::New(vm_, array, byteOffset, length);
```



## Float64ArrayRef : public TypedArrayRef

该类继承自TypedArrayRef类，表示一个64位浮点数类型的数组引用。

### New

static Local<Float64ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);

静态成员函数，用于创建一个新的Float64ArrayRef对象。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                                   |
| ---------- | --------------------- | ---- | ------------------------------------------------------ |
| vm         | const EcmaVM *        | 是   | 虚拟机对象。                                           |
| buffer     | Local<ArrayBufferRef> | 是   | 一个ArrayBufferRef类型的局部变量，要存储数据的缓冲区。 |
| byteOffset | int32_t               | 是   | 一个32位整数，表示在缓冲区中的字节偏移量。             |
| length     | int32_t               | 是   | 一个32位整数，表示数组的长度。                         |

**返回值：**

| 类型                   | 说明                                                         |
| ---------------------- | ------------------------------------------------------------ |
| Local<Float64ArrayRef> | 返回一个Float64ArrayRef类型的对象，表示创建的浮点数类型数组引用。 |

**示例：**

```c++
Local<ArrayBufferRef> array = ArrayBufferRef::New(/*....所需参数....*/);
int32_t byteOffset = 3;
int32_t length = 40;
Local<Float64ArrayRef> fl32array = Float64ArrayRef::New(vm_, array, byteOffset, length);
```



## Int8ArrayRef : public TypedArrayRef

该类继承自 TypedArrayRef 类。其作用是创建一个表示 8 位整数类型的数组引用。

### New

static Local<Int8ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);

创建一个新的Int8ArrayRef对象。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                                   |
| ---------- | --------------------- | ---- | ------------------------------------------------------ |
| vm         | const EcmaVM *        | 是   | 虚拟机对象。                                           |
| buffer     | Local<ArrayBufferRef> | 是   | 一个ArrayBufferRef类型的局部变量，要存储数据的缓冲区。 |
| byteOffset | int32_t               | 是   | 一个32位整数，表示在缓冲区中的字节偏移量。             |
| length     | int32_t               | 是   | 一个32位整数，表示数组的长度。                         |

**返回值：**

| 类型                | 说明                                         |
| ------------------- | -------------------------------------------- |
| Local<Int8ArrayRef> | 返回一个新创建的Int8ArrayRef对象的本地引用。 |

**示例：**

```c++
const int32_t length = 15;
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
Local<Int8ArrayRef> obj = Int8ArrayRef::New(vm_, arrayBuffer, 5, 6);
```



## Int16ArrayRef : public TypedArrayRef

该类继承自 TypedArrayRef 类。其作用是创建一个表示 16位整数类型的数组引用。

### New

static Local<Int16ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);

创建一个新的Int16ArrayRef对象。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                                   |
| ---------- | --------------------- | ---- | ------------------------------------------------------ |
| vm         | const EcmaVM *        | 是   | 虚拟机对象。                                           |
| buffer     | Local<ArrayBufferRef> | 是   | 一个ArrayBufferRef类型的局部变量，要存储数据的缓冲区。 |
| byteOffset | int32_t               | 是   | 一个32位整数，表示在缓冲区中的字节偏移量。             |
| length     | int32_t               | 是   | 一个32位整数，表示数组的长度。                         |

**返回值：**

| 类型                 | 说明                                          |
| -------------------- | --------------------------------------------- |
| Local<Int16ArrayRef> | 返回一个新创建的Int16ArrayRef对象的本地引用。 |

**示例：**

```c++
const int32_t length = 15;
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
Local<Int16ArrayRef> obj = Int16ArrayRef::New(vm_, arrayBuffer, 5, 6);
```



## Int32ArrayRef : public TypedArrayRef

该类继承自 TypedArrayRef 类。其作用是创建一个表示 32位整数类型的数组引用。

### New

static Local<Int32ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length);

创建一个新的Int32ArrayRef对象。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                                   |
| ---------- | --------------------- | ---- | ------------------------------------------------------ |
| vm         | const EcmaVM *        | 是   | 虚拟机对象。                                           |
| buffer     | Local<ArrayBufferRef> | 是   | 一个ArrayBufferRef类型的局部变量，要存储数据的缓冲区。 |
| byteOffset | int32_t               | 是   | 一个32位整数，表示在缓冲区中的字节偏移量。             |
| length     | int32_t               | 是   | 一个32位整数，表示数组的长度。                         |

**返回值：**

| 类型                 | 说明                                          |
| -------------------- | --------------------------------------------- |
| Local<Int32ArrayRef> | 返回一个新创建的Int32ArrayRef对象的本地引用。 |

**示例：**

```c++
const int32_t length = 15;
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
Local<Int32ArrayRef> obj = Int32ArrayRef::New(vm_, arrayBuffer, 5, 6);
```



## ProxyRef : public ObjectRef

该类继承自ObjectRef类，主要用于处理JavaScript代理对象。

### GetTarget

Local<JSValueRef> GetTarget(const EcmaVM *vm);

获取JavaScript代理对象的目标对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型              | 说明                                                         |
| ----------------- | ------------------------------------------------------------ |
| Local<JSValueRef> | 返回一个JSValueRef类型的对象，它表示JavaScript代理对象的目标对象。 |

**示例：**

```c++
JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
JSHandle<JSTaggedValue> hclass(thread_, env->GetObjectFunction().GetObject<JSFunction>());
JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
JSHandle<JSTaggedValue> value(thread_, JSTaggedValue(1));
JSObject::SetProperty(thread_, targetHandle, key, value);
JSHandle<JSTaggedValue> handlerHandle(
    factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread_, targetHandle, handlerHandle);
JSHandle<JSTaggedValue> proxytagvalue = JSHandle<JSTaggedValue>::Cast(proxyHandle);
Local<ProxyRef> object = JSNApiHelper::ToLocal<ProxyRef>(proxytagvalue);
Local<JSValueRef> value = object->GetTarget(vm_);
```

### IsRevoked

bool ProxyRef::IsRevoked();

检查JavaScript代理对象是否已被撤销。

**参数：**

无

**返回值：**

| 类型 | 说明                                                         |
| ---- | ------------------------------------------------------------ |
| bool | 布尔值：true 或 false。如果代理对象已被撤销，返回true。否则，返回false。 |

**示例：**

```c++
JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
JSHandle<JSTaggedValue> hclass(thread_, env->GetObjectFunction().GetObject<JSFunction>());
JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
JSHandle<JSTaggedValue> value(thread_, JSTaggedValue(1));
JSObject::SetProperty(thread_, targetHandle, key, value);
JSHandle<JSTaggedValue> handlerHandle(
    factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread_, targetHandle, handlerHandle);
JSHandle<JSTaggedValue> proxytagvalue = JSHandle<JSTaggedValue>::Cast(proxyHandle);
Local<ProxyRef> object = JSNApiHelper::ToLocal<ProxyRef>(proxytagvalue);
bool b = object->IsRevoked();
```

### GetHandler

Local<JSValueRef> GetHandler(const EcmaVM *vm);

获取对象的处理程序。

**参数：**

| 参数名 | 类型           | 必填 | 说明       |
| ------ | -------------- | ---- | ---------- |
| vm     | const EcmaVM * | 是   | 虚拟机对象 |

**返回值：**

| 类型              | 说明                 |
| :---------------- | :------------------- |
| Local<JSValueRef> | 返回对象的处理程序。 |

**示例：**

```c++
JSHandle<JSTaggedValue> targetHandle;
JSHandle<JSTaggedValue> handlerHandle;
JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread_, targetHandle, handlerHandle);
JSHandle<JSTaggedValue> proxytagvalue = JSHandle<JSTaggedValue>::Cast(proxyHandle);
Local<JSValueRef> result = JSNApiHelper::ToLocal<ProxyRef>(proxytagvalue)->GetHandler(vm_);
```

## EscapeLocalScope：LocalScope

`EscapeLocalScope` 类是一个继承自 `LocalScope` 的具体子类，用于管理局部变量的生命周期。
它提供了一个方法 `Escape`，允许在局部范围内提前返回局部变量的 `Local` 对象。

### EscapeLocalScope

explicit EscapeLocalScope(const EcmaVM *vm);

构造EscapeLocalScope对象。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

无

**示例：**

```c++
EscapeLocalScope scope(vm);
```

## SetIteratorRef：ObjectRef

SetIteratorRef 类是一个继承自 ObjectRef 的具体子类，用于操作 JavaScript Set 对象的迭代器。

### GetIndex

int32_t GetIndex();

获取当前 `SetIteratorRef` 对象关联的 Set 迭代器的当前索引。

通过 `JSNApiHelper::ToJSHandle(this)` 将当前的 `SetIteratorRef` 对象转换为 JavaScript 中的 `JSSetIterator` 对象。

调用 `JSSetIterator` 对象的 `GetNextIndex` 方法获取当前迭代器的索引。

**参数：**

无

**返回值：**

| 类型    | 说明               |
| ------- | ------------------ |
| int32_t | 当前迭代器的索引。 |

**示例：**

```c++
int32_t currentIndex = mySetIterator.GetIndex();
```

### GetKind

Local<JSValueRef> GetKind(const EcmaVM *vm);

获取当前 `SetIteratorRef` 对象关联的 Set 迭代器的迭代类型。

在获取之前，通过 `CHECK_HAS_PENDING_EXCEPTION_RETURN_UNDEFINED` 宏检查是否存在潜在的异常。

 通过 `JSNApiHelper::ToJSHandle(this)` 将当前的 `SetIteratorRef` 对象转换为 JavaScript 中的 `JSSetIterator` 对象。

调用 `JSSetIterator` 对象的 `GetIterationKind` 方法获取迭代类型。

根据迭代类型选择相应的字符串，然后使用 `StringRef::NewFromUtf8` 方法创建一个 `Local<JSValueRef>` 对象，表示字符串值。

**参数：**

| 参数名 | 类型           | 必填 | 说明         |
| ------ | -------------- | ---- | ------------ |
| vm     | const EcmaVM * | 是   | 虚拟机对象。 |

**返回值：**

| 类型              | 说明                         |
| ----------------- | ---------------------------- |
| Local<JSValueRef> | 返回表示迭代类型的字符串值。 |

**示例：**

```c++
Local<JSValueRef> iterationType = mySetIterator.GetKind(myEcmaVM);
```

## Uint8ClampedArrayRef：TypedArrayRef

Uint8ClampedArrayRef 类是一个继承自 TypedArrayRef 的具体子类，用于操作 JavaScript 中的 Uint8ClampedArray 对象。

### New
static Local<Uint8ClampedArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,int32_t length);
构造Uint8ClampedArrayRef对象。

会调用宏定义创建 TypedArray 的通用模板。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                                        |
| ---------- | --------------------- | ---- | ----------------------------------------------------------- |
| vm         | const EcmaVM *        | 是   | 虚拟机对象。                                                |
| buffer     | Local<ArrayBufferRef> | 是   | 表示与 `Uint8ClampedArray` 关联的 ArrayBuffer 对象。        |
| byteOffset | int32_t               | 是   | 表示从 ArrayBuffer 的哪个字节开始创建 `Uint8ClampedArray`。 |
| length     | int32_t               | 是   | 表示要创建的 `Uint8ClampedArray` 的长度。                   |

**返回值：**

| 类型                        | 说明                                     |
| --------------------------- | ---------------------------------------- |
| Local<Uint8ClampedArrayRef> | 返回`Local<Uint8ClampedArrayRef>` 对象。 |



**示例：**

```c++
Local<Uint8ClampedArrayRef> myUint8ClampedArray = Uint8ClampedArrayRef::New(myEcmaVM, myArrayBuffer, Offset, length);
```

## Uint16ArrayRef：TypedArrayRef

Uint16ArrayRef 类是一个继承自 TypedArrayRef 的具体子类，用于操作 JavaScript 中的 Uint16Array 对象。

### New

static Local<Uint16ArrayRef> New(const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset,int32_t length);

通过该函数在 JavaScript 环境中创建一个新的 `Uint16Array` 对象。

**参数：**

| 参数名     | 类型                  | 必填 | 说明                                                  |
| ---------- | --------------------- | ---- | ----------------------------------------------------- |
| vm         | const EcmaVM *        | 是   | 虚拟机对象。                                          |
| buffer     | Local<ArrayBufferRef> | 是   | 表示与 `Uint16Array` 关联的 ArrayBuffer 对象。        |
| byteOffset | int32_t               | 是   | 表示从 ArrayBuffer 的哪个字节开始创建 `Uint16Array`。 |
| length     | int32_t               | 是   | 表示要创建的 `Uint16Array` 的长度。                   |

**返回值：**

| 类型                  | 说明                               |
| --------------------- | ---------------------------------- |
| Local<Uint16ArrayRef> | 返回`Local<Uint16ArrayRef>` 对象。 |

**示例：**

```c++
Local<Uint16ArrayRef> myUint16Array = Uint16ArrayRef::New(myEcmaVM, myArrayBuffer, Offset, length);
```
