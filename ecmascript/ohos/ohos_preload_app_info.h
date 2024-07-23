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

#ifndef ECMASCRIPT_OHOS_PRELOAD_APP_INFO_H
#define ECMASCRIPT_OHOS_PRELOAD_APP_INFO_H

#include <string>
#include <set>
#include <vector>

#include "ecmascript/platform/file.h"
#include "macros.h"

namespace panda::ecmascript {
class OhosPreloadAppInfo {
public:
    constexpr static const char *const PRELOAD_PATH_PREFIX = "/system/";
    constexpr static const char *const PRELOAD_AN_FOLDER = "/ark-cache/";

    OhosPreloadAppInfo() = default;

    static std::string GetPreloadAOTFileName(const std::string &hapPath, const std::string &moduleName)
    {
        std::string aotFileName = "";
        std::string appName = "";
        std::string folderPath = "";
        std::string appPath = "";

        size_t folderEnd = 0;
        size_t appEnd = 0;
        if (hapPath.find(PRELOAD_PATH_PREFIX) != std::string::npos) {
            folderEnd = hapPath.find_last_of('/');
        }
        if (folderEnd != std::string::npos) {
            // "/system/app/AppName"
            folderPath = hapPath.substr(0, folderEnd);
            appEnd = folderPath.find_last_of('/');
            // "AppName"
            appName = folderPath.substr(appEnd + 1);
        }
        if (appEnd != std::string::npos) {
            // "/system/app"
            appPath = hapPath.substr(0, appEnd);
            // "/system/app/ark-cache/AppName/ModuleName"
            aotFileName = appPath + PRELOAD_AN_FOLDER + appName + "/" + moduleName;
        }
        std::string anFile = aotFileName + AOTFileManager::FILE_EXTENSION_AN;
        std::string aiFile = aotFileName + AOTFileManager::FILE_EXTENSION_AI;
        if (!FileExist(anFile.c_str()) || !FileExist(aiFile.c_str())) {
            aotFileName.clear();
        }
        return aotFileName;
    }
};
}  // namespace panda::ecmascript
#endif