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

#include <charconv>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "aot_compiler_constants.h"

namespace panda::ecmascript {
using fd_t = int;
}

namespace OHOS::ArkCompiler {

struct AotPkgInfo {
    std::string bundleName;
    std::string pkgPath;
    std::string abcName;
    std::string moduleName;
    std::string appIdentifier;
    std::string pgoDir;

    std::optional<uint32_t> abcOffset;
    std::optional<uint32_t> abcSize;
};

class AotArgsVerify {
public:

    static bool CheckArkProfilePath(const std::string &pgoDir, const std::string &bundleName);
    static bool CheckPathTraverse(const std::string &inputPath);
    static bool CheckArkCacheFiles(const std::unordered_map<std::string, std::string> &argsMap,
        const std::string &bundleName);
    static bool CheckAnFileSuffix(const std::string &aotFile, const std::string &anFile);
    static bool CheckArkCacheDirectoryPrefix(const std::string &aotFile, const std::string &bundleName);
    static bool CheckPkgInfoFields(const AotPkgInfo &pkgInfo, AotParserType type);
    static bool CheckFrameworkAnFile(const std::string &anFile);
    static bool CheckAOTArgs(const std::unordered_map<std::string, std::string> &argsMap);
    static bool CheckStaticAotArgs(const std::unordered_map<std::string, std::string> &argsMap);
    static bool CheckFrameworkStaticAotArgs(const std::unordered_map<std::string, std::string> &argsMap);
    static bool CheckCodeSignArkCacheFilePath(const std::string &inputPath);
    static bool CheckAbcName(const std::string &abcName, AotParserType type);
    static bool CheckModuleName(const std::string &moduleName);
    static bool CheckAbcOffsetAndSize(
        uint32_t abcOffset, uint32_t abcSize, const std::string &pkgPath);
    static bool CheckBundleUidAndGid(int32_t uid, int32_t gid);
    static bool CheckBundleUidAndGidFromArgsMap(const std::unordered_map<std::string, std::string> &argsMap);
    static bool ParseUint32Field(const nlohmann::json &jsonObj, const char *key, uint32_t &output);
    static bool ParseInt32Field(const nlohmann::json &jsonObj, const char *key, int32_t &output);
    static bool ParseAotPkgInfo(const std::string &pkgInfoStr, AotPkgInfo &info);
    static bool ParseStringField(const nlohmann::json &jsonObj, const char *key, std::string &output);
};

} // namespace OHOS::ArkCompiler

#endif // OHOS_ARKCOMPILER_AOT_ARGS_VERIFY_H
