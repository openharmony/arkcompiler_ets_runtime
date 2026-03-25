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
#include <cstring>
#include <dlfcn.h>
#include <cerrno>
#include <fcntl.h>
#include <nlohmann/json.hpp>
#include <regex>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/fs.h>

#include "aot_compiler_constants.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/file.h"

namespace OHOS::ArkCompiler {

namespace {
// Hex prefix constants for parsing hexadecimal strings
constexpr size_t HEX_PREFIX_LENGTH = 2;  // Length of "0x" or "0X"
constexpr char HEX_PREFIX_LOWER = '0';
constexpr char HEX_PREFIX_X_LOWER = 'x';
constexpr char HEX_PREFIX_X_UPPER = 'X';
constexpr int HEX_RADIX = 16;  // Base for hexadecimal numbers
}  // namespace

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

bool AotArgsVerify::CheckArkCacheFiles(const std::string &aotFile, const std::string &anFile,
    const std::string &bundleName)
{
    if (aotFile.empty()) {
        LOG_SA(ERROR) << "aotFile is empty";
        return false;
    }
    if (anFile.empty()) {
        LOG_SA(ERROR) << "anFile is empty";
        return false;
    }

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
    if (aotDir.length() < expectedBasePath.length() ||
        aotDir.substr(0, expectedBasePath.length()) != expectedBasePath) {
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
    if (anFile.length() < expectedPrefix.length() ||
        anFile.substr(0, expectedPrefix.length()) != expectedPrefix) {
        LOG_SA(ERROR) << "framework file is not in expected location: " << anFile.c_str()
                      << ", expected prefix: " << expectedPrefix.c_str();
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckPkgInfoFields(const AotCompilerArgs &args, AotParserType type)
{
    if (args.bundleName.empty()) {
        LOG_SA(ERROR) << "bundleName is empty";
        return false;
    }
    if (!CheckModuleName(args.moduleName)) {
        return false;
    }
    std::string abcName;
    if (args.moduleArkTSMode == ArgsIdx::ARKTS_DYNAMIC) {
        abcName = ArgsIdx::ABC_MODULES_PATH;
    } else {
        abcName = ArgsIdx::ABC_MODULES_STATIC_PATH;
    }
    if (!CheckAbcName(abcName, type)) {
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckTriggerTypeForAOT(const AotCompilerArgs &args)
{
    if (args.triggerType < 0 ||
        args.triggerType > static_cast<int32_t>(AotTriggerType::INSTALL)) {
        LOG_SA(ERROR) << "invalid triggerType=" << args.triggerType;
        return false;
    }
    if (args.triggerType == static_cast<int32_t>(AotTriggerType::INSTALL)) {
        LOG_SA(ERROR) << "AOT skipped for triggerType=" << static_cast<int32_t>(AotTriggerType::INSTALL);
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckAOTArgs(const AotCompilerArgs &args)
{
    if (!CheckTriggerTypeForAOT(args)) {
        return false;
    }
    if (!CheckCompileMode(args.compileMode)) {
        return false;
    }
    if (!CheckModuleArkTSMode(args.moduleArkTSMode)) {
        return false;
    }
    if (!AotArgsVerify::CheckProcessUid(args.processUid)) {
        return false;
    }
    if (!AotArgsVerify::CheckEncryptedBundle(args.isEncryptedBundle)) {
        return false;
    }

    bool isPartialMode = (args.compileMode == ArgsIdx::PARTIAL);
    if (!isPartialMode || args.pgoDir.empty() || !CheckArkProfilePath(args.pgoDir, args.bundleName)) {
        return false;
    }

    if (!CheckPkgInfoFields(args, AotParserType::DYNAMIC_AOT)) {
        return false;
    }
    if (!CheckBundleUidAndGid(args.bundleUid, args.bundleGid)) {
        return false;
    }
    std::string aotFile = args.outputPath.empty() ? "" : args.outputPath + "/" + args.moduleName;
    if (!CheckArkCacheFiles(aotFile, args.anFileName, args.bundleName)) {
        return false;
    }
    return true;
}

bool AotArgsVerify::IsSharedBundlesType(const AotCompilerArgs &args)
{
    return args.bundleType == static_cast<int32_t>(BundleType::SHARED);
}

bool AotArgsVerify::CheckStaticAotArgs(const AotCompilerArgs &args)
{
    if (!CheckCompileMode(args.compileMode)) {
        return false;
    }
    if (!CheckModuleArkTSMode(args.moduleArkTSMode)) {
        return false;
    }
    if (!AotArgsVerify::CheckProcessUid(args.processUid)) {
        return false;
    }
    if (!AotArgsVerify::CheckEncryptedBundle(args.isEncryptedBundle)) {
        return false;
    }

    if (!CheckPkgInfoFields(args, AotParserType::STATIC_AOT)) {
        return false;
    }

    bool isSharedBundles = IsSharedBundlesType(args);
    if (isSharedBundles) {
        if (!CheckSharedBundlesUidAndGid(args.bundleUid, args.bundleGid)) {
            return false;
        }
        if (!CheckSharedBundlesArkCacheFiles(args.anFileName)) {
            return false;
        }
    } else {
        if (!CheckBundleUidAndGid(args.bundleUid, args.bundleGid)) {
            return false;
        }
        std::string aotFile = args.outputPath.empty() ? "" : args.outputPath + "/" + args.moduleName;
        if (!CheckArkCacheFiles(aotFile, args.anFileName, args.bundleName)) {
            return false;
        }
    }
    return true;
}

bool AotArgsVerify::CheckFrameworkStaticAotArgs(const AotCompilerArgs &args)
{
    if (args.anFileName.empty()) {
        LOG_SA(ERROR) << "anFileName is empty";
        return false;
    }
    if (!CheckFrameworkAnFile(args.anFileName)) {
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

    // The .an file may not exist yet (memfd flow: sign first, persist later).
    // Resolve the parent directory (which always exists) and append the filename.
    std::string resolvedPath;
    std::string parentDir = inputPath.substr(0, inputPath.find_last_of('/'));
    std::string fileName = inputPath.substr(inputPath.find_last_of('/') + 1);
    if (!parentDir.empty() && panda::ecmascript::RealPath(parentDir, resolvedPath)) {
        resolvedPath += "/" + fileName;
    } else {
        // Fallback: if parent dir resolution also fails, use raw path
        // (CheckPathTraverse already validated against traversal above).
        resolvedPath = inputPath;
    }

    if (!CheckPathTraverse(resolvedPath)) {
        LOG_SA(ERROR) << "path traversal detected after realpath in fileName: " << resolvedPath.c_str();
        return false;
    }

    return IsValidArkCachePath(resolvedPath);
}

bool AotArgsVerify::IsValidArkCachePath(const std::string &resolvedPath)
{
    bool isValidAppPath = resolvedPath.compare(0, ArgsIdx::APP_ARK_CACHE_PREFIX.length(),
        ArgsIdx::APP_ARK_CACHE_PREFIX) == 0;
    bool isValidFrameworkPath = resolvedPath.compare(0, ArgsIdx::FRAMEWORK_ARK_CACHE_PREFIX.length(),
        ArgsIdx::FRAMEWORK_ARK_CACHE_PREFIX) == 0;
    bool isValidSharedBundlesPath = resolvedPath.compare(0, ArgsIdx::SHARED_BUNDLES_ARK_CACHE_PREFIX.length(),
        ArgsIdx::SHARED_BUNDLES_ARK_CACHE_PREFIX) == 0;
    if (!isValidAppPath && !isValidFrameworkPath && !isValidSharedBundlesPath) {
        LOG_SA(ERROR) << "fileName is not in valid arkcache location: " << resolvedPath.c_str()
                      << ", expected prefixes: " << ArgsIdx::APP_ARK_CACHE_PREFIX.c_str()
                      << ", " << ArgsIdx::FRAMEWORK_ARK_CACHE_PREFIX.c_str()
                      << " or " << ArgsIdx::SHARED_BUNDLES_ARK_CACHE_PREFIX.c_str();
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

bool AotArgsVerify::CheckProcessUid(int32_t processUid)
{
    if (processUid < 0) {
        LOG_SA(ERROR) << "Invalid processUid: " << processUid << ", must be >= 0";
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckEncryptedBundle(uint32_t isEncryptedBundle)
{
    if (isEncryptedBundle > 1) {
        LOG_SA(ERROR) << "Invalid isEncryptedBundle: " << isEncryptedBundle << ", must be 0 or 1";
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckCompileMode(const std::string &compileMode)
{
    if (compileMode.empty()) {
        LOG_SA(ERROR) << "compileMode is empty";
        return false;
    }
    if (compileMode != ArgsIdx::PARTIAL && compileMode != ArgsIdx::FULL) {
        LOG_SA(ERROR) << "Invalid compileMode: " << compileMode << ", expected 'partial' or 'full'";
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckModuleArkTSMode(const std::string &moduleArkTSMode)
{
    if (moduleArkTSMode.empty()) {
        LOG_SA(ERROR) << "moduleArkTSMode is empty";
        return false;
    }
    if (moduleArkTSMode != ArgsIdx::ARKTS_DYNAMIC &&
        moduleArkTSMode != ArgsIdx::ARKTS_STATIC &&
        moduleArkTSMode != ArgsIdx::ARKTS_HYBRID) {
        LOG_SA(ERROR) << "Invalid moduleArkTSMode: " << moduleArkTSMode
                      << ", expected 'dynamic', 'static', or 'hybrid'";
        return false;
    }
    return true;
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

bool AotArgsVerify::ParseUint32FieldFromHex(const std::string &hexStr, uint32_t &output)
{
    const char* start = hexStr.c_str();
    // Skip optional "0x" or "0X" prefix
    if (hexStr.size() >= HEX_PREFIX_LENGTH && start[0] == HEX_PREFIX_LOWER &&
        (start[1] == HEX_PREFIX_X_LOWER || start[1] == HEX_PREFIX_X_UPPER)) {
        start += HEX_PREFIX_LENGTH;
    }
    const char* end = start + strlen(start);
    auto result = std::from_chars(start, end, output, HEX_RADIX);
    return result.ec == std::errc();
}

bool AotArgsVerify::ParseStringField(const nlohmann::json &jsonObj, const char *key, std::string &output)
{
    if (!jsonObj.contains(key) || jsonObj[key].is_null() || !jsonObj[key].is_string()) {
        return false;
    }
    output = jsonObj[key].get<std::string>();
    return true;
}

bool AotArgsVerify::CheckSharedBundlesUidAndGid(int32_t uid, int32_t gid)
{
    if (uid != OID_SYSTEM || gid != OID_SYSTEM) {
        LOG_SA(ERROR) << "Shared bundles require BundleUid and BundleGid to be " << OID_SYSTEM;
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckSharedBundlesArkCacheFiles(const std::string &anFile)
{
    if (anFile.empty()) {
        LOG_SA(ERROR) << "anFileName is empty";
        return false;
    }

    if (!CheckPathTraverse(anFile)) {
        return false;
    }

    if (anFile.length() < ArgsIdx::SHARED_BUNDLES_ARK_CACHE_PREFIX.length() ||
        anFile.substr(0, ArgsIdx::SHARED_BUNDLES_ARK_CACHE_PREFIX.length()) !=
        ArgsIdx::SHARED_BUNDLES_ARK_CACHE_PREFIX) {
        LOG_SA(ERROR) << "Shared bundles file is not in expected location: " << anFile.c_str()
                      << ", expected prefix: " << ArgsIdx::SHARED_BUNDLES_ARK_CACHE_PREFIX.c_str();
        return false;
    }

    return true;
}

bool AotArgsVerify::CheckHapFsVerity(int fd)
{
    if (fd < 0) {
        LOG_SA(ERROR) << "invalid fd=" << fd;
        return false;
    }
    unsigned int flags = 0;
    int ret = ioctl(fd, FS_IOC_GETFLAGS, &flags);
    if (ret < 0) {
        LOG_SA(ERROR) << "ioctl FS_IOC_GETFLAGS failed, errno=" << errno
                      << " (" << strerror(errno) << "), skip fs-verity check";
        return true;
    }
    if (!(flags & FS_VERITY_FL)) {
        LOG_SA(ERROR) << "HAP file does not have fs-verity enabled, flags=0x"
                      << std::hex << flags;
        return false;
    }
    return true;
}

bool AotArgsVerify::CheckHapBundleInfo(const std::string &hapPath, const std::string &expectedBundleName,
    const std::string* expectedAppIdentifier, int32_t (*parseFunc)(const int32_t, std::string&, std::string&))
{
    std::string realPath;
    if (!panda::ecmascript::RealPath(hapPath.c_str(), realPath, true)) {
        LOG_SA(ERROR) << "Get real path failed: " << hapPath;
        return false;
    }

    int fd = open(realPath.c_str(), O_RDONLY);
    if (fd == -1) {
        LOG_SA(ERROR) << "Open file failed: " << realPath;
        return false;
    }

    if (!CheckHapFsVerity(fd)) {
        LOG_SA(ERROR) << "HAP fs-verity check failed: " << realPath;
        close(fd);
        return false;
    }

    std::string parsedBundleName;
    std::string parsedAppIdentifier;
    int32_t res = parseFunc(fd, parsedBundleName, parsedAppIdentifier);
    close(fd);

    if (res != 0) {
        LOG_SA(ERROR) << "ParseBundleNameAndAppIdentifier failed, res: " << res;
        return false;
    }

    if (parsedBundleName != expectedBundleName) {
        LOG_SA(ERROR) << "Bundle name mismatch: expected=" << expectedBundleName
                      << ", got=" << parsedBundleName;
        return false;
    }

    // Only check appIdentifier if provided (non-null)
    if (expectedAppIdentifier != nullptr && parsedAppIdentifier != *expectedAppIdentifier) {
        LOG_SA(ERROR) << "App identifier mismatch: expected=" << *expectedAppIdentifier
                      << ", got=" << parsedAppIdentifier;
        return false;
    }

    return true;
}

bool AotArgsVerify::CheckExternalPackages(const std::vector<HspModuleInfo> &externalPkgs,
    int32_t (*parseFunc)(const int32_t, std::string&, std::string&))
{
    if (externalPkgs.empty()) {
        LOG_SA(INFO) << "No external packages to verify";
        return true;
    }
    for (const auto& pkg : externalPkgs) {
        if (pkg.bundleName.empty() || pkg.hapPath.empty()) {
            LOG_SA(ERROR) << "External package missing bundleName or hapPath";
            return false;
        }
        // External packages only check bundleName, not appIdentifier
        if (!CheckHapBundleInfo(pkg.hapPath, pkg.bundleName, nullptr, parseFunc)) {
            return false;
        }
    }
    LOG_SA(INFO) << "CheckExternalPackages success";
    return true;
}

bool AotArgsVerify::CheckBundleNameAndAppIdentifier(const std::string &bundleName, const std::string &pkgPath,
    const std::string &appIdentifier, const std::vector<HspModuleInfo>& externalPkgs)
{
#if !defined(PANDA_TARGET_OHOS)
    LOG_SA(INFO) << "CheckBundleNameAndAppIdentifier non-OHOS, return false";
    return false;
#endif

    if (bundleName.empty()) {
        LOG_SA(ERROR) << "bundleName is empty";
        return false;
    }
    if (appIdentifier.empty()) {
        LOG_SA(ERROR) << "appIdentifier is empty";
        return false;
    }
    if (pkgPath.empty()) {
        LOG_SA(ERROR) << "pkgPath is empty";
        return false;
    }

    void* handle = dlopen("libhapverify.z.so", RTLD_NOW);
    if (handle == nullptr) {
        LOG_SA(ERROR) << "dlopen libhapverify.z.so failed: " << dlerror();
        return false;
    }

    using ParseBundleNameAndAppIdentifierFunc = int32_t (*)(const int32_t, std::string&, std::string&);
    ParseBundleNameAndAppIdentifierFunc parseFunc =
        reinterpret_cast<ParseBundleNameAndAppIdentifierFunc>(
            dlsym(handle, "ParseBundleNameAndAppIdentifier"));
    if (parseFunc == nullptr) {
        LOG_SA(ERROR) << "dlsym ParseBundleNameAndAppIdentifier failed";
        dlclose(handle);
        return false;
    }

    if (!CheckHapBundleInfo(pkgPath, bundleName, &appIdentifier, parseFunc)) {
        dlclose(handle);
        return false;
    }

    if (!CheckExternalPackages(externalPkgs, parseFunc)) {
        dlclose(handle);
        return false;
    }

    dlclose(handle);
    LOG_SA(INFO) << "CheckBundleNameAndAppIdentifier success";
    return true;
}

} // namespace OHOS::ArkCompiler
