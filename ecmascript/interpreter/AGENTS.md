# ecmascript/interpreter

Bytecode interpreter that executes ArkCompiler ABC instructions through a register-based virtual machine.
Primary Language: C++

## Design

The interpreter provides execution of ark bytecode with C++ implementation:

```
  ABC bytecode (.abc file)
       │
       ▼
  EcmaInterpreter::Execute()
       │
       └──▶ C++ interpreter (interpreter-inl.cpp)
             ├── switch/case dispatch
             ├── Bytecode handler functions
             ├── Type checks and guards
             └── Runtime calls (fast_runtime_stub/slow_runtime_stub)
```

**Key abstractions:**
- **EcmaInterpreter** — Main interpreter entry point and execution control
- **FrameHandler** — Stack frame management, argument passing, and return value handling
- **EcmaRuntimeCallInfo** — Execution context containing VM, arguments, and return slot
- **FastRuntimeStub** — Fast-path runtime call declarations
- **SlowRuntimeStub** — Slow-path runtime call implementations

**Assembly interpreter integration:**
- `interpreter_assembly.cpp` provides assembly interpreter call entry points and debugging utilities
- Actual assembly interpreter implementation is located in `ecmascript/compiler/interpreter_stub.cpp`
- `templates/` contains assembly template files for bytecode handler generation

## Reference Source
- `arkcompiler/ets_runtime/ecmascript/interpreter/interpreter.h` (1-90行) - Interpreter interface
- `arkcompiler/ets_runtime/ecmascript/interpreter/interpreter-inl.h` - C++ interpreter implementation

## Key Files

| File | Role |
|------|------|
| `interpreter.h/.cpp` | Main interpreter entry point |
| `interpreter-inl.h/.cpp` | C++ interpreter implementation with switch/case dispatch |
| `interpreter_assembly.h/.cpp` | Assembly interpreter call entry and debugging utilities |
| `frame_handler.h/.cpp` | Stack frame and call management |
| `fast_runtime_stub.h` | Fast runtime call declarations |
| `slow_runtime_stub.h/.cpp` | Slow runtime call implementations |
| `templates/` | Assembly template files for bytecode handler generation |

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
python ark.py x64.debug unittest   # includes interpreter tests
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/tests:host_unittest
```

## Boundaries

- Assembly interpreter code is in `ecmascript/compiler/interpreter_stub.cpp`; this directory only provides call entry points.
- C++ interpreter and assembly interpreter must maintain consistent semantics.
- Hotness counter updates trigger JIT compilation; ensure correct threshold management.
