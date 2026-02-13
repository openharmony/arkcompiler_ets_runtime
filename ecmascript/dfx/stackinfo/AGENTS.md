# AGENTS

This file provides guidance for AI agents when working with code in the stackinfo component.

## Project Overview

This is the **stackinfo component** of the OpenHarmony ArkCompiler ECMAScript runtime, part of the DFX. The module provides JavaScript stack tracing and debugging capabilities for the OpenHarmony ecosystem, enabling developers to capture and analyze call stacks in both synchronous and asynchronous contexts.

## Architecture

The stackinfo component is structured around several key subsystems:

### Core Stack Trace Components
- **JsStackInfo** (`js_stackinfo.h/cpp`) - Main stack trace builder with `BuildJsStackTrace()` and `BuildJsStackInfo()` APIs. Handles frame traversal and source mapping.
- **JSStackTrace** - Singleton class managing JSPandaFile lookup and MethodInfo caching for bytecode PC translation
- **JSSymbolExtractor** - Extracts debug information (line numbers, column numbers, function names) from compiled bytecode and source maps
- **JsStackGetter** (`js_stackgetter.h/cpp`) - Utilities for frame type identification and method info parsing

### Frame Type Identification
The system identifies multiple frame types through frame type checking:
- **JavaScript frames** - Interpreted bytecode execution
- **Native frames** - C++ builtin functions
- **AOT frames** - Ahead-of-time compiled code (stored in .aot files)
- **Fast JIT frames** - Just-in-time compiled code (fast path)
- **Baseline JIT frames** - Baseline JIT compiled code

### Async Stack Trace Components

#### Debug State Components
- **StackFrame** - Single stack frame with function name, URL, line/column numbers
- **AsyncStack** - Links async frames to parent promises (weak_ptr for safety)
- **AsyncStackTrace** (`async_stack_trace.h/cpp`) - Manages promise chain stack tracking in debug mode

#### Runtime State Components
- **PromiseNode** - Promise relationship node (promiseId, parentPromiseId, stackId)
- **AsyncStackTraceManager** (`async_stack_trace.h/cpp`) - Runtime async stack trace builder that traverses promise chains

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

Tests are in `tests/BUILD.gn`:
- **JsStackInfoTest** (`js_stackinfo_test.cpp`, `js_stackinfo_test2.cpp`) - Core stack trace functionality
- **AsyncStackTest** (`async_stack_test.cpp`) - Async stack trace with promise chains

### Running Tests

```bash
# Standalone build (preferred for development)
python ark.py x64.debug JsStackInfoTestAction
python ark.py x64.debug AsyncStackTestAction

# Full test suite
python ark.py x64.release ut
```
