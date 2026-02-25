# ecmascript/trampoline

Handwritten assembly trampoline stubs that provide the lowest-level entry points for VM dispatch and deoptimization, where C++ or compiler-generated code cannot be used.
Primary Language: Assembly (.S files) and C/C++ headers

## Design

These are raw assembly stubs for operations that must be written in assembly (e.g., manipulating the stack pointer, saving all registers, or performing atomic transitions between execution modes):

```
  Runtime / Compiled code
       │  needs raw assembly transition
       ▼
  raw_asm_stub.S (per architecture)
    ├── LazyDeoptEntry  → saves state, calls DeoptHandler
    └── (other raw entry points as needed)
       │
       ▼
  asm_defines.h (shared macro/ABI constants)
    ├── Register offsets
    ├── Frame layout constants
    └── Platform-specific macros
```

**Contrast with `ecmascript/compiler/trampoline/`:**
- **This module** (`ecmascript/trampoline/`) — Handwritten `.S` assembly; used for operations that cannot be expressed in C++.
- **`compiler/trampoline/`** — C++ code that *generates* assembly via the `assembler/` module at compile time.

**Key design decisions:**
- One `.S` file per architecture, guarded by CPU detection in `BUILD.gn`.
- `asm_defines.h` ensures assembly and C++ agree on offsets and constants.
- The `LazyDeoptEntry` stub is a key entry point: it saves registers, retrieves the glue pointer from thread-local storage, and tail-calls into the deoptimization handler.

## Key Files

| File | Role |
|------|------|
| `asm_defines.h` | Shared macros and ABI constants for assembly and C++ |
| `aarch64/raw_asm_stub.S` | ARM64 assembly trampoline stubs |
| `x64/raw_asm_stub.S` | x86-64 assembly trampoline stubs |
| `arm32/raw_asm_stub.S` | ARM32 assembly trampoline stubs |

## How to Modify

**Add a new assembly entry point:**
1. Add the implementation to the architecture-specific `.S` file(s)
2. Declare it as `.global` and add an `.extern` if it calls C++ functions
3. Add the offset constants to `asm_defines.h` if needed
4. Wire it into the runtime by referencing the symbol from C++ code
5. Ensure it's selectable by `BUILD.gn` CPU gating logic

**Update frame layout constants:**
1. Update `asm_defines.h` with new offsets
2. Update all `.S` files that reference those offsets
3. Synchronize with `deoptimizer/` AsmStackContext and `compiler/trampoline/` frame setup

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

No dedicated test directory. Validation through:
- Runtime tests that exercise deoptimization and call transitions
- Compiler/JIT tests that trigger compiled code entry/exit paths

## Boundaries

- Keep architecture-specific code in the matching subdirectory (`aarch64/`, `x64/`, `arm32/`).
- ABI/register/frame conventions must stay consistent with `stubs/`, `deoptimizer/`, and `compiler/trampoline/`.
- Keep assembly stubs minimal — they should only handle what C++ cannot (register saves, stack pointer manipulation). Logic belongs in C++ stubs.
- Implementation principle: only use handwritten assembly for parts the `assembler/` module cannot express; keep behavior logic in C++ stubs or compiler-generated code when possible.
- Sources are part of `ecma_source` in root `BUILD.gn`, gated by CPU architecture.
