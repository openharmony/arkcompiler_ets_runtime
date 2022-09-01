# ark_build
注意：目前版本部分内容还需完善，后续入库后会做具体说明

#### 介绍
方舟独立编译构建build

#### 软件架构和目录
独立编译目录结构

/arkcompiler/ 

├── ets_runtime 

    ├── build     
           
├── runtime_core   
          
├── ets_frontend         

├── third_patry         #从openharmony开源项目下载


#### 安装教程
1.  独立编译拉取arkcompiler_ets_runtime、arkcompiler_runtime_core、arkcompiler_ets_frontend三个仓（拉取时注意去掉前缀arkcompiler），执行[./ets_runtime/build/prebuilts_download/prebuilts_download.sh] 下载相关的编译所需工具。
2.  拉取三方库的四个仓到arkcompiler/third_party/xxx
3.  之后执行[./ets_runtime/build/compile_script/gen.sh ark]将.gn、.sh、ark.py文件拿出来。
4.  执行[./gen.sh]命令编译目前独立编译支持的所有目标，执行[./gen.sh abc]命令生成abc文件，执行[./gen.sh .]命令执行abc文件

#### 准备工作说明

1.  独立编译build目录放在ets_runtime仓下，它和build仓不一样，是独立编译特有的
2.  所需三方库从openharmony开源项目下载，包含 :
        https://gitee.com/openharmony/third_party_bounds_checking_function,
        https://gitee.com/openharmony/third_party_icu,
        https://gitee.com/openharmony/third_party_jsoncpp,
        https://gitee.com/openharmony/third_party_zlib

#### ark.py使用说明
ark.py独立编译入口工具概述:工具名称为ark.py，是一个方便的多合一脚本，集生成构建文件，触发构建以及部分测试运行于一体


命令行格式：

常用：
python ark.py [平台].[模式]

例如：
python ark.py x64.release

如要在构建后立刻运行262测试套：
python ark.py [平台].[模式] [-test262]

例如：
python ark.py x64.release -test262

使用清理模式，对原本已有构建产物清理并重新构建：
python ark.py [平台].[模式].[clean]

例如：
python ark.py x64.release.clean

使用自定义gn参数构建：
python ark.py [平台(可选)].[模式] [--args='xxx']

例如：
python ark.py x64.release --args='is_debug=false taget_cpu="arm64"'

编译特定目标：
python ark.py [平台].[模式].[目标]

例如：
python ark.py x64.release.ark_js_vm

一些可能需要的指令：
[-help]  获取简易的使用说明
[-v]     获取一般可用的编译各选项组合搭配

ark_js_vm使用方法：

将js文件转化为abc文件：
[./prebuilts/build-tools/common/nodejs/node-v12.18.4-linux-x64/bin/node --expose-gc [your output dir]/build/src/index.js xxx.js]
例如：
[./prebuilts/build-tools/common/nodejs/node-v12.18.4-linux-x64/bin/node --expose-gc out/x64.release/build/src/index.js test.js]

利用ark_js_vm执行生成的abc文件：
[LD_LIBRARY_PATH=[your output dir]:prebuilts/clang/ohos/linux-x86_64/llvm/lib ./[your output dir]/ark_js_vm xxx.abc]
例如:
[LD_LIBRARY_PATH=out/x64.release:prebuilts/clang/ohos/linux-x86_64/llvm/lib ./out/x64.release/ark_js_vm test.abc]

说明：此处“your output dir”为编译产物路径

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request
