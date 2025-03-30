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

#ifndef ECMASCRIPT_PLATFORM_AOT_RUNTIME_INFO_H
#define ECMASCRIPT_PLATFORM_AOT_RUNTIME_INFO_H

#include <signal.h>
#include <string>

#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/dfx/stackinfo/js_stackinfo.h"
#include "ecmascript/ohos/ohos_pkg_args.h"
#include "ecmascript/ecma_vm.h"
#include "llvm/BinaryFormat/ELF.h"

namespace panda::ecmascript {
#ifdef JIT_ESCAPE_ENABLE
void GetSignalHandler(int signal, siginfo_t *info, void *context);
void SignalReg(int signo);
#endif
void SignalAllReg();

using AotCompilerPreprocessor = kungfu::AotCompilerPreprocessor;
class AotCrashInfo {
constexpr static const char *const AOT_ESCAPE_DISABLE = "ark.aot.escape.disable";
constexpr static int AOT_CRASH_COUNT = 1;
constexpr static int OTHERS_CRASH_COUNT = 3;
constexpr static int JIT_CRASH_COUNT = 1;
constexpr static int JS_CRASH_COUNT = 3;
constexpr static int OPT_CODE_CRASH_THRESHOLD = 1;
public:
    explicit AotCrashInfo() = default;
    virtual ~AotCrashInfo() = default;

    static AotCrashInfo &GetInstance()
    {
        static AotCrashInfo singleAotCrashInfo;
        return singleAotCrashInfo;
    }

    bool IsAotEscapedOrNotInEnableList(EcmaVM *vm, const std::string &bundleName) const;
    void SetOptionPGOProfiler(JSRuntimeOptions *options, const std::string &bundleName) const;
    bool PUBLIC_API IsAotEscapedOrCompiledOnce(AotCompilerPreprocessor &cPreprocessor, int32_t &ret) const;
    static bool PUBLIC_API IsAotEscaped(const std::string &pgoRealPath = "");
    static bool IsJitEscape();
    static bool GetAotEscapeDisable();
    static std::string GetSandBoxPath();
    static int GetAotCrashCount();
    static int GetJitCrashCount();
    static int GetJsCrashCount();
    static int GetOthersCrashCount();
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_AOT_RUNTIME_INFO_H
