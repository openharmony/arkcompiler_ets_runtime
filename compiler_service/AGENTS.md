# compiler_service

OHOS SystemAbility (SA ID 5300) that exposes AOT compilation as an IPC service, allowing other system components to trigger AOT compilation of JS/TS bundles on-device.
Primary Language: C++

## Design

### 1.1 Service Architecture

The service follows OHOS's SystemAbility pattern:

```
  Client App / System (installd, BMS, etc.)
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

### 1.2 IPC Interface

The service exposes 4 IPC methods via `IAotCompilerInterface`:

| Method | Code | Purpose |
|--------|------|---------|
| `AotCompiler(args, sigData)` | `COMMAND_AOT_COMPILER` | Trigger AOT compilation, returns code signature |
| `StopAotCompiler()` | `COMMAND_STOP_AOT_COMPILER` | Kill running compilation child process |
| `GetAOTVersion(sigData)` | `COMMAND_GET_AOT_VERSION` | Query current AOT file version |
| `NeedReCompile(oldVersion, sigData)` | `COMMAND_NEED_RE_COMPILE` | Check if re-compilation is needed for given version |

### 1.3 Compilation Flow (EcmascriptAotCompiler)

The main compilation flow in `AotCompilerImpl::EcmascriptAotCompiler`:

```
  1. PrepareArgsHandler   ── check allowAotCompiler_, create AOTArgsHandler, run parser Check()
  2. ShouldPreCreateOutputFiles ── decide if .an/.ai need pre-creation
  3. PreCreateAotFiles    ── create output dir + .an file (with FD) + .ai file
                            chown/chmod to bundleUid, FD managed by CompilationCleanupGuard
  4. ForkAndExecute       ── fork child process
     ├── Parent: HapVerifyInParent (verify bundle name + appIdentifier via libhapverify.z.so)
     │           then ExecuteInParentProcess (waitpid, collect exit status)
     └── Child:  PrepareChildProcess (clear FD_CLOEXEC on guard fd, pass --an-fd)
                 DropCapabilities (setuid/setgid to bundleUid, drop all caps)
                 ExecuteInChildProcess (execv ark_aot_compiler / ark_aot)
  5. HandlePostCompilation
     ├── SendSysEvent (HiSysEvent reporting)
     ├── AOTLocalCodeSign (SignLocalCodeByFd preferred, fallback to SignLocalCode)
     ├── SetAotSecurityLabels (xattr on .an + .ai)
     └── guard.Dismiss() (close fd, keep .an/.ai files)
```

### 1.4 Argument Parsing (Factory Pattern)

`AOTArgsParserFactory::GetParser()` selects a parser based on request args:

```
  AOTArgsParserFactory::GetParser(args, isEnableStaticCompiler)
    ├── isSysComp=true && static compiler enabled
    │     └── StaticFrameworkAOTArgsParser → /system/bin/ark_aot
    ├── moduleArkTSMode="dynamic"
    │     └── AOTArgsParser → /system/bin/ark_aot_compiler
    ├── moduleArkTSMode="static"|"hybrid" && static compiler enabled
    │     └── StaticAOTArgsParser → /system/bin/ark_aot
    └── otherwise → std::nullopt (compilation rejected)
```

Parser types and their FD argument names:
| Parser | Type Enum | Executable | FD Arg |
|--------|-----------|-----------|--------|
| `AOTArgsParser` | `DYNAMIC_AOT` | `/system/bin/ark_aot_compiler` | `--an-fd` |
| `StaticAOTArgsParser` | `STATIC_AOT` | `/system/bin/ark_aot` | `--paoc-an-fd` |
| `StaticFrameworkAOTArgsParser` | `FRAMEWORK_STATIC_AOT` | `/system/bin/ark_aot` | `--paoc-an-fd` |

Dynamic AOT args: `--aot-file`, `--target-compiler-mode`, `--compiler-pkg-info` (JSON), `--compiler-external-pkg-info` (JSON), `--compiler-device-state`, `--compiler-baseline-pgo`, `--compiler-opt-bc-range`, `--compiler-thermal-level`.

Static AOT args: `--boot-panda-files` (from `/system/framework/bootpath.json`), `--paoc-output`, `--paoc-location`, `--paoc-panda-files`, `--paoc-use-profile:path=<profile>`, `--compiler-regex` (filter), plus defaults (`--load-runtimes=ets`, `--paoc-generate-symbols=true`, etc.).

### 1.5 Security & Validation

`AotArgsVerify` provides comprehensive argument validation:

- **Path traversal**: rejects `..`, `../`, `/../` in all file paths
- **Ark cache directory**: validates paths under `/data/app/el1/public/aot_compiler/ark_cache/<bundleName>/` or `/data/service/el1/public/for-all-app/framework_ark_cache/`
- **Bundle UID/GID**: must be >= 10000 for app bundles and host-private shared HSP AOT
- **ABC offset/size**: validates `abcOffset + abcSize <= HAP file size`
- **Compile mode**: must be "partial" or "full"
- **ArkTS mode**: must be "dynamic", "static", or "hybrid"
- **HAP verification**: dynamically loads `libhapverify.z.so` to verify bundleName and appIdentifier against HAP content (parent process only, for dynamic AOT)
- **Code sign path validation**: `CheckCodeSignArkCacheFilePath` resolves realpath and validates prefix

### 1.6 Runtime Governance (Event Listeners)

Three system event listeners registered in `OnStart()`, unregistered in `OnStop()`:

| Listener | Event | Behavior |
|----------|-------|----------|
| `PowerDisconnectedListener` | `COMMON_EVENT_POWER_DISCONNECTED` | Stop compiler, pause 30s, then allow |
| `ScreenStatusSubscriber` | `COMMON_EVENT_SCREEN_ON` | Stop compiler, pause 40s, then allow |
| `ThermalMgrListener` | `COMMON_EVENT_THERMAL_LEVEL_CHANGED` | Level >= 2 → pause; < 2 → allow |

Pause/allow is controlled by `allowAotCompiler_` atomic bool checked at compilation entry.

### 1.7 SA Lifecycle & Auto-Unload

- SA registered with `run-on-create: false` (lazy load)
- After each IPC call, schedules auto-unload via `DelayUnloadTask` (180s delay)
- Before each IPC call, cancels pending unload via `RemoveUnloadTask`
- Client loads SA on demand via `SystemAbilityManager::LoadSystemAbility` with `AotCompilerLoadCallback`
- Client waits for SA load with 6s timeout (`loadSaCondition_`)

### 1.8 Code Signing Flow

```
  1. SA process creates .an file with open() → FD kept in anFd_
  2. FD passed to child via fork (FD_CLOEXEC cleared) → child writes AOT output
  3. After compilation, SA signs via:
     a. SignLocalCodeByFd(anFd_) — preferred (FD-based)
     b. SignLocalCode(path) — fallback (path-based, for static/framework or FD failure)
  4. Security labels (xattr) set on .an and .ai files
```

**Key design decisions:**
- Compilation runs in a **forked child process** to isolate crashes and resource limits.
- FD-based code signing avoids path-based attacks and works with the file descriptor directly.
- HAP verification (`CheckBundleNameAndAppIdentifier`) runs in the parent process concurrently with child compilation, and kills the child if verification fails.
- Static interpreter compilation uses dedicated parsers and `/system/bin/ark_aot`.
- AOT arguments are validated through `AOTArgsHandler::Handle()` → `parser->Check()` + `parser->Parse()` before fork.
- The IPC contract is defined in `iaot_compiler_interface.h`; proxy and stub are in `interface/`.
- SA auto-unloads after 180s of inactivity to save system resources.

## Key Files

| File | Role |
|------|------|
| `src/aot_compiler_impl.cpp` | Core logic: fork child, run compiler, code sign, collect result |
| `src/aot_compiler_service.cpp` | SA lifecycle (OnStart/OnStop), event listener registration, auto-unload |
| `src/aot_compiler_client.cpp` | Client-side IPC wrapper, SA lazy loading, death recipient |
| `interface/iaot_compiler_interface.h` | IPC interface definition (4 methods) |
| `interface/aot_compiler_interface_stub.cpp` | Server-side IPC message dispatch (4 commands) |
| `interface/aot_compiler_interface_proxy.cpp` | Client-side IPC message construction |
| `include/aot_compiler_args.h` | `AotCompilerArgs` struct (Parcelable) + `HspModuleInfo` struct |
| `src/aot_args_handler.cpp` | Argument parsing via Factory pattern (Dynamic/Static/Framework parsers) |
| `include/aot_args_handler.h` | `AOTArgsHandler`, `AOTArgsParserBase`, parser classes, `AOTArgsParserFactory` |
| `src/aot_args_verify.cpp` | Comprehensive argument validation (path traversal, bundle checks, HAP verify) |
| `include/aot_args_verify.h` | `AotArgsVerify` class, `AotPkgInfo`/`AotPkgVerifyInfo`/`ExternalPkgVerifyInfo` structs |
| `include/aot_compiler_constants.h` | All constants, enums (`AotParserType`, `BundleType`, `AotTriggerType`, `RetStatusOfCompiler`), error code mappings |
| `include/aot_compiler_error_utils.h` | Error code enum (`ERR_OK`..`INVALID_ERR_CODE`) + `AotCompilerErrorUtil` |
| `include/aot_args_list.h` | Whitelisted argument sets (`aotArgsList`, `staticAOTArgsList`, default arg vectors) |
| `src/aot_compiler_load_callback.cpp` | SA load callback (delegates to `AotCompilerClient`) |
| `src/power_disconnected_listener.cpp` | Power disconnected event → `HandlePowerDisconnected()` |
| `src/screen_status_listener.cpp` | Screen on event → `HandleScreenOn()` |
| `src/thermal_mgr_listener.cpp` | Thermal level changed event → `HandleThermalLevelChanged()` |
| `sa_profile/5300.json` | SA registration (SA ID 5300, process "compiler_service", libpath "libcompiler_service.z.so") |
| `BUILD.gn` | Build targets: `libcompiler_service` (SA so), config files, SA profile |

## AotCompilerArgs Structure

The IPC argument struct (`include/aot_compiler_args.h`) carries:

| Group | Fields |
|-------|--------|
| **Identity** | `isSysComp`, `compileMode` (partial/full), `moduleArkTSMode` (dynamic/static/hybrid) |
| **Package info** | `bundleName`, `moduleName`, `appIdentifier`, `bundleUid`, `bundleGid`, `processUid` |
| **File paths** | `hapPath`, `abcPath`, `anFileName`, `outputPath`, `arkProfilePath`, `sysCompPath`, `pgoDir` |
| **ABC info** | `abcOffset`, `abcLength` |
| **Compilation options** | `optBCRangeList`, `isScreenOff`, `isEncryptedBundle`, `isEnableBaselinePgo` |
| **Extra ints** | `bundleType`, `triggerType`, `staticAndHybridModuleCnt` |
| **External packages** | `hspModules` (vector of `HspModuleInfo`: bundleName, moduleName, hapPath, moduleArkTSMode, offset, length) |

Serialization is grouped into 7 sections: Identity → PackageInfo → FilePaths → AbcInfo → CompilationOptions → ExtraInts → HspModules.

## Error Codes

Defined in `aot_compiler_error_utils.h`:

| Code | Value | Meaning |
|------|-------|---------|
| `ERR_OK` | 0 | Success |
| `ERR_OK_NO_AOT_FILE` | 10001 | No AOT file generated (no AP file, version check skip) |
| `ERR_AOT_COMPILER_PARAM_FAILED` | 10002 | Invalid arguments |
| `ERR_AOT_COMPILER_CONNECT_FAILED` | 10003 | Failed to connect to SA |
| `ERR_AOT_COMPILER_SIGNATURE_FAILED` | 10004 | Code signing failed |
| `ERR_AOT_COMPILER_SIGNATURE_DISABLE` | 10005 | Code signing disabled |
| `ERR_AOT_COMPILER_CALL_FAILED` | 10006 | Compiler execution failed |
| `ERR_AOT_COMPILER_STOP_FAILED` | 10007 | Failed to stop compiler |
| `ERR_AOT_COMPILER_CALL_CRASH` | 10008 | Child process crashed (non-SIGKILL signal) |
| `ERR_AOT_COMPILER_CALL_CANCELLED` | 10009 | Compilation cancelled (thermal/pause, or framework .an already exists) |

Child process exit codes are mapped via `RetInfoOfCompiler` in `aot_compiler_constants.h`. SIGKILL → `ERR_AOT_COMPILER_CALL_CANCELLED`, other signals → `ERR_AOT_COMPILER_CALL_CRASH`.

## Configuration Files

| File | Install Location | Purpose |
|------|-----------------|---------|
| `compiler_service.cfg` | `/system/etc/init/` | SA process init config |
| `ark_aot_compiler.cfg` | `/system/etc/init/` | Compiler child process init config |
| `system_framework_aot_enable_list.conf` | `/system/etc/ark/` | Framework AOT enable list |
| `static_app_install_aot_enable_list.conf` | `/system/etc/ark/` | Static app install AOT enable list |

## Build

Full OHOS tree only (depends on `samgr`, `ipc`, `code_signature`, `common_event_service`, etc.):
```bash
./build.sh --product-name <product-name> --build-target compiler_service:compiler_service
```

Conditional build flags:
- `CODE_SIGN_ENABLE` — enabled when `security_code_signature` part is present
- `ENABLE_COMPILER_SERVICE_GET_PARAMETER` — enabled for standard OHOS builds (allows reading system parameters like `ark.aot.enable_static_compiler`, `ark.aot.compiler_an_file_max_size`, `ark.aot.code_comment.enable`)

## Tests

Full OHOS tree only:
```bash
./build.sh --product-name <product-name> --build-target compiler_service/test:compiler_service_unittest
./build.sh --product-name <product-name> --build-target compiler_service/test:compiler_service_fuzztest
```

- Unit tests: `test/unittest/` — aotcompilerservice_unit, aotcompilerproxy_unit, aotcompilerclient_unit, aotcompilerstub_unit, aotcompilererrorutils_unit, aotcompilerimpl_unit, aotcompilerargshandler_unit, aotargsverify_unit
- Fuzz tests: `test/fuzztest/` — compilerinterfacestub_fuzzer, aotcompilerargsprepare_fuzzer
- Shared test config: `test/compiler_service_test.gni`

## How to Modify

**Add a new IPC method:**
1. Add the method declaration to `interface/iaot_compiler_interface.h`
2. Add the proxy implementation in `interface/aot_compiler_interface_proxy.cpp`
3. Add the stub dispatch case in `interface/aot_compiler_interface_stub.cpp`
4. Implement the logic in `src/aot_compiler_impl.cpp`
5. Wire through `src/aot_compiler_service.cpp`
6. Expose on client side in `src/aot_compiler_client.cpp`
7. Add unit test in `test/unittest/`

**Add a new system event listener:**
1. Create listener class in `include/` and `src/` (follow `power_disconnected_listener` pattern)
2. Register/unregister in `AotCompilerService::OnStart()`/`OnStop()`
3. Wire the callback to `AotCompilerImpl` (add `Handle*` method)

**Add a new argument to AotCompilerArgs:**
1. Add field to `include/aot_compiler_args.h` struct
2. Add serialization in the appropriate `Write*/Read*` helper method
3. Update `AotArgsVerify` if validation is needed
4. Update relevant parser in `aot_args_handler.cpp`

**Add a new parser type:**
1. Add to `AotParserType` enum in `aot_compiler_constants.h`
2. Create parser class inheriting `AOTArgsParserBase` in `aot_args_handler.h/cpp`
3. Add to `AOTArgsParserFactory::GetParser()`
4. Add validation branch in `AOTArgsParserBase::Check()`
5. Add FD arg name via `GetFdArgName()` override

## Boundaries

- IPC contract changes must be synchronized across `interface/`, `include/`, and `src/`.
- Keep `sa_profile/5300.json` consistent with the service process/lib name and SA ID.
- This module only orchestrates compilation — actual compiler logic lives in `ecmascript/compiler/`.
- Service lifecycle/listener logic stays in `src/`; do not mix test-only code into production sources.
- `AotArgsVerify::CheckBundleNameAndAppIdentifier` dynamically loads `libhapverify.z.so` — this dependency must be available on device.
- Error codes in `aot_compiler_constants.h::RetInfoOfCompiler` must stay in sync with `ets_runtime/ecmascript/aot_compiler.cpp` exit codes.
- `AotTriggerType` and `BundleType` enum values must match BMS definitions.
