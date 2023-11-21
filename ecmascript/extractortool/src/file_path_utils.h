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

#ifndef OHOS_ABILITY_BASE_FILE_PATH_UTILS_H
#define OHOS_ABILITY_BASE_FILE_PATH_UTILS_H

#include <string>
#include <vector>
#include <iostream>

namespace panda {
namespace ecmascript {
    bool StringStartWith(const std::string& str, const char* startStr, size_t startStrLen);
    bool StringEndWith(const std::string& str, const char* endStr, size_t endStrLen);
    void SplitString(const std::string& str, std::vector<std::string>& out, size_t pos = 0, const char* seps = "\\/");
    std::string JoinString(const std::vector<std::string>& strs, char sep, size_t startIndex = 0);
    std::string StripString(const std::string& str, const char* charSet = " \t\n\r");

    void FixExtName(std::string& path);
    std::string GetInstallPath(const std::string& curJsModulePath, bool module = true);
    std::string MakeNewJsModulePath(const std::string& curJsModulePath, const std::string& newJsModuleUri);
    std::string FindNpmPackageInPath(const std::string& npmPath);
    std::string FindNpmPackageInTopLevel(
        const std::string& moduleInstallPath, const std::string& npmPackage, size_t start = 0);
    std::string FindNpmPackage(const std::string& curJsModulePath, const std::string& npmPackage);
    std::string ParseOhmUri(const std::string& originBundleName, const std::string& curJsModulePath,
        const std::string& newJsModuleUri);
    std::string NormalizeUri(
        const std::string& bundleName, const std::string& curJsModulePath, const std::string& newJsModuleUri);
    bool MakeFilePath(const std::string& codePath, const std::string& modulePath, std::string& fileName);
    std::string GetLoadPath(const std::string& hapPath);
    std::string GetRelativePath(const std::string& srcPath);
}  // namespace AbilityBase
}  // namespace OHOS
#endif  // OHOS_ABILITY_BASE_FILE_PATH_UTILS_H
