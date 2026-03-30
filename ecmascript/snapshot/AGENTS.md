# AGENTS
**Name**: Snapshot
**Purpose**: Snapshot is a runtime serialization/deserialization module for the ArkTS runtime that enables fast application startup through heap snapshot persistence. Main features include:
- VM state serialization/deserialization for fast startup
- Builtins object snapshot caching
- Module snapshot for rapid module loading
- JSPandaFile snapshot for bytecode caching
- Object encoding with bit-field compression (EncodeBit)
- Memory space-aware object relocation and allocation
- Region-based memory management for snapshot storage
- Signal handler integration for snapshot error recovery
- TaskPool-based asynchronous snapshot persistence
**Primary Language**: C++

## Directory Structure
```text
snapshot/
├── mem/
│   ├── BUILD.gn                                              # Build configuration for snapshot binary generation
│   ├── snapshot.h                                            # Main Snapshot class for serialize/deserialize
│   ├── snapshot.cpp                                          # Snapshot implementation
│   ├── snapshot_processor.h                                  # SnapshotProcessor for object encoding/decoding
│   ├── snapshot_processor.cpp                                # SnapshotProcessor implementation
│   ├── snapshot_env.h                                        # SnapshotEnv for global object tracking
│   ├── snapshot_env.cpp                                      # SnapshotEnv implementation
│   ├── encode_bit.h                                          # EncodeBit for compressed object encoding
│   └── constants.h                                           # Snapshot constants
├── common/
│   ├── modules_snapshot_helper.h                             # Module snapshot utility functions
│   └── modules_snapshot_helper.cpp                           # Module snapshot helper implementation
└── tests/
    ├── BUILD.gn                                              # Test build configuration
    ├── snapshot_test.cpp                                     # Core snapshot tests
    ├── modules_snapshot_helper_test.cpp                      # Module helper tests
    └── snapshot_mock.h                                       # Test mock utilities
```

## Building
The snapshot module has both library and binary build targets:
```bash
# Build for OHOS ARM64
./build.sh --product-name rk3568 --gn-args use_musl=true --target-cpu arm64 --build-target ark_js_packages

# Build for OHOS ARM32
./build.sh --product-name rk3568 --build-target ark_js_packages

# Debug build (add to any command)
--gn-args is_debug=true

# Generate snapshot binary using gen_snapshot_bin target
./build.sh --product-name rk3568 --build-target gen_ark_snapshot_bin
```

The snapshot binary generation uses `tools/gen_snapshot.sh` script to create startup snapshots from ABC bytecode files.

## Test Suite
The snapshot module has unit tests in the tests/ directory:
```bash
# Build and generate an executable file
./build.sh --product-name rk3568 --build-target SnapshotTest --build-target ModuleSnapshotHelperTest
# Send the executable file to the device using hdc
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/SnapshotTest /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/ModuleSnapshotHelperTest /data/local/tmp/
# Run specific snapshot tests
hdc shell
cd /data/local/tmp/
chmod 777 ./SnapshotTest
./SnapshotTest
chmod 777 ./ModuleSnapshotHelperTest
./ModuleSnapshotHelperTest
```

Test coverage includes:
- **snapshot_test.cpp**: Core serialization/deserialization tests (disabled for CMC-GC)
- **modules_snapshot_helper_test.cpp**: Module snapshot helper utility tests

Note: Some tests are temporarily disabled for CMC-GC mode (see `ets_runtime_enable_cmc_gc` in BUILD.gn).

## Dependency
### Package Dependencies
| Dependency | Purpose |
|------------|---------|
| `ecmascript/js_thread` | JSThread for current thread context |
| `ecmascript/ecma_vm` | EcmaVM for VM management |
| `ecmascript/mem/heap` | Heap for memory allocation |
| `ecmascript/mem/sparse_space` | Sparse space memory management |
| `ecmascript/mem/region` | Region-based memory management |
| `ecmascript/mem/shared_heap` | SharedHeap for shared object serialization |
| `ecmascript/serializer/serialize_data` | SerializeData for module snapshots |
| `ecmascript/serializer/module_serializer` | ModuleSerializer for module snapshots |
| `ecmascript/jspandafile/js_pandafile` | JSPandaFile for bytecode file handling |
| `ecmascript/jspandafile/method_literal` | MethodLiteral for method serialization |
| `ecmascript/compiler/aot_file/aot_version` | AOT file version management |
| `ecmascript/compiler/aot_snapshot` | AOT snapshot integration |
| `common_components/taskpool/taskpool` | TaskPool for async snapshot saving |
| `common_components/taskpool/task` | Task base class for snapshot jobs |
| `common_components/serialize/serialize_utils` | Serialization utility functions |
| `libpandabase/utils/bit_field` | BitField for EncodeBit implementation |
| `icu:shared_icui18n`, `icu:shared_icuuc` | ICU internationalization |
| `zlib:libz` | Compression library |

### External Directory Interactions
- `../mem/` - Memory management subsystem (Heap, Region, Space)
- `../mem/shared_heap/` - Shared heap for concurrent GC scenarios
- `../serializer/` - Serializer for module snapshot integration
- `../module/` - Module system for module snapshot loading
- `../jspandafile/` - PandaFile manager for JSPandaFile snapshot
- `../compiler/aot_snapshot/` - AOT snapshot integration
- `../../common_components/taskpool/` - TaskPool for async operations
- `../compiler/aot_file/` - AOT file version compatibility

### Snapshot Types
The module supports three types of snapshots:
- **VM_ROOT**: Complete VM state snapshot for application startup
- **BUILTINS**: Built-in objects snapshot for caching standard library
- **AI**: AOT (Ahead-of-Time) compilation snapshot

### Key Data Structures
**EncodeBit**: 64-bit compressed object encoding with bit fields:
- Region index (10 bits)
- Object offset in region (20 bits)
- String reference flag (1 bit)
- Object type (8 bits)
- Special object flag (1 bit)
- Global const/builtins flag (1 bit)
- TS weak object flag (1 bit)
- Reference bits (16 bits)
- Unused bits (6 bits)

**SnapshotProcessor**: Core serialization/deserialization engine with:
- Region-based allocation proxies
- Object queue processing
- Field visitor pattern for traversal
- Native pointer encoding

## Boundaries
### Allowed Operations
- ✅ Add new snapshot types by extending the `SnapshotType` enum
- ✅ Extend EncodeBit format for new object types
- ✅ Add new global constants to SnapshotEnv
- ✅ Optimize object encoding/decoding algorithms
- ✅ Extend snapshot file format for new metadata
- ✅ Add new space types for memory layout variations
- ✅ Implement custom snapshot handlers for specific object types

### Prohibited Operations
- ❌ Do NOT modify the EncodeBit format without considering backward compatibility
- ❌ Do NOT change snapshot file format without updating version numbers
- ❌ Do NOT serialize unsupported object types (internal GC structures, etc.)
- ❌ Do NOT bypass region-based memory allocation during deserialization
- ❌ Do NOT use snapshot for object persistence - it's for startup optimization only
- ❌ Do NOT modify global constants in SnapshotEnv during VM execution
- ❌ Do NOT serialize cross-VM references (objects from different VM instances)
- ❌ Do NOT load snapshots from different runtime versions without version checking
- ❌ Do NOT ignore signal handlers for snapshot error recovery
- ❌ Do NOT delete snapshot files while VM is running (use ModulesSnapshotHelper)
