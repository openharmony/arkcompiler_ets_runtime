# ecmascript/compiler/aot_file

Manages the `.an` AOT artifact file format and `.ai` snapshot files: builds and parses ELF binaries containing compiled machine code, stackmaps, and metadata for the runtime to load and execute.
Primary Language: C++

## Design

The `.an` file is a standard ELF binary with custom sections:

```
  Compiler output (per-module CodeInfo)
       │
       ▼
  ElfBuilder
    ├── Merges text sections across modules
    ├── Builds symbol table and string table
    ├── Resolves relocations (AArch64, AMD64)
    └── Writes ELF binary → .an file

  Runtime loading:
  .an file
       │
       ▼
  ElfReader
    ├── Verifies ELF header and AOT version
    ├── Parses sections (text, stackmap, strtab, symtab)
    └── Reconstructs ModuleSectionDes per module

       │
       ▼
  AotFileManager
    ├── Registers .an with runtime
    ├── Binds ABC methods to compiled entry points
    └── Provides StackMapParser for deoptimization
```

**Key abstractions:**
- **ModuleSectionDes** — Per-module offset/size descriptor within the ELF.
- **AnFileInfo / AotFileInfo** — Metadata schema for loaded AOT artifacts.
- **AotVersion** — (Deprecated) Version compatibility checking between compiler and runtime.
- **BinaryBufferParser** — Low-level binary payload parser.

## Key Files

| File | Role |
|------|------|
| `elf_builder.cpp/.h` | ELF construction from compiler output |
| `elf_reader.cpp/.h` | ELF parsing for runtime loading |
| `aot_file_manager.cpp/.h` | Central AOT artifact registry in runtime |
| `an_file_info.cpp/.h` | .an file metadata and section management |
| `aot_version.cpp/.h` | Version compatibility checking |
| `binary_buffer_parser.cpp/.h` | Binary payload parsing utilities |
| `stub_file_info.cpp/.h` | Stub .an file representation |
| `module_section_des.cpp/.h` | Per-module section descriptors |

## How to Modify

**Add a new ELF section:**
1. Add the section name to `ElfSecName` enum
2. Add build logic in `ElfBuilder`
3. Add parse logic in `ElfReader`
4. Update `ModuleSectionDes` if the section is per-module
5. Add test coverage in `tests/`

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
python ark.py x64.debug unittest   # all unit tests (includes aot_file tests)
```

Full OHOS tree:
```bash
./build.sh --product-name <product-name> --build-target ecmascript/compiler/aot_file/tests:host_unittest
```

- C++ tests: `tests/aot_file_test.cpp`, `an_file_info_test.cpp`, `binary_buffer_parser_test.cpp`
- JS fixtures: `tests/aot_file_test_case/*.js` (compiled to ABC for test input)

Add new tests via `host_unittest_action` in `tests/BUILD.gn`. Place JS fixtures in `tests/aot_file_test_case/` with `es2abc_gen_abc` targets.

## Boundaries

- `AotVersion` is deprecated; avoid new version bumps and do not introduce ad-hoc version forks.
- This module is for file format and parsing only — do not add runtime policy or compiler pass logic here.
- JS test fixtures go in `tests/aot_file_test_case/` and must be wired through `tests/BUILD.gn`.
