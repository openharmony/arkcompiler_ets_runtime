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

#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"
#include "macros.h"
namespace panda::ecmascript::ohos {
class EnableAotListHelper {
public:
    static std::shared_ptr<EnableAotListHelper> GetInstance()
    {
        static auto helper = std::make_shared<EnableAotListHelper>(ENABLE_LIST_NAME, DISABLE_LIST_NAME);
        return helper;
    }

    explicit EnableAotListHelper(const std::string &enableListName, const std::string &disableListName)
    {
        ReadEnableAotList(enableListName);
        ReadEnableAotList(disableListName);
    }

    EnableAotListHelper() = default;
    ~EnableAotListHelper() = default;

    bool IsEnableList(const std::string &candidate)
    {
        return enableList_.find(candidate) != enableList_.end();
    }

    bool IsEnableList(const std::string &bundleName, const std::string &moduleName)
    {
        if (IsEnableList(bundleName)) {
            return true;
        }
        return IsEnableList(bundleName + ":" + moduleName);
    }

    bool IsDisableBlackList(const std::string &candidate)
    {
        return disableList_.find(candidate) != disableList_.end();
    }

    bool IsDisableBlackList(const std::string &bundleName, const std::string &moduleName)
    {
        if (IsDisableBlackList(bundleName)) {
            return true;
        }
        return IsDisableBlackList(bundleName + ":" + moduleName);
    }

    void AddEnableListEntry(const std::string &entry)
    {
        enableList_.insert(entry);
    }

    void AddDisableListEntry(const std::string &entry)
    {
        disableList_.insert(entry);
    }

    void Clear()
    {
        disableList_.clear();
        enableList_.clear();
    }

private:
    NO_COPY_SEMANTIC(EnableAotListHelper);
    NO_MOVE_SEMANTIC(EnableAotListHelper);
    static void Trim(std::string &data)
    {
        if (data.empty()) {
            return;
        }
        data.erase(0, data.find_first_not_of(' '));
        data.erase(data.find_last_not_of(' ') + 1);
    }

    void ReadEnableAotList(const std::string &aotListName)
    {
        if (!panda::ecmascript::FileExist(aotListName.c_str())) {
            LOG_ECMA(INFO) << "bundle enable list not exist and will pass by all. file: " << aotListName;
            return;
        }

        std::ifstream inputFile(aotListName);

        if (!inputFile.is_open()) {
            LOG_ECMA(ERROR) << "bundle enable list open failed! file: " << aotListName << ", errno: " << errno;
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
            if (aotListName == ENABLE_LIST_NAME) {
                AddEnableListEntry(appName);
            }
            if (aotListName == DISABLE_LIST_NAME) {
                AddDisableListEntry(appName);
            }
        }
    }
    std::set<std::string> enableList_ {};
    std::set<std::string> disableList_ {};
    static const std::string ENABLE_LIST_NAME;
    static const std::string DISABLE_LIST_NAME;
};
}  // namespace panda::ecmascript::ohos
#endif  // ECMASCRIPT_OHOS_ENABLE_AOT_LIST_HELPER_H

