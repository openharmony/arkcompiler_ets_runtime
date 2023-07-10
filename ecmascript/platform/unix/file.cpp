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
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/platform/map.h"

namespace panda::ecmascript {
std::string GetFileDelimiter()
{
    return ":";
}

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
        LOG_ECMA(ERROR) << "File path:" << path << " realpath failure";
        return false;
    }
    realPath = std::string(buffer);
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

int FileUnMap(MemMap addr)
{
    return munmap(addr.GetOriginAddr(), addr.GetSize());
}

JSHandle<EcmaString> ResolveFilenameFromNative(JSThread *thread, JSTaggedValue dirname,
                                               JSTaggedValue request)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CString fullname;
    CString resolvedFilename;
    CString dirnameStr = ConvertToString(EcmaString::Cast(dirname.GetTaggedObject()));
    CString requestStr = ConvertToString(EcmaString::Cast(request.GetTaggedObject()));

    if (requestStr.find("./") == 0) {
        requestStr = requestStr.substr(2); // 2 : delete './'
    }
    int suffixEnd = static_cast<int>(requestStr.find_last_of('.'));
    if (suffixEnd == -1) {
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
    }
    if (requestStr[0] == '/') { // absolute FilePath
        fullname = requestStr.substr(0, suffixEnd) + ".abc";
    } else {
        int pos = static_cast<int>(dirnameStr.find_last_of('/'));
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

bool FileExist(const char *filename)
{
    return (access(filename, 0) != -1);
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

    CString soName = base::PathHelper::GetStrippedModuleName(ConvertToString(module->GetEcmaModuleRecordName()));
    return unloadNativeModuleCallback(soName.c_str());
}
}  // namespace panda::ecmascript
