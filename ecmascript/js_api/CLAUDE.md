# AGENTS
**Name**: JS API
**Purpose**: JS API is a high-performance JavaScript API implementation layer for container data structures in the ArkTS runtime. It provides native implementations of ArkTS container APIs that bridge the JavaScript world with the underlying C++ container implementations. It delivers:
- JS-friendly container classes (JSAPIHashMap, JSAPIVector, JSAPIArrayList, etc.)
- ES6-compliant iterator implementations for all container types
- Direct mapping to ArkTS `util.Collections` API
- Lazy initialization mechanism via `ArkPrivate.Load()` pattern
- Seamless integration with JavaScript garbage collector
- Type-safe container operations with automatic memory management
- Support for both sequential (ArrayList, Vector, LinkedList, Deque, List, Stack, Queue) and associative containers (HashMap, HashSet, TreeMap, TreeSet, LightWeightMap, LightWeightSet, PlainArray)
- Special containers (Buffer, BitVector) with optimized operations
**Primary Language**: C++ with JavaScript engine integration (ArkTS/ECMAScript runtime).

## Directory Structure
```text
js_api/
├── Linear Containers:
│   ├── js_api_arraylist.h/.cpp                    # Dynamic array implementation
│   ├── js_api_vector.h/.cpp                       # Growable vector array
│   ├── js_api_linked_list.h/.cpp                 # Doubly-linked list
│   ├── js_api_list.h/.cpp                         # Another list implementation
│   ├── js_api_deque.h/.cpp                        # Double-ended queue
│   ├── js_api_queue.h/.cpp                        # FIFO queue
│   ├── js_api_stack.h/.cpp                        # LIFO stack
│   └── js_api_plain_array.h/.cpp                  # Sparse array with integer keys
├── Hash Containers:
│   ├── js_api_hashmap.h/.cpp                      # Hash-based key-value map
│   └── js_api_hashset.h/.cpp                      # Hash-based unique value set
├── Tree Containers:
│   ├── js_api_tree_map.h/.cpp                     # Red-black tree ordered map
│   └── js_api_tree_set.h/.cpp                     # Red-black tree ordered set
├── Lightweight Containers:
│   ├── js_api_lightweightmap.h/.cpp               # Optimized hash map
│   └── js_api_lightweightset.h/.cpp               # Optimized hash set
├── Special Containers:
│   ├── js_api_buffer.h/.cpp                       # Binary buffer operations
│   └── js_api_bitvector.h/.cpp                    # Bit vector for boolean storage
├── Iterators (17 iterator implementations):
│   ├── js_api_arraylist_iterator.h/.cpp
│   ├── js_api_vector_iterator.h/.cpp
│   ├── js_api_linked_list_iterator.h/.cpp
│   ├── js_api_list_iterator.h/.cpp
│   ├── js_api_deque_iterator.h/.cpp
│   ├── js_api_queue_iterator.h/.cpp
│   ├── js_api_stack_iterator.h/.cpp
│   ├── js_api_plain_array_iterator.h/.cpp
│   ├── js_api_hashmap_iterator.h/.cpp
│   ├── js_api_hashset_iterator.h/.cpp
│   ├── js_api_tree_map_iterator.h/.cpp
│   ├── js_api_tree_set_iterator.h/.cpp
│   ├── js_api_lightweightmap_iterator.h/.cpp
│   ├── js_api_lightweightset_iterator.h/.cpp
│   └── js_api_bitvector_iterator.h/.cpp
└── Note: Tests are in ../tests/ directory
```

## Building
This component is built as part of the ets_runtime shared library using the GN build system:

```bash
# Build js_api module (in OpenHarmony/ArkCompiler build system)
./build.sh --product-name <product> --build-target libark_jsruntime

# All JS API source files are compiled into libark_jsruntime.so / libark_jsruntime.dylib
# No separate js_api library is built
```

Build configuration:
- Main BUILD.gn: `/arkcompiler/ets_runtime/BUILD.gn`
- All `.cpp` files in `js_api/` are listed in ecmascript target sources
- Compiled with ECMASRIPT builtins integration

## Test Suite
Tests are located in the `/arkcompiler/ets_runtime/ecmascript/tests/` directory:

```bash
# Build and run tests
./build.sh --product-name <product> --build-target ets_runtime_unittest

# Run specific container tests
./out/<product>/tests/unittest/ets_runtime/ets_runtime_unittest --gtest_filter=*JSAPI*

# Or run container-specific tests
./out/<product>/tests/unittest/ets_runtime/ets_runtime_unittest --gtest_filter=*js_api_vector*
```

Test files (20+ test files):
- `js_api_arraylist_test.cpp` - ArrayList functionality tests
- `js_api_vector_test.cpp` - Vector functionality tests
- `js_api_linked_list_test.cpp` - LinkedList tests
- `js_api_deque_test.cpp` - Deque tests
- `js_api_queue_test.cpp` - Queue tests
- `js_api_stack_test.cpp` - Stack tests
- `js_api_plain_array_test.cpp` - PlainArray tests
- `js_api_hashmap_test.cpp` - HashMap tests
- `js_api_hashset_test.cpp` - HashSet tests
- `js_api_tree_map_test.cpp` - TreeMap tests
- `js_api_tree_set_test.cpp` - TreeSet tests
- `js_api_lightweightmap_test.cpp` - LightWeightMap tests
- `js_api_lightweightset_test.cpp` - LightWeightSet tests
- `js_api_bitvector_test.cpp` - BitVector tests
- `js_api_buffer_test.cpp` - Buffer tests
- Iterator tests for all container types

## Dependency
### Package Dependencies
| Dependency | Purpose | Platform |
|------------|---------|----------|
| `ecmascript/js_object.h` | Base JS object class | All |
| `ecmascript/js_tagged_value.h` | Tagged value representation | All |
| `ecmascript/tagged_node.h` | Linked list and tree nodes | All |
| `ecmascript/js_thread.h` | Thread-local execution context | All |
| `ecmascript/global_env.h` | Global environment for registration | All |
| `ecmascript/base/builtins_base.h` | Builtin function registration | All |
| `ecmascript/ecma_runtime_call_info.h` | Runtime call information | All |
| `ecmascript/js_array.h` | Array operations integration | All |
| `ecmascript/containers/` | Underlying C++ container implementations | All |

### External Directory Interactions
- `../` - Parent ecmascript module for core runtime components
- `../base/` - Builtins base and utility functions
- `../containers/` - Native container implementations (HashMap, Vector, etc.) used by JS API wrappers
- `../shared_objects/` - Shared container implementations for concurrent scenarios
- `../interpreter/` - Interpreter integration for runtime calls
- `../compiler/` - Compiler integration for bytecode generation
- `../../include/ecmascript/` - Public API headers
- `../tests/` - Unit test directory for all js_api tests

## Boundaries
### Allowed Operations
- ✅ Add new JS API container types following existing patterns
- ✅ Extend existing containers with new methods that match ArkTS specifications
- ✅ Implement new iterator types following ES6 iterator protocol
- ✅ Add utility functions for container operations (merge, split, transform, etc.)
- ✅ Extend error handling with specific error types for containers
- ✅ Add diagnostic and profiling hooks for container operations
- ✅ Implement custom comparators or hash functions for specialized use cases
- ✅ Add serialization/deserialization support for containers
- ✅ Extend with additional functional programming methods (map, filter, reduce, flatMap, etc.)
- ✅ Implement native optimizations for hot paths (e.g., bulk operations)

### Prohibited Operations
- ❌ Do NOT modify existing public API signatures without coordination with ArkTS language specifications
- ❌ Do NOT break ES6 iterator protocol compliance (Symbol.iterator, next(), return(), throw())
- ❌ Do NOT remove or change the lazy loading mechanism (ArkPrivate.Load pattern)
- ❌ Do NOT bypass garbage collector integration (must use proper visitation macros)
- ❌ Do NOT change error types or codes as they are part of the public API
- ❌ Do NOT modify the memory layout of existing containers without updating all accessors
- ❌ Do NOT remove or rename methods that are documented in the ArkTS API reference
- ❌ Do NOT break compatibility with existing ArkTS code that uses these containers
- ❌ Do NOT introduce blocking operations that could freeze the JavaScript event loop
- ❌ Do NOT change the relationship between JS API wrappers and underlying container implementations
