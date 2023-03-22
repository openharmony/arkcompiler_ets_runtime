/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/pgo_profiler/pgo_profiler_info.h"

#include "ecmascript/base/bit_helper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_saver.h"

namespace panda::ecmascript {
static const std::string ELEMENT_SEPARATOR = "/";
static const std::string BLOCK_SEPARATOR = ",";
static const std::string BLOCK_START = ":";
static const std::string ARRAY_START = "[";
static const std::string ARRAY_END = "]";
static const std::string NEW_LINE = "\n";
static const std::string SPACE = " ";
static const std::string BLOCK_AND_ARRAY_START = BLOCK_START + SPACE + ARRAY_START + SPACE;
static const std::string VERSION_HEADER = "Profiler Version" + BLOCK_START + SPACE;
static const std::string PANDA_FILE_INFO_HEADER = "Panda file sumcheck list" + BLOCK_AND_ARRAY_START;

bool PGOProfilerHeader::ParseFromBinary(void *buffer, PGOProfilerHeader **header)
{
    auto in = reinterpret_cast<PGOProfilerHeader *>(buffer);
    if (in->Verify()) {
        size_t desSize = in->Size();
        if (desSize > LastSize()) {
            LOG_ECMA(ERROR) << "header size error, expected size is less than " << LastSize()
                            << ", but got " << desSize;
            return false;
        }
        Build(header, desSize);
        if (memcpy_s(*header, desSize, in, in->Size()) != EOK) {
            UNREACHABLE();
        }
        return true;
    }
    return false;
}

void PGOProfilerHeader::ProcessToBinary(std::ofstream &fileStream) const
{
    fileStream.seekp(0);
    fileStream.write(reinterpret_cast<const char *>(this), Size());
}

bool PGOProfilerHeader::ParseFromText(std::ifstream &stream)
{
    std::string header;
    if (std::getline(stream, header)) {
        if (header.empty()) {
            return false;
        }
        auto index = header.find(BLOCK_START);
        if (index == std::string::npos) {
            return false;
        }
        auto version = header.substr(index + 1);
        if (!SetVersionInner(version)) {
            return false;
        }
        if (!Verify()) {
            return false;
        }
        return true;
    }
    return false;
}

bool PGOProfilerHeader::ProcessToText(std::ofstream &stream) const
{
    if (!Verify()) {
        return false;
    }
    stream << VERSION_HEADER << GetVersionInner() << NEW_LINE;
    return true;
}

void PGOPandaFileInfos::ParseFromBinary(void *buffer, SectionInfo *const info)
{
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        pandaFileInfos_.emplace(*base::ReadBufferInSize<PandaFileInfo>(&addr));
    }
    LOG_ECMA(DEBUG) << "Profiler panda file count:" << info->number_;
}

void PGOPandaFileInfos::ProcessToBinary(std::ofstream &fileStream, SectionInfo *info) const
{
    fileStream.seekp(info->offset_);
    info->number_ = pandaFileInfos_.size();
    for (auto localInfo : pandaFileInfos_) {
        fileStream.write(reinterpret_cast<char *>(&localInfo), localInfo.Size());
    }
    info->size_ = static_cast<uint32_t>(fileStream.tellp()) - info->offset_;
}

bool PGOPandaFileInfos::ParseFromText(std::ifstream &stream)
{
    std::string pandaFileInfo;
    while (std::getline(stream, pandaFileInfo)) {
        if (pandaFileInfo.empty()) {
            continue;
        }

        size_t start = pandaFileInfo.find_first_of(ARRAY_START);
        size_t end = pandaFileInfo.find_last_of(ARRAY_END);
        if (start == std::string::npos || end == std::string::npos || start > end) {
            return false;
        }
        auto content = pandaFileInfo.substr(start + 1, end - (start + 1) - 1);
        std::vector<std::string> infos = base::StringHelper::SplitString(content, BLOCK_SEPARATOR);
        for  (auto checksum : infos) {
            uint32_t result;
            if (!base::StringHelper::StrToUInt32(checksum.c_str(), &result)) {
                LOG_ECMA(ERROR) << "checksum: " << checksum << " parse failed";
                return false;
            }
            Sample(result);
        }
        return true;
    }
    return true;
}

void PGOPandaFileInfos::ProcessToText(std::ofstream &stream) const
{
    std::string pandaFileInfo = NEW_LINE + PANDA_FILE_INFO_HEADER;
    bool isFirst = true;
    for (auto &info : pandaFileInfos_) {
        if (!isFirst) {
            pandaFileInfo += BLOCK_SEPARATOR + SPACE;
        } else {
            isFirst = false;
        }
        pandaFileInfo += std::to_string(info.GetChecksum());
    }

    pandaFileInfo += (SPACE + ARRAY_END + NEW_LINE);
    stream << pandaFileInfo;
}

bool PGOPandaFileInfos::CheckSum(uint32_t checksum) const
{
    if (pandaFileInfos_.find(checksum)  == pandaFileInfos_.end()) {
        LOG_ECMA(ERROR) << "Checksum verification failed. Please ensure that the .abc and .ap match.";
        return false;
    }
    return true;
}

void PGOMethodInfo::ProcessToText(std::string &text) const
{
    text += std::to_string(GetMethodId().GetOffset());
    text += ELEMENT_SEPARATOR;
    text += std::to_string(GetCount());
    text += ELEMENT_SEPARATOR;
    text += GetSampleModeToString();
    text += ELEMENT_SEPARATOR;
    text += GetMethodName();
}

std::vector<std::string> PGOMethodInfo::ParseFromText(const std::string &infoString)
{
    std::vector<std::string> infoStrings = base::StringHelper::SplitString(infoString, ELEMENT_SEPARATOR);
    return infoStrings;
}

bool PGOMethodInfoMap::AddMethod(Chunk *chunk, EntityId methodId, const CString &methodName, SampleMode mode)
{
    auto result = methodInfos_.find(methodId);
    if (result != methodInfos_.end()) {
        auto info = result->second;
        info->IncreaseCount();
        info->SetSampleMode(mode);
        return false;
    } else {
        size_t strlen = methodName.size();
        size_t size = static_cast<size_t>(PGOMethodInfo::Size(strlen));
        void *infoAddr = chunk->Allocate(size);
        auto info = new (infoAddr) PGOMethodInfo(methodId, 1, mode, methodName.c_str());
        methodInfos_.emplace(methodId, info);
        return true;
    }
}

void PGOMethodInfoMap::Merge(Chunk *chunk, PGOMethodInfoMap *methodInfos)
{
    for (auto iter = methodInfos->methodInfos_.begin(); iter != methodInfos->methodInfos_.end(); iter++) {
        auto methodId = iter->first;
        auto fromMethodInfo = iter->second;

        auto result = methodInfos_.find(methodId);
        if (result != methodInfos_.end()) {
            auto toMethodInfo = result->second;
            toMethodInfo->Merge(fromMethodInfo);
        } else {
            size_t len = strlen(fromMethodInfo->GetMethodName());
            size_t size = static_cast<size_t>(PGOMethodInfo::Size(len));
            void *infoAddr = chunk->Allocate(size);
            auto newMethodInfo = new (infoAddr) PGOMethodInfo(methodId, fromMethodInfo->GetCount(),
                fromMethodInfo->GetSampleMode(), fromMethodInfo->GetMethodName());
            methodInfos_.emplace(methodId, newMethodInfo);
        }
        fromMethodInfo->ClearCount();
    }
}

bool PGOMethodInfoMap::ParseFromBinary(uint32_t threshold, void **buffer)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t j = 0; j < secInfo.number_; j++) {
        PGOMethodInfo *info = base::ReadBufferInSize<PGOMethodInfo>(buffer);
        if (!info->IsFilter(threshold)) {
            methodInfos_.emplace(info->GetMethodId(), info);
            LOG_ECMA(DEBUG) << "Method:" << info->GetMethodId() << ELEMENT_SEPARATOR << info->GetCount()
                            << ELEMENT_SEPARATOR << std::to_string(static_cast<int>(info->GetSampleMode()))
                            << ELEMENT_SEPARATOR << info->GetMethodName();
        }
    }
    return !methodInfos_.empty();
}

bool PGOMethodInfoMap::ProcessToBinary(uint32_t threshold, const CString &recordName,
    const SaveTask *task, std::ofstream &stream) const
{
    SectionInfo secInfo;
    std::stringstream methodStream;
    for (auto iter = methodInfos_.begin(); iter != methodInfos_.end(); iter++) {
        LOG_ECMA(DEBUG) << "Method:" << iter->first << ELEMENT_SEPARATOR << iter->second->GetCount()
                        << ELEMENT_SEPARATOR << std::to_string(static_cast<int>(iter->second->GetSampleMode()))
                        << ELEMENT_SEPARATOR << iter->second->GetMethodName();
        if (task && task->IsTerminate()) {
            LOG_ECMA(INFO) << "ProcessProfile: task is already terminate";
            return false;
        }
        auto curMethodInfo = iter->second;
        if (curMethodInfo->IsFilter(threshold)) {
            continue;
        }
        methodStream.write(reinterpret_cast<char *>(curMethodInfo), curMethodInfo->Size());
        secInfo.number_++;
    }
    if (secInfo.number_ > 0) {
        secInfo.offset_ = sizeof(SectionInfo);
        secInfo.size_ = static_cast<uint32_t>(methodStream.tellg());
        stream << recordName << '\0';
        stream.write(reinterpret_cast<char *>(&secInfo), sizeof(SectionInfo));
        stream << methodStream.rdbuf();
        return true;
    }
    return false;
}

bool PGOMethodInfoMap::ParseFromText(Chunk *chunk, uint32_t threshold, const std::vector<std::string> &content)
{
    for (auto infoString : content) {
        std::vector<std::string>  infoStrings = PGOMethodInfo::ParseFromText(infoString);
        if (infoStrings.size() < PGOMethodInfo::METHOD_INFO_COUNT) {
            LOG_ECMA(ERROR) << "method info:" << infoString << " format error";
            return false;
        }
        uint32_t count;
        if (!base::StringHelper::StrToUInt32(infoStrings[PGOMethodInfo::METHOD_COUNT_INDEX].c_str(), &count)) {
            LOG_ECMA(ERROR) << "count: " << infoStrings[PGOMethodInfo::METHOD_COUNT_INDEX] << " parse failed";
            return false;
        }
        SampleMode mode;
        if (!PGOMethodInfo::GetSampleMode(infoStrings[PGOMethodInfo::METHOD_MODE_INDEX], mode)) {
            LOG_ECMA(ERROR) << "mode: " << infoStrings[PGOMethodInfo::METHOD_MODE_INDEX] << " parse failed";
            return false;
        }
        if (count < threshold && mode == SampleMode::CALL_MODE) {
            return true;
        }
        uint32_t methodId;
        if (!base::StringHelper::StrToUInt32(infoStrings[PGOMethodInfo::METHOD_ID_INDEX].c_str(), &methodId)) {
            LOG_ECMA(ERROR) << "method id: " << infoStrings[PGOMethodInfo::METHOD_ID_INDEX] << " parse failed" ;
            return false;
        }
        std::string methodName = infoStrings[PGOMethodInfo::METHOD_NAME_INDEX];

        size_t len = methodName.size();
        void *infoAddr = chunk->Allocate(PGOMethodInfo::Size(len));
        auto info = new (infoAddr) PGOMethodInfo(EntityId(methodId), count, mode, methodName.c_str());
        methodInfos_.emplace(methodId, info);
    }

    return true;
}

void PGOMethodInfoMap::ProcessToText(uint32_t threshold, const CString &recordName, std::ofstream &stream) const
{
    std::string profilerString;
    bool isFirst = true;
    for (auto methodInfoIter : methodInfos_) {
        auto methodInfo = methodInfoIter.second;
        if (methodInfo->IsFilter(threshold)) {
            continue;
        }
        if (isFirst) {
            profilerString += NEW_LINE;
            profilerString += recordName;
            profilerString += BLOCK_AND_ARRAY_START;
            isFirst = false;
        } else {
            profilerString += BLOCK_SEPARATOR + SPACE;
        }
        methodInfo->ProcessToText(profilerString);
    }
    if (!isFirst) {
        profilerString += (SPACE + ARRAY_END + NEW_LINE);
        stream << profilerString;
    }
}

bool PGOMethodIdSet::ParseFromBinary(uint32_t threshold, void **buffer)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t j = 0; j < secInfo.number_; j++) {
        PGOMethodInfo *info = base::ReadBufferInSize<PGOMethodInfo>(buffer);
        if (!info->IsFilter(threshold)) {
            methodIdSet_.emplace(info->GetMethodId());
            LOG_ECMA(DEBUG) << "Method:" << info->GetMethodId() << ELEMENT_SEPARATOR << info->GetCount()
                            << ELEMENT_SEPARATOR << std::to_string(static_cast<int>(info->GetSampleMode()))
                            << ELEMENT_SEPARATOR << info->GetMethodName();
        }
    }

    return methodIdSet_.size() != 0;
}

bool PGORecordDetailInfos::AddMethod(const CString &recordName, EntityId methodId,
    const CString &methodName, SampleMode mode)
{
    auto iter = recordInfos_.find(recordName.c_str());
    PGOMethodInfoMap *curMethodInfos = nullptr;
    if (iter != recordInfos_.end()) {
        curMethodInfos = iter->second;
    } else {
        curMethodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
        recordInfos_.emplace(recordName.c_str(), curMethodInfos);
    }
    return curMethodInfos->AddMethod(chunk_.get(), methodId, methodName, mode);
}

void PGORecordDetailInfos::Merge(const PGORecordDetailInfos &recordInfos)
{
    for (auto iter = recordInfos.recordInfos_.begin(); iter != recordInfos.recordInfos_.end(); iter++) {
        auto recordName = iter->first;
        auto fromMethodInfos = iter->second;

        auto recordInfosIter = recordInfos_.find(recordName);
        PGOMethodInfoMap *toMethodInfos = nullptr;
        if (recordInfosIter == recordInfos_.end()) {
            toMethodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
            recordInfos_.emplace(recordName, toMethodInfos);
        } else {
            toMethodInfos = recordInfosIter->second;
        }

        toMethodInfos->Merge(chunk_.get(), fromMethodInfos);
    }
}

void PGORecordDetailInfos::ParseFromBinary(void *buffer, SectionInfo *const info)
{
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        auto recordName = base::ReadBuffer(&addr);
        PGOMethodInfoMap *methodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
        if (methodInfos->ParseFromBinary(hotnessThreshold_, &addr)) {
            recordInfos_.emplace(recordName, methodInfos);
        }
    }
}

void PGORecordDetailInfos::ProcessToBinary(const SaveTask *task, std::ofstream &fileStream, SectionInfo *info) const
{
    info->number_ = 0;
    info->offset_ = static_cast<uint32_t>(fileStream.tellp());
    for (auto iter = recordInfos_.begin(); iter != recordInfos_.end(); iter++) {
        auto recordName = iter->first;
        auto curMethodInfos = iter->second;
        if (curMethodInfos->ProcessToBinary(hotnessThreshold_, recordName, task, fileStream)) {
            info->number_++;
        }
    }
    info->size_ = static_cast<uint32_t>(fileStream.tellp()) - info->offset_;
}

bool PGORecordDetailInfos::ParseFromText(std::ifstream &stream)
{
    std::string details;
    while (std::getline(stream, details)) {
        if (details.empty()) {
            continue;
        }
        size_t blockIndex = details.find(BLOCK_AND_ARRAY_START);
        if (blockIndex == std::string::npos) {
            return false;
        }
        CString recordName = ConvertToString(details.substr(0, blockIndex));

        size_t start = details.find_first_of(ARRAY_START);
        size_t end = details.find_last_of(ARRAY_END);
        if (start == std::string::npos || end == std::string::npos || start > end) {
            return false;
        }
        auto content = details.substr(start + 1, end - (start + 1) - 1);
        std::vector<std::string> infoStrings = base::StringHelper::SplitString(content, BLOCK_SEPARATOR);
        if (infoStrings.size() <= 0)  {
            continue;
        }

        auto methodInfosIter = recordInfos_.find(recordName.c_str());
        PGOMethodInfoMap *methodInfos = nullptr;
        if (methodInfosIter == recordInfos_.end()) {
            methodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
            recordInfos_.emplace(recordName.c_str(), methodInfos);
        } else {
            methodInfos = methodInfosIter->second;
        }
        if (!methodInfos->ParseFromText(chunk_.get(), hotnessThreshold_, infoStrings)) {
            return false;
        }
    }
    return true;
}

void PGORecordDetailInfos::ProcessToText(std::ofstream &stream) const
{
    for (auto iter = recordInfos_.begin(); iter != recordInfos_.end(); iter++) {
        auto recordName = iter->first;
        auto methodInfos = iter->second;
        methodInfos->ProcessToText(hotnessThreshold_, recordName, stream);
    }
}

bool PGORecordSimpleInfos::Match(const CString &recordName, EntityId methodId)
{
    auto methodIdsIter = methodIds_.find(recordName);
    if (methodIdsIter == methodIds_.end()) {
        return false;
    }
    return methodIdsIter->second->Match(methodId);
}

void PGORecordSimpleInfos::ParseFromBinary(void *buffer, SectionInfo *const info)
{
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        auto recordName = base::ReadBuffer(&addr);
        PGOMethodIdSet *methodIds = nativeAreaAllocator_.New<PGOMethodIdSet>();
        if (methodIds->ParseFromBinary(hotnessThreshold_, &addr)) {
            methodIds_.emplace(recordName, methodIds);
        }
    }
}
} // namespace panda::ecmascript
