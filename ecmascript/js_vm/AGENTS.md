# ecmascript/js_vm

Command-line virtual machine tool for executing ArkCompiler bytecode (.abc files) with configurable runtime options.
Primary Language: C++

## Design

```
  ark_js_vm [options] file.abc
       │
       ▼
  main.cpp
       ├── Parse command-line options
       ├── Create EcmaVM with options
       ├── Load .abc file via JSPandaFileManager
       ├── Execute main function
       └── Cleanup and exit
```

**Supported options:**
- `--help`: Show usage information
- `--verbose`: Enable verbose logging
- `--gc-threads`: Set GC thread count
- `--enable-ic`: Enable inline cache
- Various runtime and debugging flags

## Reference Source
- `arkcompiler/ets_runtime/ecmascript/js_vm/main.cpp` (1-300行) - CLI entry point

## Key Files

| File | Role |
|------|------|
| `main.cpp` | CLI entry point and option parsing |
| `BUILD.gn` | Build configuration for ark_js_vm executable |

## Building

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.release ark_js_vm
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ark_js_host_linux_tools_packages
```

## Usage

```bash
# Execute ABC file
out/x64.release/arkcompiler/ets_runtime/ark_js_vm program.abc

# With options
out/x64.release/arkcompiler/ets_runtime/ark_js_vm --verbose --gc-threads=4 program.abc
```

## Tests

Integration tests in `../../test/` verify ark_js_vm behavior.

## Boundaries

- This is a CLI tool; do not add library APIs here.
- Keep option parsing simple; use external libraries if needed.
- All runtime logic is in other ecmascript/ modules.
