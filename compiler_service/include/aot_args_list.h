/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_ARKCOMPILER_AOT_ARGS_LIST_H
#define OHOS_ARKCOMPILER_AOT_ARGS_LIST_H

#include <string>
#include <unordered_set>

namespace OHOS::ArkCompiler {

constexpr const char* AOT_EXE = "/system/bin/ark_aot_compiler";
constexpr const char* STATIC_AOT_EXE = "/system/bin/ark_aot";

std::unordered_set<std::string> AotArgsList {
    "aot-file",
    "ark-properties",
    "ark-bundleName",
    "asm-interpreter",
    "asm-opcode-disable-range",
    "builtins-lazy",
    "compiler-log",
    "compiler-log-methods",
    "compiler-log-snapshot",
    "compiler-log-time",
    "compiler-type-threshold",
    "enable-ark-tools",
    "compiler-trace-bc",
    "compiler-trace-deopt",
    "compiler-trace-inline",
    "compiler-trace-value-numbering",
    "compiler-trace-instruction-combine",
    "compiler-max-inline-bytecodes",
    "compiler-deopt-threshold",
    "compiler-device-state",
    "compiler-thermal-level",
    "compiler-stress-deopt",
    "compiler-opt-code-profiler",
    "compiler-opt-bc-range",
    "compiler-opt-bc-range-help",
    "enable-force-gc",
    "enable-ic",
    "enable-runtime-stat",
    "compiler-opt-constant-folding",
    "compiler-opt-array-bounds-check-elimination",
    "compiler-opt-type-lowering",
    "compiler-opt-early-elimination",
    "compiler-opt-later-elimination",
    "compiler-opt-instr-combine",
    "compiler-opt-string",
    "compiler-opt-value-numbering",
    "compiler-opt-new-value-numbering",
    "compiler-opt-inlining",
    "compiler-opt-pgotype",
    "compiler-opt-track-field",
    "entry-point",
    "force-full-gc",
    "force-shared-gc-frequency",
    "gc-thread-num",
    "heap-size-limit",
    "icu-data-path",
    "enable-worker",
    "log-components",
    "log-debug",
    "log-error",
    "log-fatal",
    "log-info",
    "log-level",
    "log-warning",
    "gc-long-paused-time",
    "compiler-opt-max-method",
    "compiler-module-methods",
    "max-unmovable-space",
    "merge-abc",
    "enable-context",
    "compiler-opt-level",
    "reloc-mode",
    "serializer-buffer-size-limit",
    "startup-time",
    "stub-file",
    "compiler-target-triple",
    "enable-print-execute-time",
    "enable-pgo-profiler",
    "enable-mutant-array",
    "enable-elements-kind",
    "compiler-pgo-profiler-path",
    "compiler-pgo-hotness-threshold",
    "compiler-pgo-save-min-interval",
    "compiler-verify-vtable",
    "compiler-select-methods",
    "compiler-skip-methods",
    "target-compiler-mode",
    "hap-path",
    "hap-abc-offset",
    "hap-abc-size",
    "compiler-no-check",
    "compiler-pipeline-host-aot",
    "compiler-fast-compile",
    "compiler-opt-loop-peeling",
    "compiler-opt-array-onheap-check",
    "compiler-pkg-info",
    "compiler-external-pkg-info",
    "compiler-enable-external-pkg",
    "compiler-framework-abc-path",
    "compiler-enable-lexenv-specialization",
    "compiler-enable-native-inline",
    "compiler-enable-lowering-builtin",
    "compiler-enable-litecg",
    "compiler-enable-jit",
    "compiler-enable-osr",
    "compiler-trace-jit",
    "compiler-jit-hotness-threshold",
    "compiler-osr-hotness-threshold",
    "compiler-force-jit-compile-main",
    "compiler-enable-jit-pgo",
    "compiler-enable-aot-pgo",
    "compiler-enable-framework-aot",
    "compiler-enable-profile-dump",
    "compiler-typed-op-profiler",
    "compiler-opt-branch-profiling",
    "test-assert",
    "compiler-methods-range",
    "compiler-codegen-options",
    "compiler-opt-escape-analysis",
    "compiler-trace-escape-analysis",
    "compiler-opt-induction-variable",
    "compiler-trace-induction-variable",
    "compiler-memory-analysis",
    "compiler-check-pgo-version",
    "compiler-enable-baselinejit",
    "compiler-baselinejit-hotness-threshold",
    "compiler-force-baselinejit-compile-main",
    "compiler-baseline-pgo",
    "compiler-trace-builtins",
};

std::unordered_set<std::string> StaticAotArgsList {
    "boot-panda-files",
    "paoc-panda-files",
    "paoc-output",
    "load-runtimes",
    "paoc-use-cha",
};

std::vector<std::string> StaticAotDefaultArgs {
    "--load-runtimes=ets",
    "--compiler-enable-fast-interop=false",
    "--paoc-zip-panda-file=ets/modules_static.abc"
};
} // namespace OHOS::ArkCompiler
#endif // OHOS_ARKCOMPILER_AOT_ARGS_LIST_H
