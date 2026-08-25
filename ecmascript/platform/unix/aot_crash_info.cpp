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

#include "ecmascript/ohos/ohos_constants.h"
#include "ecmascript/ohos/aot_runtime_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/platform/aot_crash_info.h"
#include "ecmascript/platform/file.h"
#if (defined(PANDA_TARGET_OHOS) && !defined(STANDALONE_MODE)) || defined(ENABLE_OHOS_PARAMETER)
#include "parameters.h"
#endif

namespace panda::ecmascript {
namespace {
constexpr const char *const AOT_BUILD_COUNT_DISABLE = "ark.aot.build.count.disable";

bool GetAotBuildCountDisable()
{
#ifdef ENABLE_OHOS_PARAMETER
    return OHOS::system::GetBoolParameter(AOT_BUILD_COUNT_DISABLE, false);
#endif
    return false;
}

bool IsAotCompileSuccessOnce(const std::string &pgoRealPath)
{
    if (GetAotBuildCountDisable()) {
        return false;
    }
    int count = ohos::AotRuntimeInfo::GetInstance().GetCompileCountByType(
        ohos::RuntimeInfoType::AOT_BUILD, pgoRealPath);
    return count > 0;
}
}  // namespace

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
    if (IsAotCompileSuccessOnce(pgoRealPath)) {
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
#ifdef ENABLE_OHOS_PARAMETER
    if (ohos::EnableAotJitListHelper::GetInstance()->IsEnableAot(bundleName)) {
        options->SetEnablePGOProfiler(true);
        if (options->GetAOTHasException() || ecmascript::AnFileDataManager::GetInstance()->IsEnable()
            || IsAotEscaped()) {
            options->SetEnablePGOProfiler(false);
            pgo::PGOProfilerManager::GetInstance()->SetDisablePGO(true);
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
    std::string filePath = std::string(ohos::OhosConstants::SANDBOX_ARK_PROFILE_PATH)
        + std::string(ohos::OhosConstants::PATH_SEPARATOR)
        + std::string(ohos::OhosConstants::AOT_RUNTIME_INFO_NAME);
    return FileExist(filePath.c_str());
}

bool AotCrashInfo::IsJitEscape()
{
    std::string filePath = std::string(ohos::OhosConstants::SANDBOX_ARK_PROFILE_PATH)
        + std::string(ohos::OhosConstants::PATH_SEPARATOR)
        + std::string(ohos::OhosConstants::AOT_RUNTIME_INFO_NAME);
    return FileExist(filePath.c_str());
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
