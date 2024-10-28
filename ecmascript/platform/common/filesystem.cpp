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

#include "ecmascript/platform/filesystem.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ecmascript/platform/file.h"

#include "os/filesystem.h"

namespace panda::ecmascript::filesystem {
bool CreateDirectory(const std::string& path)
{
    if (path.empty() || Exists(path)) {
        return true;
    }
    auto pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        if (!CreateDirectory(path.substr(0, pos))) {
            return false;
        }
    }
    panda::os::CreateDirectories(path);
    if (!Exists(path)) {
        return false;
    }
    return true;
}

bool Exists(const std::string& path)
{
    return panda::ecmascript::FileExist(path.c_str());
}

std::string TempDirectoryPath()
{
    const char* temp = getenv("TMPDIR");
    if (temp) {
        return std::string(temp);
    }
    return "/tmp";
}

std::size_t FileSize(const std::string& path)
{
    struct stat info;
    stat(path.c_str(), &info);
    return info.st_size;
}

bool RemoveAll(const std::string& path)
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }

    if (S_ISDIR(info.st_mode)) {
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            return false;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            std::string entryPath = path + "/" + entry->d_name;
            if (!RemoveAll(entryPath)) {
                closedir(dir);
                return false;
            }
        }
        closedir(dir);
        return rmdir(path.c_str()) == 0;
    } else {
        return unlink(path.c_str()) == 0;
    }
}

void CreateEmptyFile(const std::string& fileName)
{
    std::string realPath;
    if (!panda::ecmascript::RealPath(fileName, realPath, false)) {
        LOG_FULL(ERROR) << "failed to create empty file: " << fileName << ", error: " << std::strerror(errno);
        return;
    }
    if (Exists(realPath)) {
        LOG_FULL(INFO) << realPath << " exists, skip creation";
        return;
    }

    auto index = realPath.find_last_of('/');
    if (index != std::string::npos) {
        std::string dir = realPath.substr(0, index);
        if (Exists(dir)) {
            LOG_FULL(INFO) << dir << " exists, skip creation";
        } else {
            LOG_FULL(INFO) << "creating folder " << dir;
            if (!CreateDirectory(dir)) {
                LOG_FULL(ERROR) << "failed to create folder " << dir << ", error: " << std::strerror(errno);
                return;
            }
        }
    }

    std::ofstream file(realPath);
    if (!file.good()) {
        LOG_FULL(ERROR) << "failed to create empty file: " << realPath << ", error: " << std::strerror(errno);
    } else {
        LOG_FULL(INFO) << "created empty file: " << realPath;
    }
    file.close();
}
} // namespace panda::ecmascript::kungfu::filesystem