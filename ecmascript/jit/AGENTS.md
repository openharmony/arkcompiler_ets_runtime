# ecmascript/jit

Manages JIT (Just-In-Time) compilation: decides when to compile hot methods, schedules compilation tasks on background threads, and installs compiled code into the runtime.
Primary Language: C++

## Design

The JIT module is the runtime-side coordinator that connects hotness detection to the compiler:

```
  Interpreter executes bytecode
       │  increments hotness counter
       ▼
  CompileDecision
    ├── Checks hotness threshold
    ├── Checks function kind support
    ├── Checks VM state (GC, etc.)
    └── Decides: compile baseline or optimized?
       │
       ▼
  Jit::Compile()
       │  creates JitTask
       ▼
  JitTask (posted to JitThread)
    ├── Sets up JitCompilationEnv
    ├── Calls into compiler pipeline (PassManager)
    └── Installs compiled code on completion
       │
       ▼
  JitResources
    ├── Manages compilation memory
    └── Tracks concurrent compilation state
```

**Key design decisions:**
- **Async compilation** — JIT compilation runs on `JitThread` to avoid blocking the main JS thread.
- **Tiered compilation** — `CompileDecision` can trigger either baseline (fast, unoptimized) or optimized (slow, PGO-guided) compilation based on method characteristics.
- **Profiling integration** — `JitProfiler` collects per-bytecode type feedback during interpretation, feeding the optimizing compiler.
- **Diagnostics** — `JitDfx` provides logging and timing information for debugging JIT behavior.

## Key Files

| File | Role |
|------|------|
| `jit.cpp/.h` | JIT lifecycle management and `Compile()` entry point |
| `compile_decision.cpp/.h` | Hotness-based compilation trigger heuristics |
| `jit_task.cpp/.h` | Compilation task lifecycle (create → compile → install) |
| `jit_thread.cpp/.h` | Background compilation thread management |
| `jit_resources.cpp/.h` | Compilation memory and resource tracking |
| `jit_profiler.cpp/.h` | Bytecode-level type feedback collection |
| `jit_dfx.cpp/.h` | JIT diagnostics and timing |

## How to Modify

**Tune JIT trigger heuristics:**
1. Modify `CompileDecision` in `compile_decision.cpp`
2. Key factors: hotness threshold (`--compiler-jit-hotness-threshold`), function kind filtering, VM state checks
3. Test with varying workloads to avoid over/under-compilation

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
python ark.py x64.debug unittest   # all unit tests (includes JIT tests)
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/jit/tests:host_unittest
```

- `tests/jit_test.cpp` — JIT compilation flow tests
- `tests/jit_time_scope_test.cpp` — Timing/profiler tests

Add new tests in `tests/BUILD.gn` under `group("host_unittest")`.

## Boundaries

- Compile-trigger policy changes must be synchronized across `compile_decision.cpp`, `jit_task.cpp`, and `jit_thread.cpp`.
- Backend compilation logic belongs in `ecmascript/compiler/` — this module only schedules and coordinates.
- Keep timing/profiler assertions in tests deterministic (avoid flaky timing-only checks).
- Deoptimization handling is in `ecmascript/deoptimizer/`, not here.
