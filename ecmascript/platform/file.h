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

#ifndef ECMASCRIPT_PLATFORM_FILE_H
#define ECMASCRIPT_PLATFORM_FILE_H

#ifdef PANDA_TARGET_WINDOWS
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#else
#include <fcntl.h>
#endif

#include <string>

namespace panda::ecmascript {
#ifdef PANDA_TARGET_WINDOWS
using fd_t = HANDLE;
#define INVALID_FD INVALID_HANDLE_VALUE

#define FILE_RDONLY GENERIC_READ
#define FILE_WRONLY GENERIC_WRITE
#define FILE_RDWR (GENERIC_READ | GENERIC_WRITE)
#else
using fd_t = int;
#define INVALID_FD (-1)

#define FILE_RDONLY O_RDONLY
#define FILE_WRONLY O_WRONLY
#define FILE_RDWR O_RDWR
#endif

#define FILE_SUCCESS 1
#define FILE_FAILED 0

std::string GetFileDelimiter();
bool RealPath(const std::string &path, std::string &realPath, bool readOnly = true);
int64_t GetFileSizeByFd(fd_t fd);
fd_t Open(const char *file, int flag);
void Close(fd_t fd);
void *FileMmap(fd_t fd, uint64_t size, uint64_t offset, fd_t *extra);
int FileUnMap(void *addr, uint64_t size, fd_t *extra);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_FILE_H
