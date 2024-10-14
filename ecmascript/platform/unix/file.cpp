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

#include <cerrno>
#include <climits>
#include <sys/mman.h>
#include <unistd.h>

#include "ecmascript/base/path_helper.h"
#include "ecmascript/module/js_module_source_text.h"

namespace panda::ecmascript {
std::string GetFileDelimiter()
{
    return ":";
}

std::string GetPathSeparator()
{
    return "/";
}

bool RealPath(const std::string &path, std::string &realPath, bool readOnly)
{
    if (path.empty() || path.size() > PATH_MAX) {
        LOG_ECMA(WARN) << "File path is illegal";
        return false;
    }
    char buffer[PATH_MAX] = { '\0' };
    if (!realpath(path.c_str(), buffer)) {
        // Maybe file does not exist.
        if (!readOnly && errno == ENOENT) {
            realPath = path;
            return true;
        }
        LOG_ECMA(ERROR) << "File path: " << path << " realpath failure. errno: " << errno;
        return false;
    }
    realPath = std::string(buffer);
    return true;
}

bool RealPathByChar(const char *path, char *realPath, int rowLength, bool readOnly)
{
    if (strlen(path) == 0 || strlen(path) > PATH_MAX) {
        return false;
    }
    char buffer[PATH_MAX] = { '\0' };
    if (!realpath(path, buffer)) {
        // Maybe file does not exist.
        if (!readOnly && errno == ENOENT) {
            if (strcpy_s(realPath, rowLength, path) != 0) {
                return false;
            }
            return true;
        }
        return false;
    }
    if (strcpy_s(realPath, rowLength, buffer) != 0) {
        return false;
    }
    return true;
}

void DPrintf(fd_t fd, const std::string &buffer)
{
    int ret = dprintf(fd, "%s", buffer.c_str());
    if (ret < 0) {
        LOG_ECMA(DEBUG) << "dprintf fd(" << fd << ") failed";
    }
}

void FSync(fd_t fd)
{
    int ret = fsync(fd);
    if (ret < 0) {
        LOG_ECMA(DEBUG) << "fsync fd(" << fd << ") failed";
    }
}

void Close(fd_t fd)
{
    close(fd);
}

MemMap FileMap(const char *fileName, int flag, int prot, int64_t offset)
{
    fd_t fd = open(fileName, flag);
    if (fd == INVALID_FD) {
        LOG_ECMA(ERROR) << fileName << " file open failed";
        return MemMap();
    }

    off_t size = lseek(fd, 0, SEEK_END);
    if (size <= 0) {
        close(fd);
        LOG_ECMA(ERROR) << fileName << " file is empty";
        return MemMap();
    }

    void *addr = mmap(nullptr, size, prot, MAP_PRIVATE, fd, offset);
    close(fd);
    return MemMap(addr, size);
}

MemMap FileMapForAlignAddress(const char *fileName, int flag, int prot,
                              int64_t offset, uint32_t offStart)
{
    fd_t fd = open(fileName, flag);
    if (fd == INVALID_FD) {
        LOG_ECMA(ERROR) << fileName << " file open failed";
        return MemMap();
    }

    off_t size = lseek(fd, 0, SEEK_END);
    if (size <= 0) {
        close(fd);
        LOG_ECMA(ERROR) << fileName << " file is empty";
        return MemMap();
    }
    void *addr = mmap(nullptr, size + offset - offStart, prot, MAP_PRIVATE, fd, offStart);
    close(fd);
    return MemMap(addr, size);
}

int FileUnMap(MemMap addr)
{
    return munmap(addr.GetOriginAddr(), addr.GetSize());
}

CString ResolveFilenameFromNative(JSThread *thread, const CString &dirname,
                                  CString request)
{
    std::string relativePath;
    if (request.find("./") == 0) {
        request = request.substr(2); // 2 : delete './'
    }
    int suffixEnd = static_cast<int>(request.find_last_of('.'));
    if (request[0] == '/') { // absolute FilePath
        relativePath = request.substr(0, suffixEnd) + ".abc";
    } else {
        int pos = static_cast<int>(dirname.find_last_of('/'));
        relativePath = dirname.substr(0, pos + 1) + request.substr(0, suffixEnd) + ".abc";
    }

    std::string absPath = "";
    if (RealPath(relativePath.c_str(), absPath)) {
        return absPath.c_str();
    }
    THROW_REFERENCE_ERROR_AND_RETURN(thread, "resolve absolute path fail", CString());
}

bool FileExist(const char *filename)
{
    return (access(filename, F_OK) != -1);
}

int Unlink(const char *filename)
{
    return unlink(filename);
}

bool TryToRemoveSO(JSThread *thread, JSHandle<SourceTextModule> module)
{
    UnloadNativeModuleCallback unloadNativeModuleCallback = thread->GetEcmaVM()->GetUnloadNativeModuleCallback();
    if (unloadNativeModuleCallback == nullptr) {
        LOG_ECMA(ERROR) << "unloadNativeModuleCallback is nullptr";
        return false;
    }

    CString soName = base::PathHelper::GetStrippedModuleName(module->GetEcmaModuleRecordNameString());
    return unloadNativeModuleCallback(soName.c_str());
}

void *LoadLib(const std::string &libname)
{
    return dlopen(libname.c_str(), RTLD_NOW);
}

void *FindSymbol(void *handle, const char *symbol)
{
    return dlsym(handle, symbol);
}

int CloseLib(void *handle)
{
    return dlclose(handle);
}

char *LoadLibError()
{
    return dlerror();
}

}  // namespace panda::ecmascript
