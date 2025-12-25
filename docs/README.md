# ArkCompiler Runtime Usage Guide

## Introduction

ArkCompiler is a unified compilation and runtime platform that supports joint compilation and running across programming languages and chip platforms.

* ArkCompiler consists of two parts: compiler toolchain and runtime.
* ArkCompiler eTS Runtime consists of four subsystems: Core Subsystem, Execution Subsystem, Compiler Subsystem, and Runtime Subsystem.
* ArkCompiler features: support for native types, concurrency model optimization, concurrency API, and security.

For details about the VM design, see [Overview](overview.md).

## Environment Setup and Compilation

### Environment Configuration

#### Setting Up the Ubuntu Environment

- Installing initial environment software (Ubuntu 18.04 or 20.04 is recommended)

  ```
  sudo apt-get update
  sudo apt-get upgrade
  sudo apt-get install git-lfs git bison flex gnupg build-essential zip curl zlib1g-dev gcc-multilib g++-multilib libc6-dev-i386 lib32ncurses-dev x11proto-core-dev libx11-dev libc++1 lib32z1-dev ccache libgl1-mesa-dev libxml2-utils xsltproc unzip m4 libtinfo5 bc npm genext2fs liblz4-tool libssl-dev ruby openjdk-8-jre-headless gdb python3-pip libelf-dev libxcursor-dev libxrandr-dev libxinerama-dev
  ```

#### Source Code Acquisition

  For details about how to download the source code, see [Obtaining Source Code](https://gitee.com/openharmony/docs/blob/master/en/device-dev/get-code/sourcecode-acquire.md).

### Code Compilation

    Note: The following commands must be executed in the root directory of the source code.

1. First compilation:

   ```
   ./build.sh --product-name rk3568
   ```
2. Compile an Ark eTS runtime after the first compilation.

   Compile for the linux-x86 architecture.

   ```
   ./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages
   ```

   Compile for the oh-arm64 architecture.

   ```
   ./build.sh --product-name rk3568 --gn-args use_musl=true --target-cpu arm64 --build-target ark_js_packages
   ```

   Compile for the oh-arm32 architecture.

   ```
   ./build.sh --product-name rk3568 --build-target  ark_js_packages
   ```
3. Compile the Ark eTS frontend after the first compilation.

   ```
   ./build.sh --product-name rk3568 --build-target ets_frontend_build
   ```

   For more compilation commands, see [ArkCompiler Development](https://gitee.com/openharmony/docs/blob/master/en/device-dev/subsystems/subsys-arkcompiler-guide.md).

Note: The preceding compilation commands are for the release version. To compile the debug version, add the compilation option **--gn-args is_debug=true**.

The binary files related to ARK are available in the following paths:

```
out/rk3568/arkcompiler/runtime_core/
out/rk3568/arkcompiler/ets_frontend/
out/rk3568/arkcompiler/ets_runtime/
out/rk3568/clang_x64/arkcompiler/runtime_core/
out/rk3568/clang_x64/arkcompiler/ets_frontend/
out/rk3568/clang_x64/arkcompiler/ets_runtime
```

## Development Example

The following describes how to develop and test Ark eTS runtime.

### HelloWorld

#### Preparations

* Compile the Ark eTS runtime and Ark eTS frontend.

  ```
  ./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --build-target ets_frontend_build
  ```

#### Running **hello-world.js**

Create the **hello-world.js** file and write the following source code into the file:

```
print("Hello World!!!");
```

Procedure:

1. Use the Ark eTS frontend to create the **hello-world.abc** file.

   ```
   /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc hello-world.js
   ```

2. Run the **hello-world.abc** file.

   1. Set the search path.

      ```
      export LD_LIBRARY_PATH=/your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime:/your_code_path/prebuilts/clang/ohos/linux-x86_64/llvm/lib:/your_code_path/out/rk3568/clang_x64/thirdparty/zlib
      ```
   2. Run **ark\_js\_vm**.

      ```
      /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime/ark_js_vm hello-world.abc
      ```

      The execution result is as follows:

      ```
      Hello World!!!
      ```

**Note**: **_your_code_path_** indicates the source code directory.

#### Disassembling **hello-world.abc**

Compile and generate the disassembler tool.

```
./build.sh --product-name rk3568 --build-target arkcompiler/runtime_core:ark_host_linux_tools_packages
```

Run the following command to export the result to the **output.pa** file:

```
/your_code_path/out/rk3568/clang_x64/arkcompiler/runtime_core/ark_disasm hello-world.abc output.pa
```

The output is as follows:

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
	builtin.idi "print", 0x0 // Load the print function.
	sta v3
	lda.str "Hello World!!!"  // Load the Hello World!!! string.
	sta v4
	builtin.tern3 v3, v4  // Call the print function.
	builtin.acc
}
```

### Performing Test Case Test262

#### Preparations

1. Run the following command to compile the Ark eTS runtime:

```
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages
```

2. Run the following command to compile the Ark eTS frontend:

```
./build.sh --product-name rk3568 --build-target ets_frontend_build
```

**Note**: The compilation command is executed in the project root directory.

#### Running Test262

Run the run\_test262.py script to download and run the Test262 test case.

Command:

```
python3 test262/run_test262.py [options]
```

The execution path is **project root directory/arkcompiler/ets_frontend**.

<table><thead align="left"><tr id="row101462717303"><th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.1"><p id="p51552743010"><a name="p51552743010"></a><a name="p51552743010"></a>Option</p>
</th>
<th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.2"><p id="p11592710304"><a name="p11592710304"></a><a name="p11592710304"></a>Description</p>
</th>
</tr>
</thead>
<tbody><tr id="row2015172763014"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p171592710306"><a name="p171592710306"></a><a name="p171592710306"></a>--h, --help</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p13151527133011"><a name="p13151527133011"></a><a name="p13151527133011"></a>Displays help information.</p>
</td>
</tr>
<tr id="row1015527173015"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1615182712308"><a name="p1615182712308"></a><a name="p1615182712308"></a>--dir  DIR</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p9556101593120"><a name="p9556101593120"></a><a name="p9556101593120"></a>Specifies the directory to be tested.</p>
</td>
</tr>
<tr id="row1015112763020"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1815182733012"><a name="p1815182733012"></a><a name="p1815182733012"></a>--file  FILE</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1615627173019"><a name="p1615627173019"></a><a name="p1615627173019"></a>Specifies the file to be tested.</p>
</td>
</tr>
<tr id="row131515277307"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p111572716304"><a name="p111572716304"></a><a name="p111572716304"></a>--mode  [{1, 2, 3}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1655718513105"><a name="p1655718513105"></a><a name="p1655718513105"></a>Specifies the mode. **1**: Default mode only; **2**: Strict mode only; **3**: Default mode and strict mode.</p>
</td>
</tr>
<tr id="row1815112753020"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p2151927193015"><a name="p2151927193015"></a><a name="p2151927193015"></a>--es51</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1715312588115"><a name="p1715312588115"></a><a name="p1715312588115"></a>Runs the Test262 ES5.1 version.</p>
</td>
</tr>
<tr id="row1915182703012"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p17151527133017"><a name="p17151527133017"></a><a name="p17151527133017"></a>--es2015  [{all, only}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p15761152983113"><a name="p15761152983113"></a><a name="p15761152983113"></a>Runs the Test262 ES2015 version. **all**: includes all test cases. **only**: includes only ES2015.</p>
</td>
</tr>
<tr id="row10924204611109"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p18924846111013"><a name="p18924846111013"></a><a name="p18924846111013"></a>--esnext</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p15495042191410"><a name="p15495042191410"></a><a name="p15495042191410"></a>Runs the Test262-ES.next version.</p>
</td>
</tr>
<tr id="row5161145010105"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p716125071020"><a name="p716125071020"></a><a name="p716125071020"></a>--engine  FILE</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p121612050181014"><a name="p121612050181014"></a><a name="p121612050181014"></a>Runs other engines for testing. You can specify the binary file (such as d8, hermes, jsc, and qjs).</p>
</td>
</tr>
<tr id="row1325585931120"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p112561595112"><a name="p112561595112"></a><a name="p112561595112"></a>--babel</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p32561959111112"><a name="p32561959111112"></a><a name="p32561959111112"></a>Sets whether to enable Babel conversion.</p>
</td>
</tr>
<tr id="row95230818126"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p12523158191210"><a name="p12523158191210"></a><a name="p12523158191210"></a>--timeout  TIMEOUT</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p65233871210"><a name="p65233871210"></a><a name="p65233871210"></a>Sets the test timeout time (in millisecond).</p>
</td>
</tr>
<tr id="row474911612120"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1274912166123"><a name="p1274912166123"></a><a name="p1274912166123"></a>--threads  THREADS</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p4749121631210"><a name="p4749121631210"></a><a name="p4749121631210"></a>Sets the number of concurrent running threads.</p>
</td>
</tr>
<tr id="row561512363122"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p26152036191218"><a name="p26152036191218"></a><a name="p26152036191218"></a>--hostArgs  HOSTARGS</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p156151636161215"><a name="p156151636161215"></a><a name="p156151636161215"></a>Passes command parameters to eHost.</p>
</td>
</tr>
<tr id="row77091648111210"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p18709164871213"><a name="p18709164871213"></a><a name="p18709164871213"></a>--ark-tool  ARK_TOOL</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p16709194812126"><a name="p16709194812126"></a><a name="p16709194812126"></a>Specifies the binary tool of Ark eTS runtime.</p>
</td>
</tr>
<tr id="row3767145231210"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p3767155201216"><a name="p3767155201216"></a><a name="p3767155201216"></a>--ark-frontend-tool  ARK_FRONTEND_TOOL</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p4767195251220"><a name="p4767195251220"></a><a name="p4767195251220"></a>Specifies the conversion tool of Ark eTS frontend.</p>
</td>
</tr>
<tr id="row753817001311"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p553870111318"><a name="p553870111318"></a><a name="p553870111318"></a>--libs-dir  LIBS_DIR</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p35384041313"><a name="p35384041313"></a><a name="p35384041313"></a>Specifies the path set of the dependency .so files. Use colons to separate multiple paths.</p>
</td>
</tr>
<tr id="row08504716135"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p11851747161314"><a name="p11851747161314"></a><a name="p11851747161314"></a>--ark-frontend  [{ts2panda, es2panda}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1085144712137"><a name="p1085144712137"></a><a name="p1085144712137"></a>Specifies the frontend.</p>
</td>
</tr>
</tbody>
</table>

#### Example

- Run test case ES51.

  ```
   python3 test262/run_test262.py --es51
  ```
- Run test case ES2015 only.

  ```
   python3 test262/run_test262.py --es2015
  ```
- Run test case ES2021 only.

  ```
   python3 test262/run_test262.py --es2021 only

  ```
- Run all ES2015, ES51, and ES2021 test cases.

  ```
   python3 test262/run_test262.py --es2021 all
  ```
- Run a test case.

  ```
   python3 test262/run_test262.py --file test262/data/test_es5/language/statements/break/12.8-1.js
  ```
- Run all test cases in a directory.

  ```
   python3 test262/run_test262.py --dir test262/data/test_es5/language/statements
  ```
- Use \`Babel\` to convert a test case into ES5 and then run the test case.

  ```
   python3 test262/run_test262.py  --babel --file test262/data/test_es5/language/statements/break/12.8-1.js
  ```

#### Test Output

The results of all Test262 test cases are available in the ***Project root directory*/arkcompiler/ets_frontend/out**. The test result in the shell is as follows:

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

### AOT Execution

#### Preparations

* Compile and generate the AOT compiler.

  ```
  ./build.sh --product-name rk3568  --build-target ets_frontend_build --build-target ark_js_host_linux_tools_packages --build-target arkcompiler/runtime_core:ark_host_linux_tools_packages
  ```

* Create the **hello-world.ts** file and write the following source code into the file:

  ```
  declare function print(arg:any):string;
  print('Hello World!!!')
  ```

#### Procedure

1. Use the Ark eTS frontend to create the **hello-world.abc** file.

   ```
   /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_frontend/es2abc --module --merge-abc hello-world.ts
   ```
2. Set the search path.

      ```
      export LD_LIBRARY_PATH=/your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime:/your_code_path/prebuilts/clang/ohos/linux-x86_64/llvm/lib:/your_code_path/out/rk3568/clang_x64/thirdparty/icu:/your_code_path/out/rk3568/clang_x64/thirdparty/zlib
      ```
3. Run **ark\_js\_vm** to collect PGO information. The generated PGO information is stored in the **hello-world.ap** file.
      ```
      /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime/ark_js_vm --enable-pgo-profiler=true --compiler-pgo-profiler-path=hello-world.ap --entry-point=hello-world hello-world.abc
      ```
4. Generate .an and .ai files using the AOT compiler. You can view the list of compiled functions in the info log.

      ```
      /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime/ark_aot_compiler  --enable-pgo-profiler=true --compiler-pgo-profiler-path=hello-world.ap --log-level=info --aot-file=./hello-world hello-world.abc
      ```
5. Run **ark\_js\_vm**.

      ```
      /your_code_path/out/rk3568/clang_x64/arkcompiler/ets_runtime/ark_js_vm --aot-file=./hello-world --entry-point=hello-world hello-world.abc
      ```

      The execution result is as follows:

      ```
      Hello World!!!
      ```

**Note**: **_your_code_path_** indicates the source code directory.

## Using the Toolchain

The Ark eTS frontend tools use the command line interaction mode and convert ArkTS code into Ark bytecode that can run on Ark eTS runtime. The toolchain supports Windows, Linux, and macOS.

### ArkTS Bytecode Compiler

Use the frontend tool to convert ArkTS files into Ark bytecode files. On Linux, you can obtain the Ark eTS frontend toolchain through full compilation or by specifying specific compilation.

Build commands:

```
$ ./build.sh --product-name rk3568 --build-target ets_frontend_build
```

```
$ cd out/rk3568/clang_x64/arkcompiler/ets_frontend/
$ ./es2abc [options] file.js
```

<a name="table2035444615599"></a>

<table><thead align="left"><tr id="row535415467591"><th class="cellrowborder" valign="top" width="12.898710128987101%" id="mcps1.1.6.1.1"><p id="p13354134619595"><a name="p13354134619595"></a><a name="p13354134619595"></a>Option</p>
</th>
<th class="cellrowborder" valign="top" width="19.33806619338066%" id="mcps1.1.6.1.3"><p id="p157281281906"><a name="p157281281906"></a><a name="p157281281906"></a>Description</p>
</th>
<th class="cellrowborder" valign="top" width="25.82741725827417%" id="mcps1.1.6.1.4"><p id="p103276335016"><a name="p103276335016"></a><a name="p103276335016"></a>Value Range</p>
</th>
<th class="cellrowborder" valign="top" width="35.066493350664935%" id="mcps1.1.6.1.5"><p id="p1835494695915"><a name="p1835494695915"></a><a name="p1835494695915"></a>Default Value</p>
</th>
</tr>
</thead>
<tbody><tr id="row1435412465598"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p881325510017"><a name="p881325510017"></a><a name="p881325510017"></a>--debug-info</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p072882813015"><a name="p072882813015"></a><a name="p072882813015"></a>Includes debug information.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p10327833305"><a name="p10327833305"></a><a name="p10327833305"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p076075115014"><a name="p076075115014"></a><a name="p076075115014"></a>-</p>
</td>
</tr>
<tr id="row1435412465598"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p881325510017"><a name="p881325510017"></a><a name="p881325510017"></a>--debugger-evaluate-expression</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p072882813015"><a name="p072882813015"></a><a name="p072882813015"></a>Evaluates base64-encoded expressions in debugger. </p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p10327833305"><a name="p10327833305"></a><a name="p10327833305"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p076075115014"><a name="p076075115014"></a><a name="p076075115014"></a>-</p>
</td>
</tr>
<tr id="row3355346105920"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p163552462595"><a name="p163552462595"></a><a name="p163552462595"></a>--dump-assembly</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p127284281905"><a name="p127284281905"></a><a name="p127284281905"></a>Outputs an assembly file.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p93278335012"><a name="p93278335012"></a><a name="p93278335012"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p1976019511306"><a name="p1976019511306"></a><a name="p1976019511306"></a>-</p>
</td>
</tr>
<tr id="row9355174675912"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p6355104616592"><a name="p6355104616592"></a><a name="p6355104616592"></a>--dump-ast</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p187287280015"><a name="p187287280015"></a><a name="p187287280015"></a>Prints the parsed abstract syntax tree (AST).</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p932819331104"><a name="p932819331104"></a><a name="p932819331104"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p1475975114013"><a name="p1475975114013"></a><a name="p1475975114013"></a>-</p>
</td>
</tr>
<tr id="row53551046175917"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p13575501218"><a name="p13575501218"></a><a name="p13575501218"></a>--dump-debug-info</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p1372811281608"><a name="p1372811281608"></a><a name="p1372811281608"></a>Prints debug information.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p133287335020"><a name="p133287335020"></a><a name="p133287335020"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p37585513019"><a name="p37585513019"></a><a name="p37585513019"></a>-</p>
</td>
</tr>
<tr id="row8355204635911"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p657125010117"><a name="p657125010117"></a><a name="p657125010117"></a>--dump-literal-buffer</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p77281528704"><a name="p77281528704"></a><a name="p77281528704"></a>Prints the content of the literal buffer.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p83281633208"><a name="p83281633208"></a><a name="p83281633208"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p17580511404"><a name="p17580511404"></a><a name="p17580511404"></a>-</p>
</td>
</tr>
<tr id="row6355124665910"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p105611505114"><a name="p105611505114"></a><a name="p105611505114"></a>--dump-size-stat</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p20728192819015"><a name="p20728192819015"></a><a name="p20728192819015"></a>Displays bytecode statistics.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p1332810331508"><a name="p1332810331508"></a><a name="p1332810331508"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p157577519014"><a name="p157577519014"></a><a name="p157577519014"></a>-</p>
</td>
</tr>
<tr id="row235584610599"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p95515501012"><a name="p95515501012"></a><a name="p95515501012"></a>--extension</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p37282028600"><a name="p37282028600"></a><a name="p37282028600"></a>Specifies the input type.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p133281033804"><a name="p133281033804"></a><a name="p133281033804"></a>['js', 'ts', 'as']</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p675665112019"><a name="p675665112019"></a><a name="p675665112019"></a>-</p>
</td>
</tr>
<tr id="row135584635915"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p4551501217"><a name="p4551501217"></a><a name="p4551501217"></a>--help</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p157285282020"><a name="p157285282020"></a><a name="p157285282020"></a>Displays help information.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p1532819334016"><a name="p1532819334016"></a><a name="p1532819334016"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p475510516018"><a name="p475510516018"></a><a name="p475510516018"></a>-</p>
</td>
</tr>
<tr id="row133555461596"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p3541550416"><a name="p3541550416"></a><a name="p3541550416"></a>--module</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p27281728502"><a name="p27281728502"></a><a name="p27281728502"></a>Compile in ESM mode.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p832833312018"><a name="p832833312018"></a><a name="p832833312018"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p1975514517020"><a name="p1975514517020"></a><a name="p1975514517020"></a>-</p>
</td>
</tr>
<tr id="row23556463595"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p135313506120"><a name="p135313506120"></a><a name="p135313506120"></a>--opt-level</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p97284281607"><a name="p97284281607"></a><a name="p97284281607"></a>Specifies the compilation optimization level.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p43281335010"><a name="p43281335010"></a><a name="p43281335010"></a>['0', '1', '2']</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p57545511102"><a name="p57545511102"></a><a name="p57545511102"></a>0</p>
</td>
</tr>
<tr id="row5356124655916"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p185311501910"><a name="p185311501910"></a><a name="p185311501910"></a>--output</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p1872818281006"><a name="p1872818281006"></a><a name="p1872818281006"></a>
Specifies the output file path.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p73281733408"><a name="p73281733408"></a><a name="p73281733408"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p77537511606"><a name="p77537511606"></a><a name="p77537511606"></a>-</p>
</td>
</tr>
<tr id="row1335654635915"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p175213504115"><a name="p175213504115"></a><a name="p175213504115"></a>--parse-only</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p20729728003"><a name="p20729728003"></a><a name="p20729728003"></a>Parses only the input file.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p4328533205"><a name="p4328533205"></a><a name="p4328533205"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p175385118014"><a name="p175385118014"></a><a name="p175385118014"></a>-</p>
</td>
</tr>
<tr id="row1335654635915"><td class="cellrowborder" valign="top" width="12.898710128987101%" headers="mcps1.1.6.1.1 "><p id="p175213504115"><a name="p175213504115"></a><a name="p175213504115"></a>--thread</p>
</td>
<td class="cellrowborder" valign="top" width="19.33806619338066%" headers="mcps1.1.6.1.3 "><p id="p20729728003"><a name="p20729728003"></a><a name="p20729728003"></a>Specifies the number of threads used for generating bytecode.</p>
</td>
<td class="cellrowborder" valign="top" width="25.82741725827417%" headers="mcps1.1.6.1.4 "><p id="p4328533205"><a name="p4328533205"></a><a name="p4328533205"></a>0 to the maximum number of threads supported by the machine</p>
</td>
<td class="cellrowborder" valign="top" width="35.066493350664935%" headers="mcps1.1.6.1.5 "><p id="p175385118014"><a name="p175385118014"></a><a name="p175385118014"></a>0</p>
</td>
</tr>
</tbody>
</table>

### Disassembler Tool

The ark_disasm disassembler converts binary Ark bytecode files into text Ark bytecode files.

Compile and generate the disassembler tool.

```
./build.sh --product-name rk3568 --build-target arkcompiler/runtime_core:ark_host_linux_tools_packages
```

Command:

```
ark_disasm [Option] Input file Output file
```

<table><thead align="left"><tr id="row125182553217"><th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.1"><p id="p175162514327"><a name="p175162514327"></a><a name="p175162514327"></a>Option</p>
</th>
<th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.2"><p id="p6512255324"><a name="p6512255324"></a><a name="p6512255324"></a>Description</p>
</th>
</tr>
</thead>
<tbody><tr id="row5511825103218"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p45172513326"><a name="p45172513326"></a><a name="p45172513326"></a>--debug</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1245695053215"><a name="p1245695053215"></a><a name="p1245695053215"></a>Enables the debug information. If **--debug-file** is not specified, the information is output in standard output mode. The default value is **false**.</p>
</td>
</tr>
<tr id="row951112515321"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p451192515323"><a name="p451192515323"></a><a name="p451192515323"></a>--debug-file</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p175142583210"><a name="p175142583210"></a><a name="p175142583210"></a>Specifies the path of the debug information output file. The default value is **std::cout**.</p>
</td>
</tr>
<tr id="row951112515321"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p451192515323"><a name="p451192515323"></a><a name="p451192515323"></a>--skip-string-literals</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p175142583210"><a name="p175142583210"></a><a name="p175142583210"></a>Replaces the string with the corresponding **string_ID**, which can reduce the size of the output file. The default value is **false**.</p>
</td>
</tr>
<tr id="row951112515321"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p451192515323"><a name="p451192515323"></a><a name="p451192515323"></a>--quiet</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p175142583210"><a name="p175142583210"></a><a name="p175142583210"></a>Opens all **--skip-*** options. The default value is **false**.</p>
</td>
</tr>
<tr id="row45116253325"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p85116259328"><a name="p85116259328"></a><a name="p85116259328"></a>--help</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1348135833214"><a name="p1348135833214"></a><a name="p1348135833214"></a>Displays help information.</p>
</td>
</tr>
<tr id="row194197407327"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p154205401325"><a name="p154205401325"></a><a name="p154205401325"></a>--verbose</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p369871173312"><a name="p369871173312"></a><a name="p369871173312"></a>Outputs more information about classes and methods in the file. The default value is **false**.</p>
</td>
</tr>
</tbody>
</table>

Input file: Ark bytecode in binary format

Output file: Ark bytecode in text format

### AOT Tool

Ahead Of Time (AOT) compilation is a process of compiling bytecode into machine code that can be executed on the target device in advance on the host. In this way, the bytecode can be fully compiled and optimized, accelerating the running speed of the target device.

Compile and generate the AOT compiler.

```
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages
```

Command:

```
ark_aot_compiler [Option] Input file
```

| Option                         | Description                                                                               |
| -----------------------       | ----------------------------------------------------------------------------------- |
| --aot-file                    | Specifies the path of the AOT output file (without the file name extension). Default value: **aot_file**.                            |
| --compiler-opt-type-lowering  | Generates machine code with a higher optimization level using type information. Default value: **true**.                             |
| --compiler-opt-max-method     | Sets the method size threshold for AOT compilation. If the method size exceeds the threshold, the method is not compiled. Default value: **32 KB**.    |
| --compiler-opt-level          | Sets the optimization level of AOT. Default value: **3**.                                                     |
| --compiler-log                | Prints the log information of AOT, such as IR graphs and assembly code. Default value: **none**.                   |
| --compiler-log-snapshot       | Prints the log information related to serialization. Default value: **false**.                                          |
| --compiler-log-time           | Prints the time consumption of each optimization pass during AOT compilation. Default value: **false**.                               |


Input file: Ark bytecode in binary format

Output file: .an file of the machine code that can be directly executed or .ai file of the serialized ConstPool (The output file path must be specified using the **--aot-file** option.)

### PGO Tool

Profile-Guided Optimization (PGO) records frequently used (hotspot) functions in application startup and performance scenarios and stores the information in the corresponding PGO Profiler file. The AOT compiler can use the information to determine whether to compile some functions. In this way, the compilation time is shortened and the size of the .an file is reduced without affecting the application running performance.

Compile and generate the AOT compiler and Ark JS VM.

```
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages
```

### Generating a PGO Profiler File

Command:

```
ark_js_vm [Option] Input file
```

| Option                   | Description                                                                               |
| ----------------------- | ----------------------------------------------------------------------------------- |
| --enable-pgo-profiler   | Enables the PGO tool. Default value: **false**.                                                   |
| --compiler-pgo-profiler-path     | Specifies the path for storing the PGO Profiler file. Default value: **none**.                                    |
| --compiler-pgo-hotness-threshold | Specifies the threshold for hotspot functions. If the number of function calls is greater than the threshold, the function is considered as a hotspot function. Default value: **2**.            |

Input file: Ark bytecode in binary format

Output file: .ap file that stores hotspot function information

### AOT Compilation Based on the PGO Profiler

Command:

```
ark_aot_compiler [Option] Input file
```
| Option                   | Description                                                                               |
| ----------------------- | ----------------------------------------------------------------------------------- |
| --compiler-pgo-profiler-path     | Specifies the path of the PGO Profiler file. Default value: **none**.                                    |
| --compiler-pgo-hotness-threshold | Specifies the threshold of the number of function calls for enabling PGO compilation. Only the functions whose number of calls recorded in the profile file is greater than the threshold are compiled. Default value: **2**.|

Input file: .ap file that contains Ark bytecode in binary format

Output file: .an file that contains the machine code for direct execution (based on PGO)
