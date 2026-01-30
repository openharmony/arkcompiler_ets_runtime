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

#include "aot_args_verify.h"

#include <charconv>
#include <dlfcn.h>
#include <fcntl.h>
#include <nlohmann/json.hpp>
#include <regex>
#include <sys/stat.h>

#include "aot_compiler_constants.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"

namespace OHOS::ArkCompiler {

bool AotArgsVerify::CheckArkProfilePath(const std::string &pgoDir, const std::string &bundleName)
{
    if (!CheckPathTraverse(pgoDir)) {
        return false;
    }
    std::string resolvedPgoDir;
    if (!panda::ecmascript::RealPath(pgoDir, resolvedPgoDir)) {
        LOG_SA(ERROR) << "failed to resolve path: " << pgoDir.c_str();
        return false;
    }

    std::regex pattern("/data/app/el1/\\d+/aot_compiler/ark_profile.*");
    if (!std::regex_match(resolvedPgoDir, pattern) || resolvedPgoDir.find(bundleName) == std::string::npos) {
        LOG_SA(ERROR) << "verify ark-profile path wrong, pgoDir: " << resolvedPgoDir << ", bundleName: " << bundleName;
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckPathTraverse(const std::string &inputPath)
{
    if (inputPath.find("..") != std::string::npos) {
        LOG_SA(ERROR) << "potential path traversal detected in path: " << inputPath.c_str();
        return false;
    }
    if (inputPath.find("../") != std::string::npos ||
        inputPath.find("..\\") != std::string::npos) {
        LOG_SA(ERROR) << "potential path traversal detected in path: " << inputPath.c_str();
        return false;
    }
    if (inputPath.find("/../") != std::string::npos ||
        inputPath.find("\\..\\") != std::string::npos) {
        LOG_SA(ERROR) << "parent directory traversal detected: " << inputPath.c_str();
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckArkCacheFiles(const std::unordered_map<std::string, std::string> &argsMap,
    const std::string &bundleName)
{
    auto aotFileIt = argsMap.find(ArgsIdx::AOT_FILE);
    auto anFileIt = argsMap.find(ArgsIdx::AN_FILE_NAME);
    if (aotFileIt == argsMap.end()) {
        LOG_SA(ERROR) << ArgsIdx::AOT_FILE.c_str() << " not found in argsMap";
        return false;
    }
    if (anFileIt == argsMap.end()) {
        LOG_SA(ERROR) << ArgsIdx::AN_FILE_NAME.c_str() << " not found in argsMap";
        return false;
    }

    const std::string &aotFile = aotFileIt->second;
    const std::string &anFile = anFileIt->second;

    return CheckAnFileSuffix(aotFile, anFile) &&
           CheckPathTraverse(aotFile) && CheckPathTraverse(anFile) &&
           CheckArkCacheDirectoryPrefix(aotFile, bundleName);
}

bool AotArgsVerify::CheckAnFileSuffix(const std::string &aotFile, const std::string &anFile)
{
    if (anFile != aotFile + ".an") {
        LOG_SA(ERROR) << "anFileName must be aotFile with .an suffix: "
                      << "aotFile=" << aotFile.c_str() << ", anFile=" << anFile.c_str();
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckArkCacheDirectoryPrefix(const std::string &aotFile, const std::string &bundleName)
{
    size_t lastSlash = aotFile.rfind('/');
    if (lastSlash == std::string::npos) {
        LOG_SA(ERROR) << "Invalid file path format for aot-file";
        return false;
    }

    std::string aotDir = aotFile.substr(0, lastSlash);
    std::string expectedBasePath = "/data/app/el1/public/aot_compiler/ark_cache/" + bundleName;
    if (aotDir.substr(0, expectedBasePath.length()) != expectedBasePath) {
        LOG_SA(ERROR) << "directory is not in expected location: " << aotDir.c_str()
                      << ", expected prefix: " << expectedBasePath.c_str();
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckFrameworkAnFile(const std::string &anFile)
{
    if (!CheckPathTraverse(anFile)) {
        return false;
    }
    const std::string expectedPrefix = "/data/service/el1/public/for-all-app/framework_ark_cache";
    if (anFile.substr(0, expectedPrefix.length()) != expectedPrefix) {
        LOG_SA(ERROR) << "framework file is not in expected location: " << anFile.c_str()
                      << ", expected prefix: " << expectedPrefix.c_str();
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckPkgInfoFields(const AotPkgInfo &pkgInfo, AotParserType type)
{
    if (!CheckModuleName(pkgInfo.moduleName)) {
        return false;
    }
    if (!CheckAbcName(pkgInfo.abcName, type)) {
        return false;
    }
    if (!CheckAbcOffsetAndSize(pkgInfo.abcOffset.value(), pkgInfo.abcSize.value(), pkgInfo.pkgPath)) {
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckAOTArgs(const std::unordered_map<std::string, std::string> &argsMap)
{
    AotPkgInfo pkgInfo;
    auto pkgInfoIt = argsMap.find(ArgsIdx::COMPILER_PKG_INFO);
    if (pkgInfoIt == argsMap.end()) {
        LOG_SA(ERROR) << ArgsIdx::COMPILER_PKG_INFO.c_str() << " not found in argsMap";
        return false;
    }
    if (!ParseAotPkgInfo(pkgInfoIt->second, pkgInfo)) {
        LOG_SA(ERROR) << "Failed to parse AOT package info from JSON";
        return false;
    }

    auto compilerModeIt = argsMap.find(ArgsIdx::TARGET_COMPILER_MODE);
    bool isPartialMode = (compilerModeIt != argsMap.end() && compilerModeIt->second == ArgsIdx::PARTIAL);
    if (!isPartialMode || pkgInfo.pgoDir.empty() || !CheckArkProfilePath(pkgInfo.pgoDir, pkgInfo.bundleName)) {
        return false;
    }

    if (!CheckPkgInfoFields(pkgInfo, AotParserType::DYNAMIC_AOT)) {
        return false;
    }
    if (!CheckBundleUidAndGidFromArgsMap(argsMap)) {
        return false;
    }
    if (!CheckArkCacheFiles(argsMap, pkgInfo.bundleName)) {
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckStaticAotArgs(const std::unordered_map<std::string, std::string> &argsMap)
{
    AotPkgInfo pkgInfo;
    auto pkgInfoIt = argsMap.find(ArgsIdx::COMPILER_PKG_INFO);
    if (pkgInfoIt == argsMap.end()) {
        LOG_SA(ERROR) << ArgsIdx::COMPILER_PKG_INFO.c_str() << " not found in argsMap";
        return false;
    }
    if (!ParseAotPkgInfo(pkgInfoIt->second, pkgInfo)) {
        LOG_SA(ERROR) << "Failed to parse AOT package info from JSON";
        return false;
    }

    if (!CheckPkgInfoFields(pkgInfo, AotParserType::STATIC_AOT)) {
        return false;
    }
    if (!CheckBundleUidAndGidFromArgsMap(argsMap)) {
        return false;
    }
    if (!CheckArkCacheFiles(argsMap, pkgInfo.bundleName)) {
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckFrameworkStaticAotArgs(const std::unordered_map<std::string, std::string> &argsMap)
{
    auto anFileIt = argsMap.find(ArgsIdx::AN_FILE_NAME);
    if (anFileIt == argsMap.end()) {
        LOG_SA(ERROR) << ArgsIdx::AN_FILE_NAME.c_str() << " not found in argsMap";
        return false;
    }
    if (!CheckFrameworkAnFile(anFileIt->second)) {
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckCodeSignArkCacheFilePath(const std::string &inputPath)
{
    if (!CheckPathTraverse(inputPath)) {
        LOG_SA(ERROR) << "path traversal detected in fileName: " << inputPath.c_str();
        return false;
    }

    std::string resolvedPath;
    if (!panda::ecmascript::RealPath(inputPath, resolvedPath)) {
        LOG_SA(ERROR) << "failed to resolve fileName path: " << inputPath.c_str();
        return false;
    }

    if (!CheckPathTraverse(resolvedPath)) {
        LOG_SA(ERROR) << "path traversal detected after realpath in fileName: " << resolvedPath.c_str();
        return false;
    }

    const std::string appArkCachePrefix = "/data/app/el1/public/aot_compiler/ark_cache/";
    const std::string frameworkArkCachePrefix = "/data/service/el1/public/for-all-app/framework_ark_cache/";
    bool isValidAppPath = resolvedPath.compare(0, appArkCachePrefix.length(), appArkCachePrefix) == 0;
    bool isValidFrameworkPath = resolvedPath.compare(0, frameworkArkCachePrefix.length(), frameworkArkCachePrefix) == 0;
    if (!isValidAppPath && !isValidFrameworkPath) {
        LOG_SA(ERROR) << "fileName is not in valid arkcache location: " << resolvedPath.c_str()
                      << ", expected prefixes: " << appArkCachePrefix.c_str()
                      << " or " << frameworkArkCachePrefix.c_str();
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckAbcName(const std::string &abcName, AotParserType type)
{
    if (abcName.empty()) {
        LOG_SA(ERROR) << "abcName is empty";
        return false;
    }
    if (type == AotParserType::DYNAMIC_AOT && abcName != ArgsIdx::ABC_MODULES_PATH) {
        LOG_SA(ERROR) << "AOT parser requires abcName='ets/modules.abc', got: " << abcName;
        return false;
    }
    if (type == AotParserType::STATIC_AOT && abcName != ArgsIdx::ABC_MODULES_STATIC_PATH) {
        LOG_SA(ERROR) << "StaticAOT parser requires abcName='ets/modules_static.abc', got: " << abcName;
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckModuleName(const std::string &moduleName)
{
    if (moduleName.empty()) {
        return true;
    }
    return CheckPathTraverse(moduleName);
}

bool AotArgsVerify::CheckAbcOffsetAndSize(
    uint32_t abcOffset, uint32_t abcSize, const std::string &pkgPath)
{
    struct stat fileStat;
    if (stat(pkgPath.c_str(), &fileStat) != 0) {
        LOG_SA(ERROR) << "Failed to get HAP package file size from: " << pkgPath;
        return false;
    }

    uint64_t hapSize = static_cast<uint64_t>(fileStat.st_size);
    if (abcOffset > hapSize) {
        LOG_SA(ERROR) << "abcOffset exceeds HAP package size: "
                      << "offset=" << abcOffset << ", hapSize=" << hapSize;
        return false;
    }
    if (abcOffset + abcSize > hapSize) {
        LOG_SA(ERROR) << "abcOffset + abcSize exceeds HAP package size: "
                      << "offset=" << abcOffset << ", size=" << abcSize
                      << ", sum=" << (abcOffset + abcSize) << ", hapSize=" << hapSize;
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckBundleUidAndGid(int32_t uid, int32_t gid)
{
    if (uid < static_cast<int32_t>(MIN_APP_UID_GID)) {
        LOG_SA(ERROR) << "Invalid BundleUid: " << uid
                      << ", must be >= " << static_cast<int32_t>(MIN_APP_UID_GID);
        return false;
    }
    if (gid < static_cast<int32_t>(MIN_APP_UID_GID)) {
        LOG_SA(ERROR) << "Invalid BundleGid: " << gid
                      << ", must be >= " << static_cast<int32_t>(MIN_APP_UID_GID);
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckBundleUidAndGidFromArgsMap(
    const std::unordered_map<std::string, std::string> &argsMap)
{
    auto bundleUidIt = argsMap.find(ArgsIdx::BUNDLE_UID);
    if (bundleUidIt == argsMap.end()) {
        LOG_SA(ERROR) << ArgsIdx::BUNDLE_UID.c_str() << " not found in argsMap";
        return false;
    }
    auto bundleGidIt = argsMap.find(ArgsIdx::BUNDLE_GID);
    if (bundleGidIt == argsMap.end()) {
        LOG_SA(ERROR) << ArgsIdx::BUNDLE_GID.c_str() << " not found in argsMap";
        return false;
    }
    int32_t uid = std::stoi(bundleUidIt->second);
    int32_t gid = std::stoi(bundleGidIt->second);
    return CheckBundleUidAndGid(uid, gid);
}

bool AotArgsVerify::ParseUint32Field(const nlohmann::json &jsonObj, const char *key, uint32_t &output)
{
    if (!jsonObj.contains(key) || jsonObj[key].is_null()) {
        return false;
    }
    if (!jsonObj[key].is_string() && !jsonObj[key].is_number_unsigned()) {
        return false;
    }

    if (jsonObj[key].is_string()) {
        std::string strValue = jsonObj[key].get<std::string>();
        const char* start = strValue.c_str();
        const char* end = start + strValue.length();
        auto result = std::from_chars(start, end, output, 16);
        if (result.ec != std::errc()) {
            return false;
        }
    } else {
        output = jsonObj[key].get<uint32_t>();
    }
    return true;
}

bool AotArgsVerify::ParseInt32Field(const nlohmann::json &jsonObj, const char *key, int32_t &output)
{
    if (!jsonObj.contains(key) || jsonObj[key].is_null()) {
        return false;
    }
    if (!jsonObj[key].is_string() && !jsonObj[key].is_number_integer()) {
        return false;
    }

    if (jsonObj[key].is_string()) {
        std::string strValue = jsonObj[key].get<std::string>();
        const char* start = strValue.c_str();
        const char* end = start + strValue.length();
        auto result = std::from_chars(start, end, output, 16);
        if (result.ec != std::errc()) {
            return false;
        }
    } else {
        output = jsonObj[key].get<int32_t>();
    }
    return true;
}

bool AotArgsVerify::ParseAotPkgInfo(const std::string &pkgInfoStr, AotPkgInfo &info)
{
    if (!nlohmann::json::accept(pkgInfoStr)) {
        return false;
    }
    nlohmann::json jsonObj = nlohmann::json::parse(pkgInfoStr);
    if (jsonObj.is_discarded() || jsonObj.is_null() || jsonObj.empty()) {
        return false;
    }
    if (!ParseStringField(jsonObj, ArgsIdx::BUNDLE_NAME.c_str(), info.bundleName)) {
        LOG_SA(ERROR) << "missing or invalid bundleName";
        return false;
    }
    if (!ParseStringField(jsonObj, ArgsIdx::PKG_PATH.c_str(), info.pkgPath)) {
        LOG_SA(ERROR) << "missing or invalid pkgPath";
        return false;
    }
    if (!ParseStringField(jsonObj, ArgsIdx::ABC_NAME.c_str(), info.abcName)) {
        LOG_SA(ERROR) << "missing or invalid abcName";
        return false;
    }
    if (!ParseStringField(jsonObj, ArgsIdx::MODULE_NAME.c_str(), info.moduleName)) {
        LOG_SA(ERROR) << "missing or invalid moduleName";
        return false;
    }
    if (!ParseStringField(jsonObj, ArgsIdx::APP_SIGNATURE.c_str(), info.appIdentifier)) {
        LOG_SA(ERROR) << "missing or invalid appIdentifier";
        return false;
    }
    if (!ParseStringField(jsonObj, ArgsIdx::PGO_DIR.c_str(), info.pgoDir)) {
        LOG_SA(ERROR) << "missing or invalid pgoDir";
        return false;
    }
    if (!ParseUint32Field(jsonObj, ArgsIdx::ABC_OFFSET.c_str(), info.abcOffset.emplace())) {
        LOG_SA(ERROR) << "missing or invalid abcOffset";
        return false;
    }
    if (!ParseUint32Field(jsonObj, ArgsIdx::ABC_SIZE.c_str(), info.abcSize.emplace())) {
        LOG_SA(ERROR) << "missing or invalid abcSize";
        return false;
    }
    return true;
}

bool AotArgsVerify::ParseStringField(const nlohmann::json &jsonObj, const char *key, std::string &output)
{
    if (!jsonObj.contains(key) || jsonObj[key].is_null() || !jsonObj[key].is_string()) {
        return false;
    }
    output = jsonObj[key].get<std::string>();
    return true;
}

} // namespace OHOS::ArkCompiler
