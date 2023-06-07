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
#include "ecmascript/js_function.h"
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"

namespace panda::ecmascript {
static const std::string ELEMENT_SEPARATOR = "/";
static const std::string BLOCK_SEPARATOR = ",";
static const std::string TYPE_SEPARATOR = "|";
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
            LOG_ECMA(ERROR) << "header size error, expected size is less than " << LastSize() << ", but got "
                            << desSize;
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
        if (!InternalSetVersion(version)) {
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
    stream << VERSION_HEADER << InternalGetVersion() << NEW_LINE;
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
        for (auto checksum : infos) {
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
    if (pandaFileInfos_.find(checksum) == pandaFileInfos_.end()) {
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

void PGOMethodTypeSet::Merge(NativeAreaAllocator *allocator, const PGOMethodTypeSet *info,
    const CMap<PGOSampleType, CMap<CString, TrackType>> &layoutDescs)
{
    for (const auto &fromType : info->typeInfoSet_) {
        auto iter = typeInfoSet_.find(fromType);
        if (iter != typeInfoSet_.end()) {
            PGOSampleType combineType = iter->GetType().CombineType(fromType.GetType());
            typeInfoSet_.erase(iter);
            typeInfoSet_.emplace(iter->GetOffset(), combineType);
        } else {
            typeInfoSet_.emplace(fromType);
        }
    }
    for (const auto &fromType : info->defineTypeSet_) {
        AddDefine(fromType.GetOffset(), fromType.GetType());
    }
    // rebuild layout desc info
    for (auto iter : layoutDescInfos_) {
        allocator->Delete(iter);
    }
    layoutDescInfos_.clear();
    for (const auto typeInfo : defineTypeSet_) {
        ASSERT(typeInfo.GetType().IsClassType());
        auto classType = typeInfo.GetType();
        auto iter = layoutDescs.find(classType);
        if (iter != layoutDescs.end()) {
            size_t size = PGOMethodLayoutDescInfos::CaculateSize(iter->second);
            void *addr = allocator->Allocate(size);
            auto descInfos = new (addr) PGOMethodLayoutDescInfos(size, typeInfo.GetOffset(), typeInfo.GetType());
            descInfos->Merge(iter->second);
            layoutDescInfos_.emplace(descInfos);
        }
    }
}

void PGOMethodTypeSet::SkipFromBinary(void **buffer)
{
    uint32_t size = base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
    for (uint32_t i = 0; i < size; i++) {
        base::ReadBufferInSize<PGOMethodTypeInfo>(buffer);
    }
}

bool PGOMethodTypeSet::ProcessToBinary(std::stringstream &stream) const
{
    uint32_t number = 0;
    std::stringstream methodStream;
    for (auto typeInfo : typeInfoSet_) {
        if (!typeInfo.GetType().IsNone()) {
            methodStream.write(reinterpret_cast<char *>(&typeInfo), sizeof(PGOMethodTypeInfo));
            number++;
        }
    }

    for (auto layoutDescInfo : layoutDescInfos_) {
        methodStream.write(reinterpret_cast<char *>(layoutDescInfo), layoutDescInfo->Size());
        number++;
    }

    stream.write(reinterpret_cast<char *>(&number), sizeof(uint32_t));
    if (number > 0) {
        stream << methodStream.rdbuf();
        return true;
    }
    return false;
}

bool PGOMethodTypeSet::ParseFromText(const std::string &typeString)
{
    std::vector<std::string> typeInfoVector = base::StringHelper::SplitString(typeString, TYPE_SEPARATOR);
    if (typeInfoVector.size() > 0) {
        for (const auto &iter : typeInfoVector) {
            std::vector<std::string> typeStrings = base::StringHelper::SplitString(iter, BLOCK_START);
            if (typeStrings.size() < METHOD_TYPE_INFO_COUNT) {
                return false;
            }

            uint32_t offset;
            if (!base::StringHelper::StrToUInt32(typeStrings[METHOD_OFFSET_INDEX].c_str(), &offset)) {
                return false;
            }
            uint32_t type;
            if (!base::StringHelper::StrToUInt32(typeStrings[METHOD_TYPE_INDEX].c_str(), &type)) {
                return false;
            }
            typeInfoSet_.emplace(offset, PGOSampleType(type));
        }
    }
    return true;
}

void PGOMethodTypeSet::ProcessToText(std::string &text) const
{
    bool isFirst = true;
    for (auto typeInfoIter : typeInfoSet_) {
        if (typeInfoIter.GetType().IsNone()) {
            continue;
        }
        if (isFirst) {
            text += ARRAY_START + SPACE;
            isFirst = false;
        } else {
            text += TYPE_SEPARATOR + SPACE;
        }
        text += std::to_string(typeInfoIter.GetOffset());
        text += BLOCK_START;
        text += typeInfoIter.GetType().GetTypeString();
    }
    for (const auto &descInfoIter : layoutDescInfos_) {
        if (isFirst) {
            text += ARRAY_START + SPACE;
            isFirst = false;
        } else {
            text += TYPE_SEPARATOR + SPACE;
        }
        descInfoIter->ProcessToText(text);
    }
    if (!isFirst) {
        text += (SPACE + ARRAY_END);
    }
}

void PGOMethodTypeSet::PGOMethodLayoutDescInfos::ProcessToText(std::string &text) const
{
    text += std::to_string(GetOffset());
    text += BLOCK_START;
    text += ARRAY_START + SPACE;
    auto current = GetFirst();
    bool isFirst = true;
    for (int i = 0; i < count_; i++) {
        if (!isFirst) {
            text += TYPE_SEPARATOR + SPACE;
        } else {
            text += GetType().GetTypeString();
            text += TYPE_SEPARATOR + SPACE;
        }
        isFirst = false;
        text += current->GetKey();
        text += BLOCK_START;
        text += std::to_string(static_cast<int32_t>(current->GetType()));
        current = GetNext(current);
    }
    text += (SPACE + ARRAY_END);
}

size_t PGOMethodTypeSet::PGOMethodLayoutDescInfos::CaculateSize(const CMap<CString, TrackType> &desc)
{
    if (desc.empty()) {
        return 0;
    }
    size_t size = sizeof(PGOMethodLayoutDescInfos) - sizeof(PGOLayoutDescInfo);
    for (const auto &iter : desc) {
        auto key = iter.first;
        size += static_cast<size_t>(PGOLayoutDescInfo::Size(key.size()));
    }
    return size;
}

void PGOMethodTypeSet::PGOMethodLayoutDescInfos::Merge(const CMap<CString, TrackType> &desc)
{
    if (desc.empty()) {
        return;
    }
    auto current = const_cast<PGOLayoutDescInfo *>(GetFirst());
    for (const auto &iter : desc) {
        auto key = iter.first;
        auto type = iter.second;
        size_t len = key.size();
        if (len <= 0) {
            continue;
        }
        new (current) PGOLayoutDescInfo(key, type);
        current = const_cast<PGOLayoutDescInfo *>(GetNext(current));
        count_++;
    }
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

PGOMethodTypeSet *PGOMethodInfoMap::GetOrInsertMethodTypeSet(Chunk *chunk, EntityId methodId)
{
    auto typeInfoSetIter = methodTypeInfos_.find(methodId);
    if (typeInfoSetIter != methodTypeInfos_.end()) {
        return typeInfoSetIter->second;
    } else {
        auto typeInfoSet = chunk->New<PGOMethodTypeSet>();
        methodTypeInfos_.emplace(methodId, typeInfoSet);
        return typeInfoSet;
    }
}

bool PGOMethodInfoMap::AddType(Chunk *chunk, EntityId methodId, int32_t offset, PGOSampleType type)
{
    auto typeInfoSet = GetOrInsertMethodTypeSet(chunk, methodId);
    ASSERT(typeInfoSet != nullptr);
    typeInfoSet->AddType(offset, type);
    return true;
}

bool PGOMethodInfoMap::AddDefine(Chunk *chunk, EntityId methodId, int32_t offset, PGOSampleType type)
{
    auto typeInfoSet = GetOrInsertMethodTypeSet(chunk, methodId);
    ASSERT(typeInfoSet != nullptr);
    typeInfoSet->AddDefine(offset, type);
    return true;
}

bool PGOMethodInfoMap::AddLayout(PGOSampleType type, JSTaggedType hclass)
{
    CMap<CString, TrackType> descInfos;
    auto rootHClass = JSHClass::Cast(JSTaggedValue(hclass).GetTaggedObject());
    if (!JSHClass::DumpForProfile(rootHClass, descInfos)) {
        return false;
    }

    auto iter = globalLayoutDescInfos_.find(type);
    CMap<CString, TrackType> descInfo;
    if (iter != globalLayoutDescInfos_.end()) {
        descInfo = iter->second;
    }
    for (const auto &info : descInfos) {
        if (descInfo.find(info.first) == descInfo.end()) {
            descInfo.emplace(info.first, info.second);
        }
    }
    if (iter == globalLayoutDescInfos_.end()) {
        globalLayoutDescInfos_.emplace(type, descInfo);
    } else {
        globalLayoutDescInfos_.at(type) = descInfo;
    }
    return true;
}

void PGOMethodInfoMap::Merge(Chunk *chunk, NativeAreaAllocator *allocator, PGOMethodInfoMap *methodInfos)
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
            auto newMethodInfo = new (infoAddr) PGOMethodInfo(
                methodId, fromMethodInfo->GetCount(), fromMethodInfo->GetSampleMode(), fromMethodInfo->GetMethodName());
            methodInfos_.emplace(methodId, newMethodInfo);
        }
        fromMethodInfo->ClearCount();
    }

    // Merge global layout desc infos to global method info map
    for (auto iter = methodInfos->globalLayoutDescInfos_.begin(); iter != methodInfos->globalLayoutDescInfos_.end();
         iter++) {
        auto classType = iter->first;
        auto result = globalLayoutDescInfos_.find(classType);
        if (result == globalLayoutDescInfos_.end()) {
            globalLayoutDescInfos_.emplace(classType, iter->second);
        } else {
            auto &toDescInfo = result->second;
            toDescInfo.insert(iter->second.begin(), iter->second.begin());
        }
    }

    for (auto iter = methodInfos->methodTypeInfos_.begin(); iter != methodInfos->methodTypeInfos_.end(); iter++) {
        auto methodId = iter->first;
        auto fromTypeInfo = iter->second;

        auto result = methodTypeInfos_.find(methodId);
        if (result != methodTypeInfos_.end()) {
            auto toTypeInfo = result->second;
            toTypeInfo->Merge(allocator, fromTypeInfo, globalLayoutDescInfos_);
        } else {
            auto typeInfoSet = chunk->New<PGOMethodTypeSet>();
            typeInfoSet->Merge(allocator, fromTypeInfo, globalLayoutDescInfos_);
            methodTypeInfos_.emplace(methodId, typeInfoSet);
        }
    }
}

bool PGOMethodInfoMap::ParseFromBinary(Chunk *chunk, uint32_t threshold, void **buffer, PGOProfilerHeader *const header)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t j = 0; j < secInfo.number_; j++) {
        PGOMethodInfo *info = base::ReadBufferInSize<PGOMethodInfo>(buffer);
        if (info->IsFilter(threshold)) {
            if (header->SupportType()) {
                PGOMethodTypeSet::SkipFromBinary(buffer);
            }
            continue;
        }
        methodInfos_.emplace(info->GetMethodId(), info);
        LOG_ECMA(DEBUG) << "Method:" << info->GetMethodId() << ELEMENT_SEPARATOR << info->GetCount()
                        << ELEMENT_SEPARATOR << std::to_string(static_cast<int>(info->GetSampleMode()))
                        << ELEMENT_SEPARATOR << info->GetMethodName();

        if (header->SupportType()) {
            auto typeInfoSet = chunk->New<PGOMethodTypeSet>();
            typeInfoSet->ParseFromBinary(buffer, [chunk](size_t size) { return chunk->Allocate(size); });
            methodTypeInfos_.emplace(info->GetMethodId(), typeInfoSet);
        }
    }
    return !methodInfos_.empty();
}

bool PGOMethodInfoMap::ProcessToBinary(uint32_t threshold, const CString &recordName, const SaveTask *task,
    std::ofstream &stream, PGOProfilerHeader *const header) const
{
    SectionInfo secInfo;
    std::stringstream methodStream;
    for (auto iter = methodInfos_.begin(); iter != methodInfos_.end(); iter++) {
        LOG_ECMA(DEBUG) << "Method:" << iter->first << ELEMENT_SEPARATOR << iter->second->GetCount()
                        << ELEMENT_SEPARATOR << std::to_string(static_cast<int>(iter->second->GetSampleMode()))
                        << ELEMENT_SEPARATOR << iter->second->GetMethodName();
        if (task && task->IsTerminate()) {
            LOG_ECMA(DEBUG) << "ProcessProfile: task is already terminate";
            return false;
        }
        auto curMethodInfo = iter->second;
        if (curMethodInfo->IsFilter(threshold)) {
            continue;
        }
        methodStream.write(reinterpret_cast<char *>(curMethodInfo), curMethodInfo->Size());
        if (header->SupportType()) {
            auto typeInfoIter = methodTypeInfos_.find(curMethodInfo->GetMethodId());
            if (typeInfoIter != methodTypeInfos_.end()) {
                typeInfoIter->second->ProcessToBinary(methodStream);
            } else {
                uint32_t number = 0;
                methodStream.write(reinterpret_cast<char *>(&number), sizeof(uint32_t));
            }
        }
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
        std::vector<std::string> infoStrings = PGOMethodInfo::ParseFromText(infoString);
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
            LOG_ECMA(ERROR) << "method id: " << infoStrings[PGOMethodInfo::METHOD_ID_INDEX] << " parse failed";
            return false;
        }
        std::string methodName = infoStrings[PGOMethodInfo::METHOD_NAME_INDEX];

        size_t len = methodName.size();
        void *infoAddr = chunk->Allocate(PGOMethodInfo::Size(len));
        auto info = new (infoAddr) PGOMethodInfo(EntityId(methodId), count, mode, methodName.c_str());
        methodInfos_.emplace(methodId, info);

        // Parse Type Info
        if (infoStrings.size() <= PGOMethodTypeSet::METHOD_TYPE_INFO_INDEX) {
            continue;
        }
        std::string typeInfos = infoStrings[PGOMethodTypeSet::METHOD_TYPE_INFO_INDEX];
        if (!typeInfos.empty()) {
            size_t start = typeInfos.find_first_of(ARRAY_START);
            size_t end = typeInfos.find_last_of(ARRAY_END);
            if (start == std::string::npos || end == std::string::npos || start > end) {
                LOG_ECMA(ERROR) << "Type info: " << typeInfos << " parse failed";
                return false;
            }
            auto typeContent = typeInfos.substr(start + 1, end - (start + 1) - 1);
            auto typeInfoSet = chunk->New<PGOMethodTypeSet>();
            if (!typeInfoSet->ParseFromText(typeContent)) {
                // delete by chunk
                LOG_ECMA(ERROR) << "Type info: " << typeInfos << " parse failed";
                return false;
            }
            methodTypeInfos_.emplace(info->GetMethodId(), typeInfoSet);
        }
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
        profilerString += ELEMENT_SEPARATOR;
        auto iter = methodTypeInfos_.find(methodInfo->GetMethodId());
        if (iter != methodTypeInfos_.end()) {
            iter->second->ProcessToText(profilerString);
        }
    }
    if (!isFirst) {
        profilerString += (SPACE + ARRAY_END + NEW_LINE);
        stream << profilerString;
    }
}

bool PGOMethodIdSet::ParseFromBinary(
    NativeAreaAllocator *allocator, uint32_t threshold, void **buffer, PGOProfilerHeader *const header)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t j = 0; j < secInfo.number_; j++) {
        PGOMethodInfo *info = base::ReadBufferInSize<PGOMethodInfo>(buffer);
        if (info->IsFilter(threshold)) {
            if (header->SupportType()) {
                PGOMethodTypeSet::SkipFromBinary(buffer);
            }
            continue;
        }
        methodIdSet_.emplace(info->GetMethodId());
        LOG_ECMA(DEBUG) << "Method:" << info->GetMethodId() << ELEMENT_SEPARATOR << info->GetCount()
                        << ELEMENT_SEPARATOR << std::to_string(static_cast<int>(info->GetSampleMode()))
                        << ELEMENT_SEPARATOR << info->GetMethodName();

        if (header->SupportType()) {
            auto typeInfoSet = allocator->New<PGOMethodTypeSet>();
            typeInfoSet->ParseFromBinary(buffer, [allocator](size_t size) { return allocator->Allocate(size); });
            methodTypeSet_.emplace(info->GetMethodId(), typeInfoSet);
        }
    }

    return methodIdSet_.size() != 0;
}

PGOMethodInfoMap *PGORecordDetailInfos::GetMethodInfoMap(const CString &recordName)
{
    auto iter = recordInfos_.find(recordName.c_str());
    if (iter != recordInfos_.end()) {
        return iter->second;
    } else {
        auto curMethodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
        recordInfos_.emplace(recordName.c_str(), curMethodInfos);
        return curMethodInfos;
    }
}

bool PGORecordDetailInfos::AddMethod(
    const CString &recordName, EntityId methodId, const CString &methodName, SampleMode mode)
{
    auto curMethodInfos = GetMethodInfoMap(recordName);
    ASSERT(curMethodInfos != nullptr);
    return curMethodInfos->AddMethod(chunk_.get(), methodId, methodName, mode);
}

bool PGORecordDetailInfos::AddType(const CString &recordName, EntityId methodId, int32_t offset, PGOSampleType type)
{
    auto curMethodInfos = GetMethodInfoMap(recordName);
    ASSERT(curMethodInfos != nullptr);
    return curMethodInfos->AddType(chunk_.get(), methodId, offset, type);
}

bool PGORecordDetailInfos::AddDefine(const CString &recordName, EntityId methodId, int32_t offset, PGOSampleType type)
{
    auto curMethodInfos = GetMethodInfoMap(recordName);
    ASSERT(curMethodInfos != nullptr);
    return curMethodInfos->AddDefine(chunk_.get(), methodId, offset, type);
}

bool PGORecordDetailInfos::AddLayout(const CString &recordName, PGOSampleType type, JSTaggedType hclass)
{
    auto curMethodInfos = GetMethodInfoMap(recordName);
    ASSERT(curMethodInfos != nullptr);
    return curMethodInfos->AddLayout(type, hclass);
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

        toMethodInfos->Merge(chunk_.get(), &nativeAreaAllocator_, fromMethodInfos);
    }
}

void PGORecordDetailInfos::ParseFromBinary(void *buffer, PGOProfilerHeader *const header)
{
    SectionInfo *info = header->GetRecordInfoSection();
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        auto recordName = base::ReadBuffer(&addr);
        PGOMethodInfoMap *methodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
        if (methodInfos->ParseFromBinary(chunk_.get(), hotnessThreshold_, &addr, header)) {
            recordInfos_.emplace(recordName, methodInfos);
        }
    }
}

void PGORecordDetailInfos::ProcessToBinary(
    const SaveTask *task, std::ofstream &fileStream, PGOProfilerHeader *const header) const
{
    auto info = header->GetRecordInfoSection();
    info->number_ = 0;
    info->offset_ = static_cast<uint32_t>(fileStream.tellp());
    for (auto iter = recordInfos_.begin(); iter != recordInfos_.end(); iter++) {
        if (task && task->IsTerminate()) {
            LOG_ECMA(DEBUG) << "ProcessProfile: task is already terminate";
            break;
        }
        auto recordName = iter->first;
        auto curMethodInfos = iter->second;
        if (curMethodInfos->ProcessToBinary(hotnessThreshold_, recordName, task, fileStream, header)) {
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
        if (infoStrings.size() <= 0) {
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

void PGORecordSimpleInfos::ParseFromBinary(void *buffer, PGOProfilerHeader *const header)
{
    SectionInfo *info = header->GetRecordInfoSection();
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        auto recordName = base::ReadBuffer(&addr);
        PGOMethodIdSet *methodIds = nativeAreaAllocator_.New<PGOMethodIdSet>();
        if (methodIds->ParseFromBinary(&nativeAreaAllocator_, hotnessThreshold_, &addr, header)) {
            methodIds_.emplace(recordName, methodIds);
        }
    }
}
} // namespace panda::ecmascript
