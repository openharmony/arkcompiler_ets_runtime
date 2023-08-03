# Circuit Viewer可视化工具单元测试

## 概述
为了验证代码与设计相符合，跟踪需求和设计的实现，发现设计和需求中存在的错误。需要用单元测试检验被测代码的一个很小的、明确的功能是否正确。
本文主要介绍Circuit Viewer可视化工具单元测试使用指导。

## 目录

    arkcompiler_ets_runtime
        └──ecmascript
             └── dfx
                  └── circuit_viewer
				      ├── ...                   # 其他文件             
				      └── test                  # 工具单元测试用例
						  ├── figures           # 图片文件
						  ├── test_viewer.py    # 单元测试代码
						  ├── requirements.txt  # python依赖库
						  └── README.md         # 指导文档    

## 软件环境准备

依赖版本：python3.8

1.右键windows开始菜单，单击运行，输入cmd，单击确定。

2.在命令行中进入到circuit_viewer\test目录下，安装依赖库命令如下：

	pip install -r requirements.txt

3.查看谷歌浏览器版本，此处得到浏览器版本为101.0.4951.67，查询网址如下：

[查看浏览器版本](chrome://version/)

4.由于浏览器版本为101.0.4951.67，查看驱动版本列表中是否有101.0.4951.67版本，若有可下载此版本驱动；若没有可下载101.0.4951最小版本，此处选择101.0.4951.15版本。进入版本系统选择列表后，选择win32（win系统不区分win32或win64）下载压缩包，下载地址如下：

[驱动下载地址](http://npm.taobao.org/mirrors/chromedriver/)

5.将解压缩获得的驱动文件放入python根目录下：

## 使用说明

在命令行中进入到circuit_viewer\test目录下：

1.执行所有用例，命令为：

	pytest -s -W ignore test_viewer.py

2.执行单个用例,如test_start，命令为：

	pytest -s -W ignore test_viewer.py::test_start

## 相关仓

暂无
