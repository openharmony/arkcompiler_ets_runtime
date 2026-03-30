# ecmascript/ic

Inline cache system for optimizing property access by caching lookup results and avoiding repeated prototype chain traversals.
Primary Language: C++

## Design

Implements inline cache with three kinds of states for fast property access:

```
  Property access: obj.prop
        │
        ▼
   IC lookup (ic_runtime.cpp)
        │
        ├──▶ Mono IC (monomorphic)
        │     ├── Single shape cached
        │     ├── Direct inline check and access
        │     └── Fastest path
        │
        ├──▶ Poly IC (polymorphic)
        │     ├── 2-4 shapes cached
        │     ├── Shape-based dispatch
        │     └── Fast path
        │
        └──▶ Mega IC (megamorphic)
              ├── Many shapes (threshold exceeded)
              ├── Hash table lookup
              └── Slowest IC path
        │
        ▼
   Prototype chain traversal (if IC miss)
        │
        ├──▶ Found property?
        │     ├── Yes → Update IC with result
        │     └── No → Return undefined
        │
        ▼
   Return value
```

**IC polymorphic states:**
- **Mono IC** — Single shape cached, fastest path with direct inline check
- **Poly IC** — 2-4 shapes cached, shape-based dispatch
- **Mega IC** — Many shapes cached, hash table lookup for megamorphic sites

**IC operation types:**
- **LoadIC** — Property getter caching
- **StoreIC** — Property setter caching
- **ElementIC** — Array element access caching
- **GlobalIC** — Global variable access caching

**IC state transitions:**
- Mono → Poly: When a second shape is encountered
- Poly → Mega: When shape count exceeds threshold (typically 4)
- IC miss triggers prototype chain traversal and IC update

**Key abstractions:**
- **ICRuntime** — IC lookup and update logic
- **ICHandler** — Compact bit-encoded handler describing property access patterns (LoadHandler/StoreHandler for inline fast path, TransitionHandler/PrototypeHandler/TransWithProtoHandler/StoreAOTHandler as TaggedObjects for complex cases)
- **ProfileTypeInfo** — Type feedback and profiling data structure storing IC handlers, hotness counters, and compilation thresholds (used by interpreter, JIT, and AOT)
- **ProfileTypeInfoCell** — Wrapper containing ProfileTypeInfo and MachineCode, shared across functions via FunctionTemplate
- **MegaICCache** — Per-VM IC cache storage
- **PropertyBox** — Property value storage

## Reference Source
- `arkcompiler/ets_runtime/ecmascript/ic/ic_runtime.h` (1-100行) - IC runtime interface
- `arkcompiler/ets_runtime/ecmascript/ic/ic_handler.h` - IC handler definitions

## Key Files

| File | Role |
|------|------|
| `ic_runtime.h/.cpp` | IC lookup and update logic |
| `ic_handler.h/.cpp` | IC handler class definitions and bit encoding implementation |
| `ic_runtime_stub.h/.cpp` | IC runtime stub functions |
| `profile`_type_info.h/.cpp` | Type feedback collection |
| `mega_ic_cache.h/.cpp` | IC cache storage |
| `properties_cache.h` | Property cache utilities |
| `property_box.h/.cpp` | Property value storage |
| `proto_change_details.h/.cpp` | Prototype change tracking |

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
python ark.py x64.debug unittest   # includes IC tests
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/ic/tests:host_unittest
```

## Boundaries

- ICRuntime is a transient stack object created per property access operation.
- IC state transitions (Mono → Poly → Mega) are irreversible; Mega IC cannot downgrade.
- Prototype chain changes must invalidate relevant ICs via ProtoChangeDetails.
- ICHandler values must fit within JSTaggedValue size for inline fast path; complex cases use TaggedObject.
- IC slots are indexed by bytecode slot ID; ensure frontend and runtime slot numbering consistency.
