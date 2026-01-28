/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_ARKCOMPILER_AOT_ARGS_VERIFY_H
#define OHOS_ARKCOMPILER_AOT_ARGS_VERIFY_H

#include <string>
#include <unordered_map>

namespace OHOS::ArkCompiler {

class AotArgsVerify {
public:
    // Validate Ark profile path format
    static bool ValidateArkProfilePath(const std::string &resolvedPgoDir, const std::string &bundleName);

    // Parse and validate compiler package info JSON to get pgoDir and bundleName
    static bool ParsePgoDirAndBundleName(const std::string &pkgInfoStr, std::string &pgoDir, std::string &bundleName);

    // Parse and validate compiler package info JSON to get bundleName only
    static bool ParseBundleNameFromPkgInfo(const std::string &pkgInfoStr, std::string &bundleName);

    // Validate path against traversal attacks
    static bool ValidatePathOnlyTraverse(const std::string &inputPath);

    // Validate and resolve real path
    static bool ValidateAndResolvePath(const std::string &inputPath, std::string &outputPath);

    // Validate Ark cache file paths
    static bool ValidateArkCacheFilePaths(const std::string &aotFile, const std::string &anFile,
                                          const std::string &bundleName);

    // Check and get AOT and AN files from argsMap
    static bool CheckAndGetAotAndAnFiles(const std::unordered_map<std::string, std::string> &argsMap,
                                          std::string &aotFile, std::string &anFile);

    // Check Ark profile and Ark cache (for AOTArgsParser)
    static bool CheckAOTArgs(const std::unordered_map<std::string, std::string> &argsMap);

    // Check static AOT args (for StaticAOTArgsParser)
    static bool CheckStaticAotArgs(const std::unordered_map<std::string, std::string> &argsMap);

    // Check framework static AOT args (for StaticFrameworkAOTArgsParser)
    static bool CheckFrameworkStaticAotArgs(const std::unordered_map<std::string, std::string> &argsMap);

    // Check code sign arkcache file path with realpath, traversal check, and strong matching
    static bool CheckCodeSignArkCacheFileName(const std::string &inputPath);
};

} // namespace OHOS::ArkCompiler

#endif // OHOS_ARKCOMPILER_AOT_ARGS_VERIFY_H
