# AGENTS
**Name**: Serializer
**Purpose**: Serializer is a JavaScript object serialization/deserialization module for the ArkTS runtime. It implements the structured clone algorithm for transmitting JavaScript objects across threads or processes. Main features include:
- Structured serialization/deserialization of JavaScript values
- Support for Transferable objects (ArrayBuffer transfer)
- Support for Shared objects (SharedArrayBuffer, Sendable objects)
- Native binding object serialization with detach/attach callbacks
- Inter-Op serialization for cross-language scenarios
- Module snapshot serialization for fast startup
- CMC (Concurrent Mark-Compact) garbage collector compatibility
- Memory space-aware object allocation during deserialization
**Primary Language**: C++

## Directory Structure
```text
serializer/
├── base_serializer.h                                          # Base serializer class definition
├── base_serializer.cpp                                        # Base serializer implementation
├── base_serializer-inl.h                                      # Inline implementations for serialization
├── base_deserializer.h                                        # Base deserializer class definition
├── base_deserializer.cpp                                      # Base deserializer implementation
├── value_serializer.h                                         # Value serializer for structured clone
├── value_serializer.cpp                                       # Value serializer implementation
├── inter_op_value_serializer.h                                # Inter-Op value serializer
├── inter_op_value_serializer.cpp                              # Inter-Op value serializer implementation
├── inter_op_value_deserializer.h                              # Inter-Op value deserializer
├── inter_op_value_deserializer.cpp                            # Inter-Op value deserializer implementation
├── module_serializer.h                                        # Module snapshot serializer
├── module_serializer.cpp                                      # Module snapshot serializer implementation
├── module_deserializer.h                                      # Module snapshot deserializer
├── module_deserializer.cpp                                    # Module snapshot deserializer implementation
├── serialize_data.h                                           # Serialization data container and encoding flags
├── serialize_chunk.h                                          # Serialization chunk for shared objects
└── tests/                                                     # Unit tests directory
    ├── BUILD.gn                                               # Test build configuration
    ├── serializer_test.cpp                                    # General serialization tests
    ├── inter_op_serializer_test.cpp                           # Inter-Op serialization tests
    └── cmc_serializer_test.cpp                                # CMC GC serialization tests
```

## Building
The serializer module is built as part of the ArkCompiler module in the GN build system. There is no independent BUILD.gn file in the serializer directory. The source files are included in the main ecmascript BUILD.gn configuration:
```bash
# Build for OHOS ARM64
./build.sh --product-name rk3568 --gn-args use_musl=true --target-cpu arm64 --build-target ark_js_packages

# Build for OHOS ARM32
./build.sh --product-name rk3568 --build-target ark_js_packages

# Debug build (add to any command)
--gn-args is_debug=true
```

## Test Suite
The serializer module has comprehensive unit tests in the tests/ directory:
```bash
# Build and generate an executable file
./build.sh --product-name rk3568 --build-target SerializerTest --build-target InterOpSerializerTest --build-target CMCSerializerTest
# Send the executable file to the device using hdc
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/SerializerTest /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/InterOpSerializerTest /data/local/tmp/
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/CMCSerializerTest /data/local/tmp/
# Run specific serializer tests
hdc shell
cd /data/local/tmp/
chmod 777 ./SerializerTest
./SerializerTest
chmod 777 ./InterOpSerializerTest
./InterOpSerializerTest
chmod 777 ./CMCSerializerTest
./CMCSerializerTest
```

Test coverage includes:
- **serializer_test.cpp**: Basic value serialization, special values, objects, arrays, typed arrays, ArrayBuffers, SharedArrayBuffers, native objects
- **inter_op_serializer_test.cpp**: Cross-language serialization scenarios
- **cmc_serializer_test.cpp**: CMC garbage collector compatibility tests

## Dependency
### Package Dependencies
| Dependency | Purpose |
|------------|---------|
| `ecmascript/js_thread` | JSThread for current thread context |
| `ecmascript/ecma_vm` | EcmaVM for VM management |
| `ecmascript/mem/heap` | Heap for memory allocation |
| `ecmascript/mem/sparse_space` | Sparse space memory management |
| `ecmascript/mem/shared_heap` | SharedHeap for shared object serialization |
| `ecmascript/shared_mm/shared_mm` | Shared memory management |
| `ecmascript/snapshot/mem/snapshot_env` | Snapshot environment configuration |
| `ecmascript/runtime` | Runtime lifecycle management |
| `ecmascript/napi/include/jsnapi_expo.h` | N-API serialization interface |
| `common_components/heap/heap` | Common heap utilities |
| `common_components/serialize/serialize_utils` | Serialization utility functions |
| `icu:shared_icui18n`, `icu:shared_icuuc` | ICU internationalization |
| `zlib:libz` | Compression library |
| `sec:libz` | Security shared library |

### External Directory Interactions
- `../mem/` - Memory management subsystem (Heap, spaces, allocators)
- `../mem/shared_heap/` - Shared heap for concurrent GC scenarios
- `../napi/` - N-API layer for external serialization interface
- `../module/` - Module serialization integration
- `../snapshot/` - Snapshot format compatibility
- `../../common_components/heap/` - Shared heap utilities
- `../../common_components/serialize/` - Common serialization utilities

### N-API Interface Integration
The serializer is exposed through N-API in [ecmascript/napi/include/jsnapi_expo.h](../napi/include/jsnapi_expo.h):
- `JSNApi::SerializeValue()` - Serialize JavaScript value
- `JSNApi::DeserializeValue()` - Deserialize JavaScript value
- `JSNApi::InterOpSerializeValue()` - Inter-Op serialization
- `JSNApi::InterOpDeserializeValue()` - Inter-Op deserialization
- `JSNApi::DeleteSerializationData()` - Free serialization data

### Key Data Structures
**SerializeData**: Container for serialized data with dynamic buffer expansion
- `EncodeFlag`: Enumeration for encoding different value types (NEW_OBJECT, REFERENCE, WEAK, PRIMITIVE, ARRAY_BUFFER, SHARED_ARRAY_BUFFER, etc.)
- `SerializedObjectSpace`: Enumeration for different memory spaces (OLD_SPACE, SHARED_OLD_SPACE, NON_MOVABLE_SPACE, etc.)

**SerializationChunk**: Efficient storage for shared object references during serialization

## Boundaries
### Allowed Operations
- ✅ Add new serializer types by extending `BaseSerializer`/`BaseDeserializer`
- ✅ Add new encoding flags for new object types in `EncodeFlag` enum
- ✅ Implement custom serialization logic for specific object types
- ✅ Extend memory space support for new heap configurations
- ✅ Optimize memory allocation strategies in deserializer
- ✅ Add new native binding types with proper detach/attach semantics

### Prohibited Operations
- ❌ Do NOT violate the structured clone algorithm specification
- ❌ Do NOT serialize unsupported internal types (HClass, internal symbols, etc.)
- ❌ Do NOT bypass memory space allocation logic - objects must be allocated to correct spaces
- ❌ Do NOT hardcode buffer size limits - use configurable limits
- ❌ Do NOT leak native bindings - all detached objects must have finalizers called
- ❌ Do NOT deserialize into incorrect memory spaces (shared vs local)
- ❌ Do NOT modify reference counting for SharedArrayBuffer without proper synchronization
- ❌ Do NOT use the serializer for object persistence (it's for transmission, not storage)
- ❌ Do NOT serialize objects with circular references across VM boundaries
