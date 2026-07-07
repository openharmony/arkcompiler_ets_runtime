# ExtractorTool 子系统知识

本文只记录 `ecmascript/extractortool/` 全量组件：HAP/ZIP 文件提取、内存映射文件访问、Source Map 解析、路径工具。JSPandaFile 的 .abc 加载机制见 `ecmascript/jspandafile/`，模块管理器对 HAP 路径调度见 `ecmascript/module/`。已有子模块 AGENTS.md：`ecmascript/extractortool/AGENTS.md`。

## 概述

ExtractorTool 是**纯文件 I/O 层**，从 HAP（本质是 ZIP）中提取 `.abc` 字节码和其余资源，通过 `FileMapper` 提供零拷贝（mmap）或安全区域映射的字节码访问，通过 `SourceMap` 将压缩后的 JS 位置翻译回原始源码位置供调试器和堆栈追踪使用。最关键的架构边界：**ExtractorTool 不得依赖 JS 运行时状态（JSThread/EcmaVM/GC）**——只做文件操作和格式解析。

## 核心模型和主链路

本节同时承担路由：按"触发词或任务"定位组件，根据"优先入口"读取代码，遵守"不变式/维护点"。

| 触发词或任务 | 主链路/运行时模型 | 优先入口 | 不变式/维护点 |
|---|---|---|---|
| Extractor / HAP 提取 / hapPath | 封装 `ZipFile`，提供按文件名提取、文件列表、buffer 获取、HAP 模型检测（Stage/FA）。`ExtractorUtil` 提供按 hapPath 索引的线程安全缓存 | `ecmascript/extractortool/src/extractor.h` | **不要绕过 `ExtractorUtil::GetExtractor` 直接实例化**——会绕过缓存导致重复解析；`ExtractorUtil::extractorMap_` 无 eviction 上限（历史坑：动态加载大量 HAP + cache=true → OOM 累积） |
| ZipFile / ZIP 解析 / deflated / stored / Central Directory | 手动解析 `LocalHeader`/`CentralDirEntry`/`EndDir` 三大结构体，支持 stored（不压缩）和 deflated（压缩）两种方法。`ZipFileReader` 抽象层支持文件读取和内存读取两种模式 | `ecmascript/extractortool/src/zip_file.h` | `__attribute__((packed))` 不可移除——手动二进制解析依赖精确布局；仅支持压缩方法 0 和 8（Z_DEFLATED），其余方法（deflate64/BZip2/LZMA/Zstd）不支持 |
| FileMapper / mmap / 内存映射 | 3 种类型：`NORMAL_MEM`（堆分配）、`SHARED_MMAP`（共享 mmap）、`SAFE_ABC`（安全区域映射，仅用于 .abc 文件）| `ecmascript/extractortool/src/file_mapper.h` | **不要对 `SAFE_ABC` 类型调用 `delete`/`free`**——其内存由安全区域管理器控制；`SAFE_ABC` 外部唯一调用者：`ecmascript/dfx/stackinfo/js_stackinfo.cpp:1079` |
| SourceMap / source map / VLQ | VLQ 解码 + 位置翻译（generated→original），多 package source map 管理 | `ecmascript/extractortool/src/source_map.h` | `sourceMapDatas_` 缓存以 `string_view` 为 key——**必须确保底层数据在缓存生命周期内有效**（历史坑：dataPtr_ 释放后 string_view key 悬空导致 use-after-free） |
| FilePathUtils / NPM package / OHM URI / ohm | NPM 包查找（`FindNpmPackage` 等）、OHM URI 解析（`ParseOhmUri`/`NormalizeUri`）、bundle 路径拼接（`MakeFilePath`）| `ecmascript/extractortool/src/file_path_utils.h` | 路径分隔符使用 `Constants::FILE_SEPARATOR`，不要硬编码；当前**无独立单元测试**——通过 Extractor 间接覆盖 |
| constants.h / 设备路径 | `ABS_CODE_PATH`、`LOCAL_CODE_PATH`、`LOCAL_BUNDLES`、`COMPRESS_PROPERTY` 等路径常量 | `ecmascript/extractortool/src/constants.h` | 这些路径与 OpenHarmony 设备文件系统布局绑定——设备侧变更需同步；不得随意修改 |
| SAFE_ABC 生命周期 | `Extractor::GetSafeData(fileName)` → `ZipFile::CreateFileMapper(path, SAFE_ABC)`：未压缩时 mmap 映射（basePtr_），压缩时解压到堆 buffer（dataPtr_）。`FileMapper` 析构时：`NORMAL_MEM` delete[] dataPtr_，`SAFE_ABC`/`SHARED_MMAP` munmap basePtr_ | `extractor.cpp:146` → `zip_file.cpp:669` | 未压缩时 mmap 区域由 `ZipFileReader` 持有的 `fd_` 保证（`closable_ = false` 时 fd 不关闭）；FileMapper 析构前 mmap 区域必须保持有效 |

## 边界和分类

| 概念 | 本模块实现 | 不要混用 |
|---|---|---|
| ExtractorTool | 纯文件 I/O 层，只做 ZIP 解析和文件映射 | 不要依赖 JSThread/EcmaVM/GC——如果代码中出现了 `JSHandle`、`JSThread*`、GC 调用，立即拒绝 |
| HAP | OpenHarmony Application Package，本质是 ZIP 文件 | 不同于主流移动平台的安装包格式；可能是嵌套 ZIP（ZIP inside ZIP） |
| FileMapper::SAFE_ABC | 安全区域映射，专用于 .abc 文件 | 不是 `NORMAL_MEM`（堆分配，自行管理）；不是 `SHARED_MMAP`（共享 mmap） |
| SourceMap VLQ | Variable-Length Quantity 编码的位置映射 | 不是普通 base64 编码；不是 source map v2（只有 v3 支持） |
| Stage Model / FA Model | OpenHarmony 两种应用模型，HAP 内部目录结构不同 | 两个模型的路径布局不可互换；`IsStageModel` 检测逻辑不能假设默认值 |
| ExtractorUtil 缓存 | 按 hapPath 索引的 `shared_ptr<Extractor>` 缓存，受 `mapMutex_` 保护 | cache=true 时无容量上限/无 LRU/无 TTL——动态加载大量 HAP 需主动 `DeleteExtractor` |

## 约束规则

- **禁止**在 ExtractorTool 代码中创建 `JSHandle`、访问 `JSThread`、或触发 GC——这是纯 I/O 层
- **禁止**绕过 `ExtractorUtil::GetExtractor` 直接实例化 `Extractor`
- **禁止**在未持有 `mapMutex_` 的情况下访问 `extractorMap_`
- **禁止**对 `SAFE_ABC` 类型的 `FileMapper` 调用 `delete` 或 `free`
- **禁止**修改 ZIP 结构体的 `__attribute__((packed))`——原因：手动二进制解析依赖精确偏移
- **禁止**硬编码路径分隔符——使用 `Constants::FILE_SEPARATOR`
- **禁止**在 source map 的 `dataPtr_` 被释放后仍使用 `sourceMapDatas_` 中的 `string_view` key
- **禁止**假设 ZIP 文件一定是完整独立的——HAP 可能是嵌套 ZIP，`SetContentLocation` 设置了子 ZIP 的偏移和长度
- 如果打包工具链（`app_pack_tool`）切换压缩方法（如改用 deflate64/LZMA），`CheckCoherencyLocalHeader` 会拒绝——需提前适配

## 修改前检查

- 改 `extractor.*` 前，确认是否引入 JS 运行时依赖（必须拒绝）→ 是否涉及 `ExtractorUtil` 缓存（检查 `mapMutex_` 线程安全）
- 改 `zip_file.*` 前，确认 ZIP 二进制解析的 `__attribute__((packed))` 和偏移计算 → 是否新增压缩方法支持
- 改 `file_mapper.*` 前，确认 `SAFE_ABC` 的 mmap 区域生命周期管理
- 改 `source_map.*` 前，确认 VLQ 编解码正确性 → 增补 `SourceMapUnitTest` 用例
- 改 `constants.h` 前，确认设备侧对应路径存在 → Stage Model 和 FA Model 下都正确
- 改 `file_path_utils.*` 前，当前无独立测试——需手动验证 NPM/OHM URI 典型场景

## 代码和测试

### 代码锚点

| 功能 | 入口文件 |
|---|---|
| HAP 提取 | `ecmascript/extractortool/src/extractor.cpp` |
| ZIP 解析 | `ecmascript/extractortool/src/zip_file.cpp` |
| 文件映射 | `ecmascript/extractortool/src/file_mapper.cpp` |
| Source Map | `ecmascript/extractortool/src/source_map.cpp` |
| 路径工具 | `ecmascript/extractortool/src/file_path_utils.cpp` |
| 路径常量 | `ecmascript/extractortool/src/constants.h` |

### 测试锚点

| 变更类型 | 测试目标 |
|---|---|
| Extractor 提取/文件列表 | `ExtractorToolTest` — extractor_test |
| ZIP 解析 | `ExtractorToolTest` — zip_file_test |
| Source Map 映射 | `ExtractorToolTest` — source_map_test + `SourceMapUnitTest` |
| FileMapper 类型变更 | `ExtractorToolTest`（涉及 ZIP/FileMapper）+ SAFE_ABC 区域兼容确认 |
| 路径工具/constants.h | 无独立测试，需编译验证 + 手动确认 Stage/FA Model 路径 |

构建从 workspace root 执行：
```bash
cd /home/xiongkang/workspace/ark_standalone_build
ninja -C out/x64.release arkcompiler/ets_runtime:libark_jsruntime
ninja -C out/x64.release arkcompiler/ets_runtime:ark_unittest
```
