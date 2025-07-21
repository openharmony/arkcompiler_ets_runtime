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

#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <climits>
#include <fileapi.h>

#ifdef ERROR
#undef ERROR
#endif

#ifdef VOID
#undef VOID
#endif

#ifdef CONST
#undef CONST
#endif

#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
std::string GetFileDelimiter()
{
    return ";";
}

std::string GetPathSeparator()
{
    return "\\";
}

bool RealPath(const std::string &path, std::string &realPath, [[maybe_unused]] bool readOnly)
{
    realPath = "";
    if (path.empty() || path.size() > PATH_MAX) {
        LOG_ECMA(WARN) << "File path is illeage";
        return false;
    }
    char buffer[PATH_MAX] = { '\0' };
    if (!_fullpath(buffer, path.c_str(), sizeof(buffer) - 1)) {
        LOG_ECMA(WARN) << "File path:" << path << " full path failure";
        return false;
    }
    realPath = std::string(buffer);
    return true;
}

bool RealPathByChar(const char *path, char *realPath, int rowLength, bool readOnly)
{
    (void)path;
    (void)realPath;
    (void)rowLength;
    (void)readOnly;
    return false;
}

void DPrintf(fd_t fd, const std::string &buffer)
{
    LOG_ECMA(DEBUG) << "Unsupport dprintf fd(" << fd << ") in windows, buffer:" << buffer;
}

void FSync(fd_t fd)
{
    LOG_ECMA(DEBUG) << "Unsupport fsync fd(" << fd << ") in windows";
}

void FdsanExchangeOwnerTag(fd_t fd)
{
    LOG_ECMA(DEBUG) << "Unsupport FdsanExchangeOwnerTag fd(" << fd << ") in windows";
}

void Close(fd_t fd)
{
    CloseHandle(fd);
}

MemMap FileMap(const char *fileName, int flag, int prot, int64_t offset)
{
    if (prot == PAGE_PROT_READWRITE) {
        flag |= FILE_RDONLY | FILE_WRONLY;
    }
    fd_t fd = CreateFile(fileName, flag, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fd == INVALID_FD) {
        LOG_ECMA(ERROR) << fileName << " file open failed";
        return MemMap();
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fd, &fileSize)) {
        CloseHandle(fd);
        LOG_ECMA(ERROR) << "GetFileSize failed with error code:" << GetLastError();
        return MemMap();
    }
    auto size = fileSize.QuadPart;
    if (size <= 0) {
        CloseHandle(fd);
        LOG_HOST_TOOL_ERROR << fileName << " file is empty";
        return MemMap();
    }

    // 32: high 32 bits
    fd_t extra = CreateFileMapping(fd, NULL, prot, size >> 32, size & 0xffffffff, nullptr);
    if (extra == nullptr) {
        CloseHandle(fd);
        LOG_ECMA(ERROR) << "CreateFileMapping failed with error code:" << GetLastError();
        return MemMap();
    }
    int accessor = (prot == PAGE_PROT_READ) ? FILE_MAP_READ : FILE_MAP_WRITE;
    void *addr = MapViewOfFile(extra, accessor, offset >> 32, offset & 0xffffffff, size);
    CloseHandle(extra);
    CloseHandle(fd);
    if (addr == nullptr) {
        LOG_ECMA(ERROR) << "MapViewOfFile failed with error code:" << GetLastError();
    }
    return MemMap(addr, size);
}

MemMap CreateFileMap([[maybe_unused]] const char *fileName, [[maybe_unused]] int fileSize,
                     [[maybe_unused]] int flag, [[maybe_unused]] int prot)
{
    LOG_ECMA(INFO) << "Unsupport CreateFileMap";
    return MemMap(nullptr, 0);
}

MemMap FileMapForAlignAddressByFd(const fd_t fd, int prot, int64_t offset, uint32_t offStart)
{
    // AOT not used, previewer used
    LOG_ECMA(INFO) << "Don't used fd:" << fd
                   << " prot:" << prot << " offset:" << offset << " offStart:" << offStart;
    return MemMap();
}

int FileUnMap(MemMap addr)
{
    if (UnmapViewOfFile(addr.GetOriginAddr()) == 0) {
        return FILE_FAILED;
    }
    return FILE_SUCCESS;
}

int FileSync(MemMap addr, int flag)
{
    LOG_ECMA(INFO) << "Don't used MemMap:" << addr.GetOriginAddr() << ", flag:" << flag;
    return -1;
}

CString ResolveFilenameFromNative(JSThread *thread, const CString &dirname,
                                  CString request)
{
    std::string relativePath;
    int suffixEnd = static_cast<int>(request.find_last_of('.'));
    if (request[1] == ':') { // absoluteFilePath
        relativePath = request.substr(0, suffixEnd) + ".abc";
    } else {
        int pos = static_cast<int>(dirname.find_last_of('\\'));
        relativePath = dirname.substr(0, pos + 1) + request.substr(0, suffixEnd) + ".abc";
    }

    std::string absPath;
    if (RealPath(relativePath, absPath)) {
        return absPath.c_str();
    }
    THROW_REFERENCE_ERROR_AND_RETURN(thread, "resolve absolute path fail", CString());
}

bool FileExist(const char *filename)
{
    return (_access(filename, 0) != -1);
}

int Unlink(const char *filename)
{
    return _unlink(filename);
}

void *LoadLib([[maybe_unused]] const std::string &liname)
{
    LOG_ECMA(INFO) << "Unsupport LoadLib";
    return nullptr;
}

void *FindSymbol([[maybe_unused]] void *handle, [[maybe_unused]] const char *symbol)
{
    LOG_ECMA(INFO) << "Unsupport FindSymbol";
    return nullptr;
}

int CloseLib([[maybe_unused]] void *handle)
{
    LOG_ECMA(INFO) << "Unsupport CloseLib";
    return 0;
}

char *LoadLibError()
{
    LOG_ECMA(INFO) << "Unsupport LoadLibError";
    return nullptr;
}

void DeleteFilesWithSuffix(const std::string &dirPath, const std::string &suffix)
{
    LOG_ECMA(INFO) << "Unsupport dirPath: " << dirPath << ", suffix: " << suffix;
}
}  // namespace panda::ecmascript
