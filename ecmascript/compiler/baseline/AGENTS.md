# ecmascript/compiler/baseline

A fast, non-optimizing compiler that translates bytecode directly to machine code without going through the full Circuit IR and optimization pass pipeline, providing faster compilation at the cost of less-optimized output.
Primary Language: C++

## Design

The baseline compiler sits between the interpreter and the optimizing compiler in the execution tier hierarchy:

```
  ABC bytecode
       │
       ▼
  BaselineCompiler
    ├── Iterates bytecodes sequentially
    ├── Uses BaselineAssembler to emit code per-bytecode
    └── Calls into BaselineStubs for complex operations
       │
       ▼
  Machine code (installed directly, no .an file)
```

**Key design decisions:**
- **No IR** — Bytecodes are lowered directly to machine instructions via `BaselineAssembler`, skipping Circuit IR entirely.
- **Stub-based** — Complex operations (e.g., type checks, property access) call into `BaselineStubs` rather than being inlined, keeping the compiler simple.
- **Architecture-dependent** — Uses the `assembler/` module's `MacroAssembler` for instruction emission.

**Key abstractions:**
- **BaselineCompiler** — Main compilation driver; walks bytecodes and dispatches to assembler helpers.
- **BaselineAssembler** — Bytecode-to-machine-code emission helpers.
- **BaselineStubBuilder / BaselineStubs** — Runtime helper stubs specific to baseline-compiled code.
- **BaselineCallSignature** — Calling conventions for baseline stubs.

## Key Files

| File | Role |
|------|------|
| `baseline_compiler.cpp/.h` | Main compilation flow |
| `baseline_assembler.cpp/.h` | Code emission helpers |
| `baseline_stub_builder.cpp/.h` | Stub construction |
| `baseline_stubs.cpp/.h` | Runtime stub definitions |
| `baseline_call_signature.cpp/.h` | Calling convention definitions |
| `baseline_compiler_builtins.h` | Built-in operation definitions |

## How to Modify

**Add baseline support for a new bytecode:**
1. Add the emission case to `BaselineCompiler` (in `baseline_compiler.cpp`)
2. If it needs a stub, add it to `BaselineStubs` and register via `BaselineCallSignature`
3. Implement architecture-specific call in `compiler/trampoline/{aarch64,x64}/baseline_call.cpp`

**Add a new baseline stub:**
1. Define the stub signature in `baseline_call_signature.cpp`
2. Implement the stub in `baseline_stubs.cpp` or `baseline_stub_builder.cpp`
3. Add call-path in `compiler/trampoline/{aarch64,x64}/baseline_call.cpp`

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

No dedicated baseline tests. Coverage through:
- Aggregate runtime tests: `ark_js_unittest`
- Related call-path code: `compiler/trampoline/{aarch64,x64}/baseline_call.cpp`

Add baseline-focused tests to `ecmascript/compiler/tests/BUILD.gn`.

## Boundaries

- Keep baseline logic separate from AOT/JIT optimization passes — baseline does no optimization.
- ABI/signature changes must be synchronized with `trampoline/` and `call_signature*` definitions.
- Compiled as part of `libark_jsoptimizer_sources` in `ecmascript/compiler/BUILD.gn`.
