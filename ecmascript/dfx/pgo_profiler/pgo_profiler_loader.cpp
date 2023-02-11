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

#include "ecmascript/dfx/pgo_profiler/pgo_profiler_loader.h"

#include <string>

#include "ecmascript/base/string_helper.h"
#include "ecmascript/dfx/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/method.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript {
bool PGOProfilerLoader::Load(const std::string &inPath, uint32_t hotnessThreshold)
{
    hotnessThreshold_ = hotnessThreshold;
    isLoaded_ = false;
    hotnessMethods_.clear();
    if (inPath.empty()) {
        return false;
    }
    std::string realPath;
    if (!RealPath(inPath, realPath)) {
        return false;
    }

    static const std::string endString = ".ap";
    if (realPath.compare(realPath.length() - endString.length(), endString.length(), endString)) {
        LOG_ECMA(ERROR) << "The file path( " << realPath << ") does not end with .ap";
        return false;
    }
    LOG_ECMA(INFO) << "Load profiler from file:" << realPath;

    fd_t fd = Open(realPath.c_str(), FILE_RDONLY);
    if (UNLIKELY(fd == INVALID_FD)) {
        LOG_ECMA(ERROR) << "open file failed";
        return false;
    }
    int64_t fileSize = GetFileSizeByFd(fd);
    if (fileSize == -1) {
        Close(fd);
        LOG_ECMA(ERROR) << "GetFileSize failed";
        return false;
    }

    fd_t extra = INVALID_FD;
    void *addr = FileMmap(fd, fileSize, 0, &extra);
    if (addr == nullptr) {
        Close(fd);
        LOG_ECMA(ERROR) << "file mmap failed";
        return false;
    }

    if (!ParseProfilerHeader(&addr)) {
        FileUnMap(addr, fileSize, &extra);
        Close(fd);
        LOG_ECMA(ERROR) << "Parse profiler header failure";
        return false;
    }
    if (!ParsePandaFileInfo(&addr)) {
        FileUnMap(addr, fileSize, &extra);
        Close(fd);
        LOG_ECMA(ERROR) << "Parse profiler panda file info failure";
        return false;
    }
    ParseProfiler(&addr);
    FileUnMap(addr, fileSize, &extra);
    Close(fd);

    isLoaded_ = true;
    return true;
}

bool PGOProfilerLoader::Verify(uint32_t checksum)
{
    isVerifySuccess_ = false;
    if (!isLoaded_) {
        return false;
    }
    for (auto info : pandaFileProfilerInfos_) {
        if (checksum == info.GetChecksum()) {
            isVerifySuccess_ = true;
            return true;
        }
    }
    LOG_ECMA(ERROR) << "Verify profiler failure";
    return false;
}

bool PGOProfilerLoader::LoadAndVerify(const std::string &inPath, uint32_t hotnessThreshold, uint32_t checksum)
{
    // When the file name is empty, Enter full compiler mode.
    if (inPath.empty()) {
        return true;
    }

    if (Load(inPath, hotnessThreshold) && Verify(checksum)) {
        return true;
    }
    return false;
}

bool PGOProfilerLoader::ParseProfilerHeader(void **buffer)
{
    if (memcpy_s(&header_, sizeof(PGOProfilerHeader), *buffer, sizeof(PGOProfilerHeader)) != EOK) {
        LOG_FULL(FATAL) << "memcopy_s failed";
        return false;
    }
    *buffer = ToVoidPtr(ToUintPtr(*buffer) + sizeof(PGOProfilerHeader));
    return header_.Verify();
}

bool PGOProfilerLoader::ParsePandaFileInfo(void **buffer)
{
    uint32_t size = *(reinterpret_cast<uint32_t *>(*buffer));
    *buffer = ToVoidPtr(ToUintPtr(*buffer) + sizeof(uint32_t));
    pandaFileProfilerInfos_.resize(size);
    for (uint32_t i = 0; i < size; i++) {
        pandaFileProfilerInfos_.emplace_back(*(reinterpret_cast<PandaFileProfilerInfo *>(*buffer)));
        *buffer = ToVoidPtr(ToUintPtr(*buffer) + sizeof(PandaFileProfilerInfo));
    }
    LOG_ECMA(DEBUG) << "Profiler panda file count:" << size;
    return true;
}

void PGOProfilerLoader::ParseProfiler(void **buffer)
{
    uint32_t recordNameCount = *(reinterpret_cast<uint32_t *>(*buffer));
    *buffer = ToVoidPtr(ToUintPtr(*buffer) + sizeof(uint32_t));
    for (uint32_t i = 0; i < recordNameCount; i++) {
        auto recordName = ConvertToString(reinterpret_cast<char *>(*buffer));
        *buffer = ToVoidPtr(ToUintPtr(*buffer) + recordName.size() + 1);
        std::unordered_set<EntityId> methodIds;

        uint32_t methodCount = *(reinterpret_cast<uint32_t *>(*buffer));
        *buffer = ToVoidPtr(ToUintPtr(*buffer) + sizeof(uint32_t));
        for (uint32_t j = 0; j < methodCount; j++) {
            MethodProfilerInfo *info = reinterpret_cast<MethodProfilerInfo *>(*buffer);
            if (info->GetCount() >= hotnessThreshold_) {
                methodIds.emplace(info->GetMethodId());
                LOG_ECMA(DEBUG) << "Method:" << info->GetMethodId() << "/" << info->GetCount()
                                << "/" << std::to_string(static_cast<int>(info->GetSampleMode()))
                                << "/" << info->GetMethodName() << "/" << info->GetMethodLength();
            }
            *buffer = ToVoidPtr(ToUintPtr(*buffer) + info->Size());
        }
        if (!methodIds.empty()) {
            hotnessMethods_.emplace(recordName, methodIds);
        }
    }
}

bool PGOProfilerLoader::Match(const CString &recordName, EntityId methodId)
{
    if (!isLoaded_) {
        return true;
    }
    if (!isVerifySuccess_) {
        return false;
    }
    auto hotnessMethodSet = hotnessMethods_.find(recordName);
    if (hotnessMethodSet == hotnessMethods_.end()) {
        return false;
    }
    return hotnessMethodSet->second.find(methodId) != hotnessMethodSet->second.end();
}
} // namespace panda::ecmascript
