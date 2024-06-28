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
#include <dlfcn.h>
#endif

#include <string>

#include "ecmascript/ecma_string.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class SourceTextModule;
#ifdef PANDA_TARGET_WINDOWS
using fd_t = HANDLE;
#define INVALID_FD INVALID_HANDLE_VALUE

#define FILE_RDONLY GENERIC_READ
#define FILE_WRONLY GENERIC_WRITE
#define FILE_RDWR (GENERIC_READ | GENERIC_WRITE)

#ifdef ERROR
#undef ERROR
#endif

#ifdef VOID
#undef VOID
#endif

#ifdef CONST
#undef CONST
#endif
#else
using fd_t = int;
#define INVALID_FD (-1)

#define FILE_RDONLY O_RDONLY
#define FILE_WRONLY O_WRONLY
#define FILE_RDWR O_RDWR
#endif

#define FILE_SUCCESS 1
#define FILE_FAILED 0

std::string PUBLIC_API GetFileDelimiter();
std::string PUBLIC_API GetPathSeparator();
bool PUBLIC_API RealPath(const std::string &path, std::string &realPath, bool readOnly = true);
bool PUBLIC_API RealPathByChar(const char *path, char *realPath, int rowLength, bool readOnly);
void DPrintf(fd_t fd, const std::string &buffer);
void Close(fd_t fd);
void FSync(fd_t fd);
MemMap PUBLIC_API FileMap(const char *fileName, int flag, int prot, int64_t offset = 0);
MemMap PUBLIC_API FileMapForAlignAddress(const char *fileName, int flag, int prot,
                                         int64_t offset, uint32_t offStart);
int PUBLIC_API FileUnMap(MemMap addr);
CString ResolveFilenameFromNative(JSThread *thread, const CString &dirname, CString request);
bool PUBLIC_API FileExist(const char *filename);
int PUBLIC_API Unlink(const char *filename);
bool TryToRemoveSO(JSThread *thread, JSHandle<SourceTextModule> module);
void *LoadLib(const std::string &libname);
void *FindSymbol(void *handle, const char *symbol);
int CloseLib(void *handle);
char *LoadLibError();
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_FILE_H
