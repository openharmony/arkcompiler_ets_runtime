# ecmascript/pgo_profiler

Collects runtime type and execution profiling data during interpretation, serializes it to `.ap` (Ark Profile) files, and provides decoding for AOT compilation to use as optimization guidance.
Primary Language: C++

## Design

### Profile Collection Pipeline

```
  Interpreter bytecode execution
       │  IC feedback, type transitions, call counts
       ▼
  PGOProfiler (per-VM instance)
    ├── WorkList queue (async batch processing)
    ├── RecordProfileType() → bytecode-level type info
    ├── ProfileDefineClass() → class shape tracking
    └── PGOPreDump() / PGODump() → flush to manager
       │
       ▼
  PGOProfilerManager (global singleton)
    ├── Collects from all VM profilers
    ├── PGODumpTask → background thread serialization
    └── Merge() → combines multiple profile sources
       │
       ▼
  PGOProfilerEncoder
    ├── Save() → writes PGO data to .ap file
    └── SaveAndRename() → atomic write (temp → validate → rename)
```

### AP File Format

The `.ap` file uses an elastic binary format with versioned sections:

```
  PGOProfilerHeader (magic: "PANDA\0\0\0")
  ├── Section 1: PGOPandaFileInfos (ABC file metadata)
  ├── Section 2: PGORecordDetailInfos (per-method profiling)
  ├── Section 3: PGOHClassLayoutDescs (type shapes)
  ├── Section 4: PGORecordPool (ABC file registry)
  ├── Section 5: PGOProtoTransitionPool (prototype transitions)
  ├── Section 6: ProfileTypes (type ID pool)
  └── Section 7: ABCFilePool (ABC file descriptors)
```

### Profile Consumption

```
  .ap file
       │
       ▼
  PGOProfilerDecoder
    ├── LoadAndVerify() → memory-map and validate checksums
    ├── GetTypeInfo() → per-method type extraction
    └── IterateHClassTreeDesc() → walk type hierarchy
       │
       ▼
  compiler/pgo_type/ (PGOTypeParser, PGOTypeManager)
       │
       ▼
  Compiler optimization passes
```

## Key Files

| File | Role |
|------|------|
| `pgo_profiler.cpp/.h` | Per-VM profile collection with WorkList |
| `pgo_profiler_manager.cpp/.h` | Global singleton; task dispatch and merging |
| `pgo_profiler_encoder.cpp/.h` | Serialization to .ap files |
| `pgo_profiler_decoder.cpp/.h` | .ap file loading and validation |
| `pgo_profiler_layout.cpp/.h` | Profile data layout orchestration |
| `pgo_profiler_info.cpp/.h` | Profile info data structures |
| `ap_file/pgo_file_info.cpp/.h` | AP file format: header, sections, versioning |
| `types/pgo_profile_type.cpp/.h` | Profile type representation |
| `prof_dump/main.cpp` | `profdump` CLI tool for profile inspection |

## How to Modify

**Add a new profiling data category:**
1. Add collection hooks in `PGOProfiler` (e.g., `RecordNewCategory()`)
2. Add storage in `PGORecordDetailInfos` or a new section in `ap_file/pgo_file_info.h`
3. Update `PGOProfilerEncoder` to serialize the new data
4. Update `PGOProfilerDecoder` to deserialize it
5. Bump the AP file version in `pgo_file_info.h`
6. Add consumer support in `compiler/pgo_type/`
7. Update `prof_dump/main.cpp` to display the new data

**Merge multiple .ap files:**
Use `PGOProfilerManager::MergeApFiles()` or the `profdump` tool.

## Building

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.debug
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ark_js_host_linux_tools_packages
./build.sh --product-name <product-name> --build-target ecmascript/pgo_profiler/prof_dump:profdump   # profdump tool only
```

## Tests

Standalone repo (from repo root `../../`):
```bash
python ark.py x64.debug unittest   # all unit tests (includes PGO profiler tests)
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/pgo_profiler/tests:host_unittest
```

- C++ tests: `tests/*_test.cpp`
- JS fixtures: `tests/pgo_test_case/*.js` (compiled to ABC for profile tests)

Add new tests in `tests/BUILD.gn` under `group("host_unittest")`. Wire JS fixtures with `es2abc_gen_abc` targets.

## Boundaries

- Profile format changes must be synchronized across encoder, decoder, and layout files — plus `prof_dump`.
- Keep profiling collection in `pgo_profiler*.cpp` and optimization consumption in `compiler/pgo_type/` — separate concerns.
- Thread safety: `PGOProfiler` uses WorkList with internal mutex; `PGOProfilerManager` uses background dump tasks to avoid GC deadlock.
- AP file versioning uses elastic header with forward-compatible section gates.
