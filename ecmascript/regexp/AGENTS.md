# ecmascript/regexp

Regular expression engine implementing ECMAScript RegExp semantics with parsing, compilation, and execution.
Primary Language: C++

## Design

```
  /pattern/flags or new RegExp(pattern, flags)
       │
       ▼
  RegExpParser
       ├── Parse pattern string
       ├── Validate syntax
       ├── Generate bytecode opcodes
       └── Return RegExp object
       │
       ▼
  RegExpExecutor
       ├── Execute against input string
       ├── Match state machine
       ├── Backtracking on failure
       └── Return match result
```

**Key abstractions:**
- **RegExpParser** — Pattern string to opcode compilation
- **RegExpExecutor** — Pattern matching engine
- **RegExpOpcode** — Bytecode instruction set for regex
- **RegExpParserCache** — Caches compiled regex patterns

## Reference Source
- `arkcompiler/ets_runtime/ecmascript/regexp/regexp_parser.h` (1-200行) - Parser interface
- `arkcompiler/ets_runtime/ecmascript/regexp/regexp_executor.h` - Executor interface

## Key Files

| File | Role |
|------|------|
| `regexp_parser.h/.cpp` | Pattern parsing and compilation |
| `regexp_executor.h/.cpp` | Pattern matching execution |
| `regexp_opcode.h/.cpp` | Regex bytecode opcodes |
| `regexp_parser_cache.h/.cpp` | Pattern compilation cache |

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
python ark.py x64.debug unittest   # includes regexp tests
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/regexp/tests:host_unittest
```

## Boundaries

- Follow ECMAScript RegExp specification exactly.
- Parser is stateful; ensure thread safety if needed.
- Cache is per-VM; do not share across VMs.
