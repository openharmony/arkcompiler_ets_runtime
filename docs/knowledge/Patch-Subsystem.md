# Patch 子系统知识

本文只记录 `ecmascript/patch/` 全量热重载和冷补丁机制：方法替换、常量池更新、Quick Fix 生命周期管理。模块加载机制见 `ecmascript/module/`，Hot Reload Manager 见 `docs/knowledge/Debugger-Subsystem.md`，JSPandaFile 解析见 `ecmascript/jspandafile/`。已有子模块 AGENTS.md：`ecmascript/patch/AGENTS.md`。

## 概述

Patch 子系统允许在应用不重启的情况下，用补丁 .abc 文件中的 `MethodLiteral` 替换正在运行的方法，覆盖 callField、codeEntryPoint、nativePointer、constantPool。最关键的架构边界：**方法替换必须在方法不在当前调用栈上时进行**——替换正在执行的方法会导致崩溃。`StageOfHotReload` 是存储在 `JSThread` 上的全局状态，影响模块加载和编译器代码生成路径。

## 核心模型和主链路

本节同时承担路由：按"触发词或任务"定位组件，根据"优先入口"读取代码，遵守"不变式/维护点"。

### 方法替换链路

```
QuickFixManager::LoadPatch(thread, patchFileName, baseFileName)
  → PatchLoader::LoadPatchInternal(thread, baseFile, patchFile, patchInfo, baseClassInfo)
    → ① GeneratePatchInfo(patchFile)
        └─ 遍历 patch 文件的 MethodLiteral，构建 PatchMethodIndex → MethodLiteral* 映射
    → ② CollectClassInfo(patchFile)
        └─ 收集类的 recordName → className 映射
    → ③ FindAndReplaceSameMethod(thread, baseFile, patchFile, patchInfo, baseClassInfo)
        └─ FindSameMethod() 查找匹配 → SaveBaseMethodInfo() 保存原始数据 → ReplaceMethod()
            ├─ destMethod->SetCallField(src->GetCallField())
            ├─ destMethod->SetLiteralInfo(srcLiteralInfo)
            ├─ destMethod->SetCodeEntryPoint(srcCodeEntryPoint)
            ├─ destMethod->SetNativePointer(srcNativePointer)
            └─ constpool 替换
    → ④ ExecuteFuncOrPatchMain(thread, patchFile, patchInfo, loadPatch=true)
        └─ 切换 StageOfHotReload 并执行 patch 入口

QuickFixManager::UnloadPatch(thread, patchFileName)
  → PatchLoader::UnloadPatchInternal(thread, patchFileName, baseFileName, patchInfo)
    → ① 从 baseMethodInfo 恢复原始方法数据 → ② ClearPatchInfo 清理
```

**关键时序**：步骤③（ReplaceMethod）必须在步骤④（ExecuteFuncOrPatchMain）之前——先替换方法，再执行 patch 入口。

### 组件路由表

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| Quick Fix / patch / 补丁 / 方法替换 | `PatchLoader::ReplaceMethod` 执行 callField/literalInfo/codeEntryPoint/nativePointer/constpool 五项替换 | `ecmascript/patch/patch_loader.h` — `ReplaceMethod()` | **严格禁止在方法可能正在执行时替换**——必须先确保线程安全点；五项替换字段变更需全量验证 Hot Reload + Cold Reload 两个路径 |
| LoadPatch / 补丁加载 | `QuickFixManager::LoadPatch` → `LoadPatchInternal` → GeneratePatchInfo → FindAndReplaceSameMethod → ExecuteFuncOrPatchMain | `ecmascript/patch/quick_fix_manager.h` + `patch_loader.h` | **必须先在 `LoadPatch` 前调用 `RegisterQuickFixQueryFunc`**——否则 `callBack_` 为空，patch 文件查询失败（历史坑：静默失败无报错） |
| UnloadPatch / 补丁卸载 | `ClearPatchInfo` 清理 `patchMethodLiterals`、`baseMethodInfo`、`baseConstpools`、`replacedRecordNames` | `patch_loader.h` — `UnloadPatchInternal()` | **卸载后不要持有 PatchInfo 裸指针**——`ClearPatchInfo` 已清理，继续使用=use-after-free |
| StageOfHotReload / 热重载阶段 | 全局状态（存储在 `JSThread`），影响模块加载、变量获取、编译器（CircuitBuilder/StubBuilder）行为 | `ecmascript/js_thread.h` | **不能在状态机中间状态执行模块操作**；新增阶段需检查所有读取该状态的代码路径（module/、compiler/） |
| Cold Patch / 冷补丁 | 启动时替换方法，`StageOfColdReload` 标记 | `patch_loader.h` — `UpdateModuleForColdPatch()` | 与 Hot Reload 路径不同，需分别验证 |
| QuickFixHelper | 轻量工具：`CreateMainFuncWithPatch` — 为 patch 入口函数创建 JSFunction | `ecmascript/patch/quick_fix_helper.h` | 仅一个方法，边界清晰 |
| PatchInfo | 单个 patch 的完整元数据：`patchMethodLiterals`、`baseMethodInfo`、`baseConstpools`、`replacedRecordNames` | `patch_loader.h` | `baseMethodInfo` 保存原始数据用于卸载恢复——卸载时泄漏会导致内存持续增长；`baseConstpools` 用 **JSHandle** 保存（不能弱化为裸指针——GC 后野指针） |
| PatchMethodIndex / BaseMethodIndex | 两个关键标识符：前者用字符串三元组 (recordName, className, methodName)，后者用索引三元组 (constpoolNum, constpoolIndex, literalIndex) | `patch_loader.h` | 两者必须能相互映射；`FindSameMethod` 匹配可能返回 nullptr——不要假设一一对应 |

## 边界和分类

| 概念 | 本模块实现 | 不要混用 |
|---|---|---|
| Patch / Quick Fix | 运行时方法替换（Hot Reload + Cold Patch），替换 callField/codeEntryPoint 等 | 不是 Debugger Hot Reload Manager（在 `ecmascript/debugger/`）；不是 AOT 重编译 |
| PatchInfo | 单个 patch 文件的完整运行时元数据 | patch 卸载后立刻失效——不要缓存跨 Load/Unload 周期 |
| baseMethodInfo | 保存被替换方法的原始数据，用于卸载时恢复 | 卸载后必须恢复原值——否则方法永久损坏 |
| MethodLiteral | 方法元数据，补丁加载的核心替换单元 | 不要假设 base 和 patch 中的 MethodLiteral 一一对应 |
| ConstantPool / constpool | 常量池替换影响所有引用该池的方法 | 不要用裸指针保存——GC 管理对象必须在 handle scope 中保护 |

## 约束规则

- **禁止**在方法可能正在执行时调用 `ReplaceMethod()`——必须先确保线程安全点
- **禁止**在 `UnloadPatchInternal` 后仍持有 `PatchInfo` 中的裸指针
- **禁止**忘记在 `LoadPatch` 前调用 `RegisterQuickFixQueryFunc`——callBack_ 为空导致 Load 静默失败
- **禁止**假设 patch 文件和 base 文件中的方法一一对应——`FindSameMethod` 可能返回 nullptr
- **禁止**在 `StageOfHotReload` 中间状态下执行模块操作——状态值有特定语义
- **禁止**让 `methodInfos_` 无限累积——`LoadPatch` 追加，需 `UnloadPatch` 清理
- **禁止**在编译器代码中忽略 `StageOfHotReload`——CircuitBuilder/StubBuilder 在 Hot Reload 期间有不同代码生成路径
- **禁止**用裸指针保存 `baseConstpools`——必须用 `JSHandle` 防 GC 回收
- **必须**保证 `ReplaceMethod` 在 `ExecuteFuncOrPatchMain` 之前执行——先替换方法，再执行入口
- 当前设计假设单线程调用 `LoadPatch`——多线程场景需额外保护

## 修改前检查

- 改 `ReplaceMethod` 替换字段集合前，确认所有五个字段（callField/literalInfo/codeEntryPoint/nativePointer/constpool）的正确性
- 改 `StageOfHotReload` 状态机前，检查所有读取该状态的代码路径（`js_thread.h`、`module/`、`compiler/`）
- 改 `FindSameMethod` 匹配逻辑前，确认 base 和 patch 方法可能不一一对应
- 改 `PatchInfo` 内存管理前，确认 `ClearPatchInfo` 是否完整清理、卸载后是否仍有裸指针引用
- 涉及 Hot Reload 与 Debugger 交互前，确认交互时序（两者都操作 StageOfHotReload）
- 改 `PatchLoader` 前，确认是否影响 `QuickFixManager` 的 callback 注册和 `methodInfos_` 管理

## 代码和测试

### 代码锚点

| 功能 | 入口文件 |
|---|---|
| 补丁加载/卸载引擎 | `ecmascript/patch/patch_loader.cpp` |
| Quick Fix 生命周期 | `ecmascript/patch/quick_fix_manager.cpp` |
| Patch 入口函数创建 | `ecmascript/patch/quick_fix_helper.cpp` |
| StageOfHotReload 定义 | `ecmascript/js_thread.h` |
| 模块系统交互（HotReload 分支） | `ecmascript/module/js_module_source_text.cpp` |
| 编译器交互（StageOfHotReload 读取） | `ecmascript/compiler/`（CircuitBuilder/StubBuilder） |

### 测试锚点

| 变更类型 | 测试目标 |
|---|---|
| 方法替换逻辑（ReplaceMethod） | `ark_quickfix_test`（位于 `test/quickfix/`） |
| StageOfHotReload 状态机 | `ark_quickfix_test` + 模块/编译器相关测试 |
| Quick Fix API | `ark_quickfix_test` + NAPI 测试（`Jsnapi_001_Test` 等） |
| 跨模块影响（module/compiler） | 对应模块的测试目标 |

构建从 workspace root 执行。Patch 模块自身无 `tests/` 目录，测试位于项目顶层：
```bash
cd /home/xiongkang/workspace/ark_standalone_build
ninja -C out/x64.release arkcompiler/ets_runtime:libark_jsruntime
python ark.py x64.release ark_quickfix_test
```
