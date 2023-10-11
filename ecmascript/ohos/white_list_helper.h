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
#include <set>
#include <string>
#include <vector>

#include "ecmascript/log_wrapper.h"
#include "macros.h"

class WhiteListHelper {
public:
    static std::shared_ptr<WhiteListHelper> GetInstance()
    {
        static auto helper = std::make_shared<WhiteListHelper>();
        return helper;
    }

    WhiteListHelper()
    {
        ReadWhiteList();
    }

    ~WhiteListHelper() = default;

    bool IsEnable(const std::string &bundleName, const std::string &moduleName)
    {
        return whiteList_.find(bundleName + ":" + moduleName) != whiteList_.end();
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

    void ReadWhiteList()
    {
        static const std::string WHITE_LIST_NAME = "/etc/ark/app_aot_white_list.conf";
        std::ifstream inputFile(WHITE_LIST_NAME);

        if (!inputFile.is_open()) {
            LOG_ECMA(ERROR) << "bundle white list not exist!";
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
            if (appName.find_first_of('#') == 0) {
                continue;
            }
            whiteList_.insert(appName);
        }
    }
    std::set<std::string> whiteList_;
};

#endif