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

#ifndef ECMASCRIPT_PGO_PROFILER_INFO_H
#define ECMASCRIPT_PGO_PROFILER_INFO_H

#include <cstdint>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <string.h>

#include "ecmascript/base/file_header.h"
#include "ecmascript/common.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/pgo_profiler/pgo_file_info.h"
#include "ecmascript/pgo_profiler/pgo_profiler_layout.h"
#include "ecmascript/pgo_profiler/pgo_record_pool.h"
#include "ecmascript/property_attributes.h"


namespace panda::ecmascript {
class SaveTask;

enum class SampleMode : uint8_t {
    HOTNESS_MODE,
    CALL_MODE,
};

/**
  |----PGOProfilerHeader
  |--------MAGIC(8)
  |------------{ 'P', 'A', 'N', 'D', 'A', '\0', '\0', '\0' }
  |--------VERSION(4)
  |------------{ '0', '0', '0', '0' }
  |--------CHECKSUM(4)
  |------------{ checksum }
  |--------FILE_SIZE(4)
  |------------{ fileSize }
  |--------HEADER_SIZE(4)
  |------------{ headerSize, from MAGIC to SECTION_NUMBER }
  |--------ENDIAN_TAG(4)
  |------------{ ENDIAN_TAG }
  |--------SECTION_NUMBER(4)
  |------------{ 4 }
  |--------PANDA_FILE_INFO_SECTION_INFO(12)
  |------------{ offset, size (reserve), number1 }
  |--------RECORD_INFO_SECTION_INFO(12)
  |------------{ offset, size (reserve), number2 }
  |--------LAYOUT_DESC_SECTION_INFO(12)
  |------------{ offset, size (reserve), number3 }
  |--------RECORD_POOL(12)
  |------------{ offset, size (reserve), recordPoolCount }
  |
  |----Section1: PGOPandaFileInfos(number1)
  |--------[{ size, CHECK_SUM }, { size, CHECK_SUM }, ...]
  |
  |----Section2: PGORecordDetailInfos(number2)
  |--------[ PGOMethodInfoMap(number4)
  |------------{ recordId, offset, size, number4 }
  |------------[ PGOMethodInfo(size1)
  |----------------{ size1, entityId, count, mode, methodName, [{ size, offset, type }, { size, offset, type }, ...]},
  |------------  PGOMethodInfo(size1)
  |----------------{ size1, entityId, count, mode, methodName, [{ size, offset, type }, { size, offset, type }, ...]},
  |--------------... ]
  |--------  PGOMethodInfoMap()
  |--------... ]
  |
  |----Section3: PGHClassLayoutDescs(number3)
  |--------{ offset, size, number5 }
  |--------[ PGOHClassLayoutDescInner(size)
  |------------{ size, type, superType, count, ptCount, ctorCount, [{ size, handle, key }, { size, heandle, key }, ...]}
  |--------  PGOHClassLayoutDescInner(size)
  |------------{ size, type, superType, count, ptCount, ctorCount, [{ size, handle, key }, { size, heandle, key }, ...]}
  |
  |----Section4: PGORecord(recordPoolCount)
  |--------{ offset, size, recordItemCount }
  |--------[ recordId, recordName ](recordItemCount)
 */
class PGOProfilerHeader : public base::FileHeaderElastic {
public:
    static constexpr VersionType TYPE_MINI_VERSION = {0, 0, 0, 2};
    static constexpr VersionType METHOD_CHECKSUM_MINI_VERSION = {0, 0, 0, 4};
    static constexpr VersionType USE_HCLASS_TYPE_MINI_VERSION = {0, 0, 0, 5};
    static constexpr VersionType FILE_CONSISTENCY_MINI_VERSION = {0, 0, 0, 6};
    static constexpr VersionType TRACK_FIELD_MINI_VERSION = {0, 0, 0, 7};
    static constexpr VersionType ELEMENTS_KIND_MINI_VERSION = {0, 0, 0, 8};
    static constexpr VersionType RECORD_POOL_MINI_VERSION = {0, 0, 0, 9};
    static constexpr VersionType FILE_SIZE_MINI_VERSION = FILE_CONSISTENCY_MINI_VERSION;
    static constexpr VersionType HEADER_SIZE_MINI_VERSION = FILE_CONSISTENCY_MINI_VERSION;
    static constexpr VersionType ELASTIC_HEADER_MINI_VERSION = FILE_CONSISTENCY_MINI_VERSION;
    static constexpr VersionType LAST_VERSION = {0, 0, 0, 9};
    static constexpr size_t SECTION_SIZE = 4;
    static constexpr size_t PANDA_FILE_SECTION_INDEX = 0;
    static constexpr size_t RECORD_INFO_SECTION_INDEX = 1;
    static constexpr size_t LAYOUT_DESC_SECTION_INDEX = 2;
    static constexpr size_t RECORD_POOL_SECTION_INDEX = 3;

    PGOProfilerHeader() : base::FileHeaderElastic(LAST_VERSION), sectionNumber_(SECTION_SIZE)
    {
        GetPandaInfoSection()->offset_ = Size();
        SetHeaderSize(Size());
    }

    static size_t LastSize()
    {
        return sizeof(PGOProfilerHeader) + (SECTION_SIZE - 1) * sizeof(SectionInfo);
    }

    static size_t Size(uint32_t sectionNumber)
    {
        return sizeof(PGOProfilerHeader) + (sectionNumber - 1) * sizeof(SectionInfo);
    }

    size_t Size() const
    {
        return Size(sectionNumber_);
    }

    bool Verify() const
    {
        return VerifyVersion("apPath file", LAST_VERSION, false);
    }

    bool Verify(void *buffer, size_t bufferSize) const
    {
        if (!Verify()) {
            return false;
        }
        if (!VerifyConsistency(buffer, bufferSize)) {
            return false;
        }
        if (!VerifyFileSize(bufferSize)) {
            return false;
        }
        return true;
    }

    bool VerifyFileSize(size_t bufferSize) const;

    bool VerifyConsistency(void *buffer, size_t bufferSize) const;

    static void Build(PGOProfilerHeader **header, size_t size)
    {
        *header = reinterpret_cast<PGOProfilerHeader *>(malloc(size));
        new (*header) PGOProfilerHeader();
    }

    static void Destroy(PGOProfilerHeader **header)
    {
        if (*header != nullptr) {
            free(*header);
            *header = nullptr;
        }
    }

    // Copy Header.
    static bool ParseFromBinary(void *buffer, size_t bufferSize, PGOProfilerHeader **header);
    void ProcessToBinary(std::fstream &fileStream) const;

    bool ParseFromText(std::ifstream &stream);
    bool ProcessToText(std::ofstream &stream) const;

    SectionInfo *GetPandaInfoSection() const
    {
        return GetSectionInfo(PANDA_FILE_SECTION_INDEX);
    }

    SectionInfo *GetRecordInfoSection() const
    {
        return GetSectionInfo(RECORD_INFO_SECTION_INDEX);
    }

    SectionInfo *GetLayoutDescSection() const
    {
        return GetSectionInfo(LAYOUT_DESC_SECTION_INDEX);
    }

    SectionInfo *GetRecordPoolSection() const
    {
        return GetSectionInfo(RECORD_POOL_SECTION_INDEX);
    }

    bool SupportType() const
    {
        return CompatibleVerify(TYPE_MINI_VERSION);
    }

    bool SupportMethodChecksum() const
    {
        return CompatibleVerify(METHOD_CHECKSUM_MINI_VERSION);
    }

    bool SupportUseHClassType() const
    {
        return CompatibleVerify(USE_HCLASS_TYPE_MINI_VERSION);
    }

    bool SupportFileConsistency() const
    {
        return CompatibleVerify(FILE_CONSISTENCY_MINI_VERSION);
    }

    bool SupportFileSize() const
    {
        return CompatibleVerify(FILE_SIZE_MINI_VERSION);
    }

    bool SupportHeaderSize() const
    {
        return CompatibleVerify(HEADER_SIZE_MINI_VERSION);
    }

    bool SupportTrackField() const
    {
        return CompatibleVerify(TRACK_FIELD_MINI_VERSION);
    }

    bool SupportElementsKind() const
    {
        return CompatibleVerify(ELEMENTS_KIND_MINI_VERSION);
    }

    bool SupportRecordPool() const
    {
        return CompatibleVerify(RECORD_POOL_MINI_VERSION);
    }

    NO_COPY_SEMANTIC(PGOProfilerHeader);
    NO_MOVE_SEMANTIC(PGOProfilerHeader);

private:
    static bool BuildFromLegacy(void *buffer, PGOProfilerHeader **header);
    static bool BuildFromElastic(void *buffer, size_t bufferSize, PGOProfilerHeader **header);

    SectionInfo *GetSectionInfo(size_t index) const
    {
        if (index >= sectionNumber_) {
            return nullptr;
        }
        return const_cast<SectionInfo *>(&sectionInfos_) + index;
    }

    uint32_t sectionNumber_ {SECTION_SIZE};
    SectionInfo sectionInfos_;
};

class PGOProfilerHeaderLegacy : public base::FileHeaderBase {
public:
    static constexpr size_t SECTION_SIZE = 3;
    static constexpr VersionType LAST_VERSION = {0, 0, 0, 5};
    PGOProfilerHeaderLegacy() : base::FileHeaderBase(LAST_VERSION), sectionNumber_(SECTION_SIZE) {};

    const uint32_t& GetSectionNumber () const
    {
        return sectionNumber_;
    }

private:
    uint32_t sectionNumber_ {SECTION_SIZE};
    SectionInfo sectionInfos_;
};

class PGOPandaFileInfos {
public:
    void Sample(uint32_t checksum)
    {
        fileInfos_.emplace(checksum);
    }

    void Clear()
    {
        fileInfos_.clear();
    }

    void ParseFromBinary(void *buffer, SectionInfo *const info);
    void ProcessToBinary(std::fstream &fileStream, SectionInfo *info) const;
    void Merge(const PGOPandaFileInfos &pandaFileInfos);
    bool VerifyChecksum(const PGOPandaFileInfos &pandaFileInfos, const std::string &base,
                        const std::string &incoming) const;

    void ProcessToText(std::ofstream &stream) const;
    bool ParseFromText(std::ifstream &stream);

    bool Checksum(uint32_t checksum) const;

private:
    class FileInfo {
    public:
        FileInfo() = default;
        FileInfo(uint32_t checksum) : size_(LastSize()), checksum_(checksum) {}

        static size_t LastSize()
        {
            return sizeof(FileInfo);
        }

        size_t Size() const
        {
            return static_cast<size_t>(size_);
        }

        bool operator<(const FileInfo &right) const
        {
            return checksum_ < right.checksum_;
        }

        uint32_t GetChecksum() const
        {
            return checksum_;
        }

    private:
        // Support extended fields
        uint32_t size_;
        uint32_t checksum_;
    };

    std::set<FileInfo> fileInfos_;
};

class PGOMethodInfo {
public:
    static constexpr int METHOD_INFO_COUNT = 4;
    static constexpr int METHOD_ID_INDEX = 0;
    static constexpr int METHOD_COUNT_INDEX = 1;
    static constexpr int METHOD_MODE_INDEX = 2;
    static constexpr int METHOD_NAME_INDEX = 3;
    static constexpr uint32_t METHOD_MAX_HIT_COUNT = 10000U;

    explicit PGOMethodInfo(PGOMethodId id) : id_(id) {}

    PGOMethodInfo(PGOMethodId id, uint32_t count, SampleMode mode, const char *methodName)
        : id_(id), count_(count), mode_(mode)
    {
        size_t len = strlen(methodName);
        size_ = static_cast<uint32_t>(Size(len));
        if (len > 0 && memcpy_s(&methodName_, len, methodName, len) != EOK) {
            LOG_ECMA(ERROR) << "SetMethodName memcpy_s failed" << methodName << ", len = " << len;
            UNREACHABLE();
        }
        *(&methodName_ + len) = '\0';
    }

    static uint32_t CalcChecksum(const char *name, const uint8_t *byteCodeArray, uint32_t byteCodeLength);

    static uint32_t CalcOpCodeChecksum(const uint8_t *byteCodeArray, uint32_t byteCodeLength);

    static int32_t Size(uint32_t length)
    {
        return sizeof(PGOMethodInfo) + AlignUp(length, GetAlignmentInBytes(PGO_ALIGN_SIZE));
    }

    int32_t Size() const
    {
        return size_;
    }

    static bool GetSampleMode(std::string content, SampleMode &mode)
    {
        if (content == "HOTNESS_MODE") {
            mode = SampleMode::HOTNESS_MODE;
        } else if (content == "CALL_MODE") {
            mode = SampleMode::CALL_MODE;
        } else {
            return false;
        }
        return true;
    }

    void IncreaseCount(int32_t inc)
    {
        count_ += static_cast<uint32_t>(inc);
    }

    void ClearCount()
    {
        count_ = 0;
    }

    void Merge(const PGOMethodInfo *info)
    {
        if (!(id_ == info->GetMethodId())) {
            LOG_ECMA(ERROR) << "The method id must same for merging";
            return;
        }
        count_ = std::min(count_ + info->GetCount(), METHOD_MAX_HIT_COUNT);
        SetSampleMode(info->GetSampleMode());
    }

    PGOMethodId GetMethodId() const
    {
        return id_;
    }

    uint32_t GetCount() const
    {
        return count_;
    }

    const char *GetMethodName() const
    {
        return &methodName_;
    }

    void SetSampleMode(SampleMode mode)
    {
        if (mode_ == SampleMode::HOTNESS_MODE) {
            return;
        }
        mode_ = mode;
    }

    SampleMode GetSampleMode() const
    {
        return mode_;
    }

    std::string GetSampleModeToString() const
    {
        std::string result;
        switch (mode_) {
            case SampleMode::HOTNESS_MODE:
                result = "HOTNESS_MODE";
                break;
            case SampleMode::CALL_MODE:
                result = "CALL_MODE";
                break;
            default:
                LOG_ECMA(ERROR) << "mode error";
        }
        return result;
    }

    bool IsFilter(uint32_t threshold) const
    {
        if (count_ < threshold && mode_ == SampleMode::CALL_MODE) {
            return true;
        }
        return false;
    }

    void ParseFromBinary(void **buffer);
    void ProcessToBinary(std::ofstream &fileStream) const;

    static std::vector<std::string> ParseFromText(const std::string &infoString);
    void ProcessToText(std::string &text) const;

    NO_COPY_SEMANTIC(PGOMethodInfo);
    NO_MOVE_SEMANTIC(PGOMethodInfo);

private:
    uint32_t size_ {0};
    PGOMethodId id_;
    uint32_t count_ {0};
    SampleMode mode_ {SampleMode::CALL_MODE};
    char methodName_ {0};
};

class PGOMethodTypeSet {
public:
    static constexpr int METHOD_TYPE_INFO_INDEX = 4;
    static constexpr int METHOD_TYPE_INFO_COUNT = 2;
    static constexpr int METHOD_OFFSET_INDEX = 0;
    static constexpr int METHOD_TYPE_INDEX = 1;

    PGOMethodTypeSet() = default;

    void AddType(uint32_t offset, PGOSampleType type)
    {
        auto result = scalarOpTypeInfos_.find(ScalarOpTypeInfo(offset, type));
        if (result != scalarOpTypeInfos_.end()) {
            auto combineType = result->GetType().CombineType(type);
            const_cast<ScalarOpTypeInfo &>(*result).SetType(combineType);
        } else {
            scalarOpTypeInfos_.emplace(offset, type);
        }
    }

    void AddCallTargetType(uint32_t offset, PGOSampleType type)
    {
        auto result = scalarOpTypeInfos_.find(ScalarOpTypeInfo(offset, type));
        if (result != scalarOpTypeInfos_.end()) {
            auto combineType = result->GetType().CombineCallTargetType(type);
            const_cast<ScalarOpTypeInfo &>(*result).SetType(combineType);
        } else {
            scalarOpTypeInfos_.emplace(offset, type);
        }
    }

    void AddObjectInfo(uint32_t offset, const PGOObjectInfo &info)
    {
        auto result = rwScalarOpTypeInfos_.find(RWScalarOpTypeInfo(offset));
        if (result != rwScalarOpTypeInfos_.end()) {
            const_cast<RWScalarOpTypeInfo &>(*result).AddObjectInfo(info);
        } else {
            rwScalarOpTypeInfos_.emplace(offset, info);
        }
    }

    void AddDefine(uint32_t offset, PGOSampleType type, PGOSampleType superType)
    {
        auto result = objDefOpTypeInfos_.find(ObjDefOpTypeInfo(offset, type, superType));
        if (result != objDefOpTypeInfos_.end()) {
            return;
        }
        objDefOpTypeInfos_.emplace(offset, type, superType);
    }

    template <typename Callback>
    void GetTypeInfo(Callback callback)
    {
        for (const auto &typeInfo : scalarOpTypeInfos_) {
            auto type = typeInfo.GetType();
            callback(typeInfo.GetOffset(), &type);
        }
        for (const auto &typeInfo : rwScalarOpTypeInfos_) {
            auto type = typeInfo.GetType();
            callback(typeInfo.GetOffset(), &type);
        }
        for (const auto &typeInfo : objDefOpTypeInfos_) {
            auto classType = typeInfo.GetType();
            callback(typeInfo.GetOffset(), &classType);
        }
    }

    void Merge(const PGOMethodTypeSet *info);
    static void SkipFromBinary(void **buffer);

    bool ParseFromBinary(void **buffer, PGOProfilerHeader *const header);
    bool ProcessToBinary(std::stringstream &stream) const;

    bool ParseFromText(const std::string &typeString);
    void ProcessToText(std::string &text) const;

    NO_COPY_SEMANTIC(PGOMethodTypeSet);
    NO_MOVE_SEMANTIC(PGOMethodTypeSet);

private:
    enum class InfoType : uint8_t { NONE, OP_TYPE, DEFINE_CLASS_TYPE = 3, USE_HCLASS_TYPE };

    class TypeInfoHeader {
    public:
        TypeInfoHeader(InfoType type, uint32_t offset) : infoType_(type), offset_(offset) {}
        TypeInfoHeader(uint32_t size, InfoType type, uint32_t offset)
            : size_(size), infoType_(type), offset_(offset) {}

        InfoType GetInfoType()
        {
            return infoType_;
        }

        int32_t Size() const
        {
            return size_;
        }

        uint32_t GetOffset() const
        {
            return offset_;
        }

    protected:
        uint32_t size_ {0};
        InfoType infoType_ {InfoType::NONE};
        uint32_t offset_ {0};
    };

    class RWScalarOpTypeInfo : public TypeInfoHeader {
    public:
        explicit RWScalarOpTypeInfo(uint32_t offset)
            : TypeInfoHeader(InfoType::USE_HCLASS_TYPE, offset) {};
        RWScalarOpTypeInfo(uint32_t offset, PGOObjectInfo info)
            : TypeInfoHeader(sizeof(RWScalarOpTypeInfo), InfoType::USE_HCLASS_TYPE, offset)
        {
            type_.AddObjectInfo(info);
        }

        bool operator<(const RWScalarOpTypeInfo &right) const
        {
            return offset_ < right.offset_;
        }

        int32_t GetCount()
        {
            return type_.GetCount();
        }

        void Merge(const RWScalarOpTypeInfo &type)
        {
            type_.Merge(type.type_);
        }

        void AddObjectInfo(const PGOObjectInfo &info)
        {
            type_.AddObjectInfo(info);
        }

        PGORWOpType GetType() const
        {
            return type_;
        }

        void ProcessToText(std::string &text) const;

    private:
        PGORWOpType type_;
    };

    class ScalarOpTypeInfo : public TypeInfoHeader {
    public:
        ScalarOpTypeInfo(uint32_t offset, PGOSampleType type)
            : TypeInfoHeader(sizeof(ScalarOpTypeInfo), InfoType::OP_TYPE, offset), type_(type) {}

        bool operator<(const ScalarOpTypeInfo &right) const
        {
            return offset_ < right.offset_;
        }

        void SetType(PGOSampleType type)
        {
            if (type_ != type) {
                type_ = type;
            }
        }

        void Merge(const ScalarOpTypeInfo &typeInfo)
        {
            PGOSampleType combineType = GetType().CombineType(typeInfo.GetType());
            SetType(combineType);
        }

        PGOSampleType GetType() const
        {
            return type_;
        }

    protected:
        ScalarOpTypeInfo(uint32_t size, InfoType infoType, uint32_t offset, PGOSampleType type)
            : TypeInfoHeader(size, infoType, offset), type_(type) {}

    private:
        PGOSampleType type_;
    };

    class ObjDefOpTypeInfo : public ScalarOpTypeInfo {
    public:
        ObjDefOpTypeInfo(uint32_t offset, PGOSampleType type, PGOSampleType superType)
            : ScalarOpTypeInfo(sizeof(ObjDefOpTypeInfo), InfoType::DEFINE_CLASS_TYPE, offset, type),
            superType_(superType) {}

        PGOSampleType GetSuperType() const
        {
            return superType_;
        }

        bool operator<(const ObjDefOpTypeInfo &right) const
        {
            return offset_ < right.GetOffset() || GetType() < right.GetType() || superType_ < right.superType_;
        }

        void ProcessToText(std::string &text) const;

    protected:
        ObjDefOpTypeInfo(
            uint32_t size, InfoType infoType, uint32_t offset, PGOSampleType type, PGOSampleType superType)
            : ScalarOpTypeInfo(size, infoType, offset, type), superType_(superType) {}

    private:
        PGOSampleType superType_;
    };

    std::set<ScalarOpTypeInfo> scalarOpTypeInfos_;
    std::set<RWScalarOpTypeInfo> rwScalarOpTypeInfos_;
    std::set<ObjDefOpTypeInfo> objDefOpTypeInfos_;
};

class PGODecodeMethodInfo {
public:
    explicit PGODecodeMethodInfo(PGOMethodId id) : methodId_(id) {}

    PGOMethodId GetMethodId() const
    {
        return methodId_;
    }

    PGOMethodTypeSet &GetPGOMethodTypeSet()
    {
        return pgoMethodTypeSet_;
    }

    void Merge(const PGODecodeMethodInfo &from);

private:
    PGOMethodId methodId_ {0};
    PGOMethodTypeSet pgoMethodTypeSet_ {};
};

class PGOHClassLayoutDescInner {
public:
    PGOHClassLayoutDescInner(size_t size, PGOSampleType type, PGOSampleType superType, ElementsKind kind)
        : size_(size), type_(type), superType_(superType)
    {
        SetElementsKind(kind);
    }

    static size_t CaculateSize(const PGOHClassLayoutDesc &desc);
    static std::string GetTypeString(const PGOHClassLayoutDesc &desc);

    void Merge(const PGOHClassLayoutDesc &desc);

    int32_t Size() const
    {
        return size_;
    }

    PGOSampleType GetType() const
    {
        return type_;
    }

    PGOHClassLayoutDesc Convert(PGOProfilerHeader *const header)
    {
        PGOHClassLayoutDesc desc(GetType().GetClassType());
        desc.SetSuperClassType(superType_.GetClassType());
        auto descInfo = GetFirst();
        for (int32_t i = 0; i < count_; i++) {
            desc.AddKeyAndDesc(descInfo->GetKey(), descInfo->GetHandler());
            descInfo = GetNext(descInfo);
        }
        for (int32_t i = 0; i < ptCount_; i++) {
            desc.AddPtKeyAndDesc(descInfo->GetKey(), descInfo->GetHandler());
            descInfo = GetNext(descInfo);
        }
        for (int32_t i = 0; i < ctorCount_; i++) {
            desc.AddCtorKeyAndDesc(descInfo->GetKey(), descInfo->GetHandler());
            descInfo = GetNext(descInfo);
        }
        if (header->SupportElementsKind()) {
            desc.SetElementsKind(GetElementsKind());
        }
        return desc;
    }

    class PGOLayoutDescInfo {
    public:
        PGOLayoutDescInfo() = default;
        PGOLayoutDescInfo(const CString &key, PGOHandler handler) : handler_(handler)
        {
            size_t len = key.size();
            size_ = Size(len);
            if (len > 0 && memcpy_s(&key_, len, key.c_str(), len) != EOK) {
                LOG_ECMA(ERROR) << "SetMethodName memcpy_s failed" << key << ", len = " << len;
                UNREACHABLE();
            }
            *(&key_ + len) = '\0';
        }

        static int32_t Size(size_t len)
        {
            return sizeof(PGOLayoutDescInfo) + AlignUp(len, GetAlignmentInBytes(PGO_ALIGN_SIZE));
        }

        int32_t Size() const
        {
            return size_;
        }

        const char *GetKey() const
        {
            return &key_;
        }

        PGOHandler GetHandler() const
        {
            return handler_;
        }

    private:
        int32_t size_ {0};
        PGOHandler handler_;
        char key_ {'\0'};
    };

private:
    const PGOLayoutDescInfo *GetFirst() const
    {
        return &descInfos_;
    }

    const PGOLayoutDescInfo *GetNext(const PGOLayoutDescInfo *current) const
    {
        return reinterpret_cast<PGOLayoutDescInfo *>(reinterpret_cast<uintptr_t>(current) + current->Size());
    }

    void SetElementsKind(ElementsKind kind)
    {
        *reinterpret_cast<ElementsKind *>(GetEnd() - sizeof(ElementsKind)) = kind;
    }

    ElementsKind GetElementsKind() const
    {
        return *reinterpret_cast<const ElementsKind *>(GetEnd() - sizeof(ElementsKind));
    }

    uintptr_t GetEnd() const
    {
        return reinterpret_cast<uintptr_t>(this) + Size();
    }

    int32_t size_;
    PGOSampleType type_;
    PGOSampleType superType_;
    int32_t count_ {0};
    int32_t ptCount_ {0};
    int32_t ctorCount_ {0};
    PGOLayoutDescInfo descInfos_;
};

class PGOMethodInfoMap {
public:
    PGOMethodInfoMap() = default;

    void Clear()
    {
        // PGOMethodInfo release by chunk
        methodInfos_.clear();
        methodTypeInfos_.clear();
    }

    bool AddMethod(Chunk *chunk, Method *jsMethod, SampleMode mode, int32_t incCount);
    bool AddType(Chunk *chunk, PGOMethodId methodId, int32_t offset, PGOSampleType type);
    bool AddCallTargetType(Chunk *chunk, PGOMethodId methodId, int32_t offset, PGOSampleType type);
    bool AddObjectInfo(Chunk *chunk, PGOMethodId methodId, int32_t offset, const PGOObjectInfo &info);
    bool AddDefine(Chunk *chunk, PGOMethodId methodId, int32_t offset, PGOSampleType type, PGOSampleType superType);
    void Merge(Chunk *chunk, PGOMethodInfoMap *methodInfos);

    bool ParseFromBinary(Chunk *chunk, uint32_t threshold, void **buffer, PGOProfilerHeader *const header);
    bool ProcessToBinary(uint32_t threshold, PGORecordId recordId, const SaveTask *task, std::fstream &fileStream,
        PGOProfilerHeader *const header) const;

    bool ParseFromText(Chunk *chunk, uint32_t threshold, const std::vector<std::string> &content);
    void ProcessToText(uint32_t threshold, const CString &recordName, std::ofstream &stream) const;

    const CMap<PGOMethodId, PGOMethodInfo *> &GetMethodInfos() const
    {
        return methodInfos_;
    }

    NO_COPY_SEMANTIC(PGOMethodInfoMap);
    NO_MOVE_SEMANTIC(PGOMethodInfoMap);

private:
    PGOMethodTypeSet *GetOrInsertMethodTypeSet(Chunk *chunk, PGOMethodId methodId);

    CMap<PGOMethodId, PGOMethodInfo *> methodInfos_;
    CMap<PGOMethodId, PGOMethodTypeSet *> methodTypeInfos_;
    CMap<PGOMethodId, uint32_t> methodsChecksum_;
    CMap<PGOSampleType, CMap<CString, TrackType>> globalLayoutDescInfos_;
};

class PGOMethodIdSet {
public:
    explicit PGOMethodIdSet(Chunk* chunk): chunk_(chunk), methodInfoMap_(chunk) {};
    ~PGOMethodIdSet() = default;

    void Clear()
    {
        candidateSet_.clear();
        for (auto &methodNameSet : methodInfoMap_) {
            methodNameSet.second.Clear();
        }
        methodInfoMap_.clear();
    }

    bool Match(EntityId methodId)
    {
        return candidateSet_.find(methodId) != candidateSet_.end();
    }

    template <typename Callback>
    bool Update(const CString &recordName, Callback callback)
    {
        std::unordered_set<EntityId> newIds = callback(recordName, candidateSet_);
        if (!newIds.empty()) {
            candidateSet_.insert(newIds.begin(), newIds.end());
            return true;
        }
        return false;
    }

    template <typename Callback>
    void GetTypeInfo(const char *methodName, Callback callback)
    {
        // for no function checksum in ap file
        auto iter = methodInfoMap_.find(methodName);
        if ((iter != methodInfoMap_.end()) && (iter->second.GetFirstMethodInfo() != nullptr)) {
            iter->second.GetFirstMethodInfo()->GetPGOMethodTypeSet().GetTypeInfo(callback);
        }
    }

    template <typename Callback>
    void GetTypeInfo(const char *methodName, uint32_t checksum, Callback callback)
    {
        auto iter = methodInfoMap_.find(methodName);
        if ((iter != methodInfoMap_.end()) && (iter->second.GetMethodInfo(checksum) != nullptr)) {
            return iter->second.GetMethodInfo(checksum)->GetPGOMethodTypeSet().GetTypeInfo(callback);
        }
        LOG_ECMA(DEBUG) << "Method checksum mismatched, name: " << methodName;
    }

    void MatchAndMarkMethod(const char *methodName, EntityId methodId)
    {
        const auto &iter = methodInfoMap_.find(methodName);
        if (iter == methodInfoMap_.end()) {
            // no matching method in PGO file.
            return;
        }
        candidateSet_.emplace(methodId);
        iter->second.SetMatch();
    }

    bool ParseFromBinary(uint32_t threshold, void **buffer, PGOProfilerHeader *const header);

    void GetMismatchResult(const CString &recordName, uint32_t &totalMethodCount, uint32_t &mismatchMethodCount,
                           std::set<std::pair<std::string, CString>> &mismatchMethodSet) const;

    void Merge(const PGOMethodIdSet &from);

    class PGOMethodNameSet {
    public:
        explicit PGOMethodNameSet(Chunk* chunk): methodMap_(chunk) {};
        void SetMatch()
        {
            methodNameMatch_ = true;
        }

        bool IsMatch() const
        {
            return methodNameMatch_;
        }

        PGODecodeMethodInfo& GetOrCreateMethodInfo(uint32_t checksum, PGOMethodId methodId)
        {
            auto methodIter = methodMap_.find(checksum);
            if (methodIter == methodMap_.end()) {
                auto ret = methodMap_.emplace(checksum, methodId);
                ASSERT(ret.second);
                methodIter = ret.first;
            }
            return methodIter->second;
        }

        void Merge(const PGOMethodNameSet &from)
        {
            for (const auto &method : from.methodMap_) {
                uint32_t checksum = method.first;
                auto methodInfo = methodMap_.find(checksum);
                if (methodInfo == methodMap_.end()) {
                    auto ret = methodMap_.emplace(checksum, method.second.GetMethodId());
                    ASSERT(ret.second);
                    methodInfo = ret.first;
                }
                methodInfo->second.Merge(method.second);
            }
        }

        PGODecodeMethodInfo *GetFirstMethodInfo()
        {
            if (methodMap_.empty()) {
                return nullptr;
            }
            return &(methodMap_.begin()->second);
        }

        PGODecodeMethodInfo *GetMethodInfo(uint32_t checksum)
        {
            auto methodInfo = methodMap_.find(checksum);
            if (methodInfo == methodMap_.end()) {
                return nullptr;
            }
            return &(methodInfo->second);
        }

        void Clear()
        {
            methodMap_.clear();
        }

    private:
        bool methodNameMatch_ {false};
        ChunkUnorderedMap<uint32_t, PGODecodeMethodInfo> methodMap_;
    };

    NO_COPY_SEMANTIC(PGOMethodIdSet);
    NO_MOVE_SEMANTIC(PGOMethodIdSet);

private:
    Chunk* chunk_;
    std::unordered_set<EntityId> candidateSet_; // methodId in abc file, DO NOT for pgo internal use
    ChunkUnorderedMap<CString, PGOMethodNameSet> methodInfoMap_;
};

class PGORecordDetailInfos {
public:
    explicit PGORecordDetailInfos(uint32_t hotnessThreshold);

    ~PGORecordDetailInfos();

    void Clear();

    // If it is a new method, return true.
    bool AddMethod(const CString &recordName, Method *jsMethod, SampleMode mode, int32_t incCount);
    bool AddType(const CString &recordName, PGOMethodId methodId, int32_t offset, PGOSampleType type);
    bool AddCallTargetType(const CString &recordName, PGOMethodId methodId, int32_t offset, PGOSampleType type);
    bool AddObjectInfo(const CString &recordName, PGOMethodId methodId, int32_t offset, const PGOObjectInfo &info);
    bool AddDefine(
        const CString &recordName, PGOMethodId methodId, int32_t offset, PGOSampleType type, PGOSampleType superType);
    bool AddLayout(PGOSampleType type, JSTaggedType hclass, PGOObjKind kind);
    void Merge(const PGORecordDetailInfos &recordInfos);

    void ParseFromBinary(void *buffer, PGOProfilerHeader *const header);
    void ProcessToBinary(const SaveTask *task, std::fstream &fileStream, PGOProfilerHeader *const header) const;

    bool ParseFromText(std::ifstream &stream);
    void ProcessToText(std::ofstream &stream) const;

    const CMap<PGORecordId, PGOMethodInfoMap *> &GetRecordInfos() const
    {
        return recordInfos_;
    }

    NO_COPY_SEMANTIC(PGORecordDetailInfos);
    NO_MOVE_SEMANTIC(PGORecordDetailInfos);

private:
    PGOMethodInfoMap *GetMethodInfoMap(const CString &recordName);
    bool ParseFromBinaryForLayout(void **buffer, PGOProfilerHeader *const header);
    bool ProcessToBinaryForLayout(NativeAreaAllocator *allocator, const SaveTask *task, std::fstream &stream) const;

    uint32_t hotnessThreshold_ {2};
    NativeAreaAllocator nativeAreaAllocator_;
    std::unique_ptr<Chunk> chunk_;
    CMap<PGORecordId, PGOMethodInfoMap *> recordInfos_;
    std::set<PGOHClassLayoutDesc> moduleLayoutDescInfos_;
    std::unique_ptr<PGORecordPool> recordPool_;
};

class PGORecordSimpleInfos {
public:
    explicit PGORecordSimpleInfos(uint32_t threshold) : hotnessThreshold_(threshold)
    {
        chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
        recordPool_ = std::make_unique<PGORecordPool>();
    }

    ~PGORecordSimpleInfos()
    {
        Clear();
    }

    void Clear()
    {
        for (const auto &iter : methodIds_) {
            iter.second->Clear();
            nativeAreaAllocator_.Delete(iter.second);
        }
        methodIds_.clear();
        recordPool_->Clear();
        chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
        recordPool_ = std::make_unique<PGORecordPool>();
    }

    bool Match(const CString &recordName, EntityId methodId);

    template <typename Callback>
    void Update(Callback callback)
    {
        for (auto iter = methodIds_.begin(); iter != methodIds_.end(); iter++) {
            auto recordName = iter->first;
            auto methodIds = iter->second;
            methodIds->Update(recordName, callback);
        }
    }

    template <typename Callback>
    void Update(const CString &recordName, Callback callback)
    {
        auto iter = methodIds_.find(recordName);
        if (iter != methodIds_.end()) {
            iter->second->Update(recordName, callback);
        } else {
            PGOMethodIdSet *methodIds = nativeAreaAllocator_.New<PGOMethodIdSet>(chunk_.get());
            if (methodIds->Update(recordName, callback)) {
                methodIds_.emplace(recordName, methodIds);
            } else {
                nativeAreaAllocator_.Delete(methodIds);
            }
        }
    }

    template <typename Callback>
    void GetTypeInfo(const CString &recordName, const char *methodName, Callback callback)
    {
        auto iter = methodIds_.find(recordName);
        if (iter != methodIds_.end()) {
            iter->second->GetTypeInfo(methodName, callback);
        }
    }

    template <typename Callback>
    void GetTypeInfo(const CString &recordName, const char *methodName, uint32_t checksum, Callback callback)
    {
        auto iter = methodIds_.find(recordName);
        if (iter != methodIds_.end()) {
            iter->second->GetTypeInfo(methodName, checksum, callback);
        }
    }

    bool GetHClassLayoutDesc(PGOSampleType classType, PGOHClassLayoutDesc **desc) const
    {
        auto iter = moduleLayoutDescInfos_.find(PGOHClassLayoutDesc(classType.GetClassType()));
        if (iter != moduleLayoutDescInfos_.end()) {
            *desc = &(const_cast<PGOHClassLayoutDesc &>(*iter));
            return true;
        }
        return false;
    }

    void MatchAndMarkMethod(const CString &recordName, const char *methodName, EntityId methodId)
    {
        auto iter = methodIds_.find(recordName);
        if (iter != methodIds_.end()) {
            return iter->second->MatchAndMarkMethod(methodName, methodId);
        }
    }

    void GetMismatchResult(uint32_t &totalMethodCount, uint32_t &mismatchMethodCount,
                           std::set<std::pair<std::string, CString>> &mismatchMethodSet) const
    {
        for (const auto &methodId : methodIds_) {
            methodId.second->GetMismatchResult(methodId.first, totalMethodCount, mismatchMethodCount,
                                               mismatchMethodSet);
        }
    }

    void ParseFromBinary(void *buffer, PGOProfilerHeader *const header);

    void Merge(const PGORecordSimpleInfos &simpleInfos);

    NO_COPY_SEMANTIC(PGORecordSimpleInfos);
    NO_MOVE_SEMANTIC(PGORecordSimpleInfos);

private:
    bool ParseFromBinaryForLayout(void **buffer, PGOProfilerHeader *const header);

    uint32_t hotnessThreshold_ {2};
    NativeAreaAllocator nativeAreaAllocator_;
    std::unique_ptr<Chunk> chunk_;
    CUnorderedMap<CString, PGOMethodIdSet *> methodIds_;
    std::unique_ptr<PGORecordPool> recordPool_;
    std::set<PGOHClassLayoutDesc> moduleLayoutDescInfos_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_PGO_PROFILER_INFO_H
