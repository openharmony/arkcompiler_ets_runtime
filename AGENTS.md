# ArkTS Runtime (ets_runtime)

The ArkTS Runtime is the default runtime for executing ArkTS, TypeScript, and JavaScript code on OpenHarmony. It implements the ECMAScript 2021 standard in strict mode and provides native APIs for C++ interoperability (NAPI), efficient container libraries, and high-performance garbage collectors.

## Project Overview

- **Repository**: `arkcompiler/ets_runtime`
- **Version**: 1.2.0 (ArkTS-Sta - Static version)
- **License**: Apache License 2.0
- **Language**: C++ (runtime), GN build system
- **Standards**: ES2021 with strict mode only
- **Related Repositories**:
  - `arkcompiler_runtime_core`: Core runtime components
  - `arkcompiler_ets_frontend`: Compiler toolchain (ts2abc, es2abc)

## Architecture

The runtime consists of four main subsystems:

### Core Subsystem
Foundational runtime libraries that are language-agnostic:
- **File component**: Carries bytecode (.abc files - ArkCompiler bytecode)
- **Tooling component**: Debugger support
- **Base library**: System call adaptations

### Execution Subsystem
- **Interpreter**: Executes bytecode
- **Inline Cache (IC)**: Optimizes property access
- **Profiling**: Captures runtime information for optimization

### Compiler Subsystem
- **Stub Compiler**: Quick code generation
- **Circuit Framework**: Intermediate representation
- **AOT Compiler**: Ahead-of-Time compilation to machine code

### Runtime Subsystem
- **Memory Management**: Object allocator and GC (CMS-GC, Partial-Compressing-GC)
- **Analysis Tools**: DFX, CPU and heap profiling
- **Concurrency**: Actor model with TaskPool API
- **Standard Libraries**: ECMAScript standard libraries
- **Type System**: TypeScript type metadata and loading
- **NAPI**: Native API for C++ interoperability

## Key Features

- **Native Type Support**: TypeScript type information preserved from compilation to runtime, enabling inline cache pre-generation and AOT compilation
- **Optimized Concurrency**: TaskPool with priority-based scheduling and automatic worker scaling
- **Security**: Static pre-compilation with code obfuscation; no sloppy mode or eval support

## Directory Structure

```
/arkcompiler/ets_runtime
+-- ecmascript/              # Core ArkTS runtime implementation
|   +-- base/                # Base helper classes and utilities
|   +-- builtins/            # ECMAScript standard library implementations
|   +-- compiler/            # AOT compiler and type inference
|   +-- interpreter/         # Bytecode interpreter
|   +-- jit/                 # Just-in-time compiler
|   +-- ic/                  # Inline cache module for optimization
|   +-- jspandafile/         # .abc bytecode file management
|   +-- mem/                 # Memory management and allocation
|   +-- napi/                # C++ native API bindings (JSNAPI)
|   +-- module/              # ES module loader
|   +-- require/             # CommonJS module loader
|   +-- ts_types/            # TypeScript type metadata handling
|   +-- js_type_metadata/    # Object layout definitions
|   +-- debugger/            # Debugger implementation
|   +-- regexp/              # Regular expression engine
|   +-- intl/                # Internationalization (i18n) support
|   +-- containers/          # Non-ECMAScript container libraries
|   +-- js_api/              # Non-standard object models
|   +-- ohos/                # OpenHarmony system-specific logic
|   +-- platform/            # Cross-platform abstraction layer
|   +-- taskpool/            # Concurrent task pool implementation
|   +-- jobs/                # Microtask queue
|   +-- shared_mm/           # Shared memory management
|   +-- shared_objects/      # Shared object implementation
|   +-- snapshot/            # Runtime snapshot for fast startup
|   +-- serializer/          # Serialization utilities
|   +-- stubs/               # Runtime stub functions
|   +-- stackmap/            # Variable location information
|   +-- deoptimizer/         # Deoptimization for compiled code
|   +-- checkpoint/          # VM safepoint implementation
|   +-- daemon/              # Shared GC concurrent threads
|   +-- dfx/                 # Debugging and performance analysis tools
|   +-- extractortool/       # Source map parsing
|   +-- patch/               # Hot/cold patching support
|   +-- pgo_profile/         # PGO profiling
|   +-- quick_fix/           # Quick fix command-line tool
|   +-- js_vm/               # Command-line VM tool (ark_js_vm)
|   +-- sdk/                 # SDK tool integration
|   +-- tests/               # Unit tests
|
+-- common_components/       # Shared components across runtime
|   +-- base/                # Basic utilities (memory, time, strings)
|   +-- heap/                # Garbage collection and memory heap
|   +-- common/              # Runtime common infrastructure
|   +-- thread/              # Threading and synchronization primitives
|   +-- taskpool/            # Task pool for concurrent execution
|   +-- platform/            # Platform-specific implementations
|
+-- compiler_service/        # AOT compiler service component
|   +-- include/             # Public API headers
|   +-- test/                # Compiler service tests
|
+-- test/                    # Integration and module test suites
|   +-- aottest/             # AOT compiler tests
|   +-- jittest/             # JIT compiler tests
|   +-- moduletest/          # Module system tests
|   +-- fuzztest/            # Fuzzing tests
|   +-- perf/                # Performance test suites
|
+-- tools/                   # Development and debugging tools
|   +-- ap_file_viewer/      # ABC file viewer
|   +-- circuit_viewer/      # Circuit IR visualizer
|
+-- docs/                    # Documentation
|   +-- overview.md          # Architecture overview
|   +-- README.md            # Usage guide
|   +-- environment-setup-and-compilation.md
|   +-- development-example.md
|   +-- using-the-toolchain.md
|   +-- aot-guide_zh.md
|   +-- figures/             # Architecture diagrams
|
+-- script/                  # Build and deployment scripts
+-- etc/                     # Configuration files
+-- bundle.json              # Component metadata and build configuration
+-- BUILD.gn                 # GN build system root file
```

## Build System

The project uses the GN (Generate Ninja) build system. Key build targets:

- `ark_js_packages`: Runtime libraries for target devices
- `ark_js_host_linux_tools_packages`: Tools for Linux host (ark_js_vm, ark_aot_compiler)
- `libark_jsruntime`: Main runtime library
- `libcompiler_service`: AOT compiler service library

## Build Commands

```bash
# Full runtime build from repository root (openharmony/ directory)
../../build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages

# With debug symbols from repository root (openharmony/ directory)
../../build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --gn-args is_debug=true

# AOT compiler build from repository root (openharmony/ directory)
../../build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages
```

## Key Tools

- **ark_js_vm**: Command-line virtual machine for executing .abc files
- **ark_aot_compiler**: AOT compiler for bytecode to machine code
- **ark_disasm**: Disassembler for .abc files (from runtime_core)
- **es2abc/ts2abc**: Frontend compilers (from ets_frontend)

## Execution Model

1. **Source Code** (.ts, .js) → **Frontend** (es2abc) → **Bytecode** (.abc)
2. **Bytecode** (.abc) → **Runtime** (ark_js_vm) → **Execution**
3. **Optional**: .abc → **AOT Compiler** → **Machine Code** (.an, .ai)

## Constraints

- Only ArkCompiler bytecode (.abc files) generated by ts2abc/es2abc can be executed
- ES2021 standard only, in strict mode
- No dynamic function creation (`new Function()`, `eval`)
- No sloppy mode support

## Documentation References

- [Runtime Overview](docs/overview.md): Detailed architecture description
- [Usage Guide](docs/README.md): Environment setup, compilation, and examples
- [Toolchain Guide](docs/using-the-toolchain.md): Frontend and AOT tool usage

## Component Relationships

```
┌─────────────────────────────────────────────────────────────┐
│                     OpenHarmony Application                 │
└─────────────────────────────┬───────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────┐
│                    ArkUI Framework                          │
└─────────────────────────────┬───────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────┐
│                  ets_runtime (this repo)                    │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐    │
│  │ Interpreter │  │   AOT/JIT    │  │    NAPI          │    │
│  │             │  │   Compiler   │  │  (C++ Binding)   │    │
│  └─────────────┘  └──────────────┘  └──────────────────┘    │
│  ┌─────────────┐  ┌──────────────┐  ┌──────────────────┐    │
│  │   Memory    │  │   Modules    │  │  Standard Libs   │    │
│  │  Management │  │   (ES/CJS)   │  │   (ECMA2021)     │    │
│  └─────────────┘  └──────────────┘  └──────────────────┘    │
└─────────────────────────────┬───────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────┐
│              runtime_core (Core Runtime Components)         │
└─────────────────────────────┬───────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────┐
│                 ets_frontend (Compiler Toolchain)           │
│                    (ts2abc, es2abc)                         │
└─────────────────────────────────────────────────────────────┘
```

## Development Notes
- The code comments in this repository should be written in English.
- The commit message should be written in English.
- Don't create commits directly. Have them reviewed.
