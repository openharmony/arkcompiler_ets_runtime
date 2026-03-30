# AGENTS.md - CommonJS Require Guidelines

This file provides guidelines for AI coding agents working on the CommonJS (require) implementation.

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
# 运行所有 CJS Manager 测试
./build.sh --product-name rk3568 --build-target CJS_Manager_TestAction

# 运行单个测试
./build.sh --product-name rk3568 --build-target CJS_Manager_TestAction --gn-args target_test_filters="JS_CJS_ManagerTest.TestName"

# 运行测试文件中的所有测试
./build.sh --product-name rk3568 --build-target CJS_Manager_TestAction --gn-args target_test_filters="JS_CJS_ManagerTest.*"

# 运行 CJS Module 测试
./build.sh --product-name rk3568 --build-target CJS_Module_TestAction
```

**NOTE:** Run all commands from **OpenHarmony root directory**.

## Overview

The `ecmascript/require` directory implements CommonJS-style `require()` functionality for legacy module support in ES6/TypeScript environments.

### Implementation

CommonJS modules use `require()` instead of ES6 `import/export`:
- `CjsModule` - CommonJS module record
- `CjsModuleCache` - Module caching
- `RequireManager` - require() implementation and resolution
- Backward compatibility with Node.js-style modules

## CommonJS Architecture

### Key Classes

| Class | Purpose |
|--------|----------|
| CjsModule | CommonJS module record (module.exports, etc.) |
| CjsModuleCache | LRU cache for loaded modules |
| RequireManager | Central require() implementation |
| JSRequire | Helper for require() operations |
| CJSInfo | CommonJS info structure containing module, require, exports, etc. |
| CjsExports | CommonJS exports object wrapper |

### CommonJS vs ES6 Modules

| Feature | CommonJS | ES6 Modules |
|----------|------------|---------------|
| Import | `require('module')` | `import * as m from 'module'` |
| Export | `module.exports = {}` | `export const value = {}` |
| Loading | Synchronous | Can be async |
| Caching | File-based cache | Module graph cache |
| Dynamic | `require(path)` with variable | `import(path)` with expression |

## Code Style Guidelines

### File Naming

| Type | Pattern | Examples |
|------|-----------|----------|
| CJS module | `js_cjs_module.cpp/h` | `js_cjs_module.cpp` |
| Cache | `js_cjs_module_cache.cpp/h` | `js_cjs_module_cache.cpp` |
| Manager | `js_cjs_require.h` | `js_cjs_require.h` |
| Require manager | `js_require_manager.cpp/h` | `js_require_manager.cpp` |
| Tests | `*_test.cpp` | `js_cjs_module_test.cpp` |

### Class Naming

```cpp
// CommonJS-specific prefix
class CjsModule final : public JSObject { ... };
class CjsModuleCache { ... };
class RequireManager { ... };

// Helper pattern
class JSRequire { ... };
class CjsExports { ... };
```

### Enum Naming

```cpp
// Use CjsModule prefix for module states
enum class CjsModuleStatus : uint8_t { 
    UNLOAD = 0x01, 
    LOADED 
};
```

### CJS Module Structure

```cpp
class CjsModule final : public JSObject {
public:
    CAST_CHECK(CjsModule, IsCjsModule);

    // Layout using ACCESSORS macro
    static constexpr size_t JS_CJS_MODULE_OFFSET = JSObject::SIZE;
    ACCESSORS(Id, JS_CJS_MODULE_OFFSET, ID_OFFSET)
    ACCESSORS(Path, ID_OFFSET, PATH_OFFSET)
    ACCESSORS(Exports, PATH_OFFSET, EXPORTS_OFFSET)
    ACCESSORS(Filename, EXPORTS_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_SIZE)
    DEFINE_ALIGN_SIZE(LAST_SIZE);

    // Status bit field
    static constexpr size_t STATUS_BITS = 2;
    FIRST_BIT_FIELD(BitField, Status, CjsModuleStatus, STATUS_BITS)

    // Module initialization
    static void InitializeModule(JSThread *thread, 
                              JSHandle<CjsModule> &module,
                              JSHandle<JSTaggedValue> &filename, 
                              JSHandle<JSTaggedValue> &dirname);

    // Require operation
    static JSTaggedValue Require(JSThread *thread, 
                                   JSHandle<EcmaString> &request,
                                   JSHandle<CjsModule> &parent,
                                   bool isMain);

    // Module loading
    static JSHandle<JSTaggedValue> Load(JSThread *thread, 
                                             JSHandle<EcmaString> &request);

    // Cache operations
    static JSHandle<JSTaggedValue> SearchFromModuleCache(JSThread *thread, 
                                                          JSHandle<JSTaggedValue> &filename);
    static void PutIntoCache(JSThread *thread, 
                                   JSHandle<CjsModule> &module,
                                   JSHandle<JSTaggedValue> &filename);
};
```

### Require Resolution Pattern

```cpp
// Standard CommonJS require() algorithm
JSHandle<JSTaggedValue> RequireManager::ResolveRequire(JSThread *thread,
                                                     JSHandle<EcmaString> &request) {
    // 1. Check if already loaded
    JSHandle<JSTaggedValue> cached = CjsModule::SearchFromModuleCache(thread, request);
    if (!cached->IsUndefined()) {
        return cached;
    }

    // 2. Resolve file path
    JSHandle<EcmaString> resolvedPath = ResolvePath(thread, request);

    // 3. Load and compile module
    JSHandle<CjsModule> module = CjsModule::Load(thread, resolvedPath);

    // 4. Cache the module
    CjsModule::PutIntoCache(thread, module, resolvedPath);

    // 5. Return module.exports
    return module->GetExports();
}
```

### Path Resolution

CommonJS path resolution follows Node.js semantics:

```cpp
// Resolution priority:
1. Core modules (no '.js' extension) - Node.js built-ins
2. Relative paths (./, ../)
3. Node modules (node_modules/ hierarchy)
4. Absolute paths

// Example resolution patterns:
// require('./local') → ./local.js
// require('../parent') → ../parent.js
// require('npm-package') → node_modules/npm-package/index.js
// require('/absolute/path') → /absolute/path.js
```

### Cache Implementation

```cpp
// LRU cache for CJS modules
class CjsModuleCache {
    // Add module to cache
    static void PutIntoCache(JSThread *thread, 
                                   JSHandle<CjsModule> &module,
                                   JSHandle<JSTaggedValue> &filename);

    // Search cache
    static JSHandle<JSTaggedValue> SearchFromModuleCache(JSThread *thread, 
                                                          JSHandle<JSTaggedValue> &filename);

    // Clear cache (for testing)
    static void ClearCache(JSThread *thread);
};
```

### Module Execution

```cpp
// CommonJS IIFE wrapper pattern
// When a CJS module is loaded, it's wrapped in:
(function(exports, require, module) {
    // Module code here
    module.exports = exports;
})(exports, require, module);

// This ensures:
// - Each module has its own scope
// - exports is mutable (module.exports)
// - require is available to the module
```

## Code Style Guidelines

### Require Function Pattern

```cpp
// Require() returns module.exports
JSHandle<JSTaggedValue> JSRequire::Require(JSThread *thread, 
                                             JSHandle<EcmaString> &request) {
    // 1. Convert request to filename
    JSHandle<EcmaString> filename = RequestToFilename(thread, request);

    // 2. Load module
    JSHandle<JSTaggedValue> moduleValue = CjsModule::Load(thread, filename);

    // 3. Get exports object
    JSHandle<JSObject> exports = JSHandle<JSObject>::Cast(moduleValue);

    // 4. Return exports
    return JSHandle<JSTaggedValue>(exports);
}
```

### Error Handling

```cpp
// Module loading errors
if (module->GetStatus() == CjsModuleStatus::UNLOAD) {
    THROW_TYPE_ERROR(thread, "Module not found: " + filename);
    return JSTaggedValue::Exception();
}

// Cyclic dependency detection
if (IsInDependencyCycle(request)) {
    THROW_TYPE_ERROR(thread, "Detected circular dependency for: " + request);
}
```

### Integration with ES6

```cpp
// CJS modules can be imported in ES6:
// import * as cjsModule from './cjs-module.cjs';
// import { someExport } from './cjs-module.cjs';

// The CJS module is wrapped with default export:
// export default require('./cjs-module.cjs');
```

## Constants

```cpp
// CommonJS-specific constants
static constexpr size_t CJS_CACHE_SIZE = 128;  // Max cached modules
static constexpr uint32_t MAX_PATH_DEPTH = 32;  // Max recursion depth
```

## Module Exports

CommonJS supports multiple export styles:

```javascript
// Style 1: Assign to module.exports
module.exports.myExport = 42;
module.exports.anotherExport = "hello";

// Style 2: Export from IIFE
(function() {
    module.exports = { value: 123 };
})();

// Style 3: Re-assign module.exports
module.exports = { new: value };

// Style 4: Named exports (with transpilation)
export.myExport = 42;
export.default = module.exports;
```

## Cache Coherence

Module cache must maintain:

```cpp
// 1. Filename normalization (resolve absolute paths)
// 2. Same file → same cached module
// 3. Cache invalidation on module reload
// 4. Main module tracking (entry point)
```

## Interop with ES6

```cpp
// When ES6 imports a CJS module:
// 1. Load as CJS module
// 2. Get module.exports object
// 3. Create ES6 namespace from exports
// 4. Return namespace as default export

// When CJS requires an ES6 module:
// 1. Load ES6 module
// 2. Get default export (if any)
// 3. Return default export
// 4. Handle namespace exports (rare)
```
