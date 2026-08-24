## Introduction

This folder contains test cases for ArkSteed JIT compiler.

Each leaf sub-directory contains a test case consisting of the following files:
* **Exactly one** JS/TS file which is the test case;
* **Exactly one** `expected_output.txt` file which holds the expected contents to stdout.

`./run_arksteed_tests.py` is the test case runner which collects all the test cases by traversing the directory tree and then run them.

Special comments of JS/TS files starting with `//!` are *annotations* specifying the runtime options and constraints to the compiled CFG (which IR nodes are expected to appear, not to appear, etc.) of this test case. The CFGs are dumped via compiler log and then parsed by the test case runner.

Beside the `//!` IR constraints, the runner verifies the live range correctness of every dumped CFG: no value's live range may end before one of its uses starts, and every value used as a (allocated) Phi input must have a live range.

## Test Case Runner

Execute the command `python3 ./run_arksteed_tests.py [<args> ...]` to run all the test cases.

Optional arguments:
* `-p <ARCH>` or `--platform <ARCH>`: Switches between platforms. `<ARCH>` can one of `x64` or `arm64`. The default value of `<ARCH>` is the same as host platform. QEMU is required for cross-platform testing.
* `-m <MODE>` or `--mode <MODE>`: Switches between debug and release build. `<MODE>` can be one of `debug` (by default) or `release`.
* `-f` or `--skip-stub`: Skips building `stub.an`.
* `-F` or `--skip-build`: Skips building all the targets.
* `-I`, `--include` or `--filter <DIR>`: Filters the test cases to be executed with given *directory pattern*. Shell-style wildcards are supported: `--include control_flow/if/else*` keeps both `else` and `elseif`.
    * `--include` can appear multiple times to include more than one directories.
* `-X` or `--exclude <DIR>`: Excludes the test cases with given *directory pattern* (wildcards supported).
    * Example: `--exclude functions/constructor` excludes all the cases in `./functions/constructor`.
    * `--exclude` can appear multiple times to exclude more than one directories.
* `-d` or `--log-debug`: Sets log level to debug.
* `-l` or `--log-components`: Sets log components *besides* compiler log (comma-separated, e.g. `-l gc,ecma`).
    * Example: `-l gc` makes the runner pass `--log-components=compiler:gc` to `ark_js_vm` (note: the VM expects a colon-separated list, and the compiler log is always enabled).
* `--print-asm-code`: Dumps generated ASM code (along with comments) to compiler log.
* `-c` or `--colored`: Enables colored text on the terminal.
* `-C` or `--fully-colored`: Enables colored text both on the terminal and in the compiler log.
* `-s` or `--stops-on-error`: Stops the runner if any test case fails.
* `-j <N>` or `--num-workers <N>`: Enables parallelism with given number of workers.
* `-v` or `--verbose`: Shows verbose information.

## Artifacts

All generated artifacts are written to a fresh `/tmp/arksteed-<platform>-<mode>-<timestamp>/` directory whose tree mirrors this test directory:

* `<case>/<case>.abc`: The compiled bytecode file;
* `<case>/<case>.disasm.txt`: Disassembly of the `.abc` file;
* `<case>/<case>.stdout.txt` / `<case>/<case>.stderr.txt`: Captured VM output. The compiler log (including the CFG dumps checked by the `//!` IR constraints) is always enabled and goes to stdout;
* `run.meta.json`: Run metadata (arguments, git revision, tool locations).

## Test Case Annotations

Annotations start with `//!`. In-line comments to annotations (starting with `#`) are supported.

* `PARAMS <params>`: Which runtime options shall be passed to the ArkTS runtime (i.e. the `ark_js_vm` executable) for this test case.
    * Example: `//! PARAMS --enable-force-gc=true --compiler-jit-hotness-threshold=100` passes these 2 parameters to `ark_js_vm`.
    * A parameter replaces the runner default of the same option (e.g. `--enable-force-gc=true` overrides the default `=false`).
    * The following options are core to the runner and **can not** be overriden by `PARAMS` annotation. An error will occur if you attempt to override them:
        * `--log-level`
        * `--log-components`
        * All `--compiler-arksteed-print-*` options
        * `--compiler-arksteed-enable-code-comment`
        * `--stub-file`
        * `--icu-data-path`
    * `PARAM` annotation shall appear at most once in each test case.
* `METHOD <function>`: Leads the IR constraints to the method specified;
    * Example: All the annotations after `//! METHOD foo` (and nefore the next `METHOD`) are the IR constraints of `function foo()`.
* `HAS <opcode>`: Specifies an IR opcode which is expected to appear at least once.
    * Example: `//! HAS I32Mul` requires that the compiled CFG of current method should contain at least one IR node of type `I32Mul`.
* `HAS_NOT <opcode>`: Specifies an IR opcode which is expected *not* to appear.
    * Example: `//! HAS_NOT I32Add` requires that the compiled CFG of current method should not contain any IR node of type `I32Add`.
* `COUNT <opcode> <n>`: Specifies an IR opcode which is expected to appear *exactly* n times.
    * Example: `//! COUNT Call 3` requires that the compiled CFG of current method should not contain exactly 3 IR nodes of type `Call`.
* `COUNT_GE <opcode> <n>`: Specifies an IR opcode which is expected to appear *at least* n times.
    * Example: `//! COUNT_GE Call 3` requires that the compiled CFG of current method should not contain 3 or more IR nodes of type `Call`.
* `COUNT_LE <opcode> <n>`: Specifies an IR opcode which is expected to appear *at most* n times.
    * Example: `//! COUNT_GE Call 5` requires that the compiled CFG of current method should not contain 5 or less IR nodes of type `Call`.

Constraints on `CallCommonStub` and `CallRuntime` **must** be followed by a stub-name pattern (shell-style wildcards supported).
* The pattern is matched against the stub name as dumped (`COStub_*` / `RTStub_*`) as well as its prefix-stripped form, so `GetPropertyByName`, `Get*ByName` and `COStub_GetPropertyByName` are all valid spellings.
* Example: `//! HAS_NOT CallCommonStub GetPropertyByName` requires that no `CallCommonStub` node calling `COStub_GetPropertyByName` appears, while e.g. `COStub_Add` calls are unaffected.
* Example: `//! COUNT CallRuntime Throw* 1` requires exactly one `CallRuntime` node whose stub name starts with `Throw`.

Example:

```js
//  PARAMS applies to all the functions in this test case. (p.s. This line is trivial JS/TS comment)
//! PARAMS  --enable-force-gc=true --compiler-jit-hotness-threshold=60000

//! METHOD      foo
//! HAS         CallCommonStub  GetPropertyByName
//! HAS_NOT     I32Mul
//! COUNT_GE    InitialValue    4
//! COUNT_LE    InitialValue    5

//! METHOD  bar
//! HAS     LoadTaggedField             # Loads GlobalEnv from LexicalEnv
//! COUNT   Call            1
```

More examples are shown in the following test cases:
* `pgo/int32/test_1`
* `pgo/int32/test_2`
