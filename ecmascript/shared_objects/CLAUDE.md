# AGENTS
**Name**: Shared Objects
**Purpose**: Shared Objects is a high-performance concurrent data structure module for the ArkTS runtime that provides thread-safe, shared-memory-capable container implementations. It enables safe concurrent access and modification of data structures across multiple Worker threads/actors. It delivers:
- Thread-safe shared containers: SharedArray, SharedMap, SharedSet, SharedTypedArray
- SendableArrayBuffer for transferable binary data between threads
- Fine-grained concurrent access control via ConcurrentApiScope with reader-writer locking
- Lock-free and wait-free synchronization primitives using atomic operations
- Support for both shared heap (SharedRegion) and off-heap memory allocation
- Full ES6 iterator protocol compliance for cross-thread iteration
- Coordinated garbage collection for shared objects across thread boundaries
- Concurrent modification detection and exception handling
- Memory consistency guarantees with proper memory ordering
**Primary Language**: C++ with atomic operations and memory barriers, integrated with ArkTS/JavaScript engine.

## Directory Structure
```text
shared_objects/
├── js_shared_object.h                          # Base class for all shared objects
├── js_shared_array.h / js_shared_array.cpp     # Thread-safe shared array implementation
├── js_shared_map.h / js_shared_map.cpp         # Thread-safe shared map (key-value) implementation
├── js_shared_set.h / js_shared_set.cpp         # Thread-safe shared set implementation
├── js_shared_typed_array.h                     # Thread-safe typed array (Int8Array, Uint8Array, etc.)
├── js_sendable_arraybuffer.h/.cpp              # Transferable ArrayBuffer for thread communication
├── js_shared_array_iterator.h/.cpp             # Iterator for SharedArray
├── js_shared_map_iterator.h/.cpp               # Iterator for SharedMap entries
├── js_shared_set_iterator.h/.cpp               # Iterator for SharedSet
├── concurrent_api_scope.h                      # RAII-style concurrent access control (reader-writer lock)
└── Note: No tests directory - tested through higher-level integration tests
```

## Building
This component is built as part of the ets_runtime shared library using the GN build system. All shared_objects source files are compiled into the main runtime library:

```bash
# Build shared_objects module (in OpenHarmony/ArkCompiler build system)
./build.sh --product-name <product> --build-target libark_jsruntime

# The objects are linked into libark_jsruntime.so / libark_jsruntime.dylib
# No separate shared_objects library is built
```

Build configuration location:
- Main BUILD.gn: `/arkcompiler/ets_runtime/BUILD.gn`
- Source files listed under `sources = []` in the ecmascript target

## Test Suite
There is no dedicated test directory for shared_objects. Testing is performed through:

1. **Integration Tests**: Located in `/arkcompiler/ets_runtime/test/` and related test suites
   - Concurrent Worker tests that create and modify shared containers
   - Sendable data transfer tests between threads
   - Actor model tests using shared memory

2. **Unit Tests**: Part of the ecmascript test suite
```bash
# Build and run integration tests
./build.sh --product-name <product> --build-target Sendable_SharedObjectFactory_Test

# Run specific concurrent/shared tests
./out/<product>/tests/unittest/ets_runtime/Sendable_SharedObjectFactory_Test --gtest_filter=*Shared* *Concurrent*
```

Test categories:
- Concurrent read/write access tests
- Thread creation and shared object passing tests
- Iterator consistency under concurrent modifications
- Memory ordering and visibility tests

## Dependency
### Package Dependencies
| Dependency | Purpose | Platform |
|------------|---------|----------|
| `ecmascript/js_object.h` | Base JS object class | All |
| `ecmascript/js_array.h` | Array implementation base | All |
| `ecmascript/js_typed_array.h` | TypedArray implementation base | All |
| `ecmascript/tagged_array.h` | Tagged value array storage | All |
| `ecmascript/containers/containers_errors.h` | Error handling utilities | All |
| `libpandabase/macros.h` | Atomic operation macros | All |
| `ecmascript/js_thread.h` | Thread-local execution context | All |
| `ecmascript/mem/barriers.h` | Memory barriers and fences | All |

### External Directory Interactions
- `../base/` - Built-in base classes and utilities (JSTaggedValue, JSHandle)
- `../` - Parent ecmascript module for GlobalEnv, JSThread, and execution context
- `../mem/` - Memory management for shared heap allocation (SharedRegion)
- `../compiler/` - Compiler integration for shared object bytecode generation
- `../interpreter/` - Interpreter support for shared object operations
- `../../include/ecmascript/` - Public API headers for runtime integration
- `../containers/` - Error codes reused from containers module (ContainerError, ErrorFlag)

## Boundaries
### Allowed Operations
- ✅ Add new shared container types (e.g., SharedLinkedList, SharedQueue)
- ✅ Extend ConcurrentApiScope for additional concurrency control patterns (e.g., optimistic locking)
- ✅ Implement new atomic operations for shared object coordination
- ✅ Add new shared memory allocators (e.g., persistent shared storage)
- ✅ Extend iterator implementations for custom traversal patterns
- ✅ Add new synchronization primitives (e.g., semaphores, condition variables)
- ✅ Implement concurrent data structure algorithms (lock-free skip lists, concurrent trees)
- ✅ Add memory consistency models for different architectures (ARM, x86, RISC-V)
- ✅ Extend SendableArrayBuffer with new transfer mechanisms
- ✅ Add diagnostic and profiling hooks for concurrent access patterns

### Prohibited Operations
- ❌ Do NOT modify the existing ModRecord-based synchronization protocol without coordinating with all dependent modules
- ❌ Do NOT remove or change the MOD_RECORD_OFFSET layout as it's critical for ConcurrentApiScope operation
- ❌ Do NOT bypass atomic operations when accessing shared object fields
- ❌ Do NOT weaken memory ordering guarantees (must maintain acquire-release semantics)
- ❌ Do NOT break ES6 iterator protocol compliance (Symbol.iterator, next(), return(), throw())
- ❌ Do NOT modify the SharedRegion memory layout without coordinating with GC
- ❌ Do NOT introduce blocking operations in ConcurrentApiScope (must remain lock-free where possible)
- ❌ Do NOT change error codes (CONCURRENT_MODIFICATION_ERROR) as they are part of the public API
- ❌ Do NOT remove thread-safety checks from existing shared objects
- ❌ Do NOT use non-atomic bit operations on shared fields without proper fencing
