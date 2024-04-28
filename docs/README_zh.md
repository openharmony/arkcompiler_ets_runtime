# 方舟运行时使用指南

## 方舟编译器简介

方舟编译器(ArkCompiler)是为支持多种编程语言、多种芯片平台的联合编译、运行而设计的统一编译运行时平台。

* ArkCompiler主要分成两个部分：编译工具链与运行时。
* ArkCompiler eTS Runtime主要由四个子系统组成：Core Subsystem、Execution Subsystem、Compiler Subsystem、Runtime subsystem。
* ArkCompiler 的设计特点：原生支持类型、并发模型优化与并发API、安全。

要了解虚拟机的详细设计请参考 [概述](overview-zh.md)

## 环境搭建和编译

### 环境配置

#### 搭建Ubuntu环境

- 初始环境软件安装（Ubuntu版本推荐18.04或20.04）

  ```
  sudo apt-get update
  sudo apt-get upgrade
  sudo apt-get install git-lfs git bison flex gnupg build-essential zip curl zlib1g-dev gcc-multilib g++-multilib libc6-dev-i386 lib32ncurses-dev x11proto-core-dev libx11-dev libc++1 lib32z1-dev ccache libgl1-mesa-dev libxml2-utils xsltproc unzip m4 libtinfo5 bc npm genext2fs liblz4-tool libssl-dev ruby openjdk-8-jre-headless gdb python3-pip libelf-dev libxcursor-dev libxrandr-dev libxinerama-dev
  ```

  详细Ubuntu安装搭建请参考 [搭建Ubuntu环境](https://developer.huawei.com/consumer/cn/training/course/video/C101639988048536240)

#### 源码获取

  下载源码请参考 [下载说明](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/get-code/sourcecode-acquire.md)

### 代码编译

    注意：下列命令需在源码根目录下执行

1. 首次编译：

   ```
   ./build.sh --product-name rk3568
   ```
2. 首次编译后增量编译方舟运行时：

   编译linux-x86版本：

   ```
   ./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages
   ```

   编译oh-arm64版本：

   ```
   ./build.sh --product-name rk3568 --gn-args use_musl=true --target-cpu arm64 --build-target ark_js_packages
   ```

   编译oh-arm32版本:

   ```
   ./build.sh --product-name rk3568 --build-target  ark_js_packages
   ```
3. 首次编译后增量编译方舟前端：

   ```
   ./build.sh --product-name rk3568 --build-target ets_frontend_build
   ```

   更多编译命令请参考 [编译命令](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/subsystems/subsys-arkcompiler-guide.md)

**说明**：上述编译命令为release版本，编译debug版本需增加编译选项：--gn-args is_debug=true。

方舟相关的二进制文件在如下路径：

```
out/rk3568/arkcompiler/runtime_core/
out/rk3568/arkcompiler/ets_frontend/
out/rk3568/arkcompiler/ets_runtime/
out/rk3568/clang_x64/arkcompiler/runtime_core/
out/rk3568/clang_x64/arkcompiler/ets_frontend/
out/rk3568/clang_x64/arkcompiler/ets_runtime
```

## 开发实例

本章节将介绍基于方舟运行时的开发测试实例。

### HelloWorld

#### 运行前准备

* 编译编译方舟运行时和方舟前端

  ```
  ./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --build-target ets_frontend_build
  ```

#### 运行hello-world.js

新建hello-world.js文件，写入以下源码：

```
print("Hello World!!!");
```

运行步骤：

1. 通过方舟前端生成hello-world.abc文件，编译命令：

   ```
   /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc hello-world.js
   ```

2. 执行hello-world.abc文件：

   1. 设置搜索路径：

      ```
      export LD_LIBRARY_PATH=/your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime:/your_code_path/prebuilts/clang/ohos/linux-x86_64/llvm/lib:/your_code_path/out/rk3568/clang_x64/thirdparty/zlib
      ```
   2. 执行ark\_js\_vm：

      ```
      /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime/ark_js_vm hello-world.abc
      ```

      执行结果如下：

      ```
      Hello World!!!
      ```

**说明**：此处“_your_code_path_”为源码目录路径。

#### 反汇编hello-world.abc

编译生成反汇编工具：

```
./build.sh --product-name rk3568 --build-target arkcompiler/runtime_core:ark_host_linux_tools_packages
```

执行如下命令，结果输出到output.pa文件中：

```
/your_code_path/out/rk3568/clang_x64/arkcompiler/runtime_core/ark_disasm hello-world.abc output.pa
```

hello-world.abc反汇编结果如下：

```
#
# source binary: hello-world.abc
#

# ====================
# LITERALS

# ====================
# RECORDS

.record _ESAnnotation <external>

.record _ESModuleMode {
	u8 isModule
}

# ====================
# METHODS

.function any func_main_0_any_any_any_any_(any a0, any a1, any a2) <static> {
	mov v2, a2
	mov v1, a1
	mov v0, a0
	builtin.acc
	sta v5
	builtin.idi "print", 0x0 // 加载print函数
	sta v3
	lda.str "Hello World!!!"  // 加载Hello World!!!字符串
	sta v4
	builtin.tern3 v3, v4  // 调用print函数
	builtin.acc
}
```

### 运行Test262测试用例

#### 运行前准备

1. 编译方舟运行时，编译命令：

```
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages
```

2. 编译方舟前端，编译命令：

```
./build.sh --product-name rk3568 --build-target ets_frontend_build
```

**说明**：编译命令执行路径为项目根目录。

#### 运行Test262

运行run\_test262.py脚本，下载及运行Test262用例。

命令行格式：

```
python3 test262/run_test262.py [options]
```

执行路径为：项目根目录/arkcompiler/ets_frontend。

<table><thead align="left"><tr id="row101462717303"><th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.1"><p id="p51552743010"><a name="p51552743010"></a><a name="p51552743010"></a>选项</p>
</th>
<th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.2"><p id="p11592710304"><a name="p11592710304"></a><a name="p11592710304"></a>描述</p>
</th>
</tr>
</thead>
<tbody><tr id="row2015172763014"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p171592710306"><a name="p171592710306"></a><a name="p171592710306"></a>--h，--help</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p13151527133011"><a name="p13151527133011"></a><a name="p13151527133011"></a>帮助提示</p>
</td>
</tr>
<tr id="row1015527173015"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1615182712308"><a name="p1615182712308"></a><a name="p1615182712308"></a>--dir  DIR</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p9556101593120"><a name="p9556101593120"></a><a name="p9556101593120"></a>选定要测试的目录</p>
</td>
</tr>
<tr id="row1015112763020"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1815182733012"><a name="p1815182733012"></a><a name="p1815182733012"></a>--file  FILE</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1615627173019"><a name="p1615627173019"></a><a name="p1615627173019"></a>选定要测试的文件</p>
</td>
</tr>
<tr id="row131515277307"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p111572716304"><a name="p111572716304"></a><a name="p111572716304"></a>--mode  [{1, 2, 3}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1655718513105"><a name="p1655718513105"></a><a name="p1655718513105"></a>模式选择，1：仅默认值；2：仅严格模式；3：默认模式和严格模式</p>
</td>
</tr>
<tr id="row1815112753020"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p2151927193015"><a name="p2151927193015"></a><a name="p2151927193015"></a>--es51</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1715312588115"><a name="p1715312588115"></a><a name="p1715312588115"></a>运行Test262 ES5.1版本</p>
</td>
</tr>
<tr id="row1915182703012"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p17151527133017"><a name="p17151527133017"></a><a name="p17151527133017"></a>--es2015  [{all, only}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p15761152983113"><a name="p15761152983113"></a><a name="p15761152983113"></a>运行Test262 ES2015版本，all：包含的所有用例；only：仅包括ES2015</p>
</td>
</tr>
<tr id="row10924204611109"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p18924846111013"><a name="p18924846111013"></a><a name="p18924846111013"></a>--esnext</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p15495042191410"><a name="p15495042191410"></a><a name="p15495042191410"></a>运行Test262-ES.next</p>
</td>
</tr>
<tr id="row5161145010105"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p716125071020"><a name="p716125071020"></a><a name="p716125071020"></a>--engine  FILE</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p121612050181014"><a name="p121612050181014"></a><a name="p121612050181014"></a>运行测试的其他引擎，指定二进制文件（如:d8,hermes,jsc,qjs...）</p>
</td>
</tr>
<tr id="row1325585931120"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p112561595112"><a name="p112561595112"></a><a name="p112561595112"></a>--babel</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p32561959111112"><a name="p32561959111112"></a><a name="p32561959111112"></a>是否使用Babel转换</p>
</td>
</tr>
<tr id="row95230818126"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p12523158191210"><a name="p12523158191210"></a><a name="p12523158191210"></a>--timeout  TIMEOUT</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p65233871210"><a name="p65233871210"></a><a name="p65233871210"></a>设置测试超时时间（以毫秒为单位）</p>
</td>
</tr>
<tr id="row474911612120"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1274912166123"><a name="p1274912166123"></a><a name="p1274912166123"></a>--threads  THREADS</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p4749121631210"><a name="p4749121631210"></a><a name="p4749121631210"></a>设置并行运行线程数</p>
</td>
</tr>
<tr id="row561512363122"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p26152036191218"><a name="p26152036191218"></a><a name="p26152036191218"></a>--hostArgs  HOSTARGS</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p156151636161215"><a name="p156151636161215"></a><a name="p156151636161215"></a>传递给eshost主机的命令行参数</p>
</td>
</tr>
<tr id="row77091648111210"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p18709164871213"><a name="p18709164871213"></a><a name="p18709164871213"></a>--ark-tool  ARK_TOOL</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p16709194812126"><a name="p16709194812126"></a><a name="p16709194812126"></a>方舟运行时的二进制工具</p>
</td>
</tr>
<tr id="row3767145231210"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p3767155201216"><a name="p3767155201216"></a><a name="p3767155201216"></a>--ark-frontend-tool  ARK_FRONTEND_TOOL</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p4767195251220"><a name="p4767195251220"></a><a name="p4767195251220"></a>方舟前端转换工具</p>
</td>
</tr>
<tr id="row753817001311"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p553870111318"><a name="p553870111318"></a><a name="p553870111318"></a>--libs-dir  LIBS_DIR</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p35384041313"><a name="p35384041313"></a><a name="p35384041313"></a>依赖so的路径集合，通过“:”分割</p>
</td>
</tr>
<tr id="row08504716135"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p11851747161314"><a name="p11851747161314"></a><a name="p11851747161314"></a>--ark-frontend  [{ts2panda, es2panda}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1085144712137"><a name="p1085144712137"></a><a name="p1085144712137"></a>指定前端</p>
</td>
</tr>
</tbody>
</table>

#### 测试运行示例

- 运行ES51测试用例：

  ```
   python3 test262/run_test262.py --es51
  ```
- 仅运行ES2015测试用例：

  ```
   python3 test262/run_test262.py --es2015
  ```
- 仅运行ES2021测试用例：

  ```
   python3 test262/run_test262.py --es2021 only

  ```
- 运行ES2015和ES51和ES2021所有测试用例：

  ```
   python3 test262/run_test262.py --es2021 all
  ```
- 运行单一测试用例：

  ```
   python3 test262/run_test262.py --file test262/data/test_es5/language/statements/break/12.8-1.js
  ```
- 运行某目录下所有测试用例：

  ```
   python3 test262/run_test262.py --dir test262/data/test_es5/language/statements
  ```
- 使用\`babel\`把单个测试用例转换成es5后再运行：

  ```
   python3 test262/run_test262.py  --babel --file test262/data/test_es5/language/statements/break/12.8-1.js
  ```

#### 测试输出

Test262所有用例的测试结果位于项目根目录/arkcompiler/ets_frontend/out下。shell中测试输出结果如下：

```
$python3 test262/run_test262.py --file test262/data/test_es2015/built-ins/Array/15.4.5.1-5-1.js

Wait a moment..........
Test command:
node
        test262/harness/bin/run.js
        --hostType=panda
        --hostPath=python3
        --hostArgs='-B test262/run_sunspider.py --ark-tool=/your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime/ark_js_vm --ark-frontend-tool=/your_code_path/out/rk3568/clang_x64/arkcompiler/ets_frontend/build/src/index.js --libs-dir=/your_code_path/out/rk3568/clang_x64/global/i18n:/your_code_path/prebuilts/clang/ohos/linux-x86_64/llvm/lib:/your_code_path/out/rk3568/clang_x64/thirdparty/zlib/ --ark-frontend=ts2panda'
        --threads=15
        --mode=only strict mode
        --timeout=60000
        --tempDir=build/test262
        --test262Dir=test262/data
        --saveCompiledTests
        test262/data/test_es5/language/statements/break/12.8-1.js

PASS test262/data/test_es2015/built-ins/Array/15.4.5.1-5-1.js (strict mode)
Ran 1 tests
1 passed
0 failed
used time is: 0:01:04.439642
```

### AOT执行

#### 运行前准备

* 编译生成AOT编译器：

  ```
  ./build.sh --product-name rk3568  --build-target ets_frontend_build --build-target ark_js_host_linux_tools_packages --build-target arkcompiler/runtime_core:ark_host_linux_tools_packages
  ```

* 新建hello-world.ts文件，写入以下源码：

  ```
  declare function print(arg:any):string;
  print('Hello World!!!')
  ```

#### 运行步骤

1. 通过方舟前端生成hello-world.abc文件，编译命令：

   ```
   /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc --module --merge-abc hello-world.ts
   ```
2. 设置搜索路径：

      ```
      export LD_LIBRARY_PATH=/your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime:/your_code_path/prebuilts/clang/ohos/linux-x86_64/llvm/lib:/your_code_path/out/rk3568/clang_x64/thirdparty/icu:/your_code_path/out/rk3568/clang_x64/thirdparty/zlib
      ```
3. 运行ark\_js\_vm，采集PGO信息，生成的PGO信息落盘在文件hello-world.ap：
      ```
      /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime/ark_js_vm --enable-pgo-profiler=true --compiler-pgo-profiler-path=hello-world.ap --entry-point=hello-world hello-world.abc
      ```
4. 通过AOT编译器生成an和ai文件，可以通过info日志观察到被编译的函数列表：

      ```
      /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime/ark_aot_compiler  --enable-pgo-profiler=true --compiler-pgo-profiler-path=hello-world.ap --log-level=info --aot-file=./hello-world hello-world.abc
      ```
5. 执行ark\_js\_vm：

      ```
      /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime/ark_js_vm --aot-file=./hello-world --entry-point=hello-world hello-world.abc
      ```

      执行结果如下：

      ```
      Hello World!!!
      ```

**说明**：此处“_your_code_path_”为源码目录路径。

## 工具链使用

方舟前端工具采用命令行交互方式，支持将ArkTS代码转换为方舟字节码，使其能够在方舟运行时上运行。支持Windows/Linux/Mac平台。

### ArkTS字节码编译工具概述

使用前端工具将ArkTS文件转换为方舟字节码文件。方舟前端工具在linux平台上可通过全量编译或指定编译前端工具链获取。

构建编译：

```
$ ./build.sh --product-name rk3568 --build-target ets_frontend_build
```

```
$ cd out/rk3568/clang_x64/arkcompiler/ets_frontend/
$ ./es2abc [options] file.js
```

<a name="table2035444615599"></a>

<table><thead align="left"><tr id="row535415467591"><th class="cellrowborder" valign="top" width="12.898710128987101%" id="mcps1.1.6.1.1"><p id="p13354134619595"><a name="p13354134619595"></a><a name="p13354134619595"></a>选项</p>
</th>
<th class="cellrowborder" valign="top" width="19.33806619338066%" id="mcps1.1.6.1.3"><p id="p157281281906"><a name="p157281281906"></a><a name="p157281281906"></a>描述</p>
</th>
<th class="cellrowborder" valign="top" width="25.82741725827417%" id="mcps1.1.6.1.4"><p id="p103276335016"><a name="p103276335016"></a><a name="p103276335016"></a>取值范围</p>
</th>
<th class="cellrowborder" valign="top" width="35.066493350664935%" id="mcps1.1.6.1.5"><p id="p1835494695915"><a name="p1835494695915"></a><a name="p1835494695915"></a>默认值</p>
</th>
</tr>
</thead>
<tbody><tr id="row1435412465598"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p881325510017"><a name="p881325510017"></a><a name="p881325510017"></a>--debug-info</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p072882813015"><a name="p072882813015"></a><a name="p072882813015"></a>携带debug信息</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p10327833305"><a name="p10327833305"></a><a name="p10327833305"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p076075115014"><a name="p076075115014"></a><a name="p076075115014"></a>-</p>
</td>
</tr>
<tr id="row1435412465598"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p881325510017"><a name="p881325510017"></a><a name="p881325510017"></a>--debugger-evaluate-expression</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p072882813015"><a name="p072882813015"></a><a name="p072882813015"></a>debugger下对输入的base64形式的表达式求值</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p10327833305"><a name="p10327833305"></a><a name="p10327833305"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p076075115014"><a name="p076075115014"></a><a name="p076075115014"></a>-</p>
</td>
</tr>
<tr id="row3355346105920"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p163552462595"><a name="p163552462595"></a><a name="p163552462595"></a>--dump-assembly</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p127284281905"><a name="p127284281905"></a><a name="p127284281905"></a>输出为汇编文件</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p93278335012"><a name="p93278335012"></a><a name="p93278335012"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p1976019511306"><a name="p1976019511306"></a><a name="p1976019511306"></a>-</p>
</td>
</tr>
<tr id="row9355174675912"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p6355104616592"><a name="p6355104616592"></a><a name="p6355104616592"></a>--dump-ast</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p187287280015"><a name="p187287280015"></a><a name="p187287280015"></a>打印解析得到的ast(抽象语法树)</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p932819331104"><a name="p932819331104"></a><a name="p932819331104"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p1475975114013"><a name="p1475975114013"></a><a name="p1475975114013"></a>-</p>
</td>
</tr>
<tr id="row53551046175917"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p13575501218"><a name="p13575501218"></a><a name="p13575501218"></a>--dump-debug-info</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p1372811281608"><a name="p1372811281608"></a><a name="p1372811281608"></a>打印debug信息</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p133287335020"><a name="p133287335020"></a><a name="p133287335020"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p37585513019"><a name="p37585513019"></a><a name="p37585513019"></a>-</p>
</td>
</tr>
<tr id="row8355204635911"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p657125010117"><a name="p657125010117"></a><a name="p657125010117"></a>--dump-literal-buffer</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p77281528704"><a name="p77281528704"></a><a name="p77281528704"></a>打印literal buffer内容</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p83281633208"><a name="p83281633208"></a><a name="p83281633208"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p17580511404"><a name="p17580511404"></a><a name="p17580511404"></a>-</p>
</td>
</tr>
<tr id="row6355124665910"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p105611505114"><a name="p105611505114"></a><a name="p105611505114"></a>--dump-size-stat</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p20728192819015"><a name="p20728192819015"></a><a name="p20728192819015"></a>显示字节码相关的统计信息</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p1332810331508"><a name="p1332810331508"></a><a name="p1332810331508"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p157577519014"><a name="p157577519014"></a><a name="p157577519014"></a>-</p>
</td>
</tr>
<tr id="row235584610599"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p95515501012"><a name="p95515501012"></a><a name="p95515501012"></a>--extension</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p37282028600"><a name="p37282028600"></a><a name="p37282028600"></a>指定输入类型</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p133281033804"><a name="p133281033804"></a><a name="p133281033804"></a>['js', 'ts', 'as']</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p675665112019"><a name="p675665112019"></a><a name="p675665112019"></a>-</p>
</td>
</tr>
<tr id="row135584635915"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p4551501217"><a name="p4551501217"></a><a name="p4551501217"></a>--help</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p157285282020"><a name="p157285282020"></a><a name="p157285282020"></a>帮助提示</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p1532819334016"><a name="p1532819334016"></a><a name="p1532819334016"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p475510516018"><a name="p475510516018"></a><a name="p475510516018"></a>-</p>
</td>
</tr>
<tr id="row133555461596"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p3541550416"><a name="p3541550416"></a><a name="p3541550416"></a>--module</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p27281728502"><a name="p27281728502"></a><a name="p27281728502"></a>按照ESM模式编译</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p832833312018"><a name="p832833312018"></a><a name="p832833312018"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p1975514517020"><a name="p1975514517020"></a><a name="p1975514517020"></a>-</p>
</td>
</tr>
<tr id="row23556463595"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p135313506120"><a name="p135313506120"></a><a name="p135313506120"></a>--opt-level</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p97284281607"><a name="p97284281607"></a><a name="p97284281607"></a>指定编译优化等级</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p43281335010"><a name="p43281335010"></a><a name="p43281335010"></a>['0', '1', '2']</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p57545511102"><a name="p57545511102"></a><a name="p57545511102"></a>0</p>
</td>
</tr>
<tr id="row5356124655916"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p185311501910"><a name="p185311501910"></a><a name="p185311501910"></a>--output</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p1872818281006"><a name="p1872818281006"></a><a name="p1872818281006"></a>
输出文件路径</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p73281733408"><a name="p73281733408"></a><a name="p73281733408"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p77537511606"><a name="p77537511606"></a><a name="p77537511606"></a>-</p>
</td>
</tr>
<tr id="row1335654635915"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p175213504115"><a name="p175213504115"></a><a name="p175213504115"></a>--parse-only</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p20729728003"><a name="p20729728003"></a><a name="p20729728003"></a>只对输入文件做解析动作</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p4328533205"><a name="p4328533205"></a><a name="p4328533205"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p175385118014"><a name="p175385118014"></a><a name="p175385118014"></a>-</p>
</td>
</tr>
<tr id="row1335654635915"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p175213504115"><a name="p175213504115"></a><a name="p175213504115"></a>--thread</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p20729728003"><a name="p20729728003"></a><a name="p20729728003"></a>指定生成字节码时所用的线程数目</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p4328533205"><a name="p4328533205"></a><a name="p4328533205"></a>0-机器支持的线程数目</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p175385118014"><a name="p175385118014"></a><a name="p175385118014"></a>0</p>
</td>
</tr>
</tbody>
</table>

### 反汇编器工具概述

工具名称为ark\_disasm，用于将二进制格式的方舟字节码文件转换为文本格式的方舟字节码文件。

编译生成反汇编工具：

```
./build.sh --product-name rk3568 --build-target arkcompiler/runtime_core:ark_host_linux_tools_packages
```

命令行格式：

```
ark_disasm [选项] 输入文件 输出文件
```

<table><thead align="left"><tr id="row125182553217"><th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.1"><p id="p175162514327"><a name="p175162514327"></a><a name="p175162514327"></a>选项</p>
</th>
<th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.2"><p id="p6512255324"><a name="p6512255324"></a><a name="p6512255324"></a>描述</p>
</th>
</tr>
</thead>
<tbody><tr id="row5511825103218"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p45172513326"><a name="p45172513326"></a><a name="p45172513326"></a>--debug</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1245695053215"><a name="p1245695053215"></a><a name="p1245695053215"></a>使能调试信息, 如果没有指定"--debug-file", 输出形式将会是标准输出。默认值是false</p>
</td>
</tr>
<tr id="row951112515321"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p451192515323"><a name="p451192515323"></a><a name="p451192515323"></a>--debug-file</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p175142583210"><a name="p175142583210"></a><a name="p175142583210"></a>调试信息输出文件路径，默认为std::cout</p>
</td>
</tr>
<tr id="row951112515321"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p451192515323"><a name="p451192515323"></a><a name="p451192515323"></a>--skip-string-literals</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p175142583210"><a name="p175142583210"></a><a name="p175142583210"></a>将字符串用对应的string_ID代替，可以减少输出文件的大小。默认值是false</p>
</td>
</tr>
<tr id="row951112515321"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p451192515323"><a name="p451192515323"></a><a name="p451192515323"></a>--quiet</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p175142583210"><a name="p175142583210"></a><a name="p175142583210"></a>打开所有--skip-*选项。默认值是false</p>
</td>
</tr>
<tr id="row45116253325"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p85116259328"><a name="p85116259328"></a><a name="p85116259328"></a>--help</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1348135833214"><a name="p1348135833214"></a><a name="p1348135833214"></a>帮助提示</p>
</td>
</tr>
<tr id="row194197407327"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p154205401325"><a name="p154205401325"></a><a name="p154205401325"></a>--verbose</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p369871173312"><a name="p369871173312"></a><a name="p369871173312"></a>输出更多关于类和方法在文件中的信息。默认值是false</p>
</td>
</tr>
</tbody>
</table>

输入文件：二进制格式的方舟字节码

输出文件：文本格式的方舟字节码

### AOT工具概述

AOT为Ahead Of Time的简称，也即提前编译，能够在Host端将字节码提前编译成Target端可运行的机器码，这样字节码可以获得充分编译优化，从而加速Target端的运行速度。

编译生成aot编译器：

```
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages
```

命令行格式：

```
ark_aot_compiler [选项] 输入文件
```

| 选项                          | 描述                                                                                |
| -----------------------       | ----------------------------------------------------------------------------------- |
| --aot-file                    | AOT输出文件的路径（不需要文件后缀）。默认值：“aot_file”                             |
| --compiler-opt-type-lowering  | 利用类型信息，生成优化级别更高的机器码。默认值：“true”                              |
| --compiler-opt-max-method     | 设置AOT编译方法的大小阈值，当方法的大小超过该值时，则不进行编译。默认值：“32KB”     |
| --compiler-opt-level          | 设置AOT的优化级别。默认值：“3”                                                      |
| --compiler-log                | AOT的日志选项，可打印出AOT生成的IR图、汇编码等信息。默认值“none”                    |
| --compiler-log-snapshot       | 打印与序列化有关的日志信息。默认值“false”                                           |
| --compiler-log-time           | 打印AOT过程中各个优化pass的耗时情况。默认值值“false”                                |


输入文件：二进制格式的方舟字节码

输出文件：直接执行的机器码的an文件、存储序列化后ConstPool的ai文件（输出文件路径需使用--aot-file选项指定）

### PGO工具概述

PGO为Profile-guided optimization的简称，也即配置文件引导的优化。该工具能够记录应用启动和性能场景中的高频（热点）函数，并将信息记录于对应的PGO Profiler文件。AOT编译器则可通过这些信息决策部分函数进行编译，从而在基本不影响应用运行性能的情况下，缩短编译时间，减少.an文件的大小。

编译生成aot编译器和ark js虚拟机

```
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages
```

### 生成PGO Profiler文件

命令行格式：

```
ark_js_vm [选项] 输入文件
```

| 选项                    | 描述                                                                                |
| ----------------------- | ----------------------------------------------------------------------------------- |
| --enable-pgo-profiler   | 开启pgo工具。默认值：“false”                                                    |
| --compiler-pgo-profiler-path     | pgo profiler文件的保存路径。默认值："none"                                     |
| --compiler-pgo-hotness-threshold | 热点函数的阈值，当函数的调用次数大于该值，则认为是热点函数。默认值：“2”             |

输入文件：二进制格式的方舟字节码

输出文件：保存有热点函数信息的ap文件

### 基于PGO Profiler的AOT编译

命令行格式：

```
ark_aot_compiler [选项] 输入文件
```
| 选项                    | 描述                                                                                |
| ----------------------- | ----------------------------------------------------------------------------------- |
| --compiler-pgo-profiler-path     | pgo profiler文件路径。默认值："none"                                     |
| --compiler-pgo-hotness-threshold | 使能pgo编译的函数调用次数阈值，profile文件中记录的调用次数大于该阈值的函数才会进行编译。默认值：“2”|

输入文件：二进制格式的方舟字节码，ap文件

输出文件：直接执行的机器码的an文件（基于pgo）
