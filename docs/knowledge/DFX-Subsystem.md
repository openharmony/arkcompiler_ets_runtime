# DFX 子系统知识

本文只记录 `ecmascript/dfx/` 全量 7 个子模块的诊断、观测、性能分析能力。GC 内存管理机制见 `ecmascript/mem/`，编译器调试信息生成见 `ecmascript/compiler/`。各子模块已有独立 AGENTS.md。

## 概述

DFX 子系统以**旁路观测**方式提供堆内存快照、CPU 采样、调用栈追踪、JIT 代码 dump、VM 统计、tracing 等诊断能力。最关键的架构边界：**DFX 代码不得修改被观测对象状态；开关关闭时必须零开销或仅一次条件分支**。所有外部接口通过 `DFXJSNApi`（`ecmascript/napi/dfx_jsnapi.cpp`）统一暴露，子模块不得自行暴露公共 API。

## 核心模型和主链路

本节同时承担路由：先按"触发词或任务"定位子模块，再根据"优先入口"读取代码，同时遵守"不变式/维护点"中的约束。

### 堆分析（hprof）

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| heap dump / heap snapshot / 堆快照 / EntryIdMap / nodeAddressIdMap | 堆快照构建器构建 `entryIdMap_`（对象地址→NodeId），`HeapSnapshot::BuildUp→FillNodes→FillEdges` 生成节点/边图。支持 sync（直接构建）和 async（fork 子进程）两种模式 | `ecmascript/dfx/hprof/` + `hprof/AGENTS.md`；序列化：`heap_snapshot_json_serializer.h` | **每次 dump 前必须 `BuildNodeAddressIdMap()`（含 `clear()`）**，不要用 `UpdateNodeAddressIdMap()` 直接追加——原因：后者只追加不清理，跨 dump 数据污染 |
| heap tracker / 堆追踪 | `HeapTracker` 按时间/分配量触发追踪，三个测试类分别覆盖不同追踪策略 | `ecmascript/dfx/hprof/` + `hprof/AGENTS.md` | 追踪开销控制与触发策略配置耦合 |
| heap sampling / 堆采样 | `HeapSampling` 基于采样的内存分配记录，非全量 | `ecmascript/dfx/hprof/` + `hprof/AGENTS.md` | 采样率过低会遗漏泄漏，过高开销不可接受 |
| rawheap / raw heap translate | `RawHeapTranslate` 将原始堆格式翻译为标准格式 | `ecmascript/dfx/hprof/rawheap_translate/` + `hprof/AGENTS.md` | 格式兼容性敏感，变更需向后兼容或提供版本号 |
| handle leak detect / 句柄泄漏检测 | `LocalHandleLeakDetect`/`GlobalHandleLeakDetect` 追踪 handle 分配/释放配对 | `ecmascript/dfx/hprof/` + `hprof/AGENTS.md` | 误报会导致错误诊断；默认关闭（`enable_handle_leak_detect=false`） |
| Hybrid Heap / 混合堆采样分析 | 混合堆采样分析工具，与标准 hprof 路径不同 | `ecmascript/dfx/hprof/hybrid/` | 不要假设 hybrid 和标准 hprof 共用同一套映射/序列化逻辑 |

### 性能采样（CPU 采样器 + vmstat）

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| CPU 采样器 / SIGPROF / 信号采样 | 基于 SIGPROF 信号的 CPU 采样，`SamplesRecord` 记录调用栈样本，输出 `CpuProfileNode` 树 | `ecmascript/dfx/` 下 CPU 采样子模块及 AGENTS.md | **信号安全是高位区**：SIGPROF handler 中禁止调用 malloc/printf/锁——原因：handler 可能在主线程持有 malloc 锁时被信号中断，导致死锁；采样缓冲溢出需处理 |
| EcmaRuntimeStat / 调用耗时统计 | `RuntimeTimerScope` RAII 统计调用耗时，宏控制开关 | `ecmascript/dfx/vmstat/ecma_runtime_stat.h` | timer 嵌套层级必须正确配对；关闭宏=零开销 |
| FunctionCallTimer / 函数计时 | JS/TS 函数计时，区分 AOT/解释器路径。宏 `ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER` 控制 | `ecmascript/dfx/vmstat/function_call_timer.h` | 默认关闭，编译宏控制 |
| 字节码路径计数 / 类型化操作计数 | bytecode typed/slow path 计数 + 编译 opcode 统计。宏控制 | `ecmascript/dfx/vmstat/` | 默认关闭 |
| JIT 预热标记 / JIT 预热 | JIT 预热标记，单例模式 | `ecmascript/dfx/vmstat/` | 单例生命周期管理 |

### 调用栈与追踪（stackinfo + tracing）

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| stackinfo / stack trace / async stack / 调用栈追踪 | `JsStackInfo` 追踪同步调用栈 + `AsyncStack` 追踪 Promise 链异步栈 | `ecmascript/dfx/stackinfo/` + `stackinfo/AGENTS.md` | 栈帧类型识别（JS/Native/AOT）易出错；Promise 链追踪需完整覆盖 resolve/reject 分支 |
| tracing / event tracing / 事件追踪 | `Tracing` 实现 `RuntimeListener`，提供 Chrome 兼容事件追踪（CPU profile 事件、内存事件）。缓冲区上限 200MB，受 `lock_` 保护 | `ecmascript/dfx/tracing/tracing.h` | 追踪点数量影响主链路；debugger + tracing 同时开启时 `BytecodePcChanged` 执行两次 listener 回调，开销叠加——不要假设只有 debugger 是唯一 RuntimeListener |

### 其余（dump_code + VmThreadControl + NativeModuleFailureInfo）

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| dump_code / JIT dump / ELF dump | `JsJitDumpElf` 手动构造 ELF 文件（text/symbol/string/rela section），写入 JIT 编译产出的机器码 | `ecmascript/dfx/dump_code/jit_dump_elf.h` | ELF section 偏移计算手动进行，无格式校验，易出错；无独立测试，变更后需编译验证 |
| VmThreadControl / VM 线程挂起/恢复 | Mutex + ConditionVariable 实现线程挂起/恢复同步 | `ecmascript/dfx/vm_thread_control.h` | 挂起和恢复必须严格配对；注意与 GC 的锁序关系——死锁风险 |
| NativeModuleFailureInfo | 继承 `JSObject`，记录 native 模块加载失败信息字符串 | `ecmascript/dfx/native_module_failure_info.h` | 失败信息字符串生命周期管理 |

## 边界和分类

| 概念 | 本模块实现 | 不要混用 |
|---|---|---|
| DFX | 旁路诊断观测子系统，不参与 JS 执行主链路 | 不要把 DFX 代码写成主链路逻辑；不要在主链路中无条件调用 DFX |
| hprof | JSEcmaVM 堆分析器，生成 Chrome Developer Tools 兼容的堆快照 | 不是 GC 本身（GC 在 `ecmascript/mem/`）；不是 C++ 堆分析（如 Valgrind Massif） |
| CPU 采样器 | 基于 SIGPROF 信号采样的 CPU 采样器，与平台信号机制耦合 | 不是计时器-based 性能分析工具；不是插桩式性能分析工具 |
| stackinfo | JS 层面的调用栈追踪 | 不是 C++ backtrace（`backtrace()`）；不是 Linux perf stack |
| Tracing | Chrome 兼容的 `RuntimeListener` 事件追踪 | 不是 Linux ftrace/perf trace；不是 HiLog/Hiview |
| nodeAddressIdMap | handle 地址 → NodeId 的映射，每次 dump 重建 | 不要跨 dump 复用——每次 dump 前必须 `BuildNodeAddressIdMap()` |
| dump_code | JIT 编译产出的 ELF 格式 dump | 不是编译器输出的 .o/.so 文件；不是 AOT 编译产物 |
| vmstat | VM 内部统计（调用耗时/bytecode计数/JIT预热标记）| 不是应用层性能监控；不是 tracing 事件 |

## 约束规则

- **禁止**在 SIGPROF handler 中调用非异步信号安全函数（malloc、printf、锁等）。原因：主线程持有锁时被信号中断 → handler 内尝试获取同一把锁 → 死锁
- **禁止**为通过测试删除 DFX 日志、事件、错误码或诊断信息
- **禁止**在 dump/heap 路径上分配大内存而不检查 OOM。原因：fork 后 COW 触发，内存翻倍可能超出设备限制
- **禁止**修改堆快照/CPU profile 的 JSON 输出格式而不更新对应序列化测试（`HeapSnapShotTest`、`RawHeapTranslateTest` 等）
- **禁止**绕过 `DFXJSNApi` 直接在子模块暴露 public 接口
- **禁止**在 dump/heap 路径中添加同步阻塞 I/O（必须异步或 fork 隔离）
- **禁止**在 DFX 子模块之间引入循环依赖
- **禁止**让 DFX 功能默认开启而不提供开关（编译时或运行时）
- **必须**使用 `BuildNodeAddressIdMap()`（含 `clear()`）而非 `UpdateNodeAddressIdMap()`——历史坑：未 clear 导致跨 dump 数据污染
- **必须**保持 DFX 输出格式（JSON schema、二进制格式）向后兼容，或提供版本号迁移路径
- 堆快照/CPU profile 的构建必须在**一致的 VM 状态**下完成（中间不能发生 GC 或代码卸载）
- `nodeAddressIdMap_` 不要假设跨 dump 保持有效
- 大快照场景的 fork 路径需验证 OOM 处理（历史坑：COW 触发导致设备内存超限崩溃）

## 修改前检查

- 改 CPU 采样模块前，确认变更是否在信号处理器路径上 → 如果是，逐一检查每个调用的异步信号安全性
- 改 `hprof/` 序列化前，确认 JSON 格式变更是否向后兼容 → 更新 `HeapSnapShotTest` + `RawHeapTranslateTest`，手动验证 Chrome Developer Tools 兼容性
- 改 `DFXJSNApi` 公共接口前，确认签名/错误码/生命周期不变 → 公共 NAPI 变更需要额外审批
- 改 DFX 开关逻辑前，确认关闭状态下零开销（不超过一次条件分支）
- 新增堆分配前，确认 OOM 路径有处理
- 涉及文件/路径常量（`constants.h`）前，确认与 OpenHarmony 设备文件系统布局一致

## 代码和测试

### 代码锚点

| 功能 | 入口文件 |
|---|---|
| 堆快照构建 | `ecmascript/dfx/hprof/`、`heap_snapshot.cpp` |
| 堆快照 JSON 序列化 | `ecmascript/dfx/hprof/heap_snapshot_json_serializer.cpp` |
| RawHeap 翻译 | `ecmascript/dfx/hprof/rawheap_translate/` |
| Handle 泄漏检测 | `ecmascript/dfx/hprof/` |
| CPU 采样器 | `ecmascript/dfx/` 下 CPU 采样子模块（`samples_record.cpp` 等） |
| 调用栈追踪 | `ecmascript/dfx/stackinfo/js_stackinfo.cpp`、`async_stack.cpp` |
| JIT Dump | `ecmascript/dfx/dump_code/jit_dump_elf.cpp` |
| Tracing | `ecmascript/dfx/tracing/tracing.cpp` |
| VM 统计 | `ecmascript/dfx/vmstat/` |
| VM 线程控制 | `ecmascript/dfx/vm_thread_control.cpp` |
| DFX 公共 NAPI | `ecmascript/napi/dfx_jsnapi.cpp` |

### 测试锚点

| 变更类型 | 测试目标 |
|---|---|
| CPU 采样器 | CPU 采样单元测试 + `SamplesRecordTest` |
| 堆快照/序列化 | `HeapSnapShotTest` + `RawHeapTranslateTest` |
| 堆追踪 | `HeapTrackerFirstTest` + `HeapTrackerSecondTest` + `HeapTrackerThirdTest` |
| 堆采样 | `HeapSamplingTest` |
| 栈追踪 | `JsStackInfoTest` + `AsyncStackTest` |
| Handle 泄漏检测 | `LocalHandleLeakDetectTest` + `GlobalHandleLeakDetectTest` |
| JSON 输出格式 | 对应序列化测试 + 手动 Chrome Developer Tools 兼容性验证 |
| dump_code / tracing / vmstat / 根文件 | 无独立测试目标，验证方式：`ninja -C out/x64.release arkcompiler/ets_runtime:libark_jsruntime` 确认编译通过 |

构建从 workspace root 执行：
```bash
cd /home/xiongkang/workspace/ark_standalone_build
ninja -C out/x64.release arkcompiler/ets_runtime:libark_jsruntime
ninja -C out/x64.release arkcompiler/ets_runtime:ark_unittest
```
