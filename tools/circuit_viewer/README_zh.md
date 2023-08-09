# Circuit IR可视化工具

## 简介

本文主要介绍Circuit IR可视化工具，它可以根据ark_aot_compiler生成的log文件实现可视化分析。目前工具支持web入口。

## 目录

    arkcompiler_ets_runtime
        └──ecmascript
             └── dfx
                  └── circuit_viewer
                        ├── dist           # 工具发布目录
                        ├── docs           # 工具说明文档
                        ├── examples       # 工具需要的文件样例
                        ├── figures        # 图片文件
                        ├── src            # Circuit IR可视化工具源码，用于界面的绘制呈现等处理
                        ├── test           # 单元测试
                        ├── OWNER          # 所有者说明
                        └── README_zh.md   # 工具使用指导文档

## 使用对象

优化Circuit IR的开发者

## 使用场景

需对Circuit IR log文件分析，窗口中展示可视化流图对应log文件内容。

## 工具使用

具体的工具使用步骤，可以左键单击以下链接了解：

[工具使用说明](./docs/INSTRUCTION_zh.md)

### 工具输出

1. 根据使用者指定的Circuit IR log文件，工具会输出对应的可视化流图。可供分析的Circuit IR log文件存放在以下路径：

		```
		arkcompiler_ets_runtime/ecmascript/dfx/circuit_viewer/examples
		```
	
在window环境下，根据输入文件生成可视化窗口，如下图所示：

 ![](./figures/example.png)

## 工具开发说明

### 对象

工具的开发者

### 开发场景

若当前工具的功能已经不能满足开发者的全部需求，则开发者可以基于已有的源码对工具进行二次开发，来增强工具的能力，编译打包生成自定义的工具。

### 开发步骤

[工具开发说明](./docs/DEVELOP_zh.md)

## 版本说明

 暂无

## FAQ

## 参与贡献

暂无

## 相关仓

暂无
