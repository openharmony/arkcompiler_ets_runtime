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

#include "libpandabase/macros.h"

namespace panda::ecmascript::ohos {
class EnableAotJitListHelper {
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

    bool IsEnableAot(const std::string &candidate) const
    {
        return (enableList_.find(candidate) != enableList_.end()) ||
               (enableList_.find(candidate + ":aot") != enableList_.end());
    }

    bool IsEnableJit(const std::string &candidate) const
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
        std::ifstream inputFile(aotJitListName);
        if (!inputFile.is_open()) {
            return;
        }
        std::string line;
        while (getline(inputFile, line)) {
            auto appName = line;
            Trim(appName);
            if (appName.empty() || appName.at(0) == '#') {
                continue;
            }
            RemoveComment(appName);
            AddEnableListEntry(appName);
        }
    }
    std::set<std::string> enableList_ {};
    static inline const std::string ENABLE_LIST_NAME = "/etc/ark/app_aot_jit_enable_list.conf";
};
}  // namespace panda::ecmascript::ohos
#endif  // ECMASCRIPT_OHOS_ENABLE_AOT_LIST_HELPER_H
