# AGENTS.md - Module System Guidelines

This file provides guidelines for AI coding agents working on the ES6 Module system implementation.

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
# 运行所有 Module 测试
./build.sh --product-name rk3568 --build-target ModuleTestAction

# 运行单个测试
./build.sh --product-name rk3568 --build-target ModuleTestAction --gn-args target_test_filters="EcmaModuleTest.TestName"

# 运行测试文件中的所有测试
./build.sh --product-name rk3568 --build-target ModuleTestAction --gn-args target_test_filters="JS_ModuleManagerTest.*"
```

**NOTE:** Run all commands from **OpenHarmony root directory**.

## Overview

The `ecmascript/module` directory implements ES6 (ECMAScript 2015) module system, including:
- SourceTextModule: ES6 module records
- ModuleManager: Module loading and lifecycle management
- ModuleResolver: Module dependency resolution
- Shared modules across worker threads
- Module namespace and exports handling

## Module System Architecture

### Key Classes

| Class | Purpose |
|--------|----------|
| SourceTextModule | ES6 module record (ECMAScript spec) |
| ModuleManager | Central module lifecycle manager |
| ModuleResolver | Import/export resolution logic |
| SharedModuleManager | Cross-thread module sharing |
| ModuleNamespace | Export name resolution |
| ImportEntry | Import entry record containing module import information |
| LocalExportEntry | Local export entry record |
| IndirectExportEntry | Indirect export entry record |
| StarExportEntry | Star export entry record |
| ResolvedBinding | Resolved binding record |
| ResolvedIndexBinding | Resolved index binding record |
| SendableClassModule | Sendable class module for cross-thread sharing |
| JSSharedModule | Shared module implementation |
| SharedModuleHelper | Shared module helper utility class |

### Module Lifecycle

```
1. Parse → SourceTextModule created
2. Resolve → ModuleResolver resolves dependencies
3. Instantiate → ModuleNamespace created, imports linked
4. Evaluate → Module executed, exports populated
```

## Code Style Guidelines

### File Naming

| Type | Pattern | Examples |
|------|-----------|----------|
| Module classes | `js_module_*.cpp/h` | `js_module_manager.cpp` |
| Helper files | `*.cpp/h` | `module_path_helper.cpp` |
| Test files | `*_test.cpp` | `js_module_manager_test.cpp` |

### Class Naming

```cpp
// Module record classes
class SourceTextModule : public ModuleRecord { ... };
class SharedModule : public ModuleRecord { ... };

// Manager classes
class ModuleManager { ... };
class SharedModuleManager { ... };

// Helper classes
class ModuleResolver { ... };
class ModulePathHelper { ... };
```

### Enum Naming

```cpp
// Module state - use 'Module' prefix
enum class ModuleStatus : uint8_t {
    UNINSTANTIATED,
    INSTANTIATING,
    INSTANTIATED,
    EVALUATING,
    EVALUATING_ASYNC,
    EVALUATED,
    ERRORED
};

// Module types - use 'Module' prefix
enum class ModuleTypes : uint8_t {
    ECMA_MODULE,
    CJS_MODULE,
    JSON_MODULE,
    NATIVE_MODULE,
    OHOS_MODULE,
    APP_MODULE,
    INTERNAL_MODULE,
    UNKNOWN,
    STATIC_MODULE
};

// Loading types
enum class LoadingTypes : uint8_t {
    STABLE_MODULE,
    DYNAMIC_MODULE,
    OTHERS
};
```

### Key Macros

```cpp
// Module manager macros
NO_COPY_SEMANTIC(ModuleManager);
NO_MOVE_SEMANTIC(ModuleManager);

// Accessor macros (for class layout)
ACCESSORS(Id, JS_CJS_MODULE_OFFSET, ID_OFFSET)
ACCESSORS(Path, ID_OFFSET, PATH_OFFSET)
ACCESSORS(Exports, PATH_OFFSET, EXPORTS_OFFSET)

// Bit field definitions
DEFINE_ALIGN_SIZE(LAST_SIZE);
static constexpr uint32_t DEAULT_DICTIONART_CAPACITY = 4;
```

### Atomic Operations

```cpp
// Use atomic for cross-thread module execute mode
std::atomic<ModuleExecuteMode> isExecuteBuffer_ {ModuleExecuteMode::ExecuteZipMode};

ModuleExecuteMode GetExecuteMode() const {
    return isExecuteBuffer_.load(std::memory_order_acquire);
}

inline void SetExecuteMode(ModuleExecuteMode mode) {
    isExecuteBuffer_.store(mode, std::memory_order_release);
}
```

### Module Resolution Pattern

```cpp
// Standard resolution pattern in ModuleManager
JSHandle<SourceTextModule> GetImportedModule(const CString &referencing) {
    // Check cache first
    if (IsModuleLoaded(referencing)) {
        return TryGetImportedModule(referencing);
    }

    // Resolve and load if not in cache
    JSHandle<SourceTextModule> module = ResolveModule(referencing);
    AddResolveImportedModule(referencing, module->GetTaggedValue());
    return module;
}
```

### Namespace Handling

```cpp
// Use CUnorderedMultiMap for namespace entries
using ResolvedMultiMap = CUnorderedMultiMap<CString *, JSHandle<JSTaggedValue>>;

// Iterate exports
resolvedModules_.ForEach([](auto it) {
    CString key = it->first;
    JSTaggedValue module = it->second.Read();
    // Process export
});
```

### Module Constants

```cpp
static constexpr int MODULE_ERROR = 1;
static constexpr int UNDEFINED_INDEX = -1;
static constexpr uint8_t DEREGISTER_MODULE_TAG = 1;
static constexpr uint32_t FIRST_ASYNC_EVALUATING_ORDINAL = 2;
static constexpr uint32_t NOT_ASYNC_EVALUATED = 0;
static constexpr bool SHARED_MODULE_TAG = true;
```

### Public API Methods

Methods exported outside module system use `PUBLIC_API` macro:

```cpp
JSHandle<SourceTextModule> PUBLIC_API HostGetImportedModule(const CString &referencing);
JSTaggedValue PUBLIC_API HostGetImportedModule(void *src);
```

## Module Deregistration

Module deregistration is handled by `js_module_deregister.cpp`:

```cpp
// When modules are deregistered:
1. Remove from resolved modules cache
2. Clean up lazy import arrays
3. Free module-related strings
4. Clear constant pool literals
```

## Shared Module Support

For worker thread module sharing:

```cpp
// Shared module manager handles cross-VM access
class SharedModuleManager {
    JSHandle<JSTaggedValue> GetSharedModule(const CString &key);
    void SetSharedModule(const CString &key, JSHandle<JSTaggedValue> &module);
};
```

## Path Resolution

Module paths are resolved using `module_path_helper.cpp`:

```cpp
// Path types:
- Absolute paths
- Relative paths (./, ../)
- Node module paths (node_modules/)
- OHOS app module paths
```

## Module Snapshot

Module state can be serialized via `module_snapshot.cpp`:

```cpp
// Snapshot format:
1. Module records
2. Module namespaces
3. Export tables
4. Dependency graphs
```

## Error Handling

```cpp
// Module errors use specific exception patterns
if (thread->HasPendingException()) {
    module->SetStatus(ModuleStatus::ERRORED);
    RecordError(thread, module);
}

// Native module failures
JSHandle<NativeModuleFailureInfo> errorInfo =
    NativeModuleFailureInfo::CreateNativeModuleFailureInfo(vm, errorMessage);
```

## Module Logging

```cpp
// Module-specific logging
ModuleLogger::LogModuleLoad(recordName);
ModuleLogger::LogModuleError(recordName, error);
```
