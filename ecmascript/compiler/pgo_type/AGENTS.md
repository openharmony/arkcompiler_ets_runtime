# ecmascript/compiler/pgo_type

Bridges PGO profiling data into the compiler's type system: parses profile types from `.ap` files, manages the mapping between profile types and runtime HClasses, and feeds this information into optimization passes.
Primary Language: C++

## Design

This module reconstructs compile-time type state from profile data and publishes it to optimization passes and AOT snapshot generation.

```
  Runtime side
  PGOProfilerEncoder  ──>  `.ap`

  Compiler side (`PGOTypeParser::CreatePGOType`)
  `.ap`
    │
    ▼
  PGOProfilerDecoder
    │
    ▼
  Stage 1: Global type metadata
    ├── IterateProtoTransitionPool() -> PGOTypeManager::RecordProtoTransType()
    └── IterateHClassTreeDesc() -> PGOHClassGenerator -> PGOTypeManager::RecordHClass()
    │
    ▼
  Stage 2: Per-method bytecode typing
    ├── For each method: PGOTypeRecorder(decoder, jsPandaFile, methodOffset)
    ├── BaseParser-derived handlers parse define-op records by PGOBCInfo type
    └── Record location/type info:
        - RecordLocationToRootType()
        - RecordLocationToElementsKind()
        - RecordConstantIndex()
    │
    ▼
  Stage 3: Consumption
    ├── Compiler passes query type/HClass data through PGOTypeManager
    └── InitAOTSnapshot() serializes HClass/symbol/array/constant-index/proto-transition info to `.ai`
```

Legacy parser classes (`PGOTypeParser` / `BaseParser` and sub-parsers) remain in code for compatibility, but new behavior should be integrated through `CreatePGOType()`.

**Key abstractions:**
- **PGOTypeParser** — (Deprecated) Visitor-pattern parser with specialized sub-parsers per type category.
- **BaseParser** — (Deprecated) Legacy parser base class kept only for compatibility with existing code.
- **PGOTypeManager** — Central type registry mapping `ProfileType` → `JSHClass`, with location-based lookups for compiler passes.
- **PGOTypeRecorder** — Per-method type annotations derived from profiling data.
- **PGOHClassGenerator** — Creates runtime HClass objects from profile type descriptions.

## Key Files

| File | Role |
|------|------|
| `pgo_type_parser.cpp/.h` | Profile type parsing with visitor pattern |
| `pgo_type_manager.cpp/.h` | Central type registry and HClass mapping |
| `pgo_type_recorder.cpp/.h` | Per-method type annotation |
| `pgo_hclass_generator.cpp/.h` | HClass generation from profile types |
| `pgo_type_location.h` | Bytecode-location-based type lookup keys |

## How to Modify

**Add a new PGO type category:**
1. Use `CreatePGOType()` as the integration entry (the `BaseParser` path is deprecated).
2. Extend the parsing/recording flow used by `CreatePGOType()` for the new category.
3. Add storage/query methods to `PGOTypeManager`.

**Change how profile types map to HClasses:**
1. Modify `PGOHClassGenerator` for generation logic
2. Update `PGOTypeManager::RecordHClass()` / `QueryHClass()` for storage
3. Ensure `AOTSnapshot` serialization still works (in `aot_snapshot/`)

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
- Compiler tests: `ecmascript/compiler/tests/`
- PGO profiler tests: `ecmascript/pgo_profiler/tests/`

Keep PGO type schema changes synchronized with `ecmascript/pgo_profiler/tests/` fixtures.

## Boundaries

- Sources are compiled as part of `ecma_source` in root `BUILD.gn`, not `libark_jsoptimizer_sources`.
- Profile data format compatibility must be preserved with `pgo_profiler/`.
- Keep PGO data parsing independent from backend codegen details.
- Consumers: `compiler/type_inference/pgo_type_infer.cpp` and other typed lowering passes.
