# Debugger 子系统知识

本文只记录 `ecmascript/debugger/` 全量调试器组件：断点管理、单步执行、栈检查、热重载、调用帧操作、事件通知。CPU 采样分析见 `docs/knowledge/DFX-Subsystem.md`「性能采样」部分，tracing 见 DFX tracing 部分。已有子模块 AGENTS.md：`ecmascript/debugger/AGENTS.md`。

## 概述

Debugger 子系统在解释器执行路径上插入**断点检查、单步控制、方法入口/出口通知**，通过 `NotificationManager` 事件分发机制与外部调试协议层通信。最关键的架构边界：**调试器钩子在解释器热路径上，非调试模式（`isDebugMode_ == false`）下必须零开销或仅一次条件分支**。跨线程通信（调试线程 ↔ JS 执行线程）采用信号驱动+条件检查模型，非传统互斥锁阻塞模型。

## 核心模型和主链路

本节同时承担路由：按"触发词或任务"定位组件，根据"优先入口"读取代码，遵守"不变式/维护点"。

### 调试核心（断点 + 单步 + 状态管理）

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| JSDebugger / 核心调试器 | 实现 `JSDebugInterface` + `RuntimeListener`，管理 3 类断点集（breakpoints/smartBreakpoints/symbolicBreakpoints），条件断点求值，单步决策 | `ecmascript/debugger/js_debugger.h` | 断点匹配性能敏感；条件断点求值需在安全线程上下文中执行 |
| breakpoint / 断点设置/移除/匹配 | 3 种断点类型匹配逻辑不同：普通断点精确 offset 匹配、smart breakpoint 基于 PC 范围匹配、symbolic breakpoint 按函数名匹配 | `js_debugger.h` — `FindBreakpoint` | 断点集合 `CUnorderedSet`，遍历 O(n)；断点数量 < 1000 可接受，不要无限增长而不提供清理机制 |
| single step / 单步执行 / HandleStep | 每条 bytecode 执行后检查：`BytecodePcChanged` → `HandleStep`（O(1) 判断，不遍历断点集） | `js_debugger.h` — `HandleStep()` + `BytecodePcChanged()` | **这是最热的路径**——不要在此添加 O(n) 操作、堆分配、或可能触发 GC 的调用 |
| JsDebuggerManager / 调试器总协调器 | 持有 `DropframeManager`、`NotificationManager`、`HotReloadManager`。管理全局调试状态（debugMode、mixedDebug、signalInterrupt 等）| `ecmascript/debugger/js_debugger_manager.h` | 多状态标志位一致性；`jsDebuggerManagerMap_`（thread-local map）受 `shared_mutex` 保护；`DebuggerNativeScope`/`DebuggerManagedScope` 必须严格配对 |
| DebuggerApi / 调试公共 API | 静态方法：栈深度、帧遍历、变量/作用域读写、方法/bytecode 访问 | `ecmascript/debugger/debugger_api.h` | `DebuggerNativeScope`/`DebuggerManagedScope` 是 RAII scope，切换 `JSThread` 状态——必须严格配对，否则线程状态泄漏 |

### 帧操作与热重载

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| dropframe / 丢帧 / DropframeManager | 在每次 `MethodEntry`/`MethodExit` 记录词法变量修改和 Promise 队列大小，支持 "Drop Last Frame" 操作 | `ecmascript/debugger/dropframe_manager.h` | `MethodEntry`/`MethodExit` 必须配对——漏掉清理会导致词法变量记录泄漏；Promise 状态恢复正确性 |
| hot reload / 热重载 / HotReloadManager | 维护 base↔patch JSPandaFile 映射，管理 patch 文件的 DebugInfo 提取器 | `ecmascript/debugger/hot_reload_manager.h` | patch 加载/卸载与 GC/代码执行的时序；不要持有 JSPandaFile 裸指针跨越 GC |
| PauseReason / 暂停原因 | 枚举定义暂停原因（16 个值），由 `PtHooks` 回调传递给外部协议层 | `ecmascript/debugger/js_debugger_interface.h` | **只能末尾追加，不能删除/重排**——原因：外部调试协议依赖枚举序数 |
| PtHooks / 调试协议回调 | 16 个纯虚回调，由外部 `ProtocolHandler` 实现（在独立调试服务中，ets_runtime 仅 forward-declare）| `ecmascript/debugger/js_debugger_interface.h` | 新增纯虚方法=不向后兼容，需协调协议层同步发布；新增非纯虚方法（带默认实现）=向后兼容 |
| NotificationManager / RuntimeListener | listener 模式分发 VM 事件。`JSDebugger` 和 `Tracing` 都注册为 listener | `ecmascript/debugger/notification_manager.h` | listener 生命周期管理（AddListener/RemoveListener 野指针）；不要假设 debugger 是唯一 listener |

### 调试线程模型

```
调试协议线程（外部）                     JS 执行线程（解释器）
  SetSignalState(true)                  每条 bytecode 执行后：
    └─ isSignalInterrupt_ = true          ├─ IsDebugMode()? → 否：跳过（仅1次bool检查）
  (等待 JS 线程到达安全点)                ├─ BytecodePcChanged → 遍历 RuntimeListener
                                          │   ├─ JSDebugger::BytecodePcChanged() → 断点/单步
                                          │   └─ Tracing::BytecodePcChanged()
                                          └─ RESET_AND_JUMP_IF_DROPFRAME()
  GetSignalState()==false? → 继续等待    断点命中 → 暂停执行
```

关键约束：`isSignalInterrupt_` 无锁（单写/多读弱一致性）；`listeners_` 无锁（仅初始化/销毁时修改，运行时只读）。

## 边界和分类

| 概念 | 本模块实现 | 不要混用 |
|---|---|---|
| JSDebugger | JS/TS 调试器，在解释器层面插入断点和单步控制 | 不是 IDE 调试器前端（如 VS Code Debugger）；不是协议层（CDP 解析在外部调试服务中） |
| breakpoint | 3 种类型：普通（精确 offset）、smart（PC 范围）、symbolic（函数名匹配）| 不要假设所有断点匹配逻辑相同 |
| dropframe | 调用帧回滚，恢复词法变量和 Promise 状态 | 不是 C++ 栈回滚；不是异常展开（stack unwinding） |
| hot reload | 运行时替换 .abc 文件中的方法 | 不是冷补丁（Cold Patch，见 `docs/knowledge/Patch-Subsystem.md`）；不是 AOT 重编译 |
| PauseReason | 暂停原因枚举，影响外部调试器行为 | 不要删除/重排枚举值——协议层依赖序数 |
| DebuggerNativeScope / DebuggerManagedScope | 切换 `JSThread` 状态的 RAII scope | 必须配对使用，不能跨线程混用 |

## 约束规则

- **禁止**在 `BytecodePcChanged`（每条 bytecode 执行后调用）中做重操作（O(n)、堆分配、GC 触发）——这是最热的路径
- **禁止**在条件断点求值时执行可能触发 GC 的操作，除非已验证在安全线程上下文中
- **禁止**让断点集合无限增长而不提供清理机制
- **禁止**绕过 `DebuggerNativeScope`/`DebuggerManagedScope` 直接操作线程状态——历史坑：配对不完整导致线程状态泄漏
- **禁止**在 `DropframeManager` 的 `MethodExit` 中漏掉清理——导致词法变量记录泄漏
- **禁止**修改 `PauseReason` 枚举值的前向兼容性（只能末尾追加）
- **禁止**在 `hot_reload_manager` 中持有 JSPandaFile 裸指针跨越 GC
- **禁止**对 `jsDebuggerManagerMap_` 在未持有 `shared_mutex` 的情况下访问——多线程读/单线程写
- **必须**保证非调试模式（`isDebugMode_ == false`）下零开销或仅一次条件分支——`interpreter-inl.h:168` 处仅 1 次 bool 检查
- `NotificationManager::listeners_` 有 2 个实现者（`JSDebugger` + `Tracing`）——不要假设只有 debugger 是唯一的 RuntimeListener
- `PtHooks` 变更前必须确认协议层有对应改动并同步合入

## 修改前检查

- 改 `js_debugger.h/cpp` 断点/单步逻辑前，确认变更是否在 `BytecodePcChanged` 热路径上 → 新增代码必须 O(1) 且无堆分配
- 改 `js_debugger_interface.h` 前，确认 `PauseReason` 是否只末尾追加、`PtHooks` 是否保持向后兼容 → 协议层同步
- 改 `dropframe_manager.*` 前，确认 `MethodEntry`/`MethodExit` 配对完整
- 改 `debugger_api.*` 前，确认 `DebuggerNativeScope`/`DebuggerManagedScope` 配对
- 改 `js_debugger_manager.*` 前，确认多状态标志位一致性和 thread-local map 的 `shared_mutex` 保护
- 涉及跨线程操作前，确认线程安全（`isSignalInterrupt_` 单写/多读、`jsDebuggerManagerMap_` shared_mutex）
- 非调试模式下的性能回归验证：debug mode 关闭时，`interpreter-inl.h` 处仅 1 次 bool 检查 + 1 次条件分支

## 代码和测试

### 代码锚点

| 功能 | 入口文件 |
|---|---|
| 核心调试器（断点/单步） | `ecmascript/debugger/js_debugger.cpp` |
| 调试器管理器 | `ecmascript/debugger/js_debugger_manager.cpp` |
| 调试 API | `ecmascript/debugger/debugger_api.cpp` |
| 帧丢弃 | `ecmascript/debugger/dropframe_manager.cpp` |
| 热重载 | `ecmascript/debugger/hot_reload_manager.cpp` |
| 事件通知 | `ecmascript/debugger/notification_manager.h` |
| 调试接口定义 | `ecmascript/debugger/js_debugger_interface.h` |
| 解释器入口（热路径） | `ecmascript/interpreter/interpreter-inl.h:168` — `IsDebugMode()` 检查 |

### 测试锚点

| 变更类型 | 测试目标 |
|---|---|
| 断点/单步/Debugger 语句 | `RuntimeDebuggerTest` |
| Drop Frame | `DropframeManagerTest` |
| Hot Reload | `HotReloadManagerTest` + 确认 `tests/single_file/patch/` 固件兼容 |
| 调试 API / 接口定义 | `RuntimeDebuggerTest` + 全量 debugger 测试 |
| `PauseReason`/`PtHooks` 变更 | 全量 debugger 测试 + 确认外部调试器兼容性 |

构建从 workspace root 执行：
```bash
cd /home/xiongkang/workspace/ark_standalone_build
ninja -C out/x64.release arkcompiler/ets_runtime:libark_jsruntime
ninja -C out/x64.release arkcompiler/ets_runtime:ark_unittest
```
