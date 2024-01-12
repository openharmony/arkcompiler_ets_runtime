/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_OHOS_WHITE_LIST_HELPER_H
#define ECMASCRIPT_OHOS_WHITE_LIST_HELPER_H

#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"
#include "macros.h"

class WhiteListHelper {
public:
    static std::shared_ptr<WhiteListHelper> GetInstance()
    {
        static const std::string WHITE_LIST_NAME = "/etc/ark/app_aot_white_list.conf";
        static auto helper = std::make_shared<WhiteListHelper>(WHITE_LIST_NAME);
        return helper;
    }

    explicit WhiteListHelper(const std::string &whiteListName)
    {
        ReadWhiteList(whiteListName);
    }

    ~WhiteListHelper() = default;

    bool IsEnable(const std::string &candidate)
    {
        return passBy_ || whiteList_.find(candidate) == whiteList_.end();
    }

    bool IsEnable(const std::string &bundleName, const std::string &moduleName)
    {
        if (!IsEnable(bundleName)) {
            return false;
        }
        return IsEnable(bundleName + ":" + moduleName);
    }

    void AddWhiteListEntry(const std::string &entry)
    {
        whiteList_.insert(entry);
    }

    void Clear()
    {
        passBy_ = false;
        whiteList_.clear();
    }

private:
    NO_COPY_SEMANTIC(WhiteListHelper);
    NO_MOVE_SEMANTIC(WhiteListHelper);
    static void Trim(std::string &data)
    {
        if (data.empty()) {
            return;
        }
        data.erase(0, data.find_first_not_of(' '));
        data.erase(data.find_last_not_of(' ') + 1);
    }

    void ReadWhiteList(const std::string &whiteListName)
    {
        if (!panda::ecmascript::FileExist(whiteListName.c_str())) {
            LOG_ECMA(INFO) << "bundle white list not exist and will pass by all. file: " << whiteListName;
            passBy_ = true;
            return;
        }

        std::ifstream inputFile(whiteListName);

        if (!inputFile.is_open()) {
            LOG_ECMA(ERROR) << "bundle white list open failed! file: " << whiteListName << ", errno: " << errno;
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
            AddWhiteListEntry(appName);
        }
    }
    bool passBy_ {false};
    std::set<std::string> whiteList_ {};
};

#endif