# ecmascript/deoptimizer

Handles the transition from optimized (AOT/JIT) code back to the interpreter when a speculative optimization assumption fails, reconstructing the interpreter frame from compiled code state.
Primary Language: C++

## Design

When optimized code encounters a condition it didn't compile for (type mismatch, missing property, etc.), it triggers deoptimization:

```
  Optimized code hits deopt point
       │
       ▼
  DeoptHandlerAsm (trampoline stub)
       │  saves registers, sets up AsmStackContext
       ▼
  Deoptimizier::CollectDeoptBundleVec()
       │  reads stackmap to find deopt metadata
       ▼
  Deoptimizier::CollectVregs()
       │  reconstructs virtual register values from:
       │    - stack slots (via stackmap locations)
       │    - callee-saved registers (via CalleeReg mapping)
       │    - constants embedded in deopt metadata
       ▼
  Deoptimizier::ConstructAsmInterpretFrame()
       │  builds an interpreter frame on the stack
       │  with correct PC, ACC, ENV, vregs
       ▼
  Resume execution in interpreter
```

**Key abstractions:**
- **Deoptimizier** — Main class; collects deopt bundle, rebuilds interpreter state.
- **AsmStackContext** — Stack-allocated struct holding caller FP, return address, inline depth, and deopt metadata during the transition.
- **CalleeReg** — Maps callee-saved register indices to stack offsets for value recovery.
- **SpecVregIndex** — Special virtual register indices (ACC, ENV, FUNC, THIS, etc.) used in deopt bundles.

**Inline deoptimization:** When inlined functions need to deopt, `inlineDepth_` in `AsmStackContext` indicates the nesting level, and vregs are decoded with a shift to separate inline frames.

**Lazy deoptimization:** Triggered by dependency invalidation (for example prototype/dependency checks), not by the deopt-threshold counter itself. The runtime clears compiled-code status and redirects affected return paths through `LazyDeoptEntry`, then resumes via interpreter frames.

**Lazy deopt flow:**
1. Dependency invalidation marks compiled methods for lazy deopt (`DeoptType::LAZYDEOPT`) and clears compiled-code status.
2. `PrepareForLazyDeopt()` scans current frames and patches return addresses to `LazyDeoptEntry`.
3. On function return, `LazyDeoptEntry` transfers control to `DeoptHandlerAsm`, then to runtime `DeoptHandler`.
4. `DeoptHandler` rebuilds interpreter frames from stackmap/deopt bundles; `ProcessLazyDeopt()` handles ACC overwrite and PC advance.
5. Execution resumes through interpreter (or baseline bridge path when applicable).

## Key Files

| File | Role |
|------|------|
| `deoptimizer.cpp/.h` | Deopt flow, vreg collection, frame reconstruction |

## How to Modify

**Add a new deopt reason:**
1. Add the reason to `DeoptType` enum (in `compiler/` headers)
2. Add display string in `Deoptimizier::DisplayItems()`
3. Ensure the compiler emits the correct deopt metadata at the new deopt point
4. Add test case that triggers the new deopt reason

**Change frame layout:**
1. Update `AsmStackContext` field offsets
2. Update `ConstructAsmInterpretFrame()` frame construction
3. Synchronize with trampoline `DeoptHandlerAsm` stub
4. Synchronize with `stackmap/` metadata format
5. Test thoroughly — incorrect frame layout causes silent corruption

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
- JIT tests: `ecmascript/jit/tests/`
- Compiler host tests: `ecmascript/compiler/tests/`
- Deopt-specific JS tests: `test/deopttest/`

## Boundaries

- Frame layout must stay consistent with JIT/AOT trampoline stack conventions and `stackmap/` metadata.
- Deopt logic must be deterministic for identical deopt metadata input.
- This module is recovery-path only — do not add optimization-pass logic here.
- Keep lazy-deopt behavior strictly tied to dependency invalidation and return-address patching; deopt-threshold is an independent regular-deopt budget mechanism.
- Sources are part of `ecma_source` in root `BUILD.gn`.
