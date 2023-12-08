## ArrayBufferRef

ArrayBufferRef 是一种通用的、固定长度的原始二进制数据缓冲区。它不能直接读取或操作，需要通过类型数组或 DataView 对象来操作其中的数据。

### New

Local<ArrayBufferRef> ArrayBufferRef::New(const EcmaVM *vm, int32_t length)；

Local<ArrayBufferRef> ArrayBufferRef::New(const EcmaVM *vm, void *buffer, int32_t length, const Deleter &deleter, void *data)；

创建一个ArrayBuffer对象。

**参数：**

| 参数名  | 类型            | 必填 | 说明                        |
| ------- | --------------- | ---- | --------------------------- |
| vm      | const EcmaVM *  | 是   | 虚拟机对象。                |
| length  | int32_t         | 是   | 指定的长度。                |
| buffer  | void *          | 是   | 指定缓冲区。                |
| deleter | const Deleter & | 是   | 删除ArrayBuffer时所作的操作 |
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
Deleter deleter = [](void *buffer, void *data) -> void {
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
Deleter deleter = [](void *buffer, void *data) -> void {
delete[] reinterpret_cast<uint8_t *>(buffer);
    int *currentData = reinterpret_cast<int *>(data);
    delete currentData;
};
Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm, buffer, 10, deleter, data);
uint8_t *getBuffer = reinterpret_cast<uint8_t *>(arrayBuffer->GetBuffer());
```

### ByteLength

int32_t  ArrayBufferRef::ByteLength([[maybe_unused]] const EcmaVM *vm)；

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

Local<BufferRef> BufferRef::New(const EcmaVM *vm, void *buffer, int32_t length, const Deleter &deleter, void *data)

创建一个BufferRef对象。

**参数：**

| 参数名  | 类型            | 必填 | 说明                                               |
| ------- | --------------- | ---- | -------------------------------------------------- |
| vm      | const EcmaVM *  | 是   | 虚拟机对象。                                       |
| length  | int32_t         | 是   | 指定的长度。                                       |
| buffer  | void *          | 是   | 指定缓冲区                                         |
| deleter | const Deleter & | 是   | 一个删除器对象，用于在不再需要缓冲区时释放其内存。 |
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
Deleter deleter = [](void *buffer, void *data) -> void {
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
Deleter deleter = [](void *buffer, void *data) -> void {
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

## FunctionRef

提供方法将函数封装为一个对象，以及对封装函数的调用。

### New

Local<FunctionRef> FunctionRef::New(EcmaVM *vm, FunctionCallback nativeFunc, Deleter deleter, void *data, bool callNapi, size_t nativeBindingsize)；

创建一个新的函数对象。

**参数：**

|      参数名       | 类型             | 必填 | 说明                                                         |
| :---------------: | ---------------- | ---- | ------------------------------------------------------------ |
|        vm         | const EcmaVM *   | 是   | 指定虚拟机对象。                                             |
|    nativeFunc     | FunctionCallback | 是   | 一个回调函数，当JS调用这个本地函数时，将调用这个回调函。     |
|      deleter      | Deleter          | 否   | 一个删除器函数，用于在不再需要`FunctionRef`对象时释放其资源。 |
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

Local<FunctionRef> FunctionRef::NewClassFunction(EcmaVM *vm, FunctionCallback nativeFunc, Deleter deleter, void *data, bool callNapi, size_t nativeBindingsize)；

创建一个新的类函数对象。

**参数：**

|      参数名       | 类型             | 必填 | 说明                                                         |
| :---------------: | ---------------- | ---- | ------------------------------------------------------------ |
|        vm         | const EcmaVM *   | 是   | 指定虚拟机对象。                                             |
|    nativeFunc     | FunctionCallback | 是   | 一个回调函数，当JS调用这个本地函数时，将调用这个回调函。     |
|      deleter      | Deleter          | 否   | 一个删除器函数，用于在不再需要`FunctionRef`对象时释放其资源。 |
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
Deleter deleter = nullptr;
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
