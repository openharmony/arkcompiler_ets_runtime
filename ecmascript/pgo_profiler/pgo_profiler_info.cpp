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
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <memory>

#include "ecmascript/base/bit_helper.h"
#include "ecmascript/base/file_header.h"
#include "ecmascript/js_function.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_method_type_set.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_profile_type_pool.h"
#include "ecmascript/pgo_profiler/ap_file/pgo_record_pool.h"
#include "ecmascript/pgo_profiler/pgo_context.h"
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_utils.h"
#include "ecmascript/pgo_profiler/types/pgo_profile_type.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "macros.h"
#include "securec.h"

namespace panda::ecmascript::pgo {
namespace {
bool ParseSectionsFromBinary(const std::list<std::weak_ptr<PGOFileSectionInterface>> &sectionList, void *buffer,
                             PGOProfilerHeader const *header)
{
    return std::all_of(sectionList.begin(), sectionList.end(), [&](const auto &sectionWeak) {
        auto section = sectionWeak.lock();
        if (section == nullptr) {
            return true;
        }
        return PGOFileSectionInterface::ParseSectionFromBinary(buffer, header, *section);
    });
}
}  // namespace

using StringHelper = base::StringHelper;
void PGOPandaFileInfos::ParseFromBinary(void *buffer, SectionInfo *const info)
{
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        fileInfos_.emplace(*base::ReadBufferInSize<FileInfo>(&addr));
    }
    LOG_ECMA(DEBUG) << "Profiler panda file count:" << info->number_;
}

void PGOPandaFileInfos::ProcessToBinary(std::fstream &fileStream, SectionInfo *info) const
{
    fileStream.seekp(info->offset_);
    info->number_ = fileInfos_.size();
    for (auto localInfo : fileInfos_) {
        fileStream.write(reinterpret_cast<char *>(&localInfo), localInfo.Size());
    }
    info->size_ = static_cast<uint32_t>(fileStream.tellp()) - info->offset_;
}

void PGOPandaFileInfos::Merge(const PGOPandaFileInfos &pandaFileInfos)
{
    for (const auto &info : pandaFileInfos.fileInfos_) {
        fileInfos_.emplace(info.GetChecksum());
    }
}

bool PGOPandaFileInfos::VerifyChecksum(const PGOPandaFileInfos &pandaFileInfos, const std::string &base,
                                       const std::string &incoming) const
{
    std::set<FileInfo> unionChecksum;
    set_union(fileInfos_.begin(), fileInfos_.end(), pandaFileInfos.fileInfos_.begin(), pandaFileInfos.fileInfos_.end(),
              inserter(unionChecksum, unionChecksum.begin()));
    if (!fileInfos_.empty() && unionChecksum.empty()) {
        LOG_ECMA(ERROR) << "First AP file(" << base << ") and the incoming file(" << incoming
                        << ") do not come from the same abc file, skip merge the incoming file.";
        return false;
    }
    return true;
}

bool PGOPandaFileInfos::ParseFromText(std::ifstream &stream)
{
    std::string pandaFileInfo;
    while (std::getline(stream, pandaFileInfo)) {
        if (pandaFileInfo.empty()) {
            continue;
        }

        size_t start = pandaFileInfo.find_first_of(DumpUtils::ARRAY_START);
        size_t end = pandaFileInfo.find_last_of(DumpUtils::ARRAY_END);
        if (start == std::string::npos || end == std::string::npos || start > end) {
            return false;
        }
        auto content = pandaFileInfo.substr(start + 1, end - (start + 1) - 1);
        std::vector<std::string> infos = StringHelper::SplitString(content, DumpUtils::BLOCK_SEPARATOR);
        for (auto checksum : infos) {
            uint32_t result;
            if (!StringHelper::StrToUInt32(checksum.c_str(), &result)) {
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
    std::string pandaFileInfo = DumpUtils::NEW_LINE + DumpUtils::PANDA_FILE_INFO_HEADER;
    bool isFirst = true;
    for (auto &info : fileInfos_) {
        if (!isFirst) {
            pandaFileInfo += DumpUtils::BLOCK_SEPARATOR + DumpUtils::SPACE;
        } else {
            isFirst = false;
        }
        pandaFileInfo += std::to_string(info.GetChecksum());
    }

    pandaFileInfo += (DumpUtils::SPACE + DumpUtils::ARRAY_END + DumpUtils::NEW_LINE);
    stream << pandaFileInfo;
}

bool PGOPandaFileInfos::Checksum(uint32_t checksum) const
{
    if (fileInfos_.find(checksum) == fileInfos_.end()) {
        LOG_ECMA(ERROR) << "Checksum verification failed. Please ensure that the .abc and .ap match.";
        return false;
    }
    return true;
}

void PGOMethodInfo::ProcessToText(std::string &text) const
{
    text += std::to_string(GetMethodId().GetOffset());
    text += DumpUtils::ELEMENT_SEPARATOR;
    text += std::to_string(GetCount());
    text += DumpUtils::ELEMENT_SEPARATOR;
    text += GetSampleModeToString();
    text += DumpUtils::ELEMENT_SEPARATOR;
    text += GetMethodName();
}

std::vector<std::string> PGOMethodInfo::ParseFromText(const std::string &infoString)
{
    std::vector<std::string> infoStrings = StringHelper::SplitString(infoString, DumpUtils::ELEMENT_SEPARATOR);
    return infoStrings;
}

uint32_t PGOMethodInfo::CalcChecksum(const char *name, const uint8_t *byteCodeArray, uint32_t byteCodeLength)
{
    uint32_t checksum = 0;
    if (byteCodeArray != nullptr) {
        checksum = CalcOpCodeChecksum(byteCodeArray, byteCodeLength);
    }

    if (name != nullptr) {
        checksum = adler32(checksum, reinterpret_cast<const Bytef *>(name), strlen(name));
    }
    return checksum;
}

uint32_t PGOMethodInfo::CalcOpCodeChecksum(const uint8_t *byteCodeArray, uint32_t byteCodeLength)
{
    uint32_t checksum = 0;
    BytecodeInstruction bcIns(byteCodeArray);
    auto bcInsLast = bcIns.JumpTo(byteCodeLength);
    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        auto opCode = bcIns.GetOpcode();
        checksum = adler32(checksum, reinterpret_cast<const Bytef *>(&opCode), sizeof(decltype(opCode)));
        bcIns = bcIns.GetNext();
    }
    return checksum;
}

bool PGOMethodInfoMap::AddMethod(NativeAreaAllocator *allocator, Method *jsMethod, SampleMode mode, int32_t incCount)
{
    PGOMethodId methodId(jsMethod->GetMethodId());
    auto result = methodInfos_.find(methodId);
    if (result != methodInfos_.end()) {
        auto info = result->second;
        info->IncreaseCount(incCount);
        info->SetSampleMode(mode);
        return false;
    } else {
        CString methodName = jsMethod->GetMethodName();
        size_t strlen = methodName.size();
        size_t size = static_cast<size_t>(PGOMethodInfo::Size(strlen));
        void *infoAddr = allocator->Allocate(size);
        auto info = new (infoAddr) PGOMethodInfo(methodId, incCount, mode, methodName.c_str());
        methodInfos_.emplace(methodId, info);
        auto checksum = PGOMethodInfo::CalcChecksum(jsMethod->GetMethodName(), jsMethod->GetBytecodeArray(),
                                                    jsMethod->GetCodeSize());
        methodsChecksum_.emplace(methodId, checksum);
        return true;
    }
}

PGOMethodTypeSet *PGOMethodInfoMap::GetOrInsertMethodTypeSet(Chunk *chunk, PGOMethodId methodId)
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

bool PGOMethodInfoMap::AddType(Chunk *chunk, PGOMethodId methodId, int32_t offset, PGOSampleType type)
{
    auto typeInfoSet = GetOrInsertMethodTypeSet(chunk, methodId);
    ASSERT(typeInfoSet != nullptr);
    typeInfoSet->AddType(offset, type);
    return true;
}

bool PGOMethodInfoMap::AddCallTargetType(Chunk *chunk, PGOMethodId methodId, int32_t offset, PGOSampleType type)
{
    auto typeInfoSet = GetOrInsertMethodTypeSet(chunk, methodId);
    ASSERT(typeInfoSet != nullptr);
    typeInfoSet->AddCallTargetType(offset, type);
    return true;
}

bool PGOMethodInfoMap::AddObjectInfo(Chunk *chunk, PGOMethodId methodId, int32_t offset, const PGOObjectInfo &info)
{
    auto typeInfoSet = GetOrInsertMethodTypeSet(chunk, methodId);
    ASSERT(typeInfoSet != nullptr);
    typeInfoSet->AddObjectInfo(offset, info);
    return true;
}

bool PGOMethodInfoMap::AddDefine(
    Chunk *chunk, PGOMethodId methodId, int32_t offset, PGOSampleType type, PGOSampleType superType)
{
    auto typeInfoSet = GetOrInsertMethodTypeSet(chunk, methodId);
    ASSERT(typeInfoSet != nullptr);
    typeInfoSet->AddDefine(offset, type, superType);
    return true;
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
            auto newMethodInfo = new (infoAddr) PGOMethodInfo(
                methodId, fromMethodInfo->GetCount(), fromMethodInfo->GetSampleMode(), fromMethodInfo->GetMethodName());
            methodInfos_.emplace(methodId, newMethodInfo);
        }
        fromMethodInfo->ClearCount();
    }

    for (auto iter = methodInfos->methodTypeInfos_.begin(); iter != methodInfos->methodTypeInfos_.end(); iter++) {
        auto methodId = iter->first;
        auto fromTypeInfo = iter->second;

        auto result = methodTypeInfos_.find(methodId);
        if (result != methodTypeInfos_.end()) {
            auto toTypeInfo = result->second;
            toTypeInfo->Merge(fromTypeInfo);
        } else {
            auto typeInfoSet = chunk->New<PGOMethodTypeSet>();
            typeInfoSet->Merge(fromTypeInfo);
            methodTypeInfos_.emplace(methodId, typeInfoSet);
        }
    }

    for (auto iter = methodInfos->methodsChecksum_.begin(); iter != methodInfos->methodsChecksum_.end(); iter++) {
        auto methodId = iter->first;
        auto result = methodsChecksum_.find(methodId);
        if (result == methodsChecksum_.end()) {
            methodsChecksum_.emplace(methodId, iter->second);
        }
    }
}

bool PGOMethodInfoMap::ParseFromBinary(Chunk *chunk, PGOContext &context, void **buffer)
{
    PGOProfilerHeader *const header = context.GetHeader();
    ASSERT(header != nullptr);
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t j = 0; j < secInfo.number_; j++) {
        PGOMethodInfo *info = base::ReadBufferInSize<PGOMethodInfo>(buffer);
        if (info->IsFilter(context.GetHotnessThreshold())) {
            if (header->SupportMethodChecksum()) {
                base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
            }
            if (header->SupportType()) {
                PGOMethodTypeSet::SkipFromBinary(buffer);
            }
            continue;
        }
        methodInfos_.emplace(info->GetMethodId(), info);
        LOG_ECMA(DEBUG) << "Method:" << info->GetMethodId() << DumpUtils::ELEMENT_SEPARATOR << info->GetCount()
                        << DumpUtils::ELEMENT_SEPARATOR << std::to_string(static_cast<int>(info->GetSampleMode()))
                        << DumpUtils::ELEMENT_SEPARATOR << info->GetMethodName();
        if (header->SupportMethodChecksum()) {
            auto checksum = base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
            methodsChecksum_.emplace(info->GetMethodId(), checksum);
        }
        if (header->SupportType()) {
            auto typeInfoSet = chunk->New<PGOMethodTypeSet>();
            typeInfoSet->ParseFromBinary(context, buffer);
            methodTypeInfos_.emplace(info->GetMethodId(), typeInfoSet);
        }
    }
    return !methodInfos_.empty();
}

bool PGOMethodInfoMap::ProcessToBinary(PGOContext &context, ProfileTypeRef recordProfileRef, const SaveTask *task,
                                       std::fstream &stream, PGOProfilerHeader *const header) const
{
    SectionInfo secInfo;
    std::stringstream methodStream;
    for (auto iter = methodInfos_.begin(); iter != methodInfos_.end(); iter++) {
        LOG_ECMA(DEBUG) << "Method:" << iter->first << DumpUtils::ELEMENT_SEPARATOR << iter->second->GetCount()
                        << DumpUtils::ELEMENT_SEPARATOR
                        << std::to_string(static_cast<int>(iter->second->GetSampleMode()))
                        << DumpUtils::ELEMENT_SEPARATOR << iter->second->GetMethodName();
        if (task && task->IsTerminate()) {
            LOG_ECMA(DEBUG) << "ProcessProfile: task is already terminate";
            return false;
        }
        auto curMethodInfo = iter->second;
        if (curMethodInfo->IsFilter(context.GetHotnessThreshold())) {
            continue;
        }
        methodStream.write(reinterpret_cast<char *>(curMethodInfo), curMethodInfo->Size());
        if (header->SupportMethodChecksum()) {
            auto checksumIter = methodsChecksum_.find(curMethodInfo->GetMethodId());
            uint32_t checksum = 0;
            if (checksumIter != methodsChecksum_.end()) {
                checksum = checksumIter->second;
            }
            methodStream.write(reinterpret_cast<char *>(&checksum), sizeof(uint32_t));
        }
        if (header->SupportType()) {
            auto typeInfoIter = methodTypeInfos_.find(curMethodInfo->GetMethodId());
            if (typeInfoIter != methodTypeInfos_.end()) {
                typeInfoIter->second->ProcessToBinary(context, methodStream);
            } else {
                uint32_t number = 0;
                methodStream.write(reinterpret_cast<char *>(&number), sizeof(uint32_t));
            }
        }
        secInfo.number_++;
    }
    if (secInfo.number_ > 0) {
        secInfo.offset_ = sizeof(SectionInfo);
        secInfo.size_ = static_cast<uint32_t>(methodStream.tellp());
        stream.write(reinterpret_cast<char *>(&recordProfileRef), sizeof(uint32_t));
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
        if (!StringHelper::StrToUInt32(infoStrings[PGOMethodInfo::METHOD_COUNT_INDEX].c_str(), &count)) {
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
        if (!StringHelper::StrToUInt32(infoStrings[PGOMethodInfo::METHOD_ID_INDEX].c_str(), &methodId)) {
            LOG_ECMA(ERROR) << "method id: " << infoStrings[PGOMethodInfo::METHOD_ID_INDEX] << " parse failed";
            return false;
        }
        std::string methodName = infoStrings[PGOMethodInfo::METHOD_NAME_INDEX];

        size_t len = methodName.size();
        void *infoAddr = chunk->Allocate(PGOMethodInfo::Size(len));
        auto info = new (infoAddr) PGOMethodInfo(PGOMethodId(methodId), count, mode, methodName.c_str());
        methodInfos_.emplace(methodId, info);

        // Parse Type Info
        if (infoStrings.size() <= PGOMethodTypeSet::METHOD_TYPE_INFO_INDEX) {
            continue;
        }
        std::string typeInfos = infoStrings[PGOMethodTypeSet::METHOD_TYPE_INFO_INDEX];
        if (!typeInfos.empty()) {
            size_t start = typeInfos.find_first_of(DumpUtils::ARRAY_START);
            size_t end = typeInfos.find_last_of(DumpUtils::ARRAY_END);
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
            profilerString += DumpUtils::NEW_LINE;
            profilerString += recordName;
            profilerString += DumpUtils::BLOCK_AND_ARRAY_START;
            isFirst = false;
        } else {
            profilerString += DumpUtils::BLOCK_SEPARATOR + DumpUtils::SPACE;
        }
        methodInfo->ProcessToText(profilerString);
        profilerString += DumpUtils::ELEMENT_SEPARATOR;
        auto checksumIter = methodsChecksum_.find(methodInfo->GetMethodId());
        if (checksumIter != methodsChecksum_.end()) {
            std::stringstream parseStream;
            parseStream << std::internal << std::setfill('0') << std::showbase
                        << std::setw(DumpUtils::HEX_FORMAT_WIDTH_FOR_32BITS) << std::hex << checksumIter->second
                        << DumpUtils::ELEMENT_SEPARATOR;
            profilerString += parseStream.str();
        }
        auto iter = methodTypeInfos_.find(methodInfo->GetMethodId());
        if (iter != methodTypeInfos_.end()) {
            iter->second->ProcessToText(profilerString);
        }
    }
    if (!isFirst) {
        profilerString += (DumpUtils::SPACE + DumpUtils::ARRAY_END + DumpUtils::NEW_LINE);
        stream << profilerString;
    }
}

bool PGOMethodIdSet::ParseFromBinary(PGOContext &context, void **buffer)
{
    PGOProfilerHeader *const header = context.GetHeader();
    ASSERT(header != nullptr);
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t j = 0; j < secInfo.number_; j++) {
        PGOMethodInfo *info = base::ReadBufferInSize<PGOMethodInfo>(buffer);
        if (info->IsFilter(context.GetHotnessThreshold())) {
            if (header->SupportMethodChecksum()) {
                base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
            }
            if (header->SupportType()) {
                PGOMethodTypeSet::SkipFromBinary(buffer);
            }
            continue;
        }
        uint32_t checksum = 0;
        if (header->SupportMethodChecksum()) {
            checksum = base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
        }
        auto ret = methodInfoMap_.try_emplace(info->GetMethodName(), chunk_);
        auto methodNameSetIter = ret.first;
        auto &methodInfo = methodNameSetIter->second.GetOrCreateMethodInfo(checksum, info->GetMethodId());
        LOG_ECMA(DEBUG) << "Method:" << info->GetMethodId() << DumpUtils::ELEMENT_SEPARATOR << info->GetCount()
                        << DumpUtils::ELEMENT_SEPARATOR << std::to_string(static_cast<int>(info->GetSampleMode()))
                        << DumpUtils::ELEMENT_SEPARATOR << info->GetMethodName();
        if (header->SupportType()) {
            methodInfo.GetPGOMethodTypeSet().ParseFromBinary(context, buffer);
        }
    }

    return !methodInfoMap_.empty();
}

void PGOMethodIdSet::GetMismatchResult(const CString &recordName, uint32_t &totalMethodCount,
                                       uint32_t &mismatchMethodCount,
                                       std::set<std::pair<std::string, CString>> &mismatchMethodSet) const
{
    totalMethodCount += methodInfoMap_.size();
    for (const auto &methodNameSet : methodInfoMap_) {
        if (methodNameSet.second.IsMatch()) {
            continue;
        }
        auto info = std::make_pair(methodNameSet.first, recordName);
        mismatchMethodSet.emplace(info);
        mismatchMethodCount++;
    }
}

void PGOMethodIdSet::Merge(const PGOMethodIdSet &from)
{
    for (const auto &methodNameSet : from.methodInfoMap_) {
        auto iter = methodInfoMap_.find(methodNameSet.first);
        if (iter == methodInfoMap_.end()) {
            auto ret = methodInfoMap_.try_emplace(methodNameSet.first, chunk_);
            iter = ret.first;
        }
        const_cast<PGOMethodNameSet &>(iter->second).Merge(methodNameSet.second);
    }
}

void PGODecodeMethodInfo::Merge(const PGODecodeMethodInfo &from)
{
    ASSERT(methodId_.IsValid() && from.methodId_.IsValid());
    if (!(methodId_ == from.methodId_)) {
        LOG_ECMA(ERROR) << "MethodId not match. " << methodId_ << " vs " << from.methodId_;
        return;
    }
    pgoMethodTypeSet_.Merge(&from.pgoMethodTypeSet_);
}

PGORecordDetailInfos::PGORecordDetailInfos(uint32_t hotnessThreshold) : hotnessThreshold_(hotnessThreshold)
{
    chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
    InitSections();
};

PGORecordDetailInfos::~PGORecordDetailInfos()
{
    Clear();
}

PGOMethodInfoMap *PGORecordDetailInfos::GetMethodInfoMap(ProfileType recordProfileType)
{
    auto iter = recordInfos_.find(recordProfileType);
    if (iter != recordInfos_.end()) {
        return iter->second;
    } else {
        auto curMethodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
        recordInfos_.emplace(recordProfileType, curMethodInfos);
        return curMethodInfos;
    }
}

bool PGORecordDetailInfos::AddMethod(ProfileType recordProfileType, Method *jsMethod, SampleMode mode, int32_t incCount)
{
    auto curMethodInfos = GetMethodInfoMap(recordProfileType);
    ASSERT(curMethodInfos != nullptr);
    ASSERT(jsMethod != nullptr);
    return curMethodInfos->AddMethod(&nativeAreaAllocator_, jsMethod, mode, incCount);
}

bool PGORecordDetailInfos::AddType(ProfileType recordProfileType, PGOMethodId methodId, int32_t offset,
                                   PGOSampleType type)
{
    auto curMethodInfos = GetMethodInfoMap(recordProfileType);
    ASSERT(curMethodInfos != nullptr);
    return curMethodInfos->AddType(chunk_.get(), methodId, offset, type);
}

bool PGORecordDetailInfos::AddCallTargetType(ProfileType recordProfileType, PGOMethodId methodId, int32_t offset,
                                             PGOSampleType type)
{
    auto curMethodInfos = GetMethodInfoMap(recordProfileType);
    ASSERT(curMethodInfos != nullptr);
    return curMethodInfos->AddCallTargetType(chunk_.get(), methodId, offset, type);
}

bool PGORecordDetailInfos::AddObjectInfo(
    ProfileType recordProfileType, EntityId methodId, int32_t offset, const PGOObjectInfo &info)
{
    auto curMethodInfos = GetMethodInfoMap(recordProfileType);
    ASSERT(curMethodInfos != nullptr);
    return curMethodInfos->AddObjectInfo(chunk_.get(), methodId, offset, info);
}

bool PGORecordDetailInfos::AddDefine(
    ProfileType recordProfileType, PGOMethodId methodId, int32_t offset, PGOSampleType type, PGOSampleType superType)
{
    auto curMethodInfos = GetMethodInfoMap(recordProfileType);
    ASSERT(curMethodInfos != nullptr);
    curMethodInfos->AddDefine(chunk_.get(), methodId, offset, type, superType);

    PGOHClassLayoutDesc descInfo(type.GetProfileType());
    descInfo.SetSuperProfileType(superType.GetProfileType());
    auto iter = moduleLayoutDescInfos_.find(descInfo);
    if (iter != moduleLayoutDescInfos_.end()) {
        moduleLayoutDescInfos_.erase(iter);
    }
    moduleLayoutDescInfos_.emplace(descInfo);
    return true;
}

bool PGORecordDetailInfos::AddLayout(PGOSampleType type, JSTaggedType hclass, PGOObjKind kind)
{
    auto hclassObject = JSHClass::Cast(JSTaggedValue(hclass).GetTaggedObject());
    PGOHClassLayoutDesc descInfo(type.GetProfileType());
    auto iter = moduleLayoutDescInfos_.find(descInfo);
    if (iter != moduleLayoutDescInfos_.end()) {
        auto &oldDescInfo = const_cast<PGOHClassLayoutDesc &>(*iter);
        if (!JSHClass::DumpForProfile(hclassObject, oldDescInfo, kind)) {
            return false;
        }
    } else {
        LOG_ECMA(DEBUG) << "The current class did not find a definition";
        return false;
    }
    return true;
}

bool PGORecordDetailInfos::UpdateElementsKind(PGOSampleType type, ElementsKind kind)
{
    PGOHClassLayoutDesc descInfo(type.GetProfileType());
    auto iter = moduleLayoutDescInfos_.find(descInfo);
    if (iter != moduleLayoutDescInfos_.end()) {
        auto &oldDescInfo = const_cast<PGOHClassLayoutDesc &>(*iter);
        oldDescInfo.UpdateElementKind(kind);
    } else {
        LOG_ECMA(DEBUG) << "The current class did not find a definition";
        return false;
    }
    return true;
}

void PGORecordDetailInfos::Merge(const PGORecordDetailInfos &recordInfos)
{
    std::map<ApEntityId, ApEntityId> idMapping;
    recordPool_->Merge(*recordInfos.recordPool_, idMapping);
    for (auto iter = recordInfos.recordInfos_.begin(); iter != recordInfos.recordInfos_.end(); iter++) {
        auto oldRecordType = iter->first;
        auto newIter = idMapping.find(oldRecordType.GetId());
        if (newIter == idMapping.end()) {
            continue;
        }
        auto newRecordType = PGOProfiler::GetRecordProfileType(oldRecordType.GetAbcId(), newIter->second);
        auto fromMethodInfos = iter->second;

        auto recordInfosIter = recordInfos_.find(newRecordType);
        PGOMethodInfoMap *toMethodInfos = nullptr;
        if (recordInfosIter == recordInfos_.end()) {
            toMethodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
            recordInfos_.emplace(newRecordType, toMethodInfos);
        } else {
            toMethodInfos = recordInfosIter->second;
        }

        toMethodInfos->Merge(chunk_.get(), fromMethodInfos);
    }
    // Merge global layout desc infos to global method info map
    for (auto info = recordInfos.moduleLayoutDescInfos_.begin(); info != recordInfos.moduleLayoutDescInfos_.end();
         info++) {
        auto fromInfo = *info;
        auto result = moduleLayoutDescInfos_.find(fromInfo);
        if (result == moduleLayoutDescInfos_.end()) {
            moduleLayoutDescInfos_.emplace(fromInfo);
        } else {
            const_cast<PGOHClassLayoutDesc &>(*result).Merge(fromInfo);
        }
    }
}

void PGORecordDetailInfos::ParseFromBinary(void *buffer, PGOProfilerHeader *const header)
{
    header_ = header;
    if (!ParseSectionsFromBinary(apSectionList_, buffer, header)) {
        return;
    }
    SectionInfo *info = header->GetRecordInfoSection();
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        ApEntityId recordId(0);
        ProfileType recordType;
        if (header->SupportProfileTypeWithAbcId()) {
            auto recordTypeRef = ProfileTypeRef(base::ReadBuffer<ApEntityId>(&addr, sizeof(ApEntityId)));
            recordType = ProfileType(*this, recordTypeRef);
            recordId = recordType.GetId();
        } else if (header->SupportRecordPool()) {
            recordId = base::ReadBuffer<ApEntityId>(&addr, sizeof(ApEntityId));
        } else {
            auto *recordName = base::ReadBuffer(&addr);
            recordPool_->TryAdd(recordName, recordId);
        }
        recordType.UpdateId(recordId);
        recordType.UpdateKind(ProfileType::Kind::LocalRecordId);
        PGOMethodInfoMap *methodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
        if (methodInfos->ParseFromBinary(chunk_.get(), *this, &addr)) {
            recordInfos_.emplace(recordType, methodInfos);
        }
    }

    info = header->GetLayoutDescSection();
    if (info == nullptr) {
        return;
    }
    if (header->SupportTrackField()) {
        ParseFromBinaryForLayout(&addr);
    }
}

bool PGORecordDetailInfos::ParseFromBinaryForLayout(void **buffer)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t i = 0; i < secInfo.number_; i++) {
        auto *info = base::ReadBufferInSize<PGOHClassLayoutDescInnerRef>(buffer);
        if (info == nullptr) {
            LOG_ECMA(INFO) << "Binary format error!";
            continue;
        }
        moduleLayoutDescInfos_.emplace(info->Convert(*this));
    }
    return true;
}

void PGORecordDetailInfos::ProcessToBinary(
    const SaveTask *task, std::fstream &fileStream, PGOProfilerHeader *const header)
{
    header_ = header;
    auto info = header->GetRecordInfoSection();
    info->number_ = 0;
    info->offset_ = static_cast<uint32_t>(fileStream.tellp());
    for (auto iter = recordInfos_.begin(); iter != recordInfos_.end(); iter++) {
        if (task && task->IsTerminate()) {
            LOG_ECMA(DEBUG) << "ProcessProfile: task is already terminate";
            break;
        }
        auto recordId = iter->first;
        auto curMethodInfos = iter->second;
        if (curMethodInfos->ProcessToBinary(*this, ProfileTypeRef(*this, recordId), task, fileStream, header)) {
            info->number_++;
        }
    }
    info->size_ = static_cast<uint32_t>(fileStream.tellp()) - info->offset_;

    info = header->GetLayoutDescSection();
    if (info == nullptr) {
        return;
    }
    info->number_ = 0;
    info->offset_ = static_cast<uint32_t>(fileStream.tellp());
    if (header->SupportType()) {
        if (!ProcessToBinaryForLayout(const_cast<NativeAreaAllocator *>(&nativeAreaAllocator_), task, fileStream)) {
            return;
        }
        info->number_++;
    }
    info->size_ = static_cast<uint32_t>(fileStream.tellp()) - info->offset_;

    for (const auto &sectionWeak : apSectionList_) {
        auto section = sectionWeak.lock();
        if (section == nullptr) {
            continue;
        }
        PGOFileSectionInterface::ProcessSectionToBinary(fileStream, header, *section);
    }
}

bool PGORecordDetailInfos::ProcessToBinaryForLayout(
    NativeAreaAllocator *allocator, const SaveTask *task, std::fstream &stream)
{
    SectionInfo secInfo;
    auto layoutBeginPosition = stream.tellp();
    stream.seekp(sizeof(SectionInfo), std::ofstream::cur);
    for (const auto &typeInfo : moduleLayoutDescInfos_) {
        if (task && task->IsTerminate()) {
            LOG_ECMA(DEBUG) << "ProcessProfile: task is already terminate";
            return false;
        }
        auto profileType = PGOSampleType(typeInfo.GetProfileType());
        auto elementsKind = typeInfo.GetElementsKind();
        size_t size = PGOHClassLayoutDescInnerRef::CaculateSize(typeInfo);
        if (size == 0) {
            continue;
        }
        auto superType = PGOSampleType(typeInfo.GetSuperProfileType());

        PGOSampleTypeRef classRef = PGOSampleTypeRef::ConvertFrom(*this, profileType);
        PGOSampleTypeRef superRef = PGOSampleTypeRef::ConvertFrom(*this, superType);
        void *addr = allocator->Allocate(size);
        auto descInfos = new (addr) PGOHClassLayoutDescInnerRef(size, classRef, superRef, elementsKind);
        descInfos->Merge(typeInfo);
        stream.write(reinterpret_cast<char *>(descInfos), size);
        allocator->Delete(addr);
        secInfo.number_++;
    }

    secInfo.offset_ = sizeof(SectionInfo);
    secInfo.size_ = static_cast<uint32_t>(stream.tellp()) -
        static_cast<uint32_t>(layoutBeginPosition) - sizeof(SectionInfo);
    stream.seekp(layoutBeginPosition, std::ofstream::beg)
        .write(reinterpret_cast<char *>(&secInfo), sizeof(SectionInfo))
        .seekp(0, std::ofstream::end);
    return true;
}

bool PGORecordDetailInfos::ParseFromText(std::ifstream &stream)
{
    std::string details;
    while (std::getline(stream, details)) {
        if (details.empty()) {
            continue;
        }
        size_t blockIndex = details.find(DumpUtils::BLOCK_AND_ARRAY_START);
        if (blockIndex == std::string::npos) {
            return false;
        }
        CString recordName = ConvertToString(details.substr(0, blockIndex));

        size_t start = details.find_first_of(DumpUtils::ARRAY_START);
        size_t end = details.find_last_of(DumpUtils::ARRAY_END);
        if (start == std::string::npos || end == std::string::npos || start > end) {
            return false;
        }
        auto content = details.substr(start + 1, end - (start + 1) - 1);
        std::vector<std::string> infoStrings = StringHelper::SplitString(content, DumpUtils::BLOCK_SEPARATOR);
        if (infoStrings.size() <= 0) {
            continue;
        }

        ApEntityId recordId(0);
        recordPool_->TryAdd(recordName, recordId);
        ProfileType profileType(0, recordId, ProfileType::Kind::LocalRecordId);
        auto methodInfosIter = recordInfos_.find(profileType);
        PGOMethodInfoMap *methodInfos = nullptr;
        if (methodInfosIter == recordInfos_.end()) {
            methodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
            recordInfos_.emplace(profileType, methodInfos);
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
    std::string profilerString;
    bool isFirst = true;
    for (auto layoutInfoIter : moduleLayoutDescInfos_) {
        if (isFirst) {
            profilerString += DumpUtils::NEW_LINE;
            profilerString += DumpUtils::ARRAY_START + DumpUtils::SPACE;
            isFirst = false;
        } else {
            profilerString += DumpUtils::BLOCK_SEPARATOR + DumpUtils::SPACE;
        }
        profilerString += PGOHClassLayoutDescInner::GetTypeString(layoutInfoIter);
    }
    if (!isFirst) {
        profilerString += (DumpUtils::SPACE + DumpUtils::ARRAY_END + DumpUtils::NEW_LINE);
        stream << profilerString;
    }
    for (auto iter = recordInfos_.begin(); iter != recordInfos_.end(); iter++) {
        auto recordId = ApEntityId(iter->first.GetId());
        auto recordName = recordPool_->GetEntry(recordId)->GetData();
        auto methodInfos = iter->second;
        methodInfos->ProcessToText(hotnessThreshold_, recordName, stream);
    }
    recordPool_->GetPool()->ProcessToText(stream);
    profileTypePool_->GetPool()->ProcessToText(stream);
}

void PGORecordDetailInfos::InitSections()
{
    recordPool_ = std::make_unique<PGORecordPool>();
    apSectionList_.emplace_back(recordPool_->GetPool());
    profileTypePool_ = std::make_unique<PGOProfileTypePool>();
    apSectionList_.emplace_back(profileTypePool_->GetPool());
}

void PGORecordDetailInfos::Clear()
{
    for (auto iter : recordInfos_) {
        iter.second->Clear();
        nativeAreaAllocator_.Delete(iter.second);
    }
    recordInfos_.clear();
    recordPool_->Clear();
    profileTypePool_->Clear();
    apSectionList_.clear();
    chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
    InitSections();
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
    header_ = header;
    ParseSectionsFromBinary(apSectionList_, buffer, header);
    SectionInfo *info = header->GetRecordInfoSection();
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        CString recordName;
        ProfileType recordType;
        if (header->SupportProfileTypeWithAbcId()) {
            auto recordTypeRef = ProfileTypeRef(base::ReadBuffer<ApEntityId>(&addr, sizeof(ApEntityId)));
            recordType = ProfileType(*this, recordTypeRef);
            auto recordId = recordType.GetId();
            recordName = recordPool_->GetEntry(recordId)->GetData();
        } else if (header->SupportRecordPool()) {
            auto recordId = base::ReadBuffer<ApEntityId>(&addr, sizeof(ApEntityId));
            recordName = recordPool_->GetEntry(recordId)->GetData();
        } else {
            recordName = base::ReadBuffer(&addr);
        }
        PGOMethodIdSet *methodIds = nativeAreaAllocator_.New<PGOMethodIdSet>(chunk_.get());
        if (methodIds->ParseFromBinary(*this, &addr)) {
            methodIds_.emplace(recordName, methodIds);
        }
    }

    info = header->GetLayoutDescSection();
    if (info == nullptr) {
        return;
    }
    if (header->SupportTrackField()) {
        ParseFromBinaryForLayout(&addr);
    }
}

void PGORecordSimpleInfos::Merge(const PGORecordSimpleInfos &simpleInfos)
{
    std::map<ApEntityId, ApEntityId> idMapping;
    recordPool_->Merge(*simpleInfos.recordPool_, idMapping);
    for (const auto &method : simpleInfos.methodIds_) {
        auto result = methodIds_.find(method.first);
        if (result == methodIds_.end()) {
            PGOMethodIdSet *methodIds = nativeAreaAllocator_.New<PGOMethodIdSet>(chunk_.get());
            auto ret = methodIds_.emplace(method.first, methodIds);
            ASSERT(ret.second);
            result = ret.first;
        }
        const_cast<PGOMethodIdSet &>(*result->second).Merge(*method.second);
    }
    // Merge global layout desc infos to global method info map
    for (const auto &moduleLayoutDescInfo : simpleInfos.moduleLayoutDescInfos_) {
        auto result = moduleLayoutDescInfos_.find(moduleLayoutDescInfo);
        if (result == moduleLayoutDescInfos_.end()) {
            moduleLayoutDescInfos_.emplace(moduleLayoutDescInfo);
        } else {
            const_cast<PGOHClassLayoutDesc &>(*result).Merge(moduleLayoutDescInfo);
        }
    }
}

bool PGORecordSimpleInfos::ParseFromBinaryForLayout(void **buffer)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t i = 0; i < secInfo.number_; i++) {
        auto *info = base::ReadBufferInSize<PGOHClassLayoutDescInnerRef>(buffer);
        if (info == nullptr) {
            LOG_ECMA(INFO) << "Binary format error!";
            continue;
        }
        moduleLayoutDescInfos_.emplace(info->Convert(*this));
    }
    return true;
}

void PGORecordSimpleInfos::InitSections()
{
    recordPool_ = std::make_unique<PGORecordPool>();
    apSectionList_.emplace_back(recordPool_->GetPool());
    profileTypePool_ = std::make_unique<PGOProfileTypePool>();
    apSectionList_.emplace_back(profileTypePool_->GetPool());
}

void PGORecordSimpleInfos::Clear()
{
    for (const auto &iter : methodIds_) {
        iter.second->Clear();
        nativeAreaAllocator_.Delete(iter.second);
    }
    methodIds_.clear();
    recordPool_->Clear();
    profileTypePool_->Clear();
    apSectionList_.clear();
    chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
    InitSections();
}

PGORecordSimpleInfos::PGORecordSimpleInfos(uint32_t threshold) : hotnessThreshold_(threshold)
{
    chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
    InitSections();
}

PGORecordSimpleInfos::~PGORecordSimpleInfos()
{
    Clear();
}
} // namespace panda::ecmascript::pgo
