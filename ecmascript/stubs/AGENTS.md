# ecmascript/stubs

Defines C++ runtime stub entry points — fast-path helpers callable from compiled code and trampolines that implement VM operations too complex to inline (property access, type checks, GC barriers, etc.).
Primary Language: C++

## Design

Runtime stubs bridge compiled machine code back into the C++ runtime:

```
  Compiled code (AOT/JIT/Baseline)
       │  needs runtime help (e.g., property lookup)
       ▼
  Call via stub ID from runtime_stub_list.h
       │
       ▼
  Trampoline dispatches to RuntimeStubs::StubName()
       │
       ▼
  C++ implementation in runtime_stubs.cpp
       │
       ▼
  Return result to compiled code
```

**Key design decisions:**
- **Stub list as single source of truth** — `runtime_stub_list.h` defines all stubs via macros. Adding/removing stubs requires updating only this file plus the implementation.
- **Inline C++ helpers** — `runtime_stubs-inl.h` and `runtime_optimized_stubs-inl.h` provide inline RuntimeStubs implementations used by `runtime_stubs.cpp`; compiled machine code calls stubs through stub IDs and runtime call paths.
- **Test stubs** — `test_runtime_stubs.cpp/.h` provides test-only stubs for verifying calling conventions.

## Key Files

| File | Role |
|------|------|
| `runtime_stub_list.h` | Macro-defined list of all runtime stubs (IDs and declarations) |
| `runtime_stubs.cpp/.h` | Runtime stub implementations |
| `runtime_stubs-inl.h` | Inline fast-path helpers |
| `runtime_optimized_stubs-inl.h` | Optimized inline stubs |
| `test_runtime_stubs.cpp/.h` | Test-only stub support |

## How to Modify

**Add a new runtime stub:**
1. Add the stub entry to `runtime_stub_list.h` (follow existing macro pattern)
2. Implement the stub in `runtime_stubs.cpp`
3. If it needs a fast inline path, add it to `runtime_stubs-inl.h`
4. The compiler can now emit calls to this stub by ID
5. Add test coverage through runtime tests

**Modify an existing stub's signature:**
1. Update the declaration in `runtime_stub_list.h`
2. Update the implementation in `runtime_stubs.cpp`
3. Update all callers in compiler (search for the stub name in `compiler/` passes)
4. Verify calling conventions match trampoline expectations

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

Test helpers are in `test_runtime_stubs.cpp/.h`. Full validation through:
- Runtime unit tests: `ark_js_unittest`
- Compiler tests that exercise stub call paths

## Boundaries

- Stub signatures and IDs must stay synchronized with `runtime_stub_list.h` — this is the single source of truth.
- Keep stubs as low-level runtime primitives; do not add high-level business logic.
- Calling conventions must match what trampolines and compiled code expect.
- Stub types: `RUNTIME_STUB_WITH_GC_LIST` stubs use the `CallRuntime` path and may trigger GC/exception/deopt; `RUNTIME_STUB_WITHOUT_GC_LIST` (NoGC stubs) use the NoGC runtime path and must not trigger GC, throw, or deopt.
- Use NoGC stubs only when it is guaranteed there is no allocation or GC risk (for example pure math, debug print, and barrier-style helpers).
- Sources are part of `ecma_source` in root `BUILD.gn`.
