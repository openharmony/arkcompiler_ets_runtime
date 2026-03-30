# ecmascript/base

Foundational utility classes and helper functions providing common abstractions used across the entire runtime: JSON, math, string, number, array, atomic, and GC utilities.
Primary Language: C++

## Design

The base module provides reusable utilities organized by functionality:

```
  Runtime components
       │
       ├──▶ JSON (parse/stringify)
       │     ├── json_parser.cpp (RFC 8259 compliant)
       │     ├── json_stringifier.cpp (optimized serialization)
       │     └── json_helper.h (public API)
       │
       ├──▶ Number/String conversion
       │     ├── dtoa_helper.cpp (double to ASCII)
       │     ├── number_helper.cpp (ECMAScript Number)
       │     └── string_helper.h (string operations)
       │
       ├──▶ Data structure helpers
       │     ├── array_helper.cpp (array operations)
       │     ├── sort_helper.cpp (TimSort, QuickSort)
       │     ├── typed_array_helper.cpp (TypedArray)
       │     └── hash_combine.h (hash functions)
       │
       ├──▶ Low-level utilities
       │     ├── atomic_helper.h (atomic operations)
       │     ├── bit_helper.h (bit manipulation)
       │     ├── aligned_struct.h (memory alignment)
       │     └── gc_helper.h (GC barriers)
       │
       └──▶ Configuration and errors
             ├── config.h (runtime configuration)
             ├── error_helper.h (error creation)
             └── builtins_base.h (builtin utilities)
```

**Key abstractions:**
- **JsonParser** — RFC 8259 compliant JSON parser with streaming support
- **JsonStringifier** — Optimized JSON serializer with replacer/space support
- **DtoaHelper** — Fast double to string conversion (Grisu3 algorithm)
- **TypedArrayHelper** — TypedArray operations (Int8Array, Float64Array, etc.)
- **AtomicHelper** — Atomic operations for shared memory

## Key Files

| File | Role |
|------|------|
| `json_parser.h/.cpp` | JSON parsing implementation |
| `json_stringifier.h/.cpp` | JSON serialization |
| `json_stringifier_optimized.cpp` | Fast path JSON stringification |
| `dtoa_helper.h/.cpp` | Double to ASCII conversion |
| `number_helper.h/.cpp` | Number operations and conversions |
| `string_helper.h` | String utility functions |
| `array_helper.h/.cpp` | Array operations |
| `sort_helper.h/.cpp` | Sorting algorithms (TimSort) |
| `typed_array_helper.h/.cpp` | TypedArray operations |
| `atomic_helper.h/.cpp` | Atomic operations |
| `bit_helper.h` | Bit manipulation utilities |
| `config.h` | Runtime configuration management |
| `error_helper.h/.cpp` | Error object creation |
| `gc_helper.h` | GC barrier helpers |
| `math_helper.h` | Math utility functions |

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
python ark.py x64.debug unittest   # includes base tests
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/base/tests:host_unittest
```

## Boundaries

- Keep utilities generic and reusable; avoid runtime-specific logic.
- JSON must follow RFC 8259 and ECMAScript JSON specification.
- Number conversions must match ECMAScript ToNumber/ToString semantics.
- Atomic operations must be lock-free where possible.
- Do not add VM or Heap dependencies; use GC helpers for barriers only.
