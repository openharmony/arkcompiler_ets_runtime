/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_ARKCOMPILER_AOTCOMPILER_CONSTANTS_H
#define OHOS_ARKCOMPILER_AOTCOMPILER_CONSTANTS_H

#include <string>
#include <unordered_set>

namespace OHOS::ArkCompiler {
namespace Cmds {
constexpr const char* ARK_AOT_COMPILER = "/system/bin/ark_aot_compiler";
} // namespace Cmds

namespace ArgsIdx {
const std::string BUNDLE_UID = "BundleUid";
const std::string BUNDLE_GID = "BundleGid";
const std::string AN_FILE_NAME = "anFileName";
const std::string APP_SIGNATURE = "appIdentifier";
const std::string ABC_PATH = "ABC-Path";
} // namespace ArgsIdx

namespace Symbols {
constexpr const char* PREFIX = "--";
constexpr const char* EQ = "=";
} // namespace Symbols

std::unordered_set<std::string> AotArgsSet {
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
};

enum class ErrOfCompile {
    COMPILE_OK = (0),
    ERR_COMPILE_FAIL = (-1),
    ERR_COMPILE_HELP = (1),
    COMPILE_NO_AP = (2),
};
} // namespace OHOS::ArkCompiler
#endif  // OHOS_ARKCOMPILER_AOTCOMPILER_CONSTANTS_H