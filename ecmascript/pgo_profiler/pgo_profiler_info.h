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

#include <memory>
#include <sstream>
#include <string.h>

#include "ecmascript/base/file_header.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/pgo_profiler/pgo_profiler_type.h"
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript {
class SaveTask;

enum class SampleMode : uint8_t {
    HOTNESS_MODE,
    CALL_MODE,
};

struct SectionInfo {
    uint32_t offset_ {0};
    // reserve
    uint32_t size_ {0};
    uint32_t number_ {0};
};
static constexpr size_t ALIGN_SIZE = 4;

/**
 * |----PGOProfilerHeader
 * |--------MAGIC
 * |--------VERSION
 * |--------SECTION_NUMBER
 * |--------PANDA_FILE_INFO_SECTION_INFO
 * |------------offset
 * |------------size (reserve)
 * |------------number
 * |--------RECORD_INFO_SECTION_INFO
 * |------------offset
 * |------------size (reserve)
 * |------------number
 * |----PGOPandaFileInfos
 * |--------SIZE
 * |--------CHECK_SUM
 * |--------...
 * |----PGORecordDetailInfos
 * |--------PGOMethodInfoMap
 * |------------PGOMethodInfo
 * |----------------size
 * |----------------id
 * |----------------count
 * |----------------mode
 * |----------------methodName
 * |----------------...
 * |------------PGOMethodTypeSet
 * |----------------PGOMethodTypeInfo
 * |--------------------size
 * |--------------------offset
 * |--------------------type
 * |----------------...
 * |----------------PGOMethodLayoutDescInfos
 * |--------------------size
 * |--------------------offset
 * |--------------------type
 * |--------------------count
 * |--------------------PGOLayoutDescInfo
 * |------------------------size
 * |------------------------type
 * |------------------------key
 * |--------------------...
 * |----------------...
 * |----PGORecordSimpleInfos
 * |--------PGOMethodIdSet
 * |------------id
 * |------------...
 */
class PGOProfilerHeader : public base::FileHeader {
public:
    static constexpr VersionType TYPE_MINI_VERSION = {0, 0, 0, 2};
    static constexpr std::array<uint8_t, VERSION_SIZE> LAST_VERSION = {0, 0, 0, 3};
    static constexpr size_t SECTION_SIZE = 2;
    static constexpr size_t PANDA_FILE_SECTION_INDEX = 0;
    static constexpr size_t RECORD_INFO_SECTION_INDEX = 1;

    PGOProfilerHeader() : base::FileHeader(LAST_VERSION), sectionNumber_(SECTION_SIZE)
    {
        GetPandaInfoSection()->offset_ = Size();
    }

    static size_t LastSize()
    {
        return sizeof(PGOProfilerHeader) + (SECTION_SIZE - 1) * sizeof(SectionInfo);
    }

    size_t Size() const
    {
        return sizeof(PGOProfilerHeader) + (sectionNumber_ - 1) * sizeof(SectionInfo);
    }

    bool Verify() const
    {
        return InternalVerify("apPath file", LAST_VERSION, false);
    }

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
    static bool ParseFromBinary(void *buffer, PGOProfilerHeader **header);
    void ProcessToBinary(std::ofstream &fileStream) const;

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

    bool SupportType() const
    {
        return InternalVerifyVersion(TYPE_MINI_VERSION);
    }

    NO_COPY_SEMANTIC(PGOProfilerHeader);
    NO_MOVE_SEMANTIC(PGOProfilerHeader);

private:
    SectionInfo *GetSectionInfo(size_t index) const
    {
        if (index >= SECTION_SIZE) {
            return nullptr;
        }
        return const_cast<SectionInfo *>(&sectionInfos_) + index;
    }

    uint32_t sectionNumber_ {SECTION_SIZE};
    SectionInfo sectionInfos_;
};

class PGOPandaFileInfos {
public:
    void Sample(uint32_t checksum)
    {
        pandaFileInfos_.insert(checksum);
    }

    void Clear()
    {
        pandaFileInfos_.clear();
    }

    void ParseFromBinary(void *buffer, SectionInfo *const info);
    void ProcessToBinary(std::ofstream &fileStream, SectionInfo *info) const;

    void ProcessToText(std::ofstream &stream) const;
    bool ParseFromText(std::ifstream &stream);

    bool CheckSum(uint32_t checksum) const;

private:
    class PandaFileInfo {
    public:
        PandaFileInfo() = default;
        PandaFileInfo(uint32_t checksum) : size_(LastSize()), checksum_(checksum) {}

        static size_t LastSize()
        {
            return sizeof(PandaFileInfo);
        }

        size_t Size()
        {
            return static_cast<size_t>(size_);
        }

        bool operator<(const PandaFileInfo &right) const
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

    std::set<PandaFileInfo> pandaFileInfos_;
};

class PGOMethodInfo {
public:
    static constexpr int METHOD_INFO_COUNT = 4;
    static constexpr int METHOD_ID_INDEX = 0;
    static constexpr int METHOD_COUNT_INDEX = 1;
    static constexpr int METHOD_MODE_INDEX = 2;
    static constexpr int METHOD_NAME_INDEX = 3;

    explicit PGOMethodInfo(EntityId id) : id_(id) {}

    PGOMethodInfo(EntityId id, uint32_t count, SampleMode mode, const char *methodName)
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

    static int32_t Size(uint32_t length)
    {
        return sizeof(PGOMethodInfo) + AlignUp(length, ALIGN_SIZE);
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

    void IncreaseCount()
    {
        count_++;
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
        count_ += info->GetCount();
        SetSampleMode(info->GetSampleMode());
    }

    EntityId GetMethodId() const
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
    EntityId id_;
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

    void Clear(NativeAreaAllocator *allocator)
    {
        for (auto iter : layoutDescInfos_) {
            allocator->Delete(iter);
        }
        layoutDescInfos_.clear();
    }

    void AddType(uint32_t offset, PGOSampleType type)
    {
        auto result = typeInfoSet_.find(PGOMethodTypeInfo(offset));
        if (result != typeInfoSet_.end()) {
            if (result->GetType() != type) {
                typeInfoSet_.erase(result);
                typeInfoSet_.emplace(offset, type);
            }
        } else {
            typeInfoSet_.emplace(offset, type);
        }
    }

    void AddDefine(uint32_t offset, PGOSampleType type)
    {
        auto result = defineTypeSet_.find(PGOMethodTypeInfo(offset));
        if (result != defineTypeSet_.end()) {
            defineTypeSet_.erase(result);
        }
        defineTypeSet_.emplace(offset, type);
    }

    template <typename Callback>
    void GetTypeInfo(Callback callback)
    {
        for (auto typeInfo : typeInfoSet_) {
            auto type = typeInfo.GetType();
            callback(typeInfo.GetOffset(), &type);
        }
        for (auto &descInfo : layoutDescInfos_) {
            descInfo->GetTypeInfo(callback);
        }
    }

    void Merge(NativeAreaAllocator *allocator, const PGOMethodTypeSet *info,
        const CMap<PGOSampleType, CMap<CString, TrackType>> &layoutDescs);
    static void SkipFromBinary(void **buffer);

    template <typename Callback>
    bool ParseFromBinary(void **buffer, Callback callback)
    {
        uint32_t size = base::ReadBuffer<uint32_t>(buffer, sizeof(uint32_t));
        for (uint32_t i = 0; i < size; i++) {
            auto typeInfo = base::ReadBufferInSize<PGOMethodInfoHeader>(buffer);
            if (typeInfo->GetInfoType() == InfoType::METHOD_TYPE_INFO_TYPE) {
                typeInfoSet_.emplace(*reinterpret_cast<PGOMethodTypeInfo *>(typeInfo));
            } else if (typeInfo->GetInfoType() == InfoType::HCLSS_DESC_INFO_TYPE) {
                void *addr = callback(typeInfo->Size());
                if (memcpy_s(addr, typeInfo->Size(), typeInfo, typeInfo->Size()) != EOK) {
                    UNREACHABLE();
                }
                layoutDescInfos_.emplace(reinterpret_cast<PGOMethodLayoutDescInfos *>(addr));
            }
        }
        return true;
    }
    bool ProcessToBinary(std::stringstream &stream) const;

    bool ParseFromText(const std::string &typeString);
    void ProcessToText(std::string &text) const;

    NO_COPY_SEMANTIC(PGOMethodTypeSet);
    NO_MOVE_SEMANTIC(PGOMethodTypeSet);

private:
    enum class InfoType : uint8_t { NONE, METHOD_TYPE_INFO_TYPE, HCLSS_DESC_INFO_TYPE };

    class PGOMethodInfoHeader {
    public:
        PGOMethodInfoHeader(InfoType type, uint32_t offset) : type_(type), offset_(offset) {}
        PGOMethodInfoHeader(uint32_t size, InfoType type, uint32_t offset)
            : size_(size), type_(type), offset_(offset) {}

        InfoType GetInfoType()
        {
            return type_;
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
        InfoType type_ {InfoType::NONE};
        uint32_t offset_ {0};
    };

    class PGOMethodTypeInfo : public PGOMethodInfoHeader {
    public:
        explicit PGOMethodTypeInfo(uint32_t offset) : PGOMethodInfoHeader(InfoType::METHOD_TYPE_INFO_TYPE, offset) {}
        PGOMethodTypeInfo(uint32_t offset, PGOSampleType type)
            : PGOMethodInfoHeader(sizeof(PGOMethodTypeInfo), InfoType::METHOD_TYPE_INFO_TYPE, offset), type_(type) {}

        bool operator<(const PGOMethodTypeInfo &right) const
        {
            return offset_ < right.offset_;
        }

        PGOSampleType GetType() const
        {
            return type_;
        }

    protected:
        PGOMethodTypeInfo(uint32_t size, InfoType infoType, uint32_t offset, PGOSampleType type)
            : PGOMethodInfoHeader(size, infoType, offset), type_(type) {}

    private:
        PGOSampleType type_;
    };

    class PGOMethodLayoutDescInfos : public PGOMethodTypeInfo {
    public:
        class PGOLayoutDescInfo {
        public:
            PGOLayoutDescInfo() = default;
            PGOLayoutDescInfo(const CString &key, TrackType type) : type_(type)
            {
                size_t len = key.size();
                size_ = Size(len);
                if (len > 0 && memcpy_s(&key_, len, key.c_str(), len) != EOK) {
                    LOG_ECMA(ERROR) << "SetMethodName memcpy_s failed" << key << ", len = " << len;
                    UNREACHABLE();
                }
                *(&key_ + len) = '\0';
                type_ = type;
            }

            static int32_t Size(size_t len)
            {
                return sizeof(PGOLayoutDescInfo) + AlignUp(len, ALIGN_SIZE);
            }

            int32_t Size() const
            {
                return size_;
            }

            const char *GetKey() const
            {
                return &key_;
            }

            TrackType GetType() const
            {
                return type_;
            }

        private:
            int32_t size_ {0};
            TrackType type_ {TrackType::NONE};
            char key_ {'\0'};
        };

        explicit PGOMethodLayoutDescInfos(size_t size, uint32_t offset, PGOSampleType type)
            : PGOMethodTypeInfo(size, InfoType::HCLSS_DESC_INFO_TYPE, offset, type) {}

        static size_t CaculateSize(const CMap<CString, TrackType> &desc);
        void Merge(const CMap<CString, TrackType> &desc);

        template <typename Callback>
        void GetTypeInfo(Callback callback)
        {
            PGOSampleLayoutDesc desc(GetType().GetClassType());
            if (count_ <= 0) {
                return;
            }
            auto descInfo = GetFirst();
            for (int32_t i = 0; i < count_; i++) {
                desc.AddKeyAndDesc(descInfo->GetKey(), descInfo->GetType());
                descInfo = GetNext(descInfo);
            }
            callback(GetOffset(), &desc);
        }

        void ProcessToText(std::string &text) const;

    private:
        const PGOLayoutDescInfo *GetFirst() const
        {
            return &descInfos_;
        }

        const PGOLayoutDescInfo *GetNext(const PGOLayoutDescInfo *current) const
        {
            return reinterpret_cast<PGOLayoutDescInfo *>(reinterpret_cast<uintptr_t>(current) + current->Size());
        }

        int32_t count_ {0};
        PGOLayoutDescInfo descInfos_;
    };

    std::set<PGOMethodTypeInfo> typeInfoSet_;
    std::set<PGOMethodTypeInfo> defineTypeSet_;
    std::set<PGOMethodLayoutDescInfos *> layoutDescInfos_;
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

    bool AddMethod(Chunk *chunk, EntityId methodId, const CString &methodName, SampleMode mode);
    bool AddType(Chunk *chunk, EntityId methodId, int32_t offset, PGOSampleType type);
    bool AddDefine(Chunk *chunk, EntityId methodId, int32_t offset, PGOSampleType type);
    bool AddLayout(PGOSampleType type, JSTaggedType hclass);
    void Merge(Chunk *chunk, NativeAreaAllocator *allocator, PGOMethodInfoMap *methodInfos);

    bool ParseFromBinary(Chunk *chunk, uint32_t threshold, void **buffer, PGOProfilerHeader *const header);
    bool ProcessToBinary(uint32_t threshold, const CString &recordName, const SaveTask *task, std::ofstream &fileStream,
        PGOProfilerHeader *const header) const;

    bool ParseFromText(Chunk *chunk, uint32_t threshold, const std::vector<std::string> &content);
    void ProcessToText(uint32_t threshold, const CString &recordName, std::ofstream &stream) const;

    NO_COPY_SEMANTIC(PGOMethodInfoMap);
    NO_MOVE_SEMANTIC(PGOMethodInfoMap);

private:
    PGOMethodTypeSet *GetOrInsertMethodTypeSet(Chunk *chunk, EntityId methodId);

    CMap<EntityId, PGOMethodInfo *> methodInfos_;
    CMap<EntityId, PGOMethodTypeSet *> methodTypeInfos_;
    CMap<PGOSampleType, CMap<CString, TrackType>> globalLayoutDescInfos_;
};

class PGOMethodIdSet {
public:
    PGOMethodIdSet() = default;

    void Clear(NativeAreaAllocator *allocator)
    {
        methodIdSet_.clear();
        for (auto iter : methodTypeSet_) {
            allocator->Delete(iter.second);
        }
        methodTypeSet_.clear();
    }

    bool Match(EntityId methodId)
    {
        return methodIdSet_.find(methodId) != methodIdSet_.end();
    }

    template <typename Callback>
    bool Update(const CString &recordName, Callback callback)
    {
        std::unordered_set<EntityId> newIds = callback(recordName, methodIdSet_);
        if (!newIds.empty()) {
            methodIdSet_.insert(newIds.begin(), newIds.end());
            return true;
        }
        return false;
    }

    template <typename Callback>
    void GetTypeInfo(EntityId methodId, Callback callback)
    {
        auto iter = methodTypeSet_.find(methodId);
        if (iter != methodTypeSet_.end()) {
            iter->second->GetTypeInfo(callback);
        }
    }

    bool ParseFromBinary(
        NativeAreaAllocator *allocator, uint32_t threshold, void **buffer, PGOProfilerHeader *const header);

    NO_COPY_SEMANTIC(PGOMethodIdSet);
    NO_MOVE_SEMANTIC(PGOMethodIdSet);

private:
    std::unordered_set<EntityId> methodIdSet_;
    std::map<EntityId, PGOMethodTypeSet *> methodTypeSet_;
};

class PGORecordDetailInfos {
public:
    explicit PGORecordDetailInfos(uint32_t hotnessThreshold) : hotnessThreshold_(hotnessThreshold)
    {
        chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
    };

    ~PGORecordDetailInfos()
    {
        Clear();
    }

    void Clear()
    {
        for (auto iter : recordInfos_) {
            iter.second->Clear();
            nativeAreaAllocator_.Delete(iter.second);
        }
        recordInfos_.clear();
        chunk_ = std::make_unique<Chunk>(&nativeAreaAllocator_);
    }

    // If it is a new method, return true.
    bool AddMethod(const CString &recordName, EntityId methodId, const CString &methodName, SampleMode mode);
    bool AddType(const CString &recordName, EntityId methodId, int32_t offset, PGOSampleType type);
    bool AddDefine(const CString &recordName, EntityId methodId, int32_t offset, PGOSampleType type);
    bool AddLayout(const CString &recordName, PGOSampleType type, JSTaggedType hclass);
    void Merge(const PGORecordDetailInfos &recordInfos);

    void ParseFromBinary(void *buffer, PGOProfilerHeader *const header);
    void ProcessToBinary(const SaveTask *task, std::ofstream &fileStream, PGOProfilerHeader *const header) const;

    bool ParseFromText(std::ifstream &stream);
    void ProcessToText(std::ofstream &stream) const;

    NO_COPY_SEMANTIC(PGORecordDetailInfos);
    NO_MOVE_SEMANTIC(PGORecordDetailInfos);

private:
    PGOMethodInfoMap *GetMethodInfoMap(const CString &recordName);

    uint32_t hotnessThreshold_ {2};
    NativeAreaAllocator nativeAreaAllocator_;
    std::unique_ptr<Chunk> chunk_;
    CMap<CString, PGOMethodInfoMap *> recordInfos_;
};

class PGORecordSimpleInfos {
public:
    explicit PGORecordSimpleInfos(uint32_t threshold) : hotnessThreshold_(threshold) {}
    ~PGORecordSimpleInfos()
    {
        Clear();
    }

    void Clear()
    {
        for (auto iter : methodIds_) {
            iter.second->Clear(&nativeAreaAllocator_);
            nativeAreaAllocator_.Delete(iter.second);
        }
        methodIds_.clear();
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
            PGOMethodIdSet *methodIds = nativeAreaAllocator_.New<PGOMethodIdSet>();
            if (methodIds->Update(recordName, callback)) {
                methodIds_.emplace(recordName, methodIds);
            } else {
                nativeAreaAllocator_.Delete(methodIds);
            }
        }
    }

    template <typename Callback>
    void GetTypeInfo(const CString &recordName, EntityId methodId, Callback callback)
    {
        auto iter = methodIds_.find(recordName);
        if (iter != methodIds_.end()) {
            iter->second->GetTypeInfo(methodId, callback);
        }
    }

    void ParseFromBinary(void *buffer, PGOProfilerHeader *const header);

    NO_COPY_SEMANTIC(PGORecordSimpleInfos);
    NO_MOVE_SEMANTIC(PGORecordSimpleInfos);

private:
    uint32_t hotnessThreshold_ {2};
    NativeAreaAllocator nativeAreaAllocator_;
    CUnorderedMap<CString, PGOMethodIdSet *> methodIds_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_PGO_PROFILER_INFO_H
