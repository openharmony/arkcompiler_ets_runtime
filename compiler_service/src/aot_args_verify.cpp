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

#include <nlohmann/json.hpp>
#include <regex>

#include "aot_compiler_constants.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"

namespace OHOS::ArkCompiler {

bool AotArgsVerify::ValidateArkProfilePath(const std::string &resolvedPgoDir, const std::string &bundleName)
{
    std::regex pattern("/data/app/el1/\\d+/aot_compiler/ark_profile.*");
    if (!std::regex_match(resolvedPgoDir, pattern) || resolvedPgoDir.find(bundleName) == std::string::npos) {
        LOG_SA(ERROR) << "verify ark-profile path wrong, pgoDir: " << resolvedPgoDir << ", bundleName: " << bundleName;
        return false;
    }
    return true;
}

bool AotArgsVerify::ParsePgoDirAndBundleName(const std::string &pkgInfoStr, std::string &pgoDir,
    std::string &bundleName)
{
    if (!nlohmann::json::accept(pkgInfoStr)) {
        LOG_SA(ERROR) << "invalid json when parse compiler-pkg-info";
        return false;
    }

    nlohmann::json pkgInfoJson = nlohmann::json::parse(pkgInfoStr);
    if (pkgInfoJson.is_discarded() || pkgInfoJson.is_null() || pkgInfoJson.empty()) {
        LOG_SA(ERROR) << "invalid json when parse compiler-pkg-info";
        return false;
    }

    const std::string pgoDirKey = "pgoDir";
    const std::string bundleNameKey = "bundleName";

    if (!pkgInfoJson.contains(bundleNameKey) ||
        pkgInfoJson[bundleNameKey].is_null() ||
        !pkgInfoJson[bundleNameKey].is_string()) {
        LOG_SA(ERROR) << "invalid or missing bundleName in compiler-pkg-info";
        return false;
    }
    if (!pkgInfoJson.contains(pgoDirKey) ||
        pkgInfoJson[pgoDirKey].is_null() ||
        !pkgInfoJson[pgoDirKey].is_string()) {
        LOG_SA(ERROR) << "invalid or missing pgoDir in compiler-pkg-info";
        return false;
    }

    pgoDir = pkgInfoJson[pgoDirKey].get<std::string>();
    bundleName = pkgInfoJson[bundleNameKey].get<std::string>();
    if (pgoDir.empty() || bundleName.empty()) {
        LOG_SA(ERROR) << "pgoDir or bundleName value is empty in compiler-pkg-info";
        return false;
    }

    return true;
}

bool AotArgsVerify::ParseBundleNameFromPkgInfo(const std::string &pkgInfoStr, std::string &bundleName)
{
    if (!nlohmann::json::accept(pkgInfoStr)) {
        LOG_SA(ERROR) << "invalid json when parse bundleName from compiler-pkg-info";
        return false;
    }

    nlohmann::json pkgInfoJson = nlohmann::json::parse(pkgInfoStr);
    if (pkgInfoJson.is_discarded() || pkgInfoJson.is_null() || pkgInfoJson.empty()) {
        LOG_SA(ERROR) << "invalid json when parse bundleName from compiler-pkg-info";
        return false;
    }

    const std::string bundleNameKey = "bundleName";
    if (!pkgInfoJson.contains(bundleNameKey) ||
        pkgInfoJson[bundleNameKey].is_null() ||
        !pkgInfoJson[bundleNameKey].is_string()) {
        LOG_SA(ERROR) << "invalid or missing bundleName in compiler-pkg-info";
        return false;
    }

    bundleName = pkgInfoJson[bundleNameKey].get<std::string>();
    if (bundleName.empty()) {
        LOG_SA(ERROR) << "bundleName value is empty in compiler-pkg-info";
        return false;
    }

    return true;
}

bool AotArgsVerify::ValidatePathOnlyTraverse(const std::string &inputPath)
{
    if (inputPath.find("..") != std::string::npos) {
        LOG_SA(ERROR) << "potential path traversal detected in path: " << inputPath.c_str();
        return false;
    }

    // Check for path traversal sequences
    if (inputPath.find("../") != std::string::npos ||
        inputPath.find("..\\") != std::string::npos) {
        LOG_SA(ERROR) << "potential path traversal detected in path: " << inputPath.c_str();
        return false;
    }

    // Check for parent directory traversal in absolute paths
    if (inputPath.find("/../") != std::string::npos ||
        inputPath.find("\\..\\") != std::string::npos) {
        LOG_SA(ERROR) << "parent directory traversal detected: " << inputPath.c_str();
        return false;
    }
    return true;
}

bool AotArgsVerify::ValidateAndResolvePath(const std::string &inputPath, std::string &outputPath)
{
    if (!ValidatePathOnlyTraverse(inputPath)) {
        return false;
    }
    if (!panda::ecmascript::RealPath(inputPath, outputPath)) {
        LOG_SA(ERROR) << "failed to resolve path: " << inputPath.c_str();
        return false;
    }
    return true;
}

bool AotArgsVerify::ValidateArkCacheFilePaths(const std::string &aotFile, const std::string &anFile,
    const std::string &bundleName)
{
    // 1. Verify anFile equals aotFile + ".an" (completely equal except for file suffix)
    if (anFile != aotFile + ".an") {
        LOG_SA(ERROR) << "anFileName must be aotFile with .an suffix: "
                      << "aotFile=" << aotFile.c_str() << ", anFile=" << anFile.c_str();
        return false;
    }

    // 2. Check for path traversal in both file paths
    if (!ValidatePathOnlyTraverse(aotFile) || !ValidatePathOnlyTraverse(anFile)) {
        LOG_SA(ERROR) << "path traversal detected in aot-file or anFileName";
        return false;
    }

    // 3. Verify parent directory strictly equals expected path
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

bool AotArgsVerify::CheckAndGetAotAndAnFiles(
    const std::unordered_map<std::string, std::string> &argsMap,
    std::string &aotFile, std::string &anFile)
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

    aotFile = aotFileIt->second;
    anFile = anFileIt->second;
    return true;
}

bool AotArgsVerify::CheckAOTArgs(const std::unordered_map<std::string, std::string> &argsMap)
{
    auto it = argsMap.find(ArgsIdx::COMPILER_PKG_INFO);
    if (it == argsMap.end()) {
        LOG_SA(ERROR) << ArgsIdx::COMPILER_PKG_INFO.c_str() << " not found in argsMap";
        return false;
    }

    std::string pgoDir;
    std::string bundleName;
    if (!ParsePgoDirAndBundleName(it->second, pgoDir, bundleName)) {
        return false;
    }

    // Validate and resolve pgoDir path
    std::string resolvedPgoDir;
    if (!ValidateAndResolvePath(pgoDir, resolvedPgoDir) ||
        !ValidateArkProfilePath(resolvedPgoDir, bundleName)) {
        return false;
    }

    // Get and validate aot-file and anFileName paths
    std::string aotFile;
    std::string anFile;
    if (!CheckAndGetAotAndAnFiles(argsMap, aotFile, anFile)) {
        return false;
    }
    if (!ValidateArkCacheFilePaths(aotFile, anFile, bundleName)) {
        return false;
    }

    return true;
}

bool AotArgsVerify::CheckStaticAotArgs(const std::unordered_map<std::string, std::string> &argsMap)
{
    // Get bundleName from pkgInfo
    auto pkgInfoIt = argsMap.find(ArgsIdx::COMPILER_PKG_INFO);
    if (pkgInfoIt == argsMap.end()) {
        LOG_SA(ERROR) << ArgsIdx::COMPILER_PKG_INFO.c_str() << " not found in argsMap";
        return false;
    }

    std::string bundleName;
    if (!ParseBundleNameFromPkgInfo(pkgInfoIt->second, bundleName)) {
        return false;
    }

    // Get and validate aot-file and anFileName paths using common function
    std::string aotFile;
    std::string anFile;
    if (!CheckAndGetAotAndAnFiles(argsMap, aotFile, anFile)) {
        return false;
    }
    if (!ValidateArkCacheFilePaths(aotFile, anFile, bundleName)) {
        return false;
    }

    return true;
}

bool AotArgsVerify::CheckFrameworkStaticAotArgs(const std::unordered_map<std::string, std::string> &argsMap)
{
    // Get anFileName path
    auto anFileIt = argsMap.find(ArgsIdx::AN_FILE_NAME);
    if (anFileIt == argsMap.end()) {
        LOG_SA(ERROR) << ArgsIdx::AN_FILE_NAME.c_str() << " not found in argsMap";
        return false;
    }

    const std::string &anFile = anFileIt->second;

    // Check for path traversal
    if (!ValidatePathOnlyTraverse(anFile)) {
        return false;
    }

    // Check that the path starts with the expected framework cache directory
    const std::string expectedPrefix = "/data/service/el1/public/for-all-app/framework_ark_cache";
    if (anFile.substr(0, expectedPrefix.length()) != expectedPrefix) {
        LOG_SA(ERROR) << "framework file is not in expected location: " << anFile.c_str()
                      << ", expected prefix: " << expectedPrefix.c_str();
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckCodeSignArkCacheFileName(const std::string &inputPath)
{
    // 1. Check for path traversal before RealPath
    if (!ValidatePathOnlyTraverse(inputPath)) {
        LOG_SA(ERROR) << "path traversal detected in fileName: " << inputPath.c_str();
        return false;
    }
    // 2. Resolve real path to prevent symlink attacks
    std::string resolvedPath;
    if (!panda::ecmascript::RealPath(inputPath, resolvedPath)) {
        LOG_SA(ERROR) << "failed to resolve fileName path: " << inputPath.c_str();
        return false;
    }
    // 3. Re-check for path traversal after RealPath (symlink could bypass)
    if (!ValidatePathOnlyTraverse(resolvedPath)) {
        LOG_SA(ERROR) << "path traversal detected after realpath in fileName: " << resolvedPath.c_str();
        return false;
    }
    // 4. Strong matching: ensure path is under valid arkcache directories
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

} // namespace OHOS::ArkCompiler
