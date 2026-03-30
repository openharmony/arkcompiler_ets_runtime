# AGENTS

This file provides guidance for AI agents when working with code in the CPU performance analysis component.

## Project Overview

This is the **CPU performance analysis** of the OpenHarmony ArkCompiler ECMAScript runtime. It provides statistical sampling-based profiling capabilities for JavaScript execution analysis in the OpenHarmony ecosystem. It is part of the larger DFX module.

## Architecture

The CPU performance analysis tool uses a signal-based sampling approach to periodically capture JavaScript stack traces during execution.

### Core Components

- **CpuProfiler** (`cpu_profiler.h/cpp`) - Main performance analysis controller that manages profiling lifecycle
  - Starts/stops profiling sessions
  - Configures sampling interval (default: 500 microseconds)
  - Signal handler (SIGPROF) for asynchronous stack capture
  - Manages profiling state (GC, NAPI calls, runtime states)
  - Supports both in-memory and file-based output

- **SamplesRecord** (`samples_record.h/cpp`) - Sample data aggregation and serialization
  - Maintains circular queue (`SamplesQueue`) for thread-safe sample collection
  - Constructs profile tree structure with `CpuProfileNode` nodes
  - Tracks execution time statistics per state (GC, interpreter, AOT, JIT, NAPI, etc.)
  - JSON serialization for browser developer tools-compatible output
  - Source map translation support for original source positions
  - NAPI stack reconstruction (跨语言调用栈)

- **SamplingProcessor** (`sampling_processor.h/cpp`) - Background sampling thread
  - Runs in dedicated pthread with configurable interval
  - Uses semaphores for synchronization with signal handler
  - Posts samples to `SamplesQueue` for processing

### Data Structures

- **FrameInfo** - Encapsulates frame metadata (scriptId, line, column, function name, URL, package)
- **CpuProfileNode** - Tree node representing a call frame with children and hit counts
- **ProfileInfo** - Complete profile with nodes array, samples, timeDeltas, and time statistics
- **MethodKey** - Unique identifier for stack frames combining method address + running state + deopt type
- **NodeKey** - Composite key for profile tree nodes (methodKey + parentId)

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

Located in `tests/` directory:
- `cpu_profiler_test.cpp` - Lifecycle and configuration tests
- `samples_record_test.cpp` - Sample collection and serialization tests

Test target: `CpuProfilerTest` (defined in `tests/BUILD.gn`)

### Running Tests

From the `ark_standalone_build` directory, you can use the standalone build commands without pulling the full OpenHarmony code:

```bash
cd ark_standalone_build
python ark.py x64.debug CpuProfilerTestAction  # Single test (debug build)
python ark.py x64.release ut                   # All tests (release build)
```
