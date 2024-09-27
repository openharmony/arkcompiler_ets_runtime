# AOT编译使用指南
- AOT(Ahead Of Time)即预先编译，在程序运行前，预先编译成高性能机器码，让程序在首次运行就能通过执行高性能机器码获得性能收益

- 方舟AOT编译器实现了PGO (Profile-Guided-Optimization）编译优化，即通过结合预先profiling的运行时类型等信息和静态类型信息，预先静态地生成高性能优化机器代码。

- 在方舟AOT编译器中，记录预先profiling运行时类型等信息的文件称为ap(ark-profile)文件。

- 对性能有高要求的开发者可以使用AOT编译提升应用运行性能。

## 使用约束
- 当前仅支持Stage模型的ArkTS工程
- 仅支持在64位的手机设备上运行
- 仅支持Build Mode为release时使用，但暂不支持发布上架到应用市场时使用

## 前置操作
- 打开ap采集开关（默认关闭。设备重启后，开关恢复默认状态）
  ```
  hdc shell param set ark.profile true
  ```


## 操作步骤
- **步骤一** &emsp;预热应用。运行应用，并进行对应的热点操作，系统会自动在设备侧生成ap文件。
    - 说明：当采集阶段执行到的js函数个数少于一定阈值时，无法生成AP文件。
- **步骤二** &emsp;使用hdc shell命令出发AOT编译，生成机器码文件
    - 输入如下命令，待命令返回后，表示编译完成
        ```
        hdc shell bm compile -m partial {bundleName}
        ```
        - 说明：以*bundleName*为*com.example.myapplication*为例，输入以下命令可触发AOT编译
          ```
          hdc shell bm compile -m partial com.example.myapplication
          ```
        - 说明：编译任务受整机资源状态管控，为提高编译成功率，请在设备熄屏状态下进行编译，并避免设备发烫情况下开启编译。
- **步骤三** &emsp;编译完成后，检查编译状态。只有该应用下所有module都编译成功，系统才认为该应用编译成功
    - 执行以下命令，通过aotCompileStatus字段查看该应用下每个module的编译状态
      - ```hdc shell bm dump -n {bundleName}```
        - 说明：以*bundleName*为*com.example.myapplication*为例，输入以下命令可查看该应用下每个module的编译状态
          - ```hdc shell bm dump -n com.example.myapplication```
      - aotCompileStatus 状态说明：
        |状态值|意义| 常见场景 |
        |:--|:--|:--|
        | 0 | 尚未进行AOT编译 | 没有生成ap文件，或待进行AOT编译 |
        | 1 | AOT编译成功 | AOT编译成功 |
        | 2 | AOT编译失败 | AOT编译器内部错误 |

- **步骤四** &emsp;应用编译成功后，重启应用。重启后会加载优化的机器码文件。
- **步骤五** &emsp;如不需要AOT编译模式，执行以下命令清除特定应用的AOT产物。
      ```
      hdc shell bm compile -r {bundleName}
      ```
    - 说明：使用过程中如出现应用闪退，界面不一致，手机功耗异常增加等不稳定情况，可按照本步骤清除AOT产物
