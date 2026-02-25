# ecmascript/compiler/aot_snapshot

Serializes compile-time type information (HClasses, constant pools, symbols) into snapshot data embedded in `.ai` files, so the runtime can reconstruct type state without re-analysis.
Primary Language: C++

## Design

During AOT compilation, the compiler resolves type shapes (HClasses) and constant pool references. This module serializes that information so the runtime can pre-populate its type system on load:

```
  PGOTypeManager (compile-time type state)
       │
       ▼
  AOTSnapshot
    ├── StoreHClassInfo()      → SnapshotGlobalData
    ├── StoreSymbolInfo()      → SnapshotGlobalData
    ├── StoreConstantPoolInfo()→ SnapshotConstantPoolData (per file)
    └── ResolveSnapshotData()  → links methods to constant pools
       │
       ▼
  Serialized into .ai file alongside .an
       │
       ▼
  Runtime: AotFileManager.LoadAiFile() → deserializes snapshot data
```

**Critical constraint:** Snapshot serialization runs with GC disabled. Do not call any interface that may trigger GC during snapshot generation.

**Key abstractions:**
- **AOTSnapshot** — Orchestrates snapshot assembly; delegates to `SnapshotGlobalData` and `SnapshotConstantPoolData`.
- **SnapshotGlobalData** — Stores HClasses, symbols, arrays, and proto-transition tables.
- **SnapshotConstantPoolData** — Per-file constant pool snapshots.

## Key Files

| File | Role |
|------|------|
| `aot_snapshot.cpp/.h` | Snapshot assembly and orchestration |
| `snapshot_constantpool_data.cpp/.h` | Per-file constant pool serialization |
| `snapshot_global_data.cpp/.h` | Global type data (HClasses, symbols) |
| `aot_snapshot_constants.h` | Shared constant definitions |

## How to Modify

**Add a new snapshot data category:**
1. Add storage/retrieval methods to `AOTSnapshot` and `SnapshotGlobalData`
2. Add a `Gen*Info()` private method in `PGOTypeManager` that populates the new data
3. Update `ResolveSnapshotData()` to include the new category
4. Ensure backward compatibility — old runtimes must handle missing new data gracefully

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
- Runtime integration tests: `ark_js_unittest`

Add snapshot-focused tests to `ecmascript/compiler/tests/BUILD.gn` when changing serialization schema.

## Boundaries

- Snapshot schema changes must be backward-compatible with existing AOT consumers.
- Sources are compiled as part of `ecma_source` in root `BUILD.gn` (not in `libark_jsoptimizer_sources`).
- Keep this module focused on serialization — optimization logic belongs in `compiler/` passes.
- Consumers: `aot_file/aot_file_manager.h`, `compiler/bytecode_info_collector.h`, `compiler/pgo_type/pgo_type_manager.h`.
