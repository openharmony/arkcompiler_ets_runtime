/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/platform/file.h"

#include <climits>
#include <sys/mman.h>
#include <unistd.h>

#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/map.h"

namespace panda::ecmascript {
bool RealPath(const std::string &path, std::string &realPath, bool readOnly)
{
    if (path.empty() || path.size() > PATH_MAX) {
        LOG_ECMA(WARN) << "File path is illeage";
        return false;
    }
    char buffer[PATH_MAX] = { '\0' };
    if (!realpath(path.c_str(), buffer)) {
        // Maybe file does not exist.
        if (!readOnly && errno == ENOENT) {
            realPath = path;
            return true;
        }
        LOG_ECMA(ERROR) << "File path" << path << " realpath failure";
        return false;
    }
    realPath = std::string(buffer);
    return true;
}

fd_t Open(const char *file, int flag)
{
    return open(file, flag);
}

void Close(fd_t fd)
{
    close(fd);
}

int64_t GetFileSizeByFd(fd_t fd)
{
    return lseek(fd, 0, SEEK_END);
}

void *FileMmap(fd_t fd, uint64_t size, uint64_t offset, [[maybe_unused]] fd_t *extra)
{
    void *addr = mmap(nullptr, size, PAGE_PROT_READWRITE, MAP_PRIVATE, fd, offset);
    LOG_ECMA_IF(addr == nullptr, FATAL) << "mmap fail";
    return addr;
}

int FileUnMap(void *addr, uint64_t size, [[maybe_unused]] fd_t *extra)
{
    return munmap(addr, size);
}
}  // namespace panda::ecmascript
