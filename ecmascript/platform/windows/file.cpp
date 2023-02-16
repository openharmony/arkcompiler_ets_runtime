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
#include <fileapi.h>
#include <shlwapi.h>

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
#include "ecmascript/platform/map.h"

namespace panda::ecmascript {
std::string GetFileDelimiter()
{
    return ";";
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

// use CreateFile instead of _open to work with CreateFileMapping
fd_t Open(const char *file, int flag)
{
    fd_t fd = CreateFile(file, flag, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    return fd;
}

void DPrintf(fd_t fd, const std::string &buffer)
{
    LOG_ECMA(DEBUG) << "Unsupport dprintf fd(" << fd << ") in windows, buffer:" << buffer;
}

void FSync(fd_t fd)
{
    LOG_ECMA(DEBUG) << "Unsupport fsync fd(" << fd << ") in windows";
}

void Close(fd_t fd)
{
    CloseHandle(fd);
}

int64_t GetFileSizeByFd(fd_t fd)
{
    LARGE_INTEGER size;
    if (!GetFileSizeEx(fd, &size)) {
        LOG_ECMA(ERROR) << "GetFileSize failed with error code:" << GetLastError();
        return -1;
    }
    return size.QuadPart;
}

void *FileMmap(fd_t fd, uint64_t size, uint64_t offset, fd_t *extra)
{
    // 32: high 32 bits
    *extra = CreateFileMapping(fd, NULL, PAGE_PROT_READ, size >> 32, size & 0xffffffff, nullptr);
    if (*extra == nullptr) {
        LOG_ECMA(ERROR) << "CreateFileMapping failed with error code:" << GetLastError();
        return nullptr;
    }
    // 32: high 32 bits
    void *addr = MapViewOfFile(*extra, FILE_MAP_READ, offset >> 32, offset & 0xffffffff, size);
    if (addr == nullptr) {
        LOG_ECMA(ERROR) << "MapViewOfFile failed with error code:" << GetLastError();
        CloseHandle(*extra);
    }
    return addr;
}

int FileUnMap(void *addr, [[maybe_unused]] uint64_t size, fd_t *extra)
{
    if (UnmapViewOfFile(addr) == 0) {
        return FILE_FAILED;
    }
    if (CloseHandle(*extra) == 0) {
        return FILE_FAILED;
    }
    return FILE_SUCCESS;
}

JSHandle<EcmaString> ResolveFilenameFromNative(JSThread *thread, JSTaggedValue dirname,
                                               JSTaggedValue request)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString fullname;
    CString resolvedFilename;
    CString dirnameStr = ConvertToString(EcmaString::Cast(dirname.GetTaggedObject()));
    CString requestStr = ConvertToString(EcmaString::Cast(request.GetTaggedObject()));
    int suffixEnd = static_cast<int>(requestStr.find_last_of('.'));
    if (suffixEnd == -1) {
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
    }
    if (requestStr[1] == ':') { // absoluteFilePath
        fullname = requestStr.substr(0, suffixEnd) + ".abc";
    } else {
        int pos = static_cast<int>(dirnameStr.find_last_of('\\'));
        if (pos == -1) {
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
        }
        fullname = dirnameStr.substr(0, pos + 1) + requestStr.substr(0, suffixEnd) + ".abc";
    }

    std::string relativePath = ConvertToStdString(fullname);
    std::string absPath = "";
    if (RealPath(relativePath, absPath)) {
        resolvedFilename = ConvertToString(absPath);
        return factory->NewFromUtf8(resolvedFilename);
    }
    CString msg = "resolve absolute path fail";
    THROW_NEW_ERROR_AND_RETURN_HANDLE(thread, ErrorType::REFERENCE_ERROR, EcmaString, msg.c_str());
}
}  // namespace panda::ecmascript
