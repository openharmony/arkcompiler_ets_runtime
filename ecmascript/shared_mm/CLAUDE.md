# AGENTS
**Name**: Shared Memory Manager (SharedMM)
**Purpose**: Shared Memory Manager is a singleton memory management component for the ArkTS runtime that provides shared heap memory allocation and reference counting for concurrent data structures. It enables safe memory sharing across multiple Worker threads/actors. It delivers:
- Centralized shared memory allocation with malloc/free-based heap management
- Reference counting for shared memory blocks to coordinate lifetime across threads
- Thread-safe operations via recursive mutex (RecursiveMutex) protection
- Memory tracking with pointer-to-refcount mapping (CMap<uint64_t, int32_t>)
- Create-or-load semantics to reuse existing shared memory blocks
- Automatic memory cleanup on reference count reaching zero
- Maximum allocation size limit (2GB) to prevent excessive memory consumption
- Optional memory zapping (memset with INVALID_VALUE) for debugging in debug builds
- Singleton pattern ensures single instance across the entire process
**Primary Language**: C++ with mutex-protected shared memory operations.

## Directory Structure
```text
shared_mm/
├── shared_mm.h                                  # Shared memory manager interface
└── shared_mm.cpp                                # Implementation with ref counting
```

**Note**: This is a minimal module with only 2 files. No tests directory - testing is performed through higher-level integration tests that use shared memory (e.g., SharedArray, SharedMap, SharedSet tests).

## Building
This component is built as part of the ets_runtime shared library using the GN build system:

```bash
# Build shared_mm module (in OpenHarmony/ArkCompiler build system)
./build.sh --product-name <product> --build-target libark_jsruntime

# The module is compiled into libark_jsruntime.so / libark_jsruntime.dylib
# No separate shared_mm library is built
```

Build configuration:
- Main BUILD.gn: `arkcompiler/ets_runtime/BUILD.gn`
- Source files: `shared_mm/shared_mm.cpp` listed in ecmascript target sources
- Compile definitions:
  - `ECMASCRIPT_ENABLE_ZAP_MEM` - Enable memory initialization with INVALID_VALUE

## Test Suite
There is no dedicated test directory for shared_mm. Testing is performed through:

1. **Integration Tests**: Higher-level shared object tests implicitly test the memory manager
   - SharedArray/SharedMap/SharedSet lifecycle tests
   - Cross-thread shared memory transfer tests
   - Reference counting correctness tests

2. **Unit Tests**: Part of the ecmascript test suite
```bash
# Build and run tests
./build.sh --product-name <product> --build-target ets_runtime_unittest

# Run tests that exercise shared memory management
./out/<product>/tests/unittest/ets_runtime/ets_runtime_unittest --gtest_filter=*Shared*
```

Test scenarios:
- Multiple Workers sharing the same memory block
- Reference count increment/decrement across thread boundaries
- Memory cleanup when all references are released
- Concurrent CreateOrLoad operations

## Dependency
### Package Dependencies
| Dependency | Purpose | Platform |
|------------|---------|----------|
| `ecmascript/mem/c_containers.h` | CMap container for pointer-to-refcount mapping | All |
| `ecmascript/platform/mutex.h` | RecursiveMutex for thread-safe operations | All |
| `ecmascript/ecma_vm.h` | EcmaVM context (forward declared) | All |
| `securec.h` | Secure memset_s for memory zapping (optional) | All |

### External Directory Interactions
- `../mem/` - Memory management utilities (CMap, allocator)
- `../platform/` - Platform-specific mutex implementation
- `../` - Parent ecmascript module for VM context
- `../../include/ecmascript/` - Public API headers for external integration
- `../shared_objects/` - Primary consumers of shared memory (SharedArray, SharedMap, SharedSet)
- `../js_object.h` - JS object base that may use shared memory

## Boundaries
### Allowed Operations
- ✅ Extend reference counting with additional statistics (allocation count, total bytes, etc.)
- ✅ Add memory allocation/deallocation callbacks for profiling
- ✅ Implement custom allocators (e.g., shared memory segments, huge pages)
- ✅ Add memory pool management for small fixed-size allocations
- ✅ Extend with memory pressure monitoring and backpressure mechanisms
- ✅ Add alignment constraints for SIMD operations
- ✅ Implement memory leak detection and debugging utilities
- ✅ Add guards/canaries for buffer overflow detection
- ✅ Extend with per-allocation metadata (size, timestamp, allocation site)
- ✅ Implement shared memory regions with different access permissions

### Prohibited Operations
- ❌ Do NOT change the singleton pattern (GetInstance must return the same instance)
- ❌ Do NOT remove the mutex protection from concurrent operations
- ❌ Do NOT modify the reference counting protocol without coordinating with all dependent modules
- ❌ Do NOT change the pointer-as-key approach (uint64_t cast) without reviewing all callers
- ❌ Do NOT bypass the reference counting mechanism (always use IncreaseRef/DecreaseRef)
- ❌ Do NOT introduce blocking operations while holding the lock
- ❌ Do NOT change the MALLOC_SIZE_LIMIT constant without reviewing security implications
- ❌ Do NOT remove or modify the RemoveSharedMemory callback signature
- ❌ Do NOT use non-thread-safe allocators (malloc/free are acceptable as they're protected by mutex)
- ❌ Do NOT change the CreateOrLoad return value semantics (true=created, false=loaded)
