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
#include <string>
#include <vector>

#include "aot_compiler_args.h"
#include "aot_compiler_constants.h"

namespace panda::ecmascript {
using fd_t = int;
}

namespace OHOS::ArkCompiler {

class AotArgsVerify {
public:

    static bool CheckArkProfilePath(const std::string &pgoDir, const std::string &bundleName);
    static bool CheckPathTraverse(const std::string &inputPath);
    static bool CheckArkCacheFiles(const std::string &aotFile, const std::string &anFile,
        const std::string &bundleName);
    static bool CheckAnFileSuffix(const std::string &aotFile, const std::string &anFile);
    static bool CheckArkCacheDirectoryPrefix(const std::string &aotFile, const std::string &bundleName);
    static bool CheckPkgInfoFields(const AotCompilerArgs &args, AotParserType type);
    static bool CheckFrameworkAnFile(const std::string &anFile);
    static bool CheckAOTArgs(const AotCompilerArgs &args);
    static bool CheckStaticAotArgs(const AotCompilerArgs &args);
    static bool CheckTriggerTypeForAOT(const AotCompilerArgs &args);
    static bool IsSharedBundlesType(const AotCompilerArgs &args);
    static bool CheckFrameworkStaticAotArgs(const AotCompilerArgs &args);
    static bool CheckCodeSignArkCacheFilePath(const std::string &inputPath);
    static bool IsValidArkCachePath(const std::string &resolvedPath);
    static bool CheckAbcName(const std::string &abcName, AotParserType type);
    static bool CheckModuleName(const std::string &moduleName);
    static bool CheckBundleUidAndGid(int32_t uid, int32_t gid);
    static bool CheckCompileMode(const std::string &compileMode);
    static bool CheckModuleArkTSMode(const std::string &moduleArkTSMode);
    static bool ParseUint32Field(const nlohmann::json &jsonObj, const char *key, uint32_t &output);
    static bool ParseInt32Field(const nlohmann::json &jsonObj, const char *key, int32_t &output);
    static bool ParseUint32FieldFromHex(const std::string &hexStr, uint32_t &output);
    static bool ParseStringField(const nlohmann::json &jsonObj, const char *key, std::string &output);
    static bool CheckHostPrivateArkCacheFiles(const AotCompilerArgs &args);
    static bool CheckHapFsVerity(int fd);

    // HAP verification (struct-based)
    static bool CheckBundleNameAndAppIdentifier(const std::string &bundleName, const std::string &pkgPath,
        const std::string &appIdentifier, const std::vector<HspModuleInfo> &externalPkgs);

private:
    static bool CheckProcessUid(int32_t processUid);
    static bool CheckEncryptedBundle(uint32_t isEncryptedBundle);
    static bool CheckHapBundleInfo(const std::string &hapPath, const std::string &expectedBundleName,
        const std::string *expectedAppIdentifier,
        int32_t (*parseFunc)(const int32_t, std::string&, std::string&));
    static bool CheckExternalPackages(const std::vector<HspModuleInfo> &externalPkgs,
        int32_t (*parseFunc)(const int32_t, std::string&, std::string&));
};

} // namespace OHOS::ArkCompiler

#endif // OHOS_ARKCOMPILER_AOT_ARGS_VERIFY_H
