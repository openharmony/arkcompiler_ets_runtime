# ArkTS Sendable 知识

本文只记录 `arkcompiler/ets_runtime` 仓内 Sendable 的知识，涉及 Sendable 对象、Sendable class/function/env、Sendable 并发容器/共享容器、NAPI 暴露、共享内存、编译/解释器入口和测试锚点。

## Sendable背景
- ArkTS提供了Sendable对象类型，Sendable共享对象分配在共享堆中，用于实现Sendable数据在不同并发实例间的引用传递。
- 共享堆（SharedHeap）是进程级别的堆空间，与虚拟机本地堆（LocalHeap）不同，LocalHeap仅限单个并发实例访问，而SharedHeap可被所有线程访问。Sendable对象的跨线程行为为引用传递，因此，一个Sendable对象可能被多个并发实例引用。判断该Sendable对象是否存活，取决于所有并发实例是否存在对此Sendable对象的引用。
- 各个并发实例的LocalHeap是隔离的。SharedHeap是进程级别的堆，可以被所有并发实例共享，但SharedHeap不能引用LocalHeap中的对象。

## 核心模型和主链路

本节同时承担领域内路由：先按“触发词或任务”定位所属分域，再看运行时主链路、优先入口和必须保持的不变式。这里的入口只指向 `ets_runtime` 代码；应用层封装和 API 使用说明只作为术语背景。

### Sendable 基础能力

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
| --- | --- | --- | --- |
| @Sendable、use sendable、DefineSendableClass、Sendable class/function 创建 | class literal 以 `ClassKind::SENDABLE` 进入定义流程；运行时读取 class literal 和 field literal，创建共享 constructor、prototype、instance HClass，并把字段类型写入属性元数据 | `ecmascript/stubs/runtime_stubs-inl.h`、`ecmascript/jspandafile/program_object.cpp`、`ecmascript/jspandafile/class_info_extractor.*`、`ecmascript/shared_object_factory.cpp` | `ClassKind::SENDABLE`、共享 HClass、prototype/constructor、`SharedFieldType` 要同步维护，不能只改其中一处 |
| Sendable 属性类型、字段写入 | 共享对象由共享 HClass 和共享堆分配共同确认身份；属性写入读取 `PropertyAttributes` 中的 `SharedFieldType`，慢路径、fast path、stub 路径都要保持同一套校验语义 | `ecmascript/property_attributes.h`、`ecmascript/js_object.cpp`、`ecmascript/object_fast_operator-inl.h`、`ecmascript/compiler/stub_builder*.{h,cpp}`、`ecmascript/shared_objects/js_shared_object.h` | 不要丢失 `SharedFieldType`；共享身份和字段类型校验必须在 C++ 慢路径、对象 fast path、编译 stub 中一致 |
| SendableEnv、sendable function/module、共享模块 | Sendable method/function 使用共享函数对象；模块记录保存 `SendableEnv`，Sendable 函数跨模块取值时走专用 module accessor 回到真实模块记录 | `ecmascript/sendable_env.h`；共享模块相关知识见 `docs/knowledge/ArkTS-Module.md` | 跨模块取值必须回到真实模块记录读取 `SendableEnv`，不要退化成普通 `LexicalEnv` 或普通 module accessor |
| SharedHeap、shared memory | 共享堆对象不能引用 LocalHeap，对共享内存的创建、加载和释放要走现有 manager | 共享内存/Shared GC 相关知识见 `docs/knowledge/ArkTS-GC.md` | native memory usage 和 SharedHeap 引用边界要同步维护 |

### Sendable 并发/共享容器

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
| --- | --- | --- | --- |
| Sendable 内建初始化、全局挂载、JS 全局名 `Sendable*` | VM 初始化阶段创建 `SharedObject`、`SharedFunction`、`SendableArray`、`SendableMap`、`SendableSet`、`SendableArrayBuffer`、共享 TypedArray、BitVector 的共享 HClass、构造函数和原型，并挂到共享内建环境 | `ecmascript/builtins/shared_builtins.cpp`、`ecmascript/global_env_fields.h`、`ecmascript/global_env_constants.*` | JS 暴露名、prototype、constructor、JSType、global env 槽位要成组维护；BitVector 的文件路径和初始化入口不同，也不能漏掉 |
| SendableArray/Map/Set、共享 TypedArray、BitVector、并发修改异常 | 容器 public API 进入 `ConcurrentApiScope`；对象内 `ModRecord` 区分读者计数和写者占用，冲突时 fail-fast 并抛 `CONCURRENT_MODIFICATION_ERROR`，runtime 层不提供阻塞锁 | `ecmascript/builtins/builtins_shared_*`、`ecmascript/shared_objects/js_shared_*`、`ecmascript/containers/containers_bitvector.*`、`ecmascript/js_api/js_api_bitvector.*`、`ecmascript/shared_objects/concurrent_api_scope.h`、`ecmascript/compiler/builtins/builtins_shared_*`、`ecmascript/compiler/builtins/linked_hashtable_stub_builder.*` | 读写 API 必须保持 fail-fast 语义，不要绕过 `ConcurrentApiScope` 或手写 `ModRecord` |
| SendableArrayBuffer、共享 TypedArray buffer | `JSSendableArrayBuffer` 持有 native pointer 和 byteLength，detach 后 data 置 null、长度置 0；typed array helper 单独处理 `JS_SENDABLE_ARRAY_BUFFER` | `ecmascript/shared_objects/js_sendable_arraybuffer.*`、`ecmascript/builtins/builtins_sendable_arraybuffer.*`、`ecmascript/base/typed_array_helper.*` | `SendableArrayBuffer` 是共享容器，但不是 `SharedArrayBuffer`；detach、构造、typed array buffer 判断不能互用 |

### Sendable 序列化和 Native

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
| --- | --- | --- | --- |
| ASON、Sendable JSON、`TransformType::SENDABLE` | `TransformType::SENDABLE` 路径生成共享对象并同步字段类型；它复用 JSON/ASON 解析和序列化框架，但数组、reviver、属性扩展语义不能直接套普通 JSON | `ecmascript/base/json_parser.cpp`、`ecmascript/base/json_stringifier*.cpp`、`ecmascript/builtins/builtins_json.cpp` | 生成对象时要带共享对象语义和字段类型；普通 JSON 的数组和 reviver 语义不能原样放开 |
| Native Sendable object/function/class、NAPI Sendable 容器、Sendable 符号导出 | Native API 可创建 sendable object/function/class、sendable array/map/set/buffer/typed array；NAPI 属性描述需要转换为共享字段类型 | `ecmascript/napi/include/jsnapi_expo.h`、`ecmascript/napi/jsnapi_expo.cpp`、`ecmascript/jsnapi_sendable.*`、`libark_jsruntime.map` | 公共 NAPI、字段类型转换、`libark_jsruntime.map` 导出要同步；不要让 Native 入口绕过 Sendable 值校验 |


## 边界和分类

| 概念 | 本仓实现 | 不要混用 |
| --- | --- | --- |
| Sendable object/class | `JS_SHARED_OBJECT`、`JS_SHARED_FUNCTION`、`JS_SHARED_ASYNC_FUNCTION`、共享 HClass | 普通 `JSObject`、普通 `JSFunction`、普通 class literal |
| SendableEnv | `SendableEnv` 是 `TaggedArray`，保留 parent env 和 scope info 槽位 | 普通 `LexicalEnv`、`SFunctionEnv`；`RuntimeLdSendableClass()` 只从 `SFunctionEnv` 取 constructor |
| Sendable 共享/并发容器 | `SendableArray/Map/Set`、共享 TypedArray、SendableArrayBuffer、BitVector；底层多为 `JS_SHARED_*` 或 `JS_SENDABLE_ARRAY_BUFFER` | EcmaScript builtins提供的标准容器并非Sendable；`SendableArrayBuffer` 不要和 ECMAScript `SharedArrayBuffer` 混用 |
| Sendable JSON / ASON | `TransformType::SENDABLE` 创建共享对象并加字段类型 | 普通 `JSON.parse/stringify` 的结构化对象 |
| NAPI Sendable | `ObjectRef::NewSWithProperties`、`FunctionRef::NewSendableClassFunction`、`SendableArrayRef/MapRef/SetRef/...` | 普通 `ObjectRef::New`、普通 `FunctionRef::NewClassFunction` |

## 约束规则

- 必须使用共享对象工厂和共享 HClass 路径创建 Sendable 对象：`NewSEcmaHClass`、`NewSharedOldSpaceJSObject`、`NewSFunction*`、`NewSJSNativePointer` 等。
- 不要把普通堆对象挂到共享对象字段上。
- 必须保留 `JSHClass::IsJSShared`、JSType、prototype、layout、instance HClass 的一致性；Sendable class 的 prototype 和 constructor HClass 初始化后不要随意改成 extensible。
- 必须保留 `SharedFieldType`。新增字段、字典模式转换、layout 拷贝、JSON/ASON 生成对象、NAPI 属性描述都要写入或传递字段类型。
- 不要绕过 `ClassHelper::MatchFieldType()`、`JSTaggedValue::IsSharedType()`、`ObjectFastOperator`、`StubBuilder` 中已有的共享值校验；C++ 慢路径和编译快路径必须保持一致。
- 不要允许非 Sendable class 继承 Sendable class，也不要允许 Sendable class 继承非 Sendable class；运行时错误消息在 `message_string.h` 中已有公共语义。
- 不要把 Sendable class literal 当普通 class literal 处理；`ConstantPool::GetClassLiteralFromCache()` 的 sendable 分支会传入 `sendableEnv` 并创建 `SClassLiteral`。
- 并发容器的读写 API 必须进入 `ConcurrentApiScope` 或等价的现有 stub helper；不要直接读写 `ModRecord`，不要改变 `MOD_RECORD_OFFSET` 布局，原因是编译 stub 和 C++ RAII 都依赖该偏移。
- `ConcurrentApiScope` 的写路径要求无读者/写者，读路径要求无写者；失败时抛并发修改异常。不要为了吞异常或提高吞吐降低 acquire-release/atomic 语义。
- Sendable 共享/并发容器中的 `SendableArrayBuffer` 要在 `Attach/Detach` 时同步 native memory usage 和 data/byteLength；不要把 `ArrayBuffer`、`SharedArrayBuffer`、`SendableArrayBuffer` 的 detach 判断互用。
- 实现并发容器的IR实现时，必须要与C++实现保持兼容。
- `TransformType::SENDABLE` 的 JSON/ASON 路径只适合生成 Sendable 结构；不要把普通 JSON 的数组、reviver、属性扩展语义直接套用。

## 状态和生命周期

| 状态对象 | 推荐维护点 | 清理或失效触发 |
| --- | --- | --- |
| Sendable class literal | `ConstantPool` + `sendableEnv` + `ClassKind::SENDABLE` | 同一 VM/多 VM 重复 define 时不要 trim 原 literal array |
| 共享对象字段类型 | `PropertyAttributes::SharedFieldTypeField` 或 `DictSharedFieldTypeField` | layout 转字典、字典转 layout、JSON/ASON/NAPI 创建对象时必须同步 |
| Sendable 共享/并发容器状态 | 对象内 `ModRecord`；`SendableArrayBuffer` 使用 `ArrayBufferData`、`ArrayBufferByteLength`、native pointer allocator data | `ConcurrentApiScope` 析构时恢复并发读写状态；`SendableArrayBuffer` 在 `Detach()` 后 data=null、length=0，transfer 场景同步 native memory usage |

## 修改前检查

- 改 Sendable class/function 前，需要同时检查解释器、baseline、AOT/JIT slowpath、runtime stub 和 `ClassInfoExtractor`
- 改共享字段或属性写入前，检查普通慢路径、fast path、dictionary mode、layout mode 都能取到相同 `SharedFieldType`
- 改共享堆或 GC 相关路径前，需要证明 SharedHeap 不引用 LocalHeap，并跑共享对象工厂/共享容器测试
- 改 Sendable 共享/并发容器 API 前，每个 public 方法是否有读/写 `ConcurrentApiScope`，并且异常时不会留下写锁或读计数；涉及 `SendableArrayBuffer` 或 typed array 时需要区分 `JS_SENDABLE_ARRAY_BUFFER` 与 `JS_SHARED_ARRAY_BUFFER`，并覆盖 detached buffer 分支
- 改 JSON/ASON Sendable 前，需要保留了 `TransformType::SENDABLE` 的数组/reviver/字段类型限制
- 改 NAPI Sendable 前，`jsnapi_expo.h` 公开 API、`libark_jsruntime.map` 符号导出、`JSNapiSendable` 字段类型需要同步

## 代码和测试

### 代码锚点

| 场景 | 代码路径 |
| --- | --- |
| 全局 Sendable builtins 初始化 | `ecmascript/builtins/shared_builtins.cpp`、`ecmascript/global_env_fields.h`、`ecmascript/global_env_constants.*` |
| Sendable class/function/env | `ecmascript/stubs/runtime_stubs-inl.h`、`ecmascript/interpreter/slow_runtime_stub.*`、`ecmascript/sendable_env.h`、`ecmascript/js_function.cpp` |
| 类字面量和字段类型 | `ecmascript/jspandafile/class_info_extractor.*`、`ecmascript/jspandafile/program_object.cpp`、`ecmascript/property_attributes.h` |
| 共享对象模型 | `ecmascript/shared_objects/`、`ecmascript/shared_object_factory.cpp`、`ecmascript/js_hclass.*`、`ecmascript/object_factory.*`、`ecmascript/js_object.*` |
| 模块访问 | `ecmascript/module/js_module_source_text.h`、`ecmascript/module/module_value_accessor.*`、`ecmascript/module/js_shared_module_manager.*` |
| 共享内存管理 | `ecmascript/shared_mm/shared_mm.*`、`ecmascript/mem/`、`common_components/heap/` 中共享堆相关路径 |
| Sendable 共享/并发容器 | `ecmascript/builtins/builtins_shared_array.*`、`ecmascript/builtins/builtins_shared_map.*`、`ecmascript/builtins/builtins_shared_set.*`、`ecmascript/builtins/builtins_shared_typedarray.*`、`ecmascript/builtins/builtins_sendable_arraybuffer.*`、`ecmascript/shared_objects/js_shared_*`、`ecmascript/shared_objects/js_sendable_arraybuffer.*`、`ecmascript/base/typed_array_helper.*`、`ecmascript/containers/containers_bitvector.*`、`ecmascript/js_api/js_api_bitvector.*` |
| 并发容器访问控制 | `ecmascript/shared_objects/concurrent_api_scope.h`、`ecmascript/compiler/builtins/builtins_shared_*_stub_builder.*` |
| JSON / ASON | `ecmascript/base/json_parser.cpp`、`ecmascript/base/json_stringifier*.cpp`、`ecmascript/builtins/builtins_json.cpp` |
| NAPI | `ecmascript/napi/include/jsnapi_expo.h`、`ecmascript/napi/jsnapi_expo.cpp`、`ecmascript/jsnapi_sendable.*` |

### 测试锚点

| 变更类型 | 最小测试锚点 |
| --- | --- |
| Sendable class/function/env/module | `test/sharedtest/{sendable,definesendableclass,sendablefunc,sendableenv,sendablecontext,sendableobj}`，以及 `test/moduletest/`、`test/aottest/` 下同名用例 |
| C++ shared object/builtins | `ecmascript/tests/{js_shared_array_test.cpp,shared_object_factory_test.cpp}`、`ecmascript/builtins/tests/builtins_shared_*_test.cpp` |
| 编译快路径 / AOT / JIT | `test/aottest/{sendable,definesendableclass,sendableenv,sendablefunc,sendablecontext,sendableobj,sharedarray,sharedmap,sharedset,sharedtypedarray}`、`test/jittest/sendable` |
| Sendable import/lazy module | `test/moduletest/sendableclassuseimport`、`test/moduletest/dynamicimportsharedmodule`、`test/moduletest/sharedmodule`、`test/aottest/shared_module` |
| Sendable 共享/并发容器 | `test/sharedtest/{sharedarray,sharedmap,sharedset,sharedset_extern,sharedtypedarray,sharedcollectionsexception,sharedic,sharedbitvector}`、`ecmascript/tests/js_sendable_arraybuffer_test.cpp`、`ecmascript/builtins/tests/builtins_sendable_arraybuffer_test.cpp`、`ecmascript/containers/tests/containers_bitvector_test.cpp`、`ecmascript/tests/js_api_bitvector*_test.cpp`、相关 `test/fuzztest/containersbitvector*_fuzzer`；GN target `Sendable_BuiltinsSharedArray_Test`、`Sendable_BuiltinsSharedArray_extra_Test`、`Sendable_BuiltinsSharedMap_Test`、`Sendable_BuiltinsSharedSet_Test`、`Sendable_BuiltinsSharedTypedArray_Test`、`Sendable_JsSendableArrayBuffer_Test`、`Sendable_BuiltinsSendableArrayBuffer_Test`、`JS_APIBitVector_Test`、`JS_APIBitVectorIterator_Test` |
| JSON / ASON Sendable | `ecmascript/base/tests/{json_parser_test.cpp,json_stringifier_test.cpp,ason_test.cpp}`、`test/sharedtest/sharedJSON` |
| NAPI Sendable | `ecmascript/napi/test/jsnapi_sendable_tests.cpp`，GN target `Jsnapi_Sendable_Test` |

构建从 OpenHarmony 根目录执行。按变更选择 GN target，例如 `Sendable_BuiltinsSharedArray_Test`、`Sendable_BuiltinsSharedArray_extra_Test`、`Sendable_BuiltinsSharedMap_Test`、`Sendable_BuiltinsSharedSet_Test`、`Sendable_BuiltinsSharedTypedArray_Test`、`Sendable_BuiltinsSendableArrayBuffer_Test`、`Sendable_JsSendableArrayBuffer_Test`、`Sendable_JsSharedArray_Test`、`Sendable_SharedObjectFactory_Test`、`JS_APIBitVector_Test`、`JS_APIBitVectorIterator_Test`、`Jsnapi_Sendable_Test`；涉及公共 NAPI 时还要检查 `libark_jsruntime.map` 导出。
