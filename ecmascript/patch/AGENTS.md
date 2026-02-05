 # AGENTS

This file provides guidance for AI agents when working with code in the patch component.

## Overview

This directory contains the **patch system** for the ArkCompiler ECMAScript runtime. The patch system enables runtime code replacement through three main mechanisms:

- **Hot Reload**: Live code updates during development without restarting the application
- **Cold Patch**: Loads patch files at application startup instead of the original files

## Architecture

### Core Components

- **`patch_loader.h/cpp`**: Core patch loading/unloading logic
- **`quick_fix_manager.h/cpp`**: Manages quick fix lifecycle
- **`quick_fix_helper.h/cpp`**: Helper utilities for quick fixes

### Key Data Structures

- **`PatchMethodIndex`**: Identifies methods to patch (recordName, className, methodName)
- **`BaseMethodIndex`**: Identifies original methods (constpoolNum, constpoolIndex, literalIndex)
- **`PatchInfo`**: Contains all patch metadata including method literals and constpools
- **`ReplacedMethod`**: Tracks which methods have been replaced

### Method Replacement Strategy

1. Methods are identified by `PatchMethodIndex` in the patch file
2. Original method info is saved using `BaseMethodIndex`
3. `ReplaceMethod()` updates:
   - Call fields
   - Literal info
   - Code entry points
   - Native pointers
   - Constant pools

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

## Testing

### Test Structure

Test cases are located in `/arkcompiler/ets_runtime/test/quickfix/`.

### Running Tests

From the `ark_standalone_build` directory:

```bash
cd ark_standalone_build
python ark.py x64.release ark_quickfix_test  # Quick fix tests (release build)
```
