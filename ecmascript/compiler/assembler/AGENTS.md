# ecmascript/compiler/assembler

Provides architecture-specific assembler and macro-assembler primitives that emit machine instructions programmatically, used by the baseline compiler, trampoline generators, and stub builders.
Primary Language: C++

## Design

The assembler is structured in two layers per architecture:

```
  Assembler (abstract base)
       │
       ├── AssemblerAarch64     ──▶ raw ARM64 instruction encoding
       │       └── ExtendAssembler  ──▶ extended ARM64 helpers
       │
       └── AssemblerX64         ──▶ raw x86-64 instruction encoding
               └── ExtendedAssemblerX64 ──▶ extended x64 helpers

  MacroAssembler (abstract base)
       │
       ├── MacroAssemblerAarch64  ──▶ high-level ARM64 patterns (push, call, etc.)
       └── MacroAssemblerX64      ──▶ high-level x64 patterns
```

**Assembler** handles raw instruction encoding (opcode + operands → bytes).
**MacroAssembler** provides higher-level patterns (function prologues, stack operations, conditional branches) built on top of the assembler.

The baseline compiler and trampoline generators use MacroAssembler directly. The stub compiler uses it through `BaselineStubBuilder`.

## Key Files

| File | Role |
|------|------|
| `assembler.h` | Abstract assembler base class |
| `macro_assembler.h` | Abstract macro-assembler base class |
| `aarch64/assembler_aarch64.cpp/.h` | ARM64 instruction encoding |
| `aarch64/macro_assembler_aarch64.cpp/.h` | ARM64 high-level emission |
| `x64/assembler_x64.cpp/.h` | x86-64 instruction encoding |
| `x64/macro_assembler_x64.cpp/.h` | x86-64 high-level emission |
| `aarch64/assembler_aarch64_constants.h` | ARM64 ISA constants |

## How to Modify

**Add a new instruction:**
1. Add encoding logic to the architecture-specific assembler (e.g., `assembler_aarch64.cpp`)
2. If it's a common pattern, add a macro-assembler wrapper
3. Add test cases in `tests/assembler_*_test.cpp`

**Add support for a new architecture:**
1. Create a new subdirectory (e.g., `riscv64/`)
2. Implement `Assembler` and `MacroAssembler` subclasses
3. Wire into `BUILD.gn` with appropriate CPU gating
4. Update trampoline generators to use the new assembler

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
python ark.py x64.debug unittest   # all unit tests (includes assembler tests)
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/compiler/tests:host_unittest
```

Test files: `tests/assembler_aarch64_test.cpp`, `tests/assembler_x64_test.cpp`, `tests/extended_assembler_x64_test.cpp`, `tests/macro_assembler_x64_test.cpp`.

Tests are registered as `AssemblerTest` in `ecmascript/compiler/tests/BUILD.gn`.

## Boundaries

- Keep instruction encoding changes scoped to the correct architecture directory (`aarch64/` vs `x64/`).
- ABI/register conventions are shared with `trampoline/` and `stubs/` — changes must be synchronized.
- This module only emits instructions; it does not decide *what* to emit — that's the job of baseline compiler, trampoline generators, and stub builders.
