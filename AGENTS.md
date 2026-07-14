# ArkTS AGENTS.md (ets_runtime)

## 1. 代码地图

本 AGENTS.md 适用于 `arkcompiler/ets_runtime`。
本仓库实现 ArkTS 运行时——OpenHarmony 上执行 ArkTS/TS/JS 的默认运行时。
提供字节码解释执行、AOT/JIT 编译、模块加载，并发共享，内存管理（GC）、NAPI C++ 互操作及 ES2021 标准库。

### 关键目录（按修改频率与风险分级）

| 目录 | 风险 | 原因 |
|-----------|------|-----|
| `ecmascript/interpreter/` | 🔴 高 | 核心节码执行引擎；此处的缺陷会影响所有 ArkTS 应用 |
| `ecmascript/module/` | 🔴 高  | ES 模块加载器；与 CommonJS 互操作 |
| `ecmascript/mem/` | 🔴 高 | GC 与分配器；对性能和稳定性高度敏感 |
| `ecmascript/napi/` | 🟡 中 | 公共 C++ API 接口层；兼容性敏感 |
| `ecmascript/builtins/` | 🟡 中 | ES 标准库；符合ECMA2021规范 |
| `ecmascript/compiler/` | 🟡 中 | AOT 编译器；类型推断 + 代码生成，与前端紧密相关 |
| `ecmascript/jit/` | 🟡 中 | JIT 编译器；性能敏感 |
| `ecmascript/ic/` | 🟡 中 | 内联缓存；与解释器紧密耦合 |
| `ecmascript/ts_types/` | 🟡 中 | 类型元数据；影响 AOT 编译质量 |
| `ecmascript/dfx/` | 🟡 中 | 诊断/观测/性能分析（CPU 采样、堆快照、栈追踪、tracing） |
| `ecmascript/debugger/` | 🟡 中 | JS调试器（断点/单步/热重载/DropFrame） |
| `ecmascript/extractortool/` | 🟡 中 | HAP/ZIP文件提取与SourceMap解析 |
| `ecmascript/patch/` | 🟡 中 | Quick Fix/Hot Reload/Cold Patch方法替换 |
| `common_components/` | 🟢 低 | 共享基础设施；稳定，较少变更 |
| `test/` | 🟢 低 | 测试相关 |
| `docs/` | 🟢 低 | 仅文档 |

### 任务-路径快速索引

- 新增/修改字节码指令 → `ecmascript/interpreter/` + `ecmascript/ic/`
- 新增 NAPI C++ API → `ecmascript/napi/` + `test/`
- GC 或内存变更 → `ecmascript/mem/` + `common_components/heap/`
- AOT/JIT 编译器变更 → `ecmascript/compiler/` + `ecmascript/stubs/`
- ES 标准库变更 → `ecmascript/builtins/` + 对照 ECMA2021 规范
- 模块加载器变更 → `ecmascript/module/` + `ecmascript/require/`
- 调试器变更 → `ecmascript/debugger/` + `docs/knowledge/Debugger-Subsystem.md`
- DFX/Profiling/堆快照变更 → `ecmascript/dfx/` + `docs/knowledge/DFX-Subsystem.md`
- ExtractorTool/HAP提取/SourceMap变更 → `ecmascript/extractortool/` + `docs/knowledge/ExtractorTool-Subsystem.md`
- Quick Fix/热重载/Patch变更 → `ecmascript/patch/` + `docs/knowledge/Patch-Subsystem.md`
- 构建系统变更 → 根目录 `BUILD.gn` + `bundle.json`

## 2. 知识路由

在规划或编辑代码之前，先对任务进行分类并阅读相应的文档。
在计划中说明：任务类别、已读文档、发现的约束条件。

### 基于任务的路由

- 公共 API 或 NAPI 行为变更 → 阅读 `ecmascript/napi/AGENTS.md`
- 架构或模块边界变更 → 阅读 `docs/overview.md`
- GC、内存或分配变更 → 阅读 `docs/knowledge/ArkTS-GC.md`
- DFX、profiling、堆快照、栈追踪、tracing 变更 → 阅读 `docs/knowledge/DFX-Subsystem.md` + 子模块 AGENTS.md
- 调试、断点、dropframe、debugger hot reload 变更 → 阅读 `docs/knowledge/Debugger-Subsystem.md` + `ecmascript/debugger/AGENTS.md`
- HAP文件提取、SourceMap、ZIP解析变更 → 阅读 `docs/knowledge/ExtractorTool-Subsystem.md`
- Quick Fix、热重载、冷补丁、Patch 变更 → 阅读 `docs/knowledge/Patch-Subsystem.md`
- AOT 编译变更 → 阅读 `docs/aot-guide_zh.md` + `compiler_service/AGENTS.md`
- 模块化变更 -> 阅读 `docs/knowledge/ArkTS-Module.md`

### 基于路径的路由

- `ecmascript/napi/` → 运行时能力封装，为napi接口inner接口；变更签名前须检查兼容性
- `ecmascript/mem/` → 变更影响所有内存管理
- `ecmascript/compiler/` → AOT 代码生成；必须与 `compiler_service/` 保持同步
- `ecmascript/interpreter/` → 解释器相关修改，须进行性能回归测试
- `common_components/heap/` → 共享GC基础设施；同时影响 ets_runtime 和 runtime_core
- `ecmascript/module/` → 模块加载相关
- `ecmascript/dfx/` → 诊断/观测/性能分析子系统；先读 `docs/knowledge/DFX-Subsystem.md`
- `ecmascript/debugger/` → 调试器子系统；先读 `docs/knowledge/Debugger-Subsystem.md`
- `ecmascript/extractortool/` → 文件提取与SourceMap；先读 `docs/knowledge/ExtractorTool-Subsystem.md`
- `ecmascript/patch/` → Quick Fix/Hot Reload/Patch；先读 `docs/knowledge/Patch-Subsystem.md`
- `ecmascript/builtins/` -> ES 标准库相关

### 基于术语的路由

当任务、issue、日志、API 或变更文件涉及以下术语时，在规划前阅读对应文档：

| 术语 | 功能描述 | 阅读 |
|------|-----------|------|
| NAPI / JSNAPI | 提供napi C++ API inner接口层；签名变更会破坏下游 | `ecmascript/napi/AGENTS.md` |
| AOT / ark_aot_compiler | 机器码编译；与 compiler_service SA 协同工作 | `compiler_service/AGENTS.md` + `docs/aot-guide_zh.md` |
| GC / CMS-GC / Partial-Compressing-GC | 内存管理关键路径；修改需关注性能与稳定性风险 | `docs/knowledge/ArkTS-GC.md` |
| DFX / HiLog / heapsnapshot / CPU profiling / SIGPROF / tracing | 诊断/观测/性能分析；旁路路径不得影响主链路性能 | `docs/knowledge/DFX-Subsystem.md` |
| debugger / 调试器 / 断点 / dropframe / single step | 调试器子系统；BytecodePcChanged是热路径，非调试模式须零开销 | `docs/knowledge/Debugger-Subsystem.md` |
| interpreter / IC / 内联缓存 | 与解释器紧密耦合；性能敏感 | `ecmascript/ic/AGENTS.md` + `ecmascript/interpreter/AGENTS.md` |
| PGO | 配置文件引导优化；与 AOT 关联 | `ecmascript/` 下 PGO 子模块 AGENTS.md + `docs/aot-guide_zh.md` |
| TaskPool / Actor / Sendable| 并发模型；影响启动性能 | `docs/knowledge/ArkTS-Concurrency.md` | （待创建）
| .abc / 字节码 / jspandafile | 字节码格式；须保持向后兼容 | `ecmascript/jspandafile/AGENTS.md` |
| Stub / StubCompiler | 运行时桩代码；必须匹配解释器调用约定 | `ecmascript/stubs/AGENTS.md` |
| Module | 模块加载器 | `docs/knowledge/ArkTS-Module.md` |
| HAP / SourceMap / VLQ / Extractor / extractortool | 文件提取层，纯I/O层不得依赖JS运行时 | `docs/knowledge/ExtractorTool-Subsystem.md` |
| Quick Fix / Hot Reload / Cold Patch / patch / 热重载 / 补丁 | 运行时代码替换，方法不在调用栈上才能替换 | `docs/knowledge/Patch-Subsystem.md` |


## 3. 约束与边界

### 架构不变量

- 仅可执行由 ts2abc/es2abc 生成的 ArkCompiler 字节码（.abc 文件）。
- 仅支持 ES2021 strict 模式；不支持 sloppy 模式。
- 不支持动态代码执行（`eval`、`new Function()`）。
- NAPI 是唯一支持的 C++ 互操作路径。
- .abc 中的类型元数据须保持编译器工具链与运行时版本之间的向后兼容。
- GC 堆布局变更须保持与 runtime_core 的 ABI 兼容性。
- DFX 代码必须是旁路观测路径，采样/追踪开关关闭时须零开销或仅一次条件分支。
- Patch 方法替换必须在方法不在当前调用栈上时进行，`StageOfHotReload` 是全局状态，变更需检查所有读取点。

### 开发注意事项

- 修改 `TaggedObject` 派生类布局时，需注意不同数据类型在 32/64 位平台上的宽度差异；`JSTaggedValue` 或 GC 访问的 tagged 字段需按 `JSTaggedValue::TaggedTypeSize()` 对齐，`SIZE` 也应为该粒度的倍数，避免 `HClass` object size 截断或字段越界。

### 不要做

- 除非明确要求，不要修改公共 NAPI 签名、错误码或生命周期语义。
- 未经批准，不要引入新的第三方生产依赖。
- 不要修改自动生成的文件（如 builtins stubs）；应修改源码生成器后重新生成。
- 不要移除或变更错误码而不更新 `docs/`。

## 4. 编译构建

### 编译命令

```bash
# 构建（在 openharmony/ 目录下执行）
../../build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages

# 带调试符号构建
../../build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --gn-args is_debug=true
```
