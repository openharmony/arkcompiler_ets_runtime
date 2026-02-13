# AGENTS
**Name**: Containers
**Purpose**: Containers is a high-performance data structure module for ArkTS runtime that provides comprehensive container implementations. It offers efficient in-memory data storage and manipulation capabilities with features including:
- Linear containers: ArrayList, Vector, LinkedList, Deque, List, Stack, Queue, PlainArray
- Hash-based containers: HashMap, HashSet, LightWeightMap, LightWeightSet
- Tree-based containers: TreeMap, TreeSet (ordered collections)
- Special containers: BitVector, Buffer (FastBuffer)
- Lazy-loading mechanism via ArkPrivate.Load() for on-demand initialization
- Full iterator support for all container types
- ES6-compatible iteration protocols with Symbol.iterator
- High-performance native implementations for ArkTS applications
**Primary Language**: C++ with embedded runtime integration for ArkTS/JavaScript engine.

## Directory Structure
```text
containers/
├── containers_arraylist.h / containers_arraylist.cpp      # Dynamic array implementation
├── containers_bitvector.h / containers_bitvector.cpp      # Bit vector for boolean storage
├── containers_buffer.h / containers_buffer.cpp            # Fast binary buffer operations
├── containers_deque.h / containers_deque.cpp              # Double-ended queue
├── containers_hashmap.h / containers_hashmap.cpp          # Hash-based key-value map
├── containers_hashset.h / containers_hashset.cpp          # Hash-based unique value set
├── containers_lightweightmap.h /.cpp                      # Lightweight hash map (optimized)
├── containers_lightweightset.h /.cpp                      # Lightweight hash set (optimized)
├── containers_linked_list.h /.cpp                         # Doubly linked list
├── containers_list.h / containers_list.cpp                # Bidirectional linked list
├── containers_plainarray.h /.cpp                          # Sparse array with integer keys
├── containers_queue.h / containers_queue.cpp              # FIFO queue
├── containers_stack.h / containers_stack.cpp              # LIFO stack
├── containers_treemap.h / containers_treemap.cpp          # Red-black tree based ordered map
├── containers_treeset.h / containers_treeset.cpp          # Red-black tree based ordered set
├── containers_vector.h / containers_vector.cpp            # Growable vector array
├── containers_private.h / containers_private.cpp          # Lazy-loading initialization
├── containers_errors.h / containers_errors.cpp            # Error handling utilities
└── tests/                                                 # Unit tests subdirectory
    ├── BUILD.gn                                          # Test build configuration
    ├── containers_test_helper.h                          # Test utilities
    ├── containers_arraylist_test.cpp
    ├── containers_bitvector_test.cpp
    ├── containers_buffer_test.cpp
    ├── containers_deque_test.cpp
    ├── containers_hashmap_test.cpp
    ├── containers_hashset_test.cpp
    ├── containers_lightweightmap_test.cpp
    ├── containers_lightweightset_test.cpp
    ├── containers_linked_list_test.cpp
    ├── containers_list_test.cpp
    ├── containers_plainarray_test.cpp
    ├── containers_queue_test.cpp
    ├── containers_stack_test.cpp
    ├── containers_treemap_test.cpp
    ├── containers_treeset_test.cpp
    └── containers_vector_test.cpp
```

## Building
The component is built as part of the ets_runtime shared library using GN build system. All container source files are compiled into the main runtime library:

```bash
# Build the containers module (within OpenHarmony/ArkCompiler build system)
./build.sh --product-name <product> --build-target libark_jsruntime

# The containers are linked into libark_jsruntime.so/libark_jsruntime.dylib
# No separate container library is built
```

## Test Suite
To execute the code tests, run the following commands:

```bash
# Build and generate test executables (split into 4 test suites)
./build.sh --product-name <product> --build-target Containers_001_Test
./build.sh --product-name <product> --build-target Containers_002_Test
./build.sh --product-name <product> --build-target Containers_003_Test
./build.sh --product-name <product> --build-target Containers_004_Test

# Run the executable file (example for Containers_001_Test)
./out/<product>/tests/unittest/ets_runtime/Containers_001_Test
```

The test suite includes 4 groups:
- **Containers_001_Test**: ArrayList, Deque, HashMap, HashSet, LightWeightMap
- **Containers_002_Test**: LightWeightSet, LinkedList, List, PlainArray, Queue
- **Containers_003_Test**: Buffer, Stack, TreeMap, TreeSet, Vector
- **Containers_004_Test**: BitVector

## Dependency
### Package Dependencies
| Dependency | Purpose | Platform |
|------------|---------|----------|
| `ecmascript/base` | Built-in base classes and utilities | All |
| `ecmascript/js_runtime` | JavaScript runtime integration | All |
| `icu:shared_icui18n` | Internationalization (i18n) | All |
| `icu:shared_icuuc` | Internationalization (uc) | All |
| `hilog:libhilog` | System logging | All |
| `libark_jsruntime_test` | Test runtime library | Test only |
| `gtest` | Google Test | Test only |

### External Directory Interactions
- `../base/` - Base built-in classes (BuiltinsBase) and utility functions
- `../interpreter/` - Interpreter integration for runtime calls
- `../` - Parent ecmascript module for GlobalEnv and JSHandle types
- `../../include/ecmascript/` - Public API headers for runtime integration
- `../../test/test_helper.gni` - Test configuration and utilities

## Boundaries
### Allowed Operations
- ✅ Add new container types by extending the ContainerTag enum and implementing initialize methods
- ✅ Implement new methods for existing containers following the existing patterns
- ✅ Add new iterator types for container traversal
- ✅ Optimize container performance (memory, speed) while maintaining API compatibility
- ✅ Extend error handling with new error types in ContainerError
- ✅ Add new utility functions to containers_private for initialization helpers
- ✅ Implement custom comparators or hash functions for specialized containers
- ✅ Add container serialization/deserialization support
- ✅ Extend buffer operations with new data type read/write methods

### Prohibited Operations
- ❌ Do NOT modify existing container APIs in ways that break backward compatibility
- ❌ Do NOT change the lazy-loading mechanism without coordinating with the ArkPrivate module
- ❌ Do NOT remove or rename ContainerTag enum values as they are referenced from JS
- ❌ Do NOT bypass iterator protocol implementation (Symbol.iterator, next(), etc.)
- ❌ Do NOT modify the error code definitions (BIND_ERROR, RANGE_ERROR, etc.)
- ❌ Do NOT use raw JSTaggedValue operations without proper type checking
- ❌ Do NOT break ES6 iterator protocol compliance
- ❌ Do NOT change thread-safety guarantees without proper synchronization
- ❌ Do NOT modify existing container memory layout without updating all accessor methods
