# ecmascript/compiler

The optimizing compiler infrastructure: transforms ABC bytecode into optimized machine code through an IR-based pass pipeline, with pluggable backends (LLVM, Maple/LiteCG).
Primary Language: C++

## Design

### Compilation Pipeline

```
  ABC bytecode
       │
       ▼
  BytecodeCircuitBuilder  ──▶  Circuit IR (sea-of-nodes graph)
       │
       ▼
  PassManager runs passes:
    PGOTypeInfer → TypedBytecodeLowering → TypedHCRLowering →
    SlowpathLowering → MCRLowering → ValueNumbering →
    InstructionCombine → Scheduler → GraphLinearizer → ...
       │
       ▼
  CodeGenerator (dispatches to backend)
       │
       ├──▶ LLVMIRGeneratorImpl  → LLVM IR → machine code
       └──▶ LiteCGIRGeneratorImpl → Maple IR → machine code
       │
       ▼
  ElfBuilder → .an file (AOT) or direct install (JIT)
```

### Assembly Interpreter Flow

The assembly interpreter path uses the `assembler/` module to generate efficient bytecode dispatch code:

```
  ABC bytecode
       │
       ▼
  Interpreter stub generation
    ├── Uses MacroAssembler for bytecode dispatch
    ├── Calls runtime stubs for complex operations
    └── Executes through assembler-generated handlers
```

### Key Abstractions

- **Circuit** — A sea-of-nodes graph. Each node is a `Gate` with an opcode (`MachineType`, `GateType`), value inputs, control flow, and dependency edges.
- **PassManager** — Runs optimization passes sequentially on each method. Each pass gets a `PassData` containing the `Circuit`, `BytecodeCircuitBuilder`, and `PassContext`.
- **Pass** (in `pass.h`) — Individual optimization. Concrete passes include `ConstantFolding`, `ValueNumbering`, `TypedBytecodeLowering`, etc.
- **CodeGenerator / CodeGeneratorImpl** — Abstract backend interface. `LLVMIRGeneratorImpl` and `LiteCGIRGeneratorImpl` are the two implementations.
- **CallSignature** — Defines calling conventions for runtime calls, stubs, and compiler builtins.

### Entry Points

- **AOT**: `aot_compiler.cpp` → `AotCompilerPreprocessor` → `PassManager::Compile()` for each method
- **JIT**: `jit_compiler.cpp` → `JitCompilationEnv` → `PassManager::Compile()` for hot method
- **Stubs**: `stub_compiler.cpp` → generates runtime stub code into stub `.an` file

### Compilation Types

This module supports three compilation/execution paths:
- **AOT Compiler** — Ahead-of-time compilation that optimizes applications before execution.
- **JIT Compiler** — Just-in-time compilation that compiles hot methods during execution.
- **Assembly Interpreter** — Fast interpreter path built with assembler primitives for bytecode execution.

## Key Files

| File | Role |
|------|------|
| `pass_manager.cpp` | Pass pipeline orchestration and per-method compilation |
| `circuit.h` / `gate.h` | IR graph container and node representation |
| `bytecode_circuit_builder.cpp` | ABC bytecode → Circuit IR translation |
| `pass.h` | All pass class definitions |
| `aot_compiler.cpp` | AOT compilation driver |
| `jit_compiler.cpp` | JIT compilation entry |
| `stub_compiler.cpp` | Stub generation entry |
| `call_signature.cpp` | Calling convention definitions |
| `code_generator.h` | Backend dispatch interface |
| `BUILD.gn` | Source ownership; defines `libark_jsoptimizer_sources` |

## How to Modify

**Add a new optimization pass:**
1. Create `your_pass.cpp` / `your_pass.h` in this directory
2. Define a class inheriting from the pass pattern in `pass.h`
3. Add the pass invocation to `PassManager::Compile()` in `pass_manager.cpp` at the correct pipeline position
4. Add source files to `libark_jsoptimizer_sources` in `BUILD.gn`
5. Add tests in `tests/` via `host_unittest_action` in `tests/BUILD.gn`

**Add a new Gate opcode:**
1. Add the opcode to the opcode list in `gate.h` (or the relevant `*_gate.h`)
2. Add lowering/handling in the relevant pass(es)
3. Add backend emission in both `codegen/llvm/` and `codegen/maple/`

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

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.debug unittest   # all unit tests (includes compiler tests)
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/compiler/tests:host_unittest
```

- Compiler pass tests: `tests/`
- Assembler tests: `assembler/tests/`
- AOT file tests: `aot_file/tests/`
- Maple backend tests: `codegen/maple/test/`

Add new tests as `*_test.cpp` under `tests/` and register via `host_unittest_action` in `tests/BUILD.gn`.

## Boundaries

- Keep pass ordering changes consistent with `pass_manager.cpp` and validate with tests.
- Keep backend-specific logic inside `codegen/maple/` or `codegen/llvm/` — do not mix backend internals into pass files.
- Calling-convention changes must be synchronized across `call_signature*`, `rt_call_signature*`, and `trampoline/`.
- Source ownership is defined in `BUILD.gn` — keep it in sync when adding files.
