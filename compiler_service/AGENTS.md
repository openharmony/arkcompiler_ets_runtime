# compiler_service

OHOS SystemAbility (SA ID 5300) that exposes AOT compilation as an IPC service, allowing other system components to trigger AOT compilation of JS bundles on-device.
Primary Language: C++

## Design

### 1.1 Service Architecture

The service follows OHOS's SystemAbility pattern:

```
  Client App / System
       │ IPC (Binder)
       ▼
  AotCompilerInterfaceProxy  ──IPC──▶  AotCompilerInterfaceStub
                                              │
                                              ▼
                                      AotCompilerService (SA lifecycle)
                                              │
                                              ▼
                                      AotCompilerImpl (singleton)
                                         │  fork()
                                         ▼
                                      Child process runs ark_aot_compiler / ark_aot
                                         │
                                         ▼
                                      .an file + .ai file + code signature
```

### 1.2 Static Interpreter Compilation Path

`compiler_service` contains a separate static-interpreter path implemented by argument parsers (not by SA listener control flow):

```
  IPC args (`moduleArkTSMode`, `isSysComp`, `target-compiler-mode`, ...)
       │
       ▼
  AOTArgsParserFactory
    ├── Dynamic mode (`moduleArkTSMode=dynamic`)
    │     └── AOTArgsParser → `/system/bin/ark_aot_compiler`
    ├── Static/Hybrid mode (`moduleArkTSMode=static|hybrid`)
    │     └── StaticAOTArgsParser → `/system/bin/ark_aot`
    └── System component (`isSysComp=1`)
          └── StaticFrameworkAOTArgsParser → `/system/bin/ark_aot`
```

Static parser behavior used by 1.2:
- Rewrites `aot-file` into `--paoc-output=<aot-file>.an`.
- Builds static interpreter args (`--boot-panda-files`, `--paoc-panda-files`, `--paoc-location`).
- For partial mode (`target-compiler-mode=partial`), appends `--paoc-use-profile:path=<pgoDir>/profile.ap`.

**Key design decisions:**
- Compilation runs in a **forked child process** to isolate crashes and resource limits.
- Service runtime governance (SA lifecycle and power/screen/thermal listeners) is separate from static interpreter argument construction.
- Static interpreter compilation uses dedicated parsers and `/system/bin/ark_aot` for static/hybrid/system-component flows.
- AOT arguments are validated through `AOTArgsHandler` + `AOTArgsVerify` before being passed to the child process.
- The IPC contract is defined in `iaot_compiler_interface.h`; proxy and stub are in `interface/`.

## Key Files

| File | Role |
|------|------|
| `src/aot_compiler_impl.cpp` | Core logic: fork child, run compiler, collect result |
| `src/aot_compiler_service.cpp` | SA lifecycle (OnStart/OnStop), event listener registration |
| `src/aot_compiler_client.cpp` | Client-side IPC wrapper |
| `interface/iaot_compiler_interface.h` | IPC interface definition |
| `interface/aot_compiler_interface_stub.cpp` | Server-side IPC message dispatch |
| `interface/aot_compiler_interface_proxy.cpp` | Client-side IPC message construction |
| `src/aot_args_handler.cpp` | Argument validation and sanitization |
| `sa_profile/5300.json` | SA registration profile |

## How to Modify

**Add a new IPC method:**
1. Add the method declaration to `interface/iaot_compiler_interface.h`
2. Add the proxy implementation in `interface/aot_compiler_interface_proxy.cpp`
3. Add the stub dispatch case in `interface/aot_compiler_interface_stub.cpp`
4. Implement the logic in `src/aot_compiler_impl.cpp`
5. Add unit test in `test/unittest/`

**Add a new system event listener:**
1. Create listener class in `include/` and `src/` (follow `power_disconnected_listener` pattern)
2. Register/unregister in `AotCompilerService::OnStart()`/`OnStop()`
3. Wire the callback to `AotCompilerImpl`

## Building

Full OHOS tree only (depends on `samgr`, `ipc`, and other system libraries; no standalone build):
```bash
./build.sh --product-name <product-name> --build-target compiler_service:compiler_service
```

## Tests

Full OHOS tree only:
```bash
./build.sh --product-name <product-name> --build-target compiler_service/test:compiler_service_unittest
./build.sh --product-name <product-name> --build-target compiler_service/test:compiler_service_fuzztest
```

- Unit tests: `test/unittest/`
- Fuzz tests: `test/fuzztest/`
- Shared test config: `test/compiler_service_test.gni`

Add new tests in `test/BUILD.gn` and include them in `compiler_service_unittest` or `compiler_service_fuzztest` groups.

## Boundaries

- IPC contract changes must be synchronized across `interface/`, `include/`, and `src/`.
- Keep `sa_profile/5300.json` consistent with the service process/lib name and SA ID.
- This module only orchestrates compilation — actual compiler logic lives in `ecmascript/compiler/`.
- Service lifecycle/listener logic stays in `src/`; do not mix test-only code into production sources.
