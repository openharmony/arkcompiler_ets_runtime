/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/js_runtime_options.h"

#include <getopt.h>
#include <type_traits>

#include "common_components/common_runtime/base_runtime_param.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/ecma_opcode_des.h"
#include "ecmascript/platform/os.h"

namespace panda::ecmascript {
const std::string PUBLIC_API COMMON_HELP_HEAD_MSG =
    "Usage: jsvm  <option>  <filename.abc>\n"
    "\n"
    "Options:\n";

const std::string PUBLIC_API COMPILER_HELP_HEAD_MSG =
    "Usage: ark_aot_compiler  <option>  --aot-file=<filename>  <filename.abc>\n"
    "\n"
    "Options:\n";

const std::string PUBLIC_API STUB_HELP_HEAD_MSG =
    "Usage: ark_stub_compiler <option>\n"
    "\n"
    "Options:\n";

const std::string PUBLIC_API HELP_OPTION_MSG =
    "--help:                               Print this message and exit\n"
    "--aot-file:                           Path (file suffix not needed) to AOT output file. Default: 'aot_file'\n"
    "--ark-properties:                     Set ark properties\n"
    "--ark-bundle-name:                    Set ark bundle name\n"
    "--asm-interpreter:                    Enable asm interpreter. Default: 'true'\n"
    "--asm-opcode-disable-range:           Opcode range when asm interpreter is enabled.\n"
    "--builtins-lazy:                      Load some builtins function later.This option is only valid in workervm.\n"
    "--compiler-log:                       Log Option For aot compiler and stub compiler,\n"
    "                                      'none': no log,\n"
    "                                      'allllircirasm' or 'all012': print all log for all methods,\n"
    "                                      'allcir' or 'all0': print IR info for all methods,\n"
    "                                      'allllir' or 'all1': print llir info for all methods,\n"
    "                                      'allasm' or 'all2': print asm log for all methods,\n"
    "                                      'alltype' or 'all3': print type infer log for all methods,\n"
    "                                      'cerllircirasm' or 'cer0112': print all log for certain method defined\n"
    "                                       in 'mlist-for-log',\n"
    "                                      'cercir' or 'cer0': print IR for methods in 'mlist-for-log',\n"
    "                                      'cerasm' or 'cer2': print log for methods in 'mlist-for-log',\n"
    "                                      Default: 'none'\n"
    "--compiler-log-methods:               Specific method list for compiler log, only used when compiler-log. "
    "                                      Default: 'none'\n"
    "--compiler-type-threshold:            enable to skip methods whose type is no more than threshold. Default: -1\n"
    "--compiler-log-snapshot:              Enable to print snapshot information. Default: 'false'\n"
    "--compiler-log-time:                  Enable to print pass compiler time. Default: 'false'\n"
    "--enable-ark-tools:                   Enable ark tools to debug. Default: 'false'\n"
    "--open-ark-tools:                     Open ark tools to return specific implementation. Default: 'false'\n"
    "--pgo-trace:                          Enable pgo trace for JS runtime. Default: 'false'\n"
    "--compiler-trace-bc:                  Enable tracing bytecode for aot runtime. Default: 'false'\n"
    "--compiler-trace-deopt:               Enable tracing deopt for aot runtime. Default: 'false'\n"
    "--compiler-trace-inline:              Enable tracing inline function for aot runtime. Default: 'false'\n"
    "--compiler-trace-builtins:            Enable tracing builtins function for aot runtime. Default: 'false'\n"
    "--compiler-trace-value-numbering:     Enable tracing value numbering for aot runtime. Default: 'false'\n"
    "--compiler-max-inline-bytecodes       Set max bytecodes count which aot function can be inlined. Default: '25'\n"
    "--compiler-deopt-threshold:           Set max count which aot function can occur deoptimization. Default: '10'\n"
    "--compiler-stress-deopt:              Enable stress deopt for aot compiler. Default: 'false'\n"
    "--compiler-opt-code-profiler:         Enable opt code Bytecode Statistics for aot runtime. Default: 'false'\n"
    "--compiler-opt-bc-range:              Range list for EcmaOpCode range Example '1:2,5:8'\n"
    "--compiler-opt-bc-range-help:         Range list for EcmaOpCode range help. Default: 'false''\n"
    "--enable-loading-stubs-log:           Enable Loading Stubs Log. Default: 'false'\n"
    "--enable-force-gc:                    Enable force gc when allocating object. Default: 'true'\n"
#ifdef USE_CMC_GC
    "--enable-cmc-gc:                      Enable cmc gc. Default: 'true'\n"
#else
    "--enable-cmc-gc:                      Enable cmc gc. Default: 'false'\n"
#endif
    "--enable-cmc-gc-concurrent-root-marking:         Enable concurrent root marking in cmc gc. Default: 'true'\n"
    "--force-shared-gc-frequency:          How frequency force shared gc . Default: '1'\n"
    "--enable-ic:                          Switch of inline cache. Default: 'true'\n"
    "--enable-runtime-stat:                Enable statistics of runtime state. Default: 'false'\n"
    "--compiler-opt-array-bounds-check-elimination: Enable Index Check elimination. Default: 'true'\n"
    "--compiler-opt-constant-folding:      Enable constant folding. Default: 'true'\n"
    "--compiler-opt-type-lowering:         Enable all type optimization pass for aot compiler. Default: 'true'\n"
    "--compiler-opt-early-elimination:     Enable EarlyElimination for aot compiler. Default: 'true'\n"
    "--compiler-opt-later-elimination:     Enable LaterElimination for aot compiler. Default: 'true'\n"
    "--compiler-opt-string:                Enable string optimization pass for aot compiler. Default: 'true'\n"
    "--compiler-opt-value-numbering:       Enable ValueNumbering for aot compiler. Default: 'true'\n"
    "--compiler-opt-inlining:              Enable inlining function for aot compiler: Default: 'true'\n"
    "--compiler-opt-pgotype:               Enable pgo type for aot compiler: Default: 'true'\n"
    "--compiler-opt-track-field:           Enable track field for aot compiler: Default: 'false'\n"
    "--entry-point:                        Full name of entrypoint function. Default: '_GLOBAL::func_main_0'\n"
    "--force-full-gc:                      If true trigger full gc, else trigger semi and old gc. Default: 'true'\n"
    "--framework-abc-file:                 Snapshot file. Default: 'strip.native.min.abc'\n"
    "--gc-long-paused-time:                Set gc's longPauseTime in millisecond. Default: '40'\n"
    "--gc-thread-num:                      Set gc thread number. Default: '7'\n"
    "--icu-data-path:                      Path to generated icu data file. Default: 'default'\n"
    "--enable-worker:                      Whether is worker vm. Default: 'false'\n"
    "--log-level:                          Log level: ['debug', 'info', 'warning', 'error', 'fatal'].\n"
    "--log-components:                     Enable logs from specified components: ['all', 'gc', 'ecma','interpreter',\n"
    "                                      'debugger', 'compiler', 'builtins', 'trace', 'jit', 'baselinejit', 'all'].\n"
    "                                      Default: 'all'\n"
    "--log-debug:                          Enable debug or above logs for components: ['all', 'gc', 'ecma',\n"
    "                                      'interpreter', 'debugger', 'compiler', 'builtins', 'trace', 'jit',\n"
    "                                      'baselinejit', 'all']. Default: 'all'\n"
    "--log-error:                          Enable error log for components: ['all', 'gc', 'ecma',\n"
    "                                      'interpreter', 'debugger', 'compiler', 'builtins', 'trace', 'jit',\n"
    "                                      'baselinejit', 'all']. Default: 'all'\n"
    "--log-fatal:                          Enable fatal log for components: ['all', 'gc', 'ecma',\n"
    "                                      'interpreter', 'debugger', 'compiler', 'builtins', 'trace', 'jit',\n"
    "                                      'baselinejit', 'all']. Default: 'all'\n"
    "--log-info:                           Enable info log for components: ['all', 'gc', 'ecma',\n"
    "                                      'interpreter', 'debugger', 'compiler', 'builtins', 'trace', 'jit',\n"
    "                                      'baselinejit', 'all']. Default: 'all'\n"
    "--log-warning:                        Enable warning log for components: ['all', 'gc', 'ecma',\n"
    "                                      'interpreter', 'debugger', 'compiler', 'trace', 'jit', \n"
    "                                      'baselinejit', 'builtins', 'all']. Default: 'all'\n"
    "--compiler-opt-max-method:            Enable aot compiler to skip method larger than limit (KB). Default: '32'\n"
    "--compiler-module-methods:            The number of max compiled methods in a module. Default: '100'\n"
    "--max-unmovable-space:                Set max unmovable space capacity\n"
    "--merge-abc:                          ABC file is merge abc. Default: 'false'\n"
    "--compiler-opt-level:                 Optimization level configuration of aot compiler. Default: '3'\n"
    "--options:                            Print compiler and runtime options\n"
    "--serializer-buffer-size-limit:       Max serializer buffer size used by the VM in Byte. Default size is 2GB\n"
    "--snapshot-file:                      Snapshot file. Default: '/system/etc/snapshot'\n"
    "--startup-time:                       Print the start time of command execution. Default: 'false'\n"
    "--stub-file:                          Path of file includes common stubs module compiled by stub compiler. \n"
    "                                      Default: 'stub.an'\n"
    "--enable-pgo-profiler:                Enable pgo profiler to sample jsfunction call and output to file. "
                                           "Default: 'false'\n"
    "--enable-mutant-array:                Enable transition between mutant array and tagged array. Default: 'false'\n"
    "--enable-elements-kind:               Enable initialization of elements kind in array. Default: 'false'\n"
    "--compiler-pgo-hotness-threshold:     Set hotness threshold for pgo in aot compiler. Default: '2'\n"
    "--compiler-pgo-profiler-path:         The pgo file output dir or the pgo file dir of AOT compiler. Default: ''\n"
    "--compiler-pgo-save-min-interval:     Set the minimum time interval for automatically saving profile, "
    "Unit seconds. Default: '30s'\n"
    "--compiler-baseline-pgo:              Enable compile the baseline Ap file. \n"
    "                                      Default: 'false'\n"
    "--compiler-target-triple:             CPU triple for aot compiler or stub compiler. \n"
    "                                      values: ['x86_64-unknown-linux-gnu', 'arm-unknown-linux-gnu', \n"
    "                                      'aarch64-unknown-linux-gnu'], Default: 'x86_64-unknown-linux-gnu'\n"
    "--enable-print-execute-time:          Enable print execute panda file spent time\n"
    "--compiler-verify-vtable:             Verify vtable result for aot compiler. Default: 'false'\n"
    "--compiler-select-methods             Compiler selected methods for aot. Only work in full compiling mode\n"
    "                                      Format:--compile-methods=record1:m1,m2,record2:m3\n"
    "--compiler-skip-methods               Compiler skpped methods for aot. Only work in full compiling mode\n"
    "                                      Format:--compile-skip-methods=record1:m1,m2,record2:m3\n"
    "--target-compiler-mode                The compilation mode at the device side, including partial, full and none.\n"
    "                                      Default: ''\n"
    "--hap-path(Deprecated)                The path of the app hap. Default: ''\n"
    "--hap-abc-offset(Deprecated)          The offset of the abc file in app hap. Default: '0'\n"
    "--hap-abc-size(Deprecated)            The size of the abc file in app hap. Default: '0'\n"
    "--compiler-fast-compile               Disable some time-consuming pass. Default: 'true'\n"
    "--compiler-no-check                   Enable remove checks for aot compiler. Default: 'false'\n"
    "--compiler-pipeline-host-aot          Enable pipeline host aot compiler. Default: 'false'\n"
    "--compiler-opt-loop-peeling:          Enable loop peeling for aot compiler: Default: 'true'\n"
    "--compiler-pkg-info                   Specify the package json info for ark aot compiler\n"
    "--compiler-external-pkg-info          Specify the external package json info for ark aot compiler\n"
    "--compiler-enable-external-pkg        Enable compile with external package for ark aot compiler\n"
    "--compiler-enable-lexenv-specialization: Enable replace ldlexvar with specific values: Default: 'true'\n"
    "--compiler-enable-native-inline:      Enable inline native function: Default: 'false'\n"
    "--compiler-enable-lowering-builtin:   Enable lowering global object: Default: 'false'\n"
    "--compiler-opt-array-onheap-check:    Enable TypedArray on heap check for aot compiler: Default: 'false'\n"
    "--compiler-enable-litecg:             Enable LiteCG: Default: 'false'\n"
    "--compiler-enable-jit:                Enable jit: Default: 'false'\n"
    "--compiler-enable-osr:                Enable osr: Default: 'false'\n"
    "--compiler-enable-framework-aot:      Enable frame aot: Default: 'true'\n"
    "--compiler-enable-pgo-space:          Enable pgo space used for compiler. Default: 'true'\n"
    "--compiler-jit-hotness-threshold:     Set hotness threshold for jit. Default: '2'\n"
    "--compiler-jit-call-threshold:        Set call threshold for jit. Default: '0'\n"
    "--compiler-osr-hotness-threshold:     Set hotness threshold for osr. Default: '2'\n"
    "--compiler-force-jit-compile-main:    Enable jit compile main function: Default: 'false'\n"
    "--compiler-trace-jit:                 Enable trace jit: Default: 'false'\n"
    "--compiler-enable-jit-pgo:            Enable jit pgo: Default: 'true'\n"
    "--compiler-typed-op-profiler:         Enable Typed Opcode Statistics for aot runtime. Default: 'false'\n"
    "--compiler-opt-branch-profiling:      Enable branch profiling for aot compiler. Default: 'true'\n"
    "--test-assert:                        Set Assert Model. Default: 'false'\n"
    "--compiler-methods-range:             Enable aot compiler to compile only in-range methods.\n"
    "                                      Default: '0:4294967295'\n"
    "--compiler-codegen-options:           Compile options passed to codegen. Default: ''\n"
    "--compiler-opt-escape-analysis:       Enable escape analysis for aot compiler. Default: 'true'\n"
    "--compiler-trace-escape-analysis:     Enable tracing escape analysis for aot compiler. Default: 'false'\n"
    "--compiler-opt-induction-variable:    Enable induciton variable analysis for aot compiler. Default: 'false'\n"
    "--compiler-trace-induction-variable:  Enable tracing induction variable for aot compiler. Default: 'false'\n"
    "--compiler-memory-analysis:           Enable memory analysis for aot compiler. Default: 'true'\n"
    "--compiler-enable-jit-fast-compile:   Enable jit fast compile. Default: 'false'\n"
    "--compiler-enable-jitfort:            Enable jit fort memory space. Default: 'false'\n"
    "--compiler-codesign-disable:          Disable codesign for jit fort. Default: 'true'\n"
    "--compiler-enable-async-copytofort:   Enable jit fort allocation and code copy in Jit thread. Default: 'true'\n"
    "--compiler-pgo-force-dump:            Enable pgo dump not interrupted by GC. Default: 'true'\n"
    "--async-load-abc:                     Enable asynchronous load abc. Default: 'true'\n"
    "--async-load-abc-test:                Enable asynchronous load abc test. Default: 'false'\n"
    "--compiler-enable-store-barrier:      Enable store barrier optimization. Default: 'true'\n"
    "--compiler-enable-ldobjvalue-opt:     Enable ldobjvalue optimization. Default: 'true'\n"
    "--compiler-enable-concurrent:         Enable concurrent compile(only support in ark_stub_compiler).\n"
    "--compile-enable-jit-verify-pass:     Enable jit compile with verify pass. Default: 'false'\n\n"
    "                                      Default: 'true'\n"
    "--compiler-opt-frame-state-elimination: Enable frame state elimination. Default: 'true'\n"
    "--enable-inline-property-optimization:  Enable inline property optimization(also enable slack tracking).\n"
    "--compiler-enable-aot-code-comment    Enable generate aot_code_comment.txt file during compilation.\n"
    "                                      Default : 'false'\n"
    "--compiler-enable-aot-lazy-deopt:     Enable lazy deopt for aot compiler. Default: 'false'\n"
    "--compiler-enable-jit-lazy-deopt:     Enable lazy deopt for jit compiler. Default: 'false'\n"
    "--compiler-an-file-max-size:          Max size of compiler .an file in MB. '0' means Default\n"
    "                                      Default: No limit for Host, '100' for TargetCompilerMode\n"
    "--compiler-jit-method-dichotomy:      Find problematic method by dichotomy. Default: 'disable'\n"
    "                                      'collect': collect methods during JIT runtime.\n"
    "                                      'filter': only compile specified methods during JIT runtime.\n"
    "--compiler-jit-method-path:           Store method names for jit method dichotomy.\n"
    "                                      Default: 'method_compiled_by_jit.cfg'\n"
    "--compiler-enable-merge-poly:         Enable poly-merge optimization for ldobjbyname. Default: 'true'\n"
    // Please add new options above this line for keep a blank line after help message.
    "\n";

bool JSRuntimeOptions::ParseCommand(const int argc, const char **argv)
{
    const struct option longOptions[] = {
        {"help", no_argument, nullptr, OPTION_HELP},
        {"aot-file", required_argument, nullptr, OPTION_AOT_FILE},
        {"ark-properties", required_argument, nullptr, OPTION_ARK_PROPERTIES},
        {"ark-bundleName", required_argument, nullptr, OPTION_ARK_BUNDLENAME},
        {"asm-interpreter", required_argument, nullptr, OPTION_ENABLE_ASM_INTERPRETER},
        {"asm-opcode-disable-range", required_argument, nullptr, OPTION_ASM_OPCODE_DISABLE_RANGE},
        {"builtins-lazy", required_argument, nullptr, OPTION_ENABLE_BUILTINS_LAZY},
        {"compiler-log", required_argument, nullptr, OPTION_COMPILER_LOG_OPT},
        {"compiler-log-methods", required_argument, nullptr, OPTION_COMPILER_LOG_METHODS},
        {"compiler-log-snapshot", required_argument, nullptr, OPTION_COMPILER_LOG_SNAPSHOT},
        {"compiler-log-time", required_argument, nullptr, OPTION_COMPILER_LOG_TIME},
        {"compiler-type-threshold", required_argument, nullptr, OPTION_COMPILER_TYPE_THRESHOLD},
        {"enable-ark-tools", required_argument, nullptr, OPTION_ENABLE_ARK_TOOLS},
        {"open-ark-tools", required_argument, nullptr, OPTION_OPEN_ARK_TOOLS},
        {"pgo-trace", required_argument, nullptr, OPTION_PGO_TRACE},
        {"compiler-trace-bc", required_argument, nullptr, OPTION_COMPILER_TRACE_BC},
        {"compiler-trace-deopt", required_argument, nullptr, OPTION_COMPILER_TRACE_DEOPT},
        {"compiler-trace-inline", required_argument, nullptr, OPTION_COMPILER_TRACE_INLINE},
        {"compiler-trace-value-numbering", required_argument, nullptr, OPTION_COMPILER_TRACE_VALUE_NUMBERING},
        {"compiler-trace-instruction-combine", required_argument, nullptr, OPTION_COMPILER_TRACE_INSTRUCTION_COMBINE},
        {"compiler-max-inline-bytecodes", required_argument, nullptr, OPTION_COMPILER_MAX_INLINE_BYTECODES},
        {"compiler-deopt-threshold", required_argument, nullptr, OPTION_COMPILER_DEOPT_THRESHOLD},
        {"compiler-device-state", required_argument, nullptr, OPTION_COMPILER_DEVICE_STATE},
        {"compiler-thermal-level", required_argument, nullptr, OPTION_COMPILER_THERMAL_LEVEL},
        {"compiler-stress-deopt", required_argument, nullptr, OPTION_COMPILER_STRESS_DEOPT},
        {"compiler-opt-code-profiler", required_argument, nullptr, OPTION_COMPILER_OPT_CODE_PROFILER},
        {"compiler-opt-bc-range", required_argument, nullptr, OPTION_COMPILER_OPT_BC_RANGE},
        {"compiler-opt-bc-range-help", required_argument, nullptr, OPTION_COMPILER_OPT_BC_RANGE_HELP},
        {"enable-loading-stubs-log", required_argument, nullptr, OPTION_ENABLE_LOADING_STUBS_LOG},
        {"enable-force-gc", required_argument, nullptr, OPTION_ENABLE_FORCE_GC},
        {"enable-cmc-gc", required_argument, nullptr, OPTION_ENABLE_CMC_GC},
        {"enable-cmc-gc-concurrent-root-marking", required_argument, nullptr,
         OPTION_ENABLE_CMC_GC_CONCURRENT_ROOT_MARKING},
        {"enable-ic", required_argument, nullptr, OPTION_ENABLE_IC},
        {"enable-runtime-stat", required_argument, nullptr, OPTION_ENABLE_RUNTIME_STAT},
        {"compiler-opt-constant-folding", required_argument, nullptr, OPTION_COMPILER_OPT_CONSTANT_FOLDING},
        {"compiler-opt-array-bounds-check-elimination", required_argument, nullptr,
         OPTION_COMPILER_OPT_ARRAY_BOUNDS_CHECK_ELIMINATION},
        {"compiler-opt-type-lowering", required_argument, nullptr, OPTION_COMPILER_OPT_TYPE_LOWERING},
        {"compiler-opt-early-elimination", required_argument, nullptr, OPTION_COMPILER_OPT_EARLY_ELIMINATION},
        {"compiler-opt-later-elimination", required_argument, nullptr, OPTION_COMPILER_OPT_LATER_ELIMINATION},
        {"compiler-opt-instr-combine", required_argument, nullptr, OPTION_COMPILER_OPT_INSTRUCTIONE_COMBINE},
        {"compiler-opt-string", required_argument, nullptr, OPTION_COMPILER_OPT_STRING},
        {"compiler-opt-value-numbering", required_argument, nullptr, OPTION_COMPILER_OPT_VALUE_NUMBERING},
        {"compiler-opt-new-value-numbering", required_argument, nullptr, OPTION_COMPILER_OPT_NEW_VALUE_NUMBERING},
        {"compiler-opt-inlining", required_argument, nullptr, OPTION_COMPILER_OPT_INLINING},
        {"compiler-opt-pgotype", required_argument, nullptr, OPTION_COMPILER_OPT_PGOTYPE},
        {"compiler-opt-track-field", required_argument, nullptr, OPTION_COMPILER_OPT_TRACK_FIELD},
        {"entry-point", required_argument, nullptr, OPTION_ENTRY_POINT},
        {"force-full-gc", required_argument, nullptr, OPTION_FORCE_FULL_GC},
        {"force-shared-gc-frequency", required_argument, nullptr, OPTION_ENABLE_FORCE_SHARED_GC_FREQUENCY},
        {"enable-heap-verify", required_argument, nullptr, OPTION_ENABLE_HEAP_VERIFY},
        {"gc-thread-num", required_argument, nullptr, OPTION_GC_THREADNUM},
        {"icu-data-path", required_argument, nullptr, OPTION_ICU_DATA_PATH},
        {"enable-worker", required_argument, nullptr, OPTION_ENABLE_WORKER},
        {"log-components", required_argument, nullptr, OPTION_LOG_COMPONENTS},
        {"log-debug", required_argument, nullptr, OPTION_LOG_DEBUG},
        {"log-error", required_argument, nullptr, OPTION_LOG_ERROR},
        {"log-fatal", required_argument, nullptr, OPTION_LOG_FATAL},
        {"log-info", required_argument, nullptr, OPTION_LOG_INFO},
        {"log-level", required_argument, nullptr, OPTION_LOG_LEVEL},
        {"log-warning", required_argument, nullptr, OPTION_LOG_WARNING},
        {"gc-long-paused-time", required_argument, nullptr, OPTION_GC_LONG_PAUSED_TIME},
        {"compiler-opt-max-method", required_argument, nullptr, OPTION_COMPILER_OPT_MAX_METHOD},
        {"compiler-module-methods", required_argument, nullptr, OPTION_COMPILER_MODULE_METHODS},
        {"max-unmovable-space", required_argument, nullptr, OPTION_MAX_UNMOVABLE_SPACE},
        {"merge-abc", required_argument, nullptr, OPTION_MERGE_ABC},
        {"enable-context", required_argument, nullptr, OPTION_ENABLE_CONTEXT},
        {"compiler-opt-level", required_argument, nullptr, OPTION_ASM_OPT_LEVEL},
        {"reloc-mode", required_argument, nullptr, OPTION_RELOCATION_MODE},
        {"serializer-buffer-size-limit", required_argument, nullptr, OPTION_SERIALIZER_BUFFER_SIZE_LIMIT},
        {"startup-time", required_argument, nullptr, OPTION_STARTUP_TIME},
        {"stub-file", required_argument, nullptr, OPTION_STUB_FILE},
        {"compiler-target-triple", required_argument, nullptr, OPTION_COMPILER_TARGET_TRIPLE},
        {"enable-print-execute-time", required_argument, nullptr, OPTION_PRINT_EXECUTE_TIME},
        {"enable-pgo-profiler", required_argument, nullptr, OPTION_ENABLE_PGO_PROFILER},
        {"enable-mutant-array", required_argument, nullptr, OPTION_ENABLE_MUTANT_ARRAY},
        {"enable-elements-kind", required_argument, nullptr, OPTION_ENABLE_ELEMENTS_KIND},
        {"compiler-pgo-profiler-path", required_argument, nullptr, OPTION_COMPILER_PGO_PROFILER_PATH},
        {"compiler-pgo-hotness-threshold", required_argument, nullptr, OPTION_COMPILER_PGO_HOTNESS_THRESHOLD},
        {"compiler-pgo-save-min-interval", required_argument, nullptr, OPTION_COMPILER_PGO_SAVE_MIN_INTERVAL},
        {"compiler-baseline-pgo", required_argument, nullptr, OPTION_COMPILER_BASELINE_PGO},
        {"compiler-verify-vtable", required_argument, nullptr, OPTION_COMPILER_VERIFY_VTABLE},
        {"compiler-select-methods", required_argument, nullptr, OPTION_COMPILER_SELECT_METHODS},
        {"compiler-skip-methods", required_argument, nullptr, OPTION_COMPILER_SKIP_METHODS},
        {"target-compiler-mode", required_argument, nullptr, OPTION_TARGET_COMPILER_MODE},
        {"hap-path", required_argument, nullptr, OPTION_HAP_PATH},
        {"hap-abc-offset", required_argument, nullptr, OPTION_HAP_ABC_OFFSET},
        {"hap-abc-size", required_argument, nullptr, OPTION_HAP_ABC_SIZE},
        {"compiler-no-check", required_argument, nullptr, OPTION_COMPILER_NOCHECK},
        {"compiler-pipeline-host-aot", required_argument, nullptr, OPTION_COMPILER_PIPELINE_HOST_AOT},
        {"compiler-fast-compile", required_argument, nullptr, OPTION_FAST_AOT_COMPILE_MODE},
        {"compiler-opt-loop-peeling", required_argument, nullptr, OPTION_COMPILER_OPT_LOOP_PEELING},
        {"compiler-opt-array-onheap-check", required_argument, nullptr, OPTION_COMPILER_OPT_ON_HEAP_CHECK},
        {"compiler-pkg-info", required_argument, nullptr, OPTION_COMPILER_PKG_INFO},
        {"compiler-external-pkg-info", required_argument, nullptr, OPTION_COMPILER_EXTERNAL_PKG_INFO},
        {"compiler-enable-external-pkg", required_argument, nullptr, OPTION_COMPILER_ENABLE_EXTERNAL_PKG},
        {"compiler-framework-abc-path", required_argument, nullptr, OPTION_COMPILER_FRAMEWORK_ABC_PATH},
        {"compiler-enable-lexenv-specialization", required_argument, nullptr,
         OPTION_COMPILER_ENABLE_LEXENV_SPECIALIZATION},
        {"compiler-enable-native-inline", required_argument, nullptr, OPTION_COMPILER_ENABLE_NATIVE_INLINE},
        {"compiler-enable-lowering-builtin", required_argument, nullptr, OPTION_COMPILER_ENABLE_LOWERING_BUILTIN},
        {"compiler-enable-litecg", required_argument, nullptr, OPTION_COMPILER_ENABLE_LITECG},
        {"compiler-enable-jit", required_argument, nullptr, OPTION_COMPILER_ENABLE_JIT},
        {"compiler-enable-dfx-hisys-event", required_argument, nullptr, OPTION_COMPILER_ENABLE_DFX_HISYS_EVENT},
        {"compiler-enable-osr", required_argument, nullptr, OPTION_COMPILER_ENABLE_OSR},
        {"compiler-trace-jit", required_argument, nullptr, OPTION_COMPILER_TRACE_JIT},
        {"compiler-jit-hotness-threshold", required_argument, nullptr, OPTION_COMPILER_JIT_HOTNESS_THRESHOLD},
        {"compiler-jit-call-threshold", required_argument, nullptr, OPTION_COMPILER_JIT_CALL_THRESHOLD},
        {"compiler-osr-hotness-threshold", required_argument, nullptr, OPTION_COMPILER_OSR_HOTNESS_THRESHOLD},
        {"compiler-force-jit-compile-main", required_argument, nullptr, OPTION_COMPILER_FORCE_JIT_COMPILE_MAIN},
        {"compiler-enable-jit-pgo", required_argument, nullptr, OPTION_COMPILER_ENABLE_JIT_PGO},
        {"compiler-enable-aot-pgo", required_argument, nullptr, OPTION_COMPILER_ENABLE_AOT_PGO},
        {"compiler-enable-framework-aot", required_argument, nullptr, OPTION_COMPILER_ENABLE_FRAMEWORK_AOT},
        {"compiler-enable-profile-dump", required_argument, nullptr, OPTION_COMPILER_ENABLE_PROPFILE_DUMP},
        {"compiler-typed-op-profiler", required_argument, nullptr, OPTION_COMPILER_TYPED_OP_PROFILER},
        {"compiler-opt-branch-profiling", required_argument, nullptr, OPTION_COMPILER_OPT_BRANCH_PROFILING},
        {"test-assert", required_argument, nullptr, OPTION_TEST_ASSERT},
        {"compiler-methods-range", required_argument, nullptr, OPTION_COMPILER_METHODS_RANGE},
        {"compiler-codegen-options", required_argument, nullptr, OPTION_COMPILER_CODEGEN_OPT},
        {"compiler-opt-escape-analysis", required_argument, nullptr, OPTION_COMPILER_OPT_ESCAPE_ANALYSIS},
        {"compiler-trace-escape-analysis", required_argument, nullptr, OPTION_COMPILER_TRACE_ESCAPE_ANALYSIS},
        {"compiler-opt-induction-variable", required_argument, nullptr, OPTION_COMPILER_OPT_INDUCTION_VARIABLE},
        {"compiler-trace-induction-variable", required_argument, nullptr, OPTION_COMPILER_TRACE_INDUCTION_VARIABLE},
        {"compiler-memory-analysis", required_argument, nullptr, OPTION_COMPILER_MEMORY_ANALYSIS},
        {"compiler-check-pgo-version", required_argument, nullptr, OPTION_COMPILER_CHECK_PGO_VERSION},
        {"compiler-enable-mega-ic", required_argument, nullptr, OPTION_COMPILER_ENABLE_MEGA_IC},
        {"compiler-enable-baselinejit", required_argument, nullptr, OPTION_COMPILER_ENABLE_BASELINEJIT},
        {"compiler-baselinejit-hotness-threshold", required_argument, nullptr,
         OPTION_COMPILER_BASELINEJIT_HOTNESS_THRESHOLD},
        {"compiler-force-baselinejit-compile-main", required_argument, nullptr,
         OPTION_COMPILER_FORCE_BASELINEJIT_COMPILE_MAIN},
        {"compiler-enable-jit-fast-compile", required_argument, nullptr, OPTION_COMPILER_ENABLE_JIT_FAST_COMPILE},
        {"compiler-enable-jitfort", required_argument, nullptr, OPTION_ENABLE_JITFORT},
        {"compiler-codesign-disable", required_argument, nullptr, OPTION_CODESIGN_DISABLE},
        {"compiler-enable-async-copytofort", required_argument, nullptr, OPTION_ENABLE_ASYNC_COPYTOFORT},
        {"compiler-pgo-force-dump", required_argument, nullptr, OPTION_COMPILER_PGO_FORCE_DUMP},
        {"compiler-enable-pgo-space", required_argument, nullptr, OPTION_COMPILER_ENABLE_PGO_SPACE},
        {"async-load-abc", required_argument, nullptr, OPTION_ASYNC_LOAD_ABC},
        {"async-load-abc-test", required_argument, nullptr, OPTION_ASYNC_LOAD_ABC_TEST},
        {"compiler-enable-store-barrier", required_argument, nullptr, OPTION_COMPILER_ENABLE_STORE_BARRIER_OPT},
        {"compiler-enable-ldobjvalue-opt", required_argument, nullptr, OPTION_COMPILER_ENABLE_LDOBJVALUE_OPT},
        {"compiler-enable-concurrent", required_argument, nullptr, OPTION_COMPILER_ENABLE_CONCURRENT},
        {"compiler-opt-frame-state-elimination", required_argument, nullptr,
         OPTION_COMPILER_OPT_FRAME_STATE_ELIMINATION},
        {"enable-inline-property-optimization", required_argument, nullptr, OPTION_ENABLE_INLINE_PROPERTY_OPTIMIZATION},
        {"compiler-enable-aot-code-comment", required_argument, nullptr, OPTION_COMPILER_ENABLE_AOT_CODE_COMMENT},
        {"compiler-enable-aot-lazy-deopt", required_argument, nullptr, OPTION_COMPILER_ENABLE_AOT_LAZY_DEOPT},
        {"compiler-enable-jit-lazy-deopt", required_argument, nullptr, OPTION_COMPILER_ENABLE_JIT_LAZY_DEOPT},
        {"compiler-enable-lazy-deopt-trace", required_argument, nullptr, OPTION_COMPILER_ENABLE_LAZY_DEOPT_TRACE},
        {"compiler-an-file-max-size", required_argument, nullptr, OPTION_COMPILER_AN_FILE_MAX_SIZE},
        {"compiler-trace-builtins", required_argument, nullptr, OPTION_COMPILER_TRACE_BUILTINS},
        {"compile-enable-jit-verify-pass", required_argument, nullptr, OPTION_ENABLE_JIT_VERIFY_PASS},
        {"compiler-jit-method-dichotomy", required_argument, nullptr, OPTION_COMPILER_JIT_METHOD_DICHOTOMY},
        {"compiler-jit-method-path", required_argument, nullptr, OPTION_COMPILER_JIT_METHOD_PATH},
        {"compiler-enable-merge-poly", required_argument, nullptr, OPTION_COMPILER_ENABLE_MERGE_POLY},
        {"mem-config", required_argument, nullptr, OPTION_MEM_CONFIG},
        {"multi-context", required_argument, nullptr, OPTION_MULTI_CONTEXT},
        {nullptr, 0, nullptr, 0},
    };

    int index = 0;
    opterr = 0;
    bool ret = false;
    int option = 0;
    arg_list_t argListStr;
    uint32_t argUint32 = 0;
    uint64_t argUInt64 = 0;
    int argInt = 0;
    bool argBool = false;
    double argDouble = 0.0;
    static std::string COLON = ":";

    if (argc <= 1) {
        return true;
    }

    while (true) {
        option = getopt_long_only(argc, const_cast<char **>(argv), "", longOptions, &index);
        LOG_ECMA(INFO) << "option: " << option << ", optopt: " << optopt << ", optind: " << optind;

        if (optind <= 0 || optind > argc) {
            return false;
        }

        if (option == -1) {
            // set dependency between options at here
            if (!IsEnableElementsKind() && IsEnableMutantArray()) {
                LOG_ECMA(ERROR) << "'enable-mutant-array' must set to false while 'enable-elements-kind' is disabled\n";
                return false;
            }
            return true;
        }

        SetOption(option);
        // unknown option or required_argument option has no argument
        if (option == OPTION_DEFAULT) {
            ret = SetDefaultValue(const_cast<char *>(argv[optind - 1]));
            if (ret) {
                continue;
            } else {
                return ret;
            }
        }

        switch (option) {
            case OPTION_AOT_FILE:
                SetAOTOutputFile(optarg);
                ecmascript::AnFileDataManager::GetInstance()->SetEnable(true);
                break;
            case OPTION_ARK_PROPERTIES:
                ret = ParseIntParam("ark-properties", &argInt);
                if (ret) {
                    SetArkProperties(argInt);
                } else {
                    return false;
                }
                break;
            case OPTION_ARK_BUNDLENAME:
                SetArkBundleName(optarg);
                break;
            case OPTION_ENABLE_ASM_INTERPRETER:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableAsmInterpreter(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ASM_OPCODE_DISABLE_RANGE:
                SetAsmOpcodeDisableRange(optarg);
                break;
            case OPTION_ENABLE_BUILTINS_LAZY:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableBuiltinsLazy(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_LOG_OPT:
                SetCompilerLogOption(optarg);
                break;
            case OPTION_COMPILER_LOG_SNAPSHOT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetCompilerLogSnapshot(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_LOG_TIME:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetCompilerLogTime(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_ARK_TOOLS:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableArkTools(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_OPEN_ARK_TOOLS:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetOpenArkTools(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_PGO_TRACE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetPGOTrace(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_TRACE_BC:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetTraceBc(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_TRACE_DEOPT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetTraceDeopt(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_TRACE_INLINE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetTraceInline(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_TRACE_VALUE_NUMBERING:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetTraceValueNumbering(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_TRACE_INSTRUCTION_COMBINE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetTraceInstructionCombine(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_MAX_INLINE_BYTECODES:
                ret = ParseUint32Param("max-inline-bytecodes", &argUint32);
                if (ret) {
                    SetMaxInlineBytecodes(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_DEOPT_THRESHOLD:
                ret = ParseUint32Param("deopt-threshol", &argUint32);
                if (ret) {
                    SetDeoptThreshold(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_DEVICE_STATE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetDeviceState(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_THERMAL_LEVEL:
                ret = ParseIntParam("compiler-thermal-level", &argInt);
                if (ret) {
                    SetThermalLevel(argInt);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_STRESS_DEOPT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetStressDeopt(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_CODE_PROFILER:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetOptCodeProfiler(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_BC_RANGE:
                SetOptCodeRange(optarg);
                break;
            case OPTION_COMPILER_OPT_BC_RANGE_HELP:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    std::string helpInfo = kungfu::GetHelpForEcmaCodeListForRange();
                    LOG_COMPILER(ERROR) << helpInfo.c_str();
                    exit(1);
                }
                break;
            case OPTION_ENABLE_FORCE_GC:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableForceGC(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_CMC_GC:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableCMCGC(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_CMC_GC_CONCURRENT_ROOT_MARKING:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableCMCGCConcurrentRootMarking(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_LOADING_STUBS_LOG:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableLoadingStubsLog(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_FORCE_FULL_GC:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetForceFullGC(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_FORCE_SHARED_GC_FREQUENCY:
                ret = ParseUint32Param("force-shared-gc-frequency", &argUint32);
                if (ret) {
                    SetForceSharedGCFrequency(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_HEAP_VERIFY:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableHeapVerify(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_IC:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableIC(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_RUNTIME_STAT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableRuntimeStat(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_GC_THREADNUM:
                ret = ParseUint32Param("gc-thread-num", &argUint32);
                if (ret) {
                    SetGcThreadNum(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_HELP:
                return false;
            case OPTION_ICU_DATA_PATH:
                SetIcuDataPath(optarg);
                break;
            case OPTION_ENABLE_WORKER:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetIsWorker(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_LOG_COMPONENTS:
                ParseListArgParam(optarg, &argListStr, COLON);
                SetLogComponents(argListStr);
                break;
            case OPTION_LOG_DEBUG:
                ParseListArgParam(optarg, &argListStr, COLON);
                SetLogDebug(argListStr);
                break;
            case OPTION_LOG_ERROR:
                ParseListArgParam(optarg, &argListStr, COLON);
                SetLogError(argListStr);
                break;
            case OPTION_LOG_FATAL:
                ParseListArgParam(optarg, &argListStr, COLON);
                SetLogFatal(argListStr);
                break;
            case OPTION_LOG_INFO:
                ParseListArgParam(optarg, &argListStr, COLON);
                SetLogInfo(argListStr);
                break;
            case OPTION_LOG_LEVEL:
                SetLogLevel(optarg);
                break;
            case OPTION_LOG_WARNING:
                ParseListArgParam(optarg, &argListStr, COLON);
                SetLogWarning(argListStr);
                break;
            case OPTION_GC_LONG_PAUSED_TIME:
                ret = ParseUint32Param("gc-long-paused-time", &argUint32);
                if (ret) {
                    SetLongPauseTime(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_MAX_METHOD:
                ret = ParseUint32Param("compiler-opt-max-method", &argUint32);
                if (ret) {
                    SetMaxAotMethodSize(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_MODULE_METHODS:
                ret = ParseUint32Param("compiler-module-methods", &argUint32);
                if (!ret) {
                    return false;
                }
                SetCompilerModuleMethods(argUint32);
                break;
            case OPTION_COMPILER_TYPE_THRESHOLD:
                ret = ParseDoubleParam("compiler-type-threshold", &argDouble);
                if (ret) {
                    SetTypeThreshold(argDouble);
                } else {
                    return false;
                }
                break;
            case OPTION_MAX_UNMOVABLE_SPACE:
                ret = ParseUint32Param("max-unmovable-space", &argUint32);
                if (ret) {
                    SetMaxNonmovableSpaceCapacity(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_LOG_METHODS:
                SetMethodsListForLog(optarg);
                break;
            case OPTION_ASM_OPT_LEVEL:
                ret = ParseUint32Param("compiler-opt-level", &argUint32);
                if (ret) {
                    SetOptLevel(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_PRINT_EXECUTE_TIME:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnablePrintExecuteTime(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_PGO_PROFILER:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnablePGOProfiler(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_MUTANT_ARRAY:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableMutantArray(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_ELEMENTS_KIND:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableElementsKind(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_PGO_PROFILER_PATH:
                if (*optarg == '\0') {
                    return false;
                }
                SetPGOProfilerPath(optarg);
                break;
            case OPTION_COMPILER_PGO_HOTNESS_THRESHOLD:
                ret = ParseUint32Param("pgo-hotness-threshold", &argUint32);
                if (ret) {
                    SetPGOHotnessThreshold(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_PGO_SAVE_MIN_INTERVAL:
                ret = ParseUint32Param("compiler-pgo-save-min-interval", &argUint32);
                if (ret) {
                    SetPGOSaveMinInterval(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_BASELINE_PGO:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableBaselinePgo(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_RELOCATION_MODE:
                ret = ParseUint32Param("reloc-mode", &argUint32);
                if (ret) {
                    SetRelocMode(argUint32);
                } else {
                    return false;
                }
                break;
            case OPTION_SERIALIZER_BUFFER_SIZE_LIMIT:
                ret = ParseUint64Param("serializer-buffer-size-limit", &argUInt64);
                if (ret) {
                    SetSerializerBufferSizeLimit(argUInt64);
                } else {
                    return false;
                }
                break;
            case OPTION_STARTUP_TIME:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetStartupTime(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_STUB_FILE:
                SetStubFile(optarg);
                break;
            case OPTION_COMPILER_TARGET_TRIPLE:
                SetTargetTriple(optarg);
                break;
            case OPTION_ENTRY_POINT:
                SetEntryPoint(optarg);
                break;
            case OPTION_MERGE_ABC:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetMergeAbc(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_CONTEXT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableContext(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_TYPE_LOWERING:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableTypeLowering(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_ARRAY_BOUNDS_CHECK_ELIMINATION:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableArrayBoundsCheckElimination(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_FRAME_STATE_ELIMINATION:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableFrameStateElimination(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_EARLY_ELIMINATION:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableEarlyElimination(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_LATER_ELIMINATION:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableLaterElimination(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_INSTRUCTIONE_COMBINE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableInstrcutionCombine(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_STRING:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableOptString(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_VALUE_NUMBERING:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableValueNumbering(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_NEW_VALUE_NUMBERING:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableNewValueNumbering(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_INLINING:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableOptInlining(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_PGOTYPE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableOptPGOType(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_TRACK_FIELD:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableOptTrackField(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_VERIFY_VTABLE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetVerifyVTable(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_SELECT_METHODS:
                SetCompilerSelectMethods(optarg);
                break;
            case OPTION_COMPILER_SKIP_METHODS:
                SetCompilerSkipMethods(optarg);
                break;
            case OPTION_TARGET_COMPILER_MODE:
                SetTargetCompilerMode(optarg);
                break;
            case OPTION_COMPILER_FRAMEWORK_ABC_PATH:
                SetCompilerFrameworkAbcPath(optarg);
                break;
            case OPTION_HAP_PATH:
                SetHapPath(optarg);
                break;
            case OPTION_HAP_ABC_OFFSET:
                ret = ParseUint32Param("hap-abc-offset", &argUint32);
                if (!ret) {
                    return false;
                }
                SetHapAbcOffset(argUint32);
                break;
            case OPTION_HAP_ABC_SIZE:
                ret = ParseUint32Param("hap-abc-size", &argUint32);
                if (!ret) {
                    return false;
                }
                SetHapAbcSize(argUint32);
                break;
            case OPTION_COMPILER_NOCHECK:
                ret = ParseBoolParam(&argBool);
                if (!ret) {
                    return false;
                }
                SetCompilerNoCheck(argBool);
                break;
            case OPTION_COMPILER_PIPELINE_HOST_AOT:
                ret = ParseBoolParam(&argBool);
                if (!ret) {
                    return false;
                }
                SetCompilerPipelineHostAOT(argBool);
                break;
            case OPTION_FAST_AOT_COMPILE_MODE:
                ret = ParseBoolParam(&argBool);
                if (!ret) {
                    return false;
                }
                SetFastAOTCompileMode(argBool);
                break;
            case OPTION_COMPILER_OPT_CONSTANT_FOLDING:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableOptConstantFolding(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_LOOP_PEELING:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableOptLoopPeeling(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_ON_HEAP_CHECK:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableOptOnHeapCheck(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_PKG_INFO:
                SetCompilerPkgJsonInfo(optarg);
                break;
            case OPTION_COMPILER_EXTERNAL_PKG_INFO:
                SetCompilerExternalPkgJsonInfo(optarg);
                break;
            case OPTION_COMPILER_ENABLE_EXTERNAL_PKG:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetCompilerEnableExternalPkg(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_LEXENV_SPECIALIZATION:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableLexenvSpecialization(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_NATIVE_INLINE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableNativeInline(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_LOWERING_BUILTIN:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableLoweringBuiltin(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_LITECG:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetCompilerEnableLiteCG(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_JIT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableJIT(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_DFX_HISYS_EVENT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableDFXHiSysEvent(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_OSR:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableOSR(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_JIT_HOTNESS_THRESHOLD:
                ret = ParseUint32Param("compiler-jit-hotness-threshold", &argUint32);
                if (ret) {
                    uint16_t val = argUint32 > std::numeric_limits<uint16_t>::max() ?
                        std::numeric_limits<uint16_t>::max() : static_cast<uint16_t>(argUint32);
                    SetJitHotnessThreshold(val);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_JIT_CALL_THRESHOLD:
                ret = ParseUint32Param("compiler-jit-call-threshold", &argUint32);
                if (ret) {
                    uint16_t val =  argUint32 > std::numeric_limits<uint16_t>::max() ?
                        std::numeric_limits<uint16_t>::max() : static_cast<uint16_t>(argUint32);
                    SetJitCallThreshold(val);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OSR_HOTNESS_THRESHOLD:
                ret = ParseUint32Param("compiler-osr-hotness-threshold", &argUint32);
                if (ret) {
                    uint16_t val = argUint32 > std::numeric_limits<uint16_t>::max() ?
                        std::numeric_limits<uint16_t>::max() : static_cast<uint16_t>(argUint32);
                    SetOsrHotnessThreshold(val);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_FORCE_JIT_COMPILE_MAIN:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetForceJitCompileMain(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_TRACE_JIT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetTraceJIT(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_TYPED_OP_PROFILER:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetTypedOpProfiler(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_BRANCH_PROFILING:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableBranchProfiling(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_TEST_ASSERT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetTestAssert(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_JIT_PGO:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableJITPGO(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_PROPFILE_DUMP:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableProfileDump(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_AOT_PGO:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableAOTPGO(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_FRAMEWORK_AOT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableFrameworkAOT(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_BASELINEJIT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableBaselineJIT(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_METHODS_RANGE:
                ParseListArgParam(optarg, &argListStr, COLON);
                SetCompilerMethodsRange(&argListStr);
                break;
            case OPTION_COMPILER_CODEGEN_OPT:
                ParseListArgParam(optarg, &argListStr, " ");
                SetCompilerCodegenOptions(argListStr);
                break;
            case OPTION_COMPILER_OPT_ESCAPE_ANALYSIS:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableEscapeAnalysis(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_TRACE_ESCAPE_ANALYSIS:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableTraceEscapeAnalysis(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_OPT_INDUCTION_VARIABLE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableInductionVariableAnalysis(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_TRACE_INDUCTION_VARIABLE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableTraceInductionVariableAnalysis(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_BASELINEJIT_HOTNESS_THRESHOLD:
                ret = ParseUint32Param("compiler-baselinejit-hotness-threshold", &argUint32);
                if (ret) {
                    uint16_t val = argUint32 > std::numeric_limits<uint16_t>::max() ?
                        std::numeric_limits<uint16_t>::max() : static_cast<uint16_t>(argUint32);
                    SetBaselineJitHotnessThreshold(val);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_MEMORY_ANALYSIS:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableMemoryAnalysis(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_CHECK_PGO_VERSION:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetCheckPgoVersion(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_FORCE_BASELINEJIT_COMPILE_MAIN:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetForceBaselineCompileMain(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_JIT_FAST_COMPILE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableJitFastCompile(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_MEGA_IC:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableMegaIC(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ASYNC_LOAD_ABC:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetAsyncLoadAbc(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ASYNC_LOAD_ABC_TEST:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetAsyncLoadAbcTest(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_JITFORT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableJitFort(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_CODESIGN_DISABLE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetDisableCodeSign(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_ASYNC_COPYTOFORT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableAsyncCopyToFort(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_CONCURRENT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetConcurrentCompile(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_STORE_BARRIER_OPT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetStoreBarrierOpt(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_LDOBJVALUE_OPT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetLdObjValueOpt(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_PGO_FORCE_DUMP:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetPgoForceDump(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_PGO_SPACE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetCompilerEnablePgoSpace(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_AOT_CODE_COMMENT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableAotCodeComment(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_INLINE_PROPERTY_OPTIMIZATION:
                 ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableInlinePropertyOptimization(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_ENABLE_JIT_VERIFY_PASS:
                 ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableJitVerifyPass(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_AOT_LAZY_DEOPT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableAotLazyDeopt(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_JIT_LAZY_DEOPT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableJitLazyDeopt(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_LAZY_DEOPT_TRACE:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableLazyDeoptTrace(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_TRACE_BUILTINS:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetTraceBuiltins(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_AN_FILE_MAX_SIZE:
                ret = ParseUint64Param("compiler-an-file-max-size", &argUInt64);
                if (ret) {
                    SetCompilerAnFileMaxByteSize(argUInt64 * 1_MB);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_ENABLE_MERGE_POLY:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetEnableMergePoly(argBool);
                } else {
                    return false;
                }
                break;
            case OPTION_COMPILER_JIT_METHOD_DICHOTOMY:
                SetJitMethodDichotomy(optarg);
                break;
            case OPTION_COMPILER_JIT_METHOD_PATH:
                SetJitMethodPath(optarg);
                break;
            case OPTION_MEM_CONFIG:
                SetMemConfigProperty(optarg);
                break;
            case OPTION_MULTI_CONTEXT:
                ret = ParseBoolParam(&argBool);
                if (ret) {
                    SetMultiContext(argBool);
                } else {
                    return false;
                }
                break;
            default:
                LOG_ECMA(ERROR) << "Invalid option\n";
                return false;
        }
    }
#if defined(PANDA_TARGET_32) || defined(CROSS_PLATFORM)
    SetEnableAotLazyDeopt(false);
    SetEnableJitLazyDeopt(false);
    SetEnableLazyDeoptTrace(false);
#endif
}

bool JSRuntimeOptions::SetDefaultValue(char* argv)
{
    if (optopt == OPTION_DEFAULT) { // unknown option
        LOG_ECMA(ERROR) << " Invalid option \"" << argv << "\"";
        return false;
    }

    if (optopt >= OPTION_LAST) { // unknown argument
        LOG_ECMA(ERROR) << "getopt: \"" << argv <<"\" argument has invalid parameter value \n";
        return false;
    }
    return true;
}

bool JSRuntimeOptions::ParseBool(const std::string &arg, bool* argBool)
{
    if ((strcmp(arg.c_str(), "false") == 0) || (strcmp(arg.c_str(), "0") == 0)) {
        *argBool = false;
    } else {
        *argBool = true;
    }
    return true;
}

bool JSRuntimeOptions::ParseDouble(const std::string &arg, double* argDouble)
{
    *argDouble = std::strtod(arg.c_str(), nullptr);
    if (errno == ERANGE) {
        return false;
    }
    return true;
}

bool JSRuntimeOptions::ParseInt(const std::string &arg, int* argInt)
{
    int64_t val;
    if (StartsWith(arg, "0x")) {
        const int HEX = 16;
        val = std::strtoll(arg.c_str(), nullptr, HEX);
    } else {
        const int DEC = 10;
        val = std::strtoll(arg.c_str(), nullptr, DEC);
    }

    if (errno == ERANGE || val < INT_MIN || val > INT_MAX) {
        return false;
    }
    *argInt = static_cast<int>(val);
    return true;
}

bool JSRuntimeOptions::ParseUint32(const std::string &arg, uint32_t* argUInt32)
{
    if (StartsWith(arg, "0x")) {
        const int HEX = 16;
        *argUInt32 = std::strtoull(arg.c_str(), nullptr, HEX);
    } else {
        const int DEC = 10;
        *argUInt32 = std::strtoull(arg.c_str(), nullptr, DEC);
    }

    if (errno == ERANGE) {
        return false;
    }
    return true;
}

bool JSRuntimeOptions::ParseUint64(const std::string &arg, uint64_t* argUInt64)
{
    if (StartsWith(arg, "0x")) {
        const int HEX = 16;
        *argUInt64 = std::strtoull(arg.c_str(), nullptr, HEX);
    } else {
        const int DEC = 10;
        *argUInt64 = std::strtoull(arg.c_str(), nullptr, DEC);
    }

    if (errno == ERANGE) {
        return false;
    }
    return true;
}

bool JSRuntimeOptions::ParseBoolParam(bool* argBool)
{
    return ParseBool(optarg, argBool);
}

bool JSRuntimeOptions::ParseDoubleParam(const std::string &option, double *argDouble)
{
    bool ret = ParseDouble(optarg, argDouble);
    if (!ret) {
        LOG_ECMA(ERROR) << "getopt: \"" << option << "\" argument has invalid parameter value \"" << optarg <<"\"\n";
    }
    return ret;
}

bool JSRuntimeOptions::ParseIntParam(const std::string &option, int *argInt)
{
    bool ret = ParseInt(optarg, argInt);
    if (!ret) {
        LOG_ECMA(ERROR) << "getopt: \"" << option << "\" argument has invalid parameter value \"" << optarg <<"\"\n";
    }
    return ret;
}

bool JSRuntimeOptions::ParseUint32Param(const std::string &option, uint32_t *argUInt32)
{
    bool ret = ParseUint32(optarg, argUInt32);
    if (!ret) {
        LOG_ECMA(ERROR) << "getopt: \"" << option << "\" argument has invalid parameter value \"" << optarg <<"\"\n";
    }
    return ret;
}

bool JSRuntimeOptions::ParseUint64Param(const std::string &option, uint64_t *argUInt64)
{
    bool ret = ParseUint64(optarg, argUInt64);
    if (!ret) {
        LOG_ECMA(ERROR) << "getopt: \"" << option << "\" argument has invalid parameter value \"" << optarg <<"\"\n";
    }
    return ret;
}

void JSRuntimeOptions::ParseListArgParam(const std::string &option, arg_list_t *argListStr, std::string delimiter)
{
    argListStr->clear();
    std::size_t strIndex = 0;
    std::size_t pos = option.find_first_of(delimiter, strIndex);
    while ((pos < option.size()) && (pos > strIndex)) {
        argListStr->push_back(option.substr(strIndex, pos - strIndex));
        strIndex = pos;
        strIndex = option.find_first_not_of(delimiter, strIndex);
        pos = option.find_first_of(delimiter, strIndex);
    }
    if (pos > strIndex) {
        argListStr->push_back(option.substr(strIndex, pos - strIndex));
    }
    return;
}

void JSRuntimeOptions::BindCPUCoreForTargetCompilation()
{
    if (!deviceIsScreenOff_ || deviceThermalLevel_ > 0) {
        BindSmallCpuCore();
    } else {
        BindMidCpuCore();
    }
}

void JSRuntimeOptions::SetOptionsForTargetCompilation()
{
    if (IsApplicationCompilation()) {
        SetTargetTriple("aarch64-unknown-linux-gnu");
        SetMaxAotMethodSize(MAX_APP_COMPILE_METHOD_SIZE);
        SetEnableOptTrackField(false);
        SetEnableOptInlining(false);
        SetEnableArrayBoundsCheckElimination(false);
        SetCompilerEnableLiteCG(true);
        SetEnableOptPGOType(true);
        if (IsCompilerAnFileMaxByteSizeDefault()) {
            SetCompilerAnFileMaxByteSize(300_MB);
        }
    }

    if (IsTargetCompilerMode()) {
        if (UNLIKELY(IsFullCompilerMode())) {
            SetEnableOptPGOType(false);
            SetPGOProfilerPath("");
        }
        BindCPUCoreForTargetCompilation();
    }

    if (IsCompilerPipelineHostAOT()) {
        SetFastAOTCompileMode(true);
        SetOptLevel(DEFAULT_OPT_LEVEL);
        SetEnableLoweringBuiltin(false);
    }
}

void JSRuntimeOptions::ParseAsmInterOption()
{
    asmInterParsedOption_.enableAsm = enableAsmInterpreter_;
    std::string strAsmOpcodeDisableRange = asmOpcodeDisableRange_;
    if (strAsmOpcodeDisableRange.empty()) {
        return;
    }

    // asm interpreter handle disable range
    size_t pos = strAsmOpcodeDisableRange.find(",");
    if (pos != std::string::npos) {
        std::string strStart = strAsmOpcodeDisableRange.substr(0, pos);
        std::string strEnd = strAsmOpcodeDisableRange.substr(pos + 1);
        int64_t inputStart;
        int64_t inputEnd;
        if (!StringToInt64(strStart, inputStart)) {
            inputStart = 0;
            LOG_ECMA_IF(!strStart.empty(), INFO) << "when get start, strStart is " << strStart;
        }
        if (!StringToInt64(strEnd, inputEnd)) {
            inputEnd = kungfu::BYTECODE_STUB_END_ID;
            LOG_ECMA_IF(!strEnd.empty(), INFO) << "when get end, strEnd is " << strEnd;
        }
        int start = static_cast<int>(inputStart);
        int end = static_cast<int>(inputEnd);
        if (start >= 0 && start < kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS && end >= 0 &&
            end < kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS && start <= end) {
            asmInterParsedOption_.handleStart = start;
            asmInterParsedOption_.handleEnd = end;
        }
    }
}

JSRuntimeOptions::JSRuntimeOptions()
{
    param_ = common::BaseRuntimeParam::DefaultRuntimeParam();
}

void JSRuntimeOptions::SetConfigHeapSize(size_t configHeapSize)
{
    common::BaseRuntimeParam::SetConfigHeapSize(param_, configHeapSize);
}

void JSRuntimeOptions::SetMemConfigProperty(const std::string &configProperty)
{
    if (configProperty == "openArkTools") {
        openArkTools_ = true;
        return;
    }

    ParseRuntimeParam(configProperty);
}

#ifdef ENABLE_CMC_GC
#define PARAM_PAESER_DEFINE(key, subKey, type, minValue, maxValue, defaultValue) \
bool Parse##_##key##_##subKey(const std::string &arg, type *result)              \
{                                                                                \
    if constexpr (std::is_floating_point<type>::value) {                         \
        double floatVal = 0.0;                                                   \
        bool ret = JSRuntimeOptions::ParseDouble(arg, &floatVal);                \
        *result = static_cast<type>(floatVal);                                   \
        return ret;                                                              \
    }                                                                            \
    if constexpr (std::is_integral<type>::value) {                               \
        uint64_t uint64Val = 0;                                                  \
        bool ret = JSRuntimeOptions::ParseUint64(arg, &uint64Val);               \
        *result = static_cast<type>(uint64Val);                                  \
        return ret;                                                              \
    }                                                                            \
    return false;                                                                \
}

RUNTIME_PARAM_LIST(PARAM_PAESER_DEFINE)
#endif

/*
 * key value;key value;...
 */
void JSRuntimeOptions::ParseRuntimeParam(const std::string &str)
{
    if (str.empty()) {
        return;
    }

    size_t length = str.length();
    for (size_t i = 0; i < length;) {
        std::string keyStr;
        std::string valueStr;
        while (i < length && isalpha(str[i])) {
            keyStr += str[i++];
        }
        if (i < length && str[i] == ' ') {
            i++;
        }
        while (i < length && (isdigit(str[i]) || str[i] == '.')) {
            valueStr += str[i++];
        }
        if (i < length && str[i] == ';') {
            i++;
        }
        if (keyStr == "jsHeap") {
            uint32_t input = 0;
            if (!ParseUint32(valueStr, &input)) {
                return;
            }
            heapSize_ = input * 1_MB;
            return;
        }

#ifdef ENABLE_CMC_GC
#define PARSE_RUNTIME_PARAM(key, subKey, type, minValue, maxValue, defaultValue)                            \
        if (keyStr == #subKey) {                                                                            \
            type res = static_cast<type>(defaultValue);                                                     \
            bool ret = Parse##_##key##_##subKey(valueStr, &res);                                            \
            if (!ret) {                                                                                     \
                LOG_ECMA(FATAL) << "Fail parse " << #key << "." << #subKey << " of value: " << valueStr;    \
                UNREACHABLE();                                                                              \
            }                                                                                               \
            if (res < static_cast<type>(minValue) || res > static_cast<type>(maxValue)) {                   \
                LOG_ECMA(FATAL) << "Set " << keyStr << " to " << res << " is out of range: [" <<            \
                    (minValue) << ", " << (maxValue) << "]";                                                \
                UNREACHABLE();                                                                              \
            }                                                                                               \
            param_.key.subKey = res;                                                                        \
            continue;                                                                                       \
        }
        RUNTIME_PARAM_LIST(PARSE_RUNTIME_PARAM)
#undef PARSE_RUNTIME_PARAM

        LOG_ECMA(FATAL) << "Unknow key: " << keyStr << " of value: " << valueStr;
        UNREACHABLE();
#endif
    }
}
#undef PAESER_DEFINE
#undef PARAM_MAP
}
