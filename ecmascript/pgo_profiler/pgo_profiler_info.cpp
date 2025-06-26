/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/js_thread.h"
#include "ecmascript/ohos/framework_helper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "libpandafile/bytecode_instruction-inl.h"
#include "zlib.h"

namespace panda::ecmascript::pgo {
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
        fileStream.write(reinterpret_cast<char *>(&localInfo), FileInfo::Size());
    }
    info->size_ = static_cast<uint32_t>(fileStream.tellp()) - info->offset_;
}

void PGOPandaFileInfos::Merge(const PGOPandaFileInfos &pandaFileInfos)
{
    for (const auto &info : pandaFileInfos.fileInfos_) {
        fileInfos_.emplace(info.GetChecksum(), info.GetAbcId());
    }
}

void PGOPandaFileInfos::MergeSafe(const PGOPandaFileInfos& pandaFileInfos)
{
    WriteLockHolder lock(fileInfosLock_);
    Merge(pandaFileInfos);
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
        pandaFileInfo += (std::to_string(info.GetAbcId()) + DumpUtils::BLOCK_START);
        pandaFileInfo += std::to_string(info.GetChecksum());
    }

    pandaFileInfo += (DumpUtils::SPACE + DumpUtils::ARRAY_END + DumpUtils::NEW_LINE);
    stream << pandaFileInfo;
}

bool PGOPandaFileInfos::Checksum(const std::unordered_map<CString, uint32_t>& fileNameToChecksumMap,
                                 const std::shared_ptr<PGOAbcFilePool>& abcFilePool) const
{
    for (const auto& fileNameToChecksumPair: fileNameToChecksumMap) {
        ApEntityId abcId(0);
        abcFilePool->GetEntryIdByNormalizedName(fileNameToChecksumPair.first, abcId);
        FileInfo tempInfo = FileInfo(fileNameToChecksumPair.second, abcId);
        auto it = fileInfos_.find(tempInfo);
        if (it != fileInfos_.end()) {
            if (it->GetChecksum() != tempInfo.GetChecksum()) {
                LOG_ECMA(ERROR)
                    << "Checksum verification failed. Please ensure that the "
                       ".abc and .ap match. Fail file: "
                    << fileNameToChecksumPair.first << "\n"
                    << " compile file checksum: "
                    << fileNameToChecksumPair.second
                    << " recorded checksum in ap file: " << it->GetChecksum();
                return false;
            }
        }
    }
    return true;
}

bool PGOPandaFileInfos::Checksum(const std::unordered_map<CString, uint32_t>& fileNameToChecksumMap) const
{
    for (const auto& fileNameToChecksumPair: fileNameToChecksumMap) {
        for (const auto &fileInfo : fileInfos_) {
            if (fileInfo.GetChecksum() == fileNameToChecksumPair.second) {
                return true;
            }
        }
    }
    LOG_ECMA(ERROR) << "Checksum verification failed. Please ensure that the .abc and .ap match.";
    return false;
}

void PGOPandaFileInfos::UpdateFileInfosAbcID(const PGOContext &context)
{
    std::set<FileInfo> newFileInfos;
    auto oldToNewInfoMap = context.GetAbcIdRemap();
    for (const auto &fileInfo : fileInfos_) {
        auto changeInfo = oldToNewInfoMap.find(fileInfo.GetAbcId());
        if (changeInfo != oldToNewInfoMap.end()) {
            newFileInfos.emplace(fileInfo.GetChecksum(), changeInfo->second);
        } else {
            newFileInfos.emplace(fileInfo);
        }
    }
    fileInfos_.swap(newFileInfos);
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

void PGOMethodInfo::ProcessToJson(ProfileType::VariantMap &function) const
{
    std::string methodName = GetMethodName();
    std::string functionName = methodName + "(" + std::to_string(GetMethodId().GetOffset()) + ")";
    function.insert(std::make_pair(DumpJsonUtils::FUNCTION_NAME, functionName));
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

bool PGOMethodInfoMap::AddMethod(Chunk *chunk, Method *jsMethod, SampleMode mode)
{
    PGOMethodId methodId(jsMethod->GetMethodId());
    auto result = methodInfos_.find(methodId);
    if (result != methodInfos_.end()) {
        auto info = result->second;
        info->IncreaseCount();
        info->SetSampleMode(mode);
        return false;
    } else {
        CString methodName = jsMethod->GetMethodName();
        size_t strlen = methodName.size();
        size_t size = static_cast<size_t>(PGOMethodInfo::Size(strlen));
        void *infoAddr = chunk->Allocate(size);
        if (infoAddr == nullptr) {
            LOG_ECMA(ERROR) << "infoAddr is null!";
            return false;
        }
        auto info = new (infoAddr) PGOMethodInfo(methodId, 0, mode, methodName.c_str());
        info->IncreaseCount();
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

bool PGOMethodInfoMap::AddDefine(Chunk *chunk, PGOMethodId methodId, int32_t offset, PGODefineOpType type)
{
    auto typeInfoSet = GetOrInsertMethodTypeSet(chunk, methodId);
    ASSERT(typeInfoSet != nullptr);
    typeInfoSet->AddDefine(offset, type);
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

bool PGOMethodInfoMap::SkipMethodFromBinary(PGOProfilerHeader* header,
                                            void** addr,
                                            void* buffer,
                                            size_t bufferSize) const
{
    if (header->SupportMethodChecksum()) {
        base::ReadBuffer<uint32_t>(addr, sizeof(uint32_t));
        if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "SkipMethodChecksum")) {
            return false;
        }
    }
    if (header->SupportType()) {
        PGOMethodTypeSet::SkipFromBinary(addr);
        if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "SkipPGOMethodTypeSet")) {
            return false;
        }
    }
    return true;
}

bool PGOMethodInfoMap::ParseMethodFromBinary(
    Chunk* chunk, PGOContext& context, PGOMethodInfo* info, void** addr, void* buffer, size_t bufferSize)
{
    PGOProfilerHeader* const header = context.GetHeader();
    methodInfos_.emplace(info->GetMethodId(), info);
    LOG_ECMA(DEBUG) << "Method:" << info->GetMethodId() << DumpUtils::ELEMENT_SEPARATOR << info->GetCount()
                    << DumpUtils::ELEMENT_SEPARATOR << std::to_string(static_cast<int>(info->GetSampleMode()))
                    << DumpUtils::ELEMENT_SEPARATOR << info->GetMethodName();

    if (header->SupportMethodChecksum()) {
        auto checksum = base::ReadBuffer<uint32_t>(addr, sizeof(uint32_t));
        if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "MethodChecksum")) {
            return false;
        }
        methodsChecksum_.emplace(info->GetMethodId(), checksum);
    }

    if (header->SupportType()) {
        auto typeInfoSet = chunk->New<PGOMethodTypeSet>();
        size_t newSize = bufferSize - (reinterpret_cast<uintptr_t>(*addr) - reinterpret_cast<uintptr_t>(buffer));
        if (!typeInfoSet->ParseFromBinary(context, addr, newSize)) {
            return false;
        }
        methodTypeInfos_.emplace(info->GetMethodId(), typeInfoSet);
    }
    return true;
}

bool PGOMethodInfoMap::ParseFromBinary(Chunk* chunk, PGOContext& context, void** addr, size_t bufferSize)
{
    PGOProfilerHeader* const header = context.GetHeader();
    ASSERT(header != nullptr);
    void* buffer = *addr;
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(addr);
    if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "PGOMethodInfoMap")) {
        return false;
    }

    for (uint32_t j = 0; j < secInfo.number_; j++) {
        PGOMethodInfo* info = base::ReadBufferInSize<PGOMethodInfo>(addr);
        if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "PGOMethodInfo")) {
            return false;
        }

        if (info->IsFilter(context.GetHotnessThreshold())) {
            if (!SkipMethodFromBinary(header, addr, buffer, bufferSize)) {
                return false;
            }
            continue;
        }

        if (!ParseMethodFromBinary(chunk, context, info, addr, buffer, bufferSize)) {
            return false;
        }
    }
    return true;
}

bool PGOMethodInfoMap::ProcessToBinary(PGOContext& context,
                                       ProfileTypeRef recordProfileRef,
                                       std::fstream& stream,
                                       PGOProfilerHeader* const header) const
{
    SectionInfo secInfo;
    std::stringstream methodStream;
    for (auto iter = methodInfos_.begin(); iter != methodInfos_.end(); iter++) {
        LOG_ECMA(DEBUG) << "Method:" << iter->first << DumpUtils::ELEMENT_SEPARATOR << iter->second->GetCount()
                        << DumpUtils::ELEMENT_SEPARATOR
                        << std::to_string(static_cast<int>(iter->second->GetSampleMode()))
                        << DumpUtils::ELEMENT_SEPARATOR << iter->second->GetMethodName();
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

        void *infoAddr = chunk->Allocate(PGOMethodInfo::Size(methodName.size()));
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
            ASSERT(end > start + 1);
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
            profilerString += DumpUtils::BLOCK_START + DumpUtils::SPACE + DumpUtils::ARRAY_START;
            profilerString += DumpUtils::NEW_LINE + DumpUtils::ALIGN;
            isFirst = false;
        } else {
            profilerString += DumpUtils::BLOCK_SEPARATOR + DumpUtils::NEW_LINE + DumpUtils::ALIGN;
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
        profilerString += (DumpUtils::NEW_LINE + DumpUtils::ARRAY_END + DumpUtils::NEW_LINE);
        stream << profilerString;
    }
}

void PGOMethodInfoMap::ProcessToJson(uint32_t threshold, ProfileType::jModuleType &jModule) const
{
    std::vector<ProfileType::VariantMap> functionArray;
    for (auto methodInfoIter : methodInfos_) {
        auto methodInfo = methodInfoIter.second;
        if (methodInfo->IsFilter(threshold)) {
            continue;
        }
        ProfileType::VariantMap function;
        methodInfo->ProcessToJson(function);
        auto iter = methodTypeInfos_.find(methodInfo->GetMethodId());
        if (iter != methodTypeInfos_.end()) {
            ProfileType::VariantVector typeArray;
            iter->second->ProcessToJson(typeArray);
            function.insert(std::make_pair(DumpJsonUtils::TYPE, typeArray));
        }
        functionArray.push_back(function);
    }
    jModule.insert(std::make_pair(DumpJsonUtils::FUNCTION, functionArray));
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
            methodInfo.GetPGOMethodTypeSet().ParseFromBinary(context, buffer, PGOProfilerEncoder::MAX_AP_FILE_SIZE);
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

bool PGORecordDetailInfos::AddMethod(ProfileType recordProfileType, Method *jsMethod, SampleMode mode)
{
    auto curMethodInfos = GetMethodInfoMap(recordProfileType);
    ASSERT(curMethodInfos != nullptr);
    ASSERT(jsMethod != nullptr);
    return curMethodInfos->AddMethod(chunk_.get(), jsMethod, mode);
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
    ProfileType recordProfileType, PGOMethodId methodId, int32_t offset, PGODefineOpType type)
{
    auto curMethodInfos = GetMethodInfoMap(recordProfileType);
    ASSERT(curMethodInfos != nullptr);
    curMethodInfos->AddDefine(chunk_.get(), methodId, offset, type);

    PGOHClassTreeDesc descInfo(type.GetProfileType());
    auto iter = hclassTreeDescInfos_.find(descInfo);
    if (iter == hclassTreeDescInfos_.end()) {
        descInfo.SetProtoPt(type.GetPrototypePt());
        hclassTreeDescInfos_.emplace(descInfo);
    } else {
        const_cast<PGOHClassTreeDesc &>(*iter).SetProtoPt(type.GetPrototypePt());
    }
    return true;
}

bool PGORecordDetailInfos::AddRootLayout(JSTaggedType hclass, ProfileType rootType)
{
    PGOHClassTreeDesc descInfo(rootType);
    auto iter = hclassTreeDescInfos_.find(descInfo);
    if (iter != hclassTreeDescInfos_.end()) {
        return const_cast<PGOHClassTreeDesc &>(*iter).DumpForRoot(hclass, rootType);
    } else {
        if (!descInfo.DumpForRoot(hclass, rootType)) {
            return false;
        }
        hclassTreeDescInfos_.emplace(descInfo);
    }
    return true;
}

bool PGORecordDetailInfos::UpdateLayout(ProfileType rootType, JSTaggedType hclass, ProfileType curType)
{
    PGOHClassTreeDesc descInfo(rootType);
    auto iter = hclassTreeDescInfos_.find(descInfo);
    if (iter != hclassTreeDescInfos_.end()) {
        return const_cast<PGOHClassTreeDesc &>(*iter).UpdateLayout(hclass, curType);
    } else {
        if (!descInfo.UpdateLayout(hclass, curType)) {
            return false;
        }
        hclassTreeDescInfos_.emplace(descInfo);
        return false;
    }
    return true;
}

bool PGORecordDetailInfos::UpdateTransitionLayout(
    ProfileType rootType, JSTaggedType parent, ProfileType parentType, JSTaggedType child, ProfileType childType)
{
    PGOHClassTreeDesc descInfo(rootType);
    auto iter = hclassTreeDescInfos_.find(descInfo);
    if (iter != hclassTreeDescInfos_.end()) {
        return const_cast<PGOHClassTreeDesc &>(*iter).UpdateForTransition(parent, parentType, child, childType);
    } else {
        if (!descInfo.UpdateForTransition(parent, parentType, child, childType)) {
            return false;
        }
        hclassTreeDescInfos_.emplace(descInfo);
    }
    return true;
}

void PGORecordDetailInfos::AddRootPtType(ProfileType rootType, ProfileType ptType)
{
    PGOHClassTreeDesc descInfo(rootType);
    auto iter = hclassTreeDescInfos_.find(descInfo);
    if (iter != hclassTreeDescInfos_.end()) {
        const_cast<PGOHClassTreeDesc &>(*iter).SetProtoPt(ptType);
    } else {
        descInfo.SetProtoPt(ptType);
        hclassTreeDescInfos_.emplace(descInfo);
    }
}

bool PGORecordDetailInfos::IsDumped(ProfileType rootType, ProfileType curType) const
{
    PGOHClassTreeDesc descInfo(rootType);
    auto iter = hclassTreeDescInfos_.find(descInfo);
    if (iter != hclassTreeDescInfos_.end()) {
        return const_cast<PGOHClassTreeDesc &>(*iter).IsDumped(curType);
    }
    return false;
}

void PGORecordDetailInfos::Merge(const PGORecordDetailInfos &recordInfos)
{
    const auto& methodInfos = recordInfos.recordInfos_;
    for (auto& iter: methodInfos) {
        auto recordType = iter.first;
        auto fromMethodInfos = iter.second;

        auto recordInfosIter = recordInfos_.find(recordType);
        PGOMethodInfoMap *toMethodInfos = nullptr;
        if (recordInfosIter == recordInfos_.end()) {
            toMethodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
            recordInfos_.emplace(recordType, toMethodInfos);
        } else {
            toMethodInfos = recordInfosIter->second;
        }

        ASSERT(toMethodInfos != nullptr);
        toMethodInfos->Merge(chunk_.get(), fromMethodInfos);
    }

    recordPool_->Merge(*recordInfos.recordPool_);
    protoTransitionPool_->Merge(*recordInfos.protoTransitionPool_);
    // Merge global layout desc infos to global method info map
    const auto& hclassTreeDescInfos = recordInfos.hclassTreeDescInfos_;
    for (auto& fromInfo: hclassTreeDescInfos) {
        auto result = hclassTreeDescInfos_.find(fromInfo);
        if (result == hclassTreeDescInfos_.end()) {
            PGOHClassTreeDesc descInfo(fromInfo.GetProfileType());
            descInfo.SetProtoPt(fromInfo.GetProtoPt());
            descInfo.Merge(fromInfo);
            hclassTreeDescInfos_.emplace(descInfo);
        } else {
            const_cast<PGOHClassTreeDesc &>(*result).Merge(fromInfo);
        }
    }
}

void PGORecordDetailInfos::MergeSafe(const PGORecordDetailInfos& recordInfos)
{
    LockHolder lock(mutex_);
    Merge(recordInfos);
}

bool PGORecordDetailInfos::ParseSectionsFromBinary(void* buffer, PGOProfilerHeader* const header)
{
    // ProfileTypePool must be parsed at first
    PGOFileSectionInterface::ParseSectionFromBinary(*this, buffer, header, *profileTypePool_->GetPool());
    if (!abcIdRemap_.empty()) {
        // step2: [abc pool merge] remap decoder's profileType pool's abcId field.
        LOG_ECMA(DEBUG) << "remap with abcRemapSize: " << abcIdRemap_.size();
        profileTypePool_->Remap(*this);
    }
    PGOFileSectionInterface::ParseSectionFromBinary(*this, buffer, header, *protoTransitionPool_);
    PGOFileSectionInterface::ParseSectionFromBinary(*this, buffer, header, *recordPool_);
    return true;
}

bool PGORecordDetailInfos::ParseRecordTypeFromBinary(PGOProfilerHeader* header,
                                                     void** addr,
                                                     void* buffer,
                                                     size_t bufferSize,
                                                     ApEntityId& recordId,
                                                     ProfileType& recordType)
{
    if (header->SupportProfileTypeWithAbcId()) {
        auto recordTypeRef = ProfileTypeRef(base::ReadBuffer<ApEntityId>(addr, sizeof(ApEntityId)));
        if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "ProfileTypeRef")) {
            return false;
        }
        auto res = ProfileType::CreateFromProfileTypeRef(*this, recordTypeRef);
        if (!res.has_value()) {
            LOG_ECMA(ERROR) << "ParseFromBinary failed, current addr: " << *addr << std::endl;
            return false;
        }
        recordType = res.value();
        recordId = recordType.GetId();
    } else if (header->SupportRecordPool()) {
        recordId = base::ReadBuffer<ApEntityId>(addr, sizeof(ApEntityId));
        if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "recordId")) {
            return false;
        }
    } else {
        auto* recordName = base::ReadBuffer(addr);
        if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "recordName")) {
            return false;
        }
        recordPool_->Add(ProfileType(recordId), recordName);
    }
    recordType.UpdateId(recordId);
    recordType.UpdateKind(ProfileType::Kind::RecordClassId);
    return true;
}

bool PGORecordDetailInfos::ParseRecordInfosFromBinary(void* buffer, PGOProfilerHeader* header, size_t bufferSize)
{
    SectionInfo* info = header->GetRecordInfoSection();
    if (info == nullptr) {
        LOG_PGO(ERROR) << "[ParseRecordInfosFromBinary] section info is nullptr";
    }
    void* addr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        ApEntityId recordId(0);
        ProfileType recordType;
        if (!ParseRecordTypeFromBinary(header, &addr, buffer, bufferSize, recordId, recordType)) {
            return false;
        }
        PGOMethodInfoMap *methodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
        ASSERT(methodInfos != nullptr);
        size_t newSize = bufferSize - (reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(buffer));
        if (!methodInfos->ParseFromBinary(chunk_.get(), *this, &addr, newSize)) {
            return false;
        }
        if (!methodInfos->GetMethodInfos().empty()) {
            recordInfos_.emplace(recordType, methodInfos);
        } else {
            nativeAreaAllocator_.Delete(methodInfos);
        }
    }
    return true;
}

bool PGORecordDetailInfos::ParseFromBinary(void* buffer, PGOProfilerHeader* const header, size_t bufferSize)
{
    header_ = header;
    if (!ParseSectionsFromBinary(buffer, header)) {
        return false;
    }
    if (!ParseRecordInfosFromBinary(buffer, header, bufferSize)) {
        return false;
    }
    SectionInfo* info = header->GetLayoutDescSection();
    if (info == nullptr) {
        return false;
    }
    if (header->SupportTrackField()) {
        void* addr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
        if (!ParseFromBinaryForLayout(&addr, buffer, bufferSize)) {
            return false;
        }
    }
    return true;
}

bool PGORecordDetailInfos::ParseFromBinaryForLayout(void** addr, void* buffer, size_t bufferSize)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(addr);
    if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "ParseFromBinaryForLayout")) {
        return false;
    }
    for (uint32_t i = 0; i < secInfo.number_; i++) {
        auto* info = base::ReadBufferInSize<PGOHClassTreeDescInnerRef>(addr);
        if (!base::CheckBufferBounds(*addr, buffer, bufferSize, "PGOHClassTreeDescInnerRef")) {
            return false;
        }
        if (info == nullptr) {
            LOG_ECMA(INFO) << "Binary format error!";
            continue;
        }
        hclassTreeDescInfos_.emplace(info->Convert(*this));
    }
    return true;
}

void PGORecordDetailInfos::ProcessToBinary(std::fstream& fileStream, PGOProfilerHeader* const header)
{
    header_ = header;
    auto info = header->GetRecordInfoSection();
    info->number_ = 0;
    info->offset_ = static_cast<uint32_t>(fileStream.tellp());
    for (auto iter = recordInfos_.begin(); iter != recordInfos_.end(); iter++) {
        auto recordId = iter->first;
        auto curMethodInfos = iter->second;
        if (curMethodInfos->ProcessToBinary(*this, ProfileTypeRef(*this, recordId), fileStream, header)) {
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
        if (!ProcessToBinaryForLayout(const_cast<NativeAreaAllocator*>(&nativeAreaAllocator_), fileStream)) {
            return;
        }
        info->number_++;
    }
    info->size_ = static_cast<uint32_t>(fileStream.tellp()) - info->offset_;
    PGOFileSectionInterface::ProcessSectionToBinary(*this, fileStream, header, *recordPool_);
    PGOFileSectionInterface::ProcessSectionToBinary(*this, fileStream, header, *protoTransitionPool_);
    // ProfileTypePool must be processed at last
    PGOFileSectionInterface::ProcessSectionToBinary(*this, fileStream, header, *profileTypePool_->GetPool());
}

bool PGORecordDetailInfos::ProcessToBinaryForLayout(NativeAreaAllocator* allocator, std::fstream& stream)
{
    SectionInfo secInfo;
    auto layoutBeginPosition = stream.tellp();
    stream.seekp(sizeof(SectionInfo), std::ofstream::cur);
    for (const auto& typeInfo: hclassTreeDescInfos_) {
        auto profileType = PGOSampleType(typeInfo.GetProfileType());
        size_t size = PGOHClassTreeDescInnerRef::CaculateSize(typeInfo);
        if (size == 0) {
            continue;
        }
        PGOSampleTypeRef classRef = PGOSampleTypeRef::ConvertFrom(*this, profileType);
        auto protoSt = PGOSampleType(typeInfo.GetProtoPt());
        PGOSampleTypeRef protoClassRef = PGOSampleTypeRef::ConvertFrom(*this, protoSt);
        void *addr = allocator->Allocate(size);
        auto descInfos = new (addr) PGOHClassTreeDescInnerRef(size, classRef, protoClassRef);
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

void PGORecordDetailInfos::ProcessToText(std::ofstream &stream) const
{
    std::string profilerString;
    bool isFirst = true;
    for (auto layoutInfoIter : hclassTreeDescInfos_) {
        if (isFirst) {
            profilerString += DumpUtils::NEW_LINE;
            profilerString += DumpUtils::ARRAY_START + DumpUtils::NEW_LINE;
            profilerString += DumpUtils::ALIGN;
            isFirst = false;
        } else {
            profilerString += DumpUtils::BLOCK_SEPARATOR + DumpUtils::NEW_LINE;
            profilerString += DumpUtils::ALIGN;
        }
        profilerString += PGOHClassTreeDescInner::GetTypeString(layoutInfoIter);
    }
    if (!isFirst) {
        profilerString += (DumpUtils::NEW_LINE + DumpUtils::ARRAY_END + DumpUtils::NEW_LINE);
        stream << profilerString;
    }
    for (auto iter = recordInfos_.begin(); iter != recordInfos_.end(); iter++) {
        const CString recordName(recordPool_->GetName(iter->first));
        if (recordName.empty()) {
            LOG_ECMA(ERROR) << "record name is empty, " << iter->first.GetTypeString();
            continue;
        }
        auto methodInfos = iter->second;
        methodInfos->ProcessToText(hotnessThreshold_, recordName, stream);
    }
    recordPool_->ProcessToText(stream);
    protoTransitionPool_->ProcessToText(stream);
    // ProfileTypePool must be processed at last
    profileTypePool_->GetPool()->ProcessToText(stream);
}

void PGORecordDetailInfos::InitSections()
{
    recordPool_ = std::make_unique<PGORecordPool>();
    protoTransitionPool_ = std::make_unique<PGOProtoTransitionPool>();
    profileTypePool_ = std::make_unique<PGOProfileTypePool>();
}

void PGORecordDetailInfos::Clear()
{
    for (auto iter : recordInfos_) {
        iter.second->Clear();
        nativeAreaAllocator_.Delete(iter.second);
    }
    for (auto iter : hclassTreeDescInfos_) {
        iter.Clear();
    }
    hclassTreeDescInfos_.clear();
    recordInfos_.clear();
    recordPool_->Clear();
    protoTransitionPool_->Clear();
    profileTypePool_->Clear();
    hclassTreeDescInfos_.clear();
    abcIdRemap_.clear();
    chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
    InitSections();
}

void PGORecordDetailInfos::ClearSafe()
{
    LockHolder lock(mutex_);
    Clear();
}

bool PGORecordSimpleInfos::Match(const CString &abcNormalizedDesc, const CString &recordName, EntityId methodId)
{
    auto abcMethodIds = methodIds_.find(abcNormalizedDesc);
    if (abcMethodIds == methodIds_.end()) {
        LOG_COMPILER(DEBUG) << "AbcDesc not found. abcNormalizedDesc: " << abcNormalizedDesc
                            << ", methodIdsCount: " << methodIds_.size();
        return false;
    }
    auto methodIdsIter = abcMethodIds->second.find(recordName);
    if (methodIdsIter == abcMethodIds->second.end()) {
        LOG_COMPILER(DEBUG) << "AbcDesc not found. recordName: " << recordName;
        return false;
    }
    return methodIdsIter->second->Match(methodId);
}

void PGORecordSimpleInfos::ParseFromBinary(void *buffer, PGOProfilerHeader *const header,
                                           std::shared_ptr<PGOAbcFilePool> &abcFilePool)
{
    header_ = header;
    // ProfileTypePool must be parsed at first
    if (!PGOFileSectionInterface::ParseSectionFromBinary(*this, buffer, header, *profileTypePool_->GetPool())) {
        LOG_ECMA(ERROR) << "Parse from binary failed for profile type pool.";
        return;
    }
    if (!abcIdRemap_.empty()) {
        // step2: [abc pool merge] remap decoder's profileType pool's abcId field.
        LOG_ECMA(DEBUG) << "remap with abcRemapSize: " << abcIdRemap_.size();
        profileTypePool_->Remap(*this);
    }
    if (!PGOFileSectionInterface::ParseSectionFromBinary(*this, buffer, header, *protoTransitionPool_)) {
        LOG_ECMA(ERROR) << "Parse from binary failed for proto transition pool.";
        return;
    }
    if (!PGOFileSectionInterface::ParseSectionFromBinary(*this, buffer, header, *recordPool_)) {
        LOG_ECMA(ERROR) << "Parse from binary failed for record pool.";
        return;
    }
    SectionInfo *info = header->GetRecordInfoSection();
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        CString recordName;
        const char *abcDesc = "";
        ProfileType recordType;
        if (header->SupportProfileTypeWithAbcId()) {
            auto recordTypeRef = ProfileTypeRef(base::ReadBuffer<ApEntityId>(&addr, sizeof(ApEntityId)));
            recordType = ProfileType(*this, recordTypeRef);
            recordName = recordPool_->GetName(recordType);
            auto abcId = recordType.GetAbcId();
            const auto *entry = abcFilePool->GetPool()->GetEntry(abcId);
            if (entry != nullptr) {
                abcDesc = entry->GetData().c_str();
            }
        } else if (header->SupportRecordPool()) {
            auto recordId = base::ReadBuffer<ApEntityId>(&addr, sizeof(ApEntityId));
            recordName = recordPool_->GetName(ProfileType(recordId));
        } else {
            recordName = base::ReadBuffer(&addr);
        }
        PGOMethodIdSet *methodIds = nativeAreaAllocator_.New<PGOMethodIdSet>(chunk_.get());
        if (methodIds->ParseFromBinary(*this, &addr)) {
            auto methodIdsResult = methodIds_.try_emplace(JSPandaFile::GetNormalizedFileDesc(abcDesc));
            // check record name, the default record name of the framework abc does not enter the aot compilation
            FrameworkHelper::GetRealRecordName(recordName);
            (methodIdsResult.first->second).emplace(recordName, methodIds);
        } else {
            nativeAreaAllocator_.Delete(methodIds);
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
    for (const auto &fromAbcMethodIds : simpleInfos.methodIds_) {
        auto toAbcMethodIds = methodIds_.try_emplace(fromAbcMethodIds.first);
        for (const auto &method : fromAbcMethodIds.second) {
            auto result = toAbcMethodIds.first->second.find(method.first);
            if (result == toAbcMethodIds.first->second.end()) {
                PGOMethodIdSet *methodIds = nativeAreaAllocator_.New<PGOMethodIdSet>(chunk_.get());
                auto ret = toAbcMethodIds.first->second.emplace(method.first, methodIds);
                ASSERT(ret.second);
                result = ret.first;
            }
            const_cast<PGOMethodIdSet &>(*result->second).Merge(*method.second);
        }
    }
    recordPool_->Merge(*simpleInfos.recordPool_);
    protoTransitionPool_->Merge(*simpleInfos.protoTransitionPool_);
    // Merge global layout desc infos to global method info map
    for (const auto &hclassTreeDescInfo : simpleInfos.hclassTreeDescInfos_) {
        auto result = hclassTreeDescInfos_.find(hclassTreeDescInfo);
        if (result == hclassTreeDescInfos_.end()) {
            PGOHClassTreeDesc descInfo(hclassTreeDescInfo.GetProfileType());
            descInfo.SetProtoPt(hclassTreeDescInfo.GetProtoPt());
            descInfo.Merge(hclassTreeDescInfo);
            hclassTreeDescInfos_.emplace(descInfo);
        } else {
            const_cast<PGOHClassTreeDesc &>(*result).Merge(hclassTreeDescInfo);
        }
    }
}

bool PGORecordSimpleInfos::ParseFromBinaryForLayout(void **buffer)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t i = 0; i < secInfo.number_; i++) {
        auto *info = base::ReadBufferInSize<PGOHClassTreeDescInnerRef>(buffer);
        if (info == nullptr) {
            LOG_ECMA(INFO) << "Binary format error!";
            continue;
        }
        hclassTreeDescInfos_.emplace(info->Convert(*this));
    }
    return true;
}

void PGORecordSimpleInfos::InitSections()
{
    recordPool_ = std::make_unique<PGORecordPool>();
    protoTransitionPool_ = std::make_unique<PGOProtoTransitionPool>();
    profileTypePool_ = std::make_unique<PGOProfileTypePool>();
}

void PGORecordSimpleInfos::Clear()
{
    for (const auto &abcMethodIds: methodIds_) {
        for (const auto &iter : abcMethodIds.second) {
            iter.second->Clear();
            nativeAreaAllocator_.Delete(iter.second);
        }
    }
    for (auto iter : hclassTreeDescInfos_) {
        iter.Clear();
    }
    hclassTreeDescInfos_.clear();
    methodIds_.clear();
    recordPool_->Clear();
    profileTypePool_->Clear();
    hclassTreeDescInfos_.clear();
    abcIdRemap_.clear();
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
