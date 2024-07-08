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

#include "ecmascript/platform/aot_crash_info.h"

namespace panda::ecmascript {
#ifdef JIT_ESCAPE_ENABLE
void GetSignalHandler(int signal, siginfo_t *info, void *context)
{
    (void)signal;
    (void)info;
    (void)context;
}

void SignalReg(int signo)
{
    (void)signo;
}
#endif

void SignalAllReg()
{
}

bool AotCrashInfo::IsAotEscapedOrNotInEnableList(EcmaVM *vm, const std::string &bundleName) const
{
    (void)vm;
    (void)bundleName;
    return false;
}

bool AotCrashInfo::IsAotEscapedOrCompiledOnce(AotCompilerPreprocessor &cPreprocessor, int32_t &ret) const
{
    (void)cPreprocessor;
    (void)ret;
    return false;
}

void AotCrashInfo::SetOptionPGOProfiler(JSRuntimeOptions *options, const std::string &bundleName) const
{
    (void)options;
    (void)bundleName;
}

bool AotCrashInfo::IsAotEscaped(const std::string &pgoRealPath)
{
    (void)pgoRealPath;
    return false;
}

bool AotCrashInfo::IsJitEscape()
{
    return false;
}

bool AotCrashInfo::GetAotEscapeDisable()
{
    return false;
}

std::string AotCrashInfo::GetSandBoxPath()
{
    return ohos::OhosConstants::SANDBOX_ARK_PROFILE_PATH;
}

int AotCrashInfo::GetAotCrashCount()
{
    return AOT_CRASH_COUNT;
}

int AotCrashInfo::GetJitCrashCount()
{
    return JIT_CRASH_COUNT;
}

int AotCrashInfo::GetJsCrashCount()
{
    return JS_CRASH_COUNT;
}

int AotCrashInfo::GetOthersCrashCount()
{
    return OTHERS_CRASH_COUNT;
}
}  // namespace panda::ecmascript