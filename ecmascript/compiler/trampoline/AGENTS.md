# ecmascript/compiler/trampoline

Generates architecture-specific call bridges (trampolines) at compile time using the assembler infrastructure, handling transitions between the interpreter, baseline code, optimized code, and runtime stubs.
Primary Language: C++ (assembly-oriented)

## Design

Unlike `ecmascript/trampoline/` (handwritten assembly `.S` files), this module generates trampolines programmatically in C++ using the `assembler/` module:

```
  StubCompiler / StubBuilder
       │
       ▼
  Trampoline generators (this module)
    ├── CommonCall      → shared call helpers (frame setup/teardown)
    ├── OptimizedCall   → calls into AOT/JIT compiled code
    ├── OptimizedFastCall → fast-path optimized calls
    ├── AsmInterpreterCall → interpreter dispatch bridges
    └── BaselineCall    → baseline compiler entry/exit
       │
       ▼
  MacroAssembler (from assembler/)
       │
       ▼
  Machine code emitted into stub .an file
```

Each trampoline type is implemented twice — once in `aarch64/` and once in `x64/` — with a common interface through `common_call.h`.

**Key responsibilities:**
- Set up correct frame layout for each call type
- Save/restore callee-saved registers per platform ABI
- Handle GlueThread pointer passing between execution modes
- Bridge calling conventions between interpreter and compiled code

## Key Files

| File | Role |
|------|------|
| `aarch64/common_call.cpp/.h` | ARM64 shared frame/call helpers |
| `aarch64/optimized_call.cpp` | ARM64 calls into optimized code |
| `aarch64/asm_interpreter_call.cpp` | ARM64 interpreter dispatch |
| `aarch64/baseline_call.cpp` | ARM64 baseline entry/exit |
| `x64/common_call.cpp/.h` | x64 shared frame/call helpers |
| `x64/optimized_call.cpp` | x64 calls into optimized code |
| `x64/asm_interpreter_call.cpp` | x64 interpreter dispatch |
| `x64/baseline_call.cpp` | x64 baseline entry/exit |

## How to Modify

**Add a new call bridge:**
1. Add the implementation in both `aarch64/` and `x64/`
2. Define the call signature in `compiler/call_signature.cpp`
3. Wire it into the stub compiler via `StubBuilder`
4. Add test coverage through compiler host tests

**Modify an existing calling convention:**
1. Update both architecture implementations
2. Synchronize with `assembler/` register conventions
3. Synchronize with `call_signature*` definitions
4. Verify via runtime tests (deopt, GC, and exception paths all depend on frame layout)

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
- Assembler tests: `ecmascript/compiler/assembler/tests/`
- Runtime integration tests that exercise call paths

## Boundaries

- Keep architecture-specific logic in the matching subdirectory — never mix ARM64 and x64 code.
- Calling-convention changes must be synchronized with `assembler/`, `call_signature*`, and `ecmascript/trampoline/` (the assembly counterpart).
- Frame layout must stay compatible with `deoptimizer/` and `stackmap/`.
