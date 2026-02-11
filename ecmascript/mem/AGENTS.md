# AGENTS
**Name**: Memory Management (mem)
**Purpose**: Memory Management is the core garbage collection and memory allocation subsystem for the ArkTS runtime. It provides comprehensive memory management capabilities including:
- Multiple garbage collection algorithms (FullGC, PartialGC, SharedGC, ConcurrentCopyGC, SweepGC)
- Generational garbage collection with young/old space separation
- Concurrent marking and sweeping for minimal pause times
- Region-based memory management for CMC (Concurrent Mark-Compact) GC
- Shared heap for multi-VM shared objects
- TLAB (Thread-Local Allocation Buffer) for fast allocation
- Memory barriers and write barriers for generational GC
- GC statistics, monitoring, and tuning
- Heap verification and debugging support
**Primary Language**: C++

## Directory Structure
```text
mem/
├── Core Memory Management
│   ├── heap.h/cpp                                           # Main Heap class
│   ├── space.h/cpp                                          # Space abstractions
│   ├── region.h/cpp                                         # Region-based memory management
│   ├── sparse_space.h/cpp                                   # Sparse space implementation
│   ├── linear_space.h/cpp                                   # Linear space implementation
│   ├── allocator.h                                          # Allocator interfaces
│   ├── tlab_allocator.h                                     # Thread-local allocation buffer
│   ├── chunk.h/cpp                                          # Memory chunk management
│   ├── tagged_object.h/cpp                                  # Tagged object representation
│   └── mem_common.h                                         # Memory-related constants
│
├── Garbage Collection
│   ├── garbage_collector.h                                  # GC base interface
│   ├── full_gc.h/cpp                                        # Full garbage collector
│   ├── partial_gc.h/cpp                                     # Partial garbage collector
│   ├── concurrent_marker.h/cpp                              # Concurrent marking
│   ├── concurrent_sweeper.h/cpp                             # Concurrent sweeping
│   ├── parallel_marker.h/cpp                                # Parallel marking
│   ├── parallel_evacuator.h/cpp                             # Parallel evacuation
│   ├── gc_stats.h/cpp                                       # GC statistics
│   ├── gc_key_stats.h/cpp                                   # GC key metrics
│   └── verification.h/cpp                                   # Heap verification
│
├── Memory Control
│   ├── mem_controller.h/cpp                                 # Memory controller
│   ├── mem_controller_utils.h/cpp                           # Memory controller utilities
│   ├── shared_mem_controller.h/cpp                          # Shared heap controller
│   ├── idle_gc_trigger.h/cpp                                # Idle-time GC trigger
│   └── allocation_inspector.h/cpp                           # Allocation inspection
│
├── Barriers and Remembered Sets
│   ├── barriers.h/cpp                                       # Write barriers
│   ├── barriers-inl.h                                       # Barrier inline implementations
│   ├── barriers_get-inl.h                                   # Barrier getters
│   ├── remembered_set.h                                     # Remembered set data structure
│   └── rset_worklist_handler.h/cpp                          # Remembered set worklist
│
├── Specialized Allocators
│   ├── native_area_allocator.h/cpp                          # Native memory area
│   ├── machine_code.h/cpp                                   # Machine code space
│   ├── heap_region_allocator.h/cpp                          # Heap region allocator
│   ├── regexp_cached_chunk.h/cpp                            # RegExp cached chunks
│   └── thread_local_allocation_buffer.h/cpp                 # TLAB implementation
│
├── Visitors and Utilities
│   ├── visitor.h                                            # GC visitor base interface
│   ├── object_xray.h                                        # Object introspection
│   ├── mark_stack.h                                         # Mark stack for GC
│   ├── mark_word.h                                          # Mark word encoding
│   ├── layout_visitor.h                                     # Layout visitor
│   ├── free_object_list.h/cpp                               # Free object list
│   ├── free_object_set.h/cpp                                # Free object set
│   └── work_manager.h/cpp                                   # Work management for parallel GC
│
├── CMC GC Subsystem (Concurrent Mark-Compact)
├── cmc_gc/
│   └── barriers_get-inl.h                                   # CMC-specific barriers
│
├── CMS Memory Subsystem (Concurrent Mark-Sweep)
├── cms_mem/
│   ├── slot_space.h/cpp                                     # Slot space for CMS
│   ├── slot_allocator.h/cpp                                 # Slot allocator
│   ├── slot_gc_allocator.h/cpp                              # Slot GC allocator
│   ├── slot_free_list.h/cpp                                 # Slot free list
│   ├── cms_region_chain_manager.h/cpp                       # CMS region chain manager
│   ├── sweep_gc.h/cpp                                       # Sweep GC
│   └── sweep_gc_visitor.h/cpp                               # Sweep GC visitor
│
├── Local CMC Subsystem
├── local_cmc/
│   ├── concurrent_copy_gc.h/cpp                             # Concurrent copy GC
│   ├── cc_evacuator.h/cpp                                   # CMC evacuator
│   └── cc_gc_visitor.h/cpp                                  # CMC GC visitor
│
├── Shared Heap Subsystem
├── shared_heap/
│   ├── shared_gc.h/cpp                                      # Shared heap GC
│   ├── shared_space.h/cpp                                   # Shared space
│   ├── shared_full_gc.h/cpp                                 # Shared full GC
│   ├── shared_concurrent_marker.h/cpp                       # Shared concurrent marker
│   ├── shared_concurrent_sweeper.h/cpp                       # Shared concurrent sweeper
│   ├── shared_gc_marker.h/cpp                               # Shared GC marker
│   ├── shared_gc_evacuator.h/cpp                            # Shared GC evacuator
│   ├── shared_gc_visitor.h/cpp                              # Shared GC visitor
│   └── shared_value_helper.h                                # Shared value utilities
│
└── Supporting Files
    ├── c_containers.h                                       # C-style containers
    ├── c_string.h/cpp                                       # C string utilities
    ├── ecma_list.h                                          # ECMA list
    ├── slots.h                                              # Slot abstractions
    ├── dyn_chunk.h/cpp                                      # Dynamic chunk
    └── gc_root.h                                            # GC root management
```

## Building
The mem module is built as part of the ecmascript module in the GN build system:
```bash
# Build for OHOS ARM64
./build.sh --product-name rk3568 --gn-args use_musl=true --target-cpu arm64 --build-target ark_js_packages

# Build for OHOS ARM32
./build.sh --product-name rk3568 --build-target ark_js_packages

# Debug build (add to any command)
--gn-args is_debug=true
```

## Test Suite
The mem module has extensive unit tests in the ecmascript/tests/ directory:
```bash
# Build and generate an executable file
./build.sh --product-name rk3568 --build-target GC_First_Test --build-target GC_Second_Test --build-target GC_Third_Test --build-target GC_NewToOldPromotion_Test --build-target GC_SharedHeapOOM_Test --build-target GC_SharedPartial_Test --build-target GC_Verify_Test --build-target ECMA_VM_Heap_Test --build-target Mem_Controller_Test --build-target Shared_Heap_Test --build-target Unified_GC_Test --build-target Weak_Ref_Old_GC_Test
# Send the executable file to the device using hdc
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/GC_First_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/GC_Second_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/GC_Third_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/GC_NewToOldPromotion_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/GC_SharedHeapOOM_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/GC_SharedPartial_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/GC_Verify_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/ECMA_VM_Heap_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/Mem_Controller_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/Shared_Heap_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/Unified_GC_Test /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/Weak_Ref_Old_GC_Test /data/local/tmp/    
# Run specific memory/GC tests
hdc shell
cd /data/local/tmp/
chmod 777 ./GC_First_Test
./GC_First_Test
chmod 777 ./GC_Second_Test
./GC_Second_Test
chmod 777 ./GC_Third_Test
./GC_Third_Test
chmod 777 ./GC_NewToOldPromotion_Test
./GC_NewToOldPromotion_Test
chmod 777 ./GC_SharedHeapOOM_Test
./GC_SharedHeapOOM_Test
chmod 777 ./GC_SharedPartial_Test
./GC_SharedPartial_Test
chmod 777 ./GC_Verify_Test
./GC_Verify_Test
chmod 777 ./ECMA_VM_Heap_Test
./ECMA_VM_Heap_Test
chmod 777 ./Mem_Controller_Test
./Mem_Controller_Test
chmod 777 ./Shared_Heap_Test
./Shared_Heap_Test
chmod 777 ./Unified_GC_Test
./Unified_GC_Test
chmod 777 ./Weak_Ref_Old_GC_Test
./Weak_Ref_Old_GC_Test
```

Test coverage includes:
- **gc_first_test.cpp**: Basic GC functionality tests
- **gc_second_test.cpp**: Secondary GC tests
- **gc_third_test.cpp**: Advanced GC tests
- **gc_region_promotion_test.cpp**: Region promotion tests
- **gc_shared_heap_oom_test.cpp**: Shared heap OOM tests
- **gc_shared_partial_test.cpp**: Shared partial GC tests
- **gc_verify_test.cpp**: Heap verification tests
- **ecma_vm_heap_test.cpp**: VM-Heap integration tests
- **mem_controller_test.cpp**: Memory controller tests
- **shared_heap_test.cpp**: Shared heap tests
- **unified_gc_test.cpp**: Unified GC tests
- **weak_ref_old_gc_test.cpp**: Weak reference tests

## Dependency
### Package Dependencies
| Dependency | Purpose |
|------------|---------|
| `ecmascript/js_thread` | JSThread for GC context |
| `ecmascript/ecma_vm` | EcmaVM for VM integration |
| `ecmascript/daemon/daemon_thread` | Daemon thread for concurrent GC |
| `ecmascript/base` | Base configurations |
| `ecmascript/cross_vm` | Cross-VM memory sharing |
| `ecmascript/napi/include/jsnapi_expo.h` | N-API memory interface |
| `ecmascript/string/external_string_table` | External string table |
| `ecmascript/platform/mutex` | Mutex for thread safety |
| `ecmascript/platform/map` | Memory mapping |
| `common_components/taskpool/taskpool` | TaskPool for parallel GC |
| `common_components/heap/heap` | Common heap utilities |
| `libpandabase/macros` | Base macros |
| `libpandabase/utils/` | Utility functions |
| `securec` | Secure C library |

### External Directory Interactions
- `../base/` - Base configurations and utilities
- `../daemon/` - Daemon thread for background GC
- `../napi/` - N-API layer for external memory management
- `../string/` - String table and external string management
- `../platform/` - Platform-specific primitives (mutex, map, etc.)
- `../compiler/` - JIT code space management
- `../dfx/` - Diagnostic features (heap profiling, etc.)
- `../../common_components/taskpool/` - Parallel task execution
- `../../common_components/heap/` - Shared heap utilities
- `../cross_vm/` - Cross-VM memory sharing

### GC Algorithms Supported
- **FullGC**: Full heap garbage collection
- **PartialGC**: Partial (generational) GC for young space
- **SharedGC**: Shared heap GC for multi-VM scenarios
- **ConcurrentCopyGC**: Concurrent mark-copy collection
- **SweepGC**: Mark-sweep garbage collection

### Memory Spaces
- **Young Spaces**: Semi-space copying for new objects
- **Old Spaces**: Old generation for long-lived objects
- **Non-Movable Space**: For objects that cannot be moved
- **Machine Code Space**: For JIT-compiled code
- **Huge Object Space**: For large objects
- **Shared Spaces**: Cross-VM shared memory spaces
- **Snapshot Space**: For deserialized objects
- **ReadOnly Space**: For immutable objects

## Boundaries
### Allowed Operations
- ✅ Add new GC algorithms by extending `GarbageCollector` base class
- ✅ Implement new memory space types by extending `Space` classes
- ✅ Add new allocation strategies (allocators, free lists)
- ✅ Extend GC statistics collection and monitoring
- ✅ Implement new barrier types for write operations
- ✅ Add new remembered set optimizations
- ✅ Optimize region management and allocation
- ✅ Implement new heap verification checks
- ✅ Add new GC trigger conditions

### Prohibited Operations
- ❌ Do NOT modify object layout without updating all GC visitors
- ❌ Do NOT bypass write barriers - all cross-generational writes must use barriers
- ❌ Do NOT allocate directly in spaces without proper GC synchronization
- ❌ Do NOT free objects without GC involvement - use GC only
- ❌ Do NOT modify mark bits concurrently without atomic operations
- ❌ Do NOT access freed objects - use verification to catch bugs
- ❌ Do NOT ignore thread safety in concurrent GC phases
- ❌ Do NOT hard-code heap size limits - use configurable parameters
- ❌ Do NOT disable heap verification in debug builds
- ❌ Do NOT mix local and shared heap object references
- ❌ Do NOT access shared heap without proper mutator locks
- ❌ Do NOT trigger GC during allocation without proper stack validation
- ❌ Do NOT modify region flags during concurrent marking
- ❌ Do NOT use raw pointers to heap objects without handles/slots
