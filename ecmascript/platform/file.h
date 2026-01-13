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

#include "ecmascript/platform/map.h"

namespace panda::ecmascript {
class JSThread;
#ifdef PANDA_TARGET_WINDOWS
using fd_t = HANDLE;
#define INVALID_FD INVALID_HANDLE_VALUE

#define FILE_RDONLY GENERIC_READ
#define FILE_WRONLY GENERIC_WRITE
#define FILE_RDWR (GENERIC_READ | GENERIC_WRITE)
#define FILE_CREAT CREATE_ALWAYS
#define FILE_TRUNC TRUNCATE_EXISTING

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
#define FILE_CREAT O_CREAT
#define FILE_TRUNC O_TRUNC
#endif

#define FILE_SUCCESS 1
#define FILE_FAILED 0

#define FILE_MS_SYNC 4 // Synchronous memory sync

std::string PUBLIC_API GetFileDelimiter();
std::string PUBLIC_API GetPathSeparator();
bool PUBLIC_API RealPath(const std::string &path, std::string &realPath, bool readOnly = true);
bool PUBLIC_API RealPathByChar(const char *path, char *realPath, int rowLength, bool readOnly);
void DPrintf(fd_t fd, const std::string &buffer);
void PUBLIC_API FdsanExchangeOwnerTag(fd_t fd);
void PUBLIC_API Close(fd_t fd);
void FSync(fd_t fd);
MemMap PUBLIC_API FileMap(const char *fileName, int flag, int prot, int64_t offset = 0);
MemMap PUBLIC_API CreateFileMap([[maybe_unused]] const char *fileName, [[maybe_unused]] int fileSize,
                                [[maybe_unused]] int flag, [[maybe_unused]] int prot);
MemMap PUBLIC_API FileMapForAlignAddressByFd(const fd_t fd, int prot, int64_t offset, uint32_t offStart);
int PUBLIC_API FileUnMap(MemMap addr);
int PUBLIC_API FileSync(MemMap addr, int flag);
CString ResolveFilenameFromNative(JSThread *thread, const CString &dirname, CString request);
bool PUBLIC_API FileExist(const char *filename);
int PUBLIC_API Unlink(const char *filename);
void *LoadLib(const std::string &libname);
void *FindSymbol(void *handle, const char *symbol);
int CloseLib(void *handle);
char *LoadLibError();
void PUBLIC_API DeleteFilesWithSuffix(const std::string &dirPath, const std::string &suffix);

class MemMapScope {
public:
    MemMapScope(MemMap memMap) : memMap_(memMap) {}
    ~MemMapScope()
    {
        FileUnMap(memMap_);
    }

private:
    MemMap memMap_ {};
};

class FileMemMapReader {
public:
    using ReadFailedCallback = std::function<void()>;
    FileMemMapReader(const MemMap &memMap, ReadFailedCallback readFailedCallback, CString logTag) : memMap_(memMap),
        readFailedCallback_(std::move(readFailedCallback)), readPtr_(static_cast<uint8_t *>(memMap.GetOriginAddr())),
        remainingSize_(memMap.GetSize()), fileSize_(memMap.GetSize()), logTag_(std::move(logTag)) {}

    template<typename T>
    bool ReadSingleData(T *readDst, const size_t readSize, const CString &message)
    {
        if (remainingSize_ < readSize) {
            LOG_ECMA(ERROR) << logTag_ << " read " << message << " remainingSize not sufficient";
            readFailedCallback_();
            return false;
        }
        if (memcpy_s(readDst, readSize, readPtr_, readSize) != EOK) {
            LOG_ECMA(ERROR) << logTag_ << " memcpy_s read " << message << " failed";
            readFailedCallback_();
            return false;
        }
        Step(readSize);
        return true;
    }

    bool ReadString(CString &readDst, const size_t readSize, const CString &message)
    {
        readDst.reserve(readSize);
        if (remainingSize_ < readSize) {
            LOG_ECMA(ERROR) << logTag_ << " read " << message << " remainingSize not sufficient";
            readFailedCallback_();
            return false;
        }
        readDst.assign(reinterpret_cast<const char*>(readPtr_), readSize);
        Step(readSize);
        return true;
    }

    template<typename T>
    bool ReadFromOffset(T *readDst, const size_t readSize, const size_t offset, const CString &message)
    {
        if (fileSize_ < offset + readSize) {
            LOG_ECMA(ERROR) << logTag_ << " read " << message << " fileSize_ not sufficient";
            readFailedCallback_();
            return false;
        }
        uint8_t *readPtr = static_cast<uint8_t *>(memMap_.GetOriginAddr());
        if (memcpy_s(readDst, readSize, readPtr + offset, readSize) != EOK) {
            LOG_ECMA(ERROR) << logTag_ << " memcpy_s read " << message << " failed";
            readFailedCallback_();
            return false;
        }
        return true;
    }

    void Step(const size_t stepSize)
    {
        readPtr_ += stepSize;
        remainingSize_ -= stepSize;
    }

    uint8_t *GetReadPtr() const
    {
        return readPtr_;
    }

private:
    MemMap memMap_ {};
    ReadFailedCallback readFailedCallback_ {};
    uint8_t *readPtr_ { nullptr };
    size_t remainingSize_ { 0 };
    size_t fileSize_ { 0 };
    CString logTag_ {};
};

class FileMemMapWriter {
public:
    FileMemMapWriter(const MemMap &memMap, CString logTag) : memMap_(memMap),
        writePtr_(static_cast<uint8_t *>(memMap.GetOriginAddr())),
        fileSize_(memMap.GetSize()), logTag_(std::move(logTag)) {}

    template<typename T>
    bool WriteSingleData(T *writeSrc, const size_t writeSize, const CString &message)
    {
        if (memcpy_s(writePtr_, writeSize, writeSrc, writeSize) != EOK) {
            LOG_ECMA(ERROR) << logTag_ << " memcpy_s write " << message << " failed";
            return false;
        }
        writePtr_ += writeSize;
        return true;
    }

    uint8_t *GetWritePtr() const
    {
        return writePtr_;
    }

    bool WriteAlignUpPadding(const size_t paddingSize)
    {
        if (paddingSize <= 0) {
            return true;
        }
        if (memset_s(writePtr_, paddingSize, 0, paddingSize) != EOK) {
            LOG_ECMA(ERROR) << logTag_ << " memset_s WriteAlignUpPadding " << paddingSize << " failed";
            return false;
        }
        writePtr_ += paddingSize;
        return true;
    }

private:
    MemMap memMap_ {};
    uint8_t *writePtr_ { nullptr };
    size_t fileSize_ { 0 };
    CString logTag_ {};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_FILE_H
