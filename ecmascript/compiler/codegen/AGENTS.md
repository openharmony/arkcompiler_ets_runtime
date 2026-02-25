# ecmascript/compiler/codegen

Translates the optimized Circuit IR into machine code through two pluggable backends: LLVM and Maple/LiteCG. The backend is selected at build time via `COMPILE_MAPLE` flag.
Primary Language: C++

## Design

Both backends implement the same abstract interface (`CodeGeneratorImpl`) and follow the same flow:

```
  Circuit IR (after optimization passes)
       │
       ▼
  CodeGenerator (dispatches to active backend)
       │
       ├──▶ LLVMIRGeneratorImpl path:
       │      LLVMIRGeneratorImpl.GenerateCode()
       │        └── LLVMIRBuilder → builds LLVM IR
       │      LLVMAssembler.Run()
       │        └── LLVM JIT engine → machine code
       │
       └──▶ Maple/LiteCG path:
              LiteCGIRGeneratorImpl.GenerateCode()
                └── LiteCGIRBuilder → builds Maple LMIR
              LiteCGAssembler.Run()
                └── LiteCG backend → machine code
```

**Backend abstraction:**
- `CodeGeneratorImpl` — `GenerateCodeForStub()` and `GenerateCode()` translate Circuit gates to backend IR.
- `Assembler` (base class in `assembler/`) — `Run()` produces machine code from backend-specific IR.
- Target-specific builders (`aarch64_builder`, `x64_builder`) handle architecture differences within each backend.

## Key Files

| File | Role |
|------|------|
| `llvm/llvm_codegen.h` | LLVM backend: `LLVMAssembler` (IR → machine code) |
| `llvm/llvm_ir_builder.cpp/.h` | Circuit IR → LLVM IR translation |
| `llvm/lib/llvm_interface.cpp/.h` | LLVM shared library wrapper (`libark_llvmcodegen`) |
| `llvm/aarch64/aarch64_builder.cpp` | ARM64-specific LLVM IR patterns |
| `llvm/x64/x64_builder.cpp` | x64-specific LLVM IR patterns |
| `maple/litecg_codegen.cpp/.h` | Maple backend: `LiteCGAssembler` |
| `maple/litecg_ir_builder.cpp/.h` | Circuit IR → Maple LMIR translation |
| `maple/BUILD.gn` | Maple backend build configuration |

## How to Modify

**Add support for a new Circuit gate in codegen:**
1. Add the lowering case in `llvm/llvm_ir_builder.cpp` (LLVM path)
2. Add the lowering case in `maple/litecg_ir_builder.cpp` (Maple path)
3. Add target-specific handling in `aarch64_builder` / `x64_builder` if needed
4. Add test coverage in `compiler/tests/` or `codegen/maple/test/`

**Tune backend compilation options:**
- LLVM: Modify `LOptions` in `llvm/llvm_codegen.h` (opt level, FP elimination, reloc mode)
- Maple: Modify `litecgOptions` vector passed to `LiteCGAssembler`

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
python ark.py x64.debug unittest   # all unit tests (includes codegen tests)
```

Full OHOS tree:
```bash
# Maple backend tests
./build.sh --product-name <product-name> --build-target ecmascript/compiler/codegen/maple/test:host_unittest

# Full compiler tests (includes backend integration)
./build.sh --product-name <product-name> --build-target ecmascript/compiler/tests:host_unittest
```

## Boundaries

- Keep backend logic strictly inside `maple/` or `llvm/` — no cross-backend coupling.
- Both backends must handle the same set of Circuit gates; when adding gates, update both.
- Maintain target portability (`aarch64` and `x64`) in both backends.
- Codegen ABI/layout changes must be validated by compiler host unittests.
