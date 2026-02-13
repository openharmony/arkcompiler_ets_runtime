 # AGENTS

This file provides guidance for AI agents when working with code in the debugger component.

## Overview

This is the **debugger module** for the ArkCompiler ECMAScript runtime. It provides debugging capabilities including breakpoints, single stepping, stack inspection, hot reload, and call frame manipulation for the OpenHarmony ArkCompiler JS runtime.

## Architecture

### Core Components

- **`js_debugger.h/cpp`**: Main debugger implementation implementing `JSDebugInterface` and `RuntimeListener`. Handles breakpoint management, single stepping, and debugger statements.
- **`js_debugger_manager.h/cpp`**: Central coordinator for debugger functionality. Manages debug mode, hot reload integration, drop frame functionality, and notification system. 
- **`debugger_api.h/cpp`**: Public API for debugger operations. Provides static methods for stack inspection, method/bytecode access, file metadata, scope/environment handling, and property access.
- **`js_debugger_interface.h`**: Interface definitions. Defines:
  - `PtHooks`: Callback interface for VM events (breakpoints, module loading, VM lifecycle)
  - `JSDebugInterface`: Core debugging interface for breakpoint management
  - `PauseReason`: Enumeration of why execution paused (breakpoint, exception, step, etc.)
- **`js_pt_location.h`**: Program location tracking (source file, bytecode offset)
- **`js_pt_method.h`**: Method-level debugging support
- **`notification_manager.h`**: Event notification system for debugger events

### Key Data Structures

- **`JSBreakpoint`**: Represents a breakpoint with source file, method, bytecode offset, and optional condition function
- **`JSPtLocation`**: Encapsulates a program location (file path and bytecode offset)
- **`PtMethod`**: Method handle for debugging operations
- **`PauseReason`**: Enum indicating why execution paused (BREAKPOINT, EXCEPTION, STEP, DEBUGGERSTMT, etc.)

## Building

This project uses the **GN (Generate Ninja)** build system, integrated with the larger OpenHarmony/ArkCompiler build infrastructure.

### Build Commands (from OpenHarmony repository root)

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

Tests are organized in `tests/BUILD.gn`:

- **RuntimeDebuggerTest** (`js_debugger_test.cpp`) - Main debugger functionality tests
- **DropframeManagerTest** (`dropframe_manager_test.cpp`) - Drop frame manipulation tests
- **HotReloadManagerTest** (`hot_reload_manager_test.cpp`) - Hot reload tests

### Running Tests

From the `ark_standalone_build` directory, you can use the standalone build commands without pulling the full OpenHarmony code:

```bash
# Compile and run specific test (debug build)
python ark.py x64.debug RuntimeDebuggerTestAction
```
