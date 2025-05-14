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

#ifndef ECMASCRIPT_PGO_PROFILER_AP_FILE_BASE_INFO_H
#define ECMASCRIPT_PGO_PROFILER_AP_FILE_BASE_INFO_H

#include <cstdint>

#include "ecmascript/common.h"
#include "libpandafile/file.h"
#include "libpandabase/macros.h"

#include "ecmascript/base/file_header.h"
#include "ecmascript/compiler/aot_file/aot_version.h"
#include "ecmascript/pgo_profiler/pgo_context.h"

namespace panda::ecmascript::pgo {
class PGOProfilerHeader;

struct SectionInfo {
    uint32_t offset_ {0};
    // reserve
    uint32_t size_ {0};
    uint32_t number_ {0};
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
  |--------PROTO_TRANSITION_POOL(12)
  |------------{ offset, size (reserve), protoTransitionPoolCount }
  |
  |----Section1: PGOPandaFileInfos(number1)
  |--------[{ size, CHECK_SUM }, { size, CHECK_SUM }, ...]
  |
  |----Section2: PGORecordDetailInfos(number2)
  |--------[ PGOMethodInfoMap(number4)
  |------------{ recordId, offset, size, number4 }
  |------------[ PGOMethodInfo(size1)
  |----------------{ size1, entityId, count, mode, methodName,
  |-------------------- [{ size, offset, profileTypeId }, { size, offset, profileTypeId }, ...]},
  |------------  PGOMethodInfo(size1)
  |----------------{ size1, entityId, count, mode, methodName,
  |-------------------- [{ size, offset, profileTypeId }, { size, offset, profileTypeId }, ...]},
  |--------------... ]
  |--------  PGOMethodInfoMap()
  |--------... ]
  |
  |----Section3: PGHClassLayoutDescs(number3)
  |--------{ offset, size, number5 }
  |--------[ PGOHClassLayoutDescInner(size)
  |------------{ size, profileTypeId, superProfileTypeId, count, ptCount, ctorCount,
  |---------------- [{ size, handle, key }, { size, heandle, key }, ...]}
  |--------  PGOHClassLayoutDescInner(size)
  |------------{ size, profileTypeId, superProfileTypeId, count, ptCount, ctorCount,
  |---------------- [{ size, handle, key }, { size, heandle, key }, ...]}
  |
  |----Section4: PGORecord(recordPoolCount)
  |--------{ offset, size, recordItemCount }
  |--------[ recordId, recordName ](recordItemCount)
  |
  |----Section5: PGOProtoTransitionTypes(protoTransitionPoolCount)
  |--------{ offset, size, protoTransitionItemCount }
  |--------[ PGOProtoTransitionType ](protoTransitionItemCount)
  |
  |----Section6: ProfileTypes(ProfileTypePoolCount)
  |--------{ offset, size, profileTypeItemCount }
  |--------[ profileTypeId, profileType(64bit) ](profileTypeItemCount)
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
    static constexpr VersionType WIDE_CLASS_TYPE_MINI_VERSION = {0, 0, 0, 10};
    static constexpr VersionType PROFILE_TYPE_WITH_ABC_ID_MINI_VERSION = {0, 0, 0, 11};
    static constexpr VersionType ELEMENTS_TRACK_INFO_MINI_VERSION = {0, 0, 0, 12};
    static constexpr VersionType PROTO_TRANSITION_POOL_MINI_VERSION = {0, 0, 0, 14};
    static constexpr VersionType MULTI_ABC_CHECKSUM_MINI_VERSION = {0, 0, 0, 15};
    static constexpr VersionType FILE_SIZE_MINI_VERSION = FILE_CONSISTENCY_MINI_VERSION;
    static constexpr VersionType HEADER_SIZE_MINI_VERSION = FILE_CONSISTENCY_MINI_VERSION;
    static constexpr VersionType ELASTIC_HEADER_MINI_VERSION = FILE_CONSISTENCY_MINI_VERSION;
    static constexpr VersionType LAST_VERSION = {0, 0, 0, 15};
    static constexpr size_t PANDA_FILE_SECTION_INDEX = 0;
    static constexpr size_t RECORD_INFO_SECTION_INDEX = 1;
    static constexpr size_t LAYOUT_DESC_SECTION_INDEX = 2;
    static constexpr size_t RECORD_POOL_SECTION_INDEX = 3;
    static constexpr size_t PROTO_TRANSITION_POOL_SECTION_INDEX = 4;
    static constexpr size_t CLASS_TYPE_POOL_SECTION_INDEX = 5;
    static constexpr size_t ABC_FILE_POOL_SECTION_INDEX = 6;
    static constexpr size_t SECTION_SIZE = 7;

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
        ASSERT(sectionNumber > 0);
        return sizeof(PGOProfilerHeader) + (sectionNumber - 1) * sizeof(SectionInfo);
    }

    size_t Size() const
    {
        return Size(sectionNumber_);
    }

    bool Verify() const
    {
        return VerifyVersion("apPath file", LAST_VERSION, IsStrictMatch());
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
        void* rawMemory = malloc(size);
        if (rawMemory == nullptr) {
            LOG_ECMA(FATAL) << "Failed to apply for memory, size :" << size;
            UNREACHABLE();
        }
        *header = reinterpret_cast<PGOProfilerHeader *>(rawMemory);
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

    SectionInfo *GetProfileTypeSection() const
    {
        return GetSectionInfo(CLASS_TYPE_POOL_SECTION_INDEX);
    }

    SectionInfo *GetAbcFilePoolSection() const
    {
        return GetSectionInfo(ABC_FILE_POOL_SECTION_INDEX);
    }

    SectionInfo *GetProtoTransitionPoolSection() const
    {
        return GetSectionInfo(PROTO_TRANSITION_POOL_SECTION_INDEX);
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

    bool SupportProtoTransitionPool() const
    {
        return CompatibleVerify(PROTO_TRANSITION_POOL_MINI_VERSION);
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

    bool SupportWideProfileType() const
    {
        return CompatibleVerify(WIDE_CLASS_TYPE_MINI_VERSION);
    }

    bool SupportProfileTypeWithAbcId() const
    {
        return CompatibleVerify(PROFILE_TYPE_WITH_ABC_ID_MINI_VERSION);
    }

    bool SupportElementsTrackInfo() const
    {
        return CompatibleVerify(ELEMENTS_TRACK_INFO_MINI_VERSION);
    }

    bool SupportMultiAbcChecksum() const
    {
        return CompatibleVerify(MULTI_ABC_CHECKSUM_MINI_VERSION);
    }

    bool IsCompatibleWithAOTFile() const
    {
        return VerifyVersion("ap file compatible aot file", GetCompatibleAnVersion(), AOTFileVersion::AN_VERSION, true);
    }

    static bool IsStrictMatch()
    {
        return strictMatch_;
    }

    static bool IschecksumListHasAbcId()
    {
        return checksumListHasAbcId_;
    }

    // just for test
    static void SetStrictMatch(bool strictMatch)
    {
        strictMatch_ = strictMatch;
    }

    // just for test
    static void SetchecksumListHasAbcId(bool checksumListHasAbcId)
    {
        checksumListHasAbcId_ = checksumListHasAbcId;
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
    static bool strictMatch_;
    static bool checksumListHasAbcId_;
};

class PGOProfilerHeaderLegacy : public base::FileHeaderBase {
public:
    static constexpr size_t SECTION_SIZE = 3;
    static constexpr VersionType LAST_VERSION = {0, 0, 0, 5};
    PGOProfilerHeaderLegacy() : base::FileHeaderBase(LAST_VERSION), sectionNumber_(SECTION_SIZE) {};

    const uint32_t &GetSectionNumber() const
    {
        return sectionNumber_;
    }

private:
    uint32_t sectionNumber_ {SECTION_SIZE};
    SectionInfo sectionInfos_;
};

class PGOFileDataInterface {
public:
    PGOFileDataInterface() = default;
    virtual ~PGOFileDataInterface() = default;
    virtual uint32_t ProcessToBinary(PGOContext &context, std::fstream &stream) = 0;
    virtual uint32_t ParseFromBinary(PGOContext &context, void **buffer, PGOProfilerHeader const *header) = 0;
    virtual bool ProcessToText(std::ofstream &stream) = 0;
    // not support yet
    virtual bool ParseFromText([[maybe_unused]] std::ifstream &stream)
    {
        return true;
    };

private:
    NO_COPY_SEMANTIC(PGOFileDataInterface);
    NO_MOVE_SEMANTIC(PGOFileDataInterface);
};

class PGOFileSectionInterface : public PGOFileDataInterface {
public:
    PGOFileSectionInterface() = default;
    ~PGOFileSectionInterface() override = default;
    virtual bool Support(PGOProfilerHeader const *header) const = 0;
    virtual SectionInfo *GetSection(PGOProfilerHeader const *header) const = 0;

    static bool ParseSectionFromBinary(PGOContext &context, void *buffer, PGOProfilerHeader const *header,
                                       PGOFileSectionInterface &section)
    {
        if (section.Support(header)) {
            SectionInfo *info = section.GetSection(header);
            if (info == nullptr) {
                return false;
            }
            void *addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + info->offset_);
            section.ParseFromBinary(context, &addr, header);
        }
        return true;
    }

    static bool ProcessSectionToBinary(PGOContext &context, std::fstream &fileStream, PGOProfilerHeader *const header,
                                       PGOFileSectionInterface &section)
    {
        auto *info = section.GetSection(header);
        if (info == nullptr) {
            return false;
        }
        info->offset_ = static_cast<uint32_t>(fileStream.tellp());
        info->number_ = section.ProcessToBinary(context, fileStream);
        info->size_ = static_cast<uint32_t>(fileStream.tellp()) - info->offset_;
        return true;
    }

private:
    NO_COPY_SEMANTIC(PGOFileSectionInterface);
    NO_MOVE_SEMANTIC(PGOFileSectionInterface);
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_AP_FILE_BASE_INFO_H