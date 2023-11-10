# 方舟eTS运行时部件<a name="ZH-CN_TOPIC_0000001183610495"></a>

- [方舟eTS运行时部件<a name="ZH-CN_TOPIC_0000001183610495"></a>](#方舟eTS运行时部件)
    - [简介<a name="section190813718209"></a>](#简介)
  - [目录<a name="section161941989596"></a>](#目录)
  - [约束<a name="section119744591305"></a>](#约束)
  - [编译构建<a name="section137768191623"></a>](#编译构建)
    - [接口说明<a name="section175841548124517"></a>](#接口说明)
    - [使用说明<a name="section129654513264"></a>](#使用说明)
  - [相关仓<a name="section1371113476307"></a>](#相关仓)

### 简介<a name="section190813718209"></a>

方舟eTS运行时是OpenHarmony上默认的ArkTS语言运行时。支持Ecmascript规范定义的标准库和高效container容器库，提供完备的C++交互ArkTS NAPI和各种高性能的垃圾回收器，驱动着万物互联时代的OpenHarmony应用程序。

更多信息请参考：[方舟运行时子系统](https://gitee.com/openharmony/docs/blob/master/zh-cn/readme/ARK-Runtime-Subsystem-zh.md)

**图1** 方舟eTS运行时部件架构图：

![](/docs/figures/zh-cn_image_ark-ts-arch.png)

## 目录<a name="section161941989596"></a>

```
/arkcompiler/ets_runtime
├─ ecmascript             # 方舟ArkTS运行时实现，包括ECMAScript标准库、解释器、内存管理等
│   ├─ base               # 基础帮助类
│   ├─ builtins           # ECMAScript标准库
│   ├─ compiler           # 编译器
│   ├─ containers         # 非ECMAScript标准容器类库
│   ├─ debugger           # 调试器
│   ├─ dfx                # 内存与性能分析工具
│   ├─ ic                 # 内联缓存模块
│   ├─ interpreter        # 解释器
│   ├─ jobs               # 微任务队列
│   ├─ js_api             # 非ECMA标准对象模型
│   ├─ js_vm              # 命令行工具
│   ├─ jspandafile        # abc文件管理模块
│   ├─ mem                # 内存管理模块
│   ├─ module             # ECMAScript module模块
│   ├─ napi               # C++接口模块
│   ├─ quick_fix          # 快速修复命令行工具
│   ├─ regexp             # 正则引擎模块
│   ├─ require            # CommonJS规范module模块
│   ├─ shared_mm          # 共享内存管理模块
│   ├─ snapshot           # 快照模块
│   ├─ stubs              # runtime桩函数
│   ├─ taskpool           # 任务池
│   ├─ tests              # 单元测试用例
│   ├─ ts_types           # TS类型管理模块
└─ test                   # 模块测试用例
```

## 约束<a name="section119744591305"></a>

* 仅支持运行方舟eTS编译器\(ts2abc或es2abc\)生成的方舟字节码文件
* 只支持ES2021标准和严格模式（use strict)
* 不支持通过字符串动态创建函数(比如new Function("console.log(1);"))

## 编译构建<a name="section137768191623"></a>

```
$./build.sh --product-name hispark_taurus_standard --build-target ark_js_host_linux_tools_packages
```

### 接口说明<a name="section175841548124517"></a>

NAPI接口说明参考[NAPI部件](https://gitee.com/openharmony/arkui_napi/blob/master/README_zh.md)

### 使用说明<a name="section129654513264"></a>

ArkTS生成字节码参考[方舟eTS编译器]( https://gitee.com/openharmony/arkcompiler_ets_frontend/blob/master/README_zh.md#%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E)

字节码执行：
```
LD_LIBRARY_PATH=out/hispark_taurus/clang_x64/arkcompiler/ets_runtime:out/hispark_taurus/clang_x64/thirdparty/icu:prebuilts/clang/ohos/linux-x86_64/llvm/lib ./out/hispark_taurus/clang_x64/arkcompiler/ets_runtime/ark_js_vm helloworld.abc

```

更多使用说明请参考：[方舟运行时使用指南](/docs/README_zh.md)

## 相关仓<a name="section1371113476307"></a>

[arkcompiler\_runtime\_core](https://gitee.com/openharmony/arkcompiler_runtime_core)

**[arkcompiler\_ets\_runtime](https://gitee.com/openharmony/arkcompiler_ets_runtime)**

[arkcompiler\_ets\_frontend](https://gitee.com/openharmony/arkcompiler_ets_frontend)