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

#include <cstdio>
#include <cstring>
#include <climits>
#include <fstream>
#include <unistd.h>
#include "file_utils.h"
#include "string_utils.h"
#include "mpl_logging.h"

#ifdef _WIN32
#include <shlwapi.h>
#endif

namespace {
const char kFileSeperatorLinuxStyleChar = '/';
const char kFileSeperatorWindowsStyleChar = '\\';
const std::string kFileSeperatorLinuxStyleStr = std::string(1, kFileSeperatorLinuxStyleChar);
const std::string kFileSeperatorWindowsStyleStr = std::string(1, kFileSeperatorWindowsStyleChar);
}  // namespace

namespace maple {
#if __WIN32
const char kFileSeperatorChar = kFileSeperatorWindowsStyleChar;
#else
const char kFileSeperatorChar = kFileSeperatorLinuxStyleChar;
#endif

#if __WIN32
const std::string kFileSeperatorStr = kFileSeperatorWindowsStyleStr;
#else
const std::string kFileSeperatorStr = kFileSeperatorLinuxStyleStr;
#endif

std::string FileUtils::SafeGetenv(const char *envVar)
{
    const char *tmpEnvPtr = std::getenv(envVar);
    CHECK_FATAL((tmpEnvPtr != nullptr), "Failed! Unable to find environment variable %s \n", envVar);

    std::string tmpStr(tmpEnvPtr);
    return tmpStr;
}

std::string FileUtils::GetRealPath(const std::string &filePath)
{
#ifdef _WIN32
    char *path = nullptr;
    if (filePath.size() > PATH_MAX || !PathCanonicalize(path, filePath.c_str())) {
        CHECK_FATAL(false, "invalid file path");
    }
#else
    char path[PATH_MAX] = {0};
    if (filePath.size() > PATH_MAX || realpath(filePath.c_str(), path) == nullptr) {
        CHECK_FATAL(false, "invalid file path");
    }
#endif
    std::string result(path, path + strlen(path));
    return result;
}

std::string FileUtils::GetFileName(const std::string &filePath, bool isWithExtension)
{
    std::string fullFileName = StringUtils::GetStrAfterLast(filePath, kFileSeperatorStr);
#ifdef _WIN32
    fullFileName = StringUtils::GetStrAfterLast(fullFileName, kFileSeperatorLinuxStyleStr);
#endif
    if (isWithExtension) {
        return fullFileName;
    }
    return StringUtils::GetStrBeforeLast(fullFileName, ".");
}

std::string FileUtils::GetFileExtension(const std::string &filePath)
{
    return StringUtils::GetStrAfterLast(filePath, ".", true);
}

std::string FileUtils::GetFileFolder(const std::string &filePath)
{
    std::string folder = StringUtils::GetStrBeforeLast(filePath, kFileSeperatorStr, true);
    std::string curSlashType = kFileSeperatorStr;
#ifdef _WIN32
    if (folder.empty()) {
        curSlashType = kFileSeperatorLinuxStyleStr;
        folder = StringUtils::GetStrBeforeLast(filePath, curSlashType, true);
    }
#endif
    return folder.empty() ? ("." + curSlashType) : (folder + curSlashType);
}

int FileUtils::Remove(const std::string &filePath)
{
    return remove(filePath.c_str());
}

bool FileUtils::IsFileExists(const std::string &filePath)
{
    std::ifstream f(filePath);
    return f.good();
}

std::string FileUtils::AppendMapleRootIfNeeded(bool needRootPath, const std::string &path,
                                               const std::string &defaultRoot)
{
    if (!needRootPath) {
        return path;
    }
    std::ostringstream ostrStream;
    if (getenv(kMapleRoot) == nullptr) {
        ostrStream << defaultRoot << path;
    } else {
        ostrStream << getenv(kMapleRoot) << kFileSeperatorStr << path;
    }
    return ostrStream.str();
}
}  // namespace maple
