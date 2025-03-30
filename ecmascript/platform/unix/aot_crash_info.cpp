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
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
#include "parameters.h"
#endif

namespace panda::ecmascript {
#ifdef JIT_ESCAPE_ENABLE
static struct sigaction s_oldSa[SIGSYS + 1]; // SIGSYS = 31
void GetSignalHandler(int signal, siginfo_t *info, void *context)
{
    [[maybe_unused]] ucontext_t *ucontext = reinterpret_cast<ucontext_t*>(context);
    [[maybe_unused]] mcontext_t &mcontext = ucontext->uc_mcontext;
    uintptr_t pc = 0;
#if defined(PANDA_TARGET_AMD64)
    pc = static_cast<uintptr_t>(mcontext.gregs[REG_RIP]);
#elif defined(PANDA_TARGET_ARM64)
    pc = static_cast<uintptr_t>(mcontext.pc);
#endif
    if (JsStackInfo::loader == nullptr) {
        ecmascript::JsStackInfo::BuildCrashInfo(false);
    } else if (!JsStackInfo::loader->InsideStub(pc) && !JsStackInfo::loader->InsideAOT(pc)) {
        ecmascript::JsStackInfo::BuildCrashInfo(false);
    } else {
        ecmascript::JsStackInfo::BuildCrashInfo(false, pc);
    }
    sigaction(signal, &s_oldSa[signal], nullptr);
    int rc = syscall(SYS_rt_tgsigqueueinfo, getpid(), syscall(SYS_gettid), info->si_signo, info);
    if (rc != 0) {
        LOG_ECMA(ERROR) << "GetSignalHandler() failed to resend signal during crash";
    }
}

void SignalReg(int signo)
{
    sigaction(signo, nullptr, &s_oldSa[signo]);
    struct sigaction newAction;
    newAction.sa_flags = SA_RESTART | SA_SIGINFO;
    newAction.sa_sigaction = GetSignalHandler;
    sigaction(signo, &newAction, nullptr);
}
#endif

void SignalAllReg()
{
#ifdef JIT_ESCAPE_ENABLE
    SignalReg(SIGABRT);
    SignalReg(SIGBUS);
    SignalReg(SIGSEGV);
    SignalReg(SIGILL);
    SignalReg(SIGKILL);
    SignalReg(SIGSTKFLT);
    SignalReg(SIGFPE);
    SignalReg(SIGTRAP);
#endif
}

bool AotCrashInfo::IsAotEscapedOrNotInEnableList(EcmaVM *vm, const std::string &bundleName) const
{
    if (!vm->GetJSOptions().WasAOTOutputFileSet() &&
        !ohos::EnableAotJitListHelper::GetInstance()->IsEnableAot(bundleName)) {
        LOG_ECMA(INFO) << "Stop load AOT because it's not in enable list";
        return true;
    }
    if (IsAotEscaped()) {
        LOG_ECMA(INFO) << "Stop load AOT because there are more crashes";
        return true;
    }
    return false;
}

bool AotCrashInfo::IsAotEscapedOrCompiledOnce(AotCompilerPreprocessor &cPreprocessor, int32_t &ret) const
{
    if (!cPreprocessor.GetMainPkgArgs()) {
        return false;
    }
    std::string pgoRealPath = cPreprocessor.GetMainPkgArgs()->GetPgoDir();
    pgoRealPath.append(ohos::OhosConstants::PATH_SEPARATOR);
    pgoRealPath.append(ohos::OhosConstants::AOT_RUNTIME_INFO_NAME);
    if (ohos::EnableAotJitListHelper::GetInstance()->IsAotCompileSuccessOnce(pgoRealPath)) {
        ret = 0;
        LOG_ECMA(INFO) << "Aot has compile success once or escaped.";
        return true;
    }
    if (IsAotEscaped(pgoRealPath)) {
        ret = -1;
        LOG_ECMA(INFO) << "Aot has escaped";
        return true;
    }
    return false;
}

void AotCrashInfo::SetOptionPGOProfiler(JSRuntimeOptions *options, const std::string &bundleName) const
{
#ifdef AOT_ESCAPE_ENABLE
    if (ohos::EnableAotJitListHelper::GetInstance()->IsEnableAot(bundleName)) {
        options->SetEnablePGOProfiler(true);
        if (options->GetAOTHasException() || ecmascript::AnFileDataManager::GetInstance()->IsEnable()) {
            options->SetEnablePGOProfiler(false);
            LOG_ECMA(INFO) << "Aot has compile success once or escaped.";
        }
    }
#endif
    (void)options;
    (void)bundleName;
}

bool AotCrashInfo::IsAotEscaped(const std::string &pgoRealPath)
{
    if (AotCrashInfo::GetAotEscapeDisable()) {
        return false;
    }
    auto escapeMap = ohos::AotRuntimeInfo::GetInstance().CollectCrashSum(pgoRealPath);
    int totalCrashes = escapeMap[ohos::RuntimeInfoType::AOT_CRASH] +
                       escapeMap[ohos::RuntimeInfoType::JIT] +
                       escapeMap[ohos::RuntimeInfoType::OTHERS] +
                       escapeMap[ohos::RuntimeInfoType::JS];
    return totalCrashes >= OPT_CODE_CRASH_THRESHOLD;
}

bool AotCrashInfo::IsJitEscape()
{
    auto escapeMap = ohos::AotRuntimeInfo::GetInstance().CollectCrashSum();
    int totalCrashes = escapeMap[ohos::RuntimeInfoType::AOT_CRASH] +
                       escapeMap[ohos::RuntimeInfoType::JIT] +
                       escapeMap[ohos::RuntimeInfoType::OTHERS] +
                       escapeMap[ohos::RuntimeInfoType::JS];
    return totalCrashes >= OPT_CODE_CRASH_THRESHOLD;
}

bool AotCrashInfo::GetAotEscapeDisable()
{
#if defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)
        return OHOS::system::GetBoolParameter(AOT_ESCAPE_DISABLE, false);
#endif
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
