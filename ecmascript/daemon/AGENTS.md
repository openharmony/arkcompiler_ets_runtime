# AGENTS
**Name**: Daemon
**Purpose**: Daemon is a background daemon thread module for the ArkTS runtime (ecmascript). It provides asynchronous execution of Garbage Collection (GC) related tasks with features including:
- Concurrent marking for CMC (Concurrent Mark-Compact) garbage collector
- Asynchronous garbage collection execution
- Unified GC marking support
- Task grouping mechanism to prevent duplicate task submission
- Singleton pattern for thread lifecycle management
- QoS (Quality of Service) priority adjustment support
- Thread-safe task queue with mutex and condition variable synchronization
**Primary Language**: C++

## Directory Structure
```text
daemon/
├── daemon_task.h                                              # Base DaemonTask class and task type definitions
├── daemon_task-inl.h                                          # Inline implementations for concrete task types (templates)
├── daemon_thread.h                                            # DaemonThread class definition
└── daemon_thread.cpp                                          # DaemonThread implementation
```

## Building
The daemon module is built as part of the ArkCompiler module in the GN build system. There is no independent BUILD.gn file in the daemon directory. The source files are included in the main ecmascript BUILD.gn configuration:
```bash
# Build for OHOS ARM64
./build.sh --product-name rk3568 --gn-args use_musl=true --target-cpu arm64 --build-target ark_js_packages

# Build for OHOS ARM32
./build.sh --product-name rk3568 --build-target ark_js_packages

# Debug build (add to any command)
--gn-args is_debug=true
```

## Test Suite
The daemon module has comprehensive unit tests located in the ecmascript/tests directory:
```bash
# Build and generate an executable file
./build.sh --product-name rk3568 --build-target GC_Daemon_Test
# Send the executable file to the device using hdc
hdc shell send ./out/rk3568/tests/unittest/ets_runtime/GC_Daemon_Test /data/local/tmp/
# Run the executable file
hdc shell
cd /data/local/tmp/
chmod 777 ./GC_Daemon_Test
./GC_Daemon_Test
```

Test coverage includes:
- Task posting and execution (`PostTaskWithDaemon`, `PostTaskTwiceWithDaemon`)
- Duplicate task prevention across task groups
- Multi-threaded task submission scenarios
- Daemon thread lifecycle management (pre-fork/post-fork scenarios)
- Mock task implementations for testing

## Dependency
### Package Dependencies
| Dependency | Purpose |
|------------|---------|
| `common_components/taskpool` | TaskPool integration (runner.h, taskpool.h) |
| `ecmascript/js_thread` | Base JSThread class for thread management |
| `ecmascript/mem/heap` | Heap memory management interface |
| `ecmascript/mem/shared_heap` | SharedHeap for concurrent GC operations |
| `ecmascript/mutator_lock` | MutatorLock for thread synchronization |
| `ecmascript/platform/mutex` | Mutex and ConditionVariable primitives |
| `ecmascript/runtime` | Runtime lifecycle management |
| `qos` (optional) | Quality of Service priority management |

### External Directory Interactions
- `../mem/` - Memory management subsystem (Heap, SharedHeap)
- `../mem/shared_heap/` - Shared heap implementation for concurrent marking
- `../platform/` - Platform-specific synchronization primitives
- `../../common_components/taskpool/` - Shared TaskPool component for concurrent task execution
- `../runtime.cpp` - Runtime startup/shutdown integration

### Component Lifecycle Integration
The DaemonThread is created and destroyed by the Runtime:
- **Creation**: `Runtime::PreCreateVm()` calls `DaemonThread::CreateNewInstance()`
- **Destruction**: `Runtime::DestroyIfLastVm()` calls `DaemonThread::DestroyInstance()`

## Boundaries
### Allowed Operations
- ✅ Add new daemon task types by extending the `DaemonTask` base class
- ✅ Implement new GC-related tasks by creating template specializations
- ✅ Modify task scheduling algorithms within `DaemonThread::Run()`
- ✅ Extend task group categories for new task types
- ✅ Add new QoS priority levels or scheduling policies
- ✅ Extend synchronization mechanisms for thread safety

### Prohibited Operations
- ❌ Do NOT directly modify the singleton instance (`instance_`) - use provided static methods
- ❌ Do NOT bypass the task grouping mechanism (`postedGroups_`, `runningGroup_`)
- ❌ Do NOT post tasks without checking daemon thread running state
- ❌ Do NOT modify `SharedMarkStatus` from non-daemon threads without proper synchronization
- ❌ Do NOT call `FinishRunningTask()` from outside the daemon thread context
- ❌ Do NOT destroy the daemon thread while tasks are still pending
- ❌ Do NOT use raw thread creation - always use the `DaemonThread` abstraction
