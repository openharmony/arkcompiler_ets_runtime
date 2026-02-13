# AGENTS

This file provides guidance for AI agents when working with code in the extractortool component.

## Overview

ExtractorTool is a C++ component of the OpenHarmony ArkCompiler ETS Runtime that handles file extraction from HAP (OpenHarmony Application Package) files and ZIP archives. It provides memory-mapped file access, source map processing for JavaScript debugging, and utilities for path manipulation in the OpenHarmony ecosystem.

## Architecture

### Core Components

- **`extractor.h/cpp`**: Main interface for file extraction operations from HAP/ZIP files. Detects and validates HAP packages, supports both stage model and FA (Feature Ability) model. Provides file listing, extraction to filesystem or memory buffers. Thread-safe caching via `ExtractorUtil` for performance.
- **`zip_file.h/cpp`**: Low-level ZIP archive parser using minizip library. Parses ZIP structure (local file headers, central directory, end of central directory). Handles both stored and deflated compression methods. Provides efficient file access without full extraction.
- **`file_mapper.h/cpp`**: Abstraction for memory-mapped file access. Three mapping types: `NORMAL_MEM` (standard heap allocation), `SHARED_MMAP` (shared memory mapping via mmap), `SAFE_ABC` (special safe region mapping for ABC bytecode files). Provides automatic decompression when accessing compressed data.
- **`source_map.h/cpp`**: Processes JavaScript source maps for debugging support. Translates generated positions back to original source locations. Handles VQl (Variable-Length Quantity) encoded mappings. Manages source map data from multiple packages.
- **`file_path_utils.h/cpp`**: Path manipulation utilities specific to OpenHarmony. Handles NPM package resolution and OHM (OpenHarmony Module) URI processing. Bundle path constants defined in `constants.h`.
- **`zip_file_reader.h/cpp`**: Interface for ZIP file readers with two implementations:
  - `zip_file_reader_io.cpp`: File-based ZIP reading
  - `zip_file_reader_mem.cpp`: Memory-based ZIP reading

### Key Data Structures

- **`FileInfo`**: Contains file metadata including filename, offset, length, and modification time
- **`FileMapperType`**: Enum for mapping types (`NORMAL_MEM`, `SHARED_MMAP`, `SAFE_ABC`)
- **`SourceMapInfo`**: Stores source map position information (before/after row/column, sources/names indices)
- **`MappingInfo`**: Represents line/column mapping information
- **`SourceMapData`**: Complete source map data including sources, package name, position mappings

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

Tests are organized in `tests/BUILD.gn`:

- **ExtractorToolTest** - Main test executable including:
  - `extractor_test.cpp` - Extractor functionality tests
  - `zip_file_test.cpp` - ZIP file parsing tests
  - `source_map_test.cpp` - Source map processing tests

### Running Tests

From the `ark_standalone_build` directory, you can use the standalone build commands:

```bash
# Compile and run specific test (release build)
python ark.py x64.release ExtractorToolTestAction

# Compile and run specific test (debug build)
python ark.py x64.debug ExtractorToolTestAction
```
