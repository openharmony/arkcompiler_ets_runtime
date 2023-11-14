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

#include "file_path_utils.h"

#include <fstream>
#include <regex>
#include <vector>
#include "constants.h"
#include <atomic>
#include <chrono>
#include <climits>

#ifdef WINDOWS_PLATFORM
#include <io.h>

namespace {
char* realpath(const char* path, char* resolvedPath)
{
    if (_access(path, 0) < 0) {
        return nullptr;
    }
    if (strcpy_s(resolvedPath, PATH_MAX, path) != 0) {
        return nullptr;
    }
    return resolvedPath;
}
}
#endif

namespace panda {
namespace ecmascript {
namespace {
constexpr char EXT_NAME_ABC[] = ".abc";
constexpr char EXT_NAME_ETS[] = ".ets";
constexpr char EXT_NAME_TS[] = ".ts";
constexpr char EXT_NAME_JS[] = ".js";
constexpr char PREFIX_BUNDLE[] = "@bundle:";
constexpr char PREFIX_MODULE[] = "@module:";
constexpr char PREFIX_LOCAL[] = "@local:";
constexpr char NPM_PATH_SEGMENT[] = "node_modules";
constexpr char NPM_ENTRY_FILE[] = "index.abc";
constexpr char NPM_ENTRY_LINK[] = "entry.txt";
constexpr char BUNDLE_INSTALL_PATH[] = "/data/storage/el1/bundle/";
constexpr char OTHER_BUNDLE_INSTALL_PATH[] = "/data/bundles/";

constexpr size_t MAX_NPM_LEVEL = 1;
constexpr size_t SEGMENTS_LIMIT_TWO = 2;
constexpr size_t SEGMENTS_LIMIT_THREE = 3;
} // namespace

bool StringStartWith(const std::string& str, const char* startStr, size_t startStrLen)
{
    return ((str.length() >= startStrLen) && (str.compare(0, startStrLen, startStr) == 0));
}

bool StringEndWith(const std::string& str, const char* endStr, size_t endStrLen)
{
    size_t len = str.length();
    return ((len >= endStrLen) && (str.compare(len - endStrLen, endStrLen, endStr) == 0));
}

void SplitString(const std::string& str, std::vector<std::string>& out, size_t pos, const char* seps)
{
    if (str.empty() || pos >= str.length()) {
        return;
    }

    size_t startPos = pos;
    size_t endPos = 0;
    while ((endPos = str.find_first_of(seps, startPos)) != std::string::npos) {
        if (endPos > startPos) {
            out.emplace_back(str.substr(startPos, endPos - startPos));
        }
        startPos = endPos + 1;
    }

    if (startPos < str.length()) {
        out.emplace_back(str.substr(startPos));
    }
}

std::string JoinString(const std::vector<std::string>& strs, char sep, size_t startIndex)
{
    std::string out;
    for (size_t index = startIndex; index < strs.size(); ++index) {
        if (!strs[index].empty()) {
            out.append(strs[index]) += sep;
        }
    }
    if (!out.empty()) {
        out.pop_back();
    }
    return out;
}

std::string StripString(const std::string& str, const char* charSet)
{
    size_t startPos = str.find_first_not_of(charSet);
    if (startPos == std::string::npos) {
        return std::string();
    }

    return str.substr(startPos, str.find_last_not_of(charSet) - startPos + 1);
}

void FixExtName(std::string& path)
{
    if (path.empty()) {
        return;
    }

    if (StringEndWith(path, EXT_NAME_ABC, sizeof(EXT_NAME_ABC) - 1)) {
        return;
    }

    if (StringEndWith(path, EXT_NAME_ETS, sizeof(EXT_NAME_ETS) - 1)) {
        path.erase(path.length() - (sizeof(EXT_NAME_ETS) - 1), sizeof(EXT_NAME_ETS) - 1);
    } else if (StringEndWith(path, EXT_NAME_TS, sizeof(EXT_NAME_TS) - 1)) {
        path.erase(path.length() - (sizeof(EXT_NAME_TS) - 1), sizeof(EXT_NAME_TS) - 1);
    } else if (StringEndWith(path, EXT_NAME_JS, sizeof(EXT_NAME_JS) - 1)) {
        path.erase(path.length() - (sizeof(EXT_NAME_JS) - 1), sizeof(EXT_NAME_JS) - 1);
    }

    path.append(EXT_NAME_ABC);
}

std::string GetInstallPath(const std::string& curJsModulePath, bool module)
{
    size_t pos = std::string::npos;
    if (StringStartWith(curJsModulePath, BUNDLE_INSTALL_PATH, std::string(BUNDLE_INSTALL_PATH).length())) {
        pos = std::string(BUNDLE_INSTALL_PATH).length() - 1;
    } else {
        if (!StringStartWith(curJsModulePath, OTHER_BUNDLE_INSTALL_PATH,
            std::string(OTHER_BUNDLE_INSTALL_PATH).length())) {
            return std::string();
        }

        pos = curJsModulePath.find('/', std::string(OTHER_BUNDLE_INSTALL_PATH).length());
        if (pos == std::string::npos) {
            return std::string();
        }
    }

    if (module) {
        pos = curJsModulePath.find('/', pos + 1);
        if (pos == std::string::npos) {
            return std::string();
        }
    }

    return curJsModulePath.substr(0, pos + 1);
}

std::string MakeNewJsModulePath(
    const std::string& curJsModulePath, const std::string& newJsModuleUri)
{
    std::string moduleInstallPath = GetInstallPath(curJsModulePath, true);
    if (moduleInstallPath.empty()) {
        return std::string();
    }

    std::vector<std::string> pathVector;
    SplitString(curJsModulePath, pathVector, moduleInstallPath.length());

    if (pathVector.empty()) {
        return std::string();
    }

    // Remove file name, reserve only dir name
    pathVector.pop_back();

    std::vector<std::string> relativePathVector;
    SplitString(newJsModuleUri, relativePathVector);

    for (auto& value : relativePathVector) {
        if (value == ".") {
            continue;
        } else if (value == "..") {
            if (pathVector.empty()) {
                return std::string();
            }
            pathVector.pop_back();
        } else {
            pathVector.emplace_back(std::move(value));
        }
    }

    std::string jsModulePath = moduleInstallPath + JoinString(pathVector, '/');
    FixExtName(jsModulePath);
    if (jsModulePath.size() >= PATH_MAX) {
        return std::string();
    }

    char path[PATH_MAX];
    if (realpath(jsModulePath.c_str(), path) != nullptr) {
        return std::string(path);
    }
    return std::string();
}

std::string FindNpmPackageInPath(const std::string& npmPath)
{
    std::string fileName = npmPath + "/" + NPM_ENTRY_FILE;

    char path[PATH_MAX];
    if (fileName.size() >= PATH_MAX) {
        return std::string();
    }
    if (realpath(fileName.c_str(), path) != nullptr) {
        return path;
    }

    fileName = npmPath + "/" + NPM_ENTRY_LINK;
    if (fileName.size() >= PATH_MAX) {
        return std::string();
    }
    if (realpath(fileName.c_str(), path) == nullptr) {
        return std::string();
    }

    std::ifstream stream(path, std::ios::ate);
    if (!stream.is_open()) {
        return std::string();
    }

    auto fileLen = stream.tellg();
    if (fileLen >= PATH_MAX) {
        return std::string();
    }

    stream.seekg(0);
    stream.read(path, fileLen);
    path[fileLen] = '\0';
    stream.close();

    std::string npmPackagePath = npmPath + '/' + StripString(path);
    if (npmPackagePath.size() >= PATH_MAX) {
        return std::string();
    }
    if (realpath(npmPackagePath.c_str(), path) == nullptr) {
        return std::string();
    }
    return path;
}

std::string FindNpmPackageInTopLevel(
    const std::string& moduleInstallPath, const std::string& npmPackage, size_t start)
{
    for (size_t level = start; level <= MAX_NPM_LEVEL; ++level) {
        std::string path = moduleInstallPath + NPM_PATH_SEGMENT + '/' + std::to_string(level) + '/' + npmPackage;
        path = FindNpmPackageInPath(path);
        if (!path.empty()) {
            return path;
        }
    }

    return std::string();
}

std::string FindNpmPackage(const std::string& curJsModulePath, const std::string& npmPackage)
{
    std::string newJsModulePath = MakeNewJsModulePath(curJsModulePath, npmPackage);
    if (!newJsModulePath.empty()) {
        return newJsModulePath;
    }
    std::string moduleInstallPath = GetInstallPath(curJsModulePath);
    if (moduleInstallPath.empty()) {
        return std::string();
    }
    std::vector<std::string> pathVector;
    SplitString(curJsModulePath, pathVector, moduleInstallPath.length());
    if (pathVector.empty()) {
        return std::string();
    }

    if (pathVector[0] != NPM_PATH_SEGMENT) {
        return FindNpmPackageInTopLevel(moduleInstallPath, npmPackage);
    }

    // Remove file name, reserve only dir name
    pathVector.pop_back();

    // Find npm package until reach top level npm path such as 'node_modules/0',
    // so there must be 2 element in vector
    while (pathVector.size() > 2) {
        std::string path =
            moduleInstallPath + JoinString(pathVector, '/') + '/' + NPM_PATH_SEGMENT + '/' + npmPackage;
        path = FindNpmPackageInPath(path);
        if (!path.empty()) {
            return path;
        }

        pathVector.pop_back();
    }

    char* p = nullptr;
    size_t index = std::strtoul(pathVector.back().c_str(), &p, 10);
    if (p == nullptr || *p != '\0') {
        return std::string();
    }

    return FindNpmPackageInTopLevel(moduleInstallPath, npmPackage, index);
}

std::string ParseOhmUri(
    const std::string& originBundleName, const std::string& curJsModulePath, const std::string& newJsModuleUri)
{
    std::string moduleInstallPath;
    std::vector<std::string> pathVector;
    size_t index = 0;

    if (StringStartWith(newJsModuleUri, PREFIX_BUNDLE, sizeof(PREFIX_BUNDLE) - 1)) {
        SplitString(newJsModuleUri, pathVector, sizeof(PREFIX_BUNDLE) - 1);

        // Uri should have atleast 3 segments
        if (pathVector.size() < SEGMENTS_LIMIT_THREE) {
            return std::string();
        }

        const auto& bundleName = pathVector[index++];
        if (bundleName == originBundleName) {
            moduleInstallPath = std::string(BUNDLE_INSTALL_PATH);
        } else {
            moduleInstallPath = std::string(OTHER_BUNDLE_INSTALL_PATH);
            moduleInstallPath.append(bundleName).append("/");
        }
        moduleInstallPath.append(pathVector[index++]).append("/");
    } else if (StringStartWith(newJsModuleUri, PREFIX_MODULE, sizeof(PREFIX_MODULE) - 1)) {
        SplitString(newJsModuleUri, pathVector, sizeof(PREFIX_MODULE) - 1);

        // Uri should have atleast 2 segments
        if (pathVector.size() < SEGMENTS_LIMIT_TWO) {
            return std::string();
        }

        moduleInstallPath = GetInstallPath(curJsModulePath, false);
        if (moduleInstallPath.empty()) {
            return std::string();
        }
        moduleInstallPath.append(pathVector[index++]).append("/");
    } else if (StringStartWith(newJsModuleUri, PREFIX_LOCAL, sizeof(PREFIX_LOCAL) - 1)) {
        SplitString(newJsModuleUri, pathVector, sizeof(PREFIX_LOCAL) - 1);

        if (pathVector.empty()) {
            return std::string();
        }

        moduleInstallPath = GetInstallPath(curJsModulePath);
        if (moduleInstallPath.empty()) {
            return std::string();
        }
    } else {
        return std::string();
    }

    if (pathVector[index] != NPM_PATH_SEGMENT) {
        return moduleInstallPath + JoinString(pathVector, '/', index);
    }

    return FindNpmPackageInTopLevel(moduleInstallPath, JoinString(pathVector, '/', index + 1));
}

std::string NormalizeUri(
    const std::string& bundleName, const std::string& curJsModulePath, const std::string& newJsModuleUri)
{
    std::string newJsModulePath;
    if (curJsModulePath.empty() || newJsModuleUri.empty()) {
        return newJsModulePath;
    }

    std::string normalizeUri = newJsModuleUri;
    std::replace(normalizeUri.begin(), normalizeUri.end(), '\\', '/');

    switch (normalizeUri[0]) {
        case '.': {
            newJsModulePath = MakeNewJsModulePath(curJsModulePath, normalizeUri);
            break;
        }
        case '@': {
            newJsModulePath = ParseOhmUri(bundleName, curJsModulePath, normalizeUri);
            if (newJsModulePath.empty()) {
                newJsModulePath = FindNpmPackage(curJsModulePath, normalizeUri);
            }
            break;
        }
        default: {
            newJsModulePath = FindNpmPackage(curJsModulePath, normalizeUri);
            break;
        }
    }

    FixExtName(newJsModulePath);
    return newJsModulePath;
}

bool MakeFilePath(const std::string& codePath, const std::string& modulePath, std::string& fileName)
{
    std::string path(codePath);
    path.append("/").append(modulePath);
    if (path.length() > PATH_MAX) {
        return false;
    }
    char resolvedPath[PATH_MAX + 1] = { 0 };
    if (realpath(path.c_str(), resolvedPath) != nullptr) {
        fileName = resolvedPath;
        return true;
    }

    auto start = path.find_last_of('/');
    auto end = path.find_last_of('.');
    if (end == std::string::npos || end == 0) {
        return false;
    }

    auto pos = path.find_last_of('.', end - 1);
    if (pos == std::string::npos) {
        return false;
    }

    path.erase(start + 1, pos - start);

    if (realpath(path.c_str(), resolvedPath) == nullptr) {
        return false;
    }

    fileName = resolvedPath;
    return true;
}

std::string GetLoadPath(const std::string& hapPath)
{
    std::regex hapPattern(std::string(Constants::ABS_CODE_PATH) + std::string(Constants::FILE_SEPARATOR));
    std::string loadPath = std::regex_replace(hapPath, hapPattern, "");
    loadPath = std::string(Constants::LOCAL_CODE_PATH) + std::string(Constants::FILE_SEPARATOR) +
        loadPath.substr(loadPath.find(std::string(Constants::FILE_SEPARATOR)) + 1);
    return loadPath;
}

std::string GetRelativePath(const std::string& srcPath)
{
    if (srcPath.empty() || srcPath[0] != '/') {
        return srcPath;
    }
    std::regex srcPattern(Constants::LOCAL_CODE_PATH);
    std::string relativePath = std::regex_replace(srcPath, srcPattern, "");
    if (relativePath.find(Constants::FILE_SEPARATOR) == 0) {
        relativePath = relativePath.substr(1);
        relativePath = relativePath.substr(relativePath.find(std::string(Constants::FILE_SEPARATOR)) + 1);
    }
    return relativePath;
}
}  // namespace AbilityBase
}  // namespace OHOS
