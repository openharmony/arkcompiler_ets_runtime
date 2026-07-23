# ArkTS模块化知识库

模块化是将ArkTS/TS/JS/so模块（一个文件对应一个模块）通过编译工具或运行时机制将这些模块加载、解析、组合并执行的过程。
本文只记录模块加载、模块类型差异、加载场景约束、快照恢复的知识。Sendable 函数、模块环境与跨线程取值见 `docs/knowledge/ArkTS-Sendable.md`。

## 设计目的

模块化系统实现 ArkTS 运行时的**模块加载、依赖解析、执行管理**全流程，遵循 ECMA-262 规范中 Module Record 的定义。

| 目标 | 说明 |
|------|------|
| 规范遵循 | SourceTextModule 的 Resolve/Instantiate/Evaluate 遵循 ECMA-262 规范 |
| 多格式支持 | 统一 ModuleManager 管理 ets/js/ts/json/so 等多种文件格式的加载 |
| 多加载模式 | 支持静态加载、动态加载（import()）、懒加载、NAPI 加载四种模式，通过 ExecuteTypes 区分 |
| 跨线程共享 | 通过 SharedTypes 区分普通模块和共享模块，实现sendable对象的共享 |
| 快照加速 | 通过 JSPandaFileSnapshot + ConstPoolSnapshot + ModuleSnapshot 三层快照，持久化首次启动解析结果，二次启动直接反序列化恢复 |
| 模块卸载 | 仅动态加载模块可卸载（ModuleDeregister），卸载时必须清理 resolvedModules_、lazyImportArrays、ConstantPool literals |

## 核心模型

### 模块加载链路

```
  → JSPandaFileManager::LoadJSPandaFile()       // 加载 abc 文件，生成 JSPandaFile
  → SourceTextModule::Instantiate()          // 链接 import/export
  → SourceTextModule::Evaluate()             // 执行模块代码
```

### 快照恢复链路（二次启动）

```
模块反序列化流程（JSNApi::ModuleDeserialize）
pandafile反序列化流程（JSPandaFileManager::UseSnapshot）
```

### 共享模块多线程执行时序

共享模块记录及其状态可跨线程复用，普通依赖模块的 `resolvedModules_` 缓存则属于当前线程。Thread 2 看到共享模块 A 已是 `INSTANTIATED` 时会跳过 Instantiate，但本线程可能尚未解析 A 的普通依赖 B。因此依赖取数必须支持“缓存查询 + 兜底 resolve + 写回缓存”。

```
Thread 1 (主线程):
  Instantiate:  resolve A + B → A 加入 resolvedSharedModules_, B 加入 resolvedModules_ → 标记 A/B INSTANTIATED

Thread 2 (子线程):
  Instantiate:  在 resolvedSharedModules_ 找到 A，状态为 INSTANTIATED → 直接返回（不再 resolve B）
  Evaluate:     取依赖 B 时 requestedModules[idx] 未命中 →
                HostResolveImportedModule 重新 resolve B →
                SetRequestedModules 写回 requestedModules[idx]
```

### 共享模块 re-export 二次解析

共享模块 A 依赖普通模块 B，而 B 通过 `export *` re-export 模块 C 时，Thread 2 不仅可能缺少 B，也可能缺少 B 的间接依赖 C。`GetExportedNames`、`ResolveExport`、`ResolveIndirectExport` 等 re-export 调用点必须同样使用缓存兜底解析，不能只修复 Evaluate 的直接依赖路径。

### Native 模块加载链路

`LoadNativeModule` 通过全局 `requireNapi` / `requireNativeModule` 加载 APP_MODULE、OHOS_MODULE、INTERNAL_MODULE、NATIVE_MODULE。当前 `LoadNativeModuleCallFunc` 使用 `JSFunction::Call`，同时显式保留 `FunctionRef::Call` 的关键调用语义：`FunctionCallScope` 在顶层调用退出时执行 Promise pending jobs，并配套跨线程与异常检查、调试器状态维护和 kept objects 清理。重构时必须保持这些语义，不能裸调用 `JSFunction::Call`。

## 边界与身份

### 多种加载场景的差别

| 加载方式 | ExecuteTypes | LoadingTypes | 特点 | 常见误用 |
|---------|-------------|--------------|------|---------|
| 静态加载 | `STATIC` | `STABLE_MODULE` | 应用启动时由框架自动加载，走完整的 Instantiate→Evaluate 流程 | - |
| 动态加载 | `DYNAMIC` | `DYNAMITC_MODULE` | 通过 `import()` 语法触发，DynamicImport 类处理，支持卸载 | 不要在动态加载回调中重入动态加载逻辑，修改时要注意napi接口的条件同步修改 |
| 懒加载 | `LAZY` | `STABLE_MODULE` | 延迟到首次使用导出内容时才执行模块代码，通过模块状态标记模块是否已执行 | 只有未执行完成的模块需要重新进入Evaluate逻辑 |
| NAPI 加载 | `NAPI` | `STABLE_MODULE` | 通过napi接口实现模块加载 | 接口返回值处理要注意兼容性 |
| 模块卸载 | `DYNAMIC` | `DYNAMITC_MODULE` | 仅动态加载模块可卸载。ModuleDeregister 清理 resolvedModules_、lazyImportArrays、ConstantPool literals | 不要对静态加载模块执行卸载（ModuleDeregister::InitForDeregisterModule 会检查 LoadingTypes） |

### 多种模块的差异

| 模块类型 | SharedTypes | 使能方式 | 内存位置 | 关键约束 |
|---------|------------|---------|---------|---------|
| 共享模块 | `SHARED_MODULE` | ets 文件使用 `"use shared"` 声明 | SharedHeap | 跨线程共享，修改时必须注意锁和时序问题。通过 SharedModuleManager 管理 |
| Sendable 模块 | `SENDABLE_FUNCTION_MODULE` | 给 SendableFunction 挂载使用 | LocalHeap | 只有部分域有值。克隆时通过 JSSharedModule::CloneEnvForSModule 处理 |
| 普通模块 | `UNSENDABLE_MODULE` | 默认模块类型 | LocalHeap | 无特殊限制，走标准模块化流程 |

### ModuleStatus 状态

```
UNINSTANTIATED → PREINSTANTIATING → INSTANTIATING → INSTANTIATED
                                                       ↓
                                                  EVALUATING → EVALUATING_ASYNC → EVALUATED
                                                       ↓
                                                     ERRORED
```

- `EVALUATING_ASYNC`：异步模块（顶层 await）的中间状态
- 任何阶段出错进入 `ERRORED`
- 遵循 ECMA 规范，模块状态只能前面的状态进入后面的状态，不能反向转换。

### RequestedModule 取数路径差异

| 函数 | 触发 resolve | 适用阶段 | 缓存未命中时行为 | 常见误用 |
|------|--------------|---------|-----------------|---------|
| `GetModuleFromCacheOrResolveNewOne` | 是（兜底） | Instantiate、Evaluate、Debugger、Deregister、re-export、ModuleValueAccessor | 走 `ModuleResolver::HostResolveImportedModule` 重新 resolve，成功后通过 `SetRequestedModules` 写回 `requestedModules[idx]` | 继续使用结果前必须传播 pending exception；不要绕过 `SetRequestedModules`：仅 normal → normal 保存 module 引用，任一端为 shared 时保存 recordName |

历史函数：`GetRequestedModuleFromCache`（纯缓存版，已删除）已被合并到 `GetModuleFromCacheOrResolveNewOne` 中，不要再保留或新增纯缓存分支。

## 缓存与生命周期

| 缓存/状态 | 存储位置 | 管理方式 | 分配时机 | 清理触发 |
|----------|---------|-----|---------|---------|
| SourceTextModule | LocalHeap | 由 ModuleManager 管理 | 加载时创建 | 静态模块：线程生命周期；动态模块：ModuleDeregister 触发 GC 后删除 |
| Shared SourceTextModule | SharedHeap | 由 SharedModuleManager 管理 | 首个线程加载时创建 | 进程生命周期，不主动清理 |

## 约束规则

- 快照恢复链路的序列化反序列化只在主线程发生
- 修改模块加载路径时，必须同步验证懒加载、动态加载、快照恢复三条路径
- 读写共享模块映射时，必须通过 SharedModuleManager 的同步接口，禁止绕过管理器锁直接修改共享模块映射
- 动态加载的模块不可以重入动态加载逻辑
- 不要对静态加载模块执行 ModuleDeregister
- 不得改变 Evaluate 对依赖图的深度优先执行顺序，避免产生模块执行时序不兼容
- Instantiate、Evaluate、Debugger、Deregister、re-export、ModuleValueAccessor 取依赖模块时，必须使用 `GetModuleFromCacheOrResolveNewOne`，禁止恢复纯缓存分支。兜底成功后必须通过 `SetRequestedModules` 写回，并在继续使用结果前传播 pending exception
- 共享模块依赖加载修改后，必须验证主线程完成 Instantiate、子线程直接 Evaluate 的场景，覆盖直接依赖和 re-export 间接依赖的兜底解析与缓存写回
- `LoadNativeModuleCallFunc` 必须保持与 `FunctionRef::Call` 等价的顶层调用语义；使用 `JSFunction::Call` 时必须保留 `FunctionCallScope` 及配套的异常、调试器和资源清理逻辑，确保 native 模块加载返回前执行 pending jobs

## 修改前检查

1. 修改会影响多条加载方式路径，需要分别测试
2. 对模块生命周期进行修改时，需要确认模块 deregister 是否正确清理了 resolvedModules_ 和 lazyImportArrays等相关native内容
3. 动态 import 需要考虑napi接口同步修改
4. 共享模块依赖或 re-export 逻辑修改后，验证主线程完成 Instantiate、子线程直接 Evaluate 时，直接依赖与间接依赖均能兜底解析、正确写回，并传播解析异常
5. 快照相关修改需要在模块序列化/反序列化路径验证，确认多线程操作无问题
6. 修改 Native 模块调用方式或入参构造时，验证动态加载执行顺序不变，且 Promise pending jobs 在 native 模块加载返回前执行

## 代码和测试

### 关键代码入口

| 功能 | 入口文件 |
|------|----------|
| 模块实例化 | `ecmascript/module/js_module_source_text.cpp` SourceTextModule::Instantiate |
| 模块执行 | `ecmascript/module/js_module_source_text.cpp` SourceTextModule::Evaluate |
| 动态 import | `ecmascript/module/js_dynamic_import.cpp` DynamicImport::ExecuteNativeOrJsonModule |
| 模块卸载 | `ecmascript/module/js_module_deregister.cpp` ModuleDeregister |
| 共享模块管理 | `ecmascript/module/js_shared_module_manager.cpp` |
| 共享模块克隆 | `ecmascript/module/js_shared_module.cpp` JSSharedModule::CloneEnvForSModule |
| 模块快照 | `ecmascript/module/module_snapshot.cpp` |
| 模块路径解析 | `ecmascript/module/module_path_helper.cpp` |
| 模块依赖解析 | `ecmascript/module/module_resolver.cpp` |
| Native 模块加载与调用语义 | `ecmascript/module/js_module_source_text.cpp` SourceTextModule::LoadNativeModuleImpl / LoadNativeModuleCallFunc |
| 依赖模块缓存、兜底解析与写回 | `ecmascript/module/js_module_source_text.cpp` SourceTextModule::GetModuleFromCacheOrResolveNewOne / SetRequestedModules |

### 测试

| 测试目标 | 测试类名 | 测试文件 |
|----------|---------|----------|
| 模块管理 | `EcmaModuleTest`、`ModuleManagerTest` | `ecmascript/module/tests/ecma_module_test.cpp`、`ecmascript/module/tests/js_module_manager_test.cpp` |
| 模块快照 | `ModuleSnapshotTest` | `ecmascript/module/tests/module_snapshot_test.cpp` |
| 共享模块 | `JSSharedModuleTest` | `ecmascript/module/tests/js_shared_module_test.cpp` |
| 动态 import | `DynamicImportTest` | `ecmascript/module/tests/js_dynamic_import_test.cpp` |
| 模块命名空间 | `ModuleNamespaceTest` | `ecmascript/module/tests/js_module_namespace_test.cpp` |
| 模块 deregister | `ModuleDeregisterTest` | `ecmascript/module/tests/js_module_deregister_test.cpp` |
| 静态模块 | `StaticModuleLoaderTest`、`StaticModuleProxyTest` | `ecmascript/module/tests/static_module_loader_test.cpp`、`ecmascript/module/tests/static_module_proxy_test.cpp` |
| NAPI 模块 | `NapiModuleLoaderTest` | `ecmascript/module/tests/napi_module_loader_test.cpp` |

上述测试是通用入口，不等同于历史问题的回归证据。修改共享模块依赖解析或 Native 调用语义时，必须新增或引用能直接验证对应时序的专项用例；若只能通过 TaskPool 应用回归，需记录应用路径、执行步骤和通过标准。
