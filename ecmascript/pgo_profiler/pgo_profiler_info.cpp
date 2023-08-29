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

#include "ecmascript/base/bit_helper.h"
#include "ecmascript/base/file_header.h"
#include "ecmascript/js_function.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"
#include "ecmascript/pgo_profiler/pgo_record_pool.h"
#include "macros.h"
#include "securec.h"
#include "zlib.h"

namespace panda::ecmascript {
using StringHelper = base::StringHelper;
bool PGOProfilerHeader::BuildFromLegacy(void *buffer, PGOProfilerHeader **header)
{
    auto *inHeader = reinterpret_cast<PGOProfilerHeaderLegacy *>(buffer);
    size_t desSize = Size(inHeader->GetSectionNumber());
    if (desSize > LastSize()) {
        LOG_ECMA(ERROR) << "header size error, expected size is less than " << LastSize() << ", but got " << desSize;
        return false;
    }
    Build(header, desSize);
    // copy header base.
    if (memcpy_s(*header, sizeof(FileHeaderBase), inHeader, sizeof(FileHeaderBase)) != EOK) {
        UNREACHABLE();
    }
    // skip elastic header field, and copy section info from incoming buffer.
    auto sectionSize = desSize - sizeof(FileHeaderElastic);
    if (memcpy_s(&((*header)->sectionNumber_), sectionSize, &(inHeader->GetSectionNumber()), sectionSize) != EOK) {
        UNREACHABLE();
    }
    return true;
}

bool PGOProfilerHeader::BuildFromElastic(void *buffer, size_t bufferSize, PGOProfilerHeader **header)
{
    auto *inHeader = reinterpret_cast<PGOProfilerHeader *>(buffer);
    if (!inHeader->Verify(buffer, bufferSize)) {
        return false;
    }
    size_t desSize = inHeader->Size();
    if (desSize > LastSize()) {
        LOG_ECMA(ERROR) << "header size error, expected size is less than " << LastSize() << ", but got " << desSize;
        return false;
    }
    Build(header, desSize);
    if (memcpy_s(*header, desSize, inHeader, desSize) != EOK) {
        UNREACHABLE();
    }
    return true;
}

bool PGOProfilerHeader::ParseFromBinary(void *buffer, size_t bufferSize, PGOProfilerHeader **header)
{
    auto *inHeaderBase = reinterpret_cast<FileHeaderBase *>(buffer);
    if (inHeaderBase->VerifyVersion("apPath file", LAST_VERSION, false)) {
        if (!inHeaderBase->CompatibleVerify(ELASTIC_HEADER_MINI_VERSION)) {
            return BuildFromLegacy(buffer, header);
        }
        return BuildFromElastic(buffer, bufferSize, header);
    }
    return false;
}

bool PGOProfilerHeader::VerifyFileSize(size_t bufferSize) const
{
    if (!SupportFileSize()) {
        return true;
    }
    if (GetFileSize() != bufferSize) {
        LOG_ECMA(ERROR) << "Verify ap file's file size failed. size: " << std::hex << bufferSize << " vs "
                        << GetFileSize();
        return false;
    }
    return true;
}

bool PGOProfilerHeader::VerifyConsistency(void *buffer, size_t bufferSize) const
{
    if (!SupportFileConsistency()) {
        return true;
    }
    uint32_t checksum = adler32(0, reinterpret_cast<const Bytef *>(buffer) + MAGIC_SIZE, VERSION_SIZE);
    checksum = adler32(checksum, reinterpret_cast<const Bytef *>(buffer) + CHECKSUM_END_OFFSET,
                       bufferSize - CHECKSUM_END_OFFSET);
    if (checksum != GetChecksum()) {
        LOG_ECMA(ERROR) << "Verify ap file's consistency failed. checksum: " << std::hex << checksum << " vs "
                        << std::hex << GetChecksum();
        return false;
    }
    return true;
}

void PGOProfilerHeader::ProcessToBinary(std::fstream &fileStream) const
{
    fileStream.seekp(0);
    if (base::FileHeaderBase::CompatibleVerify(ELASTIC_HEADER_MINI_VERSION)) {
        fileStream.write(reinterpret_cast<const char *>(this), Size());
    } else {
        // copy header base.
        fileStream.write(reinterpret_cast<const char *>(this), sizeof(FileHeaderBase));
        // skip elastic header field, and copy section info from incoming buffer.
        auto sectionSize = Size() - sizeof(FileHeaderElastic);
        fileStream.write(reinterpret_cast<const char *>(&sectionNumber_), sectionSize);
    }
}

bool PGOProfilerHeader::ParseFromText(std::ifstream &stream)
{
    std::string header;
    if (std::getline(stream, header)) {
        if (header.empty()) {
            return false;
        }
        auto index = header.find(PGODumpUtils::BLOCK_START);
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
        if (!base::FileHeaderBase::CompatibleVerify(ELASTIC_HEADER_MINI_VERSION)) {
            auto *pandaInfoSection = GetPandaInfoSection();
            pandaInfoSection->offset_ -= sizeof(PGOProfilerHeader) - sizeof(PGOProfilerHeaderLegacy);
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
    stream << PGODumpUtils::VERSION_HEADER << InternalGetVersion() << PGODumpUtils::NEW_LINE;
    if (SupportFileConsistency()) {
        stream << "FileSize: " << GetFileSize() << " ,HeaderSize: " << GetHeaderSize() << " ,Checksum: " << std::hex
               << GetChecksum() << PGODumpUtils::NEW_LINE;
    }
    return true;
}

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

        size_t start = pandaFileInfo.find_first_of(PGODumpUtils::ARRAY_START);
        size_t end = pandaFileInfo.find_last_of(PGODumpUtils::ARRAY_END);
        if (start == std::string::npos || end == std::string::npos || start > end) {
            return false;
        }
        auto content = pandaFileInfo.substr(start + 1, end - (start + 1) - 1);
        std::vector<std::string> infos = StringHelper::SplitString(content, PGODumpUtils::BLOCK_SEPARATOR);
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
    std::string pandaFileInfo = PGODumpUtils::NEW_LINE + PGODumpUtils::PANDA_FILE_INFO_HEADER;
    bool isFirst = true;
    for (auto &info : fileInfos_) {
        if (!isFirst) {
            pandaFileInfo += PGODumpUtils::BLOCK_SEPARATOR + PGODumpUtils::SPACE;
        } else {
            isFirst = false;
        }
        pandaFileInfo += std::to_string(info.GetChecksum());
    }

    pandaFileInfo += (PGODumpUtils::SPACE + PGODumpUtils::ARRAY_END + PGODumpUtils::NEW_LINE);
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
    text += PGODumpUtils::ELEMENT_SEPARATOR;
    text += std::to_string(GetCount());
    text += PGODumpUtils::ELEMENT_SEPARATOR;
    text += GetSampleModeToString();
    text += PGODumpUtils::ELEMENT_SEPARATOR;
    text += GetMethodName();
}

std::vector<std::string> PGOMethodInfo::ParseFromText(const std::string &infoString)
{
    std::vector<std::string> infoStrings = StringHelper::SplitString(infoString, PGODumpUtils::ELEMENT_SEPARATOR);
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

void PGOMethodTypeSet::Merge(const PGOMethodTypeSet *info)
{
    for (const auto &fromType : info->scalarOpTypeInfos_) {
        auto iter = scalarOpTypeInfos_.find(fromType);
        if (iter != scalarOpTypeInfos_.end()) {
            const_cast<ScalarOpTypeInfo &>(*iter).Merge(fromType);
        } else {
            scalarOpTypeInfos_.emplace(fromType);
        }
    }
    for (const auto &fromType : info->rwScalarOpTypeInfos_) {
        auto iter = rwScalarOpTypeInfos_.find(fromType);
        if (iter != rwScalarOpTypeInfos_.end()) {
            const_cast<RWScalarOpTypeInfo &>(*iter).Merge(fromType);
        } else {
            rwScalarOpTypeInfos_.emplace(fromType);
        }
    }
    for (const auto &fromType : info->objDefOpTypeInfos_) {
        AddDefine(fromType.GetOffset(), fromType.GetType(), fromType.GetSuperType());
    }
}

void PGOMethodTypeSet::SkipFromBinary(void **buffer)
{
    uint32_t size = base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
    for (uint32_t i = 0; i < size; i++) {
        base::ReadBufferInSize<ScalarOpTypeInfo>(buffer);
    }
}

bool PGOMethodTypeSet::ParseFromBinary(void **buffer, PGOProfilerHeader *const header)
{
    uint32_t size = base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
    for (uint32_t i = 0; i < size; i++) {
        auto typeInfo = base::ReadBufferInSize<TypeInfoHeader>(buffer);
        if (typeInfo->GetInfoType() == InfoType::OP_TYPE) {
            scalarOpTypeInfos_.emplace(*reinterpret_cast<ScalarOpTypeInfo *>(typeInfo));
        } else if (typeInfo->GetInfoType() == InfoType::DEFINE_CLASS_TYPE) {
            objDefOpTypeInfos_.emplace(*reinterpret_cast<ObjDefOpTypeInfo *>(typeInfo));
        } else if (header->SupportUseHClassType() && typeInfo->GetInfoType() == InfoType::USE_HCLASS_TYPE) {
            rwScalarOpTypeInfos_.emplace(*reinterpret_cast<RWScalarOpTypeInfo *>(typeInfo));
        }
    }
    return true;
}

bool PGOMethodTypeSet::ProcessToBinary(std::stringstream &stream) const
{
    uint32_t number = 0;
    std::stringstream methodStream;
    for (auto typeInfo : scalarOpTypeInfos_) {
        if (!typeInfo.GetType().IsNone()) {
            methodStream.write(reinterpret_cast<char *>(&typeInfo), typeInfo.Size());
            number++;
        }
    }
    for (auto typeInfo : rwScalarOpTypeInfos_) {
        if (typeInfo.GetCount() != 0) {
            methodStream.write(reinterpret_cast<char *>(&typeInfo), typeInfo.Size());
            number++;
        }
    }

    for (auto typeInfo : objDefOpTypeInfos_) {
        methodStream.write(reinterpret_cast<char *>(&typeInfo), typeInfo.Size());
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
    std::vector<std::string> typeInfoVector = StringHelper::SplitString(typeString, PGODumpUtils::TYPE_SEPARATOR);
    if (typeInfoVector.size() > 0) {
        for (const auto &iter : typeInfoVector) {
            std::vector<std::string> typeStrings = StringHelper::SplitString(iter, PGODumpUtils::BLOCK_START);
            if (typeStrings.size() < METHOD_TYPE_INFO_COUNT) {
                return false;
            }

            uint32_t offset;
            if (!StringHelper::StrToUInt32(typeStrings[METHOD_OFFSET_INDEX].c_str(), &offset)) {
                return false;
            }
            uint32_t type;
            if (!StringHelper::StrToUInt32(typeStrings[METHOD_TYPE_INDEX].c_str(), &type)) {
                return false;
            }
            scalarOpTypeInfos_.emplace(offset, PGOSampleType(type));
        }
    }
    return true;
}

void PGOMethodTypeSet::ProcessToText(std::string &text) const
{
    bool isFirst = true;
    for (auto typeInfoIter : scalarOpTypeInfos_) {
        if (typeInfoIter.GetType().IsNone()) {
            continue;
        }
        if (isFirst) {
            text += PGODumpUtils::ARRAY_START + PGODumpUtils::SPACE;
            isFirst = false;
        } else {
            text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
        }
        text += std::to_string(typeInfoIter.GetOffset());
        text += PGODumpUtils::BLOCK_START;
        text += typeInfoIter.GetType().GetTypeString();
    }
    for (auto rwScalarOpTypeInfoIter : rwScalarOpTypeInfos_) {
        if (rwScalarOpTypeInfoIter.GetCount() == 0) {
            continue;
        }
        if (isFirst) {
            text += PGODumpUtils::ARRAY_START + PGODumpUtils::SPACE;
            isFirst = false;
        } else {
            text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
        }
        rwScalarOpTypeInfoIter.ProcessToText(text);
    }
    for (const auto &defTypeInfoIter : objDefOpTypeInfos_) {
        if (isFirst) {
            text += PGODumpUtils::ARRAY_START + PGODumpUtils::SPACE;
            isFirst = false;
        } else {
            text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
        }
        defTypeInfoIter.ProcessToText(text);
    }
    if (!isFirst) {
        text += (PGODumpUtils::SPACE + PGODumpUtils::ARRAY_END);
    }
}

size_t PGOHClassLayoutDescInner::CaculateSize(const PGOHClassLayoutDesc &desc)
{
    if (desc.GetLayoutDesc().empty() && desc.GetPtLayoutDesc().empty() && desc.GetCtorLayoutDesc().empty()) {
        return sizeof(PGOHClassLayoutDescInner);
    }
    size_t size = sizeof(PGOHClassLayoutDescInner) - sizeof(PGOLayoutDescInfo);
    for (const auto &iter : desc.GetLayoutDesc()) {
        auto key = iter.first;
        if (key.size() > 0) {
            size += static_cast<size_t>(PGOLayoutDescInfo::Size(key.size()));
        }
    }
    for (const auto &iter : desc.GetPtLayoutDesc()) {
        auto key = iter.first;
        if (key.size() > 0) {
            size += static_cast<size_t>(PGOLayoutDescInfo::Size(key.size()));
        }
    }
    for (const auto &iter : desc.GetCtorLayoutDesc()) {
        auto key = iter.first;
        if (key.size() > 0) {
            size += static_cast<size_t>(PGOLayoutDescInfo::Size(key.size()));
        }
    }
    size += sizeof(ElementsKind);
    return size;
}

std::string PGOHClassLayoutDescInner::GetTypeString(const PGOHClassLayoutDesc &desc)
{
    std::string text;
    text += desc.GetClassType().GetTypeString();
    if (!desc.GetSuperClassType().IsNone()) {
        text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
        text += desc.GetSuperClassType().GetTypeString();
    }
    if (!Elements::IsNone(desc.GetElementsKind())) {
        text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
        text += Elements::GetString(desc.GetElementsKind());
    }
    text += PGODumpUtils::BLOCK_AND_ARRAY_START;
    bool isLayoutFirst = true;
    for (const auto &layoutDesc : desc.GetLayoutDesc()) {
        if (!isLayoutFirst) {
            text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
        } else {
            text += PGODumpUtils::ARRAY_START;
        }
        isLayoutFirst = false;
        text += layoutDesc.first;
        text += PGODumpUtils::BLOCK_START;
        text += std::to_string(layoutDesc.second.GetValue());
    }
    if (!isLayoutFirst) {
        text += PGODumpUtils::ARRAY_END;
    }
    bool isPtLayoutFirst = true;
    for (const auto &layoutDesc : desc.GetPtLayoutDesc()) {
        if (!isPtLayoutFirst) {
            text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
        } else {
            if (!isLayoutFirst) {
                text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
            }
            text += PGODumpUtils::ARRAY_START;
        }
        isPtLayoutFirst = false;
        text += layoutDesc.first;
        text += PGODumpUtils::BLOCK_START;
        text += std::to_string(layoutDesc.second.GetValue());
    }
    if (!isPtLayoutFirst) {
        text += PGODumpUtils::ARRAY_END;
    }
    bool isCtorLayoutFirst = true;
    for (const auto &layoutDesc : desc.GetCtorLayoutDesc()) {
        if (!isCtorLayoutFirst) {
            text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
        } else {
            if (!isLayoutFirst || !isPtLayoutFirst) {
                text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
            }
            text += PGODumpUtils::ARRAY_START;
        }
        isCtorLayoutFirst = false;
        text += layoutDesc.first;
        text += PGODumpUtils::BLOCK_START;
        text += std::to_string(layoutDesc.second.GetValue());
    }
    if (!isCtorLayoutFirst) {
        text += PGODumpUtils::ARRAY_END;
    }
    text += (PGODumpUtils::SPACE + PGODumpUtils::ARRAY_END);
    return text;
}

void PGOHClassLayoutDescInner::Merge(const PGOHClassLayoutDesc &desc)
{
    auto current = const_cast<PGOLayoutDescInfo *>(GetFirst());
    for (const auto &iter : desc.GetLayoutDesc()) {
        auto key = iter.first;
        auto type = iter.second;
        if (key.size() > 0) {
            new (current) PGOLayoutDescInfo(key, type);
            current = const_cast<PGOLayoutDescInfo *>(GetNext(current));
            count_++;
        }
    }
    for (const auto &iter : desc.GetPtLayoutDesc()) {
        auto key = iter.first;
        auto type = iter.second;
        if (key.size() > 0) {
            new (current) PGOLayoutDescInfo(key, type);
            current = const_cast<PGOLayoutDescInfo *>(GetNext(current));
            ptCount_++;
        }
    }
    for (const auto &iter : desc.GetCtorLayoutDesc()) {
        auto key = iter.first;
        auto type = iter.second;
        if (key.size() > 0) {
            new (current) PGOLayoutDescInfo(key, type);
            current = const_cast<PGOLayoutDescInfo *>(GetNext(current));
            ctorCount_++;
        }
    }
}

void PGOMethodTypeSet::RWScalarOpTypeInfo::ProcessToText(std::string &text) const
{
    text += std::to_string(GetOffset());
    text += PGODumpUtils::BLOCK_START;
    text += PGODumpUtils::ARRAY_START + PGODumpUtils::SPACE;
    bool isFirst = true;
    for (uint32_t i = 0; i < type_.GetCount(); i++) {
        if (!isFirst) {
            text += PGODumpUtils::TYPE_SEPARATOR + PGODumpUtils::SPACE;
        }
        isFirst = false;
        text += type_.GetObjectInfo(i).GetInfoString();
    }
    text += (PGODumpUtils::SPACE + PGODumpUtils::ARRAY_END);
}

void PGOMethodTypeSet::ObjDefOpTypeInfo::ProcessToText(std::string &text) const
{
    text += std::to_string(GetOffset());
    text += PGODumpUtils::BLOCK_START;
    text += PGODumpUtils::ARRAY_START + PGODumpUtils::SPACE;
    text += GetType().GetTypeString();
    text += PGODumpUtils::TYPE_SEPARATOR;
    text += GetSuperType().GetTypeString();
    text += (PGODumpUtils::SPACE + PGODumpUtils::ARRAY_END);
}

bool PGOMethodInfoMap::AddMethod(Chunk *chunk, Method *jsMethod, SampleMode mode, int32_t incCount)
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
        void *infoAddr = chunk->Allocate(size);
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

bool PGOMethodInfoMap::ParseFromBinary(Chunk *chunk, uint32_t threshold, void **buffer, PGOProfilerHeader *const header)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t j = 0; j < secInfo.number_; j++) {
        PGOMethodInfo *info = base::ReadBufferInSize<PGOMethodInfo>(buffer);
        if (info->IsFilter(threshold)) {
            if (header->SupportMethodChecksum()) {
                base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
            }
            if (header->SupportType()) {
                PGOMethodTypeSet::SkipFromBinary(buffer);
            }
            continue;
        }
        methodInfos_.emplace(info->GetMethodId(), info);
        LOG_ECMA(DEBUG) << "Method:" << info->GetMethodId() << PGODumpUtils::ELEMENT_SEPARATOR << info->GetCount()
                        << PGODumpUtils::ELEMENT_SEPARATOR << std::to_string(static_cast<int>(info->GetSampleMode()))
                        << PGODumpUtils::ELEMENT_SEPARATOR << info->GetMethodName();
        if (header->SupportMethodChecksum()) {
            auto checksum = base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
            methodsChecksum_.emplace(info->GetMethodId(), checksum);
        }
        if (header->SupportType()) {
            auto typeInfoSet = chunk->New<PGOMethodTypeSet>();
            typeInfoSet->ParseFromBinary(buffer, header);
            methodTypeInfos_.emplace(info->GetMethodId(), typeInfoSet);
        }
    }
    return !methodInfos_.empty();
}

bool PGOMethodInfoMap::ProcessToBinary(uint32_t threshold, PGORecordId recordId, const SaveTask *task,
    std::fstream &stream, PGOProfilerHeader *const header) const
{
    SectionInfo secInfo;
    std::stringstream methodStream;
    for (auto iter = methodInfos_.begin(); iter != methodInfos_.end(); iter++) {
        LOG_ECMA(DEBUG) << "Method:" << iter->first << PGODumpUtils::ELEMENT_SEPARATOR << iter->second->GetCount()
                        << PGODumpUtils::ELEMENT_SEPARATOR
                        << std::to_string(static_cast<int>(iter->second->GetSampleMode()))
                        << PGODumpUtils::ELEMENT_SEPARATOR << iter->second->GetMethodName();
        if (task && task->IsTerminate()) {
            LOG_ECMA(DEBUG) << "ProcessProfile: task is already terminate";
            return false;
        }
        auto curMethodInfo = iter->second;
        if (curMethodInfo->IsFilter(threshold)) {
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
        stream.write(reinterpret_cast<char *>(&recordId), sizeof(uint32_t));
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
            size_t start = typeInfos.find_first_of(PGODumpUtils::ARRAY_START);
            size_t end = typeInfos.find_last_of(PGODumpUtils::ARRAY_END);
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
            profilerString += PGODumpUtils::NEW_LINE;
            profilerString += recordName;
            profilerString += PGODumpUtils::BLOCK_AND_ARRAY_START;
            isFirst = false;
        } else {
            profilerString += PGODumpUtils::BLOCK_SEPARATOR + PGODumpUtils::SPACE;
        }
        methodInfo->ProcessToText(profilerString);
        profilerString += PGODumpUtils::ELEMENT_SEPARATOR;
        auto checksumIter = methodsChecksum_.find(methodInfo->GetMethodId());
        if (checksumIter != methodsChecksum_.end()) {
            std::stringstream parseStream;
            parseStream << std::internal << std::setfill('0') << std::showbase
                        << std::setw(PGODumpUtils::HEX_FORMAT_WIDTH_FOR_32BITS) << std::hex << checksumIter->second
                        << PGODumpUtils::ELEMENT_SEPARATOR;
            profilerString += parseStream.str();
        }
        auto iter = methodTypeInfos_.find(methodInfo->GetMethodId());
        if (iter != methodTypeInfos_.end()) {
            iter->second->ProcessToText(profilerString);
        }
    }
    if (!isFirst) {
        profilerString += (PGODumpUtils::SPACE + PGODumpUtils::ARRAY_END + PGODumpUtils::NEW_LINE);
        stream << profilerString;
    }
}

bool PGOMethodIdSet::ParseFromBinary(uint32_t threshold, void **buffer, PGOProfilerHeader *const header)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t j = 0; j < secInfo.number_; j++) {
        PGOMethodInfo *info = base::ReadBufferInSize<PGOMethodInfo>(buffer);
        if (info->IsFilter(threshold)) {
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
        LOG_ECMA(DEBUG) << "Method:" << info->GetMethodId() << PGODumpUtils::ELEMENT_SEPARATOR << info->GetCount()
                        << PGODumpUtils::ELEMENT_SEPARATOR << std::to_string(static_cast<int>(info->GetSampleMode()))
                        << PGODumpUtils::ELEMENT_SEPARATOR << info->GetMethodName();
        if (header->SupportType()) {
            methodInfo.GetPGOMethodTypeSet().ParseFromBinary(buffer, header);
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
    recordPool_ = std::make_unique<PGORecordPool>();
};

PGORecordDetailInfos::~PGORecordDetailInfos()
{
    Clear();
}

PGOMethodInfoMap *PGORecordDetailInfos::GetMethodInfoMap(const CString &recordName)
{
    auto recordId = recordPool_->TryAdd(recordName)->GetId();
    auto iter = recordInfos_.find(recordId);
    if (iter != recordInfos_.end()) {
        return iter->second;
    } else {
        auto curMethodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
        recordInfos_.emplace(recordId, curMethodInfos);
        return curMethodInfos;
    }
}

bool PGORecordDetailInfos::AddMethod(const CString &recordName, Method *jsMethod, SampleMode mode, int32_t incCount)
{
    auto curMethodInfos = GetMethodInfoMap(recordName);
    ASSERT(curMethodInfos != nullptr);
    ASSERT(jsMethod != nullptr);
    return curMethodInfos->AddMethod(chunk_.get(), jsMethod, mode, incCount);
}

bool PGORecordDetailInfos::AddType(const CString &recordName, PGOMethodId methodId, int32_t offset, PGOSampleType type)
{
    auto curMethodInfos = GetMethodInfoMap(recordName);
    ASSERT(curMethodInfos != nullptr);
    return curMethodInfos->AddType(chunk_.get(), methodId, offset, type);
}

bool PGORecordDetailInfos::AddCallTargetType(const CString &recordName, PGOMethodId methodId, int32_t offset,
                                             PGOSampleType type)
{
    auto curMethodInfos = GetMethodInfoMap(recordName);
    ASSERT(curMethodInfos != nullptr);
    return curMethodInfos->AddCallTargetType(chunk_.get(), methodId, offset, type);
}

bool PGORecordDetailInfos::AddObjectInfo(
    const CString &recordName, EntityId methodId, int32_t offset, const PGOObjectInfo &info)
{
    auto curMethodInfos = GetMethodInfoMap(recordName);
    ASSERT(curMethodInfos != nullptr);
    return curMethodInfos->AddObjectInfo(chunk_.get(), methodId, offset, info);
}

bool PGORecordDetailInfos::AddDefine(
    const CString &recordName, PGOMethodId methodId, int32_t offset, PGOSampleType type, PGOSampleType superType)
{
    auto curMethodInfos = GetMethodInfoMap(recordName);
    ASSERT(curMethodInfos != nullptr);
    curMethodInfos->AddDefine(chunk_.get(), methodId, offset, type, superType);

    PGOHClassLayoutDesc descInfo(type.GetClassType());
    descInfo.SetSuperClassType(superType.GetClassType());
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
    PGOHClassLayoutDesc descInfo(type.GetClassType());
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

void PGORecordDetailInfos::Merge(const PGORecordDetailInfos &recordInfos)
{
    std::map<PGORecordId, PGORecordId> idMapping;
    recordPool_->Merge(*recordInfos.recordPool_, idMapping);

    for (auto iter = recordInfos.recordInfos_.begin(); iter != recordInfos.recordInfos_.end(); iter++) {
        auto oldRecordId = iter->first;
        auto newRecordId = idMapping.at(oldRecordId);

        auto fromMethodInfos = iter->second;

        auto recordInfosIter = recordInfos_.find(newRecordId);
        PGOMethodInfoMap *toMethodInfos = nullptr;
        if (recordInfosIter == recordInfos_.end()) {
            toMethodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
            recordInfos_.emplace(newRecordId, toMethodInfos);
        } else {
            toMethodInfos = recordInfosIter->second;
        }

        toMethodInfos->Merge(chunk_.get(), fromMethodInfos);
    }
    // Merge global layout desc infos to global method info map
    for (auto info = recordInfos.moduleLayoutDescInfos_.begin(); info != recordInfos.moduleLayoutDescInfos_.end();
         info++) {
        auto result = moduleLayoutDescInfos_.find(*info);
        if (result == moduleLayoutDescInfos_.end()) {
            moduleLayoutDescInfos_.emplace(*info);
        } else {
            const_cast<PGOHClassLayoutDesc &>(*result).Merge(*info);
        }
    }
}

void PGORecordDetailInfos::ParseFromBinary(void *buffer, PGOProfilerHeader *const header)
{
    SectionInfo *info = header->GetRecordInfoSection();
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        PGORecordId recordId(0);
        if (header->SupportRecordPool()) {
            recordId = base::ReadBuffer<PGORecordId>(&addr, sizeof(PGORecordId));
        } else {
            auto *recordName = base::ReadBuffer(&addr);
            recordId = recordPool_->TryAdd(recordName)->GetId();
        }
        PGOMethodInfoMap *methodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
        if (methodInfos->ParseFromBinary(chunk_.get(), hotnessThreshold_, &addr, header)) {
            recordInfos_.emplace(recordId, methodInfos);
        }
    }

    info = header->GetLayoutDescSection();
    if (info == nullptr) {
        return;
    }
    if (header->SupportTrackField()) {
        ParseFromBinaryForLayout(&addr, header);
    }

    info = header->GetRecordPoolSection();
    if (info == nullptr) {
        return;
    }
    if (header->SupportRecordPool()) {
        recordPool_->ParseFromBinary(&addr, header);
    }
}

bool PGORecordDetailInfos::ParseFromBinaryForLayout(void **buffer, PGOProfilerHeader *const header)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t i = 0; i < secInfo.number_; i++) {
        PGOHClassLayoutDescInner *info = base::ReadBufferInSize<PGOHClassLayoutDescInner>(buffer);
        if (info == nullptr) {
            LOG_ECMA(INFO) << "Binary format error!";
            continue;
        }
        moduleLayoutDescInfos_.emplace(info->Convert(header));
    }
    return true;
}

void PGORecordDetailInfos::ProcessToBinary(
    const SaveTask *task, std::fstream &fileStream, PGOProfilerHeader *const header) const
{
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
        if (curMethodInfos->ProcessToBinary(hotnessThreshold_, recordId, task, fileStream, header)) {
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

    info = header->GetRecordPoolSection();
    if (info == nullptr) {
        return;
    }
    info->number_ = 0;
    info->offset_ = static_cast<uint32_t>(fileStream.tellp());
    info->number_ = recordPool_->ProcessToBinary(fileStream);
    info->size_ = static_cast<uint32_t>(fileStream.tellp()) - info->offset_;
    header->SetFileSize(static_cast<uint32_t>(fileStream.tellp()));
}

bool PGORecordDetailInfos::ProcessToBinaryForLayout(
    NativeAreaAllocator *allocator, const SaveTask *task, std::fstream &stream) const
{
    SectionInfo secInfo;
    auto layoutBeginPosition = stream.tellp();
    stream.seekp(sizeof(SectionInfo), std::ofstream::cur);
    for (const auto &typeInfo : moduleLayoutDescInfos_) {
        if (task && task->IsTerminate()) {
            LOG_ECMA(DEBUG) << "ProcessProfile: task is already terminate";
            return false;
        }
        auto classType = PGOSampleType(typeInfo.GetClassType());
        auto elementsKind = typeInfo.GetElementsKind();
        size_t size = PGOHClassLayoutDescInner::CaculateSize(typeInfo);
        if (size == 0) {
            continue;
        }
        auto superType = PGOSampleType(typeInfo.GetSuperClassType());
        void *addr = allocator->Allocate(size);
        auto descInfos = new (addr) PGOHClassLayoutDescInner(size, classType, superType, elementsKind);
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
        size_t blockIndex = details.find(PGODumpUtils::BLOCK_AND_ARRAY_START);
        if (blockIndex == std::string::npos) {
            return false;
        }
        CString recordName = ConvertToString(details.substr(0, blockIndex));

        size_t start = details.find_first_of(PGODumpUtils::ARRAY_START);
        size_t end = details.find_last_of(PGODumpUtils::ARRAY_END);
        if (start == std::string::npos || end == std::string::npos || start > end) {
            return false;
        }
        auto content = details.substr(start + 1, end - (start + 1) - 1);
        std::vector<std::string> infoStrings = StringHelper::SplitString(content, PGODumpUtils::BLOCK_SEPARATOR);
        if (infoStrings.size() <= 0) {
            continue;
        }

        PGORecordId recordId = recordPool_->TryAdd(recordName)->GetId();
        auto methodInfosIter = recordInfos_.find(recordId);
        PGOMethodInfoMap *methodInfos = nullptr;
        if (methodInfosIter == recordInfos_.end()) {
            methodInfos = nativeAreaAllocator_.New<PGOMethodInfoMap>();
            recordInfos_.emplace(recordId, methodInfos);
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
            profilerString += PGODumpUtils::NEW_LINE;
            profilerString += PGODumpUtils::ARRAY_START + PGODumpUtils::SPACE;
            isFirst = false;
        } else {
            profilerString += PGODumpUtils::BLOCK_SEPARATOR + PGODumpUtils::SPACE;
        }
        profilerString += PGOHClassLayoutDescInner::GetTypeString(layoutInfoIter);
    }
    if (!isFirst) {
        profilerString += (PGODumpUtils::SPACE + PGODumpUtils::ARRAY_END + PGODumpUtils::NEW_LINE);
        stream << profilerString;
    }
    for (auto iter = recordInfos_.begin(); iter != recordInfos_.end(); iter++) {
        auto recordId = iter->first;
        auto recordName = recordPool_->GetRecord(recordId)->GetRecordName();
        auto methodInfos = iter->second;
        methodInfos->ProcessToText(hotnessThreshold_, recordName, stream);
    }
    recordPool_->ProcessToText(stream);
}

void PGORecordDetailInfos::Clear()
{
    for (auto iter : recordInfos_) {
        iter.second->Clear();
        nativeAreaAllocator_.Delete(iter.second);
    }
    recordInfos_.clear();
    recordPool_->Clear();
    chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
    recordPool_ = std::make_unique<PGORecordPool>();
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
    if (header->SupportRecordPool()) {
        SectionInfo *info = header->GetRecordPoolSection();
        if (info == nullptr) {
            return;
        }
        void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
        recordPool_->ParseFromBinary(&addr, header);
    }
    SectionInfo *info = header->GetRecordInfoSection();
    void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
    for (uint32_t i = 0; i < info->number_; i++) {
        CString recordName;
        if (header->SupportRecordPool()) {
            auto recordId = base::ReadBuffer<PGORecordId>(&addr, sizeof(PGORecordId));
            recordName = recordPool_->GetRecord(recordId)->GetRecordName();
        } else {
            recordName = base::ReadBuffer(&addr);
        }
        PGOMethodIdSet *methodIds = nativeAreaAllocator_.New<PGOMethodIdSet>(chunk_.get());
        if (methodIds->ParseFromBinary(hotnessThreshold_, &addr, header)) {
            methodIds_.emplace(recordName, methodIds);
        }
    }

    info = header->GetLayoutDescSection();
    if (info == nullptr) {
        return;
    }
    if (header->SupportTrackField()) {
        ParseFromBinaryForLayout(&addr, header);
    }
}

void PGORecordSimpleInfos::Merge(const PGORecordSimpleInfos &simpleInfos)
{
    std::map<PGORecordId, PGORecordId> idMapping;
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

bool PGORecordSimpleInfos::ParseFromBinaryForLayout(void **buffer, PGOProfilerHeader *const header)
{
    SectionInfo secInfo = base::ReadBuffer<SectionInfo>(buffer);
    for (uint32_t i = 0; i < secInfo.number_; i++) {
        PGOHClassLayoutDescInner *info = base::ReadBufferInSize<PGOHClassLayoutDescInner>(buffer);
        if (info == nullptr) {
            LOG_ECMA(INFO) << "Binary format error!";
            continue;
        }
        moduleLayoutDescInfos_.emplace(info->Convert(header));
    }
    return true;
}
} // namespace panda::ecmascript
