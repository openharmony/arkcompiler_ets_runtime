# AGENTS

This file provides guidance for AI agents when working with code in the hprof component.

## Project Overview

This is the **hprof (heap profiling)** component of the OpenHarmony ArkCompiler ECMAScript runtime. The hprof module provides comprehensive heap analysis capabilities for JavaScript memory debugging and optimization in the OpenHarmony ecosystem.

## Architecture

The hprof component is structured around several key subsystems:

### Core Profiling Components
- **HeapProfiler** (`heap_profiler.h/cpp`) - Main heap analysis implementation using singleton pattern with `HeapProfilerInterface`
- **HeapSnapshot** (`heap_snapshot.h/cpp`) - Captures heap state at specific points with object graph traversal
- **HeapTracker** (`heap_tracker.h/cpp`) - Real-time heap monitoring for allocation events and GC object movement
- **HeapSampling** (`heap_sampling.h/cpp`) - Statistical sampling for allocation profiling

### Data Model
- **Node/Edge System** - Object graph representation with `NodeType` enum (ARRAY, STRING, OBJECT, CODE, CLOSURE, etc.) and `EdgeType` for relationships
- **EntryIdMap** - Maps heap object addresses to unique node IDs, handles object relocation during GC
- **Visitor Pattern** - `HeapRootVisitor` traverses heap root objects for graph construction

### Output and Serialization
- **Stream Architecture** - Abstract `Stream` interface with `FileStream` for file-based output
- **JSON Serialization** - `HeapSnapshotJsonSerializer` produces json-format heap snapshot
- **Raw Heap Processing** - `rawheap_dump.h/cpp` for low-level binary heap dump handling

### Utilities
- **rawheap_translate/** - Standalone tool for translating raw heap snapshot to a json-format snapshot
- **StringHashMap** - Custom string deduplication and storage
- **CallFrameInfo** - Stack trace capture for allocation samples

### Memory Management Integration
- Integrates with GC through `HeapMarker` for object liveness tracking
- Handle leak detection (local and global handles)
- Automatic heap dump on Out-Of-Memory conditions
- Custom allocators (`CUnorderedMap`, `CVector`) for performance

## Building

This project uses the **GN (Generate Ninja)** build system, integrated with the larger OpenHarmony/ArkCompiler build infrastructure.

### Build Commands

Full OpenHarmony build (from repository root):
```bash
# Host tools (Linux x64)
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages

# Device targets (libark_jsruntime shared object)
./build.sh --product-name rk3568 --build-target libark_jsruntime

# Add --gn-args is_debug=true for debug builds
```

Standalone build (from `ark_standalone_build/`):
```bash
# Build VM tools
python ark.py x64.release ark_js_vm
```

## Testing

### Test Structure

Tests are organized into multiple test suites in `tests/BUILD.gn`:

- **HeapDumpTest**
- **HeapSnapShotTest**
- **HeapTrackerFirstTest/SecondTest/ThirdTest**
- **HeapSamplingTest**
- **HProfTest**
- **RawHeapTranslateTest**
- **JSMetadataTest**
- **LocalHandleLeakDetectTest** / **GlobalHandleLeakDetectTest**

### Running Tests

From the `ark_standalone_build` directory, you can use the standalone build commands without pulling the full OpenHarmony code:

```bash
# Compile and run specific test (debug build)
python ark.py x64.debug HeapDumpTestAction
python ark.py x64.debug HeapSnapShotTestAction

# Compile and run all ets_runtime tests (release build)
python ark.py x64.release ut
```
