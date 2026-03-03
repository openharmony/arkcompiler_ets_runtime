# ecmascript/builtins

ECMAScript standard library implementation providing all built-in objects and functions (Array, Object, String, Promise, etc.) per ECMA-262 specification.
Primary Language: C++

## Design

Implements ECMAScript 2021 standard library through builtin functions:

```
  Global Object
       │
       ├──▶ Standard Objects
       │     ├── Array, Object, String, Number, Boolean
       │     ├── Map, Set, WeakMap, WeakSet
       │     ├── Promise, AsyncFunction, Generator
       │     ├── Date, RegExp, Symbol, BigInt
       │     └── Proxy, Reflect
       │
       ├──▶ Typed Arrays
       │     ├── Int8Array, Uint8Array, Uint8ClampedArray
       │     ├── Int16Array, Uint16Array
       │     ├── Int32Array, Uint32Array
       │     ├── Float32Array, Float64Array
       │     └── BigInt64Array, BigUint64Array
       │
       ├──▶ Shared Memory (ECMAScript 2021)
       │     └── SharedArrayBuffer
       │
       ├──▶ Sendable (ArkTS Extensions)
       │     ├── SharedArray, SharedMap, SharedSet
       │     └── SharedTypedArray
       │
       ├──▶ Internationalization (ECMA-402)
       │     ├── Intl.Collator, Intl.NumberFormat
       │     ├── Intl.DateTimeFormat, Intl.ListFormat
       │     └── Intl.Segmenter, Intl.DisplayNames
       │
       └──▶ Global Functions
             ├── eval (disabled in strict mode)
             ├── isNaN, isFinite, parseFloat, parseInt
             └── encodeURI, encodeURIComponent, etc.
```

**Key abstractions:**
- **Builtins** — Builtin ID system and registration
- **Builtin functions** — C++ implementations of ECMAScript methods
- **Lazy initialization** — For initializing some builtins for the child thread
- **Sendable objects** — ArkTS extension for cross-thread shared objects (SharedArray, SharedMap, SharedSet, SharedTypedArray)

## Reference Source
- `arkcompiler/ets_runtime/ecmascript/builtins/builtins.h` (1-500行) - Builtin ID system
- `arkcompiler/ets_runtime/ecmascript/builtins/builtins_array.cpp` - Array methods
- `arkcompiler/ets_runtime/ecmascript/builtins/builtins_promise.cpp` - Promise implementation

## Key Files

| File | Role |
|------|------|
| `builtins.h/.cpp` | Builtin ID system and registration |
| `builtins_global.h/.cpp` | Global functions and properties |
| `builtins_array.h/.cpp` | Array methods |
| `builtins_object.h/.cpp` | Object methods |
| `builtins_string.h/.cpp` | String methods |
| `builtins_promise.h/.cpp` | Promise implementation |
| `builtins_regexp.h/.cpp` | RegExp methods |
| `builtins_typedarray.h/.cpp` | TypedArray base |
| `builtins_shared_*.h/.cpp` | Sendable shared objects |
| `builtins_intl/*.h/.cpp` | Internationalization support |
| `shared_builtins.cpp` | Shared builtin implementations |

## Building

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.debug
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ark_js_host_linux_tools_packages
```

## Tests

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.debug unittest   # includes builtins tests
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/builtins/tests:host_unittest
```

## Boundaries

- Follow ECMA-262 and ECMA-402 specifications exactly for standard features.
- ArkTS extensions (SharedArray, SharedMap, SharedSet, SharedTypedArray) are non-standard but documented.
- Sendable objects require thread-safe implementations.
- Intl features depend on ICU library version.
