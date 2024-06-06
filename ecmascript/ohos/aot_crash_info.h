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

#ifndef ECMASCRIPT_COMPILER_OHOS_AOT_CRASH_INFO_H
#define ECMASCRIPT_COMPILER_OHOS_AOT_CRASH_INFO_H

#include "ohos_constants.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ohos/aot_runtime_info.h"
#include "ecmascript/ohos/enable_aot_list_helper.h"
#ifdef AOT_ESCAPE_ENABLE
#include "parameters.h"
#endif

namespace panda::ecmascript::ohos {
class AotCrashInfo {
    constexpr static const char *const SPLIT_STR = "|";
    constexpr static const char *const AOT_ESCAPE_DISABLE = "ark.aot.escape.disable";
    constexpr static int AOT_CRASH_COUNT = 1;
    constexpr static int OTHERS_CRASH_COUNT = 3;
    constexpr static int CRASH_LOG_SIZE = 3;
    constexpr static int JIT_CRASH_COUNT = 1;
    constexpr static int JS_CRASH_COUNT = 3;
public:
    
    explicit AotCrashInfo() = default;
    virtual ~AotCrashInfo() = default;

    static AotCrashInfo &GetInstance()
    {
        static AotCrashInfo singleAotCrashInfo;
        return singleAotCrashInfo;
    }

    bool IsAotEscapeOrNotInEnableList(EcmaVM *vm, const std::string &bundleName) const
    {
        if (!vm->GetJSOptions().WasAOTOutputFileSet() &&
            !EnableAotJitListHelper::GetInstance()->IsEnableAot(bundleName)) {
            LOG_ECMA(INFO) << "Stop load AOT because it's not in enable list";
            return true;
        }
        if (IsAotEscape()) {
            LOG_ECMA(INFO) << "Stop load AOT because there are more crashes";
            return true;
        }
        return false;
    }

    bool IsAotEscapeOrCompileOnce(const std::string &pgoDir, int32_t &ret)
    {
        if (ohos::AotCrashInfo::IsAotEscape(pgoDir)) {
            LOG_COMPILER(ERROR) << "Stop compile AOT because there are multiple crashes";
            ret = -1;
            return true;
        }
        if (ohos::EnableAotJitListHelper::GetInstance()->IsAotCompileSuccessOnce(pgoDir)) {
            LOG_ECMA(ERROR) << "Aot has compile success once.";
            ret = 0;
            return true;
        }
        return false;
    }

    void SetOptionPGOProfiler(JSRuntimeOptions *options, const std::string &bundleName) const
    {
        if (EnableAotJitListHelper::GetInstance()->IsEnableAot(bundleName)) {
            options->SetEnablePGOProfiler(true);
        }
        if (EnableAotJitListHelper::GetInstance()->IsAotCompileSuccessOnce()) {
            options->SetEnablePGOProfiler(false);
            LOG_ECMA(INFO) << "Aot has compile success once.";
        }
        if (IsAotEscape()) {
            options->SetEnablePGOProfiler(false);
            LOG_ECMA(INFO) << "Aot has escaped.";
        }
    }

    static bool IsAotEscape(const std::string &pgoRealPath = "")
    {
        if (GetAotEscapeDisable()) {
            return false;
        }
        auto escapeMap = AotRuntimeInfo::GetInstance().CollectCrashSum(pgoRealPath);
        return escapeMap[RuntimeInfoType::AOT] >= GetAotCrashCount() ||
            escapeMap[RuntimeInfoType::OTHERS] >= GetOthersCrashCount() ||
            escapeMap[RuntimeInfoType::JS] >= GetJsCrashCount();
    }

    static bool IsJitEscape()
    {
        auto escapeMap = AotRuntimeInfo::GetInstance().CollectCrashSum();
        return escapeMap[RuntimeInfoType::JIT] >= GetJitCrashCount() ||
            escapeMap[RuntimeInfoType::AOT] >= GetAotCrashCount() ||
            escapeMap[RuntimeInfoType::OTHERS] >= GetOthersCrashCount() ||
            escapeMap[RuntimeInfoType::JS] >= GetJsCrashCount();
    }

    static bool GetAotEscapeDisable()
    {
#ifdef AOT_ESCAPE_ENABLE
        return OHOS::system::GetBoolParameter(AOT_ESCAPE_DISABLE, false);
#endif
        return false;
    }

    static std::string GetSandBoxPath()
    {
        return OhosConstants::SANDBOX_ARK_PROFILE_PATH;
    }

    static int GetAotCrashCount()
    {
        return AOT_CRASH_COUNT;
    }

    static int GetJitCrashCount()
    {
        return JIT_CRASH_COUNT;
    }

    static int GetJsCrashCount()
    {
        return JS_CRASH_COUNT;
    }

    static int GetOthersCrashCount()
    {
        return OTHERS_CRASH_COUNT;
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_OHOS_AOT_CRASH_INFO_H
