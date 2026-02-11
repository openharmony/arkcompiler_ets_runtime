# ecmascript/stackmap

Builds and parses stackmap metadata that records where GC roots and deoptimization values live (registers, stack slots) at each safepoint in compiled code, with backend-specific parsers for LLVM and Maple/LiteCG formats.
Primary Language: C++

## Design

The compiler backends (LLVM, Maple) each produce stackmap data in their own format. This module provides a unified Ark stackmap format and backend-specific parsers:

```
  Compiler backend emits stackmaps
       │
       ├── LLVM backend → LLVM stackmap section
       │       │
       │       ▼
       │   LLVMStackMapParser → parse LLVM format
       │
       └── Maple/LiteCG backend → LiteCG stackmap
               │
               ▼
           LiteCGStackMapType → parse LiteCG format
       │
       ▼
  ArkStackMapBuilder
    └── Converts backend-specific data → unified Ark stackmap format
       │
       ▼
  Ark stackmap (embedded in .an file)
       │
       ▼
  ArkStackMapParser (at runtime)
    └── Queries: "at PC offset X, where are the GC roots / deopt values?"
```

**Two consumers of stackmaps:**
1. **GC** — Needs to know which stack slots / registers hold object references at safepoints.
2. **Deoptimizer** — Needs to recover virtual register values from physical locations at deopt points.

## Key Files

| File | Role |
|------|------|
| `ark_stackmap_builder.cpp/.h` | Builds unified Ark stackmap from backend data |
| `ark_stackmap_parser.cpp/.h` | Runtime parsing: PC offset → location queries |
| `ark_stackmap.h` | Ark stackmap data structures |
| `cg_stackmap.h` | Backend-agnostic stackmap info interface |
| `llvm/llvm_stackmap_parser.cpp/.h` | LLVM stackmap section parser |
| `llvm/llvm_stackmap_type.cpp/.h` | LLVM stackmap type definitions |
| `litecg/litecg_stackmap_type.cpp/.h` | LiteCG stackmap type definitions |

## How to Modify

**Add a new stackmap entry type:**
1. Add the type to `ark_stackmap.h`
2. Update `ArkStackMapBuilder` to emit the new type
3. Update `ArkStackMapParser` to parse it
4. Update the relevant backend parser (`llvm/` or `litecg/`) to provide the data

## Building

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.debug
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ark_js_host_linux_tools_packages
```

## Tests

Run all unit tests: `python ark.py x64.debug unittest` (from repo root `../../`)

No dedicated test directory. Coverage through:
- Compiler host tests: `ecmascript/compiler/tests/`
- JIT tests: `ecmascript/jit/tests/`
- Runtime deoptimization and GC tests

## Boundaries

- Keep backend-specific parsing isolated: LLVM in `llvm/`, LiteCG in `litecg/`.
- Binary layout must stay consistent between `ArkStackMapBuilder` (write) and `ArkStackMapParser` (read).
- Stackmap interfaces must stay aligned with `deoptimizer/` (for value recovery) and `trampoline/` (for frame metadata).
- Sources are part of `ecma_source` in root `BUILD.gn`.
