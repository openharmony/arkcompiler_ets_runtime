# GC 领域知识

本文记录 ets_runtime GC 子系统的核心架构与专家经验，覆盖 LocalHeap GC 和 SharedHeap GC。
枚举值、空间列表等可从代码中直接获取的细节不在此重复。

---

## 核心架构：双层堆

```
BaseHeap                       ← Local/Shared GC 互斥（读写锁 / STW / 并发适配，详见线程模型章节）
  ├─ Heap (LocalHeap)          ← 每 VM 一个，JSThread 驱动 GC
  └─ SharedHeap (全局单例)     ← 跨 VM 共享，DaemonThread 驱动 GC
```

- LocalHeap 空间分为可移动（SemiSpace、OldSpace、CompressSpace）和不可移动（NonMovable、HugeObject、MachineCode、ReadOnly）两类。
- SharedHeap 空间结构与 LocalHeap 对称，均以 Shared 前缀命名。
- 内存管理以 Region（256KB）为基本单位，GC 标记/复制/压缩均按 Region 粒度操作。

## GC 类型与触发路径

| TriggerGCType | 堆 | 算法 | 典型触发 |
|---|---|---|---|
| `YOUNG_GC` | Local | SemiSpace 复制 | Young Space 满 |
| `OLD_GC` | Local | 部分 Mark-Sweep-Compact + 并发标记 | Old Space 超限 |
| `FULL_GC` | Local | 全堆 Mark-Sweep-Compact | OldGC 后仍不足、Native 内存超限 |
| `LOCAL_CC` | Local | 并发复制 | 配置开启且 Old Space 超限 |
| `SHARED_GC` | Shared | Mark-Sweep + 并发标记 + 并发 Sweep | 共享对象分配超限 |
| `SHARED_PARTIAL_GC` | Shared | 部分 Mark-Sweep（仅 CSet 区域） | 碎片化触发 |
| `SHARED_FULL_GC` | Shared | 全堆 Mark-Sweep-Compact | OOM 前最后尝试 |
| `SHARED_CC` | Shared | 并发复制（实验性） | 配置开启 |
| `APPSPAWN_FULL_GC` | Local | 压缩到 AppSpawn 空间 | Fork 前 |
| `UNIFIED_GC` | 跨 VM | 统一多 VM GC | 跨 VM 引用清理 |

GC 从轻到重逐级升级，`ALLOCATION_FAILED` 是 OOM 前最后一次尝试，**不要在此路径增加新的堆分配**。

## 线程模型与并发控制

### 线程状态与 STW 原理
- JSThread 的 `ThreadState`（RUNNING / NATIVE / IS_SUSPENDED）是 STW 的协作基础：
  - **RUNNING**：执行 JS 字节码、读写堆，STW 期间须在 checkpoint 被阻塞。
  - **NATIVE**：进入 C++/native，STW 期间**允许继续**，但切回 RUNNING 时被阻塞。
  - **IS_SUSPENDED**：被 `SuspendAll` 挂起。
- **STW 本质**：`SuspendAllScope`（`ecmascript/checkpoint/thread_state_transition.h`）发起 → `Runtime::SuspendAll` → `SuspendAllThreadsImpl` 对每个 JSThread 设 `SUSPEND_REQUEST` + `ACTIVE_BARRIER`。RUNNING 线程在 `CheckSafepoint()` 被阻塞；NATIVE 线程允许在 native 继续、切回 RUNNING 时先过 barrier 再被阻塞。即**阻止 JS 线程在 running 态运行（允许 native 态）以防止读写堆内存，保证 GC 安全**——并非"停所有线程"。

### Local/Shared GC 互斥（多机制并存，非仅读写锁）
- **读写锁 `gcExclusiveRWLock_`**（`BaseHeap` 静态 RWLock）：Local GC 取**读锁**（多个 VM 的 Local GC 可并发），SharedCC 取**写锁**独占。**不要绕过此锁**。
- **SharedFullGC 靠全程 STW**：`SharedFullGC::RunPhases` + `SetGCThreadQosPriority(STW)` + `SuspendAll`，STW 本身阻塞所有 JSThread（含 Local GC mutator），从而阻止 Local GC 运行。
- **SharedPartialGC 靠并发安全适配**：`SharedPartialGC::RunPhases` + `CheckOngoingConcurrentMarking` + 并发 marker/sweeper + 屏障，不全程 STW，靠并发标记/清扫与屏障安全协调。

### SharedGC 线程分工与重入防护
- **SharedGC 线程模型**：由 `DaemonThread` 驱动 GC，JSThread 通过 `SharedHeap::WaitGCFinished()` 等待，**不要在 JSThread 中直接执行 SharedGC 主体**。
- **GC 重入防护**：`RecursionScope` 防止递归触发 GC，违反会 `LOG_GC(FATAL)`。

## 写屏障

- **Local Heap**（`Barriers::Update`）：记录 Old→Young 跨代引用到 RSet；并发标记时 SATB 语义——新引用必须 Push 到标记栈，**不要跳过**。
- **Shared Heap**（`Barriers::UpdateShared`）：记录跨 Region 引用到 RSet；将 Shared 对象 Push 到 `SharedGCWorkManager`。
- 所有跨代/跨 Region 写操作必须经过屏障，**不要绕过写屏障直接修改对象引用**。

## 空间方向性（易错点）

- `SwapNewSpace()`：交换 active/inactive SemiSpace，仅在 YoungGC 中使用。
- `SwapOldSpace()`：交换 OldSpace/CompressSpace。**交换后 CompressSpace 是旧 OldSpace（FROM 空间），不是 Swap 之后的 OldSpace**。这是最常见的方向性错误。
- `MergeToOldSpaceSync()`：合并到 OldSpace 需要锁或 STW。

## 弱引用处理

两阶段模型：
1. **STW 中**：`ProcessWeakReference()` 处理并发标记队列中的弱引用。
2. **GC 后**：清理 Native 绑定、全局弱引用存储、StringTable 等。

回调规则：
- FROM 区域未转发的对象视为已死，**必须返回 `nullptr`，不要返回原对象**。
- 必须检查 `!objectRegion`（空指针防护）。
- 必须调用 `ClearVMCachedConstantPool()` 清理缓存。

## GC 抑制场景

- **SmartGC 敏感期**（`ENTER_HIGH_SENSITIVE`）：GC 被抑制，退出后可能触发 pending GC。**不要在敏感期强制触发 GC**，除非 `forceGC_` 为 true。
- **启动期**（`ON_STARTUP`）：GC 被抑制；`JUST_FINISH_STARTUP` 时尝试 Compress 回收。
- **并发标记开关**：`CONFIG_DISABLE` 表示配置级关闭，**不可通过代码恢复**。

## 关键代码入口

| 文件 | 职责 |
|---|---|
| `ecmascript/mem/heap.h` / `heap.cpp` | Heap/SharedHeap 定义，`CollectGarbageImpl()` GC 触发入口 |
| `ecmascript/mem/garbage_collector.h` | GC 抽象接口 `RunPhases()` |
| `ecmascript/mem/full_gc.cpp` | Local Full GC |
| `ecmascript/mem/partial_gc.cpp` | Local Partial/Old GC |
| `ecmascript/mem/concurrent_marker.h/cpp` | 本地并发标记器 |
| `ecmascript/mem/barriers.h` / `barriers.cpp` / `barriers-inl.h` | 读写屏障 |
| `ecmascript/mem/region.h` | Region 定义与标志位 |
| `ecmascript/mem/space.h` / `space.cpp` | 空间抽象层 |
| `ecmascript/mem/shared_heap/shared_gc.cpp` | SharedGC 实现 |
| `ecmascript/mem/shared_heap/shared_full_gc.cpp` | SharedFullGC（全程 STW） |
| `ecmascript/mem/shared_heap/shared_partial_gc.cpp` | SharedPartialGC（并发标记/清扫） |
| `ecmascript/mem/shared_heap/shared_cc.h/cpp` | SharedCC（实验性，取写锁） |
| `ecmascript/common_enum.h` | `TriggerGCType`, `GCReason`, `MarkType` 等枚举定义 |
| `ecmascript/js_thread.h/cpp` | ThreadState 状态机、CheckSafepoint、SharedCCStatus |
| `ecmascript/checkpoint/thread_state_transition.h` | `SuspendAllScope`/`SuspendAllAction`（STW 入口） |
| `ecmascript/runtime.cpp` | `Runtime::SuspendAll` / `SuspendAllThreadsImpl`（设 SUSPEND_REQUEST+ACTIVE_BARRIER） |
| `ecmascript/daemon/daemon_thread.h/cpp` | DaemonThread GC 驱动 |

## 测试与验证

GC 测试入口在 `ecmascript/tests/` 目录，关键测试目标：

```bash
./build.sh --product-name rk3568 --build-target GC_First_Test --build-target GC_Second_Test \
  --build-target GC_Third_Test --build-target GC_Verify_Test --build-target GC_SharedPartial_Test \
  --build-target Shared_Heap_Test --build-target Unified_GC_Test --build-target Weak_Ref_Old_GC_Test
```

- 堆验证（`VerifyKind`）：GC 各阶段可开启验证（`VERIFY_PRE_GC`/`VERIFY_POST_GC`、`VERIFY_MARK_*`、`VERIFY_EVACUATE_*`、`VERIFY_WEAK_REF`），debug 构建下不要禁用。
- 修改 GC visitor 逻辑时，要确保所有空间类型的对象布局变化都被正确处理。

## 修改前必检项

- 修改非 STW 区间的 GC 状态时，要确认操作的原子性和线程安全性。
- 修改 Copy/Evacuate GC 流程时，要确认 FROM/TO 空间方向正确（Swap 后 FROM = 旧空间）。
- 新增对象引用赋值路径时，要确认写屏障已正确覆盖跨代/跨 Region 引用。
- 修改弱引用回调逻辑时，要确认 FROM 区域未转发对象返回 `nullptr` 而非原指针。
- 计算变长对象（String、Array）大小时，要使用 `SizeFromJSHClass()` 而非 `GetSize()`。
- 触发 GC 前，要确认当前不在 SmartGC 敏感期或启动期，或 `forceGC_` 为 true。
- 修改 SharedHeap 相关逻辑时，要确认跨 VM 的 SharedGC 生命周期未被破坏。
- 修改 Local/Shared GC 互斥逻辑时，要确认所用机制（SharedFullGC 全程 STW / SharedPartialGC 并发适配 / SharedCC 写锁）与该 GC 类型一致，不要默认所有 SharedGC 都只靠 `gcExclusiveRWLock_`。
- 修改对象布局或新增 GC visitor 时，要确认所有空间的 visitor 已同步更新。
- 修改并发标记/并发 Sweep 逻辑时，要确认 MarkWord 标记位的修改使用了原子操作。
- 修改 Region 标志位时，要确认不在并发标记期间修改，或已持有正确的锁。
