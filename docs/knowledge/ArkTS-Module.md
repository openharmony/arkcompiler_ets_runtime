# ArkTS模块化知识库

模块化是将ArkTS/TS/JS/so模块（一个文件对应一个模块）通过编译工具或运行时机制将这些模块加载、解析、组合并执行的过程。
本文只记录模块加载、模块类型差异、加载场景约束、快照恢复的知识。

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

## 缓存与生命周期

| 缓存/状态 | 存储位置 | 管理方式我感觉 | 分配时机 | 清理触发 |
|----------|---------|-----|---------|---------|
| SourceTextModule | LocalHeap | 由 ModuleManager 管理 | 加载时创建 | 静态模块：线程生命周期；动态模块：ModuleDeregister 触发 GC 后删除 |
| Shared SourceTextModule | SharedHeap | 由 SharedModuleManager 管理 | 首个线程加载时创建 | 进程生命周期，不主动清理 |

## 约束规则

- 快照恢复链路的序列化反序列化只在主线程发生
- 修改模块加载路径时，必须同步验证懒加载、动态加载、快照恢复三条路径
- 共享模块修改时，必须使用 SharedModuleManager 的锁保护，禁止跳过锁直接访问 SourceTextModule 的字段
- 动态加载的模块不可以重入动态加载逻辑
- 不要对静态加载模块执行 ModuleDeregister
- 任何修改都不能对Evaluate内深度遍历文件执行顺序产生影响（会产生不兼容）

## 修改前检查

1. 修改会影响多条加载方式路径，需要分别测试
2. 对模块生命周期进行修改时，需要确认模块 deregister 是否正确清理了 resolvedModules_ 和 lazyImportArrays等相关native内容
3. 动态 import 需要考虑napi接口同步修改
4. 共享模块修改后，考虑多线程并发场景，同一个模块，需要模拟所有的状态位，在两个线程的相应处理需要枚举确认无问题
5. 快照相关修改需要在模块序列化/反序列化路径验证，确认多线程操作无问题

## 代码和测试

### 关键代码入口

| 功能 | 入口文件 |
|------|----------|
| 模块实例化 | `module/js_module_source_text.cpp` SourceTextModule::Instantiate |
| 模块执行 | `module/js_module_source_text.cpp` SourceTextModule::Evaluate |
| 动态 import | `module/js_dynamic_import.cpp` DynamicImport::ExecuteNativeOrJsonModule |
| 模块卸载 | `module/js_module_deregister.cpp` ModuleDeregister |
| 共享模块管理 | `module/js_shared_module_manager.cpp` |
| 共享模块克隆 | `module/js_shared_module.cpp` JSSharedModule::CloneEnvForSModule |
| 模块快照 | `module/module_snapshot.cpp` |
| 模块路径解析 | `module/module_path_helper.cpp` |
| 模块依赖解析 | `module/module_resolver.cpp` |

### 测试

| 测试目标 | 测试类名 | 测试文件 |
|----------|---------|----------|
| 模块管理 | `EcmaModuleTest`、`ModuleManagerTest` | `module/tests/ecma_module_test.cpp`、`module/tests/js_module_manager_test.cpp` |
| 模块快照 | `ModuleSnapshotTest` | `module/tests/module_snapshot_test.cpp` |
| 共享模块 | `JSSharedModuleTest` | `module/tests/js_shared_module_test.cpp` |
| 动态 import | `DynamicImportTest` | `module/tests/js_dynamic_import_test.cpp` |
| 模块命名空间 | `ModuleNamespaceTest` | `module/tests/js_module_namespace_test.cpp` |
| 模块 deregister | `ModuleDeregisterTest` | `module/tests/js_module_deregister_test.cpp` |
| 静态模块 | `StaticModuleLoaderTest`、`StaticModuleProxyTest` | `module/tests/static_module_loader_test.cpp`、`module/tests/static_module_proxy_test.cpp` |
| NAPI 模块 | `NapiModuleLoaderTest` | `module/tests/napi_module_loader_test.cpp` |