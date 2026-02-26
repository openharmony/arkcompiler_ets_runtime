# AGENTS.md - NAPI Guidelines

This file provides guidelines for AI coding agents working on the Node-API (NAPI) implementation.

## Build and Test Commands

### 编译参数说明

| **参数名称** | **描述** | **示例** |
| --- | --- | --- |
| `--product-name` | 指定产品名称 | rk3568 |
| `--build-target` | 指定构建编译对象 | ark_js_host_unittest：单元测试
ark_js_host_linux_tools_packages：工具包
ets_frontend_build：前端编译 |
| `--gn-args` | 传递编译参数 | is_debug=true：启用调试模式 |

### 常用编译命令

#### Release 编译

编译发布版本（无调试信息）：

```bash
./build.sh --product-name rk3568 \
           --build-target ark_js_host_unittest \
           --build-target ark_js_unittest \
           --build-target ark_js_host_linux_tools_packages
```

#### Debug 编译

编译调试版本（包含调试信息）：

```bash
./build.sh --product-name rk3568 \
           --build-target ark_js_host_unittest \
           --build-target ark_js_unittest \
           --build-target ark_js_host_linux_tools_packages \
           --gn-args is_debug=true
```

### 运行测试

```bash
# 运行所有 NAPI 测试
./build.sh --product-name rk3568 --build-target Jsnapi_001_TestAction

# 运行单个测试
./build.sh --product-name rk3568 --build-target Jsnapi_001_TestAction --gn-args target_test_filters="JSNApiSampleTest.TestName"

# 运行测试文件中的所有测试
./build.sh --product-name rk3568 --build-target Jsnapi_001_TestAction --gn-args target_test_filters="JSNApiSampleTest.*"

# 运行 sendable 测试
./build.sh --product-name rk3568 --build-target Jsnapi_Sendable_TestAction

# 运行 NAPI sample 测试
./build.sh --product-name rk3568 --build-target JsnapiSampleAction
```

**NOTE:** Run all commands from **OpenHarmony root directory**.

## Overview

The `ecmascript/napi` directory provides a C++ API for interacting with the JavaScript runtime, allowing native modules to be written in C++ and called from JavaScript/TypeScript.

### Documentation

See `ecmascript/napi/README.md` (Chinese) for comprehensive API documentation covering:
- ArrayBufferRef, BufferRef, BooleanRef
- ObjectRef, FunctionRef, NumberRef, StringRef
- Typed array types
- Native pointers and callbacks

## NAPI Architecture

### Key Classes

| Class | Purpose |
|--------|----------|
| JSNApiHelper | NAPI helper class providing conversion between JavaScript values and NAPI values |
| NativeReferenceHelper | Native reference helper class managing native object lifecycle and callbacks |
| Callback | Callback function registration and management |
| JSNApiClassCreationHelper | NAPI class creation helper for creating JavaScript classes and binding native methods |

### Reference Types

| Type | Purpose |
|-------|----------|
| IntegerRef, NumberRef, BooleanRef, StringRef | Primitive value references |
| ObjectRef, ArrayRef, FunctionRef | Object and function references |
| ArrayBufferRef, BufferRef | Buffer and array buffer references |
| Int8ArrayRef, Uint8ArrayRef, etc. | Typed array references |
| SharedInt8ArrayRef, SharedUint8ArrayRef, etc. | Shared typed array references |
| NativePointerRef | Native pointer wrapper for C++ objects |
| SendableArrayBufferRef | Sendable array buffer for cross-thread sharing |

### Error Types

| Type | Purpose |
|-------|----------|
| ErrorRef | Base error type |
| TypeErrorRef, RangeErrorRef, SyntaxErrorRef, ReferenceErrorRef | Specific error types |
| EvalErrorRef, AggregateErrorRef, OOMErrorRef, TerminationErrorRef | Additional error types |

## Code Style Guidelines

### File Naming

| Type | Pattern | Examples |
|------|-----------|----------|
| Core API | `jsnapi.cpp` | Main implementation |
| Helpers | `jsnapi_helper.cpp/h` | Helper functions |
| Expos | `jsnapi_expo.cpp` | API exports |
| DFX | `dfx_jsnapi.cpp` | Debugging features |
| Creation | `jsnapi_class_creation_helper.cpp/h` | Object creation |
| Tests | `jsnapi_*_tests.cpp` | Test files |

### Namespace and Using

```cpp
namespace panda {
// Bring ecmascript types into panda namespace
using ecmascript::ECMAObject;
using ecmascript::EcmaString;
using ecmascript::EcmaVM;
using ecmascript::JSThread;
using ecmascript::JSArray;
using ecmascript::JSObject;
using ecmascript::JSFunction;
using ecmascript::JSTaggedValue;
// ... more using declarations

// Template aliases for NAPI types
template<typename T>
using JSHandle = ecmascript::JSHandle<T>;

template<typename T>
using JSMutableHandle = ecmascript::JSMutableHandle<T>;
}
```

### VM Lifecycle

```cpp
// Create JSVM
RuntimeOption option;
option.SetLogLevel(common::LOG_LEVEL::ERROR);
EcmaVM *vm = JSNApi::CreateJSVM(option);

// Get thread context
JSThread *thread = vm->GetJSThread();

// Use LocalScope for RAII handle management
LocalScope scope(vm);

// Destroy VM
JSNApi::DestroyJSVM(vm);
```

### NAPI Reference Types

All NAPI types are reference-counted handles:

```cpp
// Create primitive references
Local<IntegerRef> intRef = IntegerRef::New(vm, 42);
Local<NumberRef> numRef = NumberRef::New(vm, 3.14);
Local<BooleanRef> boolRef = BooleanRef::New(vm, true);
Local<StringRef> strRef = StringRef::NewFromUtf8(vm, "hello");

// Create object references
Local<ObjectRef> objRef = ObjectRef::New(vm);
Local<ArrayRef> arrRef = ArrayRef::New(vm, 10);
Local<FunctionRef> funcRef = FunctionRef::New(vm, callback);

// Get values from references
int intValue = intRef->Value();
double numValue = numRef->Value();
bool boolValue = boolRef->Value();
std::string strValue = strRef->ToString(vm)->GetUtf8();
```

### Object Property Access

```cpp
Local<ObjectRef> obj = ObjectRef::New(vm);

// Set property
obj->Set(vm, StringRef::NewFromUtf8(vm, "key"), IntegerRef::New(vm, 42));

// Get property
Local<JSValueRef> value = obj->Get(vm, StringRef::NewFromUtf8(vm, "key"));

// Check if property exists
bool hasProperty = obj->Has(vm, StringRef::NewFromUtf8(vm, "key"));

// Delete property
obj->Delete(vm, StringRef::NewFromUtf8(vm, "key"));

// Get all property names
Local<ArrayRef> keys = obj->GetOwnPropertyNames(vm);
```

### Function Callbacks

```cpp
// Define callback signature
using FunctionCallbackInfo = JSHandle<JSTaggedValue>(*)(JsiRuntimeCallInfo *);

// Create native function
JSHandle<JSTaggedValue> MyCallback(JsiRuntimeCallInfo *runtimeCallInfo) {
    EcmaVM *vm = runtimeCallInfo->GetVM();
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    
    // Process arguments
    int32_t argc = runtimeCallInfo->GetArgsNumber();
    for (int i = 0; i < argc; i++) {
        Local<JSValueRef> arg = runtimeCallInfo->GetCallArg(i);
        // Handle argument
    }
    
    // Return value
    return JSHandle<JSTaggedValue>::Cast(IntegerRef::New(vm, 42));
}

// Create FunctionRef
Local<FunctionRef> funcRef = FunctionRef::New(vm, MyCallback);
```

### Array Operations

```cpp
Local<ArrayRef> arr = ArrayRef::New(vm, 10);

// Set element
arr->Set(vm, 0, IntegerRef::New(vm, 100));

// Get element
Local<JSValueRef> elem = arr->Get(vm, 0);

// Get length
uint32_t length = arr->Length(vm);

// Push element
arr->Push(vm, IntegerRef::New(vm, 200));

// Convert to JSValue
JSHandle<JSTaggedValue> value(arr->ToTaggedValue());
```

### Typed Arrays

```cpp
// Create typed array from ArrayBuffer
Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm, 1024);
Local<Int8ArrayRef> int8Array = Int8ArrayRef::New(vm, buffer, 0, 1024);

// Get buffer data
void *data = buffer->GetBuffer();
int8_t *int8Data = static_cast<int8_t *>(data);

// Create with external buffer
NativePointerCallback deleter = [](void *env, void *buffer, void *data) {
    delete[] static_cast<uint8_t *>(buffer);
};
Local<ArrayBufferRef> extBuffer = ArrayBufferRef::New(vm, externalData, 1024, deleter, data);
```

### Native Pointers

```cpp
// Create native pointer
void *nativeData = new MyClass();
NativePointerCallback deleter = [](void *env, void *pointer, void *data) {
    delete static_cast<MyClass *>(pointer);
};

Local<NativePointerRef> nativePtr = NativePointerRef::New(vm, nativeData, deleter, nativeData);

// Get native data
MyClass *obj = static_cast<MyClass *>(nativePtr->GetNativePointerAddr());
```

### Exception Handling

```cpp
// Use CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN macro
JSHandle<JSTaggedValue> MyFunction(JsiRuntimeCallInfo *runtimeCallInfo) {
    EcmaVM *vm = runtimeCallInfo->GetVM();
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    
    // Check for pending exceptions
    if (thread->HasPendingException()) {
        return JSHandle<JSTaggedValue>(thread->GlobalConstants()->GetHandledUndefined());
    }
    
    // Throw exception
    JSValueRef::ThrowTypeError(vm, "Invalid argument");
    
    return value;
}

// Catch exceptions in tests
try {
    // Code that may throw
} catch (const JSException &e) {
    // Handle exception
}
```

### Thread Management

```cpp
// Use ThreadManagedScope for cross-thread calls
{
    ecmascript::ThreadManagedScope managedScope(thread);
    
    // JS operations here
    // Scope ensures proper thread context
}

// Use LocalScope for handle lifecycle
{
    LocalScope scope(vm);
    Local<StringRef> str = StringRef::NewFromUtf8(vm, "test");
    // str is valid within scope
}
// str is out of scope here
```

### Sendable Objects

For shared memory between worker threads:

```cpp
// Create sendable array buffer
Local<ArrayBufferRef> sendableBuffer = SendableArrayBufferRef::New(vm, 1024);

// Mark as sendable
bool isSendable = SendableArrayBufferRef::IsSendable(vm, sendableBuffer);

// Transfer to worker
Local<JSValueRef> transferred = SendableArrayBufferRef::Transfer(vm, sendableBuffer);
```

### Constants

```cpp
// Common constants
static constexpr uint32_t DEFAULT_GC_POOL_SIZE = 256 * 1024 * 1024;
static constexpr size_t DEFAULT_GC_THREAD_NUM = 7;
static constexpr size_t DEFAULT_LONG_PAUSE_TIME = 40;
```

## FFI (Foreign Function Interface)

For calling native functions from JS:

```cpp
// FFI patterns are in ffi_workload.cpp
// Used for high-performance native bindings
```

## DFX (Debugging Features)

Debugging support in `dfx_jsnapi.cpp`:

```cpp
// Profiling, heap tracking, memory leak detection
// Enabled through runtime options
```

## Cross-Thread Callbacks

```cpp
using ConcurrentCallback = void (*)(
    Local<JSValueRef> result, 
    bool success, 
    void *taskInfo, 
    void *data
);

// Register concurrent callback
// Used for async operations
```
