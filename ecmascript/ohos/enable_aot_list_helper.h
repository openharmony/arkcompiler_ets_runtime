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

#ifndef ECMASCRIPT_OHOS_ENABLE_AOT_LIST_HELPER_H
#define ECMASCRIPT_OHOS_ENABLE_AOT_LIST_HELPER_H

#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ohos_constants.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/ohos/aot_runtime_info.h"
#include "macros.h"

#ifdef AOT_ESCAPE_ENABLE
#include "parameters.h"
#endif
namespace panda::ecmascript::ohos {
class EnableAotJitListHelper {
constexpr static const char *const AOT_BUILD_COUNT_DISABLE = "ark.aot.build.count.disable";
constexpr static const char *const ARK_PROFILE = "ark.profile";
public:
    static std::shared_ptr<EnableAotJitListHelper> GetInstance()
    {
        static auto helper = std::make_shared<EnableAotJitListHelper>(ENABLE_LIST_NAME);
        return helper;
    }

    explicit EnableAotJitListHelper(const std::string &enableListName)
    {
        ReadEnableList(enableListName);
    }

    EnableAotJitListHelper() = default;
    virtual ~EnableAotJitListHelper() = default;

    bool IsEnableAot(const std::string &candidate)
    {
        // The enable logic is not only controlled by the whitelist, but also by the ark.profile switch
        if (IsEnabledByArkProfiler()) {
            return true;
        }
        return (enableList_.find(candidate) != enableList_.end()) ||
               (enableList_.find(candidate + ":aot") != enableList_.end());
    }

    bool IsEnableJit(const std::string &candidate)
    {
        return (enableList_.find(candidate) != enableList_.end()) ||
               (enableList_.find(candidate + ":jit") != enableList_.end());
    }

    void AddEnableListEntry(const std::string &entry)
    {
        enableList_.insert(entry);
    }

    void Clear()
    {
        enableList_.clear();
    }

    virtual bool IsEnabledByArkProfiler() const
    {
#ifdef AOT_ESCAPE_ENABLE
        return OHOS::system::GetBoolParameter(ARK_PROFILE, false);
#endif
        return false;
    }

    static bool GetAotBuildCountDisable()
    {
#ifdef AOT_ESCAPE_ENABLE
        return OHOS::system::GetBoolParameter(AOT_BUILD_COUNT_DISABLE, false);
#endif
        return false;
    }

    void AddEnableListCount(bool isCompileSuccess, const std::string &pgoPath = "") const
    {
        if (!isCompileSuccess) {
            return;
        }
        std::string runtimePgoRealPath = pgoPath;
        runtimePgoRealPath.append(ohos::OhosConstants::PATH_SEPARATOR);
        runtimePgoRealPath.append(ohos::OhosConstants::AOT_RUNTIME_INFO_NAME);
        ohos::AotRuntimeInfo::GetInstance().BuildCompileRuntimeInfo(ohos::RuntimeInfoType::AOT_BUILD,
            runtimePgoRealPath);
    }

    bool IsAotCompileSuccessOnce(const std::string &pgoRealPath = "") const
    {
        if (GetAotBuildCountDisable()) {
            return false;
        }
        int count = ohos::AotRuntimeInfo::GetInstance().GetCompileCountByType(
            ohos::RuntimeInfoType::AOT_BUILD, pgoRealPath);
        if (count > 0) {
            return true;
        }
        return false;
    }

private:
    NO_COPY_SEMANTIC(EnableAotJitListHelper);
    NO_MOVE_SEMANTIC(EnableAotJitListHelper);

    static void Trim(std::string &data)
    {
        if (data.empty()) {
            return;
        }
        data.erase(0, data.find_first_not_of(' '));
        data.erase(data.find_last_not_of(' ') + 1);
    }

    static void RemoveComment(std::string &data)
    {
        size_t hashPos = data.find('#');
        if (hashPos != std::string::npos) {
            data = data.substr(0, hashPos);
        }
        size_t lastNonSpace = data.find_last_not_of(' ');
        if (lastNonSpace != std::string::npos) {
            data.erase(lastNonSpace + 1);
        }
    }

    void ReadEnableList(const std::string &aotJitListName)
    {
        std::string realPath;
        if (!panda::ecmascript::RealPath(aotJitListName, realPath, false)) {
            LOG_ECMA(DEBUG) << "Fail to get realPath: " << aotJitListName;
            return;
        }
        if (!panda::ecmascript::FileExist(realPath.c_str())) {
            LOG_ECMA(DEBUG) << "bundle enable list not exist and will pass by all. file: " << realPath;
            return;
        }

        std::ifstream inputFile(realPath);

        if (!inputFile.is_open()) {
            LOG_ECMA(ERROR) << "bundle enable list open failed! file: " << aotJitListName << ", errno: " << errno;
            return;
        }

        std::string line;
        while (getline(inputFile, line)) {
            auto appName = line;
            Trim(appName);
            // skip empty line
            if (appName.empty()) {
                continue;
            }
            // skip comment line
            if (appName.at(0) == '#') {
                continue;
            }
            // remove the comment
            RemoveComment(appName);
            AddEnableListEntry(appName);
        }
    }
    std::set<std::string> enableList_ {};
    PUBLIC_API static const std::string ENABLE_LIST_NAME;
};
}  // namespace panda::ecmascript::ohos
#endif  // ECMASCRIPT_OHOS_ENABLE_AOT_LIST_HELPER_H
