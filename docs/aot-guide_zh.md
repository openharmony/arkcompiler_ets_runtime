# AOT编译使用指南
- AOT(Ahead Of Time)即预先编译，在程序运行前，预先编译成高性能机器码，让程序在首次运行就能通过执行高性能机器码获得性能收益

- 方舟AOT编译器实现了PGO (Profile-Guided-Optimization）编译优化，即通过结合预先profiling的运行时类型等信息和静态类型信息，预先静态地生成高性能优化机器代码。

- 在方舟AOT编译器中，记录预先profiling运行时类型等信息的文件称为ap(ark-profile)文件。

- 对性能有高要求的开发者可以使用AOT编译提升应用运行性能。

- AOT编译可以通过下面几种方式进行
  1. 通过ARK工具链对ArkTS源码进行编译。详细指导参见：[AOT执行](https://gitee.com/openharmony/arkcompiler_ets_runtime/blob/master/docs/README_zh.md#aot%E6%89%A7%E8%A1%8C)
  2. 通过OpenHarmony提供的系统能力对应用进行编译。本文仅介绍该编译方式。

## 使用约束
- 当前仅支持Stage模型的ArkTS工程
- 目前仅HAP支持该功能
- 仅支持在64位的设备上运行

## 前置操作
- 打开ap采集开关（默认关闭。设备重启后，开关恢复默认状态）
  ```
  hdc shell param set ark.profile true
  ```


## 操作步骤
- **步骤一** &emsp;预热应用。运行应用，并进行对应的热点操作，系统会自动在设备侧生成ap文件。

- **步骤二** &emsp;触发AOT，生成机器码文件。
    - 当前支持以下几种方法触发AOT：
      - **方法一** &emsp;闲时编译：熄屏且充电状态下，会触发AOT编译。
      - **方法二** &emsp;快速触发：输入如下命令，待命令行返回后，表示编译完成。
        ```
        hdc shell bm compile -m partial {bundleName}
        ```
        - 说明：以*bundleName*为*com.example.myapplication*为例，输入以下命令可触发AOT编译
          ```
          hdc shell bm compile -m partial com.example.myapplication
          ```
    - 编译完成后，检查编译状态。只有该应用下所有module都编译成功，系统才认为该应用编译成功
      - 可通过以下命令查看该应用下每个module的编译状态（aotCompileStatus字段）。
      - ```hdc shell bm dump -n {bundleName}```
        - 说明：以*bundleName*为*com.example.myapplication*为例，输入以下命令可查看该应用下每个module的编译状态
          - ```hdc shell bm dump -n com.example.myapplication```
      - aotCompileStatus 状态说明：
      
        |状态值|意义| 常见场景 |
        |:--|:--|:--|
        | 0 | 尚未进行AOT编译 | 没有生成ap文件，或待进行AOT编译 |
        | 1 | AOT编译成功 | AOT编译成功 |
        | 2 | AOT编译失败 | AOT编译器内部错误 |

- **步骤三** &emsp;应用编译成功后，重启应用。重启后会加载优化的机器码文件。

## 附录
### AOT编译命令行详解
- 编译特定应用：
    ```
    bm compile -m partial {bundleName}
    ```
    - 说明：以*bundleName*为*com.example.myapplication*为例，输入以下命令可触发AOT编译
      - bm compile -m partial com.example.myapplication
- 编译所有已存在ap的应用：
    ```
    bm compile -m partial -a
    ```
- 清除AOT产物：
  - 针对特定应用:
    ```
    bm compile -r {bundleName}
    ```
    - 说明：以*bundleName*为*com.example.myapplication*为例，输入以下命令可清除AOT产物
      ```
      bm compile -r com.example.myapplication
      ```
  - 针对所有应用：
    ```
    bm compile -r -a
    ```